#ifndef A7_TOUCH_AUTOLOAD_PROGRESS_H
#define A7_TOUCH_AUTOLOAD_PROGRESS_H
#include <stdint.h>
#define AS_PREPARING 0x7003u
#define AS_CANCELLED 0x7004u
#define AS_CANCEL_TOO_LATE 0x7045u
#define AS_CONTEXT_CHANGED 0x7052u
#define AS_FILE_CHANGED 0x7053u
/* 48KiB file data leaves bounded room under64KiB for boot/proc/manifest
 * metadata. Parent worker drives one step per20ms; no thread or idle timer. */
#define AS_HASH_CHUNK 49152u
#define AS_PROGRESS_FILES 5u
/* File order owner,self,view4,view2,Obj. All 5 SHA+bracketed stat identities
 * must complete and all paths be restat-identical before native bind/queue. */
enum as_progress_phase {AS_P_HASH=1,AS_P_REVALIDATE=2,AS_P_BIND_QUEUE=3,AS_P_QUEUED=4,AS_P_CANCELLED=5,AS_P_FAILED=6,AS_P_MP_HASH=7,AS_P_V3_HASH=8};
struct as_progress_report {uint32_t abi,bytes,active,phase,file_index,file_count,steps,chunk_limit,last_io_bytes,total_file_bytes,file_size,file_done,owner_qualified,last_power_error,result,queued,selfpin,open_file,worker_tid,generation;};
/* 7003: independent selfpin + only regular-file hash/data state, NO queued
 * native callback or input hook. Original UI refs opened for a final bind are
 * closed again before returning7003 if queue is busy. Reentry is serialized on
 * the same existing owner worker/process. Each call requires fresh eligibility.
 * ui0 only while this pure preparing state cancels/closes its own FDs/refs and
 * returns7004; after queue/installed returnsTOO_LATE without touching UI. */
#endif
