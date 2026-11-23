/* Offline-built tap/fence core. No event injection, native calls or allocation. */
#ifndef A7R3_INPUT_TAP_H
#define A7R3_INPUT_TAP_H
#include <stdint.h>
#include "native_raw_tp_abi.h"

enum {
    INPUT_TAP_ELIGIBLE = 1u,
    INPUT_TAP_UI_OWNER = 2u,
    INPUT_TAP_RUNNING = 4u,
    INPUT_TAP_TIMER_PHASE = 8u,
    INPUT_TAP_FEED_REQUIRED = 7u,
    INPUT_TAP_FENCE_REQUIRED = 15u,
    INPUT_TAP_FENCE_DELAY_MS = 20u,
    INPUT_TAP_FENCE_MAX_ATTEMPTS = 4u
};

/* Caller's independently verified page identity, not a native pointer owner.
 * epoch/view/page must be nonzero. generation distinguishes a recycled page.
 * Flags attest the current call's guards; none is remembered as authorization.
 */
typedef struct {
    uint32_t epoch, view, page, generation, flags;
} InputTapContext;

typedef struct {
    uint16_t width, height, max_distance;
    uint16_t reserved;
    uint32_t max_duration_ms;
} InputTapConfig;

typedef struct {
    InputTapContext context;
    int16_t origin_x, origin_y, x, y;
    uint16_t contact, start_tick, end_tick, reserved;
    uint32_t duration_ms, serial;
} InputTapCandidate;

/* Snapshot must be collected on the verified Framework TimerManager phase.
 * An arbitrary thread seeing count/head/tail==0 is insufficient: the final
 * popped node can still be in its GUI handler. See native-phase-evidence.json.
 */
typedef struct {
    uint32_t count, head, tail;
    uint32_t mouse0_buttons;
    int32_t gui_token;
} InputTapFence;

enum InputTapPhase {
    INPUT_TAP_IDLE, INPUT_TAP_TRACKING, INPUT_TAP_BLOCKED, INPUT_TAP_WAIT_FENCE
};
enum InputTapEffect {
    INPUT_TAP_NONE = 0,
    INPUT_TAP_ARM20 = 1u,
    INPUT_TAP_CANCEL_TIMER = 2u
};
enum InputTapFenceResult {
    INPUT_TAP_FENCE_IGNORE,
    INPUT_TAP_FENCE_REARM20,
    INPUT_TAP_FENCE_COMMIT,
    INPUT_TAP_FENCE_CANCEL
};

typedef struct {
    InputTapConfig config;
    InputTapCandidate candidate;
    uint32_t phase, serial, timer_cookie, due_ms, attempts, start_ms;
    uint16_t held_mask;
    uint16_t reserved;
} InputTapState;

/* Invalid config returns0 and leaves a disabled state. Default:640x480,
 * Euclidean displacement<=12px, duration<=512ms, at most4 future20ms fences.
 * All now_ms values use the same wrapping monotonic millisecond clock.
 */
InputTapConfig input_tap_default_config(void);
int input_tap_init(InputTapState *, const InputTapConfig *);

/* Glue copies blob1B then17 before calling stock; stock is called exactly once
 * outside this module. Feed those local24B copies in1B->17 order after stock.
 * A new down invalidates a pending candidate immediately, including a17 down
 * after a1B release in the same Event. Process the WHOLE Event before arming.
 * If any feed returnsCANCEL_TIMER, cancel the existing native one-shot; arm
 * only when the final phase isWAIT_FENCE and its final cookie is still current.
 * Release coordinates may be absent: use the last valid held position.
 */
uint32_t input_tap_feed(InputTapState *, const A7R3InfraInputEvent *,
                        const InputTapContext *, uint32_t now_ms);

/* Cancel/revoke on power quiescence, owner/page exit or uninstall. No native
 * timer cancellation is performed here; caller consumes the returned effect.
 * An active contact stays BLOCKED until all known contacts have ended.
 */
uint32_t input_tap_cancel(InputTapState *);

/* cookie is the one captured when the current native one-shot was armed.
 * REARM20 means create another future20ms one-shot, same cookie, no worker.
 * An early native callback (the stock dispatcher allows2ms lookahead) consumes
 * one attempt and REARMs20; it never commits early. Stale cookie:IGNORE.
 * Wrong phase/context/owner/running: CANCEL. At most4 actual timer callbacks.
 * COMMIT is one-shot: candidate_out is copied, then pending state is revoked.
 * Caller must recheck its native menu/root/node guards before issuing action.
 */
enum InputTapFenceResult input_tap_fence(InputTapState *, uint32_t cookie,
    const InputTapContext *, const InputTapFence *, uint32_t now_ms,
    InputTapCandidate *candidate_out);
#endif
