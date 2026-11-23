#ifndef TOUCH_HOOK_BRIDGE_H
#define TOUCH_HOOK_BRIDGE_H
#include <stdint.h>
#include "native_raw_tp_abi.h"
#include "envelope.h"
struct rt_frame {uint32_t pb_event_id,pb_header_ok;uint32_t source_ok,count,lr,event,recognized,stock_counted,copy_ok;A7R3InfraInputEvent samples[2];uint32_t private_envelope;struct touch_envelope envelope;};
typedef void(*rt_stock_cb)(void*);
#ifdef __cplusplus
extern "C" {
#endif
rt_stock_cb rt_hook_begin(void*,uint32_t,struct rt_frame*);
void rt_hook_copy(void*,struct rt_frame*);
void rt_hook_copy_failed(struct rt_frame*);
void rt_hook_finish(struct rt_frame*);
void rt_hook_threw(struct rt_frame*);
void rt_hook_optional_threw(struct rt_frame*);
struct ts;
void tr_observed_impl(void*,struct ts*,void*,uint32_t);
void tr_observed_failed(void*,struct ts*,void*,uint32_t);
void rt_fence_impl(void*,struct ts*,void*,uint32_t);
void rt_fence_failed(void*,struct ts*,void*,uint32_t);
void tr_observed_guard(void*,struct ts*,void*);
void rt_fence_guard(void*,struct ts*,void*);
void rt_pb_fence_guard(void*,struct ts*,void*);
void rt_input_hook(void*);
#ifdef __cplusplus
}
#endif
#endif
