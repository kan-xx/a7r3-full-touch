#ifndef A7R_TOUCH_BOOT_MANIFEST_H
#define A7R_TOUCH_BOOT_MANIFEST_H
#include <stdint.h>
/* External root-owned, read-only artifact generated AFTER the final owner and
 * its identity-bound touch core. No camera destination exists for this WIP.
 * The only paths/symbol are fixed by the native port, never from this record.
 */
struct touch_boot_manifest {
 char magic[8];
 uint32_t abi,bytes,build,entry_api,core_bytes,entry_offset;
 uint8_t core_sha256[32],entry_anchor[16],owner_sha256[32],installation[16];
};
_Static_assert(sizeof(struct touch_boot_manifest)==128,"boot manifest ABI");
int tbm_validate(const struct touch_boot_manifest *);
#endif
