#include "boot_manifest.h"
int tbm_validate(const struct touch_boot_manifest *m) {
 static const char magic[8]={'A','7','T','B','M','0','0','1'};
 if(!m||m->abi!=1||m->bytes!=128||m->entry_api!=1||
    m->build!=0x3111fe07u||m->core_bytes<16384||m->core_bytes>1048576||
    !m->entry_offset||(m->entry_offset&~1u)>0x01000000u)return 0;
 /* entry_offset is ELF linked VA relative to load bias, NOT a file offset.
  * The sealing builder must prove its executable PT_LOAD membership and map
  * entry_anchor through that segment, never compare it to raw file size. */
 for(unsigned i=0;i<8;i++)if(m->magic[i]!=magic[i])return 0;
 const uint8_t *groups[]={m->core_sha256,m->entry_anchor,m->owner_sha256,m->installation};
 const unsigned sizes[]={32,16,32,16};
 for(unsigned i=0;i<4;i++){unsigned any=0;for(unsigned j=0;j<sizes[i];j++)any|=groups[i][j];if(!any)return 0;}
 return 1;
}
