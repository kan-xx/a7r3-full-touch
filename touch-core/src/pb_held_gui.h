#ifndef A7_PB_HELD_GUI_H
#define A7_PB_HELD_GUI_H
#include <stdint.h>
struct pbg_fence {uint32_t count,head,tail,buttons;int32_t token;};
struct pbg_source {uint32_t obj,held;};
struct pbg_ports {void*context;int(*read)(void*,uint32_t,void*,uint32_t);int(*fence)(void*,struct pbg_fence*);};
struct pbg_evidence {struct pbg_fence fence;uint32_t enabled,manager,manager_vptr,record,record_words[8],matching_mask,status,reason,read_mask;};
enum {PBG_OK=0,PBG_QUEUE=1,PBG_TOKEN=2,PBG_CHANGED=3,PBG_READ=4,PBG_ARGUMENT=5};
enum {PBG_DISABLED=1,PBG_MANAGER=2,PBG_MANAGER_CLASS=3,PBG_TOKEN_RANGE=4,PBG_RECORD_CLASS=5,PBG_RECORD_TOKEN=6,PBG_RECORD_STATE=7,PBG_CONTACT_RANGE=8,PBG_CONTACT_HELD=9,PBG_AMBIGUOUS=10,PBG_BUTTONS=11};
/* PB-only read proof. A held GUI token is an index, not a contact ID or mouse ID.
 * No native calls, writes, queue pumping, capture clearing or action authority.
 * Full caller UI/power/current-view/key/native-canvas qualification remains required. */
int pbg_check(const struct pbg_ports*,const struct pbg_source*,struct pbg_evidence*);
#endif
