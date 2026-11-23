#ifndef A7_PINCH_H
#define A7_PINCH_H
#include <stddef.h>
#include <stdint.h>
/* Pure local state. It never reads a native pointer or dispatches an Event. */
enum { PP_NONE=0, PP_ENTER=1, PP_EXPAND=2, PP_CONTRACT=3 };
enum { PF_UI_OWNER=1, PF_POWER_ACT=2, PF_TYPED_VIEW=4, PF_STILL=8,
       PF_ORIGINAL_RETURNED=16, PF_SOURCE_ATTESTED=32, PF_BINDING_CURRENT=64,
       PF_FEED_REQUIRED=127, PF_GUI_FENCE=128, PF_CAPTURE_CURRENT=256,
       PF_ACTION_REQUIRED=511 };
typedef struct {
 uint32_t epoch,power,view_id,view_cookie,phase,layout,image_cookie,flags;
} PinchContext;
typedef struct {
 uint32_t source;
 int16_t y,x;
 int32_t state;
 uint32_t opaque_c;
 uint16_t flags,ticks;
 uint32_t opaque_14;
} PinchPacket;
/* ABI reserve allow_single_entry must remain zero. A held-finger action may
 * not transition SinglePB175 into a different captured View. */
typedef struct { uint32_t min_span,rate_ms,max_gap_ms,allow_single_entry; } PinchConfig;
typedef struct {
 PinchContext context;
 uint32_t serial,kind,span2,now_ms;
 uint16_t held_mask;
 int16_t center_x,center_y;
} PinchPlan;
typedef struct {
 PinchPacket last[16];
 uint16_t seen,held;
 uint32_t blocked,tap_suppressed,serial,baseline2,last_tick,last_feed_ms;
 uint32_t clock_seen,started,last_action_ms,action_seen,uncertain;
 PinchConfig config;
 PinchContext context;
} PinchState;
#ifdef __cplusplus
extern "C" {
#endif
PinchConfig pinch_default_config(void);
int pinch_init(PinchState *,const PinchConfig *);
/* Samples must be private copies of original blob1B then17, after stock once.
 * Process the whole Event; never dispatch from between two samples. */
void pinch_feed(PinchState *,const PinchContext *,const PinchPacket *,size_t,uint32_t);
/* Pure geometry and held-state rejection before canvas capture. A true
 * result still requires the complete native qualification and pinch_plan. */
int pinch_motion_pending(const PinchState *);
/* Future source-attested UI GUI fence; two physically held contacts are
 * required. PF_CAPTURE_CURRENT is freshly proven by the native adapter:
 * capture/hit are absent or belong to the current typed ZoomPB widget tree.
 * Unknown capture or any view transition rejects the action. Reusing the tap
 * fence's mouse-buttons==0 would prevent pinching. */
int pinch_plan(const PinchState *,const PinchContext *,uint32_t,PinchPlan *);
int pinch_plan_current(const PinchState *,const PinchContext *,const PinchPlan *,uint32_t);
/* A normal native return consumes one step, even if original evaluator vetoed
 * it. A throw is sticky; no automatic replay/cleanup of unknown Sony Event. */
void pinch_complete(PinchState *,const PinchPlan *,int normal_return,int uncertain);
void pinch_cancel(PinchState *);
#ifdef __cplusplus
}
#endif
#endif
