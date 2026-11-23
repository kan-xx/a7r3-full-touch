/* Offline, focus-only implementation contract. Not installed in camera.
 * Caller: native UI owner, true Fn view 0x44; same active-view/epoch lease.
 * Original dispatcher must be viewUnified4 base+0x19432f after anchors/hash check.
 */
#ifndef A7R3_NATIVE_FN_FOCUS_CONTRACT_H
#define A7R3_NATIVE_FN_FOCUS_CONTRACT_H
#include <stdint.h>
struct a7_native_fn_api {
 uint32_t (*dispatch)(void *view,uint32_t action);
 int (*validate_current)(void *view); /* verify 0x44, primary vptr, epoch, LCD */
};
enum a7_fn_focus_result { A7_FN_REJECT=0,A7_FN_FOCUSED=1 };
static int32_t a7_fn_load_i32(const uint8_t*p){int32_t v;__builtin_memcpy(&v,p,4);return v;}
static int a7_fn_focus_grid_cell(const struct a7_native_fn_api *a,void *v,int32_t target) {
 uint8_t *view=(uint8_t*)v;
 if(!a||!a->dispatch||!a->validate_current||!v||!a->validate_current(v)||target<0||target>=12)return A7_FN_REJECT;
 /* layout2 is the original 12-cell grid. State1 excludes active edits/belts.
  * No writes to view+14c, model nodes, modes, or disabled flags. */
 if(a7_fn_load_i32(view+4)!=2||a7_fn_load_i32(view+0x360)!=1||view[0x37c]||view[0x201])return A7_FN_REJECT;
 if(a7_fn_load_i32(view+0x12c)<0||a7_fn_load_i32(view+0x12c)>1)return A7_FN_REJECT;
 int32_t current=a7_fn_load_i32(view+0x14c);
 if(current<0||current>=12||a7_fn_load_i32(view+0x298+target*8)==-0xe1)return A7_FN_REJECT;
 /* Target nonempty also proves the original internal skip-empty loop has a
  * terminating cell. Bounded adapter loop still verifies progress/lease. */
 for(uint32_t hops=0;hops<12;++hops) {
  if(!a->validate_current(v)||a7_fn_load_i32(view+4)!=2||a7_fn_load_i32(view+0x360)!=1||view[0x37c])return A7_FN_REJECT;
  if(current==target)return A7_FN_FOCUSED;
  a->dispatch(v,1); /* native next-cell action, including update/check helpers */
  if(!a->validate_current(v))return A7_FN_REJECT;
  int32_t next=a7_fn_load_i32(view+0x14c);
  if(next<0||next>=12||next==current||a7_fn_load_i32(view+0x298+next*8)==-0xe1)return A7_FN_REJECT;
  current=next;
 }
 return A7_FN_REJECT;
}
#endif
