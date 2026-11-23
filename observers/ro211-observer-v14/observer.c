/* Separate short-lived RO observer. Only its bounded exclusive TMPFS log is
 * written. No Sony calls, isHit, model creation, UI events or core private read. */
#define main prior_readonly_main
#include "model_collect.c"
#undef main
#include <signal.h>
#include <time.h>
#ifdef __APPLE__
#include <sys/mount.h>
#else
#include <sys/statfs.h>
#endif
#define OBS_BYTES 524288u
#define OBS_RECORD 2048u
#define OBS_SLOTS 256u
#define ACTIVE_FIRST 6u
#define ACTIVE_SLOTS 192u
#define POST_FIRST 198u
#define POST_SLOTS 58u
#define OBS_MAX_MS 7200000u
#define OBS_RETURN_MS 60000u
#define F_STABLE 1u
#define F_META 2u
#define F_TYPED211 4u
#define F_NORMAL 8u
#define F_CAPTURE211 16u
#define F_READY_NAMES 32u
struct compact_widget {uint32_t address,ok,typed,header[10],parent_node,parent_parent,parent_widget,owner_valid,owner,event_valid,event_id[21];uint8_t event_enable[21],pad[3];char type[64];};
struct body {uint32_t status,flags,vm,mm,top_id,top_view,top_index,view_vptr,view_phase,ws,scope_generation,reserved;uint32_t mouse[5],gui[4];struct result model;struct e9_fields e9;struct compact_widget hit,capture;};
struct record {uint32_t m[16];struct body a,b;uint8_t pad[OBS_RECORD-64-2*sizeof(struct body)-4];uint32_t crc;};
_Static_assert(sizeof(struct body)<=990,"two complete payloads fit fixed record");
_Static_assert(sizeof(struct record)==OBS_RECORD,"RO record2048");
static volatile sig_atomic_t terminate_requested;
static void terminate_signal(int s){(void)s;terminate_requested=1;}
static uint64_t mono_ms(void){struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return 0;return(uint64_t)t.tv_sec*1000u+(uint32_t)t.tv_nsec/1000000u;}
static uint32_t crc32(const void*p,unsigned n){const uint8_t*b=p;uint32_t c=~0u;for(unsigned i=0;i<n;i++){c^=b[i];for(unsigned j=0;j<8;j++)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}return~c;}
static int write_at(int fd,const void*p,unsigned n,uint32_t off){unsigned done=0;for(unsigned k=0;k<16&&done<n;k++){ssize_t z=pwrite(fd,(const uint8_t*)p+done,n-done,off+done);if(z<0&&errno==EINTR)continue;if(z<=0)return 0;done+=(unsigned)z;}return done==n;}
static void pack_widget(struct compact_widget*d,uint32_t ptr){if(!ptr)return;struct widget w={0};widget(&w,ptr);d->address=w.address;d->ok=w.ok;d->typed=w.typed;memcpy(d->header,w.header,sizeof d->header);d->parent_node=w.parent_node;d->parent_parent=w.parent_parent;d->parent_widget=w.parent_widget;d->owner_valid=w.owner_valid;d->owner=w.owner;d->event_valid=w.event_valid;memcpy(d->event_id,w.event_id,sizeof d->event_id);for(unsigned i=0;i<21;i++)d->event_enable[i]=(uint8_t)w.event_enable[i];memcpy(d->type,w.type,63);}
static uint32_t scope_id,scope_view,scope_vm,scope_generation;
static int quick_top(struct body*s){uint32_t vm=0,again=0,m[28],slots[39*4];if(!word(held[0].base+0x12d4c04,&vm)||!vm||(vm&3)||!rd(vm,m,sizeof m)||m[0]!=39||m[2]!=held[0].base+0x12d4e2c||!m[8]||(m[8]&3)||!rd(m[8],slots,sizeof slots)||!word(held[0].base+0x12d4c04,&again)||again!=vm)return 0;s->vm=vm;s->mm=m[6];for(unsigned i=0;i<39;i++)if(slots[4*i+1]){if(slots[4*i+1]&3)return 0;s->top_id=slots[4*i+2];s->top_view=slots[4*i+1];s->top_index=i;}
 if(!s->top_view)return 0;unsigned count=0;for(unsigned i=0;i<39;i++)if(slots[4*i+1]&&slots[4*i+2]==s->top_id)count++;if(count!=1||!word(s->top_view,&s->view_vptr)||!word(s->top_view+4,&s->view_phase))return 0;
 if(scope_id!=s->top_id||scope_view!=s->top_view||scope_vm!=vm){scope_id=scope_view=scope_vm=0;struct sample proof;if(!current_scene(&proof)||proof.current_view!=s->top_view||proof.view_id!=s->top_id)return 0;scope_id=s->top_id;scope_view=s->top_view;scope_vm=vm;if(!++scope_generation)scope_generation++;}
 s->scope_generation=scope_generation;
 return(s->top_id==175&&s->view_vptr==held[1].base+0x861970)||(s->top_id==211&&s->view_vptr==held[2].base+0xa8f48);}
static void snapshot_body(struct body*s){memset(s,0,sizeof*s);if(!quick_top(s))s->status|=1u;
 if(!word(held[0].base+0x129f470,&s->ws)||!s->ws||s->ws>UINT32_MAX-0x4bc||!rd(s->ws+0x4a8,s->mouse,20)||!rd(held[0].base+0x129f448,s->gui,12)||!rd(held[0].base+0x1231a68,s->gui+3,4))s->status|=2u;
 if(s->mouse[3])pack_widget(&s->hit,s->mouse[3]);if(s->mouse[4]){if(s->mouse[4]==s->mouse[3])s->capture=s->hit;else pack_widget(&s->capture,s->mouse[4]);}
 if(!s->mm||(s->mm&3)||!lookup(s->mm,0xe9,&s->model))s->status|=4u;else{struct model_item item={0};item.base=s->model;if(!e9_detail(&item,s->mm))s->status|=8u;s->e9=item.e9;}
 if(s->e9.typed&&s->e9.still_status==PINCH_STILL_NORMAL)s->flags|=F_NORMAL;
 if(s->top_id==211&&s->view_vptr==held[2].base+0xa8f48)s->flags|=F_TYPED211;
 if((s->flags&F_TYPED211)&&s->mouse[4])s->flags|=F_CAPTURE211;
 if((s->flags&F_NORMAL)&&((uint8_t*)s->e9.stream)[0xb0]&&((uint8_t*)s->e9.stream)[0xba]&&memchr((uint8_t*)s->e9.stream+0xb0,0,10)&&memchr((uint8_t*)s->e9.stream+0xba,0,14))s->flags|=F_READY_NAMES;
}
static int stat_same_meta(const struct stat*a,const struct stat*b){return a->st_dev==b->st_dev&&a->st_ino==b->st_ino&&a->st_size==b->st_size&&a->st_mode==b->st_mode&&a->st_uid==b->st_uid&&a->st_gid==b->st_gid&&a->st_nlink==b->st_nlink&&a->st_mtime==b->st_mtime&&a->st_ctime==b->st_ctime;}
static int exe_fd=-1;static struct stat exe_stat;
static int refresh_memory_map(void){return loadmaps()&&bindlibs();}
static int metadata_qualified(void){if(!epoch()||!refresh_memory_map())return 0;for(unsigned i=0;i<3;i++){struct stat a,b;if(fstat(held[i].fd,&a)||lstat(libs[i].path,&b)||!stat_same_meta(&a,&held[i].st)||!stat_same_meta(&a,&b))return 0;}struct stat a,b;char p[64];snprintf(p,sizeof p,"/proc/%u/exe",target);return exe_fd>=0&&!fstat(exe_fd,&a)&&!stat(p,&b)&&stat_same_meta(&a,&exe_stat)&&stat_same_meta(&a,&b);}
static int initialize_observer(void){for(unsigned i=0;i<3;i++)held[i].fd=-1;if(!epoch())return 0;for(unsigned i=0;i<3;i++){held[i].fd=open(libs[i].path,O_RDONLY|O_NOFOLLOW|O_CLOEXEC);if(held[i].fd<0||!hashfd(held[i].fd,libs[i].sha,libs[i].bytes,libs[i].bytes)||fstat(held[i].fd,&held[i].st)||!factory_profile(i))return 0;}char p[64];snprintf(p,sizeof p,"/proc/%u/exe",target);exe_fd=open(p,O_RDONLY|O_CLOEXEC);if(exe_fd<0||!hashfd(exe_fd,"866729eb0bc1e6f5aba04d8bf8e45491985afe89a078ccb106a1ce2476865225",0,64u*1024u*1024u)||fstat(exe_fd,&exe_stat))return 0;snprintf(p,sizeof p,"/proc/%u/mem",target);memfd=open(p,O_RDONLY|O_CLOEXEC);if(memfd<0||!unchanged())return 0;struct sample scene;return current_scene(&scene)&&metadata_qualified();}
struct ring_state{int fd;uint64_t started,last_meta,last_post,return_since;uint32_t seq,samples,stored,active_count,post_count,first211,firstready,firstreturn,firstcapture,lastcapture,meta_checks,reason,header_serial;struct body lasta,lastb;uint32_t have_last;};
static int write_record_slot(struct ring_state*c,unsigned slot,struct record*r){r->crc=crc32(r,OBS_RECORD-4);return slot<OBS_SLOTS&&write_at(c->fd,r,sizeof*r,slot*OBS_RECORD);}
static int ring_sample(struct ring_state*c,uint64_t now,struct record*r){c->samples++;r->m[0]=0x31524f52;r->m[1]=1;r->m[2]=++c->seq;r->m[3]=(uint32_t)now;r->m[4]=now>>32;int stable=!memcmp(&r->a,&r->b,sizeof r->a);r->m[7]=(stable?F_STABLE:0)|F_META|r->a.flags|r->b.flags;r->m[8]=r->a.status;r->m[9]=r->b.status;r->m[13]=c->meta_checks;r->m[14]=(uint32_t)(now-c->last_meta);
 int is211=(r->a.flags|r->b.flags)&F_TYPED211,cap=(r->a.flags|r->b.flags)&F_CAPTURE211;
 if(is211&&!c->first211){c->first211=c->seq;if(!write_record_slot(c,1,r))return 0;}
 if(is211&&stable&&!r->a.status&&(r->a.flags&F_READY_NAMES)&&!c->firstready){c->firstready=c->seq;if(!write_record_slot(c,2,r))return 0;}
 if(cap){if(!c->firstcapture){c->firstcapture=c->seq;if(!write_record_slot(c,4,r))return 0;}c->lastcapture=c->seq;if(!write_record_slot(c,5,r))return 0;}
 if(c->first211&&stable&&!r->a.status&&r->a.top_id==175&&r->b.top_id==175){if(!c->return_since){c->return_since=now;if(!c->firstreturn){c->firstreturn=c->seq;if(!write_record_slot(c,3,r))return 0;}}}else c->return_since=0;
 int changed=!c->have_last||memcmp(&c->lasta,&r->a,sizeof r->a)||memcmp(&c->lastb,&r->b,sizeof r->b);
 if(is211&&changed){if(!write_record_slot(c,ACTIVE_FIRST+c->active_count%ACTIVE_SLOTS,r))return 0;c->active_count++;c->stored++;}
 else if(!is211&&(!c->last_post||now-c->last_post>=1000)){if(!write_record_slot(c,POST_FIRST+c->post_count%POST_SLOTS,r))return 0;c->post_count++;c->stored++;c->last_post=now;}
 c->lasta=r->a;c->lastb=r->b;c->have_last=1;
 if(c->return_since&&now-c->return_since>=OBS_RETURN_MS)c->reason=1;if(now-c->started>=OBS_MAX_MS)c->reason=2;return 1;}
static int write_header(struct ring_state*c,uint64_t now){uint8_t b[OBS_RECORD]={0};memcpy(b,"A7RO14B1",8);uint32_t w[32]={1,OBS_BYTES,OBS_RECORD,sizeof(struct body),target,(uint32_t)start,(uint32_t)(start>>32),(uint32_t)getpid(),c->seq,c->samples,c->stored,c->active_count,c->post_count,c->first211,c->firstready,c->firstreturn,c->firstcapture,c->lastcapture,c->meta_checks,c->reason,(uint32_t)c->started,(uint32_t)(c->started>>32),(uint32_t)now,(uint32_t)(now>>32),ACTIVE_FIRST,ACTIVE_SLOTS,POST_FIRST,POST_SLOTS,++c->header_serial,100,1000,OBS_MAX_MS};memcpy(b+8,w,sizeof w);memcpy(b+136,boot,37);for(unsigned i=0;i<3;i++)memcpy(b+176+4*i,&held[i].base,4);uint32_t crc=crc32(b,OBS_RECORD-4);memcpy(b+OBS_RECORD-4,&crc,4);return write_at(c->fd,b,sizeof b,0);}
static int safe_name(const char*p){unsigned n=0;if(strncmp(p,"ro211-",6))return 0;for(;p[n];n++)if(n>=79||!((p[n]>='a'&&p[n]<='z')||(p[n]>='A'&&p[n]<='Z')||(p[n]>='0'&&p[n]<='9')||p[n]=='.'||p[n]=='_'||p[n]=='-'))return 0;return n>6;}
int observer_entry(int ac,char**av){if(ac!=5||!safe_name(av[4]))return 2;uint64_t p=0,t=0;if(!decimal(av[1],&p)||!decimal(av[2],&t)||p<2||p>0x7fffffffu||!t||strlen(av[3])!=36)return 3;for(unsigned i=0;i<36;i++){char c=av[3][i];if(i==8||i==13||i==18||i==23){if(c!='-')return 3;}else if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return 3;}target=p;start=t;memcpy(boot,av[3],37);if(!initialize_observer()){printf("{\"started\":false,\"code\":4,\"no_target_write\":true}\n");return 4;}
 int dir=open("/tmp_bt",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);struct statfs fs;int out=-1;if(dir<0||fstatfs(dir,&fs)||fs.f_type!=0x01021994||(uint64_t)fs.f_bavail*fs.f_bsize<OBS_BYTES+8192){if(dir>=0)close(dir);return 5;}out=openat(dir,av[4],O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);close(dir);if(out<0)return 5;if(ftruncate(out,OBS_BYTES)){close(out);return 5;}
 struct sigaction sa={0};sa.sa_handler=terminate_signal;sigemptyset(&sa.sa_mask);sigaction(SIGTERM,&sa,0);sigaction(SIGINT,&sa,0);struct ring_state c={0};c.fd=out;c.started=mono_ms();c.last_meta=c.started;c.meta_checks=1;if(!c.started||!write_header(&c,c.started)){close(out);return 6;}printf("{\"started\":true,\"pid\":%u,\"ring\":\"/tmp_bt/%s\",\"bytes\":%u,\"no_target_write\":true,\"target_calls\":0}\n",(unsigned)getpid(),av[4],OBS_BYTES);fflush(stdout);
 while(!c.reason){uint64_t began=mono_ms();if(terminate_requested){c.reason=3;break;}if(!began){c.reason=4;break;}if(began-c.last_meta>=1000){if(!metadata_qualified()){c.reason=4;break;}c.last_meta=began;c.meta_checks++;if(!write_header(&c,began)){c.reason=5;break;}}
  struct record r={0};observe_reads=observe_bytes=observe_failures=0;observing=1;snapshot_body(&r.a);snapshot_body(&r.b);observing=0;uint64_t after=mono_ms();r.m[6]=(uint32_t)(after-began);r.m[10]=observe_reads;r.m[11]=observe_bytes;r.m[12]=observe_failures;if(!ring_sample(&c,after,&r)){c.reason=5;break;}uint64_t spent=mono_ms()-began;if(spent<100){struct timespec wait={(time_t)0,(long)(100-spent)*1000000L};nanosleep(&wait,0);}}
 uint64_t ended=mono_ms();int good=write_header(&c,ended)&&!fsync(out);int own=out;out=-1;if(close(own))good=0;if(memfd>=0)close(memfd);for(unsigned i=0;i<3;i++)if(held[i].fd>=0)close(held[i].fd);if(exe_fd>=0)close(exe_fd);printf("{\"finished\":true,\"reason\":%u,\"samples\":%u,\"stored\":%u,\"first211\":%u,\"first_ready211\":%u,\"first_capture211\":%u,\"last_capture211\":%u,\"file_closed\":%s,\"no_target_write\":true,\"target_calls\":0}\n",c.reason,c.samples,c.stored,c.first211,c.firstready,c.firstcapture,c.lastcapture,good?"true":"false");return good?0:6;}

#ifndef OBS_TEST
int main(int ac,char**av){return observer_entry(ac,av);}
#endif
