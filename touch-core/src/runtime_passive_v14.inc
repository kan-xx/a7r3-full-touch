/* Integrated passive snapshot (NOT_HW_QUALIFIED): after current211 base/
 * registry/V3/typedE9, before reference read. This cannot authorize pinch. */
#include "pinch_passive.h"
#ifndef A7_PB_NATIVE_TEST
_Static_assert(700u+ENVELOPE_WORDS<=P14_DIAG_START,"envelope report cannot overlap passive");
_Static_assert(740u+sizeof(struct sync_result)/sizeof(uint32_t)<=P14_DIAG_START,"sync report cannot overlap passive");
#endif
#include "runtime_passive_parent.inc"
static uint32_t rt_p14_attempt,rt_p14_nonzero;
static void rt_p14_diagnose(uint32_t view,uint32_t element,uint32_t model,uint32_t complete){
 if(rt_p14_attempt!=UINT32_MAX)rt_p14_attempt++;
 struct p14_diag_source source={s.r.obj_base,rt_pb.view3_base,view,rt.ws,element,model,complete,RW(60),rt_p14_attempt,rt_now()};
 struct p14_diag_port port={0,snapshot_read};struct p14_diag report;p14_passive_read(&port,&source,&report);rt_pb_passive_parent(&report);
 if(report.word[10]||!rt_p14_nonzero){__builtin_memcpy(&RW(P14_DIAG_START),&report,sizeof report);if(report.word[10])rt_p14_nonzero=1;}
}
