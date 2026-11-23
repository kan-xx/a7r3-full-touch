#ifndef A7R_MENU_SEMANTIC_REGIONS_H
#define A7R_MENU_SEMANTIC_REGIONS_H
#include "native_catalog.h"
enum mtr_error {MTR_OK=0,MTR_ARGUMENT=1,MTR_SCOPE=2,MTR_IDENTITY=3,MTR_GEOMETRY=4,MTR_FAMILY=5};
struct mtr_rect {float left,top,right,bottom;uint32_t enabled;};
struct mtr_map {uint32_t valid,count,error;struct surface_identity identity;struct surface_target source[NS_TARGETS];struct mtr_rect region[NS_TARGETS];};
struct mtr_context {const struct ns_io*io;const struct ns_catalog*catalog;const struct mtr_map*map;};
/* UI-owner only, immediately after qualified ns_collect. hit(0,0) refreshes
 * original lazy bounds; read copies native312B bodies. No Event is dispatched.
 * panel dimensions MUST be freshly verified native logical dimensions.
 * Unknown scope/family leaves valid0: mtr_hit then preserves native ns_hit.
 * Never retain this map across an input callback/native action/layout change. */
int mtr_build(const struct ns_io*,const struct ns_catalog*,uint32_t panel_width,uint32_t panel_height,struct mtr_map*);
/* surface_pick[_tap] callback, with unchanged surface actions/row semantics.
 * All expanded rectangles derive from the just-collected native world bounds.
 * No allocation, thread, IO port retention, coordinate calibration or scaling. */
int mtr_hit(void*,const struct surface_target*,const struct surface_point*);
#endif
