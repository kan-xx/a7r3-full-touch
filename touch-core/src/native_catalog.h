#ifndef TOUCH_NATIVE_CATALOG_H
#define TOUCH_NATIVE_CATALOG_H
#include "surface_actions.h"
#include "surface_hit.h"
#define NS_TARGETS 48u
struct ns_io {
 void*context;
 int(*read)(void*,uint32_t,void*,uint32_t);
 int(*get_widget)(void*,uint32_t view,uint32_t resource,uint32_t*out);
 int(*parent)(void*,uint32_t widget,uint32_t*out);
 int(*count)(void*,uint32_t root,int kind,int32_t tab,int32_t page);
 int(*hit)(void*,uint32_t widget,const struct surface_point*);
 int(*local)(void*,uint32_t widget,const struct surface_point*,struct surface_point*);
 int(*width)(void*,uint32_t widget,float*);
 int(*height)(void*,uint32_t widget,float*);
 int(*popup_value)(void*,uint32_t popup,int kind,int32_t index);
};
struct ns_input {uint32_t obj,view2,view4,view_id,view,power,qualified;};
struct ns_catalog {struct surface_identity identity;uint32_t status,count;int32_t row_count,tab_count,page_count;struct surface_target targets[NS_TARGETS];};
/* Exact original view/widget classes and pointers are checked here; native API
 * implementations/ELF anchors, source UI caller and actual power request must
 * already be qualified by the single runtime owner. */
int ns_collect(const struct ns_io*,const struct ns_input*,struct ns_catalog*);
/* Revalidate only one already-cataloged target immediately before a native
 * mutation. Does not grant a new page or return an unseen target. */
int ns_target_current(const struct ns_io*,const struct ns_input*,const struct ns_catalog*,const struct surface_target*);
int ns_hit(void*context,const struct surface_target*,const struct surface_point*);
struct ns_hit_context {const struct ns_io*io;const struct ns_catalog*catalog;};
#endif
