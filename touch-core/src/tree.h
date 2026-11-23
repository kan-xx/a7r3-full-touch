#ifndef TOUCH_TREE_H
#define TOUCH_TREE_H
#include <stdint.h>
#define TW_NODES 128u
#define TW_BUTTONS 24u
#define TW_ANCESTORS 16u
enum {TW_DEFAULT=1,TW_BUTTON=2,TW_POPUP=4};
enum {TW_OK=0,TW_BADARG=40,TW_READ=41,TW_VTABLE=42,TW_LINK=43,TW_LIMIT=44,TW_BUDGET=45,TW_NO_ROOT=46};
struct tw_class {uint32_t id,vptr,last,on_mouse,hook,is_hit,def_setter,flags;};
/* Geometry is a read of existing Widget caches, never a lazy getter and never
 * a fresh hit-test/action lease. Raw IEEE754 words preserve NaN bit patterns. */
struct tw_node {uint32_t address,vptr,class_id,role,depth,parent,last_child,prev_sibling,mode,flags,runtime_widget_id,on_mouse,hook,is_hit,default_present,default_mode,aux_kind,aux1,aux2,geometry_present;uint32_t geometry[14];int32_t margin[4];};
struct tw_button {uint32_t address,enabled_mask,event_ids[21],reserved;};
struct tw_report {uint32_t abi,bytes,status,sequence,view_id,view_obj,view_vptr,roots[2],root_ids[2],root_count,node_count,button_count,unknown_count,truncated,reads,read_bytes,failed_address,reserved[5];struct tw_node nodes[TW_NODES];struct tw_button buttons[TW_BUTTONS];};
struct tw_ports {void*context;int(*read)(void*,uint32_t,void*,uint32_t);int(*links)(void*,uint32_t,uint32_t*,uint32_t*,uint32_t*);const struct tw_class*classes;uint32_t class_count;};
int tw_capture(const struct tw_ports*,struct tw_report*);
#if defined(__arm__)
_Static_assert(sizeof(struct tw_node)==152,"node ABI");
_Static_assert(sizeof(struct tw_button)==96,"button ABI");
_Static_assert(sizeof(struct tw_report)==21856,"tree ABI");
#endif
#endif
