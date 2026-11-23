#include "native.h"
#include "runtime_perf.h"
#include "target_identity.h"
#include "runtime_identity.h"
#include "bounded_file_hash.h"
#include "view3_identity.h"
#include "inspect_identity.h"
#include "navigation_scope.h"
#include "menu_snapshot.h"
#include "menu_maps.h"
#include "platform.h"
#include "sha256_local.h"
extern long syscall(long,...);extern void*malloc(size_t);extern void free(void*);
extern void*dlsym(void*,const char*),*dlopen(const char*,int);extern int dlclose(void*);
extern int pthread_mutex_trylock(void*),pthread_mutex_unlock(void*),open(const char*,int,...),close(int),read(int,void*,size_t),pread64(int,void*,size_t,long long);
struct info{const char*file;void*base;const char*symbol;void*address;};extern int dladdr(const void*,struct info*);
struct ts{int32_t sec,nsec;};typedef void(*cbtype)(void*,struct ts*,void*);
static struct {uint32_t busy,initialized,autostart_preparing,origin,auto_install,queue_locked,entry_tid,pending_op,inspect_attempts,view_global,registry_singleton;struct tr_request q;struct tr_report r;struct as_ui_state auto_ui;void*self,*view,*record;int fd;void*manager;int(*now)(struct ts*);void*(*add)(void*,struct ts*,cbtype,void*);void(*remove)(void*,void*);uint32_t expected_return;} s={.fd=-1};
static struct bf_identity verified_fw[6];static uint32_t verified_fw_mask;
static void remember_hash(const char*,const struct np_stat*);
static int verified_current(unsigned);
static int verified_mp_current(void);
#define MP_CORE_PATH "/usr/lib/liba7r_menu_product_v21.so"
#define MP_FILE_BYTES 70160u
static const uint8_t mp_file_sha[32]={0xc6,0x75,0x29,0xca,0x3c,0xc2,0xb1,0x5d,0xc1,0x76,0xaf,0x58,0x97,0x5a,0x0c,0x8e,0xc8,0x41,0xc6,0x8a,0x84,0x63,0x59,0xa5,0x56,0x0c,0x0e,0xb7,0xf4,0xa2,0x5a,0xac};
static uint32_t as_revoked_request,as_recovery_activation,as_current_power;
static uint32_t as_claim; /*0 none,1 autostart/data,2 queued-autostart,4 service*/
static int as_preverified(void);
static char maptext[131072];
static struct tw_class tree_classes[TREE_CLASS_COUNT];
static struct a7_menu_snapshot snapshot;static struct a7_menu_maps snapshot_maps;
static int same(const void*a,const void*b,unsigned n){const uint8_t*p=a,*q=b;unsigned d=0;while(n--)d|=*p++^*q++;return !d;}
static unsigned slen(const char*p){unsigned n=0;while(p[n])n++;return n;}
static int equal(const char*a,const char*b){return a&&b&&slen(a)==slen(b)&&same(a,b,slen(a));}
static int enter(void){uint32_t zero=0;return __atomic_compare_exchange_n(&s.busy,&zero,1,0,__ATOMIC_ACQUIRE,__ATOMIC_RELAXED);}static void leave(void){s.entry_tid=0;__atomic_store_n(&s.busy,0,__ATOMIC_RELEASE);}
static uint32_t pid(void){return syscall(20);}static uint32_t tid(void){return syscall(224);}
static int bounded(const char*name,void*out,unsigned cap){int f=open(name,0x80000);if(f<0)return -1;unsigned used=0;int good=1;while(used<cap){int n=read(f,(uint8_t*)out+used,cap-used);if(n<0){good=0;break;}if(!n)break;used+=n;}char tail;if(used==cap&&read(f,&tail,1))good=0;if(close(f))good=0;return good?(int)used:-1;}
static int decimal(const char*p,const char*e,uint64_t*v){uint64_t n=0;if(p==e)return 0;for(;p<e;p++){if(*p<'0'||*p>'9'||n>(UINT64_MAX-9)/10)return 0;n=n*10+*p-'0';}*v=n;return 1;}
static int epoch_raw(void){
 char b[1024];int n=bounded("/proc/sys/kernel/random/boot_id",b,40);if(n!=37||!same(b,s.q.boot,36)||b[36]!='\n'||pid()!=s.q.pid)return 0;
 n=bounded("/proc/self/stat",b,sizeof b-1);if(n<1)return 0;b[n]=0;char*p=b;while(*p&&*p!=' ')p++;uint64_t id;if(!decimal(b,p,&id)||id!=s.q.pid)return 0;
 char*last=0;for(char*q=p;*q;q++)if(*q==')')last=q;if(!last||last[1]!=' '||!last[2]||last[3]!=' ')return 0;p=last+4;
 uint64_t parent=0,start=0;for(unsigned field=4;field<=22;field++){char*e=p;while(*e&&*e!=' '&&*e!='\n')e++;if(e==p)return 0;if(field==4&&!decimal(p,e,&parent))return 0;if(field==22&&!decimal(p,e,&start))return 0;p=*e==' '?e+1:e;}
 return parent==1&&start==s.q.start;
}
static int rt_epoch_get(void);static void rt_epoch_put(int);
static int epoch(void){if(rt_epoch_get())return 1;uint32_t t=rt_perf_clock();int ok=epoch_raw();rt_epoch_put(ok);rt_perf_end(RP_EPOCH,t);return ok;}
static int request_match(const struct tr_request*q){return q&&q->version==7&&q->pid==s.q.pid&&q->ui_tid==s.q.ui_tid&&q->start==s.q.start&&q->major==s.q.major&&q->minor==s.q.minor&&q->inode==s.q.inode&&q->base==s.q.base&&q->uid==s.q.uid&&q->gid==s.q.gid&&q->mode==s.q.mode&&!q->arg1&&((q->op==7||q->op==9)?q->arg0<6:q->op==11?q->arg0<2:!q->arg0)&&same(q->nonce,s.q.nonce,16)&&same(q->boot,s.q.boot,40);}
static int file_identity(void){
 int f=open("/usr/lib/viewUnified4.so",0x88000);if(f<0)return 0;struct np_stat a,b;struct sha256 h;uint8_t data[4096],digest[32];int good=!syscall(197,f,&a)&&a.size==2753064&&a.ino==s.q.inode&&a.uid==s.q.uid&&a.gid==s.q.gid&&a.mode==(0x8000|s.q.mode)&&a.nlink==1&&((a.dev>>8)&0xfff)==s.q.major&&((a.dev&255)|((a.dev>>12)&0xfff00))==s.q.minor;
 sha_init(&h);unsigned total=0;for(unsigned i=0;good&&i<1024;i++){int n=read(f,data,sizeof data);if(n<0){good=0;break;}if(!n)break;total+=n;if(total>2753064){good=0;break;}sha_add(&h,data,n);}
 sha_end(&h,digest);if(total!=2753064||!same(digest,target_file_sha,32)||syscall(197,f,&b)||a.dev!=b.dev||a.ino!=b.ino||a.size!=b.size||a.mode!=b.mode||a.uid!=b.uid||a.gid!=b.gid||a.nlink!=b.nlink||a.mtime!=b.mtime||a.mtimens!=b.mtimens||a.ctime!=b.ctime||a.ctimens!=b.ctimens)good=0;if(close(f))good=0;if(good)remember_hash("/usr/lib/viewUnified4.so",&b);return good;
}

static int ui_identity(void*x){(void)x;return epoch()&&tid()==s.q.ui_tid&&s.r.dispatch_flag==1&&s.r.return_matches;}
static void*head(void){return (uint8_t*)s.manager+4;}static void*mutex(void){return(uint8_t*)s.manager+8;}
static int snapshot_read(void*x,uint32_t a,void*out,uint32_t n){(void)x;return s.fd>=0&&n&&a7_menu_maps_contains(&snapshot_maps,a,n)&&pread64(s.fd,out,n,(long long)a)==(int)n;}
static int refresh_raw(void){int n=bounded("/proc/self/maps",maptext,sizeof maptext-1);return n>0&&a7_menu_parse_maps(maptext,n,&snapshot_maps);}
static int rt_maps_get(void);static void rt_maps_put(int);
static int refresh(void){if(rt_maps_get())return 1;uint32_t t=rt_perf_clock();int ok=refresh_raw();rt_maps_put(ok);rt_perf_end(RP_MAPS,t);return ok;}
static int hash_file(const char*path,uint32_t size,const uint8_t expected[32]){
 int f=open(path,0x88000);if(f<0)return 0;struct np_stat a,b;struct sha256 h;uint8_t data[4096],digest[32];int good=!syscall(197,f,&a)&&a.size==size&&(a.mode&0170000)==0100000&&a.nlink==1;
 sha_init(&h);unsigned total=0;for(unsigned i=0;good&&i<8192;i++){int n=read(f,data,sizeof data);if(n<0){good=0;break;}if(!n)break;total+=n;if(total>size){good=0;break;}sha_add(&h,data,n);}
 sha_end(&h,digest);if(total!=size||!same(digest,expected,32)||syscall(197,f,&b)||a.dev!=b.dev||a.ino!=b.ino||a.size!=b.size||a.mode!=b.mode||a.uid!=b.uid||a.gid!=b.gid||a.nlink!=b.nlink||a.mtime!=b.mtime||a.mtimens!=b.mtimens||a.ctime!=b.ctime||a.ctimens!=b.ctimens)good=0;if(close(f))good=0;if(good)remember_hash(path,&b);return good;
}
static struct bf_identity identity_of(const struct np_stat*a){return(struct bf_identity){a->dev,a->ino,a->size,a->mode,a->nlink,a->uid,a->gid,a->mtime,a->mtimens,a->ctime,a->ctimens};}
static const char*verified_path(unsigned i){return i==0?"/usr/lib/libObj.so":i==1?"/usr/lib/viewUnified2.so":i==2?"/usr/lib/viewUnified4.so":i==3?POWER_CORE_PATH:i==4?MP_CORE_PATH:i==5?VIEW3_PATH:0;}
static void remember_hash(const char*path,const struct np_stat*stat){for(unsigned i=0;i<6;i++)if(equal(path,verified_path(i))){verified_fw[i]=identity_of(stat);verified_fw_mask|=1u<<i;}}
static int verified_current(unsigned i){if(i>=6||!(verified_fw_mask&(1u<<i)))return 0;int f=open(verified_path(i),0xa0000);if(f<0)return 0;struct np_stat stat={0};int ok=!syscall(197,f,&stat);struct bf_identity now=identity_of(&stat);if(ok)ok=bf_identity_equal(&now,&verified_fw[i]);if(close(f))ok=0;return ok;}
static int verified_mp_current(void){return verified_current(4);}
static int view3_profile(const struct bf_identity*a){return a->dev==0xfc0f&&a->size==VIEW3_FILE_BYTES&&a->mode==(0100000|VIEW3_FILE_MODE)&&a->nlink==1&&a->uid==VIEW3_FILE_UID&&a->gid==VIEW3_FILE_GID;}
static int verified_view3_current(void){return view3_profile(&verified_fw[5])&&verified_current(5);}
static uint32_t resolve_slot(const struct class_slot_def*d,uint32_t obj,uint32_t view){if(d->symbol[0])return (uintptr_t)dlsym(0,d->symbol)+d->offset;if(d->module==1)return obj+d->offset;if(d->module==2)return view+d->offset;return 0;}
static int qualify_classes(uint32_t obj,uint32_t view){
 for(unsigned i=0;i<TREE_CLASS_COUNT;i++){const struct class_def*d=&class_defs[i];struct tw_class*k=&tree_classes[i];k->id=d->id;k->vptr=(d->module==1?obj:view)+d->vptr;k->flags=d->flags;k->last=resolve_slot(&d->slots[0],obj,view);k->on_mouse=resolve_slot(&d->slots[1],obj,view);k->hook=resolve_slot(&d->slots[2],obj,view);k->is_hit=resolve_slot(&d->slots[3],obj,view);k->def_setter=resolve_slot(&d->slots[4],obj,view);if(k->last!=obj+0x5c93a9||!k->on_mouse||!k->hook||!k->is_hit||((k->flags&TW_DEFAULT)&&!k->def_setter))return 0;
 }return 1;
}
static int tree_links(void*x,uint32_t a,uint32_t*parent,uint32_t*last,uint32_t*prev){
 (void)x;uint32_t b=s.r.obj_base;/* No virtual call or guessed embedded linkage offsets. */
 *parent=(uintptr_t)((void*(*)(void*))(uintptr_t)(b+0x5e2341))((void*)(uintptr_t)a);
 *last=(uintptr_t)((void*(*)(void*))(uintptr_t)(b+0x5c93a9))((void*)(uintptr_t)a);
 *prev=(uintptr_t)((void*(*)(void*))(uintptr_t)(b+0x5e235f))((void*)(uintptr_t)a);return 1;
}
static int selected_identity(uint32_t id){return snapshot.registry_is_singleton==1&&snapshot.configured_slots==39&&snapshot.selected_count==1&&equal(snapshot.selected_module,(id==96||id==0x44)?"viewUnified4.so":"viewUnified2.so")&&equal(snapshot.selected_factory,id==96?"ViewIroiroConToInstance":id==0x44?"ViewFnMenuToInstance":"ViewSettingMenuToInstance");}
static int inspect_to_raw(struct tr_report*d,uint32_t id,int want_tree,uint32_t sequence){
 struct tr_inspection*r=&d->inspection;for(unsigned i=0;i<sizeof*r;i++)((uint8_t*)r)[i]=0;r->abi=2;r->bytes=sizeof*r;r->selected_id=id;r->sequence=sequence;
 if(!epoch()||tid()!=s.q.ui_tid)return r->status=TP_IDENTITY;if(!refresh())return r->status=30;
 r->status=a7_menu_take_snapshot(snapshot_read,0,s.view_global,s.registry_singleton,id,&snapshot);
 r->selected_count=snapshot.selected_count;r->selected_node=snapshot.selected_node;r->active_count=snapshot.active_count;r->reads=snapshot.reads;r->read_bytes=snapshot.bytes;
 if(r->status)return r->status;if(!selected_identity(id))return r->status=31;
 for(unsigned i=0;i<sizeof r->module-1&&snapshot.selected_module[i];i++)r->module[i]=snapshot.selected_module[i];for(unsigned i=0;i<sizeof r->factory-1&&snapshot.selected_factory[i];i++)r->factory[i]=snapshot.selected_factory[i];
 uint32_t highest=0,matches=0;const struct a7_menu_view*active=0;
 for(unsigned i=0;i<snapshot.active_count;i++){const struct a7_menu_view*v=&snapshot.active[i];if(v->bank==0&&v->index>=highest){highest=v->index;r->top_id=v->id;r->top_obj=v->view;}if(v->id==id&&v->bank==0){active=v;matches++;}}
 if(!want_tree)return 0;struct tw_report*t=&d->tree;for(unsigned i=0;i<sizeof*t;i++)((uint8_t*)t)[i]=0;t->abi=2;t->bytes=sizeof*t;t->sequence=sequence;t->view_id=id;
 if(!matches){t->status=TW_NO_ROOT;return 0;}if(matches!=1||!active||active->registry_matches!=1||!equal(active->module,r->module)||!equal(active->factory,r->factory))return t->status=47;
 t->view_obj=active->view;t->view_vptr=active->vptr;struct info vi={0};
 if(!dladdr((void*)(uintptr_t)active->vptr,&vi)||!vi.base||(!equal(vi.file,id==96?"/usr/lib/viewUnified4.so":"/usr/lib/viewUnified2.so")&&!equal(vi.file,id==96?"//usr/lib/viewUnified4.so":"//usr/lib/viewUnified2.so")))return t->status=48;
 uint32_t base=(uintptr_t)vi.base,expected=base+(id==96?0x287c50:0x863890);uint8_t anchor[16],viewmem[0x98];uint32_t factory=base+(id==96?0xee500:0x20c348);
 if(active->vptr!=expected||!snapshot_read(0,factory,anchor,16)||!same(anchor,id==96?view4_factory_anchor:view2_factory_anchor,16)||!snapshot_read(0,active->view,viewmem,sizeof viewmem))return t->status=49;
 /* getWidget is the exact exported ViewBase primary-this method, not a
  * function read from a historic or arbitrary object's vtable. */
 void*(*get_widget)(void*,uint32_t)=(void*)(uintptr_t)(s.r.obj_base+0x3da895);
 t->root_ids[0]=id==96?0xdd716810:0xa056f604;t->root_ids[1]=id==96?0xf2bb65b2:0;t->root_count=id==96?2:1;
 struct tr_layout*l=&d->layout;for(unsigned i=0;i<sizeof*l;i++)((uint8_t*)l)[i]=0;l->abi=4;l->bytes=sizeof*l;l->primary_view=active->view;l->sequence=sequence;
 l->resource_group_id=(uint32_t)viewmem[0x74]|(uint32_t)viewmem[0x75]<<8|(uint32_t)viewmem[0x76]<<16|(uint32_t)viewmem[0x77]<<24;
 for(unsigned i=0;i<6;i++){const uint8_t*p=viewmem+0x80+4*i;l->layout_ids[i]=(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;if(l->layout_ids[i]!=0x35cc4d2e)l->active_layout_count++;}
 /* WidgetLayouter::getWidget(j,j,j,Error&) is STATIC: View+74 is a
  * resource-group ID, never a pointer. The exact original ViewBase getter
  * receives primary View this and handles all six layout IDs itself. */
 for(unsigned i=0;i<t->root_count;i++){
  l->get_widget_calls++;t->roots[i]=(uintptr_t)get_widget((void*)(uintptr_t)active->view,t->root_ids[i]);l->returned_roots[i]=t->roots[i];if(!t->roots[i])continue;l->root_id_mask|=1u<<i;
  uint32_t root_vptr=0,root_id=0,expected_vptr=s.r.lib_base+(id==96?(i?0x8b33a8:0x8b69e8):0x8bc9d0);
  if((t->roots[i]&3)||!snapshot_read(0,t->roots[i],&root_vptr,4)||!snapshot_read(0,t->roots[i]+0x24,&root_id,4)||root_vptr!=expected_vptr)return t->status=51;
  l->actual_runtime_ids[i]=root_id;
 }
 if(!t->roots[0]&&!t->roots[1]){t->status=TW_NO_ROOT;return 0;}
 if(want_tree==2)return 0;struct tw_ports p={0,snapshot_read,tree_links,tree_classes,TREE_CLASS_COUNT};return tw_capture(&p,t);
}
static int rt_inspect_get(struct tr_report*,uint32_t,int,uint32_t);
static void rt_inspect_put(const struct tr_report*,uint32_t,int,uint32_t,int);
static int inspect_to(struct tr_report*d,uint32_t id,int tree,uint32_t sequence){if(rt_inspect_get(d,id,tree,sequence))return 0;uint32_t t=rt_perf_clock();int rc=inspect_to_raw(d,id,tree,sequence);rt_inspect_put(d,id,tree,sequence,rc);rt_perf_end(RP_INSPECT,t);return rc;}
#include "runtime_native.inc"
static int as_ui_drive(void*record);
void tr_observed_impl(void*record,struct ts*due,void*data,uint32_t native_lr){
 (void)due;if(data!=&s||record!=s.record||s.r.phase!=1)return;
 s.r.callbacks++;s.r.callback_tid=tid();s.r.callback_pid=pid();s.r.dispatch_flag=*(uint32_t*)((uint8_t*)s.manager+0x20);s.r.callback_return=native_lr;s.r.return_matches=s.r.callback_return==s.expected_return;
 if(!ui_identity(0))s.r.last_error=TP_IDENTITY;else if(s.pending_op==1){if(s.auto_install){int deferred=as_ui_drive(record);if(deferred){rt_publish();return;}}else s.r.last_error=rt_bind_ui();}else if(s.pending_op==3)s.r.last_error=rt_install_ui();else if(s.pending_op==4)s.r.last_error=rt_detach_ui();else if(s.pending_op==6)s.r.last_error=rt_snapshot_ui();else if(s.pending_op==7)s.r.last_error=rt_synthetic_ui();else if(s.pending_op==8)s.r.last_error=rt_marker_ui();else if(s.pending_op==9)s.r.last_error=rt_choice_ui();else if(s.pending_op==10)s.r.last_error=rt_back_ui();else if(s.pending_op==11)s.r.last_error=rt_lease_ui();else s.r.last_error=20;
 if(s.auto_install&&s.r.last_error)as_ui_fail(&s.auto_ui,s.r.last_error);s.r.phase=2;s.record=0;s.remove(head(),record);rt_publish();
}
void tr_observed_failed(void*record,struct ts*due,void*data,uint32_t lr){(void)due;if(data!=&s||record!=s.record||lr!=s.expected_return)return;rt.gate=0;rt_pb_revoke(PB_CANCEL_THROW);input_tap_cancel(&rt.tap);s.r.last_error=180;if(s.auto_install)as_ui_fail(&s.auto_ui,180);s.r.phase=2;s.record=0;s.remove(head(),record);RW(7)=RW(55)=180;rt_publish();}
static int cleanup(void){if(!rt_pb_cleanup()){s.r.close_uncertain|=64;return 0;}if(!rt_ffc_cleanup())s.r.close_uncertain|=16;if(rt_view4_ref){void*p=rt_view4_ref;if(dlclose(p))s.r.close_uncertain|=8;else rt_view4_ref=0;}if(rt.power_ref){void*p=rt.power_ref;if(dlclose(p))s.r.close_uncertain|=4;else rt.power_ref=0;}if(s.fd>=0){int f=s.fd;s.fd=-1;if(close(f))s.r.close_uncertain|=1;}if(s.view){void*v=s.view;if(dlclose(v))s.r.close_uncertain|=2;else s.view=0;}return !s.r.close_uncertain;}
static int bind(void){
 s.r.bind_stage=1;if(!epoch())return 0;if(!as_preverified()&&(!file_identity()||!hash_file("/usr/lib/viewUnified2.so",VIEW2_SIZE,view2_sha)||!hash_file("/usr/lib/libObj.so",RUNTIME_OBJ_FILE_SIZE,runtime_obj_sha)||!hash_file(POWER_CORE_PATH,POWER_FILE_SIZE,power_file_sha)||!hash_file(MP_CORE_PATH,MP_FILE_BYTES,mp_file_sha)||!hash_file(VIEW3_PATH,VIEW3_FILE_BYTES,view3_file_sha)||!verified_view3_current()))return 0;s.r.identity_valid=1;
 /* Independent of view4's mapping: NOLOAD only the currently resident view2. */
 s.view=dlopen("/usr/lib/viewUnified2.so",6);if(!s.view)return 0;void*f=dlsym(s.view,"ViewSettingMenuToInstance");struct info v={0};
 if(!f||!dladdr(f,&v)||!v.base||(uintptr_t)f!=(uintptr_t)v.base+0x20c349||(!equal(v.file,"/usr/lib/viewUnified2.so")&&!equal(v.file,"//usr/lib/viewUnified2.so"))||!same((void*)((uintptr_t)f&~1u),view2_factory_anchor,16))return 0;
 s.r.lib_base=(uintptr_t)v.base;s.r.bind_stage=2;
 void**tm=dlsym(0,"timerManager");struct info d={0};if(!tm||!dladdr(tm,&d)||!d.base||(uintptr_t)tm-(uintptr_t)d.base!=0x12d4acc)return 0;uint32_t b=(uintptr_t)d.base;s.r.obj_base=b;
 for(unsigned i=0;i<4;i++)if(!same((void*)(uintptr_t)(b+obj_anchor_offsets[i]),obj_anchor_bytes[i],16))return 0;
 for(unsigned i=0;i<sizeof tree_anchor_offsets/sizeof tree_anchor_offsets[0];i++)if(!same((void*)(uintptr_t)(b+tree_anchor_offsets[i]),tree_anchor_bytes[i],16))return 0;
 if((uintptr_t)dlsym(0,"_ZN8ViewBase9getWidgetEj")!=b+0x3da895||!qualify_classes(b,s.r.lib_base))return 0;
 s.now=dlsym(0,"_Z10timeGetNowP8timespec");void**vm=dlsym(0,"pViewManager");if((uintptr_t)s.now!=b+0x82bf17||(uintptr_t)vm!=b+0x12d4c04)return 0;s.view_global=(uintptr_t)vm;s.registry_singleton=b+0x12d4e2c;
 s.fd=open("/proc/self/mem",0x80000);if(s.fd<0)return 0;uint32_t manager=0,owner[10];if(pread64(s.fd,&manager,4,(long long)(uintptr_t)tm)!=4||!manager||(manager&3)||pread64(s.fd,owner,sizeof owner,(long long)manager)!=sizeof owner||owner[9]!=s.q.pid)return 0;
 s.manager=(void*)(uintptr_t)manager;s.r.manager=manager;s.expected_return=b+0x82c267;s.add=(void*)(uintptr_t)(b+0x82c0ab);s.remove=(void*)(uintptr_t)(b+0x82beff);s.r.bind_stage=3;return as_preverified()?1:refresh();
}
static int queue(uint32_t op,struct tr_report*out){
 if(pthread_mutex_trylock(mutex()))return 1;s.queue_locked=1;if(s.r.phase==1||*(uint32_t*)((uint8_t*)s.manager+0x20)){pthread_mutex_unlock(mutex());s.queue_locked=0;return 2;}
 struct ts now={0};if(s.now(&now)||now.sec<0||now.nsec<0||now.nsec>=1000000000){pthread_mutex_unlock(mutex());s.queue_locked=0;return 25;}
 s.pending_op=op;s.r.callback_operation=op;s.r.phase=1;s.r.safe_to_unload=0;s.r.submit_tid=tid();s.r.last_error=0;s.record=s.add(head(),&now,tr_observed_guard,&s);int rc=s.record?0:26;if(!s.record){s.r.phase=3;s.r.safe_to_unload=1;}s.r.owns_reference=s.self!=0;*out=s.r;pthread_mutex_unlock(mutex());s.queue_locked=0;return rc;
}
static int prepare_mode(void*ref,const struct tr_request*q,uint32_t*adopted,struct tr_report*out,uint32_t origin){
 if(!ref||!q||!adopted||!out)return 20;*adopted=0;if(origin==1){uint32_t zero=0;if(!__atomic_compare_exchange_n(&as_claim,&zero,4,0,__ATOMIC_ACQ_REL,__ATOMIC_RELAXED))return 2;}if(!enter()){if(origin==1)__atomic_store_n(&as_claim,0,__ATOMIC_RELEASE);return origin==2?AS_SAFE_BUSY:1;}s.entry_tid=tid();if(s.initialized){int existing=origin==2&&s.origin==2&&s.self&&epoch()&&q->pid==s.q.pid&&q->start==s.q.start&&q->ui_tid==s.q.ui_tid&&q->inode==s.q.inode&&q->uid==s.q.uid&&q->gid==s.q.gid&&q->mode==s.q.mode&&same(q->boot,s.q.boot,40)&&!s.r.last_error&&!s.r.close_uncertain&&!rt.uncertain&&(s.r.phase==1||(rt.active&&rt.gate));leave();return existing?AS_ALREADY_ACCEPTED:origin==2?AS_ALREADY_STARTED:21;}s.initialized=1;s.origin=origin;s.auto_install=origin==2;s.q=*q;s.r.version=7;s.r.bytes=sizeof s.r;s.r.build=TR_BUILD;s.r.pid=q->pid;s.r.ui_tid=q->ui_tid;for(unsigned i=0;i<16;i++)s.r.nonce[i]=q->nonce[i];
 if(!bind()){s.r.last_error=22;if(!cleanup()){s.self=ref;*adopted=1;s.r.owns_reference=1;}if(origin==1&&!s.self)__atomic_store_n(&as_claim,0,__ATOMIC_RELEASE);*out=s.r;leave();return 22;}s.self=ref;*adopted=1;s.r.attempted=1;int rc=queue(1,out);if(rc){int cleaned=cleanup();if(cleaned){s.self=0;*adopted=0;}s.r.owns_reference=s.self!=0;if(origin==1&&!s.self)__atomic_store_n(&as_claim,0,__ATOMIC_RELEASE);*out=s.r;if(origin==2&&(rc==1||rc==2)&&cleaned&&!s.record&&!s.queue_locked){/* No native callback was queued and every owned resource closed. */rt_reset_unstarted();__builtin_memset((uint8_t*)&s+4,0,sizeof s-4);s.fd=-1;leave();return AS_SAFE_BUSY;}}leave();return rc;
}
static int prepare(void*ref,const struct tr_request*q,uint32_t*adopted,struct tr_report*out){return prepare_mode(ref,q,adopted,out,1);}

static int query(const struct tr_request*q,struct tr_report*out){
 if(!q||!out)return 20;if(!enter())return 1;if(!s.self||!request_match(q)||!epoch()){leave();return 23;}if(!s.manager){*out=s.r;leave();return s.autostart_preparing?2:22;}if(pthread_mutex_trylock(mutex())){leave();return 1;}if(s.r.phase==2)s.r.safe_to_unload=1;s.r.owns_reference=1;*out=s.r;if(!rt_cached(&out->runtime)){pthread_mutex_unlock(mutex());leave();return 1;}int rc=s.r.phase==1?2:0;pthread_mutex_unlock(mutex());leave();return rc;
}
static int action(const struct tr_request*q,struct tr_report*out){
 if(!q||!out||(q->op!=3&&q->op!=4&&q->op!=6&&q->op!=7&&q->op!=8&&q->op!=9&&q->op!=10&&q->op!=11))return 20;if(!enter())return 1;if(!s.self||!request_match(q)||!epoch()){leave();return 23;}if(s.autostart_preparing){*out=s.r;leave();return 2;}if(!s.manager||s.r.bind_stage!=3){*out=s.r;leave();return 22;}if(pthread_mutex_trylock(mutex())){leave();return 1;}int allowed=s.r.phase==2&&s.r.safe_to_unload&&s.inspect_attempts<256;pthread_mutex_unlock(mutex());if(!allowed){leave();return 24;}s.inspect_attempts++;s.r.attempted=1|(s.inspect_attempts<<8);rt_cmd_arg=q->arg0;int rc=queue(q->op,out);leave();return rc;
}
static int stop(const struct tr_request*q,void**transfer,struct tr_report*out){
 if(!q||!transfer||!out)return 20;*transfer=0;if(!enter())return 1;if(!s.self||!request_match(q)||!epoch()){leave();return 23;}if(s.autostart_preparing){*out=s.r;leave();return 2;}if(!s.manager){*out=s.r;leave();return 27;}if(pthread_mutex_trylock(mutex())){leave();return 1;}if(s.r.phase==1||!rt_retired()){*out=s.r;pthread_mutex_unlock(mutex());leave();return 24;}s.r.safe_to_unload=1;s.r.phase=3;pthread_mutex_unlock(mutex());int rc=cleanup()?0:27;if(!rc){*transfer=s.self;s.self=0;__atomic_store_n(&as_claim,0,__ATOMIC_RELEASE);}s.r.owns_reference=s.self!=0;*out=s.r;leave();return rc;
}
#include "autoload_native.inc"
static const struct tr_api api={7,sizeof api,TR_BUILD,prepare,query,action,stop,discover};
__attribute__((visibility("default")))const struct tr_api*a7r_touch_full_runtime_v7(void){return &api;}
