#include "tap_session.h"
uint32_t ts_event(InputTapState*s,uint32_t*cookie,uint32_t*when,const A7R3InfraInputEvent*p,uint32_t count,int valid,const InputTapContext*c,uint32_t now){
 if(!s||!cookie||!when||!valid||!p||!count||count>2){if(s)input_tap_cancel(s);return TS_CANCEL_NATIVE;}uint32_t before=s->timer_cookie,effects=0,result=0;for(uint32_t i=0;i<count;i++)effects|=input_tap_feed(s,p+i,c,now);if(effects&INPUT_TAP_CANCEL_TIMER)result|=TS_CANCEL_NATIVE;if(s->phase==INPUT_TAP_WAIT_FENCE&&s->timer_cookie){if(s->timer_cookie!=before){*cookie=s->timer_cookie;*when=now;}result|=TS_ARM_NATIVE;}return result;
}
uint32_t ts_fence(InputTapState*s,uint32_t released,uint32_t when,uint32_t scheduled,const InputTapContext*c,const InputTapFence*f,uint32_t now,InputTapCandidate*out,uint32_t*after){
 if(!s||!out||!after)return TS_NONE;
 /* A stale native callback must not cancel a newer TRACKING contact. It may
  * arrange the NEW pending cookie, but never commits from its old record. */
 if(scheduled!=s->timer_cookie)return s->phase==INPUT_TAP_WAIT_FENCE&&s->timer_cookie&&released==s->timer_cookie&&now-when<=160?TS_ARM_NATIVE:TS_NONE;
 if(released!=s->timer_cookie||now-when>160){input_tap_cancel(s);return TS_CANCEL_NATIVE;}
 enum InputTapFenceResult r=input_tap_fence(s,scheduled,c,f,now,out);
 if(r==INPUT_TAP_FENCE_REARM20)return TS_ARM_NATIVE;if(r==INPUT_TAP_FENCE_CANCEL)return TS_CANCEL_NATIVE;if(r==INPUT_TAP_FENCE_COMMIT){*after=s->serial;return TS_COMMIT;}return TS_NONE;
}
