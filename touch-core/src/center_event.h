#ifndef A7_CENTER_EVENT_H
#define A7_CENTER_EVENT_H
#include "navigation.h"
#ifdef __cplusplus
extern "C" {
#endif
/* UI-owner caller must first validate current native ViewSettingMenu id0xaa,
 * current root/epoch/page, completed release, and fresh selected row.
 * This only submits fixed logical center to that view. No arbitrary event ID.
 * A7_NAV_OK reports ownership transfer, not actual UI acceptance/completion.
 */
void a7_center_event_submit(const struct a7_navigation_api *,struct a7_navigation_result *);
#ifdef __cplusplus
}
#endif
#endif
