/* Fresh native image-canvas membership. No historical object whitelist. */
#include "pinch_capture.h"
#ifndef A7_PB_NATIVE_TEST
_Static_assert(700u+ENVELOPE_WORDS<=PBC_PARENT_DIAG_FIRST,"envelope ends before parent diagnostics");
#endif
struct rt_pb_capture_context {struct pi_key key;uint32_t serial,view_manager;};
#ifndef A7_PB_NATIVE_TEST
static uint32_t rt_pb_native_get_widget(uint32_t view,uint32_t hash){return(uintptr_t)((void*(*)(void*,uint32_t))(uintptr_t)(s.r.obj_base+0x3da895))((void*)(uintptr_t)view,hash);}
#endif
static uint32_t rt_pb_capture_resolve(void*x,uint32_t view,uint32_t hash){(void)x;
 static const uint8_t anchor[16]={0x2d,0xe9,0xf0,0x43,0x83,0xb0,0x06,0x46,0x00,0xaf,0x88,0x46,0x05,0x46,0x00,0xf1};uint8_t live[16];
 if(hash!=PBC_CANVAS_RESOURCE||!rt_read(s.r.obj_base+0x3da894,live,16)||!same(live,anchor,16))return 0;
 rt_segment_invalidate();uint32_t parent=rt_pb_native_get_widget(view,hash);rt_segment_invalidate();return parent;
}
static int rt_pb_capture_scope(void*x,const struct pbc_source*source){struct rt_pb_capture_context*c=x;uint32_t vm=0,v=0,e=0,m=0,ze=0,zm=0;
 if(!c||!rt_pb.in_fence||rt_pb.paused||rt_pb.policy_epoch!=c->key.policy_epoch||rt_pb.view_epoch!=c->key.view_epoch||rt_pb.source_sequence!=source->source_sequence||rt_pb.gesture.serial!=c->serial||rt_pb.gesture.held!=source->held||!rt_pb.image.valid||rt_pb.image.generation!=source->cookie)return 0;
 if(!rt_pb_base(&vm)||vm!=c->view_manager||!rt_pb_registry(&v)||v!=source->view||v!=c->key.view_object||rt.ws!=source->ws||RW(60)!=source->power||RW(60)!=c->key.packed_power||s.r.lib_base!=source->view2||rt_pb.view3_base!=source->view3||!rt_pb_view3_bind())return 0;
 if(!rt_pb_model(vm,0xe9,&e,&m)||e!=c->key.e9_element||m!=c->key.e9_model)return 0;
 struct pi_source ps={s.r.obj_base,rt_pb.view3_base,e,m,c->key.e9_complete,v,RW(60),rt_pb.policy_epoch,rt_pb.view_epoch,0,7};struct pi_read_port rp={0,snapshot_read};struct pi_key actual,expected=c->key;uint32_t flags=0;
 if(pi_read_reference(&rp,&ps,&actual,&flags)||!(flags&PI_NATIVE_READY)||actual.view_phase!=source->phase)return 0;
 actual.view_phase=expected.view_phase;/* reader already requires phase3/4 */
 if(!same(&actual,&expected,sizeof actual)||!rt_pb_model(vm,0x59,&ze,&zm))return 0;
 if(!rt_pb_gui_held(0))return 0;
 uint32_t id=0,again=0;return rt_pb_peek_top(&id,&again)&&id==211&&again==v&&!rt_pb.paused&&rt_pb.gesture.serial==c->serial&&rt_pb.source_sequence==source->source_sequence;
}
static int rt_pb_capture_hit(void*x,uint32_t w,int16_t x0,int16_t y0){(void)x;struct surface_point p={(float)x0,(float)y0};return rt_native_hit(0,w,&p);}
static int rt_pb_capture_current(const struct pi_key *key,uint32_t ws,uint32_t vm){
 struct rt_pb_capture_context context={*key,rt_pb.gesture.serial,vm};
 struct pbc_source source={s.r.obj_base,rt_pb.view3_base,key->view_object,key->view_phase,ws,rt_pb.gesture.held,rt_pb.source_sequence,key->packed_power,rt_pb.image.generation,rt_now(),{0,0},{0,0},s.r.lib_base};
 unsigned n=0;for(unsigned i=0;i<16;i++)if(rt_pb.gesture.held&(1u<<i)){if(n==2)break;source.x[n]=rt_pb.gesture.last[i].x;source.y[n]=rt_pb.gesture.last[i].y;n++;}
 struct pbc_ports ports={&context,snapshot_read,rt_pb_capture_hit,rt_pb_capture_resolve,rt_pb_capture_scope};struct pbc_evidence evidence;int rc=pbc_current(&ports,&source,&evidence);rt_pb.capture_attempts++;evidence.word[2]=rt_pb.capture_attempts;
 InputTapFence f={0};if(rt_drain_snapshot(&f)){evidence.word[32]=f.count;evidence.word[33]=f.head;evidence.word[34]=f.tail;evidence.word[35]=f.gui_token;}
 if(evidence.word[10]||!rt_pb.capture_has_nonzero){__builtin_memcpy(&RW(564),evidence.word,sizeof evidence.word);__builtin_memcpy(&RW(PBC_PARENT_DIAG_FIRST),evidence.parent,sizeof evidence.parent);if(evidence.word[10])rt_pb.capture_has_nonzero=1;}
 rt_pb.last_result=rc?PB_CAPTURE_UNPROVEN+rc:0;return rc==PBC_OK;
}
