#include "native_mode_policy.h"
#include <stddef.h>

static void zero_bytes(void *destination, size_t count) {
    unsigned char *p = destination;
    while (count--) *p++ = 0;
}

struct target_spec { uint32_t resource, view_id, class_kind; };
static int spec(uint32_t target, struct target_spec *s) {
    if (!s) return 0;
    switch (target) {
    case NMP_IROIRO_MENU_SELECT:
        s->resource = 0xdd716810u; s->view_id = 0x60;
        s->class_kind = NMP_CLASS_MENU_SELECT; return 1;
    case NMP_IROIRO_BAR_DIAL:
        s->resource = 0xf2bb65b2u; s->view_id = 0x60;
        s->class_kind = NMP_CLASS_BAR_DIAL; return 1;
    case NMP_FN_OUTER_BELT:
        s->resource = 0xe22901f2u; s->view_id = 0x44;
        s->class_kind = NMP_CLASS_FN_BELT; return 1;
    default: return 0;
    }
}
static void increment(uint32_t *x) { if (*x != UINT32_MAX) ++*x; }
static int valid_policy(const struct nmp_policy *p) {
    struct target_spec s; return p && spec(p->target, &s);
}
static int valid_ports(const struct nmp_ports *p) {
    return p && p->current && p->touchable && p->mode;
}
static int same(const struct nmp_identity *a, const struct nmp_identity *b) {
    return a->view == b->view && a->widget == b->widget &&
        a->runtime_id == b->runtime_id && a->resource_id == b->resource_id &&
        a->class_kind == b->class_kind &&
        a->view_runtime_id == b->view_runtime_id &&
        a->scope_epoch == b->scope_epoch &&
        a->power_generation == b->power_generation;
}
static int shape(const struct target_spec *s, const struct nmp_identity *i) {
    const uint32_t proof = NMP_CURRENT_VIEW | NMP_TYPED_NATIVE | NMP_PROVEN_PORTS;
    return i->view && i->widget && i->resource_id == s->resource &&
        i->view_runtime_id == s->view_id && i->class_kind == s->class_kind &&
        i->scope_epoch && i->power_generation &&
        i->current_mode <= 3 && i->default_mode <= 3 &&
        (i->flags & proof) == proof;
}
static int enable_guard(const struct nmp_identity *i) {
    return (i->flags & NMP_ENABLE_FLAGS) == NMP_ENABLE_FLAGS && !i->buttons_down;
}
static int restore_guard(const struct nmp_identity *i) {
    return (i->flags & NMP_RESTORE_FLAGS) == NMP_RESTORE_FLAGS && !i->buttons_down;
}
static void clear_record(struct nmp_policy *p) {
    zero_bytes(&p->saved_record, sizeof(p->saved_record)); p->tracked = 0;
}
static void retire(struct nmp_policy *p, enum nmp_retire_reason why) {
    if (p->tracked) {
        increment(&p->stats.retired); increment(&p->stats.unrestored_total);
        p->stats.last_retire_reason = (uint32_t)why;
    }
    clear_record(p);
}
static int deferred(struct nmp_policy *p) {
    increment(&p->stats.deferred); return NMP_DEFERRED;
}
static int uncertain(struct nmp_policy *p) {
    increment(&p->stats.write_uncertain);
    retire(p, NMP_RETIRE_WRITE_UNCERTAIN); return NMP_UNAVAILABLE;
}
static int read_current(const struct nmp_ports *ports, const struct target_spec *s,
                        struct nmp_identity *out) {
    int r; zero_bytes(out, sizeof(*out));
    r = ports->current(ports->context, s->resource, out);
    return r == 1 && shape(s, out) ? 1 : r == 0 ? 0 : -1;
}
static int native_reset(const struct nmp_identity *i) {
    /* Original default2 and no remaining active mode0: native code has
     * definitively superseded our default0. Do not overwrite its gray/hidden. */
    return i->default_mode == 2 && i->current_mode != 0;
}
static void account_native_reset(struct nmp_policy *p) {
    increment(&p->stats.native_reset); clear_record(p);
}
static enum nmp_retire_reason mismatch_reason(const struct nmp_policy *p,
                                              const struct nmp_identity *i) {
    return p->saved_record.power_generation != i->power_generation ?
        NMP_RETIRE_POWER : NMP_RETIRE_IDENTITY;
}

int nmp_init(struct nmp_policy *p, enum nmp_target target) {
    struct target_spec s;
    if (!p || !spec((uint32_t)target, &s)) return 0;
    zero_bytes(p, sizeof(*p)); p->target = (uint32_t)target; p->accept_new = 1;
    return 1;
}
int nmp_start(struct nmp_policy *p) {
    if (!valid_policy(p)) return 0;
    p->accept_new = 1; return 1;
}
void nmp_abandon_current(struct nmp_policy *p, enum nmp_retire_reason reason) {
    if (!valid_policy(p)) return;
    retire(p, reason == NMP_RETIRE_NONE ? NMP_RETIRE_STOP_UNAVAILABLE : reason);
}

int nmp_enable_current(const struct nmp_ports *ports, struct nmp_policy *p) {
    struct target_spec s; struct nmp_identity before, fresh, after; int r, called;
    if (!valid_policy(p)) return NMP_INVALID;
    if (!p->accept_new) return NMP_STOPPED;
    if (!valid_ports(ports)) return NMP_INVALID;
    (void)spec(p->target, &s);
    r = read_current(ports, &s, &before);
    if (r != 1) {
        retire(p, r == 0 ? NMP_RETIRE_ABSENT : NMP_RETIRE_UNREADABLE);
        return r == 0 ? NMP_IDLE : NMP_UNAVAILABLE;
    }
    if (p->tracked && !same(&before, &p->saved_record))
        retire(p, mismatch_reason(p, &before));
    if (p->tracked) {
        if (native_reset(&before)) account_native_reset(p);
        else if (before.default_mode != 0) retire(p, NMP_RETIRE_NATIVE_CHANGED);
        else if (before.current_mode == 0)
            return enable_guard(&before) ? NMP_CURRENT_ENABLED : deferred(p);
        else return deferred(p); /* Current native gray/hidden/disabled. */
    }
    if (!enable_guard(&before)) return deferred(p);
    if (before.current_mode == 0 && before.default_mode == 0) {
        increment(&p->stats.already_native); return NMP_ALREADY_NATIVE;
    }
    if (before.current_mode != 2 || before.default_mode != 2) return deferred(p);
    /* Resolve again immediately before mutation. Never convert a saved token
     * into this: a replacement during discovery is deferred to the next turn. */
    r = read_current(ports, &s, &fresh);
    if (r != 1) return r == 0 ? NMP_IDLE : NMP_UNAVAILABLE;
    if (!same(&before, &fresh) || !enable_guard(&fresh) ||
        fresh.current_mode != 2 || fresh.default_mode != 2) return deferred(p);
    p->saved_record = fresh; p->tracked = 1; /* Potential native-data change. */
    called = ports->touchable(ports->context, &fresh, 1);
    r = read_current(ports, &s, &after);
    if (!called || r != 1 || !same(&fresh, &after) ||
        after.current_mode != 0 || after.default_mode != 0) return uncertain(p);
    increment(&p->stats.applied);
    return enable_guard(&after) ? NMP_APPLIED : deferred(p);
}

int nmp_stop_current(const struct nmp_ports *ports, struct nmp_policy *p) {
    struct target_spec s; struct nmp_identity before, fresh, after, gray_fresh;
    uint32_t final_mode; int r, called;
    if (!valid_policy(p)) return NMP_INVALID;
    p->accept_new = 0;
    if (!p->tracked) return NMP_IDLE;
    if (!valid_ports(ports)) return NMP_INVALID;
    (void)spec(p->target, &s);
    r = read_current(ports, &s, &before);
    if (r != 1) {
        retire(p, r == 0 ? NMP_RETIRE_ABSENT : NMP_RETIRE_UNREADABLE);
        return r == 0 ? NMP_RETIRED : NMP_UNAVAILABLE;
    }
    if (!same(&before, &p->saved_record)) {
        retire(p, mismatch_reason(p, &before)); return NMP_RETIRED;
    }
    if (native_reset(&before)) { account_native_reset(p); return NMP_NATIVE_RESET; }
    if (before.default_mode != 0) {
        retire(p, NMP_RETIRE_NATIVE_CHANGED); return NMP_RETIRED;
    }
    if (!restore_guard(&before)) return deferred(p);
    r = read_current(ports, &s, &fresh);
    if (r != 1) {
        retire(p, r == 0 ? NMP_RETIRE_ABSENT : NMP_RETIRE_UNREADABLE);
        return r == 0 ? NMP_RETIRED : NMP_UNAVAILABLE;
    }
    if (!same(&before, &fresh)) {
        retire(p, mismatch_reason(p, &fresh)); return NMP_RETIRED;
    }
    if (!restore_guard(&fresh) || fresh.current_mode != before.current_mode ||
        fresh.default_mode != 0) return deferred(p);
    final_mode = fresh.current_mode == 3 ? 3 : fresh.current_mode == 1 ? 1 : 2;
    called = ports->touchable(ports->context, &fresh, 0);
    r = read_current(ports, &s, &after);
    if (!called || r != 1 || !same(&fresh, &after) || after.default_mode != 2 ||
        after.current_mode != (final_mode == 3 ? 3u : 2u)) return uncertain(p);
    if (final_mode == 1) {
        /* Native default setter cannot preserve gray1. Re-resolve before the
         * second original call; never call setMode on the earlier snapshot. */
        if (!restore_guard(&after)) return uncertain(p);
        r = read_current(ports, &s, &gray_fresh);
        if (r != 1 || !same(&after, &gray_fresh) ||
            !restore_guard(&gray_fresh) || gray_fresh.default_mode != 2 ||
            gray_fresh.current_mode != 2) return uncertain(p);
        called = ports->mode(ports->context, &gray_fresh, 1);
        r = read_current(ports, &s, &after);
        if (!called || r != 1 || !same(&gray_fresh, &after) ||
            after.current_mode != 1 || after.default_mode != 2) return uncertain(p);
    }
    increment(&p->stats.restored); clear_record(p); return NMP_RESTORED;
}
