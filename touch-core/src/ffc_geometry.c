#include "ffc_geometry.h"
static int gate(const struct ffc_geometry_ports*p,uint32_t w,const struct ffc_authority*a){return p->boundary(p->context,w,a)==1;}
static int integer(float x,int32_t*out){if(x!=x||x<-4096.0f||x>4096.0f)return 0;int32_t n=(int32_t)x;if((float)n!=x)return 0;*out=n;return 1;}
int ffc_measure_label(const struct ffc_geometry_ports*p,uint32_t w,const struct ffc_authority*a,struct ffc_rect*out){
 if(!p||!out||!a||!w||p->qualified!=1||!p->boundary||!p->local||!p->width||!p->height)return 0;
 struct ffc_point zero={0,0},x={1,0},y={0,1},p0,px,py;
 if(!gate(p,w,a))return 0;p->local(&p0,(void*)(uintptr_t)w,&zero);
 if(!gate(p,w,a))return 0;p->local(&px,(void*)(uintptr_t)w,&x);
 if(!gate(p,w,a))return 0;p->local(&py,(void*)(uintptr_t)w,&y);
 if(px.x-p0.x!=1.0f||px.y-p0.y!=0.0f||py.x-p0.x!=0.0f||py.y-p0.y!=1.0f)return 0;
 if(!gate(p,w,a))return 0;float width=p->width((void*)(uintptr_t)w);
 if(!gate(p,w,a))return 0;float height=p->height((void*)(uintptr_t)w);
 int32_t left,top,iw,ih;
 if(!integer(-p0.x,&left)||!integer(-p0.y,&top)||!integer(width,&iw)||!integer(height,&ih)||iw<=0||iw>640||ih<=0||ih>480||!gate(p,w,a))return 0;
 *out=(struct ffc_rect){left,top,left+iw,top+ih};return 1;
}
