#ifndef TOUCH_NATIVE_H
#define TOUCH_NATIVE_H
#include <stdint.h>
#include <stddef.h>
#include "patch_core.h"
#include "tree.h"
#include "autoload_contract.h"
#include "autoload_ui.h"
#include "autoload_progress.h"
#include "navigation.h"
#define TR_BUILD 0x3111fe07u
#define TR_CORE_PATH "/tmp_bt/a7r-touch-full-v7/liba7r_touch_full_runtime_v7.so"
#define TR_CORE_SYMBOL "a7r_touch_full_runtime_v7"
/* Candidate Senser F910/libproF9: parent must confirm namespace is unused. */
struct tr_request {uint32_t version,op;uint8_t nonce[16],boot[40];uint32_t pid,ui_tid;uint64_t start;uint32_t major,minor;uint64_t inode;uint32_t base,uid,gid,mode,arg0,arg1;};
struct tr_inspection {uint32_t abi,bytes,status,selected_count,selected_node,top_id,top_obj,active_count,reads,read_bytes,selected_id,sequence;char module[64],factory[64];};
struct tr_layout {uint32_t abi,bytes,status,resource_group_id,layout_ids[6],active_layout_count,primary_view,root_id_mask,get_widget_calls,returned_roots[2],sequence,actual_runtime_ids[2],reserved[8];};
struct tr_runtime {uint32_t word[1024];};
struct tr_report {uint32_t version,bytes,build,pid,ui_tid,lib_base,obj_base,manager,phase,callbacks,submit_tid,callback_tid,callback_pid,dispatch_flag,return_matches,callback_return,safe_to_unload,owns_reference,attempted,identity_valid,bind_stage,close_uncertain,last_error,callback_operation;uint8_t nonce[16];struct tr_layout layout;struct tr_inspection inspection;struct a7_navigation_result navigation;struct tw_report tree;struct tr_runtime runtime;};
struct tr_probe {uint32_t abi,bytes,pid,ui_tid;uint64_t start;uint8_t boot[40];};
struct tr_discovery {uint32_t abi,bytes,build,status,origin,flags,core_base,api_entry,autoload_entry,callbacks,phase,last_error;struct tr_request identity;char path[96];struct as_report autostart;struct as_ui_report bootstrap;struct as_progress_report progress;};
struct tr_api {uint32_t version,bytes,build;int(*prepare)(void*,const struct tr_request*,uint32_t*,struct tr_report*);int(*query)(const struct tr_request*,struct tr_report*);int(*action)(const struct tr_request*,struct tr_report*);int(*stop)(const struct tr_request*,void**,struct tr_report*);int(*discover)(const struct tr_probe*,struct tr_discovery*);};
struct plugin_packet {const uint8_t*input;uint32_t input_size,input_capacity;const uint8_t*output;uint32_t output_size;void(*output_free)(void*);};
#if defined(__arm__)
_Static_assert(sizeof(struct tr_request)==120,"request ABI after wire magic");
_Static_assert(sizeof(struct tr_layout)==108,"layout ABI");
_Static_assert(sizeof(struct tr_inspection)==176,"inspection ABI");
_Static_assert(sizeof(struct tr_report)==26412,"native report ABI");
_Static_assert(sizeof(struct tr_api)==32,"API ABI");
_Static_assert(sizeof(struct tr_probe)==64,"probe ABI");
_Static_assert(sizeof(struct tr_discovery)==424,"discovery ABI");
#endif
#ifdef __cplusplus
extern "C" {
#endif
const struct tr_api*a7r_touch_full_runtime_v7(void);
#ifdef __cplusplus
}
#endif
#endif
