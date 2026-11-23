#ifndef TOUCH_POWER_GATE_H
#define TOUCH_POWER_GATE_H
#include <stdint.h>
struct pg_ports {void*context;int(*read)(void*,uint32_t,uint32_t*);int(*report)(void*,uint32_t*,uint32_t);};
struct pg_proof {uint32_t status,request,state,act_claim,event,generation,worker_generation,audio_phase,menu_phase;};
int pg_revoked(const struct pg_ports*,uint32_t*);
int pg_check(const struct pg_ports*,struct pg_proof*);
#endif
