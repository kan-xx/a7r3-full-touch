#include "menu_regions.h"
#include <stddef.h>
/* Same libc ABI, explicit declarations for the firmware -nostdlib build. */
extern void *memcpy(void*,const void*,size_t);
extern void *memset(void*,int,size_t);
extern int memcmp(const void*,const void*,size_t);
struct geometry {float x,y,w,h;};
static uint32_t u32(const unsigned char*p){uint32_t v;memcpy(&v,p,4);return v;}
static float f32(const unsigned char*p){float v;memcpy(&v,p,4);return v;}
static int finite(float x){return x==x&&x>-1000000.0f&&x<1000000.0f;}
static float min(float a,float b){return a<b?a:b;}
static float max(float a,float b){return a>b?a:b;}
static int matching(const struct ns_catalog*c,const struct mtr_map*m){return c&&m&&m->valid&&!c->status&&c->count==m->count&&c->count<=NS_TARGETS&&surface_identity_equal(&c->identity,&m->identity)&&!memcmp(c->targets,m->source,c->count*sizeof*c->targets);}
static int body_valid(const unsigned char*b,const struct surface_target*t){return u32(b)==t->vptr&&u32(b+0x24)==t->runtime_id&&b[0x21]<=3&&b[0x21]!=1&&b[0x21]!=3&&(u32(b+0x38)&2);}
static int geometry(const struct ns_io*p,const struct surface_target*t,struct geometry*g){
 unsigned char b[0x138];struct surface_point zero={0,0};
 if(!p->read(p->context,t->node,b,0x40)||!body_valid(b,t))return 0;
 /* isHit is an original geometry/cache query, not a synthetic mouse event. */
 p->hit(p->context,t->node,&zero);
 if(!p->read(p->context,t->node,b,sizeof b)||!body_valid(b,t))return 0;
 if(f32(b+0x64)!=1||f32(b+0x68)!=1)return 0;
 for(unsigned i=0;i<8;i++)if(b[0x130+i])return 0;
 *g=(struct geometry){f32(b+0x90),f32(b+0x94),f32(b+0x98),f32(b+0x9c)};
 return finite(g->x)&&finite(g->y)&&finite(g->w)&&finite(g->h)&&g->w>0&&g->h>0&&g->x>=0&&g->y>=0&&g->x+g->w<=640&&g->y+g->h<=480&&f32(b+0x5c)==g->x&&f32(b+0x60)==g->y;
}
static int box(const struct geometry*g,float x,float y,float w,float h){return g->x==x&&g->y==y&&g->w==w&&g->h==h;}
static int fail(struct mtr_map*m,uint32_t why){m->valid=0;m->error=why;return 0;}
int mtr_build(const struct ns_io*p,const struct ns_catalog*c,uint32_t pw,uint32_t ph,struct mtr_map*m){
 if(!m)return 0;memset(m,0,sizeof*m);m->error=MTR_ARGUMENT;
 if(!p||!p->read||!p->hit||!c||c->status||!c->count||c->count>NS_TARGETS)return 0;
 const struct surface_identity*s=&c->identity;
 if(pw!=640||ph!=480||s->view_id!=0xaa||s->kind!=SURFACE_MENU||s->layout!=1||s->phase!=1||!s->power||!s->view||!s->root||!s->runtime_id||s->popup||s->popup_node||s->first||c->tab_count<1||c->tab_count>7||c->page_count<1||c->page_count>15||c->row_count<0||c->row_count>6||s->tab<1||s->tab>c->tab_count||s->page<1||s->page>c->page_count)return fail(m,MTR_SCOPE);
 unsigned char root[0x40];if(!p->read(p->context,s->root,root,sizeof root)||root[0x21]!=2||u32(root+0x24)!=s->runtime_id||!(u32(root+0x38)&2))return fail(m,MTR_IDENTITY);
 struct geometry geom[NS_TARGETS];unsigned tabs[7]={0},pages[15]={0},back=0,prev=0,next=0;
 for(unsigned i=0;i<c->count;i++){
  const struct surface_target*t=c->targets+i;struct geometry*g=geom+i;
  if(t->flags!=1||!t->node||!t->vptr||!t->runtime_id||!geometry(p,t,g))return fail(m,MTR_GEOMETRY);
  switch(t->kind){
  case TARGET_TAB:if(t->index>=(uint32_t)c->tab_count||++tabs[t->index]>2||!box(g,17+86*(float)t->index,0,91,66))return fail(m,MTR_FAMILY);break;
  case TARGET_PAGE:if(t->index>=(uint32_t)c->page_count||++pages[t->index]>1||!box(g,314-9*(float)(c->page_count-1)+18*(float)t->index,446,12,14))return fail(m,MTR_FAMILY);break;
  case TARGET_PAGE_PREV:if(t->index||++prev>1||g->x>32||!box(g,g->x,93,8,16))return fail(m,MTR_FAMILY);break;
  case TARGET_PAGE_NEXT:if(t->index||++next>1||g->x<600||!box(g,g->x,93,8,16))return fail(m,MTR_FAMILY);break;
  case TARGET_ROW:if(t->index>=(uint32_t)c->row_count||!box(g,17,126+50*(float)t->index,606,48))return fail(m,MTR_FAMILY);break;
  case TARGET_BACK:if(t->index||++back>1||!box(g,15,434,610,40))return fail(m,MTR_FAMILY);break;
  default:return fail(m,MTR_FAMILY);
  }
 }
 if(back!=1)return fail(m,MTR_FAMILY);
 for(int i=0;i<c->tab_count;i++)if(!tabs[i])return fail(m,MTR_FAMILY);
 for(int i=0;i<c->page_count;i++)if(!pages[i])return fail(m,MTR_FAMILY);
 m->identity=*s;m->count=c->count;memcpy(m->source,c->targets,c->count*sizeof*c->targets);
 for(unsigned i=0;i<c->count;i++){
  const struct surface_target*t=c->targets+i;const struct geometry*g=geom+i;struct mtr_rect*r=m->region+i;
  float cx=g->x+g->w/2,cy=g->y+g->h/2;
  if(t->kind==TARGET_TAB){
   /* Native91px glyphs at86px spacing overlap5px. Split exactly halfway
    * between native centers, so every coordinate has at most one category. */
   r->left=t->index?cx-43:g->x;r->right=t->index+1<(uint32_t)c->tab_count?cx+43:g->x+g->w;r->top=g->y;r->bottom=g->y+g->h;r->enabled=1;
  }else if(t->kind==TARGET_PAGE){
   /* Interior18px spacing makes44px disjoint width mathematically impossible
    * while preserving each native glyph. Increase height14->48, outer ends24. */
   r->left=t->index?cx-9:cx-24;r->right=t->index+1<(uint32_t)c->page_count?cx+9:cx+24;r->top=cy-24;r->bottom=cy+24;r->enabled=1;
  }else if(t->kind==TARGET_PAGE_PREV||t->kind==TARGET_PAGE_NEXT){
   r->left=max(0,cx-24);r->right=min(640,cx+24);r->top=max(66,cy-24);r->bottom=min(126,cy+24);r->enabled=1;
  }
  if(r->enabled&&(r->left<0||r->right>640||r->top<0||r->bottom>480||r->left>=r->right||r->top>=r->bottom))return fail(m,MTR_GEOMETRY);
  /* Footer dots stay left of BACK's actual native-local right80px region. */
  if(t->kind==TARGET_PAGE&&(r->left<15||r->right>545||r->top<424))return fail(m,MTR_GEOMETRY);
 }
 m->error=MTR_OK;m->valid=1;return 1;
}
int mtr_hit(void*context,const struct surface_target*t,const struct surface_point*p){
 struct mtr_context*x=context;if(!x||!x->io||!x->catalog||!t||!p)return 0;
 struct ns_hit_context native={x->io,x->catalog};
 if(!matching(x->catalog,x->map))return ns_hit(&native,t,p);
 for(unsigned i=0;i<x->map->count;i++)if(!memcmp(t,x->map->source+i,sizeof*t)){
  const struct mtr_rect*r=x->map->region+i;
  if(r->enabled)return finite(p->x)&&finite(p->y)&&p->x>=r->left&&p->x<r->right&&p->y>=r->top&&p->y<r->bottom;
  return ns_hit(&native,t,p);
 }
 /* Never create a semantic target absent from this exact current catalog. */
 return 0;
}
