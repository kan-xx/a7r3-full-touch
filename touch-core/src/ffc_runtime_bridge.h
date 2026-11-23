#ifndef A7_FFC_RUNTIME_BRIDGE_H
#define A7_FFC_RUNTIME_BRIDGE_H
#include "ffc_adapter.h"
#include "native_catalog.h"
#include "envelope.h"
/* Internal catalog kinds only. Existing 13-word surface and 32-word Sony wire
 * stay unchanged. Complete provider identity is kept in this owner-only seal. */
#define SURFACE_FFC 5u
#define TARGET_FFC_ROW 9u
struct ffc_runtime_bridge {
 struct ffc_live current,sealed;
 struct touch_envelope expected;
 struct ffc_cancel_ticket cancel;
 struct ffc_result result;
 uint32_t current_valid,source_generation,sealed_valid,executing;
 uint32_t catalogs,seals,rejected,actions,uncertain;
};
int ffr_catalog(struct ffc_runtime_bridge*,const struct ffc_projection_ports*,struct ns_catalog*);
int ffr_hit(const struct ffc_runtime_bridge*,const struct surface_target*,const struct surface_point*);
int ffr_seal(struct ffc_runtime_bridge*,const struct touch_envelope*);
void ffr_revoke(struct ffc_runtime_bridge*);
/* Called inside the raw emitter's independent boundary, immediately before
 * each fixed key. Never accepts authority from an old stored address. */
int ffr_raw_boundary(const struct ffc_runtime_bridge*,const struct ffc_live*);
/* Only a real gesture envelope may activate FFC. Center uses the raw15 native
 * mapper, never the AA/Fn directed13 path. No borrowed read after release. */
int ffr_apply(struct ffc_runtime_bridge*,const struct ffc_action_ports*,const struct touch_envelope*,int activate);
#endif
