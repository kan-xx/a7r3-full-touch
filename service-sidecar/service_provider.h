#ifndef A7_TOUCH_SERVICE_PROVIDER_H
#define A7_TOUCH_SERVICE_PROVIDER_H
#include "native.h"
enum sp_mode {SP_DISCOVERY=0,SP_CONTROL=1,SP_PREPARE=2};
enum sp_code {SP_AMBIGUOUS=0x7031,SP_PROVIDER_EXISTS=0x7032,SP_STARTING=0x7033,SP_BAD_PROVIDER=0x7034};
struct sp_binding {const struct tr_api*table;uint32_t base,getter,autostart;};
struct sp_ports {void*context;void*(*open)(void*,unsigned kind,int noload);int(*binding)(void*,void*,struct sp_binding*);int(*close)(void*,void*);};
struct sp_selection {void*handle;struct sp_binding binding;struct tr_discovery discovery;uint32_t kind,close_uncertain;};
/* kind0 permanent, kind1 temporary. Every returned function pointer is valid
 * ONLY while out.handle is held. No mutation is called while selecting. */
int sp_select(const struct sp_ports*,const struct tr_probe*,const struct tr_request*,enum sp_mode,struct sp_selection*);
/* Clear every callable pointer BEFORE close; never retain/cache across calls. */
int sp_release(const struct sp_ports*,struct sp_selection*);
#endif
