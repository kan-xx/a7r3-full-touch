#include "report_cache.h"
void rc_publish(struct rc_cache*c,const uint32_t*in){__atomic_add_fetch(&c->seq,1,__ATOMIC_ACQ_REL);for(unsigned i=0;i<1024;i++)__atomic_store_n(c->words+i,in[i],__ATOMIC_RELAXED);__atomic_add_fetch(&c->seq,1,__ATOMIC_RELEASE);}
int rc_read(const struct rc_cache*c,uint32_t*out){for(unsigned a=0;a<3;a++){uint32_t s=__atomic_load_n(&c->seq,__ATOMIC_ACQUIRE);if(!s||(s&1))continue;for(unsigned i=0;i<1024;i++)out[i]=__atomic_load_n(c->words+i,__ATOMIC_RELAXED);__atomic_thread_fence(__ATOMIC_ACQUIRE);if(s==__atomic_load_n(&c->seq,__ATOMIC_RELAXED))return 1;}return 0;}
