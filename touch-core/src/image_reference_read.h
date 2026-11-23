#ifndef A7_PB_IMAGE_REFERENCE_READ_H
#define A7_PB_IMAGE_REFERENCE_READ_H
#include "image_lease.h"
struct pi_source {
 uint32_t obj_base,view3_base,e9_element,e9_model,e9_complete,view_object;
 uint32_t packed_power,policy_epoch,view_epoch,observed_media_epoch,flags;
};
struct pi_read_port {void*context;int(*read)(void*,uint32_t,void*,uint32_t);};
enum {PIR_OK=0,PIR_ARGUMENT=1,PIR_READ=2,PIR_CHANGED=3,PIR_TYPED=4,
      PIR_BUSY=5,PIR_REFERENCE=6,PIR_NAME=7};
/* PIR_REFERENCE6 is reserved for the former CID reader; V14 does not emit it.
 * Caller already proved exact original library SHA/anchors, current boot/PID,
 * native UI owner, highest registry ZoomPb211 factory, fresh E9 lookup, and
 * power. `read` is a bounded memory copy only, no Sony/renderer/Model calls.
 * This port observes current logical contents. It is not a global UID lookup.
 * Normal-still and native RAWFileInfo still types are exactly5/15; no broad
 * SinglePb mask whitelist is used. Raw type stays in the full key, so5/15
 * transitions invalidate its private generation. On any failure outputs are zero; no object pointer may be reused for action.
 * A stable key can be returned without PI_NATIVE_READY during native busy.
 * That is observation only: no wheel command may execute until freshly ready.
 */
int pi_read_reference(const struct pi_read_port*,const struct pi_source*,struct pi_key*,uint32_t*flags);
/* Diagnostic-only projection of the same two copies: bit31 means stable
 * copies completed; bits0..5 identify model/complete/stream/view/type/phase
 * mismatch. Zero on argument/read/changed failure. No extra target reads. */
struct pi_read_detail {uint32_t checked_mask,raw_type,view_phase;};
int pi_read_reference_detailed(const struct pi_read_port*,const struct pi_source*,struct pi_key*,uint32_t*flags,struct pi_read_detail*);
#endif
