#include "synchronous_directed.h"
struct sync_event {uint32_t*refs,id;uint8_t dest,category,pad[2];void*params;};
struct sync_params {void*vector;uint32_t*refs;uint8_t*sorted;uint32_t*next;uint8_t*flag;};
struct sync_vector {void**begin,**end,**capacity;};
#if defined(__arm__)
static_assert(sizeof(sync_event)==16,"original Event");static_assert(sizeof(sync_params)==20,"original params");static_assert(sizeof(sync_vector)==12,"original vector");
#endif
extern "C" void a7_sync_directed(const sync_api*a,a7_directed_event_kind kind,uint32_t view,sync_result*r){
 if(!r)return;*r={};r->abi=1;r->view_id=view;
 if(!a||a->qualified!=1||a->heap.qualified!=1||!a->heap.allocate||!a->heap.release||!a->heap.event_dtor||!a->view_manager||!a->dispatch){r->status=35;return;}
 uint32_t id=kind==A7_DIRECTED_MENU_CENTER&&view==0xaa?0x130aa005:kind==A7_DIRECTED_MENU_BACK&&view==0xaa?0x130aa006:kind==A7_DIRECTED_FN_CENTER&&view==0x44?0x13044005:0;if(!id){r->status=30;return;}r->event_id=id;
 const size_t sizes[8]={sizeof(sync_event),4,sizeof(sync_params),sizeof(sync_vector),4,1,4,1};void*p[8]={};
 for(unsigned i=0;i<8;i++){p[i]=a->heap.allocate(sizes[i]);if(!p[i]){r->status=31;for(unsigned j=0;j<i;j++){a->heap.release(p[j]);r->releases++;}return;}r->allocations++;for(unsigned j=0;j<sizes[i];j++)static_cast<uint8_t*>(p[i])[j]=0;}
 sync_event*e=static_cast<sync_event*>(p[0]);sync_params*l=static_cast<sync_params*>(p[2]);e->refs=static_cast<uint32_t*>(p[1]);*e->refs=1;e->id=id;e->dest=4;e->params=l;l->vector=p[3];l->refs=static_cast<uint32_t*>(p[4]);*l->refs=1;l->sorted=static_cast<uint8_t*>(p[5]);l->next=static_cast<uint32_t*>(p[6]);*l->next=1;l->flag=static_cast<uint8_t*>(p[7]);r->constructed=1;
 try{r->called=1;a->dispatch(a->view_manager,e);r->returned=1;}catch(...){r->status=33;r->retained=1;r->retained_event=reinterpret_cast<uintptr_t>(e);return;}
 a->heap.event_dtor(e);a->heap.release(e);r->releases=8;
}
