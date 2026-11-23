#include "input_tap.h"

static uint32_t next_serial(uint32_t serial) {
    serial++;
    return serial ? serial : 1u;
}
static int valid_config(const InputTapConfig *c) {
    return c && c->width && c->height && c->width <= 32767u &&
        c->height <= 32767u && c->max_distance && c->max_distance <= 1024u &&
        c->max_duration_ms && c->max_duration_ms <= 4096u;
}
static int allowed(const InputTapContext *c, uint32_t required) {
    return c && c->epoch && c->view && c->page &&
        (c->flags & required) == required;
}
static int same_context(const InputTapContext *a, const InputTapContext *b) {
    return a->epoch == b->epoch && a->view == b->view &&
        a->page == b->page && a->generation == b->generation;
}
static int valid_point(const InputTapState *s, const A7R3InfraInputEvent *p) {
    return p->x >= 0 && p->y >= 0 && (uint16_t)p->x < s->config.width &&
        (uint16_t)p->y < s->config.height;
}
static int displaced(const InputTapState *s, const A7R3InfraInputEvent *p) {
    int32_t dx = (int32_t)p->x - s->candidate.origin_x;
    int32_t dy = (int32_t)p->y - s->candidate.origin_y;
    uint32_t limit = s->config.max_distance;
    /* Coordinates are checked before this call: products fit signed32. */
    return (uint32_t)(dx * dx + dy * dy) > limit * limit;
}
static int expired(const InputTapState *s, const A7R3InfraInputEvent *p,
                   uint32_t now) {
    return now - s->start_ms > s->config.max_duration_ms ||
        a7r3_tp_elapsed_ms(s->candidate.start_tick, p->timestamp_ticks) >
            s->config.max_duration_ms;
}

InputTapConfig input_tap_default_config(void) {
    InputTapConfig c = {640, 480, 12, 0, 512};
    return c;
}
int input_tap_init(InputTapState *s, const InputTapConfig *c) {
    InputTapState empty = {0};
    if (!s) return 0;
    *s = empty;
    s->serial = 1;
    if (!valid_config(c)) return 0;
    s->config = *c;
    return 1;
}
uint32_t input_tap_cancel(InputTapState *s) {
    uint32_t effect;
    if (!s) return INPUT_TAP_NONE;
    effect = s->phase == INPUT_TAP_WAIT_FENCE ? INPUT_TAP_CANCEL_TIMER : 0;
    s->serial = next_serial(s->serial);
    s->timer_cookie = 0;
    s->due_ms = 0;
    s->attempts = 0;
    s->phase = s->held_mask ? INPUT_TAP_BLOCKED : INPUT_TAP_IDLE;
    return effect;
}

uint32_t input_tap_feed(InputTapState *s, const A7R3InfraInputEvent *p,
                        const InputTapContext *ctx, uint32_t now) {
    uint32_t effect = INPUT_TAP_NONE;
    uint16_t bit;
    uint32_t contact;
    int down, end;
    if (!s || !p) return INPUT_TAP_NONE;
    if (!valid_config(&s->config)) return input_tap_cancel(s);
    if (p->source_code != A7R3_RAW_TP_SOURCE) return input_tap_cancel(s);
    contact = a7r3_tp_contact(p);
    bit = (uint16_t)(1u << contact);
    down = a7r3_tp_native_down_or_move(p->state);
    end = a7r3_tp_native_up_or_cancel(p->state);
    if (!down && !end) return input_tap_cancel(s);

    /* Update contact bookkeeping even on an ineligible page. A held finger
     * crossing into a new page must not become a fresh press there. */
    if (down) {
        if (s->phase == INPUT_TAP_WAIT_FENCE) effect |= input_tap_cancel(s);
        s->held_mask = (uint16_t)(s->held_mask | bit);
        if (!allowed(ctx, INPUT_TAP_FEED_REQUIRED)) {
            effect |= input_tap_cancel(s);
            return effect;
        }
        if (s->phase == INPUT_TAP_BLOCKED) return effect;
        if (s->held_mask != bit) {
            effect |= input_tap_cancel(s);
            return effect;
        }
        if (s->phase == INPUT_TAP_IDLE) {
            s->serial = next_serial(s->serial);
            s->candidate.context = *ctx;
            s->candidate.origin_x = s->candidate.x = p->x;
            s->candidate.origin_y = s->candidate.y = p->y;
            s->candidate.contact = (uint16_t)contact;
            s->candidate.start_tick = p->timestamp_ticks & 0x7fffu;
            s->candidate.end_tick = s->candidate.start_tick;
            s->candidate.duration_ms = 0;
            s->candidate.serial = s->serial;
            s->candidate.reserved = 0;
            s->start_ms = now;
            s->phase = INPUT_TAP_TRACKING;
        }
        if (!same_context(ctx, &s->candidate.context) ||
            s->candidate.contact != contact || !valid_point(s, p) ||
            displaced(s, p) || expired(s, p, now)) {
            effect |= input_tap_cancel(s);
            return effect;
        }
        s->candidate.x = p->x;
        s->candidate.y = p->y;
        s->candidate.end_tick = p->timestamp_ticks & 0x7fffu;
        return effect;
    }

    s->held_mask = (uint16_t)(s->held_mask & (uint16_t)~bit);
    if (s->phase == INPUT_TAP_WAIT_FENCE) {
        /* A duplicate normal release of the same contact is harmless. An
         * unobserved second contact or explicit cancellation revokes the tap. */
        if (!a7r3_tp_clean_release(p->state) ||
            s->candidate.contact != contact ||
            !allowed(ctx, INPUT_TAP_FEED_REQUIRED) ||
            !same_context(ctx, &s->candidate.context))
            effect |= input_tap_cancel(s);
        return effect;
    }
    if (s->phase == INPUT_TAP_BLOCKED) {
        if (!s->held_mask) s->phase = INPUT_TAP_IDLE;
        return effect;
    }
    if (s->phase != INPUT_TAP_TRACKING) return effect;
    if (s->candidate.contact != contact || s->held_mask ||
        !a7r3_tp_clean_release(p->state) ||
        !allowed(ctx, INPUT_TAP_FEED_REQUIRED) ||
        !same_context(ctx, &s->candidate.context) || expired(s, p, now)) {
        effect |= input_tap_cancel(s);
        return effect;
    }
    /* Native MSS also releases at the last held position. Release packet
     * coordinates can be -1/-1 or the no-touch sentinel and are not consulted. */
    s->candidate.end_tick = p->timestamp_ticks & 0x7fffu;
    s->candidate.duration_ms =
        a7r3_tp_elapsed_ms(s->candidate.start_tick, p->timestamp_ticks);
    s->phase = INPUT_TAP_WAIT_FENCE;
    s->timer_cookie = s->serial;
    s->due_ms = now + INPUT_TAP_FENCE_DELAY_MS;
    s->attempts = 0;
    return effect | INPUT_TAP_ARM20;
}

enum InputTapFenceResult input_tap_fence(InputTapState *s, uint32_t cookie,
    const InputTapContext *ctx, const InputTapFence *queue, uint32_t now,
    InputTapCandidate *out) {
    if (!s || s->phase != INPUT_TAP_WAIT_FENCE || !cookie ||
        cookie != s->timer_cookie) return INPUT_TAP_FENCE_IGNORE;
    if (!queue || !out || !allowed(ctx, INPUT_TAP_FENCE_REQUIRED) ||
        !same_context(ctx, &s->candidate.context) || s->held_mask ||
        s->candidate.serial != s->serial) {
        (void)input_tap_cancel(s);
        return INPUT_TAP_FENCE_CANCEL;
    }
    s->attempts++;
    if ((int32_t)(now - s->due_ms) < 0 ||
        queue->count || queue->head || queue->tail || queue->mouse0_buttons ||
        queue->gui_token != -1) {
        if (s->attempts < INPUT_TAP_FENCE_MAX_ATTEMPTS) {
            s->due_ms = now + INPUT_TAP_FENCE_DELAY_MS;
            return INPUT_TAP_FENCE_REARM20;
        }
        (void)input_tap_cancel(s);
        return INPUT_TAP_FENCE_CANCEL;
    }
    *out = s->candidate;
    (void)input_tap_cancel(s);
    return INPUT_TAP_FENCE_COMMIT;
}
