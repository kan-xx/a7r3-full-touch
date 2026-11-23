#include "touch_diagnostic.h"
int touch_diag_read(const struct touch_diag_ports*p,uint32_t view,struct touch_diag*out){
 if(!p||!out||!p->qualified||!p->read||!p->lookup_existing)return 0;for(unsigned i=0;i<sizeof*out;i++)((uint8_t*)out)[i]=0;out->abi=1;out->bytes=sizeof*out;out->touch_disp=out->touch_range=out->user_device=2;
 if(p->get_touch_disp)out->touch_disp=p->get_touch_disp(p->context);if(p->get_touch_range)out->touch_range=p->get_touch_range(p->context);if(view&&p->get_user_device)out->user_device=p->get_user_device(p->context,view);
 if(!p->read(p->context,p->pviewmanager,&out->vm,4)||!out->vm||!p->read(p->context,out->vm+0x18,&out->model_manager,4)||!out->model_manager){out->status=1;return 1;}
 out->element=p->lookup_existing(p->context,out->model_manager,0xd9);if(!out->element){out->status=2;return 1;}
 if(!p->read(p->context,out->element+0x1c,&out->model,4)||!out->model||!p->read(p->context,out->model,&out->model_vptr,4)||out->model_vptr!=p->obj_base+0x11ffd40){out->status=3;return 1;}
 uint8_t initialized=0,range=0,forced=0;if(!p->read(p->context,out->model+0x8a,&initialized,1)){out->status=4;return 1;}out->initialized=initialized;if(!initialized){out->status=5;return 1;}
 if(!p->read(p->context,out->model+0x84,&out->desired,4)||!p->read(p->context,out->model+0x88,&range,1)||!p->read(p->context,out->model+0x89,&forced,1)){out->status=4;return 1;}out->range=range&128?(int32_t)range-256:range;out->forced=forced;return 1;
}
