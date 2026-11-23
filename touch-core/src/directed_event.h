#ifndef A7_FIXED_DIRECTED_EVENT_H
#define A7_FIXED_DIRECTED_EVENT_H
#include "navigation.h"
#ifdef __cplusplus
extern "C" {
#endif
/* This is a closed set of intents, not an Event ID interface. */
enum a7_directed_event_kind {
    A7_DIRECTED_MENU_CENTER = 1,
    A7_DIRECTED_MENU_BACK = 2,
    A7_DIRECTED_FN_CENTER = 3
};
/* Fixed category0/destination4, empty ParamList, native Event ownership.
 * The caller supplies the actually observed highest active View ID, after
 * verifying UI owner, native phase, power, page/epoch/root and action scope.
 * MENU intents require View0xAA; FN_CENTER requires View0x44. Other kinds or
 * views are rejected before allocation. No raw Event ID is accepted.
 * Queued Event contains no callback/data pointer owned by this DSO.
 * submitted means ownership transferred, not action or physical acceptance.
 */
void a7_directed_event_submit(const struct a7_navigation_api *,
    enum a7_directed_event_kind, uint32_t observed_active_view,
    struct a7_navigation_result *);
#ifdef __cplusplus
}
#endif
#endif
