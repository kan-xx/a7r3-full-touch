#ifndef TOUCH_TAP_SESSION_H
#define TOUCH_TAP_SESSION_H
#include "input_tap.h"
enum {TS_NONE=0,TS_CANCEL_NATIVE=1,TS_ARM_NATIVE=2,TS_COMMIT=3};
uint32_t ts_event(InputTapState*,uint32_t*release_cookie,uint32_t*release_ms,const A7R3InfraInputEvent*,uint32_t count,int copy_valid,const InputTapContext*,uint32_t now);
uint32_t ts_fence(InputTapState*,uint32_t release_cookie,uint32_t release_ms,uint32_t scheduled_cookie,const InputTapContext*,const InputTapFence*,uint32_t now,InputTapCandidate*,uint32_t*post_commit_serial);
#endif
