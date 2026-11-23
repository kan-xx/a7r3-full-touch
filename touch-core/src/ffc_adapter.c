#include "ffc_adapter.h"
static void zero(void*p,size_t n){uint8_t*b=p;while(n--)*b++=0;}
static int bytes_same(const void*a,const void*b,size_t n){const uint8_t*x=a,*y=b;while(n--)if(*x++!=*y++)return 0;return 1;}
static void ticket(struct ffc_result*r,const struct ffc_live*f,enum ffc_key key){r->cancel.valid=1;r->cancel.pid=f->authority.pid;r->cancel.owner=f->authority.owner;r->cancel.serial=f->serial;r->cancel.key=key;for(unsigned i=0;i<16;i++)r->cancel.nonce[i]=f->authority.nonce[i];}
static int finish(const struct ffc_action_ports*p,struct ffc_pair*q,struct ffc_pair_report*b,struct ffc_result*r,int status){
 int cleanup=ffc_pair_dispose(&p->raw,q,b);r->allocated+=b->allocated;r->freed+=b->released;r->presses+=q->attempted[0];r->releases+=q->attempted[1];
 if(cleanup==FFC_NATIVE_UNCERTAIN||b->retained){r->native_uncertain=1;r->cancel.valid=0;for(unsigned i=0;i<2;i++)r->retained_event[i]=b->retained_event[i];status=FFC_NATIVE_UNCERTAIN;}
 r->status=(uint32_t)status;return status;
}
static int current(const struct ffc_action_ports*p,const struct ffc_live*lease,struct ffc_live*f,uint32_t row,uint32_t held,int back){
 int rc=ffc_project(&p->projection,f);if(rc)return rc;
 if(!ffc_same_lease(lease,f)||f->row!=row||f->held!=held||(!back&&f->pending))return FFC_BAD_SCOPE;
 return FFC_OK;
}
static int pair(const struct ffc_action_ports*p,const struct ffc_live*lease,enum ffc_key key,uint32_t row,uint32_t after_row,struct ffc_result*r){
 struct ffc_pair q;struct ffc_pair_report b;struct ffc_live f;int rc;zero(&q,sizeof q);zero(&b,sizeof b);
 rc=ffc_pair_prepare(&p->raw,key,&q,&b);if(rc){r->allocated+=b.allocated;r->freed+=b.released;r->status=(uint32_t)rc;return rc;}
 rc=current(p,lease,&f,row,0,key==FFC_BACK);if(rc)return finish(p,&q,&b,r,rc);
 rc=ffc_pair_emit(&p->raw,&q,0,&f,&b);if(rc)return finish(p,&q,&b,r,rc);
 /* The press returned normally. This scalar ticket NEVER authorizes replay of
  * a release; a later fresh same-serial held may only be cancelled by close. */
 ticket(r,&f,key);
 rc=current(p,lease,&f,after_row,(uint32_t)key,key==FFC_BACK);
 if(rc)return finish(p,&q,&b,r,FFC_ABORTED_HELD);
 /* Independent fresh lookup immediately before the release, after proving the
  * native navigation effect. A lost power/serial/scope never gets a late key. */
 rc=current(p,lease,&f,after_row,(uint32_t)key,key==FFC_BACK);
 if(rc)return finish(p,&q,&b,r,FFC_ABORTED_HELD);
 rc=ffc_pair_emit(&p->raw,&q,1,&f,&b);
 if(rc)return finish(p,&q,&b,r,rc==FFC_NATIVE_UNCERTAIN?rc:FFC_ABORTED_HELD);
 if(key==FFC_CENTER||key==FFC_BACK)
  return finish(p,&q,&b,r,FFC_SUBMITTED); /* No borrowed-view reads after commit. */
 rc=current(p,lease,&f,after_row,0,0);
 if(rc)return finish(p,&q,&b,r,FFC_ABORTED_HELD);
 r->cancel.valid=0;r->navigation_pairs++;
 return finish(p,&q,&b,r,FFC_OK);
}
static int begin(const struct ffc_action_ports*p,struct ffc_result*r,struct ffc_live*f,int back){
 if(!r)return FFC_BAD_API;zero(r,sizeof*r);r->status=FFC_BAD_API;r->target_row=UINT32_MAX;
 if(!p)return FFC_BAD_API;int rc=ffc_project(&p->projection,f);if(rc){r->status=(uint32_t)rc;return rc;}
 if(f->held||(!back&&f->pending)){r->status=FFC_BUSY;return FFC_BUSY;}
 return FFC_OK;
}
int ffc_click(const struct ffc_action_ports*p,int32_t x,int32_t y,struct ffc_result*r){
 struct ffc_live lease;int rc=begin(p,r,&lease,0);if(rc)return rc;
 int target=ffc_hit(&lease,x,y);if(target<0){r->status=FFC_NO_HIT;return FFC_NO_HIT;}
 r->target_row=(uint32_t)target;uint32_t row=lease.row,down=((uint32_t)target+6-row)%6,up=(row+6-(uint32_t)target)%6;
 enum ffc_key direction=down<=up?FFC_DOWN:FFC_UP;uint32_t steps=down<=up?down:up;
 if(steps>3){r->status=FFC_BAD_SCOPE;return FFC_BAD_SCOPE;}
 for(unsigned i=0;i<steps;i++){uint32_t next=direction==FFC_UP?(row+5)%6:(row+1)%6;rc=pair(p,&lease,direction,row,next,r);if(rc)return rc;row=next;}
 return pair(p,&lease,FFC_CENTER,row,row,r);
}
int ffc_back(const struct ffc_action_ports*p,struct ffc_result*r){struct ffc_live lease;int rc=begin(p,r,&lease,1);if(rc)return rc;return pair(p,&lease,FFC_BACK,lease.row,lease.row,r);}
int ffc_cancel_known(const struct ffc_action_ports*p,const struct ffc_cancel_ticket*t,struct ffc_result*r){
 struct ffc_live first,fresh;struct ffc_cancel_ticket supplied;
 if(!r)return FFC_BAD_API;
 if(t){supplied=*t;t=&supplied;} /* Permit &result.cancel with result reused. */
 zero(r,sizeof*r);r->status=FFC_NO_CANCEL;
 if(!p||!p->close_current||!t||t->valid!=1||t->key<FFC_UP||t->key>FFC_BACK)return FFC_NO_CANCEL;
 int rc=ffc_project(&p->projection,&first);if(rc){r->status=(uint32_t)rc;return rc;}
 if(first.authority.pid!=t->pid||first.authority.owner!=t->owner||first.serial!=t->serial||first.held!=t->key||!bytes_same(first.authority.nonce,t->nonce,16))return FFC_NO_CANCEL;
 rc=ffc_project(&p->projection,&fresh);if(rc||!ffc_same_lease(&first,&fresh)||fresh.held!=t->key){r->status=FFC_BAD_SCOPE;return FFC_BAD_SCOPE;}
 rc=p->close_current(p->projection.context,&fresh);
 r->status=rc==1?FFC_CANCEL_SUBMITTED:rc<0?FFC_NATIVE_UNCERTAIN:FFC_BAD_SCOPE;
 r->native_uncertain=rc<0;return (int)r->status; /* No old-view reads, no reopen. */
}
