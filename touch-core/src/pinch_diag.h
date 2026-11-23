#ifndef PINCH_DIAG_V15_H
#define PINCH_DIAG_V15_H
#include <stdint.h>
#define P15D_FIRST 663u
#define P15D_WORDS 5u
#define P15D_MAGIC 0x41375044u
_Static_assert(P15D_FIRST>=660u+3u,"NMP660..662");
_Static_assert(P15D_FIRST+P15D_WORDS<=668u,"PB668 begins after diagnostic");
enum {P15D_READY=1,P15D_M59=2,P15D_CAPTURE=3,P15D_CONDITION=4,
 P15D_AFTER_GETTER=5,P15D_NO_PLAN=6,P15D_BLOCKED_BEFORE_ARM=7,
 P15D_GUI_QUEUE=8,P15D_REFERENCE=9,P15D_GUI_TOKEN=10};
/* Value-only telemetry. These functions return no authorization and cannot
 * read target memory, call a native method, change pause, or dispatch. */
static unsigned p15d_pop(uint32_t n){unsigned p=0;while(n){p+=n&1u;n>>=1;}return p;}
static void p15d_notification(uint32_t out[P15D_WORDS],uint32_t event_id,uint32_t now,
 uint32_t held,uint32_t source_ok,uint32_t stock_counted,uint32_t ui_match,
 uint32_t observed_top,uint32_t observed_view){
 if(!out||source_ok!=1||stock_counted!=1||ui_match!=1||held&0xffff0000u||
 p15d_pop(held)!=2||observed_top!=211||!observed_view)return;
 out[0]=P15D_MAGIC;out[1]=event_id;out[2]=now;if(out[3]!=UINT32_MAX)out[3]++;
}
/* Call only at an existing failed branch after current211 base proof. The
 * stage is retained historical diagnosis, never a qualification predicate. */
static void p15d_block(uint32_t out[P15D_WORDS],uint32_t stage){
 if(out&&stage>=P15D_READY&&stage<=P15D_GUI_TOKEN){out[0]=P15D_MAGIC;out[4]=stage;}}
#endif
