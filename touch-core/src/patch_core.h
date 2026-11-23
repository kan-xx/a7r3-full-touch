#ifndef TOUCH_PATCH_CORE_H
#define TOUCH_PATCH_CORE_H
#include <stdint.h>
#define TP_PAGE_OFFSET 0xf1000u
#define TP_PAGE_BYTES 4096u
enum {TP_OK=0,TP_ALREADY=1,TP_IDENTITY=10,TP_PERM,TP_READ,TP_FOREIGN,TP_PROTECT,TP_WRITE,TP_CACHE,TP_RX,TP_VERIFY,TP_UNCERTAIN};
enum {TP_PREFLIGHT=1,TP_WRITABLE,TP_WRITING,TP_FLUSHING,TP_RX_RESTORE,TP_DONE,TP_ROLLBACK=9,TP_ROLLED_BACK,TP_REBOOT_REQUIRED};
struct tp_report {uint32_t abi,bytes,op,phase,result,detail,base,page,original_perm,final_perm,reads,writes,flush_calls,protect_calls,rollback_attempted,rollback_completed,uncertain,before[5],after[5];};
struct tp_ports {void*context;int(*identity)(void*);int(*permission)(void*,uint32_t);int(*protect)(void*,uint32_t,int);int(*read16)(void*,uint32_t,uint16_t*);int(*write16)(void*,uint32_t,uint16_t);int(*page_kind)(void*,uint32_t);int(*flush)(void*,uint32_t,uint32_t);};
/* op0 observes; op1 enables; op2 explicitly reverts. No disk/loader access.
 * Native caller must run on its freshly certified original UI callback.
 * permission uses Linux PROT bits: RX=5, transient RWX=7. page_kind must hash
 * all4096 live bytes:1 exact original,2 exact five-halfword candidate,0 unknown.
 * Any uncertain rollback requires process restart; do not unload the provider.
 */
int tp_run(const struct tp_ports*,uint32_t,uint32_t,struct tp_report*);
#endif
