#include <stdint.h>
#define rt_segment_invalidate test_profile_invalidate
#define rt_perf_clock test_profile_clock
#define rt_perf_end test_profile_end
#include "runtime_perf.h"

extern "C" int printf(const char *, ...);
static struct { struct { uintptr_t obj_base; } r; } s;
static unsigned order[6], order_count;
static uint32_t now_us, profiled_slot, profiled_us;
static bool throw_from_wheel;
static void *expected_manager, *expected_event;

static void wheel(void *manager, void *event) {
    order[order_count++] = (manager == expected_manager && event == expected_event) ? 3 : 99;
    now_us += 137;
    if (throw_from_wheel)
        throw 7;
}

extern "C" void rt_segment_invalidate(void) { order[order_count++] = 1; }
extern "C" uint32_t rt_perf_clock(void) { order[order_count++] = 2; return now_us; }
extern "C" void rt_perf_end(uint32_t slot, uint32_t began) {
    order[order_count++] = 4;
    profiled_slot = slot;
    profiled_us = now_us - began;
}

#include "actual_dispatch.inc"

extern "C" int pb_dispatch_profile_selftest(void) {
    expected_manager = (void *)(uintptr_t)1;
    expected_event = (void *)(uintptr_t)2;
    s.r.obj_base = (uintptr_t)&wheel - 0x8326c9u;
    order_count = 0;
    now_us = 1000;
    rt_pb_dispatch(expected_manager, expected_event);
    if (order_count != 5 || order[0] != 1 || order[1] != 2 || order[2] != 3 ||
        order[3] != 4 || order[4] != 1 || profiled_slot != 15 || profiled_us != 137)
        return 1;

    order_count = 0;
    profiled_slot = profiled_us = 0;
    throw_from_wheel = true;
    try {
        rt_pb_dispatch(expected_manager, expected_event);
        return 1;
    } catch (int value) {
        if (value != 7 || order_count != 3 || order[0] != 1 || order[1] != 2 ||
            order[2] != 3 || profiled_slot || profiled_us)
            return 1;
    }
    printf("PB dispatch profile cases=2 normal_only=1 slot=15\n");
    return 0;
}
