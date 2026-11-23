#include "image_lease.h"
static void zero(void*p,unsigned n){uint8_t*d=p;while(n--)*d++=0;}
static int equal(const void*a,const void*b,unsigned n){const uint8_t*x=a,*y=b;unsigned d=0;while(n--)d|=*x++^*y++;return !d;}
static void copy(void*a,const void*b,unsigned n){uint8_t*x=a;const uint8_t*y=b;while(n--)*x++=*y++;}
static int same_key(const struct pi_key*a,const struct pi_key*b){
 /* Original ZoomPb accepts +/- in both phase3 and phase4. Phase is a fresh
  * eligibility guard, not an image identifier. Keep the same image lease only
  * inside that proved phase class; all other bytes still compare exactly. */
 if(a->view_id==211&&b->view_id==211&&(a->view_phase==3||a->view_phase==4)&&(b->view_phase==3||b->view_phase==4)){
  unsigned off=(unsigned)((const uint8_t*)&a->view_phase-(const uint8_t*)a);
  return equal(a,b,off)&&equal((const uint8_t*)a+off+4,(const uint8_t*)b+off+4,sizeof*a-off-4);
 }
 return equal(a,b,sizeof*a);
}
static int name(const uint8_t*p,unsigned n){if(!p[0])return 0;for(unsigned i=1;i<n;i++)if(!p[i]){for(i++;i<n;i++)if(p[i])return 0;return 1;}return 0;}
void pi_init(struct pi_lease*s){if(s)zero(s,sizeof*s);}
void pi_revoke(struct pi_lease*s){if(s){s->valid=0;zero(&s->key,sizeof s->key);}}
static int eligible(const struct pi_key*k,uint32_t f){
 if(!k||(f&PI_REQUIRED)!=PI_REQUIRED||!k->packed_power||!k->policy_epoch||!k->view_epoch||!k->libobj_base||!k->e9_element||!k->e9_model||!k->e9_complete||!k->view_object||!pi_raw_type_supported(k->raw_type)||k->reference_kind!=PI_REF_STREAM_FILE)return 0;
 if(k->view_id!=211||(k->view_phase!=3&&k->view_phase!=4))return 0;
 return name(k->directory,10)&&name(k->filename,14);
}
int pi_observe(struct pi_lease*s,const struct pi_key*k,uint32_t flags,uint32_t*cookie){
 if(cookie)*cookie=0;if(!s||!cookie)return PI_INVALID;
 if(s->exhausted){pi_revoke(s);return PI_EXHAUSTED;}
 if(!eligible(k,flags)){pi_revoke(s);return PI_INVALID;}
 if(s->valid&&same_key(&s->key,k)){s->key.view_phase=k->view_phase;*cookie=s->generation;return PI_SAME;}
 unsigned changed=s->valid;
 if(s->generation==0xffffffffu){s->exhausted=1;pi_revoke(s);return PI_EXHAUSTED;}
 s->generation++;copy(&s->key,k,sizeof*k);s->valid=1;*cookie=s->generation;
 return changed?PI_CHANGED:PI_NEW;
}
