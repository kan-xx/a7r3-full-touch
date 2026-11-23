#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include "observer_layout.h"
#include "menu_snapshot.h"
#include "sha256_local.h"
#define NREC 511u
static struct {uint32_t h[64];uint32_t rec[NREC][64];} logbuf;
static struct a7_menu_snapshot snap;
static uint32_t target,base,obj,ui;static uint64_t expected_start;
static uint8_t boot[40],nonce[16];static int memfd=-1;static volatile sig_atomic_t stopped;static int clock_failed;
static void halt(int n){(void)n;stopped=1;}
static uint32_t ms(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t)){clock_failed=1;return 0;}return(uint32_t)t.tv_sec*1000u+t.tv_nsec/1000000u;}
static int file(const char*p,void*out,unsigned cap){int f=open(p,O_RDONLY|O_CLOEXEC);if(f<0)return -1;int n=read(f,out,cap);close(f);return n;}
static int identity(void){uint8_t b[40];if(file("/proc/sys/kernel/random/boot_id",b,40)!=37||memcmp(b,boot,36)||b[36]!='\n')return 0;char path[64],text[2048];snprintf(path,sizeof path,"/proc/%u/stat",target);int n=file(path,text,sizeof text-1);if(n<1)return 0;text[n]=0;char*p=strrchr(text,')');if(!p||p[1]!=' '||p[3]!=' ')return 0;p+=4;uint64_t start=0;for(unsigned field=4;field<=22;field++){char*e=0;unsigned long long v=strtoull(p,&e,10);if(e==p)return 0;if(field==22)start=v;p=e+1;}return start==expected_start;}
static int rd(void*x,uint32_t a,void*out,uint32_t n){(void)x;return a&&a<=UINT32_MAX-n&&pread(memfd,out,n,a)==(ssize_t)n;}
static int word(uint32_t a,uint32_t*out){return rd(0,a,out,4);}
static int cache(uint32_t*w,uint32_t*seq){uint32_t a,b;for(unsigned n=0;n<3;n++){if(!word(base+OL_CACHE_SEQ,&a)||!a||(a&1))continue;if(!rd(0,base+OL_CACHE_WORDS,w,4096)||!word(base+OL_CACHE_SEQ,&b))continue;if(a==b){*seq=a;return 1;}}return 0;}
static int corefile(void){static const uint8_t want[32]={0xaf,0x51,0xbc,0x62,0x41,0x98,0x89,0xf9,0x3f,0x00,0x08,0xf3,0xb9,0xd4,0x46,0x50,0x7a,0x71,0x08,0x00,0xf2,0x98,0x6e,0x49,0xd8,0xc0,0xd9,0xb9,0x41,0x0c,0xc2,0x6b};int f=open("/usr/lib/liba7r_touch_full_runtime_v7.so",O_RDONLY|O_CLOEXEC);if(f<0)return 0;struct sha256 s;sha_init(&s);uint8_t b[4096],d[32];unsigned total=0;for(unsigned n=0;n<40;n++){int k=read(f,b,sizeof b);if(k<0){close(f);return 0;}if(!k)break;total+=k;sha_add(&s,b,k);}close(f);sha_end(&s,d);return total==122624&&!memcmp(d,want,32);}
static unsigned xhex(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:99;}
static int core_identity(void){uint8_t actual_nonce[16],actual_boot[40];uint32_t pidfield=0,uifield=0;uint64_t startfield=0;
 if(!rd(0,base+OL_REQUEST_NONCE,actual_nonce,16)||memcmp(actual_nonce,nonce,16)||!rd(0,base+OL_REQUEST_BOOT,actual_boot,40)||memcmp(actual_boot,boot,36)||!word(base+OL_REQUEST_PID,&pidfield)||pidfield!=target||!word(base+OL_REQUEST_UI_TID,&uifield)||uifield!=ui||!rd(0,base+OL_REQUEST_START,&startfield,8)||startfield!=expected_start)return 0;return 1;}
int main(int ac,char**av){
 if(ac!=11){fprintf(stderr,"usage: observer PID START UI COREBASE OBJBASE BOOT36 NONCE32 SECONDS INTERVAL_MS /tmp_bt/out.bin\n");return 2;}
 target=strtoul(av[1],0,0);expected_start=strtoull(av[2],0,0);ui=strtoul(av[3],0,0);base=strtoul(av[4],0,0);obj=strtoul(av[5],0,0);unsigned seconds=strtoul(av[8],0,0),interval=strtoul(av[9],0,0);
 if(!target||!expected_start||!ui||!base||!obj||strlen(av[6])!=36||strlen(av[7])!=32||seconds<1||seconds>120||interval<4||interval>10||strncmp(av[10],"/tmp_bt/",8)||strlen(av[10])>160||strstr(av[10],"/../"))return 3;
 memcpy(boot,av[6],36);for(unsigned i=0;i<16;i++){unsigned a=xhex(av[7][2*i]),b=xhex(av[7][2*i+1]);if(a>15||b>15)return 3;nonce[i]=a*16+b;}
 if(!identity()||!corefile())return 4;char path[64];snprintf(path,sizeof path,"/proc/%u/mem",target);memfd=open(path,O_RDONLY|O_CLOEXEC);if(memfd<0)return 5;
 if(!core_identity()){close(memfd);return 6;}
 signal(SIGTERM,halt);signal(SIGINT,halt);
 memcpy(logbuf.h,"A7TOBS01",8);logbuf.h[2]=2;logbuf.h[3]=sizeof logbuf;logbuf.h[4]=256;logbuf.h[5]=NREC;logbuf.h[6]=target;logbuf.h[7]=ui;logbuf.h[8]=base;logbuf.h[9]=obj;memcpy(logbuf.h+16,boot,40);memcpy(logbuf.h+26,nonce,16);memcpy(logbuf.h+30,&expected_start,8);
 uint32_t launch=ms(),begin=0,initial_blobs=0,capture=0,last[8]={0},previous_blobs=0,have=0,lastcb=0,writes=0,iofail=0,cachefail=0,exit_reason=0;logbuf.h[40]=interval;logbuf.h[41]=seconds;logbuf.h[42]=0;logbuf.h[45]=launch;logbuf.h[47]=900000;logbuf.h[43]=getpid();printf("OBSERVER_READY pid=%u ui=%u seconds=%u interval=%u ring=%zu standby_ms=900000 capture_starts=first_new_TP_blob no_target_write=1\n",target,ui,seconds,interval,sizeof logbuf);fflush(stdout);
 while(!stopped){uint32_t now=ms();if(clock_failed){exit_reason=5;break;}if(capture&&now-begin>=seconds*1000u)break;if(!capture&&now-launch>=900000u){exit_reason=4;break;}uint32_t w[1024],seq=0,tap[21],again[21],ws=0,vs=0,cb=0,gws=0,points[5]={0},mode=0,adjust=0,clocksource=0;uint8_t adj=0;
  if(!identity()){exit_reason=1;break;}
  if(!cache(w,&seq)){cachefail++;goto wait;}if(w[0]!=7||w[1]!=4096){exit_reason=2;break;}if(!have)initial_blobs=w[19];else if(!capture&&w[19]!=initial_blobs){capture=1;begin=ms();logbuf.h[42]=begin;logbuf.h[44]=begin-launch;logbuf.h[46]=initial_blobs;logbuf.h[48]=w[19];}
  if(!rd(0,base+OL_TAP,tap,sizeof tap)||!rd(0,base+OL_TAP,again,sizeof again)){iofail++;goto wait;}
  int pointer_ok=word(base+OL_RT_WS,&ws)&&word(base+OL_RT_VS,&vs)&&word(vs+0x14,&cb);
  uint32_t key[8]={w[19],w[14],w[220],tap[14],tap[15],tap[16],tap[20],w[22]};
  if(have&&cb==lastcb&&!memcmp(key,last,sizeof key))goto wait;
  if(!capture&&have)goto wait;
  uint32_t rec[64]={0};rec[0]=writes+1;rec[1]=capture?ms()-begin:0;rec[2]=seq;rec[3]=1;memcpy(rec+4,tap+14,6*4);rec[10]=tap[20];memcpy(rec+11,tap+3,11*4);rec[52]=!memcmp(tap,again,sizeof tap);if(rec[52]&&w[14]==tap[15]&&w[59]==tap[14])rec[3]|=16;rec[57]=again[15];
  rec[60]=cb;rec[61]=base+OL_INPUT_HOOK;rec[62]=vs;rec[63]=ws;if(have&&cb!=lastcb)rec[3]|=32;
  if(pointer_ok&&rd(0,ws+0x4a8,points,20)&&word(obj+0x129f470,&gws)&&gws==ws&&cb==base+OL_INPUT_HOOK){rec[3]|=2;memcpy(rec+22,points,20);}
  if(word(obj+0x1231a64,&clocksource)&&rd(0,obj+0x129f120,&adj,1)&&word(obj+0x12d4e28,&mode)){adjust=adj;rec[3]|=4;rec[27]=clocksource;rec[28]=adjust;rec[29]=mode;}
  rec[30]=w[19];rec[31]=w[16];rec[32]=w[17];rec[33]=w[220];rec[34]=w[219];rec[35]=w[20];rec[36]=w[21];rec[37]=w[22];rec[38]=w[235];rec[39]=w[236];rec[40]=w[7];rec[41]=w[34];rec[42]=w[60];rec[43]=w[62];rec[44]=w[229];rec[51]=have?w[19]-previous_blobs:0;rec[53]=w[239];rec[54]=w[3]|w[4]<<8|w[6]<<16;rec[55]=w[43];rec[56]=capture?ms()-begin:0;rec[58]=cachefail;rec[59]=iofail;
  int sr=a7_menu_take_snapshot(rd,0,obj+0x12d4c04,obj+0x12d4e2c,0xaa,&snap);rec[47]=sr;if(!sr){uint32_t high=0;for(unsigned i=0;i<snap.active_count;i++)if(!snap.active[i].bank&&snap.active[i].index>=high){high=snap.active[i].index;rec[45]=snap.active[i].id;rec[46]=snap.active[i].view;}rec[3]|=8;}
  if(!identity()){exit_reason=1;break;}if(!core_identity()){exit_reason=3;break;}memcpy(logbuf.rec[writes%NREC],rec,sizeof rec);writes++;memcpy(last,key,sizeof key);have=1;previous_blobs=w[19];lastcb=cb;
wait:{struct timespec z={0,(long)(capture?interval:10)*1000000};nanosleep(&z,0);}
 }
 close(memfd);logbuf.h[10]=writes;logbuf.h[11]=writes>NREC?writes-NREC:0;logbuf.h[12]=capture?ms()-begin:0;logbuf.h[44]=capture?begin-launch:ms()-launch;logbuf.h[49]=stopped?1:0;logbuf.h[50]=capture;logbuf.h[13]=exit_reason;logbuf.h[14]=cachefail;logbuf.h[15]=iofail;
 int out=open(av[10],O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if(out<0)return 7;unsigned used=0;while(used<sizeof logbuf){ssize_t n=write(out,(uint8_t*)&logbuf+used,sizeof logbuf-used);if(n<=0){close(out);return 8;}used+=n;}if(close(out))return 9;
 printf("OBSERVER_DONE records=%u retained=%u cachefail=%u iofail=%u reason=%u bytes=%zu no_target_write=1\n",writes,writes>NREC?NREC:writes,cachefail,iofail,exit_reason,sizeof logbuf);return exit_reason?10:0;
}
