/* Include in runtime_native.inc AFTER rt_gesture/rt_drained/rt_nav_api, with
 * ffc_runtime_bridge.h + ffc_geometry.h and the prototypes below visible before
 * rt_current. No MP body hash is performed on the UI thread. The host supplies
 * verified_mp_current(): exact70160B/SHA qualification from service prepare or
 * sliced AS bf ticks, followed by the existing small inode/stat ticket check. */
#define RT_FFC_PATH "/usr/lib/liba7r_menu_product_v21.so"
#define RT_FFC_PATH_DOUBLE "//usr/lib/liba7r_menu_product_v21.so"
static struct ffc_runtime_bridge rt_ffc;
static void*rt_ffc_ref;static uint32_t rt_ffc_base,rt_ffc_api,rt_ffc_query;
static struct a7_menu_snapshot rt_ffc_snapshot;
static int verified_mp_current(void);
static int rt_ffc_bind(void){
 if(mp_optional_absent)return 0;
 if(rt_ffc_ref)return verified_mp_current();
 if(!verified_mp_current())return 0;
 void*h=dlopen(RT_FFC_PATH,6);if(!h)return 0;
 void*f=dlsym(h,"a7r_menu_product_v21");struct info d={0},da={0},dq={0};uint32_t api[9];
 static const uint8_t raw[16]={0x2d,0xe9,0xf0,0x4f,0x83,0xb0,0x04,0x46,0x00,0xaf,0x08,0x46,0x89,0x46,0xcc,0xf4};
 uint32_t b=0,actual=0;
 int ok=f&&dladdr(f,&d)&&d.base&&(b=(uintptr_t)d.base)&&
  (equal(d.file,RT_FFC_PATH)||equal(d.file,RT_FFC_PATH_DOUBLE))&&
  same((void*)(uintptr_t)(s.r.obj_base+0x8305bc),raw,16);
 if(ok){
  const void*aptr=((const void*(*)(void))f)();
  actual=(uintptr_t)aptr;
  ok=aptr&&dladdr(aptr,&da)&&da.base==d.base&&
     rt_read(actual,api,sizeof api)&&api[0]==1&&api[1]==36&&api[2]==0x3110d015&&
     api[4]&&dladdr((const void*)(uintptr_t)api[4],&dq)&&dq.base==d.base&&verified_mp_current();
 }
 if(!ok){if(dlclose(h))rt.uncertain=1;return 0;}
 rt_ffc_ref=h;rt_ffc_base=b;rt_ffc_api=actual;rt_ffc_query=api[4];return 1;
}
/* Read-only scope for projection, including stock's legitimate pressed queue.
 * Only the final strict projection/raw boundary requires drain/buttons zero. */
static int rt_ffc_owner(int metadata){
 uint32_t vm=0,vs=0,ws=0,cb=0;uint8_t adjust=0;uint32_t viewmode=0;
 if(!rt_ffc_ref||pid()!=s.q.pid||tid()!=s.q.ui_tid||!rt_ffc_base||!rt_ffc_query||!rt_power()||
  (metadata&&(!epoch()||!verified_mp_current()))||
  !rt_read(s.view_global,&vm,4)||!vm||!rt_read(vm+0x14,&vs,4)||vs!=rt.vs||
  !rt_read(vs+8,&ws,4)||ws!=rt.ws||!rt_read(rt.callback_cell,&cb,4)||
  cb!=(rt.active?(uintptr_t)rt_input_hook:rt.original)||
  !rt_read(s.r.obj_base+0x129f120,&adjust,1)||adjust||
  !rt_read(s.r.obj_base+0x12d4e28,&viewmode,4)||viewmode)return 0;
 return 1;
}
static int rt_ffc_query_port(void*x,uint32_t out[FFC_REPORT_BYTES/4]){
 (void)x;if(!rt_ffc_owner(0))return 3;
 return((int(*)(void*))(uintptr_t)rt_ffc_query)(out);
}
static int rt_ffc_authority(void*x,struct ffc_authority*out){
 (void)x;if(!out||!rt_ffc_owner(1))return 0;
 uint32_t report[FFC_REPORT_BYTES/4];if(rt_ffc_query_port(0,report))return 0;
 __builtin_memset(out,0,sizeof*out);out->flags=FFC_SOURCE_AUTHORITY;
 InputTapFence fence={0};if(rt_drain_snapshot(&fence)){
  if(!fence.count&&!fence.head&&!fence.tail&&fence.gui_token==-1)out->flags|=FFC_DRAINED;
  if(!fence.mouse0_buttons&&!rt.tap.held_mask)out->flags|=FFC_PHYSICAL_BUTTONS_ZERO;
 }
 out->pid=s.q.pid;out->owner=s.q.ui_tid;out->power_generation=RW(60);out->scope_epoch=rt.epoch_tag;
 out->api_version=1;out->api_bytes=36;out->api_build=0x3110d015;out->mp_base=rt_ffc_base;out->obj_base=s.r.obj_base;
 __builtin_memcpy(out->nonce,(uint8_t*)report+1132,16);return 1;
}
static int rt_ffc_registry(void*x,struct ffc_registry*out){
 (void)x;if(!out||!rt_ffc_owner(0)||a7_menu_take_snapshot(snapshot_read,0,s.view_global,s.registry_singleton,0xffc,&rt_ffc_snapshot))return 0;
 const struct a7_menu_snapshot*z=&rt_ffc_snapshot;__builtin_memset(out,0,sizeof*out);
 out->vm=z->manager;out->configured_slots=z->configured_slots;out->registry_singleton=z->registry_is_singleton;out->selected_count=z->selected_count;
 uint32_t seen=0,highest=0;
 for(unsigned i=0;i<z->active_count;i++){
  const struct a7_menu_view*v=&z->active[i];
  if(v->bank==0&&v->index>=highest){highest=v->index;out->top_id=v->id;out->top_index=v->index;out->top_object=v->view;}
  if(v->id!=0xffc)continue;out->ffc_count++;
  if(v->bank>1||(seen&(1u<<v->bank)))return 0;seen|=1u<<v->bank;
  struct ffc_slot*t=&out->banks[v->bank];t->bank=v->bank;t->index=v->index;t->id=v->id;t->object=v->view;t->vptr=v->vptr;t->registry_matches=v->registry_matches;
  __builtin_memcpy(t->module,v->module,96);__builtin_memcpy(t->factory,v->factory,96);
 }
 return seen==3;
}
static int rt_ffc_geometry_boundary(void*x,uint32_t label,const struct ffc_authority*a){
 (void)x;uint32_t report[FFC_REPORT_BYTES/4],vp=0,fields[3],labels[8],live=0;
 if(!a||!rt_ffc_owner(0)||RW(60)!=a->power_generation||rt.epoch_tag!=a->scope_epoch||
  rt_ffc_query_port(0,report)||!same((uint8_t*)report+1132,a->nonce,16)||
  report[4]!=a->pid||report[5]!=a->owner||report[25]!=a->obj_base||!report[15]||
  !rt_read(report[15]+0x30,fields,sizeof fields)||!fields[0]||!fields[1]||fields[2]!=report[48]||
  !rt_read(fields[0]+6468,labels,sizeof labels)||!rt_read(fields[0]+7100,&live,4)||live!=8||
  !rt_read(label,&vp,4)||vp!=a->obj_base+0x11eab90)return 0;
 for(unsigned i=1;i<=6;i++)if(labels[i]==label)return 1;return 0;
}
static int rt_ffc_rect(void*x,uint32_t label,const struct ffc_authority*a,struct ffc_rect*out){
 (void)x;uint32_t b=s.r.obj_base;
 struct ffc_geometry_ports p={0,rt_ffc_geometry_boundary,(void*)(uintptr_t)(b+0x5c8ec5),(void*)(uintptr_t)(b+0x5c8fc9),(void*)(uintptr_t)(b+0x5c8fd7),1};
 return ffc_measure_label(&p,label,a,out);
}
static struct ffc_projection_ports rt_ffc_projection(void){return(struct ffc_projection_ports){0,rt_ffc_authority,rt_ffc_query_port,rt_ffc_registry,snapshot_read,rt_ffc_rect};}
static int rt_ffc_collect(struct ns_catalog*out){
 if(!rt_ffc_bind())return 0;struct ffc_projection_ports p=rt_ffc_projection();return ffr_catalog(&rt_ffc,&p,out);
}
static int rt_ffc_raw_boundary(void*x,const struct ffc_live*f,unsigned release){
 (void)x;(void)release;
 if(!rt.active||!rt.gate||rt.uncertain||!rt_action_serial||!rt_gesture(0,rt_action_serial)||!rt_drained(0))return 0;
 struct ffc_projection_ports p=rt_ffc_projection();struct ffc_live fresh;
 return ffc_project(&p,&fresh)==FFC_OK&&ffc_same_lease(f,&fresh)&&fresh.row==f->row&&fresh.held==f->held&&fresh.pending==f->pending&&ffr_raw_boundary(&rt_ffc,&fresh);
}
static int rt_ffc_close_current(void*x,const struct ffc_live*f){
 (void)x;if(!rt_gesture(0,rt_action_serial)||!rt_drained(0))return 0;
 struct ffc_projection_ports p=rt_ffc_projection();struct ffc_live fresh;
 if(ffc_project(&p,&fresh)||!ffc_same_lease(f,&fresh)||fresh.held!=f->held)return 0;
 struct a7_navigation_api a;if(!rt_nav_api(&a))return 0;
 a7_navigation_target(&a,1,0xffc,0,&s.r.navigation);
 if(s.r.navigation.caught_exception&&s.r.navigation.exception_unlock){rt.gate=0;rt.uncertain=1;return -1;}
 return !s.r.navigation.status&&s.r.navigation.submitted?1:0; /* NO borrowed read. */
}
static struct ffc_action_ports rt_ffc_action_ports(void){
 struct ffc_action_ports p;__builtin_memset(&p,0,sizeof p);p.projection=rt_ffc_projection();
 p.raw=(struct ffc_raw_ports){0,malloc,free,(void*)(uintptr_t)(s.r.obj_base+0x82cffb),(void*)(uintptr_t)(s.r.obj_base+0x8305bd),rt_ffc_raw_boundary,1};p.close_current=rt_ffc_close_current;
 return p;
}
static int rt_ffc_apply(const struct touch_envelope*e,int activate){
 rt_segment_invalidate();
 struct ffc_action_ports p=rt_ffc_action_ports();
 int ok=ffr_apply(&rt_ffc,&p,e,activate);
 rt_segment_invalidate();
 __builtin_memcpy(&RW(908),&rt_ffc.result,sizeof rt_ffc.result);RW(900)=rt_ffc.catalogs;RW(901)=rt_ffc.seals;RW(902)=rt_ffc.actions;RW(903)=rt_ffc.rejected;
 if(rt_ffc.uncertain){rt.gate=0;rt.uncertain=1;RW(55)=206;}
 return ok;
}
static int rt_target_hit(void*x,const struct surface_target*t,const struct surface_point*p){return t&&t->kind==TARGET_FFC_ROW?ffr_hit(&rt_ffc,t,p):ns_hit(x,t,p);}
static int rt_ffc_cleanup(void){
 ffr_revoke(&rt_ffc);if(!rt_ffc_ref)return 1;
 if(dlclose(rt_ffc_ref))return 0;rt_ffc_ref=0;rt_ffc_base=rt_ffc_api=rt_ffc_query=0;return 1;
}
static void rt_ffc_reset(void){__builtin_memset(&rt_ffc,0,sizeof rt_ffc);rt_ffc_ref=0;rt_ffc_base=rt_ffc_api=rt_ffc_query=0;}
