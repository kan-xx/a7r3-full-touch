#ifndef A7_PINCH_DISPATCH_H
#define A7_PINCH_DISPATCH_H
#include "pinch.h"
/* All ports are trusted native wrappers resolved/anchored by the owning
 * runtime, never obtained from packet data or retained widget records. */
typedef struct {
 uint32_t qualified;
 void *opaque;
 int (*fresh)(void *,PinchContext *,void **view_manager,uint32_t *now_ms);
 void *(*allocate)(size_t);
 void (*release)(void *);
 void (*event_dtor)(void *);
 void (*dispatch)(void *view_manager,void *event);
} PinchDispatchPorts;
typedef struct {
 uint32_t status,event_id,allocations,releases,called,returned,uncertain;
 uintptr_t retained_event;
} PinchDispatchResult;
#ifdef __cplusplus
extern "C" {
#endif
/* Does not invoke stock TP callback: caller must already have invoked it
 * exactly once and reached the source-attested future GUI frame. */
void pinch_dispatch(PinchState *,const PinchPlan *,const PinchDispatchPorts *,PinchDispatchResult *);
#ifdef __cplusplus
}
#endif
#endif
