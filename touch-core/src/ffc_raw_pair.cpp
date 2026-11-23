#include "ffc_adapter.h"
struct native_event {uint32_t*refs,id;uint8_t dest,category,pad[2];void*params;};
struct native_params {void*vector;uint32_t*refs;uint8_t*sorted;uint32_t*next;uint8_t*flag;};
struct native_vector {void**begin,**end,**capacity;};
#if defined(__arm__)
static_assert(sizeof(native_event)==16,"Sony Event ABI");
static_assert(sizeof(native_params)==20,"Sony ParamList ABI");
static_assert(sizeof(native_vector)==12,"Sony vector ABI");
#endif
static void clear(void*p,size_t n){auto*b=static_cast<uint8_t*>(p);while(n--)*b++=0;}
static uint32_t key_id(uint32_t key){switch(key){case FFC_UP:return 0x15010070;case FFC_DOWN:return 0x15010080;case FFC_CENTER:return 0x15010060;case FFC_BACK:return 0x15010110;default:return 0;}}
static int good(const ffc_raw_ports*a){return a&&a->qualified==1&&a->allocate&&a->release&&a->event_dtor&&a->raw_dispatch&&a->boundary;}
extern "C" int ffc_pair_prepare(const ffc_raw_ports*a,ffc_key key,ffc_pair*p,ffc_pair_report*r){
 if(!r)return FFC_BAD_API;clear(r,sizeof*r);r->status=FFC_BAD_API;
 if(!good(a)||!p||!key_id(key)||p->prepared)return FFC_BAD_API;
 for(unsigned i=0;i<16;i++)if(p->blocks[i])return FFC_BAD_API;
 p->key=0;for(unsigned i=0;i<2;i++)p->attempted[i]=p->returned[i]=p->retained[i]=0;
 const size_t sizes[8]={sizeof(native_event),4,sizeof(native_params),sizeof(native_vector),4,1,4,1};
 unsigned allocated=0;
 try{for(;allocated<16;allocated++){void*q=a->allocate(sizes[allocated%8]);if(!q)break;p->blocks[allocated]=q;clear(q,sizes[allocated%8]);r->allocated++;}}
 catch(...){/* malloc is the qualified port; retain no partially constructed Event. */}
 if(allocated!=16){for(unsigned i=0;i<allocated;i++){a->release(p->blocks[i]);p->blocks[i]=nullptr;r->released++;}r->status=FFC_ALLOC;return FFC_ALLOC;}
 for(unsigned index=0;index<2;index++){
  void**q=p->blocks+index*8;auto*e=static_cast<native_event*>(q[0]);auto*l=static_cast<native_params*>(q[2]);
  e->refs=static_cast<uint32_t*>(q[1]);*e->refs=1;e->id=key_id(key)+(index?1:2);e->dest=4;e->params=l;
  l->vector=q[3];l->refs=static_cast<uint32_t*>(q[4]);*l->refs=1;l->sorted=static_cast<uint8_t*>(q[5]);l->next=static_cast<uint32_t*>(q[6]);*l->next=1;l->flag=static_cast<uint8_t*>(q[7]);
 }
 p->key=key;p->prepared=1;r->status=FFC_OK;return FFC_OK;
}
extern "C" int ffc_pair_emit(const ffc_raw_ports*a,ffc_pair*p,unsigned release,const ffc_live*f,ffc_pair_report*r){
 if(!r||!good(a)||!p||!f||!p->prepared||release>1||p->attempted[release]||
    (release&&!p->returned[0])||(!release&&p->attempted[1])){if(r)r->status=FFC_BAD_API;return FFC_BAD_API;}
 if((f->authority.flags&FFC_REQUIRED_AUTHORITY)!=FFC_REQUIRED_AUTHORITY||!f->vm||
    (release?f->held!=p->key:f->held!=0)||(p->key!=FFC_BACK&&f->pending)||
    !a->boundary(a->context,f,release)){r->status=FFC_BAD_SCOPE;return FFC_BAD_SCOPE;}
 p->attempted[release]=1;r->called++;
 try{a->raw_dispatch(reinterpret_cast<void*>(static_cast<uintptr_t>(f->vm)),p->blocks[8*release]);p->returned[release]=1;r->returned++;r->status=FFC_OK;return FFC_OK;}
 catch(...){p->retained[release]=1;r->retained++;r->retained_event[release]=reinterpret_cast<uintptr_t>(p->blocks[8*release]);r->status=FFC_NATIVE_UNCERTAIN;return FFC_NATIVE_UNCERTAIN;}
}
extern "C" int ffc_pair_dispose(const ffc_raw_ports*a,ffc_pair*p,ffc_pair_report*r){
 if(!r||!good(a)||!p||!p->prepared)return FFC_BAD_API;
 for(unsigned index=0;index<2;index++){
  if(p->retained[index]||!p->blocks[index*8])continue;
  try{a->event_dtor(p->blocks[index*8]);a->release(p->blocks[index*8]);r->released+=8;for(unsigned k=0;k<8;k++)p->blocks[index*8+k]=nullptr;}
  catch(...){p->retained[index]=1;r->retained++;r->retained_event[index]=reinterpret_cast<uintptr_t>(p->blocks[index*8]);r->status=FFC_NATIVE_UNCERTAIN;}
 }
 p->prepared=0;return r->retained?FFC_NATIVE_UNCERTAIN:FFC_OK;
}
