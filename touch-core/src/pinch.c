#include "pinch.h"
static uint32_t bump(uint32_t n){return ++n?n:1;}
static int valid(const PinchConfig*c){return c->min_span>=8&&c->min_span<=400&&c->rate_ms>=64&&c->rate_ms<=512&&c->max_gap_ms>=128&&c->max_gap_ms<=4096&&c->allow_single_entry==0;}
static unsigned pop(uint16_t n){unsigned c=0;while(n){c+=n&1;n>>=1;}return c;}
static int phaseeq(const PinchContext*a,const PinchContext*b){return a->phase==b->phase||(a->view_id==211&&b->view_id==211&&(a->phase==3||a->phase==4)&&(b->phase==3||b->phase==4));}
static int ctxeq(const PinchContext*a,const PinchContext*b){return a->epoch==b->epoch&&a->power==b->power&&a->view_id==b->view_id&&a->view_cookie==b->view_cookie&&phaseeq(a,b)&&a->layout==b->layout&&a->image_cookie==b->image_cookie;}
static int allowed(const PinchState*s,const PinchContext*c,uint32_t mask){
 if(!valid(&s->config))return 0;
 if(!c||!c->epoch||!c->power||!c->view_cookie||!c->image_cookie||(c->flags&mask)!=mask)return 0;
 if(c->view_id==211)return c->phase==3||c->phase==4;
 return 0;
}
static int peq(const PinchPacket*a,const PinchPacket*b){const unsigned char*x=(const unsigned char*)a,*y=(const unsigned char*)b;for(size_t i=0;i<sizeof(*a);i++)if(x[i]!=y[i])return 0;return 1;}
static uint32_t span(const PinchState*s,int16_t*cx,int16_t*cy){unsigned i=0,j=0;for(;i<16;i++)if(s->held&(1u<<i))break;for(j=i+1;j<16;j++)if(s->held&(1u<<j))break;if(j>=16)return 0;int32_t dx=s->last[i].x-s->last[j].x,dy=s->last[i].y-s->last[j].y;*cx=(int16_t)((s->last[i].x+s->last[j].x)/2);*cy=(int16_t)((s->last[i].y+s->last[j].y)/2);return (uint32_t)(dx*dx+dy*dy);}
static uint32_t motion_kind(const PinchState *state, uint32_t *distance2,
                            int16_t *center_x, int16_t *center_y) {
    *distance2 = span(state, center_x, center_y);
    if (*distance2 < state->config.min_span * state->config.min_span)
        return PP_NONE;
    if ((uint64_t)*distance2 * 64u >= (uint64_t)state->baseline2 * 81u)
        return PP_EXPAND;
    if ((uint64_t)*distance2 * 81u <= (uint64_t)state->baseline2 * 64u)
        return PP_CONTRACT;
    return PP_NONE;
}

int pinch_motion_pending(const PinchState *state) {
    if (!state || state->uncertain || state->blocked || !state->started ||
        pop(state->held) != 2 || !valid(&state->config))
        return 0;

    uint32_t distance2;
    int16_t center_x, center_y;
    return motion_kind(state, &distance2, &center_x, &center_y) != PP_NONE;
}

PinchConfig pinch_default_config(void){PinchConfig c={24,128,512,0};return c;}
int pinch_init(PinchState*s,const PinchConfig*c){if(!s)return 0;PinchState z={0};*s=z;s->serial=1;if(!c||!valid(c))return 0;s->config=*c;return 1;}
void pinch_cancel(PinchState*s){if(!s)return;s->serial=bump(s->serial);s->started=0;s->baseline2=0;s->blocked=s->held!=0;}
static void packet(PinchState*s,const PinchContext*c,const PinchPacket*p,uint32_t now){
 unsigned id=p->flags&15u;uint16_t bit=(uint16_t)(1u<<id);uint32_t t=p->ticks&0x7fffu;
 if(p->source!=0x11013001u){pinch_cancel(s);return;}
 if((s->seen&bit)&&peq(&s->last[id],p))return;
 /* Coalesced history can repeat an old sample; never resurrect an old press. */
 if((s->seen&bit)&&(((t-(s->last[id].ticks&0x7fffu))&0x7fffu)>=0x4000u))return;
 int down=(uint32_t)p->state==1u||(uint32_t)p->state==0x80000001u;
 int end=(uint32_t)p->state==0u||(uint32_t)p->state==2u||(uint32_t)p->state==3u||(uint32_t)p->state==0x80000002u;
 if(!down&&!end){pinch_cancel(s);return;}
 int late_end=0;
 if(s->clock_seen){uint32_t dt=(t-s->last_tick)&0x7fffu;
  if(dt>=0x4000u){if(!end)return;late_end=1;}
  if((!late_end&&(dt<<4)>s->config.max_gap_ms)||now-s->last_feed_ms>s->config.max_gap_ms)pinch_cancel(s);
 }
 /* An UP newer than THIS contact's last sample must clear it even when a
  * different contact has advanced the global watermark. Do not rewind that
  * watermark, and retain the per-contact old-sample rejection above. */
 s->clock_seen=1;if(!late_end)s->last_tick=t;s->last_feed_ms=now;s->last[id]=*p;s->seen|=bit;s->serial=bump(s->serial);
 if(end){s->held&=(uint16_t)~bit;if(s->started||s->tap_suppressed)pinch_cancel(s);if(!s->held){s->blocked=0;s->tap_suppressed=0;s->started=0;s->baseline2=0;}return;}
 if(!s->held){if(c)s->context=*c;else{PinchContext z={0};s->context=z;}}
 else if(!c||!ctxeq(c,&s->context))pinch_cancel(s);
 s->held|=bit;
 if(pop(s->held)>=2)s->tap_suppressed=1;
 if(!allowed(s,c,PF_FEED_REQUIRED)||(s->started&&!ctxeq(c,&s->context))||s->uncertain||(p->flags&16u)||p->x<0||p->x>=640||p->y<0||p->y>=480||pop(s->held)>2){pinch_cancel(s);return;}
 if(s->blocked)return;
 if(pop(s->held)==2&&!s->started){int16_t x,y;uint32_t d=span(s,&x,&y);if(d>=s->config.min_span*s->config.min_span){s->context=*c;s->baseline2=d;s->started=1;}}
}
void pinch_feed(PinchState*s,const PinchContext*c,const PinchPacket*p,size_t n,uint32_t now){if(!s)return;if(!p||!n||n>2){pinch_cancel(s);return;}for(size_t i=0;i<n;i++)packet(s,c,p+i,now);}
int pinch_plan(const PinchState*s,const PinchContext*c,uint32_t now,PinchPlan*out){
 if(!s||!out||s->uncertain||s->blocked||!s->started||pop(s->held)!=2||!allowed(s,c,PF_ACTION_REQUIRED)||!ctxeq(c,&s->context)||now-s->last_feed_ms>s->config.max_gap_ms||(s->action_seen&&now-s->last_action_ms<s->config.rate_ms))return 0;
 int16_t cx,cy;uint32_t d,k=motion_kind(s,&d,&cx,&cy);
 if(!k)return 0;
 PinchPlan z={0};z.context=*c;z.serial=s->serial;z.kind=k;z.span2=d;z.now_ms=now;z.held_mask=s->held;z.center_x=cx;z.center_y=cy;*out=z;return 1;
}
int pinch_plan_current(const PinchState*s,const PinchContext*c,const PinchPlan*p,uint32_t now){PinchPlan q;if(!p||!pinch_plan(s,c,now,&q))return 0;return p->serial==q.serial&&p->kind==q.kind&&p->span2==q.span2&&p->held_mask==q.held_mask&&ctxeq(&p->context,&q.context);}
void pinch_complete(PinchState*s,const PinchPlan*p,int normal,int uncertain){if(!s)return;if(uncertain){s->uncertain=1;pinch_cancel(s);return;}if(!normal||!p||p->serial!=s->serial)return;s->baseline2=p->span2;s->last_action_ms=p->now_ms;s->action_seen=1;s->serial=bump(s->serial);if(p->kind==PP_ENTER)pinch_cancel(s);}
