#ifndef A7_PB_IMAGE_LEASE_H
#define A7_PB_IMAGE_LEASE_H
#include <stdint.h>
/* Epochs are locally attested invalidation generations, never guessed native
 * fields. observed_media_epoch is optional: use zero without a proved source.
 * policy_epoch revokes on observed other input/lifecycle changes, excluding this
 * gesture's TP. The key is a CURRENT logical reference, not a global image ID. */
/* STREAM_FILE schema2: native type/full normalized names/original xSize,
 * ySize and fileSize. No CContID and no rendering playDims/rotation/fps.
 * Sizes may be zero as in the native getter; this is a private observation
 * key, never an action authorization or a global media UID. */
struct pi_key {
 uint32_t packed_power,observed_media_epoch,policy_epoch,libobj_base;
 uint32_t e9_element,e9_model,e9_complete;
 uint32_t reference_kind,image_width,image_height,file_size;
 uint32_t view_id,view_object,view_phase,view_epoch,raw_type;
 uint8_t directory[10],filename[14];
};
enum {PI_REF_STREAM_FILE=1};
/* Original RAWFileInfo=false/true normal-still branches map MWF4/14 to5/15.
 * These are discrete enum values, never a bit mask or filename inference. */
enum {PI_RAW_TYPE_NORMAL_STILL=5,PI_RAW_TYPE_RAW_STILL=15};
static inline int pi_raw_type_supported(uint32_t type){return type==PI_RAW_TYPE_NORMAL_STILL||type==PI_RAW_TYPE_RAW_STILL;}
enum {PI_UI_OWNER=1,PI_POWER_READY=2,PI_E9_TYPED=4,PI_SOURCE_STABLE=8,
      PI_NATIVE_READY=16,PI_REQUIRED=15};
enum {PI_INVALID=0,PI_NEW=1,PI_SAME=2,PI_CHANGED=3,PI_EXHAUSTED=4};
struct pi_lease {struct pi_key key;uint32_t generation,valid,exhausted;};
_Static_assert(sizeof(struct pi_key)==88,"pi key POD ABI");
_Static_assert(sizeof(struct pi_lease)==100,"pi lease POD ABI");
/* Only call init for a genuinely fresh core/boot context, never to clear a live
 * exhausted lease. A returned cookie only indexes this private state; it is not
 * a globally unique image ID and is not an authorization to zoom. Native
 * Only current ZoomPb211 phase3/4 is supported. READY is deliberately separate
 * from identity; a held gesture may wait for native rendering. Each action
 * still needs READY plus original condition2 and current-view/capture guards. The
 * original wheel-equivalent handler operates the current logical image. */
void pi_init(struct pi_lease*);
void pi_revoke(struct pi_lease*);
int pi_observe(struct pi_lease*,const struct pi_key*,uint32_t,uint32_t*cookie);
#endif
