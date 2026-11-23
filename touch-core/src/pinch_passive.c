#include "pinch_passive.h"
static void zero(void*p,unsigned n){uint8_t*b=p;while(n--)*b++=0;}
void p14_passive_read(const struct p14_diag_port*p,const struct p14_diag_source*s,struct p14_diag*out){
 if(!out)return;zero(out,sizeof*out);uint32_t*d=out->word;d[0]=0xa7b01401;d[1]=1;
 if(!p||!p->read||!s||!s->obj||s->obj>UINT32_MAX-0x11ed7a0||!s->view3||s->view3>UINT32_MAX-0xa8f48||!s->view||(s->view&3)||s->view>UINT32_MAX-8||!s->ws||(s->ws&3)||s->ws>UINT32_MAX-0x4bc){d[4]=P14D_SOURCE;return;}
 d[2]=s->attempt;d[3]=s->now;d[5]=s->view;d[7]=s->ws;d[19]=s->e9_element;d[20]=s->e9_model;d[21]=s->e9_complete;d[27]=s->power;
 uint32_t v[2],mouse[5],header[10],slots[42];
 if(!p->read(p->context,s->view,v,8)){d[4]=P14D_VIEW_READ;return;}d[6]=v[1];
 if(v[0]!=s->view3+0xa8f48||(v[1]!=3&&v[1]!=4)){d[4]=P14D_VIEW_TYPE;return;}
 if(s->e9_complete&&s->e9_complete<=UINT32_MAX-0x1e4){
  if(p->read(p->context,s->e9_complete+0x114,d+22,8))d[4]|=1u<<8;
  if(p->read(p->context,s->e9_complete+0x184,d+24,4))d[4]|=1u<<9;
  if(p->read(p->context,s->e9_complete+0x1cc,d+25,4))d[4]|=1u<<10;
  if(p->read(p->context,s->e9_complete+0x1d6,d+26,4))d[4]|=1u<<11;}
 if(!p->read(p->context,s->ws+0x4a8,mouse,20)){d[4]|=P14D_MOUSE_READ;return;}
 d[8]=mouse[2];d[9]=mouse[3];d[10]=mouse[4];uint32_t w=mouse[4];
 if(!w)return;if(w&3||w>UINT32_MAX-0x34c){d[4]|=P14D_HEADER_READ;return;}
 if(!p->read(p->context,w,header,40)){d[4]|=P14D_HEADER_READ;return;}
 d[11]=header[0];d[12]=header[9];d[14]=((uint8_t*)header)[0x21];
 if(header[0]!=s->obj+0x11ed7a0){d[4]|=P14D_UNKNOWN_CLASS;return;}
 if(!p->read(p->context,w+0x14c,d+13,4)||!p->read(p->context,w+0x38,d+15,4)){d[4]|=P14D_OWNER_READ;return;}
 if(!p->read(p->context,w+0x2a4,slots,sizeof slots)){d[4]|=P14D_SLOT_READ;return;}
 for(unsigned i=0;i<21;i++)if(slots[2*i+1]&255u){d[16]|=1u<<i;uint32_t id=slots[2*i];if(id==0x30||id==0x130d3030)d[17]|=1u<<i;if(id==0x31||id==0x130d3031)d[18]|=1u<<i;}
}
