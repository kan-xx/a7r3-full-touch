#ifndef A7_TOUCH_BOUNDED_FILE_HASH_H
#define A7_TOUCH_BOUNDED_FILE_HASH_H
#include <stdint.h>
#define BF_FILES 5u
#define BF_CHUNK_MAX 49152u
#define BF_ROOT_ONLY 1u
struct bf_identity {uint64_t dev,inode;int64_t size;uint32_t mode,nlink,uid,gid,mtime,mtimens,ctime,ctimens;};
struct bf_config {const char*path;uint32_t bytes,mode,flags;uint8_t sha256[32];};
union bf_sha_storage {uint64_t align;uint8_t bytes[128];};
struct bf_state {uint32_t abi,active,index,verified_mask,total_bytes,last_bytes,steps,error,close_uncertain;int32_t fd;uint32_t file_done;union bf_sha_storage sha;struct bf_identity before,snapshot[BF_FILES];};
struct bf_ports {void*context;int(*open)(void*,const char*);int(*stat)(void*,int,struct bf_identity*);int(*read)(void*,int,void*,uint32_t);int(*close)(void*,int);};
enum bf_result {BF_MORE=0,BF_DONE=1,BF_FAILED=2};
/* Pure regular-file only. Exactly5 configs. expected mode0 allows the original
 * file mode, otherwise requires exact permissions. All require regular type,
 * flashdev252:15,nlink1,size; ROOT_ONLY additionally uid/gid0. Each step hashes
 * at most48KiB; ports/func pointers are never retained between calls. */
int bf_init(struct bf_state*);
int bf_step(struct bf_state*,const struct bf_config[BF_FILES],const struct bf_ports*);
/* Close once, mark fd consumed BEFORE close. Unknown close effects are sticky;
 * never retry the descriptor. Cancel has no Sony object/UI/hardware effects. */
int bf_cancel(struct bf_state*,const struct bf_ports*);
/* Only after all5 digests: reopen/stat/close every exact path and compare saved
 * identity includingmtime/ctime ns. No file body read. */
int bf_revalidate(struct bf_state*,const struct bf_config[BF_FILES],const struct bf_ports*);
int bf_identity_equal(const struct bf_identity*,const struct bf_identity*);
#endif
