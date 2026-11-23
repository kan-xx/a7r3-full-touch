#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <stdint.h>
#ifdef A7_MIN_DECL
#include <stddef.h>
typedef void FILE;
typedef long ssize_t;
extern FILE *stderr;
extern int open(const char*,int,...),close(int),fclose(FILE*),ferror(FILE*),putchar(int),printf(const char*,...),fprintf(FILE*,const char*,...),snprintf(char*,size_t,const char*,...),sscanf(const char*,const char*,...);
extern ssize_t read(int,void*,size_t),write(int,const void*,size_t),pread64(int,void*,size_t,long long);
extern int fsync(int),nanosleep(const void*,void*),clock_gettime(int,void*);
extern int *__errno_location(void);
#define errno (*__errno_location())
#define EINTR 4
extern void (*signal(int,void(*)(int)))(int);
typedef int sig_atomic_t;
struct timespec {long tv_sec,tv_nsec;};
extern FILE *fopen(const char*,const char*);
extern char *fgets(char*,int,FILE*),*strrchr(const char*,int);
extern unsigned long strtoul(const char*,char**,int);
extern unsigned long long strtoull(const char*,char**,int);
extern size_t strlen(const char*),strcspn(const char*,const char*);
extern void *memset(void*,int,size_t),*memcpy(void*,const void*,size_t);
extern int memcmp(const void*,const void*,size_t),strcmp(const char*,const char*);
#define O_RDONLY 0
#define O_CLOEXEC 0x80000
#define PRIu64 "llu"
#define pread(f,p,n,o) pread64(f,p,n,(long long)(o))
typedef long long off_t;
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#endif
/* Read-only separate process. All offsets below are explicit hexadecimal.
 * Original lookup834ff6: MM+0x88 map; root+8/end+4; node pair+0x10,
 * pair.second+4. Original comparator3bb3d8 is signed int less-than. */
struct mapping {uint32_t start,end,offset;char perms[5],path[256];};
static struct mapping maps[1024];static unsigned nmaps;
static int memfd=-1;static unsigned pid;static uint64_t starttime;static uint32_t obj,view2;static char boot[37];
static int rf(const char*p,void*out,unsigned cap){int f=open(p,O_RDONLY|O_CLOEXEC);if(f<0)return -1;int n=read(f,out,cap);close(f);return n;}
static int identity(void){char b[40],path[64],text[2048];if(rf("/proc/sys/kernel/random/boot_id",b,sizeof b)!=37||memcmp(b,boot,36)||b[36]!='\n')return 0;snprintf(path,sizeof path,"/proc/%u/stat",pid);int n=rf(path,text,sizeof text-1);if(n<1)return 0;text[n]=0;char*p=strrchr(text,')');if(!p||p[1]!=' '||p[3]!=' '||p[2]=='Z')return 0;p+=4;uint64_t v=0;for(unsigned f=4;f<=22;f++){char*end=0;v=strtoull(p,&end,10);if(end==p)return 0;p=end+1;}return v==starttime;}
static int load_maps(void){char path[64],line[512];snprintf(path,sizeof path,"/proc/%u/maps",pid);FILE*f=fopen(path,"r");if(!f)return 0;nmaps=0;while(fgets(line,sizeof line,f)){if(nmaps==1024){fclose(f);return 0;}struct mapping*m=&maps[nmaps];char dev[32];unsigned long inode=0;int at=0;memset(m,0,sizeof*m);if(sscanf(line,"%x-%x %4s %x %31s %lu %n",&m->start,&m->end,m->perms,&m->offset,dev,&inode,&at)!=6||m->start>=m->end){fclose(f);return 0;}char*q=line+at;while(*q==' ')q++;size_t n=strcspn(q,"\n");if(n>=sizeof m->path){fclose(f);return 0;}memcpy(m->path,q,n);nmaps++;}int ok=!ferror(f);fclose(f);return ok;}
static const struct mapping *mapped(uint32_t a,uint32_t n){if(!a||!n||a>UINT32_MAX-n)return 0;for(unsigned i=0;i<nmaps;i++)if(maps[i].start<=a&&a+n<=maps[i].end&&maps[i].perms[0]=='r')return &maps[i];return 0;}
static int rd(uint32_t a,void*out,uint32_t n){return mapped(a,n)&&pread(memfd,out,n,(off_t)a)==(ssize_t)n;}
static int word(uint32_t a,uint32_t*out){return !(a&3)&&rd(a,out,4);}
static int anchors(void){
 static const uint8_t a[16]={0x90,0xb5,0x85,0xb0,0x00,0xaf,0x07,0xf1,0x10,0x03,0x00,0xf1,0x88,0x04,0x20,0x46};
 static const uint8_t b[8]={0x80,0xb5,0x00,0xaf,0x80,0x68,0x80,0xbd};
 static const uint8_t c[8]={0x00,0x68,0x80,0xb5,0x10,0x30,0x00,0xaf};
 uint8_t x[16];const struct mapping*m=mapped(obj,4);if(!m||m->start!=obj||m->offset||strcmp(m->path,"/usr/lib/libObj.so"))return 0;
 return rd(obj+0x834ff6,x,16)&&!memcmp(x,a,16)&&rd(obj+0x834a90,x,8)&&!memcmp(x,b,8)&&rd(obj+0x834a34,x,8)&&!memcmp(x,c,8);
}

/* Wire ABI1: 96B header, 192B value-only record. No target-native call.
 * cb[] retains callback offsets 0,4,10,14,18,1c,20,24,28,2c.
 * Excludes only its threadID08 and semaphore0c; +11 is cb[2] bits8..15.
 * key[] is the complete 0x44 materialized key record, uninterpreted bytes. */
enum { K_VM=1,K_VS=2,K_TP=4,K_TPFLAGS=8,K_II=16,K_IIFLAGS=32,K_CB=64,K_KEY=128,K_GLOBALS=256 };
enum { E_VM=1,E_TP=2,E_II=4,E_LINK=8,E_DUP=16,E_KEY=32,E_GLOBALS=64 };
enum { R_FIRST=1,R_CHANGE=2,R_HEARTBEAT=3,R_ONCE_END=4,R_TIME_END=5,R_FULL_END=6,R_IDENTITY_END=7,R_MAPS_END=8,R_SIGTERM_END=9,R_CLOCK_END=10 };
struct observation {
 uint32_t known,error,vm,vs,callback,tp,tp_vptr,tp_flags,input,input_vptr,input_plus20;
 uint32_t view_status,tp_filter,callback_record,cb[10],key[17],pre_callback,post_callback;
};
struct record {uint32_t sequence,elapsed_ms,poll,reason,stable;struct observation value;};
struct log_header {uint32_t magic,abi,header_bytes,record_bytes,max_records,period_ms,max_ms,pid,start_lo,start_hi,obj,view2;char boot[40];uint32_t reserved[2];};
_Static_assert(sizeof(struct observation)==172,"observation wire");
_Static_assert(sizeof(struct record)==192,"record wire");
_Static_assert(sizeof(struct log_header)==96,"header wire");
static volatile sig_atomic_t requested_stop;
static void request_stop(int signal_number){(void)signal_number;requested_stop=1;}
static int clock_ms(uint32_t*out){struct timespec t;if(clock_gettime(1,&t)||t.tv_sec<0||t.tv_nsec<0||t.tv_nsec>=1000000000)return 0;*out=(uint32_t)t.tv_sec*1000u+(uint32_t)t.tv_nsec/1000000u;return 1;}
static int write_all(const void*p,unsigned n){while(n){ssize_t k=write(1,p,n);if(k<0&&errno==EINTR)continue;if(k<=0)return 0;p=(const uint8_t*)p+k;n-=(unsigned)k;}return 1;}
static int all_anchors(void){
 static const uint8_t cb[16]={0x0a,0x68,0x08,0x4b,0xb0,0xb5,0x9a,0x42,0x00,0xaf,0x04,0x46,0x0d,0x46,0x02,0xd1};
 static const uint8_t getter[16]={0x04,0x4b,0x05,0x4a,0x7b,0x44,0x80,0xb5,0x00,0xaf,0x9b,0x58,0x18,0x68,0xc0,0xf3};
 uint8_t b[16];const struct mapping*m=mapped(view2,4);
 return anchors()&&m&&m->start==view2&&!m->offset&&!strcmp(m->path,"/usr/lib/viewUnified2.so")&&rd(view2+0x286eb8,b,16)&&!memcmp(b,cb,16)&&rd(view2+0x2e6270,b,16)&&!memcmp(b,getter,16);
}
static void take(struct observation*out){
 memset(out,0,sizeof*out);
 if(word(view2+0xa6cd2c,&out->view_status)&&word(obj+0x129eb80,&out->tp_filter)&&word(obj+0x129ebf0,&out->pre_callback)&&word(obj+0x129ebfc,&out->post_callback))out->known|=K_GLOBALS;else out->error|=E_GLOBALS;
 uint32_t slots=0,registry=0;
 if(word(obj+0x12d4c04,&out->vm)&&out->vm&&word(out->vm,&slots)&&slots==39&&word(out->vm+8,&registry)&&registry==obj+0x12d4e2c){
  out->known|=K_VM;
  if(word(out->vm+0x14,&out->vs)&&out->vs&&word(out->vs+0x14,&out->callback))out->known|=K_VS;else out->error|=E_VM;
 }else out->error|=E_VM;
 if(word(obj+0x12d4ebc,&out->tp)&&out->tp&&word(out->tp,&out->tp_vptr)&&out->tp_vptr==obj+0x1205820){
  out->known|=K_TP;uint8_t suppression=0,event=0,raw=0;
  if(rd(out->tp+0x20,&suppression,1)&&rd(out->tp+0x32,&event,1)&&rd(out->tp+0x35,&raw,1)){out->tp_flags=suppression|((uint32_t)event<<8)|((uint32_t)raw<<16);out->known|=K_TPFLAGS;}else out->error|=E_TP;
 }else out->error|=E_TP;
 if(!word(obj+0x12d4ee8,&out->input)||!out->input||!word(out->input,&out->input_vptr)||out->input_vptr!=obj+0x12058a0){out->error|=E_II;return;}
 out->known|=K_II;
 if(word(out->input+0x20,&out->input_plus20))out->known|=K_IIFLAGS;else out->error|=E_II;
 uint32_t node=0,seen[64],found=0;unsigned used=0;
 if(!word(out->input+0x9c,&node)){out->error|=E_LINK;return;}
 while(node){
  if((node&3)||used==64){out->error|=E_LINK;return;}
  for(unsigned i=0;i<used;i++)if(seen[i]==node){out->error|=E_LINK;return;}
  seen[used++]=node;
  uint32_t head[2],c[12];
  if(!rd(node,head,8)||head[1]!=out->input||!rd(node,c,sizeof c)){out->error|=E_LINK;return;}
  if(c[5]==11){
   found++;
   if(found!=1){out->error|=E_DUP;out->known&=~(K_CB|K_KEY);return;}
   out->callback_record=node;
   static const unsigned offsets[10]={0,1,4,5,6,7,8,9,10,11};
   for(unsigned i=0;i<10;i++)out->cb[i]=c[offsets[i]];
   if(c[7]==view2+0x286eb9){
    out->known|=K_CB;
    if(c[10]==1&&c[11]&&rd(c[11],out->key,sizeof out->key))out->known|=K_KEY;else out->error|=E_KEY;
   }else out->error|=E_LINK;
  }
  node=c[0];
 }
}
static int parse32(const char*s,uint32_t*out){char*e=0;unsigned long n=strtoul(s,&e,0);if(!e||*e||!n||n>UINT32_MAX)return 0;*out=(uint32_t)n;return 1;}
int main(int ac,char**av){
 if(ac!=7||(strcmp(av[1],"--once")&&strcmp(av[1],"--watch"))){fprintf(stderr,"usage: input_watch --once|--watch PID START OBJ_BASE VIEW2_BASE BOOT36\n");return 2;}
 int watch=!strcmp(av[1],"--watch");uint32_t argpid=0;char*end=0;
 if(!parse32(av[2],&argpid)||argpid<2)return 2;pid=argpid;starttime=strtoull(av[3],&end,0);
 if(!end||*end||!starttime||!parse32(av[4],&obj)||!parse32(av[5],&view2)||(obj&4095)||(view2&4095)||obj>UINT32_MAX-0x12d4f00u||view2>UINT32_MAX-0xa6cd30u||strlen(av[6])!=36)return 2;
 for(unsigned i=0;i<36;i++)if((i==8||i==13||i==18||i==23)?av[6][i]!='-':!((av[6][i]>='0'&&av[6][i]<='9')||(av[6][i]>='a'&&av[6][i]<='f')))return 2;
 memcpy(boot,av[6],36);
 if(!identity()||!load_maps())return 3;
 char path[64];snprintf(path,sizeof path,"/proc/%u/mem",pid);memfd=open(path,O_RDONLY|O_CLOEXEC);if(memfd<0)return 4;
 if(!all_anchors()){close(memfd);return 5;}
 if(signal(15,request_stop)==(void(*)(int))-1||signal(1,(void(*)(int))1)==(void(*)(int))-1){close(memfd);return 6;}
 struct log_header h={0x31575443u,1,96,192,2048,100,1200000,pid,(uint32_t)starttime,(uint32_t)(starttime>>32),obj,view2,{0},{0,0}};memcpy(h.boot,boot,36);
 uint32_t began=0,now=0,last_maps=0,last_emit=0;if(!clock_ms(&began)||!write_all(&h,sizeof h)){close(memfd);return 7;}last_maps=last_emit=began;
 struct record r;struct observation previous;memset(&previous,0,sizeof previous);unsigned count=0,polls=0;int have_previous=0,exit_code=0,sleep_failed=0;
 for(;;){
  memset(&r,0,sizeof r);r.sequence=count+1;r.poll=++polls;
  if(!clock_ms(&now)){r.reason=R_CLOCK_END;exit_code=8;}else r.elapsed_ms=now-began;
  if(!r.reason&&requested_stop)r.reason=R_SIGTERM_END;
  if(!r.reason&&sleep_failed){r.reason=R_CLOCK_END;exit_code=10;}
  if(!r.reason&&(!identity())){r.reason=R_IDENTITY_END;exit_code=3;}
  if(!r.reason&&(uint32_t)(now-last_maps)>=1000){if(!load_maps()||!all_anchors()){r.reason=R_MAPS_END;exit_code=5;}last_maps=now;}
  if(!r.reason){struct observation a;take(&a);take(&r.value);if(!identity()){memset(&r.value,0,sizeof r.value);r.reason=R_IDENTITY_END;exit_code=3;}else r.stable=!memcmp(&a,&r.value,sizeof a);}
  if(!r.reason&&!watch)r.reason=R_ONCE_END;
  if(!r.reason&&r.elapsed_ms>=1200000u)r.reason=R_TIME_END;
  if(!r.reason&&count==2047)r.reason=R_FULL_END;
  if(!r.reason){if(!have_previous)r.reason=R_FIRST;else if(memcmp(&previous,&r.value,sizeof previous)||!r.stable)r.reason=R_CHANGE;else if((uint32_t)(now-last_emit)>=1000)r.reason=R_HEARTBEAT;}
  if(r.reason){if(!write_all(&r,sizeof r)){exit_code=9;break;}count++;last_emit=now;previous=r.value;have_previous=1;}
  if(r.reason>=R_ONCE_END)break;
  struct timespec delay={0,100000000};if(nanosleep(&delay,0)&&!requested_stop)sleep_failed=1;
 }
 int close_error=close(memfd);memfd=-1;int flush_error=fsync(1),stdout_close_error=close(1);if(flush_error||stdout_close_error||close_error)exit_code=11;
 fprintf(stderr,"INPUT_WATCH ended records=%u polls=%u status=%d no_target_write=1\n",count,polls,exit_code);return exit_code;
}
