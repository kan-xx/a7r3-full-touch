#include "typed_still.h"
#include <stddef.h>
#ifdef A7_STILL_ARM
void *memset(void *,int,size_t);
int memcmp(const void *,const void *,size_t);
#else
#include <string.h>
#endif
enum { E9_MODEL_VP=0x11fee30, E9_PRIMARY_VP=0x11fecf8,
 STREAM_VP=0xdc1f08, MODEL_SUB=0x8a4, STREAM_SUB=0x11c };
static int sum(uint32_t a,uint32_t b,uint32_t *o){
 if(a>UINT32_MAX-b)return 0;
 *o=a+b;return 1;
}
static int readat(const PinchStillPorts *p,uint32_t a,uint32_t off,uint32_t *o){
 uint32_t at;return sum(a,off,&at)&&p->read32(p->opaque,at,o);
}
static int one(const PinchStillPorts *p,PinchStillSnapshot *s){
 PinchE9Reference r={0,0};uint32_t vp,expect,offset,top,stream;
 if(!p->lookup_e9(p->opaque,&r)||!r.element||!r.model_subobject||
    (r.element&3)||(r.model_subobject&3)||r.model_subobject<MODEL_SUB)return 0;
 if(!readat(p,r.element,0x1c,&vp)||vp!=r.model_subobject)return 0;
 if(!readat(p,r.model_subobject,0,&vp)||!sum(p->obj_base,E9_MODEL_VP,&expect)||vp!=expect)return 0;
 if(vp<8||!p->read32(p->opaque,vp-8,&offset)||offset!=(uint32_t)-MODEL_SUB)return 0;
 top=r.model_subobject-MODEL_SUB;
 if(!readat(p,top,0,&vp)||!sum(p->obj_base,E9_PRIMARY_VP,&expect)||vp!=expect)return 0;
 if(!sum(top,STREAM_SUB,&stream)||!readat(p,stream,0,&vp)||
    !sum(p->obj_base,STREAM_VP,&expect)||vp!=expect)return 0;
 s->element=r.element;s->model_subobject=r.model_subobject;
 s->complete_object=top;s->stream_object=stream;
 if(!readat(p,stream,0x68,&s->raw_type)||!readat(p,stream,0x78,&s->width)||
    !readat(p,stream,0x7c,&s->height)||!readat(p,stream,0x8c,&s->rotation))return 0;
 return 1;
}
int pinch_still_snapshot(const PinchStillPorts *p,PinchStillSnapshot *out){
 PinchStillSnapshot a={0},b={0};
 if(!out)return PINCH_STILL_INVALID;
 memset(out,0,sizeof(*out));
 if(!p||!p->obj_base||!p->lookup_e9||!p->read32||!one(p,&a)||!one(p,&b)||
    memcmp(&a,&b,sizeof(a)))return PINCH_STILL_INVALID;
 *out=a;
 /* Bounds only qualify a usable decoded image, not a substitute for type.
  * Rotation is a native degree scalar; no portrait coordinate conversion is
  * inferred here. The production image-cookie provider remains mandatory. */
 if(a.raw_type==5&&a.width&&a.height&&a.width<=65535&&a.height<=65535)
  return PINCH_STILL_NORMAL;
 return PINCH_STILL_OBSERVED;
}
