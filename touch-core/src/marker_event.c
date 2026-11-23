#include "marker_event.h"
struct marker_event32 { uint32_t *refs,id;uint8_t dest,category,pad[2];void *params; };
struct center_params32 { void *vector;uint32_t *refs;uint8_t *sorted;uint32_t *next;uint8_t *flag; };
struct center_vector32 { void **begin,**end,**capacity; };
#if defined(__arm__)
_Static_assert(sizeof(struct marker_event32)==16,"original Event ABI");
_Static_assert(sizeof(struct center_params32)==20,"original ParamList ABI");
_Static_assert(sizeof(struct center_vector32)==12,"original vector ABI");
#endif
void a7_marker_event_submit(const struct a7_navigation_api*a,struct a7_navigation_result*r){
 if(!r)return;
 for(size_t i=0;i<sizeof(*r);i++)((uint8_t*)r)[i]=0;
 r->version=1;r->event_id=0x11013008;r->destination=1;r->category=0;r->target_view=0;
 if(!a||a->qualified!=1||!a->event_manager||!a->event_dtor||!a->push||!a->trylock||!a->unlock||!a->allocate||!a->release){r->status=A7_NAV_BAD_API;return;}
 /* The empty ParamList reproduces original Event(id,4,0) without its partially
  * initialized, throwing allocation path. All ownership is allocated first.
  * No native ParamInt constructor and no DSO callback enters the event. */
 const size_t sizes[8]={sizeof(struct marker_event32),4,sizeof(struct center_params32),sizeof(struct center_vector32),4,1,4,1};
 void*p[8]={0};
 for(unsigned i=0;i<8;i++){
  p[i]=a->allocate(sizes[i]);
  if(!p[i]){r->status=A7_NAV_ALLOC;for(unsigned j=0;j<i;j++){a->release(p[j]);r->release_count++;}return;}
  r->allocation_count++;for(size_t j=0;j<sizes[i];j++)((uint8_t*)p[i])[j]=0;
 }
 struct marker_event32*e=p[0];struct center_params32*l=p[2];struct center_vector32*v=p[3];
 e->refs=p[1];*e->refs=1;e->id=0x11013008;e->dest=1;e->category=0;e->params=l;
 l->vector=v;l->refs=p[4];*l->refs=1;l->sorted=p[5];l->next=p[6];*l->next=1;l->flag=p[7];
 r->prepared=1;
 /* Qualified category0 ignores push's bool (both branches never read it).
  * The existing false/caught wrapper preserves its single pre-link throw
  * ownership rule. This trylock is a contention precheck, not lock transfer. */
 if(a->trylock((uint8_t*)a->event_manager+16)){r->precheck_busy=1;r->status=A7_NAV_BUSY;}
 else{a->unlock((uint8_t*)a->event_manager+16);r->status=(uint32_t)a7_navigation_push_caught(a,e,r);}
 if(r->status==A7_NAV_OK){r->submitted=1;return;}
 a->event_dtor(e);a->release(e);r->release_count=8;
}
