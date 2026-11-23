#ifndef TOUCH_DEFERRED_RECEIVER_H
#define TOUCH_DEFERRED_RECEIVER_H
#include "envelope.h"
struct envelope_pending {uint32_t active,sequence,nonce[4];struct touch_envelope expected;};
struct envelope_receiver_ports {
 void*context;
 int(*snapshot)(void*,struct surface_identity*);
 int(*gesture)(void*,uint32_t expected_serial); /* current gate/serial/held=0 */
 int(*drained)(void*); /* real ViewSystem route, no GUI handler on stack */
 int(*same_hit)(void*,const struct touch_envelope*); /* origin/end and fresh target */
 int(*apply)(void*,const struct touch_envelope*,int activate);
 uint32_t(*condition)(void*,uint32_t view,uint32_t which);
 int(*choice_center_sync)(void*,const struct touch_envelope*);
};
enum envelope_result {ENV_IGNORED=0,ENV_REVOKED=1,ENV_APPLIED=2};
/* source attestation and structural no17/1B classification happen before this
 * pure decision function; original stock callback already ran exactly once. */
#ifdef __cplusplus
extern "C" {
#endif
int envelope_receive(struct envelope_pending*,const struct envelope_receiver_ports*,const struct touch_envelope*,uint32_t now_ms,uint32_t current_serial,uint16_t held_contacts,int running);
#ifdef __cplusplus
}
#endif
#endif
