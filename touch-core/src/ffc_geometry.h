#ifndef A7_FFC_NATIVE_GEOMETRY_H
#define A7_FFC_NATIVE_GEOMETRY_H
#include "ffc_adapter.h"
struct ffc_point {float x,y;};
struct ffc_geometry_ports {
    void *context;
    int (*boundary)(void*,uint32_t current_label,const struct ffc_authority*);
    /* Proven Obj Thumb5c8ec5, sret(out, Widget*this, const Point*world). */
    void (*local)(struct ffc_point*,void*,const struct ffc_point*);
    float (*width)(void*);  /* Original Obj Thumb5c8fc9, soft-float ABI. */
    float (*height)(void*); /* Original Obj Thumb5c8fd7, soft-float ABI. */
    uint32_t qualified;
};
#ifdef __cplusplus
extern "C" {
#endif
int ffc_measure_label(const struct ffc_geometry_ports*,uint32_t current_label,
                       const struct ffc_authority*,struct ffc_rect*);
#ifdef __cplusplus
}
#endif
#endif
