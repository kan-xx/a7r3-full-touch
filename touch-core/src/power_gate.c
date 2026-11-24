#include "power_gate.h"
#include "power_layout.h"
/* Supports both exported A7R_Lifecycle_LiveState(uint32_t[12]) (compiler-independent)
 * and fallback raw offset reads relative to historical v14 base. */
static int sample_all(const struct pg_ports*p,uint32_t*r_out,uint32_t out[11]){
 if(p->live_state){
  uint32_t w[12]={0};
  if(p->live_state(p->context,w))return 0;
  *r_out=w[0];
  for(unsigned i=0;i<11;i++)out[i]=w[i+1];
  return 1;
 }
 if(!p->read||!p->read(p->context,PG_BOOTSTRAP+PG_B_OWNER_REQUEST,r_out))return 0;
 const uint32_t offsets[11]={PG_BOOTSTRAP+PG_B_ERROR,PG_BOOTSTRAP+PG_B_PREPARED,PG_BOOTSTRAP+PG_B_OWNER_ERROR,PG_BOOTSTRAP+PG_B_BG_ERROR,PG_BOOTSTRAP+PG_B_BG_READY,PG_GLOBAL_PHASE,PG_GLOBAL_STOP,PG_GLOBAL_ERROR,PG_GLOBAL_FINAL_REQUESTED,PG_BOOTSTRAP+PG_B_BOOTSTRAP_STATE,PG_BOOTSTRAP+PG_B_ACT_CLAIM};
 for(unsigned i=0;i<11;i++)if(!p->read(p->context,offsets[i],out+i))return 0;
 return 1;
}
static int common(const uint32_t*v){return!v[0]&&v[1]==1&&!v[2]&&!v[3]&&v[4]==1&&v[5]==22&&!v[6]&&!v[7]&&!v[8];}
int pg_check(const struct pg_ports*p,struct pg_proof*out){
 uint32_t report[2048],v[11],again[11],r=0,r2=0;if(!p||(!p->live_state&&!p->read)||!p->report||!out)return 0;for(unsigned i=0;i<sizeof*out;i++)((uint8_t*)out)[i]=0;
 if(!sample_all(p,&r,v)){out->status=1;return 0;}out->request=r;out->state=v[9];out->act_claim=v[10];
 if(!common(v)){out->status=2;return 0;}uint32_t gen=r>>3;int initial=r==1&&v[9]==3;int active=(r&7)==5&&gen&&v[10]==gen&&(v[9]==3||v[9]==4);if(!initial&&!active){out->status=3;return 0;}
 if(p->report(p->context,report,sizeof report)){out->status=4;return 0;}
 out->event=report[PG_W_LO_EVENT];out->generation=report[PG_W_LO_GENERATION];out->worker_generation=report[PG_W_WORKER_GENERATION];out->audio_phase=report[PG_W_AUDIO_PHASE];out->menu_phase=report[PG_W_MENU_PHASE];
 if(report[PG_W_ABI]!=PG_REPORT_ABI||report[PG_W_BYTES]!=PG_REPORT_BYTES||report[PG_W_BUILD]!=PG_REPORT_BUILD||report[PG_W_PHASE]!=22||report[PG_W_READY]!=1||report[PG_W_ERROR]||report[PG_W_STOP]||report[PG_W_FINAL_REQUESTED]||report[PG_W_BG_ERROR]||report[PG_W_INGRESS_ERROR]||report[PG_W_LO_ERROR]||report[PG_W_AUDIO_PHASE]||report[PG_W_MENU_PHASE]||(report[PG_W_AUDIO_CONTROL]&3)||(report[PG_W_MENU_CONTROL]&3)){out->status=5;return 0;}
 if(active&&(report[PG_W_LO_EVENT]!=5||report[PG_W_LO_GENERATION]!=gen||report[PG_W_WORKER_GENERATION]!=gen||report[PG_W_ACT_CLAIM]!=gen)){out->status=6;return 0;}
 if(initial&&report[PG_W_BOOTSTRAP_STATE]!=3){out->status=6;return 0;}
 if(!sample_all(p,&r2,again)||r2!=r||!common(again)||again[9]!=v[9]||again[10]!=v[10]){out->status=7;return 0;}return 1;
}
/* A separate failure-cause proof, never admission. Only a stable explicit
 * INACT/SUS/EXIT request with all instantaneous fatal/error fields clear can
 * authorize waiting for a NEW full ACT. Status 2/3/5 alone proves nothing. */
int pg_revoked(const struct pg_ports*p,uint32_t*out){
 uint32_t a[11],b[11],r=0,r2=0;if(!p||(!p->live_state&&!p->read)||!out)return 0;*out=0;
 if(!sample_all(p,&r,a)||!common(a))return 0;
 uint32_t event=r&7;if(!r||(event!=2&&event!=3&&event!=6))return 0;
 if(!sample_all(p,&r2,b)||!common(b)||r2!=r)return 0;
 for(unsigned i=0;i<11;i++)if(a[i]!=b[i])return 0;*out=r;return 1;
}
