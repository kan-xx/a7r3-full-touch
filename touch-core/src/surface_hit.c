#include "surface_hit.h"
static unsigned priority(unsigned kind){return kind==TARGET_BACK?100:kind==TARGET_PAGE?10:20;}
int surface_pick(const struct surface_target*t,uint32_t n,void*x,surface_native_hit hit,const struct surface_point*p){
 if(!t||!hit||!p||n>48)return -1;int found=-1,ambiguous=0;unsigned best=~0u;
 for(uint32_t i=0;i<n;i++){if(!(t[i].flags&1)||!hit(x,t+i,p))continue;unsigned rank=priority(t[i].kind);if(rank<best){best=rank;found=i;ambiguous=0;}else if(rank==best&&(found<0||t[i].kind!=t[found].kind||t[i].index!=t[found].index))ambiguous=1;}
 return ambiguous?-1:found;
}
int surface_pick_tap(const struct surface_target*t,uint32_t n,void*x,surface_native_hit hit,const struct surface_point*a,const struct surface_point*b){int i=surface_pick(t,n,x,hit,a),j=surface_pick(t,n,x,hit,b);return i>=0&&j>=0&&t[i].kind==t[j].kind&&t[i].index==t[j].index?j:-1;}
