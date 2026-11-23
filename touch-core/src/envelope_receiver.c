#include "envelope_receiver.h"
static int equal(const void*a,const void*b,unsigned n){const uint8_t*p=a,*q=b;unsigned d=0;for(unsigned i=0;i<n;i++)d|=p[i]^q[i];return !d;}
int envelope_receive(struct envelope_pending*s,const struct envelope_receiver_ports*p,const struct touch_envelope*e,uint32_t now,uint32_t serial,uint16_t held,int running){
 if(!s||!p||!e||!s->active||!p->snapshot||!p->gesture||!p->drained||!p->same_hit||!p->apply)return ENV_IGNORED;
 if(e->magic!=ENVELOPE_MAGIC||e->abi!=6||e->build!=ENVELOPE_BUILD||e->sequence!=s->sequence||!equal(e->nonce,s->nonce,sizeof e->nonce))return ENV_IGNORED;
 /* Revoke before any native mutation or helper that can throw/reenter. An old
  * or tampered matching envelope cannot be replayed after a partial action. */
 s->active=0;if(!equal(e,&s->expected,sizeof*e)||!running||held||serial!=e->post_commit_serial||now-e->release_ms>160||!p->gesture(p->context,e->post_commit_serial))return ENV_REVOKED;
 struct surface_identity current;if(!p->snapshot(p->context,&current)||!surface_identity_equal(&current,&e->surface)||!p->drained(p->context)||!p->gesture(p->context,e->post_commit_serial))return ENV_REVOKED;
 if(e->policy==ENV_REAL_TAP){if(!p->same_hit(p->context,e))return ENV_REVOKED;return p->apply(p->context,e,1)?ENV_APPLIED:ENV_REVOKED;}
 if(e->policy==ENV_BACK_ONLY){if(e->target.kind!=TARGET_BACK||current.view_id!=0xaa)return ENV_REVOKED;return p->apply(p->context,e,1)?ENV_APPLIED:ENV_REVOKED;}
 if(e->policy==ENV_FOCUS_ONLY){if(e->target.kind==TARGET_BACK)return ENV_REVOKED;return p->apply(p->context,e,0)?ENV_APPLIED:ENV_REVOKED;}
 if(e->policy!=ENV_CHOICE_OPEN_ONLY||!p->condition||!p->choice_center_sync||current.view_id!=0xaa||current.kind!=SURFACE_MENU||current.phase!=1||e->target.kind!=TARGET_ROW||e->target.index>=6)return ENV_REVOKED;
 if(!p->apply(p->context,e,0)||!p->snapshot(p->context,&current)||!surface_identity_equal(&current,&e->surface)||!p->drained(p->context)||!p->gesture(p->context,e->post_commit_serial))return ENV_REVOKED;
 if(p->condition(p->context,current.view,1)||!p->condition(p->context,current.view,2))return ENV_REVOKED;
 if(!p->snapshot(p->context,&current)||!surface_identity_equal(&current,&e->surface)||!p->drained(p->context)||!p->gesture(p->context,e->post_commit_serial))return ENV_REVOKED;
 return p->choice_center_sync(p->context,e)?ENV_APPLIED:ENV_REVOKED;
}
