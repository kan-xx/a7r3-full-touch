#include "pinch_dispatch.h"
struct SonyEvent {uint32_t *refs,id;uint8_t dest,category,pad[2];void *params;};
struct SonyParams {void *vector;uint32_t *refs;uint8_t *sorted;uint32_t *next;uint8_t *flag;};
struct SonyVector {void **begin,**end,**capacity;};
#if defined(__arm__)
static_assert(sizeof(SonyEvent)==16,"Event ABI");
static_assert(sizeof(SonyParams)==20,"ParamList ABI");
static_assert(sizeof(SonyVector)==12,"vector ABI");
static_assert(sizeof(PinchPacket)==24,"raw packet ABI");
#endif
static uint32_t fixed_id(const PinchPlan*p){
 if(p->context.view_id==211){if(p->kind==PP_EXPAND)return 0x130d3003;if(p->kind==PP_CONTRACT)return 0x130d3004;}
 return 0;
}
extern "C" void pinch_dispatch(PinchState*s,const PinchPlan*p,const PinchDispatchPorts*a,PinchDispatchResult*r){
 if(!r)return;*r={};
 if(!s||!p||!a||a->qualified!=1||!a->fresh||!a->allocate||!a->release||!a->event_dtor||!a->dispatch){r->status=1;return;}
 r->event_id=fixed_id(p);if(!r->event_id){r->status=2;return;}
 PinchContext c={};void *vm=nullptr;uint32_t now=0;
 try{if(!a->fresh(a->opaque,&c,&vm,&now)||!vm||!pinch_plan_current(s,&c,p,now)){r->status=3;return;}}
 catch(...){r->status=7;r->uncertain=1;pinch_complete(s,p,0,1);return;}
 const size_t sizes[8]={sizeof(SonyEvent),4,sizeof(SonyParams),sizeof(SonyVector),4,1,4,1};void *v[8]={};
 for(unsigned i=0;i<8;i++){v[i]=a->allocate(sizes[i]);if(!v[i]){r->status=4;for(unsigned j=0;j<i;j++){a->release(v[j]);r->releases++;}return;}r->allocations++;for(size_t j=0;j<sizes[i];j++)static_cast<uint8_t*>(v[i])[j]=0;}
 SonyEvent*e=static_cast<SonyEvent*>(v[0]);SonyParams*l=static_cast<SonyParams*>(v[2]);e->refs=static_cast<uint32_t*>(v[1]);*e->refs=1;e->id=r->event_id;e->dest=4;e->params=l;l->vector=v[3];l->refs=static_cast<uint32_t*>(v[4]);*l->refs=1;l->sorted=static_cast<uint8_t*>(v[5]);l->next=static_cast<uint32_t*>(v[6]);*l->next=1;l->flag=static_cast<uint8_t*>(v[7]);
 /* Re-resolve after every allocation has succeeded. No old borrowed View is
  * touched. A power/scene/sample change releases all Sony-only local blocks. */
 PinchContext c2={};void *vm2=nullptr;uint32_t now2=0;
 int current=0;
 try{current=a->fresh(a->opaque,&c2,&vm2,&now2)&&vm2==vm&&pinch_plan_current(s,&c2,p,now2);}
 catch(...){r->status=7;r->uncertain=1;pinch_complete(s,p,0,1);}
 if(!current){if(!r->status)r->status=5;for(unsigned i=0;i<8;i++){a->release(v[i]);r->releases++;}return;}
 try{r->called=1;a->dispatch(vm2,e);r->returned=1;}catch(...){r->status=6;r->uncertain=1;r->retained_event=reinterpret_cast<uintptr_t>(e);pinch_complete(s,p,0,1);return;}
 /* 8326c9 handles fixed13 through8314dd/82efcd synchronously. Original
  * evaluator checks its own current phase/media/model veto on every action. */
 a->event_dtor(e);a->release(e);r->releases=8;pinch_complete(s,p,1,0);
}
