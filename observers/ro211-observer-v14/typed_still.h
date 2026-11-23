#ifndef A7_PINCH_TYPED_STILL_H
#define A7_PINCH_TYPED_STILL_H
#include <stdint.h>
/* These ports are trusted runtime-owned readers, never Sony Event parameters.
 * lookup_e9 must freshly walk the accepted ModelManager map on the UI owner,
 * without constructing any model. read32 must reject unmapped/overflow reads.
 * Obj full SHA, all anchors, UI/power/source/registry identity are qualified by
 * the caller before invoking this helper. No prior model pointer is cached. */
typedef struct {
 uint32_t element, model_subobject;
} PinchE9Reference;
typedef struct {
 void *opaque;
 uint32_t obj_base;
 int (*lookup_e9)(void *,PinchE9Reference *);
 int (*read32)(void *,uint32_t,uint32_t *);
} PinchStillPorts;
typedef struct {
 uint32_t element,model_subobject,complete_object,stream_object;
 uint32_t raw_type,width,height,rotation;
} PinchStillSnapshot;
enum { PINCH_STILL_INVALID=0, PINCH_STILL_OBSERVED=1,
       PINCH_STILL_NORMAL=2 };
/* Returns NORMAL only for native normal-still app code5, never unknown0.
 * OBSERVED preserves other valid typed metadata for read-only diagnostics.
 * This snapshot is NOT an image cookie or a pin on a Sony object. */
int pinch_still_snapshot(const PinchStillPorts *,PinchStillSnapshot *);
#endif
