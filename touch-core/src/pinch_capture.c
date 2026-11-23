#include "pinch_capture.h"
struct observation {uint32_t view[2],mouse[5],header[10],visible,owner,user[2],slots[42],parent_header[10],parent_visible,parent_owner,parent_default,events[3];};
static void zero(void*p,unsigned n){uint8_t*x=p;while(n--)*x++=0;}
static int same(const void*a,const void*b,unsigned n){const uint8_t*x=a,*y=b;unsigned v=0;while(n--)v|=*x++^*y++;return !v;}
static unsigned pop(unsigned n){unsigned c=0;while(n){c+=n&1;n>>=1;}return c;}
static uint32_t xy(int16_t x,int16_t y){return(uint16_t)x|((uint32_t)(uint16_t)y<<16);}
static int collect(const struct pbc_ports*p,const struct pbc_source*s,uint32_t parent,struct observation*o){
 zero(o,sizeof*o);
#define RD(a,d,n)do{if(!p->read(p->context,a,d,n))return 0;}while(0)
 RD(s->view,o->view,8);RD(s->ws+0x4a8,o->mouse,20);RD(parent,o->parent_header,40);
 if(o->parent_header[0]!=s->view2+0x8bf418)return 1;
 RD(parent+0x38,&o->parent_visible,4);RD(parent+0x14c,&o->parent_owner,4);RD(parent+0x180,&o->parent_default,4);
 RD(parent+0x590,o->events,8);RD(parent+0x5a0,o->events+2,4);
 uint32_t child=parent+0x1c0;RD(child,o->header,40);
 if(o->header[0]!=s->obj+0x11ed7a0)return 1;
 RD(child+0x38,&o->visible,4);RD(child+0x14c,&o->owner,4);RD(child+0x15c,o->user,8);RD(child+0x2a4,o->slots,sizeof o->slots);
#undef RD
 return 1;
}
int pbc_current(const struct pbc_ports*p,const struct pbc_source*s,struct pbc_evidence*out){
 if(!out)return PBC_ARGUMENT;zero(out,sizeof*out);uint32_t*d=out->word,*pd=out->parent;d[0]=0x41374350;d[1]=2;pd[0]=0x41375043;
 if(!p||!s||!p->read||!p->hit||!p->get_widget||!p->scope||!s->obj||s->obj>UINT32_MAX-0x11ed7a0||!s->view3||s->view3>UINT32_MAX-0xa8f48||!s->view2||s->view2>UINT32_MAX-0x8bf418||!s->view||s->view&3||s->view>UINT32_MAX-8||!s->ws||s->ws&3||s->ws>UINT32_MAX-0x4bc||s->held&0xffff0000u||pop(s->held)!=2||!s->source_sequence||!s->power||!s->cookie||(s->phase!=3&&s->phase!=4))return d[4]=PBC_ARGUMENT;
 d[3]=s->now;d[5]=s->view;d[6]=s->phase;d[7]=s->held;d[23]=xy(s->x[0],s->y[0]);d[24]=xy(s->x[1],s->y[1]);d[27]=s->source_sequence;d[28]=s->power;d[29]=s->cookie;d[30]=s->ws;d[31]=s->view3;
 uint32_t parent=p->get_widget(p->context,s->view,PBC_CANVAS_RESOURCE);d[17]=pd[1]=parent;
 if(!p->scope(p->context,s))return d[4]=PBC_SCOPE;
 if(!parent||parent&3||parent>UINT32_MAX-0x5a4)return d[4]=PBC_RESOURCE;
 struct observation a,b;if(!collect(p,s,parent,&a))return d[4]=PBC_READ;uint32_t child=parent+0x1c0;
 d[8]=a.mouse[2];d[9]=a.mouse[3];d[10]=a.mouse[4];d[11]=a.header[0];d[12]=a.header[9];d[13]=a.owner;d[14]=((const uint8_t*)a.header)[0x21];d[15]=a.visible;d[18]=a.parent_header[0];d[19]=a.user[0];d[20]=a.user[1];d[21]=a.mouse[0];d[22]=a.mouse[1];pd[2]=((uint8_t*)a.parent_header)[0x21]|(a.parent_default<<8);pd[3]=a.parent_visible;pd[4]=a.parent_owner;for(unsigned i=0;i<3;i++)pd[5+i]=a.events[i];
 if(a.view[0]!=s->view3+0xa8f48||a.view[1]!=s->phase)return d[4]=PBC_VIEW;
 if(a.parent_header[0]!=s->view2+0x8bf418)return d[4]=PBC_PARENT_CLASS;
 if(a.mouse[2]!=1)return d[4]=PBC_BUTTONS;
 if(a.mouse[3]!=child||a.mouse[4]!=child)return d[4]=PBC_CAPTURE;
 if(a.header[0]!=s->obj+0x11ed7a0)return d[4]=PBC_CLASS;
 if((a.header[4]&~3u)!=parent+4||a.parent_header[7]!=parent)return d[4]=PBC_PARENT_LINK;
 if(a.owner!=211||a.parent_owner!=211)return d[4]=PBC_OWNER;
 if(d[14]!=0)return d[4]=PBC_MODE;
 if(((uint8_t*)a.parent_header)[0x21]!=0)return d[4]=PBC_PARENT_MODE;
 if(!(a.visible&2)||!(a.parent_visible&2))return d[4]=PBC_VISIBLE;
 if(a.user[0]!=parent||a.user[1]!=s->view2+0x57e629)return d[4]=PBC_CALLBACK;
 if(a.events[0]!=0x31||a.events[1]!=0x30||a.events[2]!=0x33)return d[4]=PBC_PARENT_EVENTS;
 for(unsigned i=0;i<21;i++){unsigned enabled=a.slots[2*i+1]&255u;if(enabled)d[16]|=1u<<i;if(a.slots[2*i]!=0x11012004||enabled!=((PBC_NATIVE_MASK>>i)&1u))return d[4]=PBC_SLOTS;}
 for(unsigned i=0;i<2;i++)if(s->x[i]>=0&&s->x[i]<640&&s->y[i]>=0&&s->y[i]<480){
  if(p->hit(p->context,child,s->x[i],s->y[i]))d[25]|=1u<<i;
  if(!p->scope(p->context,s))return d[4]=PBC_SCOPE;
  if(p->hit(p->context,parent,s->x[i],s->y[i]))d[25]|=1u<<(i+2);
  if(!p->scope(p->context,s))return d[4]=PBC_SCOPE;
 }
 if(d[25]!=15)return d[4]=PBC_HIT;
 uint32_t again=p->get_widget(p->context,s->view,PBC_CANVAS_RESOURCE);
 if(!p->scope(p->context,s))return d[4]=PBC_SCOPE;
 if(again!=parent)return d[4]=PBC_CHANGED;
 if(!collect(p,s,again,&b))return d[4]=PBC_READ;
 if(!same(&a,&b,sizeof a))return d[4]=PBC_CHANGED;
 d[26]=1;return d[4]=PBC_OK;
}
