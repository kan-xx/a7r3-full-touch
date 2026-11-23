#include "envelope.h"
#include "navigation.h"
struct env_event {uint32_t*refs,id;uint8_t dest,category,pad[2];void*params;};
struct env_params {void*vector;uint32_t*refs;uint8_t*sorted;uint32_t*next;uint8_t*flag;};
struct env_vector {void**begin,**end,**capacity;};
struct env_int {void*vptr;uint32_t type,key,sequence,value;};
#if defined(__arm__)
_Static_assert(sizeof(struct env_event)==16,"original Event ABI");
_Static_assert(sizeof(struct env_params)==20,"original ParamList ABI");
_Static_assert(sizeof(struct env_vector)==12,"original vector ABI");
_Static_assert(sizeof(struct env_int)==20,"original ParamInt ABI");
_Static_assert(ENVELOPE_WORDS==32,"fixed private envelope schema");
#endif
static int allowed(const struct touch_envelope*e){
 if(!e||e->magic!=ENVELOPE_MAGIC||e->abi!=6||e->build!=ENVELOPE_BUILD||!e->sequence||!e->surface.power||!e->surface.view||!e->surface.root||!e->post_commit_serial)return 0;
 if(!(e->nonce[0]|e->nonce[1]|e->nonce[2]|e->nonce[3]))return 0;
 if(e->policy<ENV_REAL_TAP||e->policy>ENV_BACK_ONLY)return 0;
 uint32_t k=e->target.kind,n=e->target.index;int valid=0;
 if(e->surface.view_id==0xaa&&e->surface.kind==SURFACE_MENU&&e->surface.phase==1)valid=(k==TARGET_ROW&&n<6)||(k==TARGET_TAB&&n<7)||(k==TARGET_PAGE&&n<15)||((k==TARGET_PAGE_PREV||k==TARGET_PAGE_NEXT||k==TARGET_BACK)&&!n);
 else if(e->surface.view_id==0xaa&&e->surface.kind==SURFACE_POPUP&&e->surface.phase==2)valid=(k==TARGET_POPUP_ROW&&n<6)||(k==TARGET_BACK&&!n);
 else if(e->surface.view_id==0x44&&e->surface.kind==SURFACE_FN&&e->surface.layout==2&&e->surface.phase==1)valid=k==TARGET_FN_CELL&&n<12;
 else if(e->surface.view_id==0xffc&&e->surface.kind==SURFACE_FFC&&e->surface.layout==0&&e->surface.phase==1&&e->surface.first&&e->policy==ENV_REAL_TAP)valid=k==TARGET_FFC_ROW&&n<6;
 if(!valid)return 0;
 if(e->policy==ENV_CHOICE_OPEN_ONLY&&(e->surface.view_id!=0xaa||e->surface.kind!=SURFACE_MENU||e->surface.phase!=1||k!=TARGET_ROW))return 0;
 if(e->policy==ENV_FOCUS_ONLY&&k==TARGET_BACK)return 0;if(e->policy==ENV_BACK_ONLY&&(e->surface.view_id!=0xaa||k!=TARGET_BACK))return 0;return 1;
}
void a7_touch_envelope_submit(const struct a7_navigation_api*a,const struct touch_envelope*e,struct a7_navigation_result*r){
 if(!r)return;for(unsigned i=0;i<sizeof*r;i++)((uint8_t*)r)[i]=0;r->version=1;r->event_id=ENVELOPE_EVENT;r->destination=1;r->category=0;
 if(!a||a->qualified!=1||!a->event_manager||!a->param_ctor||!a->event_dtor||!a->push||!a->trylock||!a->unlock||!a->allocate||!a->release){r->status=A7_NAV_BAD_API;return;}
 if(!allowed(e)){r->status=A7_NAV_IDENTITY;return;}
 const unsigned total=9+ENVELOPE_WORDS;void*p[9+ENVELOPE_WORDS];for(unsigned i=0;i<total;i++)p[i]=0;
 const size_t base[9]={sizeof(struct env_event),4,sizeof(struct env_params),sizeof(struct env_vector),4,1,4,1,ENVELOPE_WORDS*sizeof(void*)};
 for(unsigned i=0;i<total;i++){size_t n=i<9?base[i]:sizeof(struct env_int);p[i]=a->allocate(n);if(!p[i]){r->status=A7_NAV_ALLOC;for(unsigned j=0;j<i;j++){a->release(p[j]);r->release_count++;}return;}r->allocation_count++;for(unsigned j=0;j<n;j++)((uint8_t*)p[i])[j]=0;}
 struct env_event*event=p[0];struct env_params*l=p[2];struct env_vector*v=p[3];event->refs=p[1];*event->refs=1;event->id=ENVELOPE_EVENT;event->dest=1;event->category=0;event->params=l;l->vector=v;l->refs=p[4];*l->refs=1;l->sorted=p[5];l->next=p[6];*l->next=ENVELOPE_WORDS+1;l->flag=p[7];v->begin=p[8];v->end=v->begin+ENVELOPE_WORDS;v->capacity=v->end;
 /* Original ParamInt constructor sets native vptr/type without allocating.
  * Keys are fixed private values; neither stock17 nor1B is present. */
 const uint8_t*bytes=(const uint8_t*)e;for(unsigned i=0;i<ENVELOPE_WORDS;i++){uint32_t value=(uint32_t)bytes[4*i]|(uint32_t)bytes[4*i+1]<<8|(uint32_t)bytes[4*i+2]<<16|(uint32_t)bytes[4*i+3]<<24;int32_t signed_value;__builtin_memcpy(&signed_value,&value,4);struct env_int*x=p[9+i];a->param_ctor(x,signed_value);x->key=ENVELOPE_KEY_BASE+i;x->sequence=i+1;v->begin[i]=x;}
 r->prepared=1;if(a->trylock((uint8_t*)a->event_manager+16)){r->precheck_busy=1;r->status=A7_NAV_BUSY;}else{a->unlock((uint8_t*)a->event_manager+16);r->status=a7_navigation_push_caught(a,event,r);}if(r->status==A7_NAV_OK){r->submitted=1;return;}a->event_dtor(event);a->release(event);r->release_count=total;
}
