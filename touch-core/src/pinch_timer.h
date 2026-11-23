#ifndef A7_PINCH_TIMER_H
#define A7_PINCH_TIMER_H
#include <stdint.h>
struct ts;
struct pb_timer_ports {
 void *head,*mutex;
 int(*trylock)(void*),(*unlock)(void*);
 void*(*add)(void*,struct ts*,void(*)(void*,struct ts*,void*),void*);
 void(*remove)(void*,void*);
};
struct pb_timer_result {uint32_t status,called,returned,unlocked;void *handle;};
enum{PBT_OK=0,PBT_BUSY=1,PBT_THROW=2,PBT_UNLOCK=3,PBT_BAD=4};
#ifdef __cplusplus
extern "C" {
#endif
void pb_timer_add(const struct pb_timer_ports*,struct ts*,void(*)(void*,struct ts*,void*),void*,int,struct pb_timer_result*);
void pb_timer_remove(const struct pb_timer_ports*,void*,int,struct pb_timer_result*);
#ifdef __cplusplus
}
#endif
#endif
