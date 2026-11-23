#ifndef TOUCH_SYNC_DIRECTED_H
#define TOUCH_SYNC_DIRECTED_H
#include "directed_event.h"
struct sync_api {struct a7_navigation_api heap;void*view_manager;void(*dispatch)(void*,void*);uint32_t qualified;};
struct sync_result {uint32_t abi,status,constructed,called,returned,retained,allocations,releases,event_id,view_id;uintptr_t retained_event;};
#ifdef __cplusplus
extern "C" {
#endif
/* Fixed13 only; original8326c9 borrows Event synchronously and does not cache
 * these IDs. Normal return destroys all8 blocks. On native throw it may still
 * expose borrowed Event in a view's currentEvent; preserve Sony-only storage
 * and report retained/uncertain, never auto-retry or touch an old view. */
void a7_sync_directed(const struct sync_api*,enum a7_directed_event_kind,uint32_t observed_view,struct sync_result*);
#ifdef __cplusplus
}
#endif
#endif
