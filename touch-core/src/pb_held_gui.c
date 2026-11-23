#include "pb_held_gui.h"
struct observation {struct pbg_fence fence;uint32_t manager,table[33];uint8_t enabled;};
static void zero(void*p,unsigned n){uint8_t*d=p;while(n--)*d++=0;}
static int same(const void*a,const void*b,unsigned n){const uint8_t*x=a,*y=b;unsigned d=0;while(n--)d|=*x++^*y++;return !d;}
static unsigned pop(uint32_t n){unsigned c=0;while(n){c+=n&1u;n>>=1;}return c;}
static int observe(const struct pbg_ports*p,const struct pbg_source*s,struct observation*o,uint32_t*mask){
 zero(o,sizeof*o);*mask=0;
 if(!p->fence(p->context,&o->fence))return 0;*mask|=1;
 if(!p->read(p->context,s->obj+0x129ee55,&o->enabled,1))return 0;*mask|=2;
 if(!p->read(p->context,s->obj+0x129ee60,&o->manager,4))return 0;*mask|=4;
 if(!o->manager||(o->manager&3)||o->manager>UINT32_MAX-0x84)return 1;
 if(!p->read(p->context,o->manager,o->table,4))return 0;*mask|=8;
 if(o->table[0]!=s->obj+0x11ed420)return 1;
 if(!p->read(p->context,o->manager+4,o->table+1,sizeof o->table-4))return 0;*mask|=16;
 return 1;
}
static void evidence(const struct observation*o,struct pbg_evidence*d){
 d->fence=o->fence;d->enabled=o->enabled;d->manager=o->manager;d->manager_vptr=o->table[0];
 if(o->manager&&!(o->manager&3)&&o->manager<=UINT32_MAX-0x84&&o->fence.token>=0&&o->fence.token<4){
  unsigned index=(unsigned)o->fence.token;d->record=o->manager+4+index*0x20;
  for(unsigned i=0;i<8;i++)d->record_words[i]=o->table[1+index*8+i];
  for(unsigned i=0;i<4;i++)if(o->table[1+i*8+2]==index&&o->table[1+i*8+3]!=0)d->matching_mask|=1u<<i;
 }
}
int pbg_check(const struct pbg_ports*p,const struct pbg_source*s,struct pbg_evidence*d){
 if(!d)return PBG_ARGUMENT;zero(d,sizeof*d);
 if(!p||!s||!p->read||!p->fence||!s->obj||s->obj>UINT32_MAX-0x129ee64||s->held&0xffff0000u||pop(s->held)!=2)return d->status=PBG_ARGUMENT;
 struct observation a,b;uint32_t ma=0,mb=0;
 int ra=observe(p,s,&a,&ma);evidence(&a,d);d->read_mask=ma;
 if(!ra)return d->status=PBG_READ;
 if(!observe(p,s,&b,&mb)){d->read_mask|=mb<<8;return d->status=PBG_READ;}d->read_mask|=mb<<8;
 if(!same(&a,&b,sizeof a))return d->status=PBG_CHANGED;
#define TOKEN(reason_code) do{d->reason=reason_code;return d->status=PBG_TOKEN;}while(0)
 if(a.enabled!=1)TOKEN(PBG_DISABLED);
 if(!a.manager||(a.manager&3)||a.manager>UINT32_MAX-0x84)TOKEN(PBG_MANAGER);
 if(a.table[0]!=s->obj+0x11ed420)TOKEN(PBG_MANAGER_CLASS);
 if(a.fence.token<0||a.fence.token>3)TOKEN(PBG_TOKEN_RANGE);
 unsigned index=(unsigned)a.fence.token;const uint32_t*r=a.table+1+index*8;
 if(r[0]!=s->obj+0x11efd38)TOKEN(PBG_RECORD_CLASS);
 if(r[2]!=index)TOKEN(PBG_RECORD_TOKEN);
 if(r[3]!=3)TOKEN(PBG_RECORD_STATE);
 if(r[1]>15)TOKEN(PBG_CONTACT_RANGE);
 if(!(s->held&(1u<<r[1])))TOKEN(PBG_CONTACT_HELD);
 if(d->matching_mask!=(1u<<index))TOKEN(PBG_AMBIGUOUS);
 if(a.fence.buttons!=1)TOKEN(PBG_BUTTONS);
#undef TOKEN
 if(a.fence.count||a.fence.head||a.fence.tail)return d->status=PBG_QUEUE;
 return d->status=PBG_OK;
}
