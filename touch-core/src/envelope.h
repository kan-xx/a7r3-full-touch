#ifndef TOUCH_DEFERRED_ENVELOPE_H
#define TOUCH_DEFERRED_ENVELOPE_H
#include <stdint.h>
#include "surface_actions.h"
#define ENVELOPE_MAGIC 0xa7e66001u
#define ENVELOPE_BUILD 0x3111fe07u
#define ENVELOPE_EVENT 0x11013008u
#define ENVELOPE_KEY_BASE 0xa7e60000u
/* No17/1B key, blob or code pointer. Every field is an original ParamInt value.
 * A closed opaque envelope that stock ignores via its proven no-TP-blob branch.
 * Runtime must require fixed sender-owned pending sequence and every current
 * identity at receive time; this is not an arbitrary event/action interface. */
enum envelope_policy {ENV_REAL_TAP=1,ENV_CHOICE_OPEN_ONLY=2,ENV_FOCUS_ONLY=3,ENV_BACK_ONLY=4};
struct touch_envelope {
 uint32_t magic,abi,build,nonce[4],sequence;
 struct surface_identity surface;
 struct surface_target target;
 uint32_t origin_xy,end_xy,release_ms,post_commit_serial,policy;
};
#define ENVELOPE_WORDS (sizeof(struct touch_envelope)/4u)


/* Declared implementation contract: fixed event/dest1/category0, no raw IDs.
 * a7_navigation_api original ParamInt constructor is required. Allocation and
 * all params are fully owned before transferring to the native queue. */
struct a7_navigation_api;struct a7_navigation_result;
#ifdef __cplusplus
extern "C" {
#endif
void a7_touch_envelope_submit(const struct a7_navigation_api*,const struct touch_envelope*,struct a7_navigation_result*);
#ifdef __cplusplus
}
#endif
#endif
