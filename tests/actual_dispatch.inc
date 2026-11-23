static void rt_pb_dispatch(void *vm,void *event){
 rt_segment_invalidate();
 uint32_t began=rt_perf_clock();
 ((void(*)(void*,void*))(uintptr_t)(s.r.obj_base+0x8326c9))(vm,event);
 rt_perf_end(RP_TOTAL,began);
 rt_segment_invalidate();
}
