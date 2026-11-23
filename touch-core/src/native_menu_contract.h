/* Offline implementation contract, not deployed.
 * All function pointers must be base+Thumb anchors from native-menu-anchors.json.
 * Caller: verified original UI owner callback, current ViewSettingMenu only.
 */
#ifndef A7R3_NATIVE_MENU_CONTRACT_H
#define A7R3_NATIVE_MENU_CONTRACT_H
#include <stdint.h>
struct a7_menu_path { int32_t tab,page,row,unused; uint8_t valid,pad[3]; };
_Static_assert(sizeof(struct a7_menu_path)==20,"original ARM struct-by-value ABI");
struct a7_native_menu_api {
 void (*sync)(void *view);                  /* view2+20d593 */
 void (*set_path)(void *view,struct a7_menu_path); /* view2+2157b1 */
 void (*refresh)(void *view);               /* view2+20e509 */
 int32_t (*row_count)(void *widget,int32_t tab,int32_t page); /* view2+56d5c3 */
 uint32_t (*condition)(void *view,uint32_t condition); /* view2+2108a9 */
 void (*dispatch)(void *view,uint32_t action,uint32_t r2,uint32_t r3); /* view2+2179db */
 void (*direction)(void *view,uint32_t direction); /* view2+21588d */
 int (*validate_current)(void *view,void *widget); /* project-provided current-view/lease check */
};
enum a7_touch_menu_result { A7_MENU_REJECT=0,A7_MENU_FOCUSED=1,A7_MENU_DISPATCHED_POPUP=2,A7_MENU_DISPATCHED_EXECUTE=3 };
/* row is derived from actual live row geometry. Tab/page are kept unchanged.
 * No global mode changes, no key Event substitution, no direct setting writes.
 * Ordinary rows with original condition1/2 are supported; special node types
 * remain focused without being executed.
 */
static int a7_menu_focus_row(const struct a7_native_menu_api *a,void *v,void *w,int32_t row,int activate) {
 uint8_t *view=(uint8_t*)v,*widget=(uint8_t*)w;
 if(!a||!v||!w||!a->validate_current(v,w))return A7_MENU_REJECT;
 if(*(uint32_t*)(view+0x130)!=1 || view[0x194])return A7_MENU_REJECT;
 a->sync(v);
 int32_t tab=*(int32_t*)(view+0x134),page=*(int32_t*)(view+0x138);
 int32_t count=a->row_count(w,tab,page);
 if(tab<1||page<1||count<1||count>6||row<0||row>=count)return A7_MENU_REJECT;
 /* If the tab header currently owns the cursor, native DOWN enters row0 and
  * clears the internal header-focus flag. Do not raw-write widget+1bc. */
 if(widget[0x1bc]) {
  a->direction(v,1);
  if(!a->validate_current(v,w)||widget[0x1bc]||*(uint32_t*)(view+0x130)!=1)return A7_MENU_REJECT;
 }
 struct a7_menu_path path={tab,page,row,-1,1,{0,0,0}};
 a->set_path(v,path);
 a->refresh(v);
 if(!a->validate_current(v,w)||*(int32_t*)(view+0x134)!=tab||*(int32_t*)(view+0x138)!=page||*(int32_t*)(view+0x13c)!=row)return A7_MENU_REJECT;
 if(!activate)return A7_MENU_FOCUSED;
 /* These are the same first two predicates/actions used by original EventId5
  * in layout1. A nonzero node+a0 veto is checked within each predicate. */
 if(a->condition(v,1)){a->dispatch(v,10,0,0);return A7_MENU_DISPATCHED_EXECUTE;}
 if(a->condition(v,2)){a->dispatch(v,1,0,0);return A7_MENU_DISPATCHED_POPUP;}
 return A7_MENU_FOCUSED;
}
#endif
