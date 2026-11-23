#include "ffc_adapter.h"
#include "ffc_profile.h"
static void zero(void*p,size_t n){uint8_t*b=p;while(n--)*b++=0;}
static int bytes_same(const void*a,const void*b,size_t n){const uint8_t*x=a,*y=b;while(n--)if(*x++!=*y++)return 0;return 1;}
static int text(const char*a,const char*b){for(unsigned i=0;i<96;i++){if(a[i]!=b[i])return 0;if(!a[i])return 1;}return 0;}
static int auth(const struct ffc_authority*a,uint32_t required){unsigned nonce=0;for(unsigned i=0;i<16;i++)nonce|=a->nonce[i];return nonce&&(a->flags&required)==required&&a->pid&&a->owner&&a->power_generation&&a->scope_epoch&&a->mp_base&&a->obj_base&&!(a->mp_base&4095)&&!(a->obj_base&4095)&&a->mp_base<UINT32_MAX-0x40000u&&a->obj_base<UINT32_MAX-0x1300000u&&a->api_version==1&&a->api_bytes==36&&a->api_build==0x3110d015u;}
static int same_auth(const struct ffc_authority*a,const struct ffc_authority*b,uint32_t required){return auth(a,required)&&auth(b,required)&&a->pid==b->pid&&a->owner==b->owner&&a->power_generation==b->power_generation&&a->scope_epoch==b->scope_epoch&&a->mp_base==b->mp_base&&a->obj_base==b->obj_base&&bytes_same(a->nonce,b->nonce,16);}
static int rd(const struct ffc_projection_ports*p,uint32_t a,void*out,uint32_t n){return a&&n&&a<=UINT32_MAX-n&&p->read(p->context,a,out,n)==1;}
static int registry(const struct ffc_registry*r){
 if(!r->vm||r->configured_slots!=39||r->registry_singleton!=1||r->selected_count!=1||r->ffc_count!=2||r->top_id!=0xffc||r->top_index!=24||!r->top_object)return 0;
 for(unsigned b=0;b<2;b++){const struct ffc_slot*v=&r->banks[b];if(v->bank!=b||v->index!=24||v->id!=0xffc||v->object!=r->top_object||v->object>UINT32_MAX-0xb0||v->vptr!=v->object+0x44||v->registry_matches!=1||!text(v->module,"/usr/lib/liba7r_menu_product_v21.so")||!text(v->factory,"A7ProductViewFactory21"))return 0;}return 1;
}
int ffc_same_lease(const struct ffc_live*a,const struct ffc_live*b){return a&&b&&same_auth(&a->authority,&b->authority,FFC_SOURCE_AUTHORITY)&&a->vm==b->vm&&a->object==b->object&&a->vptr==b->vptr&&a->control==b->control&&a->serial==b->serial&&a->graphics==b->graphics;}
static int project(const struct ffc_projection_ports*p,struct ffc_live*out,uint32_t required){
 struct ffc_live f;struct ffc_authority after_auth;struct ffc_registry r,after_r;
 uint32_t report[FFC_REPORT_BYTES/4],view[44],page[24],gfx[26],labels[8],n,label_alias;
 if(!out)return FFC_BAD_API;zero(out,sizeof*out);zero(&f,sizeof f);zero(&r,sizeof r);zero(&after_r,sizeof after_r);zero(&after_auth,sizeof after_auth);
 if(!p||!p->authority||!p->query||!p->registry||!p->read||!p->rect)return FFC_BAD_API;
 if(p->authority(p->context,&f.authority)!=1||!auth(&f.authority,required))return FFC_BAD_SCOPE;
 if(p->query(p->context,report)!=0)return FFC_BUSY;
 if(report[0]!=1||report[1]!=FFC_REPORT_BYTES||report[2]!=2||report[3]||report[4]!=f.authority.pid||report[5]!=f.authority.owner||report[6]!=1||report[17]!=1||report[22]||!bytes_same((uint8_t*)report+1132,f.authority.nonce,16))return FFC_BAD_REPORT;
 if(report[46]!=1||report[47]!=f.authority.owner||!report[48]||report[51]!=1||report[52]||report[53]||report[59]!=1)return FFC_BAD_REPORT;
 if(report[121]!=1||report[122]!=6||report[123]||report[124]||report[125]||report[126]!=f.authority.owner||report[131]!=1||report[137]!=1||report[138]!=1)return FFC_BAD_REPORT;
 if(p->registry(p->context,&r)!=1||!registry(&r)||report[15]!=r.top_object||report[16]!=r.banks[0].vptr||report[25]!=f.authority.obj_base)return FFC_BAD_SCOPE;
 f.vm=r.vm;f.object=r.top_object;f.vptr=r.banks[0].vptr;
 if(!rd(p,f.object,view,sizeof view)||view[0]!=f.vptr||view[15]!=0||view[16]!=f.authority.obj_base+0x12054e0u)return FFC_BAD_LAYOUT;
 for(unsigned i=0;i<27;i++)if(view[17+i]!=f.authority.mp_base+ffc_method_offsets[i])return FFC_BAD_LAYOUT;
 f.graphics=view[12];f.control=view[13];f.serial=view[14];
 if(!f.control||!f.graphics||(f.control&3)||(f.graphics&3)||!f.serial||f.serial!=report[48]||!rd(p,f.control,page,sizeof page))return FFC_BAD_LAYOUT;
 if(page[6]!=1||page[7]!=f.authority.owner||page[8]!=f.serial||page[11]!=1||page[12]||page[13]||page[19]!=1||page[20]>5||page[21]>4||page[22]>1)return FFC_BAD_SCOPE;
 f.row=page[20];f.held=page[21];f.pending=page[22];
 if(f.graphics>UINT32_MAX-7104u||!rd(p,f.graphics+116,gfx,sizeof gfx)||!rd(p,f.graphics+6464,&label_alias,4)||!rd(p,f.graphics+6468,labels,sizeof labels)||!rd(p,f.graphics+7100,&n,4))return FFC_BAD_LAYOUT;
 if(gfx[0]!=1||gfx[1]!=6||gfx[2]||gfx[3]||gfx[4]||gfx[5]!=f.authority.owner||gfx[10]!=1||gfx[16]!=1||gfx[17]!=1||gfx[18]||n!=8||labels[0]!=label_alias||labels[0]!=gfx[14]||labels[0]!=report[135])return FFC_BAD_LAYOUT;
 for(unsigned i=0;i<8;i++){
  uint32_t vp;if(!labels[i]||(labels[i]&3)||!rd(p,labels[i],&vp,4)||vp!=f.authority.obj_base+0x11eab90u)return FFC_BAD_LAYOUT;
  for(unsigned j=0;j<i;j++)if(labels[i]==labels[j])return FFC_BAD_LAYOUT;
 }
 for(unsigned i=0;i<6;i++){
  struct ffc_rect*q=&f.rect[i];f.label[i]=labels[i+1];
  if(p->rect(p->context,f.label[i],&f.authority,q)!=1||q->left!=48||q->right!=592||q->top!=112+(int32_t)i*48||q->bottom!=160+(int32_t)i*48)return FFC_BAD_LAYOUT;
 }
 if(p->registry(p->context,&after_r)!=1||!registry(&after_r)||after_r.vm!=r.vm||after_r.top_object!=r.top_object||after_r.banks[0].vptr!=r.banks[0].vptr||p->authority(p->context,&after_auth)!=1||!same_auth(&f.authority,&after_auth,required))return FFC_BAD_SCOPE;
 /* Last source-projection read is still from freshly re-proved current object.
  * Source fields and power/owner may not change during this read-only lookup. */
 uint32_t tail[3],live[5];
 if(!rd(p,after_r.top_object+0x30,tail,sizeof tail)||tail[0]!=f.graphics||tail[1]!=f.control||tail[2]!=f.serial||!rd(p,f.control+0x4c,live,sizeof live)||live[0]!=1||live[1]!=f.row||live[2]!=f.held||live[3]!=f.pending)return FFC_BAD_SCOPE;
 *out=f;return FFC_OK;
}
int ffc_hit(const struct ffc_live*f,int32_t x,int32_t y){if(!f)return -1;for(int i=0;i<6;i++)if(x>=f->rect[i].left&&x<f->rect[i].right&&y>=f->rect[i].top&&y<f->rect[i].bottom)return i;return -1;}

int ffc_project(const struct ffc_projection_ports*p,struct ffc_live*out){return project(p,out,FFC_REQUIRED_AUTHORITY);}
int ffc_project_catalog(const struct ffc_projection_ports*p,struct ffc_live*out){return project(p,out,FFC_SOURCE_AUTHORITY);}
