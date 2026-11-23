#ifndef TOUCH_SURFACE_HIT_H
#define TOUCH_SURFACE_HIT_H
#include "surface_actions.h"
struct surface_point {float x,y;};
typedef int(*surface_native_hit)(void*,const struct surface_target*,const struct surface_point*);
/* Targets already passed exact class, mode/ancestor/model and current-layout
 * guards. This layer adds semantic priority/ambiguity, never guesses geometry. */
int surface_pick(const struct surface_target*,uint32_t,void*,surface_native_hit,const struct surface_point*);
int surface_pick_tap(const struct surface_target*,uint32_t,void*,surface_native_hit,const struct surface_point*,const struct surface_point*);
#endif
