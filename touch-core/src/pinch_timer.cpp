#include "pinch_timer.h"
class timer_lock {
 const pb_timer_ports *p;pb_timer_result *r;bool own;
 public:timer_lock(const pb_timer_ports *a,pb_timer_result *b):p(a),r(b),own(false){}
 bool acquire(int already){if(already)return true;if(p->trylock(p->mutex)){r->status=PBT_BUSY;return false;}own=true;return true;}
 ~timer_lock() noexcept {if(own){try{if(p->unlock(p->mutex))r->status=PBT_UNLOCK;else r->unlocked=1;}catch(...){r->status=PBT_UNLOCK;}}}
};
static bool valid(const pb_timer_ports *p,int locked){return p&&p->head&&p->mutex&&p->trylock&&p->unlock&&(locked==0||locked==1);}
extern "C" void pb_timer_add(const pb_timer_ports *p,ts *due,void(*callback)(void*,ts*,void*),void *data,int locked,pb_timer_result *r){
 if(!r)return;*r={};if(!valid(p,locked)||!p->add||!due||!callback||!data){r->status=PBT_BAD;return;}
 try{timer_lock guard(p,r);if(!guard.acquire(locked))return;r->called=1;r->handle=p->add(p->head,due,callback,data);r->returned=1;}
 catch(...){if(!r->status)r->status=PBT_THROW;}
}
extern "C" void pb_timer_remove(const pb_timer_ports *p,void *record,int locked,pb_timer_result *r){
 if(!r)return;*r={};if(!valid(p,locked)||!p->remove||!record){r->status=PBT_BAD;return;}
 try{timer_lock guard(p,r);if(!guard.acquire(locked))return;r->called=1;p->remove(p->head,record);r->returned=1;}
 catch(...){if(!r->status)r->status=PBT_THROW;}
}
