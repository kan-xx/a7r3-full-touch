#include <stdint.h>
#include <stddef.h>
#ifndef A7_ARM_MIN_DECL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
extern void *malloc(size_t);extern void free(void*);
extern void *memcpy(void*,const void*,size_t);extern void *memset(void*,int,size_t);
extern int memcmp(const void*,const void*,size_t);
extern int printf(const char*,...);extern int strcmp(const char*,const char*);
#endif
#include "hook_bridge.h"
#include "runtime_perf.h"
#include "tap_session.h"
#include "image_reference_read.h"
#include "native_catalog.h"
#include "menu_snapshot.h"
struct ts{int32_t sec,nsec;};
enum {OBJ=0x10000000,V3=0x18000000,VIEW=0x20000000,TOP=0x21000000,
 V2=0x14000000,PARENT=0x2afffe40,ELEMENT=0x22000000,VM=0x23000000,VS=0x24000000,WS=0x25000000,
 MM=0x26000000,MSS=0x31000000,MANAGER=0x27000000,GLOBAL=0x10000100,CALLBACK=0x30000000};
static struct {struct {uint32_t pid,ui_tid;}q;struct{uint32_t obj_base,lib_base;}r;uint32_t view_global,registry_singleton,expected_return;void*manager;
 int(*now)(struct ts*);void*(*add)(void*,struct ts*,void(*)(void*,struct ts*,void*),void*);void(*remove)(void*,void*);}s;
static struct {uint32_t active,gate,uncertain,vs,ws,callback_cell,epoch_tag,release_cookie,release_ms,cookie;InputTapState tap;struct{uint32_t word[1024];}report;}rt;
static struct{uint32_t active;}rt_pending;static unsigned rt_ffc;
#define RW(i) rt.report.word[i]
#define rt_input_hook ((void*)(uintptr_t)CALLBACK)
struct cell {uint32_t address,size;uint8_t bytes[4096];};static struct cell cells[32];static unsigned ncells;
static unsigned checks,cases,error_line,stock_calls,publishes,adds,removes,condition_calls,dispatch_calls,snapshot_calls;
static unsigned now_ms,tid_value,power_good,epoch_good,maps_good,bind_good,capture_good,queue_busy;
static unsigned stock_throw,copy_throw,rt_inflight,condition_throw,refresh_calls;
static unsigned top_id,registry_bad,lookup_bad,condition_good,condition_change,condition_pause;
static unsigned condition_gui_change,allocation_gui_change;
static unsigned native_render_change,getter_calls,getter_throw,getter_change,getter_mutation,hit_calls,hit_mutation,hit_mutation_kind,lookup59_bad;
static unsigned allocations,releases,fail_allocate,lock_bad,lock_depth,native_throw,remove_throw,add_throw,close_bad,close_calls;
static struct rt_frame input;
static void *native_timer;static void(*native_timer_callback)(void*,struct ts*,void*);static void*native_timer_data;
extern void test_throw_native(void);
#define CK(x) do{checks++;if(!(x)){error_line=__LINE__;return 1;}}while(0)
static void place(uint32_t a,const void*b,unsigned n){
 for(unsigned i=0;i<ncells;i++)if(a>=cells[i].address&&a+n<=cells[i].address+cells[i].size){memcpy(cells[i].bytes+a-cells[i].address,b,n);return;}
 cells[ncells].address=a;cells[ncells].size=n;memcpy(cells[ncells++].bytes,b,n);
}
static void word(uint32_t a,uint32_t v){place(a,&v,4);}
static int snapshot_read(void*x,uint32_t a,void*out,uint32_t n){(void)x;
 for(unsigned i=0;i<ncells;i++)if(a>=cells[i].address&&n<=cells[i].size&&a-cells[i].address<=cells[i].size-n){memcpy(out,cells[i].bytes+a-cells[i].address,n);return 1;}return 0;}
static int rt_read(uint32_t a,void*out,uint32_t n){return snapshot_read(0,a,out,n);}
static int equal(const char*a,const char*b){return !strcmp(a,b);}
static int same(const void*a,const void*b,unsigned n){return !memcmp(a,b,n);}
static int epoch(void){return epoch_good;}static uint32_t tid(void){return tid_value;}
static int refresh(void){refresh_calls++;return maps_good;}static int rt_power(void){RW(60)=9;return power_good;}
static uint32_t rt_now(void){return now_ms;}
static int rt_quick_top(uint32_t*i,uint32_t*v){*i=top_id;*v=VIEW;return maps_good;}
int a7_menu_take_snapshot(a7_menu_read rd,void*x,uint32_t vm,uint32_t reg,uint32_t id,struct a7_menu_snapshot*q){(void)rd;(void)x;(void)vm;(void)reg;
 snapshot_calls++;
 memset(q,0,sizeof*q);q->registry_is_singleton=1;q->configured_slots=39;q->selected_count=1;
 memcpy(q->selected_module,"viewUnified3.so",16);memcpy(q->selected_factory,"ViewZoomPbToInstance",21);
 q->active_count=1;q->active[0].id=id;q->active[0].view=VIEW;q->active[0].registry_matches=1;
 memcpy(q->active[0].module,"viewUnified3.so",16);memcpy(q->active[0].factory,"ViewZoomPbToInstance",21);
 q->selected_node=0x2a000000;q->selected_module_ptr=0x2a000100;q->selected_factory_ptr=0x2a000200;
 if(registry_bad==1)q->selected_count=2;if(registry_bad==2)q->active[0].registry_matches=0;
 if(registry_bad==3)q->selected_module[0]='X';return 0;}
static InputTapFence current_fence;
static int rt_drain_snapshot(InputTapFence*f){*f=current_fence;if(queue_busy)f->count=1;return 1;}
static struct{uint32_t token[1];}rt_segment={{1}};
static int rt_segment_usable(void){return 1;}
void rt_segment_invalidate(void){RW(1022)++;}
uint32_t rt_segment_enter(uint32_t kind){(void)kind;rt_segment_invalidate();return 1;}
void rt_segment_leave(uint32_t token){(void)token;rt_segment_invalidate();}
uint32_t rt_perf_clock(void){return now_ms*1000;}
void rt_perf_end(uint32_t a,uint32_t b){(void)a;(void)b;}
static void rt_publish(void){publishes++;}
static void ffr_revoke(void*x){(void)x;}
static int rt_cancel_native_timer(int locked){(void)locked;return 1;}
static void *head(void){return(void*)(uintptr_t)(MANAGER+4);}
static void *mutex(void){return(void*)(uintptr_t)(MANAGER+8);}
static int pthread_mutex_trylock(void*x){(void)x;if(lock_bad)return 1;lock_depth++;return 0;}
static int pthread_mutex_unlock(void*x){(void)x;if(lock_depth)lock_depth--;return 0;}
static int fake_now(struct ts*t){t->sec=now_ms/1000;t->nsec=(now_ms%1000)*1000000;return 0;}
static void *fake_add(void*h,struct ts*t,void(*cb)(void*,struct ts*,void*),void*d){(void)h;(void)t;if(add_throw)test_throw_native();adds++;native_timer=(void*)(uintptr_t)(0x70000000+adds*32);native_timer_callback=cb;native_timer_data=d;return native_timer;}
static void fake_remove(void*h,void*r){(void)h;if(remove_throw)test_throw_native();if(r==native_timer)native_timer=0;removes++;}
static void *allocate(size_t n){allocations++;if(allocation_gui_change&&allocations==8)word(MSS+4+12,4);if(allocations==fail_allocate)return 0;return malloc(n);}
static void release(void*p){releases++;free(p);}
static int dlclose(void*h){(void)h;close_calls++;return close_bad;}
static uint32_t rt_pb_lookup(uint32_t mm,uint32_t id){(void)mm;if(lookup_bad||(id==0x59&&lookup59_bad))return 0;return id==0xe9?ELEMENT:id==0x59?ELEMENT+0x100:0;}
static int rt_pb_peek_top(uint32_t*i,uint32_t*v){return rt_quick_top(i,v);}
static uint32_t rt_pb_native_get_widget(uint32_t,uint32_t);
static int rt_pb_view3_bind(void);
static int rt_pb_condition(uint32_t v);
static void rt_pb_dtor(void *p);
static void rt_pb_dispatch(void *vm,void *p);
static int rt_native_hit(void*x,uint32_t w,const struct surface_point*p){(void)x;hit_calls++;if(hit_mutation==hit_calls){if(hit_mutation_kind==1)word(VIEW+4,4);else if(hit_mutation_kind==2)queue_busy=1;else if(hit_mutation_kind==3)word(MSS+4+12,4);else power_good=0;}return capture_good&&(w==0x2b000000||w==PARENT)&&p->x>=0&&p->x<640&&p->y>=0&&p->y<480;}
#define A7_PB_NATIVE_TEST
#define malloc allocate
#define free release
#include "runtime_pinch.inc"
#undef malloc
#undef free
static uint32_t rt_pb_native_get_widget(uint32_t view,uint32_t hash){getter_calls++;if(getter_throw)test_throw_native();if(view!=VIEW||hash!=PBC_CANVAS_RESOURCE)return 0;if(getter_mutation&&getter_calls==1){if(getter_mutation==1)power_good=0;if(getter_mutation==2)top_id=175;if(getter_mutation==3)word(TOP+0x1bc,77);if(getter_mutation==4)rt_pb.gesture.serial++;if(getter_mutation==5)rt_pb.source_sequence++;}if(getter_change&&getter_calls==2)return PARENT+0x1000;return PARENT;}
static int rt_pb_view3_bind(void){rt_pb.view3_base=V3;return bind_good;}
static int rt_pb_condition(uint32_t v){(void)v;condition_calls++;if(condition_throw)test_throw_native();if(condition_change)word(TOP+0x1bc,77);if(condition_pause)rt_pb_pause(PB_PAUSE_NOTIFICATION);if(condition_gui_change==1)word(MSS+4+12,4);else if(condition_gui_change==2)queue_busy=1;return condition_good;}
struct Ev{uint32_t*refs,id;uint8_t dest,category,pad[2];void*params;};
struct Pa{void*vector;uint32_t*refs;uint8_t*sorted;uint32_t*next;uint8_t*flag;};
static void rt_pb_dtor(void*p){struct Ev*e=p;struct Pa*l=e->params;release(e->refs);release(l->vector);release(l->refs);release(l->sorted);release(l->next);release(l->flag);release(l);}
static void rt_pb_dispatch(void*vm,void*p){struct Ev*e=p;(void)vm;dispatch_calls++;
 if(e->id!=0x130d3003&&e->id!=0x130d3004)native_throw=1;
 if(e->dest!=4||e->category||*e->refs!=1)native_throw=1;
 if(native_throw)test_throw_native();if(native_render_change){word(TOP+0x19c,800);word(TOP+0x1a0,600);word(VIEW+4,4);}}
static void original_stock(void*event){(void)event;stock_calls++;if(stock_throw)test_throw_native();}
rt_stock_cb rt_hook_begin(void*e,uint32_t lr,struct rt_frame*f){(void)e;rt_inflight++;f->source_ok=1;f->lr=lr;RW(16)++;return original_stock;}
void rt_hook_copy(void*event,struct rt_frame*f){uint32_t source=f->source_ok,lr=f->lr;*f=*(struct rt_frame*)event;f->source_ok=source;f->lr=lr;f->stock_counted=0;if(copy_throw)test_throw_native();}
static void rt_receive(void*p){(void)p;}
static void rt_native_leases(void){}
static int rt_current(InputTapContext*c,int r){(void)c;(void)r;return 0;}
static int rt_manual_scope_possible(void){return 0;}
static int rt_arm(int x){(void)x;return 0;}
uint32_t ts_event(InputTapState*a,uint32_t*b,uint32_t*c,const A7R3InfraInputEvent*d,uint32_t e,int f,const InputTapContext*g,uint32_t h){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;return 0;}
void tr_observed_impl(void*a,struct ts*b,void*c,uint32_t d){(void)a;(void)b;(void)c;(void)d;}
void tr_observed_failed(void*a,struct ts*b,void*c,uint32_t d){(void)a;(void)b;(void)c;(void)d;}
void rt_fence_impl(void*a,struct ts*b,void*c,uint32_t d){(void)a;(void)b;(void)c;(void)d;}
void rt_fence_failed(void*a,struct ts*b,void*c,uint32_t d){(void)a;(void)b;(void)c;(void)d;}
#include "actual_post.inc"
extern void pb_test_input_hook(void*);
extern int test_try_hook(void*);
extern int test_try_pb_fence(void*,void*,uint32_t);
static void reset(void){memset(&rt_pb,0,sizeof rt_pb);memset(&rt,0,sizeof rt);memset(&s,0,sizeof s);memset(&input,0,sizeof input);ncells=0;
 stock_calls=publishes=adds=removes=condition_calls=dispatch_calls=allocations=releases=snapshot_calls=0;
 now_ms=100;tid_value=392;power_good=epoch_good=maps_good=bind_good=capture_good=condition_good=1;stock_throw=copy_throw=rt_inflight=condition_throw=refresh_calls=0;rt_p14_attempt=rt_p14_nonzero=0;
 condition_gui_change=allocation_gui_change=0;getter_calls=getter_throw=getter_change=getter_mutation=hit_calls=hit_mutation=hit_mutation_kind=lookup59_bad=0;native_render_change=queue_busy=registry_bad=lookup_bad=condition_change=condition_pause=fail_allocate=lock_bad=lock_depth=native_throw=remove_throw=add_throw=close_bad=close_calls=0;top_id=211;
 s.q.ui_tid=392;s.r.obj_base=OBJ;s.r.lib_base=V2;s.view_global=GLOBAL;s.registry_singleton=0x28000000;s.expected_return=0x82c267;s.manager=(void*)(uintptr_t)MANAGER;s.now=fake_now;s.add=fake_add;s.remove=fake_remove;
 rt.active=rt.gate=1;rt.vs=VS;rt.ws=WS;rt.callback_cell=VS+0x14;rt.epoch_tag=1;
 uint8_t zero[4096]={0};place(TOP,zero,0x928);place(VM,zero,128);place(VS,zero,64);place(WS,zero,0x500);place(ELEMENT,zero,0x200);place(VIEW,zero,8);
 word(GLOBAL,VM);word(VM+0x14,VS);word(VM+0x18,MM);word(VS+8,WS);word(VS+0x14,CALLBACK);
 word(OBJ+0x129f470,WS);word(OBJ+0x129f120,0);word(OBJ+0x12d4e28,0);word(MANAGER+0x20,1);
 word(ELEMENT+0x1c,TOP+0x8a4);word(ELEMENT+0x11c,0x29000000);
 word(TOP+0x8a4,OBJ+0x11fee30);word(OBJ+0x11fee30-8,(uint32_t)-0x8a4);
 word(TOP,OBJ+0x11fecf8);word(TOP+0x11c,OBJ+0xdc1f08);word(TOP+0x20,3);
 word(TOP+0x114,0xffffffffu);word(TOP+0x118,0xffffffffu);word(TOP+0x194,7952);word(TOP+0x198,5304);word(TOP+0x1bc,84000000);word(TOP+0x184,5);
 place(TOP+0x1cc,"100MSDCF",9);place(TOP+0x1d6,"DSC00001.JPG",13);
 word(VIEW,V3+0xa8f48);word(VIEW+4,3);memset(&current_fence,0,sizeof current_fence);current_fence.gui_token=-1;current_fence.mouse0_buttons=1;
 uint32_t mss[33]={0};mss[0]=OBJ+0x11ed420;for(unsigned i=0;i<4;i++){mss[1+8*i]=OBJ+0x11efd38;mss[2+8*i]=i;mss[3+8*i]=i;mss[4+8*i]=i<2?3:0;}place(MSS,mss,sizeof mss);word(OBJ+0x129ee60,MSS);uint8_t enabled=1;place(OBJ+0x129ee55,&enabled,1);current_fence.gui_token=0;
 uint32_t record[6]={0,0,0,211,0x2a000100,0x2a000200};place(0x2a000000,record,sizeof record);
 place(PARENT,zero,0x5a4);word(PARENT,V2+0x8bf418);word(PARENT+0x1c,PARENT);word(PARENT+0x14c,211);word(PARENT+0x38,2);word(PARENT+0x590,0x31);word(PARENT+0x594,0x30);word(PARENT+0x5a0,0x33);word(0x2b000000+0x10,PARENT+4);word(0x2b000000+0x15c,PARENT);word(0x2b000000+0x160,V2+0x57e629);word(0x2b000000,OBJ+0x11ed7a0);word(0x2b000024,77);word(0x2b000038,2);word(0x2b00014c,211);
 for(unsigned i=0;i<21;i++){word(0x2b000000+0x2a4+8*i,0x11012004);word(0x2b000000+0x2a4+8*i+4,(PBC_NATIVE_MASK>>i)&1u);}
 const uint8_t anchor[16]={0x2d,0xe9,0xf0,0x43,0x83,0xb0,0x06,0x46,0x00,0xaf,0x88,0x46,0x05,0x46,0x00,0xf1};place(OBJ+0x3da894,anchor,16);
 word(WS+0x4b0,1);word(WS+0x4b4,0x2b000000);word(WS+0x4b8,0x2b000000);
 native_timer=0;native_timer_callback=0;native_timer_data=0;rt_pb_init();}
static void event(unsigned id,int x,int state,unsigned tick){struct rt_frame f={0};f.source_ok=f.stock_counted=f.recognized=f.copy_ok=f.pb_header_ok=1;f.pb_event_id=0x11013008;f.count=1;
 f.samples[0].source_code=0x11013001;f.samples[0].x=x;f.samples[0].y=100;f.samples[0].state=state;f.samples[0].flags=id;f.samples[0].timestamp_ticks=tick;
 now_ms+=16;pb_test_input_hook(&f);}
static void begin(void){event(0,100,1,10);event(1,200,1,11);event(1,240,0x80000001,12);}
static void fire(void){void*r=rt_pb.timer;now_ms+=20;rt_segment_invalidate();rt_pb_fence_impl(r,0,&rt_pb,s.expected_return);rt_segment_invalidate();}
static void notice(uint32_t id){struct rt_frame f={0};f.source_ok=f.stock_counted=f.pb_header_ok=1;f.pb_event_id=id;pb_test_input_hook(&f);}
int runtime_pinch_selftest(void){checks=cases=error_line=0;
 cases++;reset();word(TOP+0x184,15);begin();fire();CK(dispatch_calls==1&&rt_pb.image.generation==1&&rt_pb.last_key.raw_type==15);CK(RW(904)==0x41374936u&&RW(905)==0x80000000u&&RW(906)==15&&RW(907)==3);
 cases++;reset();word(TOP+0x184,15);begin();native_render_change=1;fire();now_ms+=128;event(1,290,0x80000001,24);fire();CK(dispatch_calls==2&&rt_pb.image.generation==1&&rt_pb.last_key.view_phase==4);CK(RW(906)==15&&RW(907)==4);
 cases++;reset();begin();word(TOP+0x184,15);fire();CK(!dispatch_calls&&rt_pb.image.generation==2&&rt_pb.gesture.blocked&&!rt_pb.timer);CK(rt_pb.last_key.raw_type==15&&RW(905)==0x80000000u);
 cases++;reset();word(TOP+0x184,15);begin();word(TOP+0x184,5);fire();CK(!dispatch_calls&&rt_pb.image.generation==2&&rt_pb.gesture.blocked&&!rt_pb.timer);CK(rt_pb.last_key.raw_type==5);
 cases++;reset();word(TOP+0x184,18);begin();CK(!dispatch_calls&&!rt_pb.image.generation&&!rt_pb.timer&&rt_pb.last_reader_result==PIR_TYPED);CK(RW(905)==(0x80000000u|(16u<<8)|PIR_TYPED)&&RW(906)==18);
 cases++;reset();word(TOP+0x184,15);word(TOP+0x11c,0);begin();CK(!dispatch_calls&&!rt_pb.image.generation&&!rt_pb.timer&&rt_pb.last_reader_result==PIR_TYPED);CK(RW(905)==(0x80000000u|(4u<<8)|PIR_TYPED)&&RW(906)==15);
 cases++;reset();word(TOP+0x184,15);word(VIEW+4,9);begin();CK(!dispatch_calls&&!rt_pb.image.generation&&!rt_pb.timer&&rt_pb.last_reader_result==PIR_TYPED);CK(RW(905)==(0x80000000u|(32u<<8)|PIR_TYPED)&&RW(907)==9);
 cases++;reset();rt_pb_reset_unstarted();CK(rt_pb_cleanup()&&!rt_pb.initialized);rt_pb_init();CK(rt_pb.initialized);
 cases++;reset();begin();CK(rt_pb.gesture.held==3&&rt_pb.timer&&adds==1);CK(rt_pb.gesture.context.layout==211);unsigned snapshots_before=snapshot_calls;fire();CK(dispatch_calls==1&&releases==8&&rt_pb.result.returned&&!rt_pb.timer&&!rt_pb.inflight);CK(rt_pb.image.generation==1);CK(condition_calls==1&&snapshot_calls-snapshots_before==10&&getter_calls==8&&hit_calls==16);
 cases++;reset();begin();word(VIEW+4,4);fire();CK(dispatch_calls==1);now_ms+=128;event(1,290,0x80000001,24);fire();CK(dispatch_calls==2&&rt_pb.image.generation==1);
 cases++;reset();begin();capture_good=0;fire();CK(!dispatch_calls&&!condition_calls&&!rt_pb.timer&&rt_pb.last_result==PB_CAPTURE_UNPROVEN+PBC_HIT);
 for(unsigned n=1;n<=8;n++){cases++;reset();begin();fail_allocate=n;fire();CK(!dispatch_calls&&rt_pb.result.status==4&&releases==n-1);}
 cases++;reset();begin();queue_busy=1;fire();CK(!dispatch_calls&&rt_pb.timer&&rt_pb.defer_left==1);queue_busy=0;fire();CK(dispatch_calls==1&&!rt_pb.timer);
 cases++;reset();begin();queue_busy=1;fire();fire();fire();CK(!dispatch_calls&&!rt_pb.timer&&rt_pb.defer_left==0&&adds==3);
 cases++;reset();begin();notice(0x18000042);CK(rt_pb.paused&&!rt_pb.timer&&rt_pb.gesture.held==3);now_ms+=200;CK(!dispatch_calls);event(1,260,0x80000001,26);fire();CK(dispatch_calls==1);
 cases++;reset();begin();notice(0x150200a2);CK(rt_pb.gesture.blocked&&!rt_pb.timer);event(1,260,0x80000001,14);CK(!rt_pb.timer);event(0,-1,0,15);event(1,-1,0,16);CK(!rt_pb.gesture.held);event(0,100,1,17);event(1,200,1,18);event(1,240,0x80000001,19);fire();CK(dispatch_calls==1);
 cases++;reset();begin();word(TOP+0x88,1);fire();CK(!dispatch_calls&&!rt_pb.timer&&rt_pb.image.valid&&rt_pb.image.generation==1);word(TOP+0x88,0);event(1,260,0x80000001,14);fire();CK(dispatch_calls==1);
 cases++;reset();begin();condition_change=1;fire();CK(!dispatch_calls&&rt_pb.gesture.blocked);
 cases++;reset();begin();condition_pause=1;fire();CK(!dispatch_calls&&rt_pb.paused&&!rt_pb.timer);
 for(unsigned n=1;n<=3;n++){cases++;reset();begin();registry_bad=n;fire();CK(!dispatch_calls);}
 for(unsigned n=0;n<7;n++){cases++;reset();begin();if(n==0)power_good=0;if(n==1)epoch_good=0;if(n==2)maps_good=0;if(n==3)word(OBJ+0x129f120,1);if(n==4)word(OBJ+0x12d4e28,1);if(n==5)word(VS+0x14,0);if(n==6)top_id=175;fire();CK(!dispatch_calls);}
 cases++;reset();begin();void *old=rt_pb.timer;CK(rt_pb_detach(1));CK(rt_pb_retired()&&!rt_pb.timer);rt_pb_fence_impl(old,0,&rt_pb,s.expected_return);CK(!dispatch_calls);
 cases++;reset();begin();lock_bad=1;notice(0x18000042);CK(rt_pb.paused&&rt_pb.timer&&!rt_pb_retired());lock_bad=0;fire();CK(!dispatch_calls&&rt_pb_retired());
 cases++;reset();begin();rt_pb_fence_impl(rt_pb.timer,0,&rt_pb,123);CK(rt.uncertain&&!rt.gate&&rt_pb.timer&&!rt_pb_retired());
 cases++;reset();begin();native_throw=1;fire();CK(rt.uncertain&&!rt.gate&&rt_pb.result.retained_event&&rt_pb.result.uncertain&&!rt_pb.inflight);
 cases++;reset();begin();remove_throw=1;notice(0x18000042);CK(!lock_depth&&rt.uncertain&&!rt.gate&&rt_pb.timer&&!rt_pb_retired());
 cases++;reset();add_throw=1;event(0,100,1,10);event(1,200,1,11);CK(!lock_depth&&rt.uncertain&&!rt.gate);
 cases++;reset();begin();power_good=0;CK(rt_pb_detach(1)&&!rt_pb.gesture.held&&!rt_pb.timer);power_good=1;now_ms+=200;begin();fire();CK(dispatch_calls==1&&rt_pb.image.generation==2);
 cases++;reset();rt_pb.view3_ref=(void*)1;close_bad=1;CK(!rt_pb_cleanup()&&close_calls==1&&rt_pb.view3_close_uncertain);CK(!rt_pb_cleanup()&&close_calls==1);
 cases++;reset();copy_throw=1;event(0,100,1,10);CK(stock_calls==1&&RW(17)==1&&!rt_inflight&&!rt_pb.timer);
 cases++;reset();begin();stock_throw=1;struct rt_frame failed={0};CK(test_try_hook(&failed)==1&&stock_calls==4&&RW(17)==4&&!rt_inflight&&!rt.gate);CK(rt_pb_detach(1)&&!rt_pb.gesture.held&&!rt_pb.timer);
 for(unsigned n=0;n<3;n++){cases++;reset();begin();notice(0x18000042);if(n==0)word(TOP+0x194,55);if(n==1)top_id=175;if(n==2)power_good=0;event(1,260,0x80000001,14);fire();CK(!dispatch_calls);}
 cases++;reset();top_id=175;event(0,100,1,10);CK(!refresh_calls&&!snapshot_calls&&!rt_pb.timer&&!condition_calls);
 cases++;reset();begin();condition_throw=1;fire();CK(condition_calls==1&&rt.uncertain&&!rt.gate&&!rt_pb.inflight&&!dispatch_calls&&rt_pb.result.status==7&&releases==8);
 cases++;reset();unsigned before=rt_pb.frames;rt_pb_fence_impl(0,0,&rt_pb,s.expected_return);CK(!rt_pb.frames&&before==0&&!rt.uncertain);
 cases++;reset();begin();capture_good=0;fire();uint32_t retained[36];memcpy(retained,&RW(564),sizeof retained);CK(retained[10]==0x2b000000&&retained[4]==PBC_HIT);top_id=175;event(1,260,0x80000001,14);CK(!memcmp(retained,&RW(564),sizeof retained));
 cases++;reset();begin();native_render_change=1;fire();CK(dispatch_calls==1&&rt_pb.last_key.reference_kind==PI_REF_STREAM_FILE&&rt_pb.last_key.file_size==84000000);now_ms+=128;event(1,290,0x80000001,24);fire();CK(dispatch_calls==2&&rt_pb.image.generation==1&&rt_pb.last_key.view_phase==4);
 cases++;reset();uint8_t absent=0;place(TOP+0x1d6,&absent,1);begin();CK(!rt_pb.source_sequence&&!rt_pb.image.generation&&!dispatch_calls&&!condition_calls&&rt_pb.last_reader_result==PIR_NAME);CK(RW(P14_DIAG_START+10)==0x2b000000&&RW(P14_DIAG_START+13)==211&&RW(P14_DIAG_START+22)==0xffffffffu&&RW(P14_DIAG_START+23)==0xffffffffu);CK((RW(P14_DIAG_START+4)&255u)==0);uint32_t passive[28];memcpy(passive,&RW(P14_DIAG_START),sizeof passive);top_id=175;event(0,110,0x80000001,15);CK(!memcmp(passive,&RW(P14_DIAG_START),sizeof passive));
 cases++;reset();place(TOP+0x1d6,&absent,1);word(0x2b000000,0x12345678);begin();CK(!dispatch_calls&&!rt_pb.source_sequence&&(RW(P14_DIAG_START+4)&255u)==P14D_UNKNOWN_CLASS&&RW(P14_DIAG_START+11)==0x12345678&&!RW(P14_DIAG_START+13)&&!RW(P14_DIAG_START+16));
 for(unsigned n=1;n<=5;n++){cases++;reset();begin();getter_mutation=n;fire();CK(!dispatch_calls&&!condition_calls&&getter_calls==1&&!hit_calls&&rt_pb.last_result==PB_CAPTURE_UNPROVEN+PBC_SCOPE);}
 for(unsigned n=1;n<=4;n++){cases++;reset();begin();hit_mutation=n;fire();CK(!dispatch_calls&&!condition_calls&&getter_calls==1&&hit_calls==n&&rt_pb.last_result==PB_CAPTURE_UNPROVEN+PBC_SCOPE);}
 for(unsigned n=1;n<=4;n++){cases++;reset();begin();hit_mutation=n;hit_mutation_kind=1;fire();CK(!dispatch_calls&&!condition_calls&&getter_calls==1&&hit_calls==n&&rt_pb.last_result==PB_CAPTURE_UNPROVEN+PBC_SCOPE);}
 cases++;reset();begin();getter_change=1;fire();CK(!dispatch_calls&&getter_calls==2&&rt_pb.last_result==PB_CAPTURE_UNPROVEN+PBC_CHANGED);
 cases++;reset();begin();uint8_t bad_anchor=0;place(OBJ+0x3da894,&bad_anchor,1);fire();CK(!dispatch_calls&&!getter_calls&&rt_pb.last_result==PB_CAPTURE_UNPROVEN+PBC_RESOURCE);
 cases++;reset();begin();getter_throw=1;void*rec=rt_pb.timer;/* actualcaughtUI wrapper supplies LR fixture via helper */CK(test_try_pb_fence(rec,&rt_pb,s.expected_return)==1);CK(rt.uncertain&&!rt.gate&&!rt_pb.inflight&&!rt_pb.timer&&!dispatch_calls);
 cases++;reset();begin();current_fence.gui_token=1;word(MSS+4+4,1);word(MSS+4+8*4+4,0);fire();CK(dispatch_calls==1&&RW(253)==1&&RW(1010)==1&&RW(1009)==0);
 cases++;reset();begin();current_fence.gui_token=-1;fire();CK(!dispatch_calls&&!getter_calls&&rt_pb.last_result==41&&RW(667)==P15D_GUI_TOKEN&&RW(241)==0x41374746);
 cases++;reset();begin();queue_busy=1;current_fence.gui_token=-1;fire();CK(!dispatch_calls&&!rt_pb.timer&&!rt_pb.retry_queue&&rt_pb.last_result==41);
 cases++;reset();begin();word(MSS+4+12,4);fire();CK(!dispatch_calls&&!getter_calls&&rt_pb.last_result==41&&((RW(242)>>16)&255)==PBG_RECORD_STATE);
 cases++;reset();begin();condition_gui_change=1;fire();CK(!dispatch_calls&&condition_calls==1&&releases==8&&((RW(242)>>8)&255)==PBG_TOKEN&&((RW(242)>>16)&255)==PBG_RECORD_STATE);
 cases++;reset();begin();CK(RW(732)==0x41375052&&RW(733)==PARENT&&RW(734)==V2+0x8bf418&&RW(735)==PARENT&&RW(736)==V2+0x57e629&&RW(737)==0x31&&RW(738)==0x30&&RW(739)==0x33);fire();CK(dispatch_calls==1&&RW(732)==0x41375043);uint32_t strict_parent[8];memcpy(strict_parent,&RW(732),sizeof strict_parent);event(1,290,0x80000001,24);CK(!memcmp(strict_parent,&RW(732),sizeof strict_parent));
 cases++;reset();begin();hit_mutation=1;hit_mutation_kind=2;fire();CK(!dispatch_calls&&!rt_pb.timer&&!rt_pb.retry_queue&&hit_calls==1&&((RW(242)>>8)&255)==PBG_QUEUE);
 cases++;reset();begin();hit_mutation=1;hit_mutation_kind=3;fire();CK(!dispatch_calls&&!rt_pb.timer&&!rt_pb.retry_queue&&hit_calls==1&&((RW(242)>>8)&255)==PBG_TOKEN);
 cases++;reset();begin();condition_gui_change=2;fire();CK(!dispatch_calls&&!rt_pb.timer&&!rt_pb.retry_queue&&condition_calls==1&&releases==8&&((RW(242)>>8)&255)==PBG_QUEUE);
 cases++;reset();begin();allocation_gui_change=1;fire();CK(!dispatch_calls&&!rt_pb.timer&&!rt_pb.retry_queue&&!condition_calls&&releases==8&&((RW(242)>>8)&255)==PBG_TOKEN);
 cases++;reset();begin();lookup59_bad=1;fire();CK(!dispatch_calls&&RW(667)==P15D_M59&&!getter_calls);
 cases++;reset();begin();condition_good=0;fire();CK(!dispatch_calls&&RW(667)==P15D_CONDITION&&releases==8);
 cases++;reset();event(0,100,1,10);event(1,200,1,11);fire();CK(!dispatch_calls&&RW(667)==P15D_NO_PLAN);
 cases++;reset();begin();notice(0x18000042);CK(RW(663)==P15D_MAGIC&&RW(664)==0x18000042&&RW(666)==1&&rt_pb.paused&&!rt_pb.timer);
#include "v17_cases.inc"
 return 0;
}
extern int capture_selftest(void);extern int held_gui_selftest(void);
extern int pb_geometry_selftest(void);
extern int pb_dispatch_profile_selftest(void);
int runtime_pinch_test_entry(void){int rc=runtime_pinch_selftest();if(!rc)rc=capture_selftest();if(!rc)rc=held_gui_selftest();if(!rc)rc=pb_geometry_selftest();if(!rc)rc=pb_dispatch_profile_selftest();printf("runtime_pinch cases=%u checks=%u error=%u\n",cases,checks,error_line);if(rc)printf("PB last=%u capstatus=%u cap=%u vp=%u owner=%u mode=%u visible=%u slots=%u/%u held=%u source=%u native=%u allocations=%u\n",rt_pb.last_result,RW(568),RW(574),RW(575),RW(577),RW(578),RW(579),RW(581),RW(582),rt_pb.gesture.held,rt_pb.source_sequence,dispatch_calls,allocations);return rc;}
#ifndef A7_ARM_MIN_DECL
int main(void){return runtime_pinch_test_entry();}
#endif
