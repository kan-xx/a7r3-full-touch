#ifndef A7_NATIVE_MODE_POLICY_H
#define A7_NATIVE_MODE_POLICY_H
#include <stdint.h>

/* All public operations on a policy must be serialized by the real UI owner
 * (or after its exclusive fence). A raw power notifier updates the runtime's
 * atomic gate, not this non-atomic accounting object concurrently.
 * Numeric native-data accounting only. This object owns NO module references,
 * callbacks, vptrs, queues, timers, native objects, or historical heap pointers.
 * All native calls use a newly resolved stack snapshot, never saved_record. */
enum nmp_target {
    NMP_IROIRO_MENU_SELECT = 1,
    NMP_IROIRO_BAR_DIAL = 2,
    NMP_FN_OUTER_BELT = 3
};
enum nmp_class {
    NMP_CLASS_MENU_SELECT = 1,
    NMP_CLASS_BAR_DIAL = 2,
    NMP_CLASS_FN_BELT = 3
};
enum nmp_flags {
    NMP_CURRENT_VIEW = 1u << 0,
    NMP_TYPED_NATIVE = 1u << 1,
    NMP_PROVEN_PORTS = 1u << 2,
    NMP_ELIGIBLE_SCOPE = 1u << 3,
    NMP_QUIESCENT = 1u << 4,
    NMP_ANCESTORS_MODE0 = 1u << 5,
    NMP_VISIBLE = 1u << 6,
    NMP_POWER_AWAKE = 1u << 7,
    NMP_DRAINED = 1u << 8
};
#define NMP_ENABLE_FLAGS (NMP_CURRENT_VIEW | NMP_TYPED_NATIVE | \
    NMP_PROVEN_PORTS | NMP_ELIGIBLE_SCOPE | NMP_QUIESCENT | \
    NMP_ANCESTORS_MODE0 | NMP_VISIBLE | NMP_POWER_AWAKE | NMP_DRAINED)
#define NMP_RESTORE_FLAGS (NMP_CURRENT_VIEW | NMP_TYPED_NATIVE | \
    NMP_PROVEN_PORTS | NMP_QUIESCENT | NMP_POWER_AWAKE | NMP_DRAINED)

struct nmp_identity {
    /* Same leading shape as ir_identity: these are comparison tokens, not
     * function addresses and never handles that this library dereferences. */
    uint32_t view, widget, runtime_id, resource_id, class_kind;
    uint32_t current_mode, default_mode;
    uint32_t view_runtime_id;
    /* Runtime must change scope_epoch whenever continuity is not proven,
     * including any observed current-view loss/replacement or registry reset.
     * Zero/unknown epoch or power generation may not authorize a mutation. */
    uint32_t scope_epoch, power_generation;
    uint32_t flags, buttons_down;
};

struct nmp_ports {
    void *context;
    /* Fresh current View -> original getWidget -> typed identity, bounded
     * parent checks, native bindings, current power/scope/drain/buttons state.
     * Return 1 current snapshot, 0 absent, -1 unreadable/unprovable. It must NOT
     * use a saved widget pointer. All functions execute in the real UI owner. */
    int (*current)(void *, uint32_t resource, struct nmp_identity *);
    /* Proven runtime wrappers around the exact original native methods only.
     * Revalidate current identity and owner/power at the call boundary; do not
     * retain the snapshot pointer. Return 1 iff the original call returned
     * normally, else 0 (the library still treats possible mutation as unknown).
     * Wrappers must not add event/queue pumping between these ports; original
     * native reset may invoke its own callbacks, hence every next mutation
     * re-resolves the current identity before using it.
     * touchable uses PAS setTouchable(bool), or native setDefWidgetMode(0/2)
     * for the Fn outer belt. mode uses original Widget::setMode(1) only. */
    int (*touchable)(void *, const struct nmp_identity *fresh, int enabled);
    int (*mode)(void *, const struct nmp_identity *fresh, uint32_t mode);
};

enum nmp_result {
    NMP_INVALID = -2,
    NMP_UNAVAILABLE = -1,
    NMP_DEFERRED = 0,
    NMP_APPLIED = 1,
    NMP_CURRENT_ENABLED = 2,
    NMP_ALREADY_NATIVE = 3,
    NMP_RESTORED = 4,
    NMP_RETIRED = 5,
    NMP_IDLE = 6,
    NMP_STOPPED = 7,
    NMP_NATIVE_RESET = 8
};
enum nmp_retire_reason {
    NMP_RETIRE_NONE = 0,
    NMP_RETIRE_ABSENT = 1,
    NMP_RETIRE_UNREADABLE = 2,
    NMP_RETIRE_IDENTITY = 3,
    NMP_RETIRE_NATIVE_CHANGED = 4,
    NMP_RETIRE_WRITE_UNCERTAIN = 5,
    NMP_RETIRE_STOP_UNAVAILABLE = 6,
    NMP_RETIRE_POWER = 7
};
struct nmp_stats {
    uint32_t applied, restored, native_reset, already_native, deferred;
    /* Cumulative reports, not a count of still-live retired native objects. */
    uint32_t retired, unrestored_total, write_uncertain, last_retire_reason;
};
struct nmp_policy {
    uint32_t target, accept_new, tracked;
    /* Only a continuously current instance's baseline. Cleared on retirement;
     * no historical widget identity is retained. Never passed to native ports. */
    struct nmp_identity saved_record;
    struct nmp_stats stats;
};

#ifdef __cplusplus
extern "C" {
#endif
int nmp_init(struct nmp_policy *, enum nmp_target);
int nmp_start(struct nmp_policy *);
int nmp_enable_current(const struct nmp_ports *, struct nmp_policy *);
/* Stops further enable operations immediately, then attempts only the freshly
 * proven current record. DEFERRED permits a later stop retry, or abandon below.
 * It never makes module shutdown depend on native-data restoration. */
int nmp_stop_current(const struct nmp_ports *, struct nmp_policy *);
/* No native calls or heap access: retire current numeric accounting on power
 * loss, shutdown after unavailable drain, etc. Module-reference cleanup is a
 * separate runtime obligation. This does not claim native modes were restored. */
void nmp_abandon_current(struct nmp_policy *, enum nmp_retire_reason);
#ifdef __cplusplus
}
#endif
#endif
