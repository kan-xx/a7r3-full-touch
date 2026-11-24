#include "bounded_file_hash.h"
#include "sha256_local.h"

enum {BE_STATE=1,BE_CONFIG,BE_PORTS,BE_OPEN,BE_STAT,BE_IDENTITY,
      BE_READ,BE_EARLY_EOF,BE_TRAILING,BE_CHANGED,BE_DIGEST,BE_CLOSE,
      BE_CANCELLED,BE_OVERFLOW};
#define BF_MASK ((1u<<BF_FILES)-1u)
#define BF_IO_BYTES 4096u
#define BF_READS_MAX (BF_CHUNK_MAX/BF_IO_BYTES)
#define BF_FLAGS_VALID (BF_ROOT_ONLY|BF_DYNAMIC_ELF)
_Static_assert(sizeof(struct sha256)<=sizeof(union bf_sha_storage),"SHA state fits opaque storage");

static void zero(void*out,unsigned n){uint8_t*p=out;while(n--)*p++=0;}
static void copy(void*out,const void*in,unsigned n){uint8_t*p=out;const uint8_t*q=in;while(n--)*p++=*q++;}
static int ports_ok(const struct bf_ports*p,int body){return p&&p->open&&p->stat&&p->close&&(!body||p->read);}
static int close_once(struct bf_state*s,const struct bf_ports*p){
 int fd=s->fd;s->fd=-1;
 if(fd>=0&&(!p||!p->close||p->close(p->context,fd))){s->close_uncertain=1;return 0;}
 return !s->close_uncertain;
}
static int failed(struct bf_state*s,const struct bf_ports*p,unsigned error){
 if(!s->error)s->error=error;s->active=0;
 if(!close_once(s,p)&&!s->error)s->error=BE_CLOSE;
 return BF_FAILED;
}
static int configs_ok(const struct bf_config*c){
 if(!c)return 0;uint64_t total=0;
 for(unsigned i=0;i<BF_FILES;i++){
  if(!c[i].path||!c[i].path[0]||(c[i].mode&~07777u)||(c[i].flags&~BF_FLAGS_VALID))return 0;
  total+=c[i].bytes;
 }
 return total<=UINT32_MAX;
}
static int expected(const struct bf_identity*i,const struct bf_config*c){
 int size_ok=(c->flags&BF_DYNAMIC_ELF)?(i->size>=52&&i->size<=1048576&&(!c->bytes||i->size==(int64_t)c->bytes)):(i->size==(int64_t)c->bytes);
 return i->dev==0xfc0fu&&size_ok&&i->nlink==1&&
        (i->mode&0170000u)==0100000u&&(!c->mode||(i->mode&07777u)==c->mode)&&
        (!(c->flags&BF_ROOT_ONLY)||(!i->uid&&!i->gid));
}
static int elf_byte_ok(uint32_t pos,uint8_t b){
 static const uint8_t head[7]={0x7f,'E','L','F',1,1,1};
 static const uint8_t type_mach[4]={3,0,40,0};
 if(pos<7u)return b==head[pos];
 if(pos>=16u&&pos<20u)return b==type_mach[pos-16u];
 return 1;
}
int bf_identity_equal(const struct bf_identity*a,const struct bf_identity*b){
 return a&&b&&a->dev==b->dev&&a->inode==b->inode&&a->size==b->size&&
        a->mode==b->mode&&a->nlink==b->nlink&&a->uid==b->uid&&a->gid==b->gid&&
        a->mtime==b->mtime&&a->mtimens==b->mtimens&&a->ctime==b->ctime&&a->ctimens==b->ctimens;
}
int bf_init(struct bf_state*s){
 if(!s)return BF_FAILED;
 /* Fresh callers zero-initialize. A live/uncertain descriptor cannot be erased. */
 if(s->abi==1&&(s->fd>=0||s->close_uncertain))return BF_FAILED;
 zero(s,sizeof*s);s->abi=1;s->active=1;s->fd=-1;return BF_MORE;
}
int bf_cancel(struct bf_state*s,const struct bf_ports*p){
 if(!s||s->abi!=1)return BF_FAILED;
 s->active=0;s->last_bytes=0;if(!s->error)s->error=BE_CANCELLED;
 return close_once(s,p)?BF_DONE:BF_FAILED;
}
int bf_step(struct bf_state*s,const struct bf_config c[BF_FILES],const struct bf_ports*p){
 if(!s||s->abi!=1)return BF_FAILED;
 s->last_bytes=0;
 if(s->error||s->close_uncertain)return failed(s,p,s->error?s->error:BE_CLOSE);
 if(s->index==BF_FILES&&s->verified_mask==BF_MASK&&!s->active&&s->fd<0)return BF_DONE;
 if(!s->active||s->index>=BF_FILES)return failed(s,p,BE_STATE);
 if(s->verified_mask!=((1u<<s->index)-1u))return failed(s,p,BE_STATE);
 if(!configs_ok(c))return failed(s,p,BE_CONFIG);
 if(!ports_ok(p,1))return failed(s,p,BE_PORTS);
 if(s->steps==UINT32_MAX)return failed(s,p,BE_OVERFLOW);s->steps++;
 const struct bf_config*cfg=c+s->index;struct sha256 sha;
 uint64_t progress=s->file_done;for(unsigned i=0;i<s->index;i++)progress+=(uint64_t)s->snapshot[i].size;
 if(progress!=s->total_bytes)return failed(s,p,BE_STATE);
 if(s->fd<0){
  if(s->file_done)return failed(s,p,BE_STATE);
  int fd=p->open(p->context,cfg->path);if(fd<0)return failed(s,p,BE_OPEN);s->fd=fd;
  if(p->stat(p->context,fd,&s->before))return failed(s,p,BE_STAT);
  if(!expected(&s->before,cfg))return failed(s,p,BE_IDENTITY);
  zero(&sha,sizeof sha);sha_init(&sha);copy(s->sha.bytes,&sha,sizeof sha);
 }
 uint32_t target_bytes=(cfg->flags&BF_DYNAMIC_ELF)?(uint32_t)s->before.size:cfg->bytes;
 if(s->file_done>target_bytes)return failed(s,p,BE_STATE);
 if(!expected(&s->before,cfg))return failed(s,p,BE_STATE);
 copy(&sha,s->sha.bytes,sizeof sha);
 if(sha.n!=s->file_done||sha.used>=64||sha.used!=(s->file_done&63u))return failed(s,p,BE_STATE);
 uint8_t buffer[BF_IO_BYTES];unsigned reads=0;
 while(s->file_done<target_bytes&&s->last_bytes<BF_CHUNK_MAX&&reads<BF_READS_MAX){
  uint32_t n=target_bytes-s->file_done;if(n>sizeof buffer)n=sizeof buffer;
  if(n>BF_CHUNK_MAX-s->last_bytes)n=BF_CHUNK_MAX-s->last_bytes;
  int got=p->read(p->context,s->fd,buffer,n);reads++;
  if(got<0||(uint32_t)got>n)return failed(s,p,BE_READ);
  if(!got)return failed(s,p,BE_EARLY_EOF);
  if(UINT32_MAX-s->total_bytes<(uint32_t)got)return failed(s,p,BE_OVERFLOW);
  if(cfg->flags&BF_DYNAMIC_ELF){
   for(unsigned k=0;k<(unsigned)got&&s->file_done+k<20u;k++)
    if(!elf_byte_ok(s->file_done+k,buffer[k]))return failed(s,p,BE_DIGEST);
  }
  sha_add(&sha,buffer,(unsigned)got);s->file_done+=(uint32_t)got;
  s->total_bytes+=(uint32_t)got;s->last_bytes+=(uint32_t)got;
 }
 copy(s->sha.bytes,&sha,sizeof sha);
 if(s->file_done<target_bytes||s->last_bytes==BF_CHUNK_MAX||reads==BF_READS_MAX)return BF_MORE;
 int got=p->read(p->context,s->fd,buffer,1);
 if(got<0||got>1)return failed(s,p,BE_READ);
 if(got){s->last_bytes++;if(s->total_bytes!=UINT32_MAX)s->total_bytes++;return failed(s,p,BE_TRAILING);}
 struct bf_identity after;
 if(p->stat(p->context,s->fd,&after))return failed(s,p,BE_STAT);
 if(!bf_identity_equal(&s->before,&after))return failed(s,p,BE_CHANGED);
 uint8_t digest[32];sha_end(&sha,digest);
 if(!(cfg->flags&BF_DYNAMIC_ELF)){
  unsigned diff=0;for(unsigned i=0;i<32;i++)diff|=digest[i]^cfg->sha256[i];
  if(diff)return failed(s,p,BE_DIGEST);
 }
 if(!close_once(s,p))return failed(s,p,BE_CLOSE);
 s->snapshot[s->index]=s->before;s->verified_mask|=1u<<s->index;s->index++;s->file_done=0;
 if(s->index==BF_FILES){s->active=0;return BF_DONE;}
 return BF_MORE;
}
int bf_revalidate(struct bf_state*s,const struct bf_config c[BF_FILES],const struct bf_ports*p){
 if(!s||s->abi!=1)return BF_FAILED;s->last_bytes=0;
 if(s->error||s->close_uncertain)return failed(s,p,s->error?s->error:BE_CLOSE);
 if(s->active||s->index!=BF_FILES||s->verified_mask!=BF_MASK||s->fd>=0)return failed(s,p,BE_STATE);
 if(!configs_ok(c))return failed(s,p,BE_CONFIG);
 if(!ports_ok(p,0))return failed(s,p,BE_PORTS);
 for(unsigned i=0;i<BF_FILES;i++){
  int fd=p->open(p->context,c[i].path);if(fd<0)return failed(s,p,BE_OPEN);s->fd=fd;
  struct bf_identity current;
  if(p->stat(p->context,fd,&current))return failed(s,p,BE_STAT);
  if(!expected(&current,c+i)||!bf_identity_equal(&current,s->snapshot+i))return failed(s,p,BE_CHANGED);
  if(!close_once(s,p))return failed(s,p,BE_CLOSE);
 }
 return BF_DONE;
}
