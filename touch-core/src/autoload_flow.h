#ifndef A7_TOUCH_AUTOLOAD_FLOW_H
#define A7_TOUCH_AUTOLOAD_FLOW_H
#include "native.h"
#include "autoload_contract.h"
struct as_ports {void*context;int(*validate_borrowed)(void*,void*,uint32_t);int(*fresh)(void*,uint32_t,struct tr_request*);void*(*pin)(void*,void*);int(*prepare_owned)(void*,void*,const struct tr_request*,uint32_t*);int(*close_owned)(void*,void*);void(*uncertain)(void*,void*,uint32_t);};
#ifdef __cplusplus
extern "C" {
#endif
/* No owner unwind, no caller-handle close. SAFE_BUSY is returned only after
 * this invocation had no queued callback/adoption and closed its independent
 * ref with a known successful result. Other errors are terminal to the loader. */
int as_start(const struct as_ports*,void*borrowed,uint32_t ui,struct as_report*);
int A7R_Touch_Autostart(void*borrowed,uint32_t trusted_ui_tid);
#ifdef __cplusplus
}
#endif
#endif
