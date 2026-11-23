#include "hook_bridge.h"
#include "runtime_perf.h"
class runtime_segment {uint32_t token,began,stage;public:runtime_segment(uint32_t kind,uint32_t which,int enabled=1) noexcept:token(0),began(0),stage(which){try{token=enabled?rt_segment_enter(kind):0;if(token)began=rt_perf_clock();}catch(...){if(token){try{rt_segment_leave(token);}catch(...){}token=0;}rt_segment_invalidate();}}~runtime_segment() noexcept {if(token){try{rt_perf_end(stage,began);}catch(...){}try{rt_segment_leave(token);}catch(...){rt_segment_invalidate();}}}};
static uint32_t profile_clock() noexcept {try{return rt_perf_clock();}catch(...){return 0;}}
static void profile_end(uint32_t k,uint32_t t) noexcept {try{rt_perf_end(k,t);}catch(...){}}
/* Stock exactly once even if our optional pre-copy fails; original exceptions
 * preserve their original propagation. Every retained native entry selfpins. */
extern "C" void rt_input_hook(void*event){
 struct rt_frame f={};uint32_t lr=(uintptr_t)__builtin_return_address(0);
 rt_stock_cb original=rt_hook_begin(event,lr,&f);
 try{runtime_segment segment(RS_PRE,RP_PRE,f.source_ok);rt_hook_copy(event,&f);}catch(...){rt_hook_copy_failed(&f);}
 uint32_t stock_began=f.source_ok?profile_clock():0;try{original(event);}catch(...){if(f.source_ok)profile_end(RP_STOCK,stock_began);rt_hook_threw(&f);throw;}if(f.source_ok)profile_end(RP_STOCK,stock_began);
 try{runtime_segment segment(RS_POST,RP_POST,f.source_ok);rt_hook_finish(&f);}catch(...){rt_hook_optional_threw(&f);}
}

extern "C" void tr_observed_guard(void*r,struct ts*t,void*d){uint32_t lr=(uintptr_t)__builtin_return_address(0);try{tr_observed_impl(r,t,d,lr);}catch(...){tr_observed_failed(r,t,d,lr);}}
extern "C" void rt_fence_guard(void*r,struct ts*t,void*d){uint32_t lr=(uintptr_t)__builtin_return_address(0);try{runtime_segment segment(RS_TIMER,RP_FENCE);rt_fence_impl(r,t,d,lr);}catch(...){rt_fence_failed(r,t,d,lr);}}

#include "pinch_runtime_bridge.h"
extern "C" void rt_pb_fence_guard(void*r,struct ts*t,void*d){uint32_t lr=(uintptr_t)__builtin_return_address(0);try{runtime_segment segment(RS_TIMER,RP_FENCE);rt_pb_fence_impl(r,t,d,lr);}catch(...){try{rt_pb_fence_failed(r,t,d,lr);}catch(...){/* Unknown native removal keeps its owned timer and blocks unload. */}}}
