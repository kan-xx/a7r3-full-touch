#ifndef A7_FFC_TOUCH_ADAPTER_H
#define A7_FFC_TOUCH_ADAPTER_H
#include <stdint.h>
#include <stddef.h>
#define FFC_ACCEPTED_BYTES 70160u
#define FFC_ACCEPTED_SHA256 "3aa925564d548cd0b93b604e1fd6dc9aa70955edc0c3f0e685c2be4a215620d7"
#define FFC_REPORT_BYTES 1148u
enum ffc_key { FFC_UP=1, FFC_DOWN=2, FFC_CENTER=3, FFC_BACK=4 };
enum ffc_authority_flags {
    FFC_SHA_PROVEN=1u, FFC_PUBLIC_API_PROVEN=2u, FFC_NATIVE_PORTS_PROVEN=4u,
    FFC_UI_OWNER=8u, FFC_POWER_ACTIVE=16u, FFC_DRAINED=32u,
    FFC_PHYSICAL_BUTTONS_ZERO=64u
};
#define FFC_SOURCE_AUTHORITY 31u
#define FFC_REQUIRED_AUTHORITY 127u
struct ffc_authority {
    uint32_t flags,pid,owner,power_generation,scope_epoch;
    uint32_t api_version,api_bytes,api_build;
    uint32_t mp_base,obj_base;
    uint8_t nonce[16];
};
struct ffc_slot {
    uint32_t bank,index,id,object,vptr,registry_matches;
    char module[96],factory[96];
};
struct ffc_registry {
    uint32_t vm,configured_slots,registry_singleton,selected_count;
    uint32_t top_id,top_index,top_object,ffc_count;
    struct ffc_slot banks[2];
};
struct ffc_rect {int32_t left,top,right,bottom;};
struct ffc_live {
    struct ffc_authority authority;
    uint32_t vm,object,vptr,control,serial,graphics,row,held,pending;
    uint32_t label[6];
    struct ffc_rect rect[6];
};
/* All callbacks are fixed proven runtime ports, invoked only synchronously in
 * the real UI owner. This library never stores these ports in a native object.
 * authority checks the runtime's atomic power/scope, NOT mp_power.query(owner).
 * query calls only the qualified public API1/36/build3110d015 query, copying
 * exactly1148B. read must be mapping-bounded and never dereference stale memory.
 * rect must recheck authority, qualify actual GEN_Label and use original world
 * geometry in the accepted640x480 plane. No private MP functions/global writes.
 */
struct ffc_projection_ports {
    void *context;
    int (*authority)(void*,struct ffc_authority*);
    int (*query)(void*,uint32_t out[FFC_REPORT_BYTES/4]);
    int (*registry)(void*,struct ffc_registry*);
    int (*read)(void*,uint32_t address,void*,uint32_t bytes);
    int (*rect)(void*,uint32_t label,const struct ffc_authority*,struct ffc_rect*);
};
enum ffc_status {
    FFC_OK=0, FFC_BAD_API=1, FFC_BAD_SCOPE=2, FFC_BAD_REPORT=3,
    FFC_BAD_LAYOUT=4, FFC_BUSY=5, FFC_NO_HIT=6, FFC_ALLOC=7,
    FFC_ABORTED_HELD=8, FFC_NATIVE_UNCERTAIN=9, FFC_SUBMITTED=10,
    FFC_CANCEL_SUBMITTED=11, FFC_NO_CANCEL=12
};
/* A ticket owns no objects/pointers and never authorizes a late release. It is
 * meaningful only after a known normal press, starting from freshly proved0
 * held. Recovery must independently observe the same serial and matching held.
 * CENTER/BACK release is not followed by a borrowed-view read; a later fresh
 * owner invocation may cancel the SAME serial via fixed original close only.
 * Native throw/unknown must invalidate it: no automatic close or replay. */
struct ffc_cancel_ticket {
    uint32_t valid,pid,owner,serial,key;
    uint8_t nonce[16];
};
struct ffc_pair {
    void *blocks[16];
    uint32_t prepared,key,attempted[2],returned[2],retained[2];
};
struct ffc_raw_ports {
    void *context;
    void *(*allocate)(size_t);
    void (*release)(void*);
    void *(*event_dtor)(void*);
    void (*raw_dispatch)(void *fresh_vm,void *event); /* Original8305bd only. */
    int (*boundary)(void*,const struct ffc_live *fresh,unsigned release);
    uint32_t qualified;
};
struct ffc_pair_report {
    uint32_t status,allocated,released,called,returned,retained;
    uintptr_t retained_event[2];
};
struct ffc_result {
    uint32_t status,target_row,navigation_pairs,presses,releases;
    uint32_t allocated,freed,native_uncertain;
    uintptr_t retained_event[2];
    struct ffc_cancel_ticket cancel;
};
struct ffc_action_ports {
    struct ffc_projection_ports projection;
    struct ffc_raw_ports raw;
    /* Optional fixed original close11012002,dest4,Param6FFC. Must recheck this
     * fresh current identity/power; queued Event is Sony-only, with no DSO ptr.
     * 1=ownership transferred, 0=not submitted, -1=uncertain/native throw.
     * Must never reopen automatically or use this after unknown dispatch. */
    int (*close_current)(void*,const struct ffc_live*);
};
#ifdef __cplusplus
extern "C" {
#endif
/* Read-only catalog collection may run while stock has GUI work/buttons.
 * It preserves actual flags and never authorizes key emission. */
int ffc_project_catalog(const struct ffc_projection_ports*,struct ffc_live*);
/* Final action projection remains strict: drained and buttons zero. */
int ffc_project(const struct ffc_projection_ports*,struct ffc_live*);
int ffc_same_lease(const struct ffc_live*,const struct ffc_live*);
int ffc_hit(const struct ffc_live*,int32_t x,int32_t y);
int ffc_pair_prepare(const struct ffc_raw_ports*,enum ffc_key,struct ffc_pair*,struct ffc_pair_report*);
int ffc_pair_emit(const struct ffc_raw_ports*,struct ffc_pair*,unsigned release,const struct ffc_live*,struct ffc_pair_report*);
int ffc_pair_dispose(const struct ffc_raw_ports*,struct ffc_pair*,struct ffc_pair_report*);
/* Stateless, bounded synchronous actions; no async worker/timer or old object.
 * Click uses six measured row rects; BACK has no hidden touch rectangle. */
int ffc_click(const struct ffc_action_ports*,int32_t x,int32_t y,struct ffc_result*);
int ffc_back(const struct ffc_action_ports*,struct ffc_result*);
int ffc_cancel_known(const struct ffc_action_ports*,const struct ffc_cancel_ticket*,struct ffc_result*);
#ifdef __cplusplus
}
#endif
#endif
