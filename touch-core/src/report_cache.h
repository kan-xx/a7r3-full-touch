#ifndef TOUCH_REPORT_CACHE_H
#define TOUCH_REPORT_CACHE_H
#include <stdint.h>
struct rc_cache {uint32_t seq,words[1024];};
void rc_publish(struct rc_cache*,const uint32_t*);
int rc_read(const struct rc_cache*,uint32_t*);
#endif
