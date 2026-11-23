#ifndef A7_P14_PASSIVE_H
#define A7_P14_PASSIVE_H
#include <stdint.h>
struct p14_diag_source {uint32_t obj,view3,view,ws,e9_element,e9_model,e9_complete,power,attempt,now;};
struct p14_diag_port {void*context;int(*read)(void*,uint32_t,void*,uint32_t);};
struct p14_diag {uint32_t word[28];};
enum {P14D_SOURCE=1,P14D_VIEW_READ=2,P14D_VIEW_TYPE=3,P14D_MOUSE_READ=4,P14D_HEADER_READ=5,P14D_UNKNOWN_CLASS=6,P14D_OWNER_READ=7,P14D_SLOT_READ=8};
#define P14_DIAG_START 752u
#define P14_DIAG_WORDS 28u
_Static_assert(P14_DIAG_START>750u,"sync_result occupies740..750");
_Static_assert(P14_DIAG_START+P14_DIAG_WORDS<=780u,"NMP starts780");
/* Pure passive diagnostic. No native function/action/hit port exists. Caller
 * attests fresh UI, current registry211 and ACT before this bounded snapshot.
 * It produces NO authorization flags. Unknown classes get only40B header;
 * owner/slot reads occur only for exact direct GEN_Button. Diagnostic bytes
 * may be observed stable by a later independent snapshot, not lifetime-pinned. */
void p14_passive_read(const struct p14_diag_port*,const struct p14_diag_source*,struct p14_diag*);
#endif
