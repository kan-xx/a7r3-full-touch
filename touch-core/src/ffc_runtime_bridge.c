#include "ffc_runtime_bridge.h"
static int eq(const void*a,const void*b,unsigned n){const unsigned char*x=a,*y=b;unsigned d=0;for(unsigned i=0;i<n;i++)d|=x[i]^y[i];return !d;}
static void zero(void*p,unsigned n){unsigned char*b=p;while(n--)*b++=0;}
static int target(const struct ffc_runtime_bridge*b,const struct surface_target*t){return b&&b->current_valid&&t&&t->kind==TARGET_FFC_ROW&&t->index<6&&t->node==b->current.label[t->index]&&t->vptr==b->current.authority.obj_base+0x11eab90u&&t->runtime_id==b->current.serial&&t->flags==1;}
static struct surface_identity identity(const struct ffc_runtime_bridge*b){const struct ffc_live*f=&b->current;struct surface_identity s={f->authority.power_generation,0xffc,f->object,f->control,f->serial,SURFACE_FFC,0,1,0,0,0,0,b->source_generation};return s;}
int ffr_catalog(struct ffc_runtime_bridge*b,const struct ffc_projection_ports*p,struct ns_catalog*out){
 if(!b||!out)return 0;zero(out,sizeof*out);out->status=1;struct ffc_live fresh;
 if(ffc_project_catalog(p,&fresh)!=FFC_OK){b->current_valid=0;return 0;}
 if(!b->current_valid||!ffc_same_lease(&b->current,&fresh)||!eq(b->current.label,fresh.label,sizeof fresh.label)||!eq(b->current.rect,fresh.rect,sizeof fresh.rect)){
  if(!++b->source_generation)++b->source_generation;
 }
 b->current=fresh;b->current_valid=1;b->catalogs++;out->identity=identity(b);out->count=6;out->row_count=6;
 for(unsigned i=0;i<6;i++)out->targets[i]=(struct surface_target){TARGET_FFC_ROW,i,fresh.label[i],fresh.authority.obj_base+0x11eab90u,fresh.serial,1};
 out->status=0;return 1;
}
int ffr_hit(const struct ffc_runtime_bridge*b,const struct surface_target*t,const struct surface_point*p){
 if(!target(b,t)||!p||p->x!=p->x||p->y!=p->y||p->x<0||p->x>639||p->y<0||p->y>479)return 0;
 const struct ffc_rect*r=&b->current.rect[t->index];return p->x>=r->left&&p->x<r->right&&p->y>=r->top&&p->y<r->bottom;
}
void ffr_revoke(struct ffc_runtime_bridge*b){if(b){b->sealed_valid=0;b->executing=0;}}
int ffr_seal(struct ffc_runtime_bridge*b,const struct touch_envelope*e){
 if(!b||!e)return 0;ffr_revoke(b);if(e->surface.kind!=SURFACE_FFC)return 1;
 struct surface_identity s=identity(b);if(e->policy!=ENV_REAL_TAP||!b->current_valid||!surface_identity_equal(&s,&e->surface)||!target(b,&e->target)||!e->sequence||!e->post_commit_serial)return 0;
 b->sealed=b->current;b->expected=*e;b->sealed_valid=1;b->seals++;return 1;
}
int ffr_raw_boundary(const struct ffc_runtime_bridge*b,const struct ffc_live*f){return b&&b->executing&&b->sealed_valid&&f&&ffc_same_lease(&b->sealed,f)&&(f->authority.flags&FFC_REQUIRED_AUTHORITY)==FFC_REQUIRED_AUTHORITY;}
int ffr_apply(struct ffc_runtime_bridge*b,const struct ffc_action_ports*p,const struct touch_envelope*e,int activate){
 if(!b||!p||!e||!activate||e->policy!=ENV_REAL_TAP||!b->sealed_valid||b->executing||b->uncertain||!eq(e,&b->expected,sizeof*e)){if(b)b->rejected++;return 0;}
 struct ffc_live fresh;int rc=ffc_project(&p->projection,&fresh);
 if(rc||!ffc_same_lease(&b->sealed,&fresh)||!target(b,&e->target)||!ffc_same_lease(&b->current,&fresh)){b->rejected++;return 0;}
 int x0=(int16_t)e->origin_xy,y0=(int16_t)(e->origin_xy>>16),x1=(int16_t)e->end_xy,y1=(int16_t)(e->end_xy>>16);
 if(ffc_hit(&fresh,x0,y0)!=(int)e->target.index||ffc_hit(&fresh,x1,y1)!=(int)e->target.index){b->rejected++;return 0;}
 /* A known normal press can be cancelled on a later separately qualified UI
  * invocation. Never replay its release into a recovered power activation. */
 if(b->cancel.valid&&fresh.held){
  struct ffc_result r;rc=ffc_cancel_known(p,&b->cancel,&r);b->result=r;
  if(rc==FFC_CANCEL_SUBMITTED)b->cancel.valid=0;
  if(rc==FFC_NATIVE_UNCERTAIN)b->uncertain=1;
  b->rejected++;return 0;
 }
 if(fresh.held||fresh.pending){b->rejected++;return 0;}
 b->cancel.valid=0;b->executing=1;b->actions++;
 rc=ffc_click(p,x1,y1,&b->result);b->executing=0;b->cancel=b->result.cancel;
 if(rc==FFC_NATIVE_UNCERTAIN||b->result.native_uncertain){b->uncertain=1;b->cancel.valid=0;}
 /* ffc_click guarantees CENTER release is the last source use. Do not refresh
  * current/catalog or inspect a borrowed view/control after this call. */
 return rc==FFC_SUBMITTED;
}
