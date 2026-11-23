#ifndef TOUCH_SURFACE_ACTIONS_H
#define TOUCH_SURFACE_ACTIONS_H
#include <stdint.h>
enum surface_kind {SURFACE_MENU=1,SURFACE_POPUP=2,SURFACE_FN=3,SURFACE_IROIRO_NATIVE=4,SURFACE_FFC=5};
enum target_kind {TARGET_ROW=1,TARGET_TAB=2,TARGET_PAGE=3,TARGET_PAGE_PREV=4,TARGET_PAGE_NEXT=5,TARGET_BACK=6,TARGET_POPUP_ROW=7,TARGET_FN_CELL=8,TARGET_FFC_ROW=9};
struct surface_identity {uint32_t power,view_id,view,root,runtime_id,kind,layout,phase;int32_t tab,page;uint32_t popup,popup_node,first;};
struct surface_target {uint32_t kind,index,node,vptr,runtime_id,flags;};
/* validate must compare the full surface identity, not a lossy hash. Every
 * port below calls an anchored original native API on the original UI owner. */
struct surface_ports {
 void*context;
 int(*validate)(void*,const struct surface_identity*);
 int(*tab_count)(void*,uint32_t root);
 int(*page_count)(void*,uint32_t root,int32_t tab);
 int(*select_path)(void*,uint32_t view,int32_t tab,int32_t page,int32_t row);
 int(*refresh)(void*,uint32_t view);
 int(*direction)(void*,uint32_t view,uint32_t direction);
 int(*focus_row)(void*,const struct surface_identity*,int32_t row);
 int(*popup_count)(void*,uint32_t popup,int visible);
 int(*popup_first)(void*,uint32_t popup);
 int(*popup_position)(void*,uint32_t popup);
 int(*popup_style)(void*,uint32_t popup,int32_t absolute_index);
 int(*popup_move)(void*,uint32_t root,int next);
 int(*fn_focus)(void*,const struct surface_identity*,int32_t index);
 int(*submit)(void*,uint32_t view_id,int back);
};
enum surface_result {SURFACE_REJECT=0,SURFACE_FOCUSED=1,SURFACE_DISPATCHED=2};
/* Input coordinates are already converted by verified original native geometry.
 * Synthetic calls must pass activate=0: they can select but never enter/back. */
int surface_apply(const struct surface_ports*,const struct surface_identity*,const struct surface_target*,int activate);
int surface_identity_equal(const struct surface_identity*,const struct surface_identity*);
#endif
