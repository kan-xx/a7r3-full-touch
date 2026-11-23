#ifndef A7_TOUCH_AUTOLOAD_UI_H
#define A7_TOUCH_AUTOLOAD_UI_H
#include <stdint.h>
#define AS_UI_BUSY_PROOF 4u
#define AS_UI_INSTALL_REJECT 92u
#define AS_UI_POWER_PENDING 0x7042u
#define AS_UI_POWER_TIMEOUT 0x7043u
#define AS_UI_BAD_PROOF 0x7044u
#define AS_UI_MAX_DEFER 8u
struct as_ui_report {uint32_t bound,callbacks,busy_count,result;};
struct as_ui_state {struct as_ui_report report;uint32_t terminal,done;};
struct as_ui_ports {void*context;int(*bind)(void*);int(*install)(void*,uint32_t*power_proof);};
enum as_ui_decision {AS_UI_DONE=0,AS_UI_DEFER20=1,AS_UI_TERMINAL=2};
/* Already on exact original UI/TimerManager callback, caller keeps selfpin.
 * Only a CURRENT failed install with PG_REPORT_BUSY may defer; never admission
 * without a coherent active proof, and never SAFE_BUSY after a callback exists. */
int as_ui_once(struct as_ui_state*,const struct as_ui_ports*);
void as_ui_fail(struct as_ui_state*,uint32_t error);
#endif
