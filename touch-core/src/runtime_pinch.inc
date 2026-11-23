/* Included after runtime_cache.inc and the existing rt_drain/rt_nav ports.
 * Called only AFTER stock normal return. No persisted borrowed native View.
 * RW668..699 are private PB diagnostics, outside all V12 report regions. */
#include "pinch_runtime_bridge.h"
#include "pinch.h"
#include "pinch_dispatch.h"
#include "image_reference_read.h"
#include "pinch_native_identity.h"
#include "pinch_timer.h"
#include "pinch_diag.h"
#define P16_IMAGE_DIAG_FIRST 904u
_Static_assert(P16_IMAGE_DIAG_FIRST>=900u+4u&&P16_IMAGE_DIAG_FIRST+4u<=908u,"FFC counters / image diagnostic / FFC result");
_Static_assert(sizeof(PinchPacket)==sizeof(A7R3InfraInputEvent),"PB raw packet size");
static struct {
 PinchState gesture;struct pi_lease image;struct a7_menu_snapshot registry;
 void *timer,*view3_ref;uint32_t view3_base,view3_close_uncertain,initialized,disabled,paused;
 uint32_t policy_epoch,view_epoch,view_object,source_sequence,timer_sequence;
 uint32_t in_fence,inflight,dispatching,last_result,last_cancel,retry_queue,defer_left,dispatch_fresh;
 uint32_t registry_token,registry_invalidation,registry_view;
 uint32_t capture_attempts,capture_has_nonzero,gui_attempts;
 uint32_t last_reader_flags,last_reader_result,frames,actions,pauses,cancels;
 struct pi_key last_key;PinchDispatchResult result;
} rt_pb;
enum {PB_CANCEL_SOURCE=1,PB_CANCEL_KEY=2,PB_CANCEL_IDENTITY=3,
 PB_CANCEL_LIFECYCLE=4,PB_CANCEL_THROW=5,PB_CANCEL_COPY=6,
 PB_PAUSE_NOTIFICATION=20,PB_CAPTURE_UNPROVEN=30};
static void rt_pb_report(void){uint32_t *p=&RW(668);p[0]=0x41375042;p[1]=2;
 p[2]=rt_pb.initialized;p[3]=rt_pb.disabled|(rt_pb.view3_close_uncertain<<8);p[4]=rt_pb.paused;
 p[5]=rt_pb.gesture.held;p[6]=rt_pb.gesture.serial;p[7]=rt_pb.image.generation;
 p[8]=rt_pb.policy_epoch;p[9]=rt_pb.view_epoch;p[10]=rt_pb.view_object;
 p[11]=rt_pb.source_sequence;p[12]=rt_pb.timer_sequence;p[13]=(uintptr_t)rt_pb.timer;
 p[14]=rt_pb.inflight;p[15]=rt_pb.last_result;p[16]=rt_pb.last_cancel;
 p[17]=rt_pb.last_reader_flags;p[18]=rt_pb.last_reader_result;p[19]=rt_pb.frames;
 p[20]=rt_pb.actions;p[21]=rt_pb.pauses;p[22]=rt_pb.cancels;
 p[23]=rt_pb.view3_base;p[24]=rt_pb.last_key.reference_kind;p[25]=rt_pb.last_key.file_size;
 p[26]=rt_pb.last_key.view_phase;p[27]=rt_pb.result.status;
 p[28]=rt_pb.result.called;p[29]=rt_pb.result.returned;p[30]=rt_pb.result.uncertain;
 p[31]=rt_pb.gesture.tap_suppressed;}
static int rt_pb_bump(uint32_t *n){if(*n==UINT32_MAX){rt_pb.disabled=1;return 0;}++*n;return 1;}
static void rt_pb_init(void){if(rt_pb.initialized)return;rt_pb.initialized=1;
 rt_pb.policy_epoch=rt_pb.view_epoch=1;rt_pb.paused=1;pi_init(&rt_pb.image);
 PinchConfig cfg=pinch_default_config();if(!pinch_init(&rt_pb.gesture,&cfg))rt_pb.disabled=1;}
/* Pure cancellation is safe even in a C++ exception handler. A queued owned
 * timer remains a module reference until its normal callback/removal retires. */
static void rt_pb_revoke(uint32_t reason){rt_pb_init();rt_pb.last_cancel=reason;
 rt_pb.paused=1;rt_pb.cancels++;pinch_cancel(&rt_pb.gesture);pi_revoke(&rt_pb.image);
 rt_pb_bump(&rt_pb.policy_epoch);rt_pb.last_reader_flags=0;rt_pb_report();}
static struct pb_timer_ports rt_pb_timer_ports(void){return(struct pb_timer_ports){head(),mutex(),pthread_mutex_trylock,pthread_mutex_unlock,s.add,s.remove};}
static int rt_pb_cancel_timer(int locked){if(!rt_pb.timer)return 1;
 struct pb_timer_ports api=rt_pb_timer_ports();struct pb_timer_result r;
 pb_timer_remove(&api,rt_pb.timer,locked,&r);if(r.status==PBT_BUSY)return 0;
 if(r.status){rt.gate=0;rt.uncertain=1;rt_pb.last_result=50+r.status;return 0;}
 rt_pb.timer=0;return 1;}
static void rt_pb_pause(uint32_t reason){rt_pb_init();rt_pb.paused=1;rt_pb.pauses++;
 rt_pb.last_cancel=reason;rt_pb_cancel_timer(0);rt_pb_report();}
#ifndef A7_PB_NATIVE_TEST
static uint32_t rt_pb_lookup(uint32_t mm,uint32_t id){
 return(uintptr_t)((void*(*)(void*,uint32_t))(uintptr_t)(s.r.obj_base+0x834ff7))((void*)(uintptr_t)mm,id);}
static int rt_pb_condition(uint32_t v){return((int(*)(void*))(uintptr_t)(rt_pb.view3_base+0x421d1))((void*)(uintptr_t)v);}
static void rt_pb_dtor(void *e){((void*(*)(void*))(uintptr_t)(s.r.obj_base+0x82cffb))(e);}
static void rt_pb_dispatch(void *vm,void *event){
 rt_segment_invalidate();
 uint32_t began=rt_perf_clock();
 ((void(*)(void*,void*))(uintptr_t)(s.r.obj_base+0x8326c9))(vm,event);
 rt_perf_end(RP_TOTAL,began);
 rt_segment_invalidate();
}
/* Negative-only peek deliberately has no refresh/epoch/model call. Its reads
 * use the bounded current memory reader; any failure rejects. The full base
 * refresh plus independent registry snapshot remains mandatory on positive211. */
static int rt_pb_peek_top(uint32_t*out_id,uint32_t*out_view){
 uint32_t vm=0,again=0,m[28],slots[39*4];
 if(!rt_read(s.view_global,&vm,4)||!vm||!rt_read(vm,m,sizeof m)||m[0]!=39||
  m[2]!=s.registry_singleton||!m[8]||(m[8]&3)||!rt_read(m[8],slots,sizeof slots)||
  !rt_read(s.view_global,&again,4)||again!=vm)return 0;
 uint32_t id=0,v=0;for(unsigned i=0;i<39;i++)if(slots[4*i+1]){
  if(slots[4*i+1]&3)return 0;id=slots[4*i+2];v=slots[4*i+1];}
 *out_id=id;*out_view=v;return 1;
}
static int rt_pb_view3_bind(void){
 if(rt_pb.view3_close_uncertain)return 0;
 if(!verified_view3_current())return 0;
 if(rt_pb.view3_ref)return rt_pb.view3_base!=0;
 void*h=dlopen("/usr/lib/viewUnified3.so",6);if(!h)return 0;
 void*f=dlsym(h,"ViewZoomPbToInstance");struct info d={0};
 int ok=f&&dladdr(f,&d)&&d.base&&(uintptr_t)f==(uintptr_t)d.base+0x40c45&&
  (equal(d.file,"/usr/lib/viewUnified3.so")||equal(d.file,"//usr/lib/viewUnified3.so"));
 if(ok)for(unsigned i=0;i<PB_VIEW3_ANCHORS;i++)if(!same((void*)((uintptr_t)d.base+pb_view3_offsets[i]),pb_view3_anchors[i],16)){ok=0;break;}
 if(!ok){if(dlclose(h)){rt.uncertain=1;rt.gate=0;rt_pb.view3_ref=h;rt_pb.view3_close_uncertain=1;}return 0;}
 rt_pb.view3_ref=h;rt_pb.view3_base=(uintptr_t)d.base;return 1;
}
#endif
static int rt_pb_base(uint32_t*);
static int rt_pb_registry(uint32_t*);
static int rt_pb_model(uint32_t,uint32_t,uint32_t*,uint32_t*);
#include "runtime_held_gui.inc"
#include "runtime_pinch_capture.inc"
static int rt_pb_registry(uint32_t *view){
 uint32_t id=0,v=0;if(!rt_quick_top(&id,&v)||id!=211||!v)return 0;
 uint32_t token=rt_segment_usable()?rt_segment.token[0]:0;
 if(token&&rt_pb.registry_token==token&&rt_pb.registry_invalidation==RW(1022)&&rt_pb.registry_view==v){
  const struct a7_menu_snapshot *old=&rt_pb.registry;uint32_t node[6];
  if(rt_read(old->selected_node,node,sizeof node)&&node[3]==211&&
   node[4]==old->selected_module_ptr&&node[5]==old->selected_factory_ptr){*view=v;return 1;}
  rt_pb.registry_token=0;
 }
 if(a7_menu_take_snapshot(snapshot_read,0,s.view_global,s.registry_singleton,211,&rt_pb.registry))return 0;
 const struct a7_menu_snapshot *q=&rt_pb.registry;
 if(q->registry_is_singleton!=1||q->configured_slots!=39||q->selected_count!=1||
 !equal(q->selected_module,"viewUnified3.so")||!equal(q->selected_factory,"ViewZoomPbToInstance"))return 0;
 uint32_t highest=0,top=0,topview=0,n=0;
 for(unsigned i=0;i<q->active_count;i++){const struct a7_menu_view *a=q->active+i;
  if(a->bank==0&&a->index>=highest){highest=a->index;top=a->id;topview=a->view;}
  if(a->bank==0&&a->id==211){if(a->view!=v||a->registry_matches!=1||
   !equal(a->module,"viewUnified3.so")||!equal(a->factory,"ViewZoomPbToInstance"))return 0;
   n++;}}
 if(n!=1||top!=211||topview!=v)return 0;
 rt_pb.registry_token=token;rt_pb.registry_invalidation=RW(1022);rt_pb.registry_view=v;*view=v;return 1;
}
static int rt_pb_model(uint32_t vm,uint32_t id,uint32_t *element,uint32_t *model){
 uint32_t mm=0;if(!rt_read(vm+0x18,&mm,4)||!mm)return 0;
 uint32_t e=rt_pb_lookup(mm,id),m=0;if(!e||(e&3)||e>UINT32_MAX-0x20||!rt_read(e+0x1c,&m,4)||!m||(m&3))return 0;
 *element=e;*model=m;return 1;
}
static int rt_pb_base(uint32_t *vm){uint32_t v=0,vs=0,ws=0,globalws=0,cb=0,mode=0;uint8_t adjust=1;
 if(!rt.active||!rt.gate||rt.uncertain||rt_pb.disabled||!epoch()||tid()!=s.q.ui_tid||!refresh()||
 !rt_read(s.view_global,&v,4)||!v||!rt_read(v+0x14,&vs,4)||vs!=rt.vs||
 !rt_read(vs+8,&ws,4)||ws!=rt.ws||!rt_read(s.r.obj_base+0x129f470,&globalws,4)||globalws!=ws||
 !rt_read(rt.callback_cell,&cb,4)||cb!=(uintptr_t)rt_input_hook||
 !rt_read(s.r.obj_base+0x129f120,&adjust,1)||adjust||
 !rt_read(s.r.obj_base+0x12d4e28,&mode,4)||mode||!rt_power())return 0;
 *vm=v;return 1;
}
#include "runtime_passive_v14.inc"
#include "runtime_diag_v15.inc"
/* Fresh model/current reference, no global media-identity claim. Busy only
 * clears READY; it does not revoke a stable key or a held gesture. */
static int rt_pb_context(PinchContext *out,void **manager,uint32_t *now,int action){
 uint32_t vm=0,v=0,e=0,m=0,vp=0,offset=0,againE=0,againM=0;
 __builtin_memset(out,0,sizeof *out);
 /* A negative-only current registry probe keeps MENU/FFC/SinglePB off the
  * full playback qualification path. A positive result grants no authority;
  * base, registry, typed reference and native capture are all revalidated. */
 uint32_t quick_id=0,quick_view=0;
 if(!rt_pb_peek_top(&quick_id,&quick_view)||quick_id!=211||!quick_view)return 0;
 if(!rt_pb_base(&vm)||!rt_pb_registry(&v)||!rt_pb_view3_bind())return 0;
 if(rt_pb.view_object!=v){rt_pb.view_object=v;if(!rt_pb_bump(&rt_pb.view_epoch))return 0;}
 if(!rt_pb_model(vm,0xe9,&e,&m)||m<0x8a4||!rt_read(m,&vp,4)||vp!=s.r.obj_base+0x11fee30||
 !rt_read(vp-8,&offset,4)||offset!=(uint32_t)-0x8a4)return 0;
 rt_p14_diagnose(v,e,m,m-0x8a4);
 struct pi_source source={s.r.obj_base,rt_pb.view3_base,e,m,m-0x8a4,v,RW(60),rt_pb.policy_epoch,rt_pb.view_epoch,0,7};
 struct pi_read_port reader={0,snapshot_read};struct pi_key key;uint32_t flags=0,cookie=0;
 struct pi_read_detail detail={0};int rc=pi_read_reference_detailed(&reader,&source,&key,&flags,&detail);rt_pb.last_reader_result=rc;rt_pb.last_reader_flags=flags;
 RW(P16_IMAGE_DIAG_FIRST)=0x41374936u;RW(P16_IMAGE_DIAG_FIRST+1)=((uint32_t)rc&255u)|((detail.checked_mask&63u)<<8)|(detail.checked_mask&0x80000000u);RW(P16_IMAGE_DIAG_FIRST+2)=detail.raw_type;RW(P16_IMAGE_DIAG_FIRST+3)=detail.view_phase;
 if(rc||!rt_pb_model(vm,0xe9,&againE,&againM)||againE!=e||againM!=m){p15d_block(&RW(P15D_FIRST),P15D_REFERENCE);return 0;}
 int lease=pi_observe(&rt_pb.image,&key,flags,&cookie);
 if(lease!=PI_NEW&&lease!=PI_SAME&&lease!=PI_CHANGED)return 0;
 if(lease==PI_CHANGED)pinch_cancel(&rt_pb.gesture);
 rt_pb.last_key=key;*manager=(void*)(uintptr_t)vm;*now=rt_now();
 *out=(PinchContext){rt.epoch_tag,RW(60),211,v,key.view_phase,211,cookie,PF_FEED_REQUIRED};
 if(action){uint32_t ze=0,zm=0;
  if(!rt_pb.in_fence||rt_pb.paused||!rt_pb.source_sequence)return 0;
  if(!rt_pb_gui_held(1))return 0;
  if(!(flags&PI_NATIVE_READY)){p15d_block(&RW(P15D_FIRST),P15D_READY);return 0;}
  if(!rt_pb_model(vm,0x59,&ze,&zm)){p15d_block(&RW(P15D_FIRST),P15D_M59);return 0;}
  /* Only the first fence pass may skip canvas work for an impossible move.
   * Keep GUI retry handling above, and apply rate and identity in the plan. */
  if(action==2&&!rt_pb.dispatching&&!pinch_motion_pending(&rt_pb.gesture)){
   p15d_block(&RW(P15D_FIRST),P15D_NO_PLAN);
   return 0;
  }
  if(!rt_pb_capture_current(&key,rt.ws,vm)){p15d_block(&RW(P15D_FIRST),P15D_CAPTURE);return 0;}
  if(action==1){rt_segment_invalidate();int ready=rt_pb_condition(v);rt_segment_invalidate();
   if(!ready){p15d_block(&RW(P15D_FIRST),P15D_CONDITION);return 0;}
  /* The getter may notify/reenter; recheck all base identities and raw key on
   * the next fresh call too. Never use a pre-native snapshot as final proof. */
  PinchContext after={0};void *vm_after=0;uint32_t now_after=0;
  if(rt_pb.paused||!rt_pb_context(&after,&vm_after,&now_after,0)||
   vm_after!=(void*)(uintptr_t)vm||after.view_cookie!=key.view_object||
   after.image_cookie!=cookie||!(rt_pb.last_reader_flags&PI_NATIVE_READY)||
   !rt_pb_gui_held(0)||
   !rt_pb_capture_current(&rt_pb.last_key,rt.ws,vm)){p15d_block(&RW(P15D_FIRST),P15D_AFTER_GETTER);return 0;}
  *out=after;*manager=vm_after;*now=now_after;
   out->flags|=PF_GUI_FENCE|PF_CAPTURE_CURRENT;
  }else out->flags|=PF_GUI_FENCE|PF_CAPTURE_CURRENT;
 }
 return 1;
}
static int rt_pb_fresh(void *opaque,PinchContext *c,void **vm,uint32_t *now){(void)opaque;
 /* The first dispatcher lookup is an allocation precheck. The second, after
  * all8 allocations, performs native condition2 plus another full raw key.
  * There is one original getter per attempted step, not three. */
 unsigned stage=++rt_pb.dispatch_fresh;return rt_pb_context(c,vm,now,stage>=2?1:2);}
static int rt_pb_arm(int locked){
 if(rt_pb.timer||rt_pb.paused||rt_pb.disabled||!rt.active||!rt.gate||rt.uncertain||
 !rt_pb.gesture.started||!rt_pb.gesture.held)return 0;
 struct ts due={0};int rc=s.now(&due);
 if(rc||due.sec<0||due.nsec<0||due.nsec>=1000000000||
  (due.sec==INT32_MAX&&due.nsec>=980000000))return 0;
 if(!rc){due.nsec+=20000000;if(due.nsec>=1000000000){due.nsec-=1000000000;due.sec++;}
  struct pb_timer_ports api=rt_pb_timer_ports();struct pb_timer_result r;
  rt_pb.timer_sequence=rt_pb.source_sequence;pb_timer_add(&api,&due,rt_pb_fence_guard,&rt_pb,locked,&r);
  if(r.handle)rt_pb.timer=r.handle;if(r.status&&r.status!=PBT_BUSY){rt.gate=0;rt.uncertain=1;rt_pb.last_result=50+r.status;}}
 return !rc&&rt_pb.timer;
}
/* f has a private pre-stock header ID. Unknown notifications pause actions but
 * retain held/history; only a subsequent fresh REAL TP can authorize resumption.
 * Real hardware input/navigation revokes the entire gesture. */
static void rt_pb_after_event(struct rt_frame *f){rt_pb_init();if(!f->source_ok)return;
 if(!f->stock_counted||!rt.active||!rt.gate||rt.uncertain){rt_pb_revoke(PB_CANCEL_SOURCE);return;}
 if(f->private_envelope){rt_pb_revoke(PB_CANCEL_KEY);rt_pb_cancel_timer(0);return;}
 if(!f->recognized){if(!f->pb_header_ok||(f->pb_event_id&0xff000000u)==0x15000000u||
  f->pb_event_id==0x11012001||f->pb_event_id==0x11012002){rt_pb_revoke(PB_CANCEL_KEY);rt_pb_cancel_timer(0);}
  else {rt_p15_note_notification(f);rt_pb_pause(PB_PAUSE_NOTIFICATION);}return;}
 if(!f->copy_ok||!f->count||f->count>2){rt_pb_revoke(PB_CANCEL_COPY);rt_pb_cancel_timer(0);return;}
 PinchContext c={0};void *vm=0;uint32_t now=rt_now();int good=rt_pb_context(&c,&vm,&now,0);
 if(!good){rt_pb_revoke(PB_CANCEL_IDENTITY);}
 else{rt_pb.paused=0;rt_pb.defer_left=2;if(!rt_pb_bump(&rt_pb.source_sequence))return;}
 /* memcpy avoids native/C strict-aliasing assumptions for the identical 24B. */
 PinchPacket samples[2];__builtin_memcpy(samples,f->samples,f->count*sizeof(PinchPacket));
 pinch_feed(&rt_pb.gesture,good?&c:0,samples,f->count,now);
 if(good&&rt_pb.gesture.tap_suppressed){input_tap_cancel(&rt.tap);rt_pending.active=0;ffr_revoke(&rt_ffc);rt_cancel_native_timer(0);}
 if(good&&!rt_pb.paused&&rt_pb.gesture.started&&__builtin_popcount((unsigned)rt_pb.gesture.held)==2)rt_pb_arm(0);
 else {if(good&&__builtin_popcount((unsigned)rt_pb.gesture.held)==2)p15d_block(&RW(P15D_FIRST),P15D_BLOCKED_BEFORE_ARM);rt_pb_cancel_timer(0);}rt_pb_report();
}
void rt_pb_fence_impl(void *record,struct ts *due,void *data,uint32_t lr){(void)due;
 if(!record||data!=&rt_pb||record!=rt_pb.timer)return;__atomic_add_fetch(&rt_pb.inflight,1,__ATOMIC_ACQ_REL);
 uint32_t dispatch=0;if(!epoch()||tid()!=s.q.ui_tid||lr!=s.expected_return||
 !rt_read((uintptr_t)s.manager+0x20,&dispatch,4)||dispatch!=1){
  rt.uncertain=1;rt.gate=0;rt_pb_revoke(PB_CANCEL_SOURCE);__atomic_sub_fetch(&rt_pb.inflight,1,__ATOMIC_RELEASE);return;}
 if(!rt_pb_cancel_timer(1)){__atomic_sub_fetch(&rt_pb.inflight,1,__ATOMIC_RELEASE);return;}rt_pb.frames++;
 /* A newer fully-stock-processed TP sample is allowed; insisting on equality
  * would starve a 20ms fence when the panel emits motion faster than 50Hz. */
 if(rt_pb.paused||!rt_pb.timer_sequence||!rt_pb.source_sequence||!rt.active||!rt.gate||rt.uncertain){__atomic_sub_fetch(&rt_pb.inflight,1,__ATOMIC_RELEASE);return;}
 rt_pb.in_fence=1;rt_pb.retry_queue=0;PinchContext c={0};void *vm=0;uint32_t now=0;PinchPlan plan;
 int qualified=rt_pb_context(&c,&vm,&now,2);
 if(qualified&&!pinch_plan(&rt_pb.gesture,&c,now,&plan)){p15d_block(&RW(P15D_FIRST),P15D_NO_PLAN);qualified=0;}
 if(qualified){
  PinchDispatchPorts ports={1,0,rt_pb_fresh,malloc,free,rt_pb_dtor,rt_pb_dispatch};
  rt_pb.dispatching=1;rt_pb.dispatch_fresh=0;pinch_dispatch(&rt_pb.gesture,&plan,&ports,&rt_pb.result);rt_pb.dispatching=0;
  rt_pb.last_result=rt_pb.result.status;if(rt_pb.result.called)rt_pb.actions++;
  if(rt_pb.result.uncertain){rt.uncertain=1;rt.gate=0;rt_pb_revoke(PB_CANCEL_THROW);}
 }
 rt_pb.in_fence=0;
 /* Only a GUI-queue defer can rearm, at most twice per fresh real TP. Native
  * busy/unknown notifications/capture failure never gain this authorization. */
 if(rt_pb.retry_queue&&rt_pb.defer_left&&!rt_pb.paused&&!rt.uncertain&&rt.active&&rt.gate){rt_pb.defer_left--;rt_pb_arm(1);}
 __atomic_sub_fetch(&rt_pb.inflight,1,__ATOMIC_RELEASE);rt_pb_report();rt_publish();
}
void rt_pb_fence_failed(void *record,struct ts *due,void *data,uint32_t lr){(void)due;
 if(!record||data!=&rt_pb||lr!=s.expected_return)return;rt.gate=0;rt.uncertain=1;
 rt_pb.in_fence=rt_pb.dispatching=0;if(__atomic_load_n(&rt_pb.inflight,__ATOMIC_ACQUIRE))__atomic_sub_fetch(&rt_pb.inflight,1,__ATOMIC_RELEASE);rt_pb_revoke(PB_CANCEL_THROW);
 if(rt_pb.timer==record)rt_pb_cancel_timer(1);rt_pb_report();
}
static int rt_pb_detach(int locked){rt_pb_revoke(PB_CANCEL_LIFECYCLE);int removed=rt_pb_cancel_timer(locked);
 if(removed){PinchState empty={0};empty.config=rt_pb.gesture.config;empty.serial=rt_pb.gesture.serial;
  empty.uncertain=rt_pb.gesture.uncertain;rt_pb.gesture=empty;}
 /* Only local contact state is cleared. Never alter Sony capture/buttons.
  * Keep policy/image generations monotonic across suspend/resume. */
 return removed;}
static int rt_pb_retired(void){return !rt_pb.timer&&!__atomic_load_n(&rt_pb.inflight,__ATOMIC_ACQUIRE)&&!rt_pb.in_fence&&!rt_pb.dispatching;}
static int rt_pb_cleanup(void){if(!rt_pb_retired()||rt_pb.view3_close_uncertain)return 0;
 if(rt_pb.view3_ref){void*h=rt_pb.view3_ref;if(dlclose(h)){rt_pb.view3_close_uncertain=1;return 0;}rt_pb.view3_ref=0;rt_pb.view3_base=0;}return 1;}
static void rt_pb_reset_unstarted(void){__builtin_memset(&rt_pb,0,sizeof rt_pb);}
