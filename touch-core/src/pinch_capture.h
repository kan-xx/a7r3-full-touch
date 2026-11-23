#ifndef A7_PINCH_CAPTURE_H
#define A7_PINCH_CAPTURE_H
#include <stdint.h>
#define PBC_CANVAS_RESOURCE 0x2030c734u
#define PBC_NATIVE_MASK 0x1000c0u
#define PBC_PARENT_DIAG_FIRST 732u
#define PBC_PARENT_DIAG_WORDS 8u
_Static_assert(PBC_PARENT_DIAG_FIRST+PBC_PARENT_DIAG_WORDS<=740u,"sync740 boundary");
struct pbc_source {uint32_t obj,view3,view,phase,ws,held,source_sequence,power,cookie,now;int16_t x[2],y[2];uint32_t view2;};
struct pbc_ports {void *context;int(*read)(void*,uint32_t,void*,uint32_t);int(*hit)(void*,uint32_t,int16_t,int16_t);uint32_t(*get_widget)(void*,uint32_t,uint32_t);int(*scope)(void*,const struct pbc_source*);};
struct pbc_evidence {uint32_t word[36],parent[8];};
enum {PBC_OK=0,PBC_ARGUMENT=1,PBC_READ=2,PBC_BUTTONS=3,PBC_CAPTURE=4,PBC_CLASS=5,PBC_OWNER=6,PBC_MODE=7,PBC_VISIBLE=8,PBC_SLOTS=9,PBC_HIT=10,PBC_CHANGED=11,PBC_VIEW=12,PBC_RESOURCE=13,PBC_PARENT_CLASS=14,PBC_PARENT_LINK=15,PBC_CALLBACK=16,PBC_PARENT_EVENTS=17,PBC_PARENT_MODE=18,PBC_SCOPE=19};
/* Current native getWidget2030c734 must resolve typed PAS_TouchPosition;
 * its exact child+1c0 must be native hit==capture. Callback/userdata, native
 * events31/30/33 and all21 sentinel slots/exact enable mask are verified.
 * Both parent and child are visible/current mode0, and both points hit both.
 * Native resource is freshly resolved a second time, with current scope/power
 * checks after EACH native call. No historical pointer or guessed canvas ID. */
int pbc_current(const struct pbc_ports*,const struct pbc_source*,struct pbc_evidence*);
#endif
