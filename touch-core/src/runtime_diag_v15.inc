/* Integrated r2 pure retained report; NOT_HW_QUALIFIED.
 * Existing source/UI/current211/held2 predicates narrow notification logging;
 * a positive negative-only peek never authorizes a native action. */
#include "pinch_diag.h"
static void rt_p15_note_notification(const struct rt_frame*f){
 if(!f||!f->source_ok||!f->stock_counted||tid()!=s.q.ui_tid||
  __builtin_popcount((unsigned)rt_pb.gesture.held)!=2)return;
 uint32_t id=0,view=0;if(!rt_pb_peek_top(&id,&view))return;
 p15d_notification(&RW(P15D_FIRST),f->pb_event_id,rt_now(),rt_pb.gesture.held,
 f->source_ok,f->stock_counted,1,id,view);
}
