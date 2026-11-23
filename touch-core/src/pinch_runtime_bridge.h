#ifndef A7_PINCH_RUNTIME_BRIDGE_H
#define A7_PINCH_RUNTIME_BRIDGE_H
#include <stdint.h>
struct ts;
struct rt_frame;
#ifdef __cplusplus
extern "C" {
#endif
void rt_pb_fence_impl(void *,struct ts *,void *,uint32_t);
void rt_pb_fence_failed(void *,struct ts *,void *,uint32_t);
void rt_pb_fence_guard(void *,struct ts *,void *);
#ifdef __cplusplus
}
#endif
#endif
