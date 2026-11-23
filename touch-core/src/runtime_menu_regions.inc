/* D10: local MENU-only hit extension. No raw callback/timer/action changes.
 * The exact FW3.10 InfraTp singleton cell and vptr follow original
 * GetInstance843B38 and ctor8438A4, pinned by verified libObj file SHA.
 * GetInstance is NOT called (it increments refs/may allocate). */
#include "menu_regions.h"
static int rt_menu_region_map(const struct ns_io*io,struct mtr_map*m){
 __builtin_memset(m,0,sizeof*m);
 if(rt_catalog.identity.view_id!=0xaa||rt_catalog.identity.kind!=SURFACE_MENU||rt_catalog.identity.layout!=1||rt_catalog.identity.phase!=1)return 0;
 uint32_t tp=0,vp=0,again=0,wh[2]={0};uint32_t base=s.r.obj_base;
 if(!rt_read(base+0x12d4ebc,&tp,4)||!tp||!rt_read(tp,&vp,4)||vp!=base+0x1205820||!rt_read(tp+0x58,wh,8)||!rt_read(base+0x12d4ebc,&again,4)||again!=tp)return 0;
 return mtr_build(io,&rt_catalog,wh[0],wh[1],m);
}
/* Called ONLY after finite fence COMMIT or at deferred receiver re-hit. It is
 * intentionally absent from rt_current/raw-MOVE processing. Map never escapes
 * this UI-owner stack frame and is rebuilt on receiver validation. */
static int rt_pick_tap_body(const struct surface_point*a,const struct surface_point*b){
 struct ns_io io=rt_io();struct ns_hit_context original={&io,&rt_catalog};
 if(rt_catalog.identity.view_id==0xaa&&rt_catalog.identity.kind==SURFACE_MENU&&rt_catalog.identity.layout==1&&rt_catalog.identity.phase==1){
  struct mtr_map map;
  if(rt_menu_region_map(&io,&map)){
   struct mtr_context extended={&io,&rt_catalog,&map};
   return surface_pick_tap(rt_catalog.targets,rt_catalog.count,&extended,mtr_hit,a,b);
  }
 }
 /* Popup/Fn/FFC and unknown/new geometry retain the existing exact algorithm. */
 return surface_pick_tap(rt_catalog.targets,rt_catalog.count,&original,rt_target_hit,a,b);
}

static int rt_pick_tap(const struct surface_point*a,const struct surface_point*b){uint32_t began=rt_perf_clock();int rc=rt_pick_tap_body(a,b);rt_perf_end(RP_HIT,began);return rc;}
