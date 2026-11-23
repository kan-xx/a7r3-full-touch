#ifndef A7_RUNTIME_PERF_H
#define A7_RUNTIME_PERF_H
#include <stdint.h>
enum {RP_EPOCH,RP_MAPS,RP_INSPECT,RP_CATALOG,RP_POWER,RP_PRE,RP_STOCK,
 RP_POST,RP_FENCE,RP_RECEIVE,RP_APPLY,RP_LEASE,RP_HIT,RP_PUBLISH,RP_FAST,RP_TOTAL};
enum {RS_PRE=1,RS_POST=2,RS_TIMER=3};
#ifdef __cplusplus
extern "C" {
#endif
uint32_t rt_perf_clock(void);
void rt_perf_end(uint32_t,uint32_t);
uint32_t rt_segment_enter(uint32_t);
void rt_segment_leave(uint32_t);
void rt_segment_invalidate(void);
#ifdef __cplusplus
}
#endif
#endif
