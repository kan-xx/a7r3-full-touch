/* PB held-GUI qualification only. MENU/Fn/tap release fences are unchanged. */
#include "pb_held_gui.h"
#define PBG_DIAG_FIRST 241u
#define PBG_DIAG_WORDS 15u
#define PBG_META_FIRST 896u
#define PBG_RECORD_FIRST 1008u
_Static_assert(PBG_DIAG_FIRST>240u&&PBG_DIAG_FIRST+PBG_DIAG_WORDS<=256u,"leases240/catalog256");
_Static_assert(PBG_META_FIRST>=888u+8u&&PBG_META_FIRST+4u<=900u,"rawmeta888/FFC900");
_Static_assert(PBG_RECORD_FIRST>=928u+8u*10u&&PBG_RECORD_FIRST+8u<=1016u,"raw928/cache1016");
static int rt_pb_gui_fence(void*x,struct pbg_fence*out){(void)x;InputTapFence f={0};
 if(!rt_drain_snapshot(&f))return 0;*out=(struct pbg_fence){f.count,f.head,f.tail,f.mouse0_buttons,f.gui_token};return 1;}
static void rt_pb_gui_diagnose(const struct pbg_evidence*e){
 uint32_t*d=&RW(PBG_DIAG_FIRST);d[0]=0x41374746;d[1]=1u|(e->status<<8)|(e->reason<<16);d[2]=++rt_pb.gui_attempts;
 d[3]=rt_now();d[4]=rt_pb.source_sequence;d[5]=rt_pb.timer_sequence;d[6]=rt_pb.gesture.held;d[7]=rt_pb.view_object;
 d[8]=e->fence.count;d[9]=e->fence.head;d[10]=e->fence.tail;d[11]=e->fence.buttons;d[12]=(uint32_t)e->fence.token;d[13]=e->manager;d[14]=e->record;
 uint32_t*m=&RW(PBG_META_FIRST);m[0]=e->enabled;m[1]=e->manager_vptr;m[2]=e->matching_mask;m[3]=e->read_mask;
 __builtin_memcpy(&RW(PBG_RECORD_FIRST),e->record_words,sizeof e->record_words);
}
static int rt_pb_gui_held(int allow_retry){struct pbg_source source={s.r.obj_base,rt_pb.gesture.held};struct pbg_ports ports={0,snapshot_read,rt_pb_gui_fence};struct pbg_evidence evidence;
 int rc=pbg_check(&ports,&source,&evidence);rt_pb_gui_diagnose(&evidence);
 if(rc){rt_pb.last_result=rc==PBG_QUEUE?40:rc==PBG_TOKEN?41:rc==PBG_READ?42:43;
  if(rc==PBG_QUEUE){if(allow_retry)rt_pb.retry_queue=1;p15d_block(&RW(P15D_FIRST),P15D_GUI_QUEUE);}else p15d_block(&RW(P15D_FIRST),P15D_GUI_TOKEN);return 0;}
 return 1;
}
