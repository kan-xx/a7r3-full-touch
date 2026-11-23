#ifndef TOUCH_READONLY_DIAGNOSTIC_H
#define TOUCH_READONLY_DIAGNOSTIC_H
#include <stdint.h>
struct touch_diag {uint32_t abi,bytes,status,vm,model_manager,element,model,model_vptr,initialized;int32_t touch_disp,touch_range,user_device;uint32_t desired;int32_t range;uint32_t forced;};
struct touch_diag_ports {
 void*context;uint32_t obj_base,pviewmanager,qualified;
 int(*read)(void*,uint32_t,void*,uint32_t);
 int32_t(*get_touch_disp)(void*); /* original static int(void), error2 preserved */
 int32_t(*get_touch_range)(void*); /* original static signed int(void), error2 preserved */
 int32_t(*get_user_device)(void*,uint32_t current_verified_view);
 uint32_t(*lookup_existing)(void*,uint32_t model_manager,uint32_t id);
};
int touch_diag_read(const struct touch_diag_ports*,uint32_t verified_view,struct touch_diag*);
#endif
