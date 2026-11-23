#include "image_reference_read.h"
struct observation {
 uint32_t model_vptr,complete_vptr,stream_vptr,view_vptr,view_phase;
 uint32_t model_phase,width,height,file_size,raw_type;
 uint8_t busy[2],directory[10],filename[14],pad[2];
};
static void zero(void*p,unsigned n){uint8_t*x=p;while(n--)*x++=0;}
static int equal(const void*a,const void*b,unsigned n){const uint8_t*x=a,*y=b;unsigned d=0;while(n--)d|=*x++^*y++;return !d;}
static int bounded_name(uint8_t*out,const uint8_t*raw,unsigned bytes){unsigned i=0;if(!raw[0])return 0;for(;i<bytes;i++){if(!raw[i])return 1;out[i]=raw[i];}return 0;}
static int collect(const struct pi_read_port*p,const struct pi_source*s,struct observation*o){
 zero(o,sizeof*o);
#define RD(a,d,n)do{if(!p->read(p->context,(a),(d),(n)))return 0;}while(0)
 RD(s->e9_model,&o->model_vptr,4);RD(s->e9_complete,&o->complete_vptr,4);
 RD(s->e9_complete+0x11c,&o->stream_vptr,4);RD(s->view_object,&o->view_vptr,4);
 RD(s->view_object+4,&o->view_phase,4);RD(s->e9_complete+0x20,&o->model_phase,4);
 RD(s->e9_complete+0x88,o->busy,2);RD(s->e9_complete+0x194,&o->width,8);RD(s->e9_complete+0x1bc,&o->file_size,4);
 RD(s->e9_complete+0x184,&o->raw_type,4);RD(s->e9_complete+0x1cc,o->directory,10);
 RD(s->e9_complete+0x1d6,o->filename,14);
#undef RD
 return 1;
}
int pi_read_reference_detailed(const struct pi_read_port*p,const struct pi_source*s,struct pi_key*out,uint32_t*flags,struct pi_read_detail*detail){
 if(detail)zero(detail,sizeof *detail);
 if(out)zero(out,sizeof*out);if(flags)*flags=0;
 if(!p||!p->read||!s||!out||!flags||(s->flags&7)!=7||!s->obj_base||(s->obj_base&4095)||s->obj_base>0xffffffffu-0x11ff000||!s->view3_base||(s->view3_base&4095)||s->view3_base>0xffffffffu-0xa8f4c||!s->e9_element||!s->e9_complete||s->e9_complete>0xffffffffu-0x8a8||s->e9_model!=s->e9_complete+0x8a4||!s->view_object||s->view_object>0xfffffff7u||!s->packed_power||!s->policy_epoch||!s->view_epoch)return PIR_ARGUMENT;
 struct observation a,b;
 if(!collect(p,s,&a)||!collect(p,s,&b))return PIR_READ;
 if(!equal(&a,&b,sizeof a))return PIR_CHANGED;
 uint32_t mismatch=(a.model_vptr!=s->obj_base+0x11fee30?1u:0u)|(a.complete_vptr!=s->obj_base+0x11fecf8?2u:0u)|(a.stream_vptr!=s->obj_base+0xdc1f08?4u:0u)|(a.view_vptr!=s->view3_base+0xa8f48?8u:0u)|(!pi_raw_type_supported(a.raw_type)?16u:0u)|((a.view_phase!=3&&a.view_phase!=4)?32u:0u);
 if(detail)*detail=(struct pi_read_detail){0x80000000u|mismatch,a.raw_type,a.view_phase};
 if(mismatch)return PIR_TYPED;
 if(a.model_phase==2)return PIR_BUSY;
 struct pi_key key={s->packed_power,s->observed_media_epoch,s->policy_epoch,s->obj_base,s->e9_element,s->e9_model,s->e9_complete,PI_REF_STREAM_FILE,a.width,a.height,a.file_size,211,s->view_object,a.view_phase,s->view_epoch,a.raw_type,{0},{0}};
 if(!bounded_name(key.directory,a.directory,10)||!bounded_name(key.filename,a.filename,14))return PIR_NAME;
 *out=key;*flags=PI_REQUIRED|((!a.busy[0]&&!a.busy[1])?PI_NATIVE_READY:0);return PIR_OK;
}
int pi_read_reference(const struct pi_read_port*p,const struct pi_source*s,struct pi_key*out,uint32_t*flags){return pi_read_reference_detailed(p,s,out,flags,0);}
