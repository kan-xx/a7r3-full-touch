#ifndef A7_MENU_FOCUS_V13_H
#define A7_MENU_FOCUS_V13_H
#include "native_menu_contract.h"
/* Same-row optimization ONLY AFTER original model->view sync. complete proves
 * sync's void early-return did not leave a stale row cache. validate checks the
 * exact pre-touch surface/page/power and current target again after sync. */
static int a7_menu_focus_row_v13(const struct a7_native_menu_api*a,void*v,void*w,int32_t row,int activate,int(*complete)(void*)){
 (void)&a7_menu_focus_row; /* keep the original contract available for baseline verification */
 uint8_t*view=v,*widget=w;
 if(!a||!v||!w||!complete||!a->validate_current(v,w))return A7_MENU_REJECT;
 if(*(uint32_t*)(view+0x130)!=1||view[0x194])return A7_MENU_REJECT;
 a->sync(v);
 if(!a->validate_current(v,w)||*(uint32_t*)(view+0x130)!=1||view[0x194]||!complete(v))return A7_MENU_REJECT;
 int32_t tab=*(int32_t*)(view+0x134),page=*(int32_t*)(view+0x138);
 int32_t count=a->row_count(w,tab,page);
 if(tab<1||page<1||count<1||count>6||row<0||row>=count)return A7_MENU_REJECT;
 if(!widget[0x1bc]&&*(int32_t*)(view+0x13c)==row&&*(int32_t*)(widget+0x1d4)==tab&&*(int32_t*)(widget+0x1d8)==page&&*(int32_t*)(widget+0x1dc)==row){
  if(!a->validate_current(v,w)||*(uint32_t*)(view+0x130)!=1||view[0x194]||widget[0x1bc]||*(int32_t*)(view+0x134)!=tab||*(int32_t*)(view+0x138)!=page||*(int32_t*)(view+0x13c)!=row)return A7_MENU_REJECT;
 }else{
  if(widget[0x1bc]){
   a->direction(v,1);
   if(!a->validate_current(v,w)||widget[0x1bc]||*(uint32_t*)(view+0x130)!=1)return A7_MENU_REJECT;
  }
  struct a7_menu_path path={tab,page,row,-1,1,{0,0,0}};
  a->set_path(v,path);
  a->refresh(v);
  if(!a->validate_current(v,w)||*(int32_t*)(view+0x134)!=tab||*(int32_t*)(view+0x138)!=page||*(int32_t*)(view+0x13c)!=row)return A7_MENU_REJECT;
 }
 if(!activate)return A7_MENU_FOCUSED;
 if(a->condition(v,1)){a->dispatch(v,10,0,0);return A7_MENU_DISPATCHED_EXECUTE;}
 if(a->condition(v,2)){a->dispatch(v,1,0,0);return A7_MENU_DISPATCHED_POPUP;}
 return A7_MENU_FOCUSED;
}
#endif
