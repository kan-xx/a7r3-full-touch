#ifndef A7R_NATIVE_PLATFORM_H
#define A7R_NATIVE_PLATFORM_H
#include <stdint.h>
#include <stddef.h>
#define NP_BUILD 0x3110d206u
#define NP_EXPORT
#define NP_MAX_MAPS 2048u
#define NP_MAX_LINE 768u
enum {NP_COLD,NP_OPENING,NP_HELD,NP_FAILED,NP_RELEASED};
enum {NP_OK,NP_STATE,NP_IDENTITY,NP_SYSTEM,NP_MAPS,NP_LIBRARY,NP_FD,NP_CHANGED,NP_CLOSE,NP_THREAD};
enum {NP_R=1,NP_W=2,NP_X=4,NP_SHARED=8};
enum {NP_OTHER,NP_IM,NP_OSAL,NP_LIBC,NP_UIPC};
struct np_map {uint32_t lo,hi,offset,flags,kind,major,minor;uint64_t inode;};
struct np_maps {uint32_t count,bytes;struct np_map rows[NP_MAX_MAPS];};
/* Linux ARM EABI stat64, matched to Sony's published asm/stat.h. */
struct np_stat {uint64_t dev;uint8_t pad0[4];uint32_t oldino,mode,nlink,uid,gid;uint64_t rdev;uint8_t pad3[4];int64_t size;uint32_t blksize;uint64_t blocks;uint32_t atime,atimens,mtime,mtimens,ctime,ctimens;uint64_t ino;};
struct np_status {uint32_t abi,bytes,build,phase,error,pid,worker_tid,provider_mask,osal_base,libc_base,uconf,pool_lo,pool_hi,map_count,map_bytes,reads,checks,close_uncertain,source_fd;int32_t duplicate_fd,mem_fd;uint32_t duplicate_cloexec,source_flags,duplicate_flags,duplicate_acquired,release_attempted,pool_offset,conf_changed_mask,conf_identity_changed_mask;};
/* First failure is immutable. Release diagnostics do not erase its cause. */
struct np_diagnostic {uint32_t abi,bytes,stage,first_stage,reason;int32_t call_rc;uint32_t map_row,addr,length,expected_lo,expected_hi,pool_lo,pool_hi,pool_offset,pool_flags,release_checks,release_error,reserved[7];char line[160];};
struct np_context {uint32_t claimed;struct np_status s;void*refs[2];uint32_t conf[9];struct np_stat device;struct np_maps maps;struct np_diagnostic diagnostic;};
NP_EXPORT uint32_t np_size(void);
NP_EXPORT int np_open(struct np_context*);
NP_EXPORT int np_check(struct np_context*);
/* Cleanup only: validates the retained original open-file duplicate and its
 * provider/worker lifetime. It never grants acquisition, reads, or ACKs. */
NP_EXPORT int np_check_release(struct np_context*);
NP_EXPORT int np_read(struct np_context*,uint32_t,void*,uint32_t);
NP_EXPORT int np_report(const struct np_context*,struct np_status*);
NP_EXPORT int np_release_preuse(struct np_context*);
int np_parse_map(const char*,struct np_map*);
int np_range(const struct np_maps*,uint32_t,uint32_t,uint32_t);
/* One worker, zero-initialized context. A retained duplicate pins the open file,
 * not its userspace mapping. These APIs do not send any ioctl, lifecycle ACK,
 * register an add-on, create a worker or grant production start. Read and check
 * reject a fork child or a different worker. Release is ONLY before handing
 * descriptors/pointers to another component: there is no post-use close API.
 * Maps/uconf comparisons are sampled checks, not synchronization against
 * original munmap/close/re-init. Bootstrap must establish that lifetime. */
#endif
