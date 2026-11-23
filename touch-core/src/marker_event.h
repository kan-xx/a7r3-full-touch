#ifndef A7_MARKER_EVENT_H
#define A7_MARKER_EVENT_H
#include "navigation.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Fixed diagnostic raw TP Event11013008/dest1/category0 with empty ParamList.
 * No blob, no coordinates, no arbitrary event ID; original stock lacks17 and
 * returns without GUI/hardware. Requires actual native Framework routing.
 * Success means queued, not proof that the real hook was invoked. */
void a7_marker_event_submit(const struct a7_navigation_api *,struct a7_navigation_result *);
#ifdef __cplusplus
}
#endif
#endif
