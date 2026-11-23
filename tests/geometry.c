#include "pinch.h"
#include <stdio.h>

static PinchState make_gesture(unsigned baseline, int dx, int dy, const PinchContext *ctx) {
    PinchState st;
    PinchConfig cfg = pinch_default_config();
    pinch_init(&st, &cfg);
    st.started = 1;
    st.held = 3;
    st.last[0].x = 100;
    st.last[0].y = 100;
    st.last[1].x = (int16_t)(100 + dx);
    st.last[1].y = (int16_t)(100 + dy);
    st.baseline2 = baseline * baseline;
    st.last_feed_ms = 100;
    st.context = *ctx;
    return st;
}

int pb_geometry_selftest(void) {
    /* Exact squared-distance boundaries:
     *   expand:   64 * span2 >= 81 * baseline2  (ratio >= 9/8 = 1.125x)
     *   contract: 81 * span2 <= 64 * baseline2  (ratio <= 8/9 = 0.8889x)
     *   min_span: span2 >= 24^2 = 576
     */
    const struct {
        unsigned baseline;
        int dx, dy;
        unsigned kind;
    } boundaries[] = {
        {96, 107, 0, PP_NONE},     /* 107 < 96*9/8 = 108 -> NO_PLAN */
        {96, 108, 0, PP_EXPAND},   /* 108 == 96*9/8 -> EXPAND */
        {90, 81, 0, PP_NONE},      /* 81 > 90*8/9 = 80 -> NO_PLAN */
        {90, 80, 0, PP_CONTRACT},  /* 80 == 90*8/9 -> CONTRACT */
        {27, 24, 0, PP_CONTRACT},  /* 24 == min_span -> CONTRACT */
        {27, 23, 0, PP_NONE},      /* 23 < min_span (24) -> NO_PLAN */
        {90, 48, 64, PP_CONTRACT}, /* hypot(48,64)=80 == 90*8/9 -> CONTRACT */
        {96, 0, 108, PP_EXPAND},   /* vertical 108 == 96*9/8 -> EXPAND */
    };
    const PinchContext context = {1, 1, 211, 2, 3, 211, 3, PF_ACTION_REQUIRED};
    unsigned checks = 0;

#define CHECK(expr)                                                   \
    do {                                                              \
        ++checks;                                                     \
        if (!(expr)) {                                                \
            printf("PB geometry failed at line %d\n", __LINE__);      \
            return 1;                                                 \
        }                                                             \
    } while (0)

    for (unsigned i = 0; i < sizeof(boundaries) / sizeof(boundaries[0]); ++i) {
        PinchState st = make_gesture(boundaries[i].baseline, boundaries[i].dx, boundaries[i].dy, &context);
        PinchPlan plan = {0};
        int want_ok = boundaries[i].kind != PP_NONE;
        int pending = pinch_motion_pending(&st);
        int plan_ok = pinch_plan(&st, &context, 100, &plan);
        CHECK(pending == want_ok);
        CHECK(plan_ok == want_ok);
        CHECK(plan.kind == boundaries[i].kind);
    }

    /* Cadence (128ms) and stale-feed (512ms) guards */
    PinchState st = make_gesture(96, 108, 0, &context);
    PinchPlan plan = {0};
    st.action_seen = 1;
    st.last_action_ms = 100;
    CHECK(pinch_motion_pending(&st) == 1);
    CHECK(!pinch_plan(&st, &context, 227, &plan)); /* 127ms < 128ms cadence */
    CHECK(pinch_plan(&st, &context, 228, &plan) && plan.kind == PP_EXPAND);
    CHECK(!pinch_plan(&st, &context, 613, &plan)); /* 613 - 100 = 513ms > 512ms max_gap */

    PinchContext incomplete = context;
    incomplete.flags = PF_FEED_REQUIRED;
    CHECK(!pinch_plan(&st, &incomplete, 228, &plan));

    st.blocked = 1;
    CHECK(!pinch_motion_pending(&st));
    st.blocked = 0;
    st.held = 1; /* single finger only */
    CHECK(!pinch_motion_pending(&st));
    CHECK(!pinch_motion_pending(0));

    printf("PB geometry checks=%u\n", checks);
#undef CHECK
    return 0;
}
