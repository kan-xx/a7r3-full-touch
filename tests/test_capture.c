#include "pinch_capture.h"
#ifdef A7_ARM_MIN_DECL
extern int printf(const char*,...);
#else
#include <stdio.h>
#endif
struct fixture {uint8_t view[8],mouse[20],parent[0x600];unsigned reads,hits,getters,scopes,fail_read,change_read,scope_bad,getter_change,hit_bad,child_extra,change_default;};
static struct fixture f;static struct pbc_source s;static struct pbc_evidence e;
static void zero(void*p,unsigned n){uint8_t*b=p;while(n--)*b++=0;}static void copy(void*d,const void*p,unsigned n){uint8_t*x=d;const uint8_t*y=p;while(n--)*x++=*y++;}static void put(unsigned off,uint32_t v){copy(f.parent+off,&v,4);}
#define P 0x40000000u
#define CHILD (P+0x1c0u)
static int read_port(void*u,uint32_t a,void*out,uint32_t n){(void)u;f.reads++;if(f.reads==f.fail_read)return 0;if(f.reads==f.change_read){if(f.change_default)put(0x180,3);else put(0x1c0+0x160,0);}const uint8_t*p=0;uint32_t b=0,z=0;if(a>=s.view&&a<s.view+8){p=f.view;b=s.view;z=8;}if(a>=s.ws+0x4a8&&a<s.ws+0x4bc){p=f.mouse;b=s.ws+0x4a8;z=20;}if(a>=P&&a<P+sizeof f.parent){p=f.parent;b=P;z=sizeof f.parent;if(a==CHILD+0x15c||a==CHILD+0x2a4)f.child_extra++;}if(!p||n>z||a-b>z-n)return 0;copy(out,p+a-b,n);return 1;}
static int hit_port(void*u,uint32_t w,int16_t x,int16_t y){(void)u;(void)x;(void)y;f.hits++;if(w==CHILD)return !(f.hit_bad&1);if(w==P)return !(f.hit_bad&2);return 0;}
static uint32_t getter(void*u,uint32_t view,uint32_t hash){(void)u;f.getters++;if(view!=s.view||hash!=PBC_CANVAS_RESOURCE)return 0;return f.getter_change&&f.getters==2?P+0x1000:P;}
static int scope(void*u,const struct pbc_source*x){(void)u;(void)x;f.scopes++;return f.scopes!=f.scope_bad;}
static void reset(void){zero(&f,sizeof f);zero(&s,sizeof s);s.obj=0x10000000;s.view2=0x18000000;s.view3=0x20000000;s.view=0x30000000;s.phase=3;s.ws=0x50000000;s.held=3;s.source_sequence=s.power=s.cookie=1;s.now=100;s.x[0]=100;s.x[1]=300;s.y[0]=100;s.y[1]=200;uint32_t v=s.view3+0xa8f48;copy(f.view,&v,4);v=3;copy(f.view+4,&v,4);v=1;copy(f.mouse+8,&v,4);v=CHILD;copy(f.mouse+12,&v,4);copy(f.mouse+16,&v,4);
 put(0,s.view2+0x8bf418);put(0x1c,P);put(0x14c,211);put(0x38,2);put(0x590,0x31);put(0x594,0x30);put(0x5a0,0x33);put(0x1c0,s.obj+0x11ed7a0);put(0x1c0+0x10,P+4);put(0x1c0+0x24,77);put(0x1c0+0x38,2);put(0x1c0+0x14c,211);put(0x1c0+0x15c,P);put(0x1c0+0x160,s.view2+0x57e629);for(unsigned i=0;i<21;i++){put(0x1c0+0x2a4+8*i,0x11012004);put(0x1c0+0x2a4+8*i+4,(PBC_NATIVE_MASK>>i)&1u);}}
static unsigned cases,checks,line;
#define CK(x)do{checks++;if(!(x)){line=__LINE__;return 1;}}while(0)
static int run(void){struct pbc_ports p={0,read_port,hit_port,getter,scope};
 cases++;reset();CK(!pbc_current(&p,&s,&e)&&f.hits==4&&f.getters==2&&f.scopes==6&&f.reads==26&&e.word[26]);CK(e.word[16]==PBC_NATIVE_MASK&&e.parent[5]==0x31&&e.parent[6]==0x30&&e.parent[7]==0x33);
 cases++;reset();s.held=0x10003;CK(pbc_current(&p,&s,&e)==PBC_ARGUMENT&&!f.reads&&!f.getters);
 cases++;reset();s.x[1]=640;CK(pbc_current(&p,&s,&e)==PBC_HIT&&f.hits==2);
 cases++;reset();put(0,1);CK(pbc_current(&p,&s,&e)==PBC_PARENT_CLASS&&f.reads==3&&!f.child_extra&&!f.hits);
 cases++;reset();put(0x1c0,1);CK(pbc_current(&p,&s,&e)==PBC_CLASS&&!f.child_extra&&!f.hits);
 cases++;reset();put(0x1c0+0x10,P+8);CK(pbc_current(&p,&s,&e)==PBC_PARENT_LINK);
 cases++;reset();put(0x1c,P+4);CK(pbc_current(&p,&s,&e)==PBC_PARENT_LINK);
 for(unsigned n=0;n<2;n++){cases++;reset();put(n?0x1c0+0x14c:0x14c,175);CK(pbc_current(&p,&s,&e)==PBC_OWNER);}
 for(unsigned n=1;n<=3;n++){cases++;reset();f.parent[0x1c0+0x21]=n;CK(pbc_current(&p,&s,&e)==PBC_MODE);cases++;reset();f.parent[0x21]=n;CK(pbc_current(&p,&s,&e)==PBC_PARENT_MODE);}
 cases++;reset();put(0x180,2);CK(pbc_current(&p,&s,&e)==PBC_OK);
 cases++;reset();put(0x180,2);f.change_read=14;f.change_default=1;CK(pbc_current(&p,&s,&e)==PBC_CHANGED);
 for(unsigned n=0;n<2;n++){cases++;reset();put(n?0x1c0+0x38:0x38,0);CK(pbc_current(&p,&s,&e)==PBC_VISIBLE);}
 for(unsigned n=0;n<2;n++){cases++;reset();put(0x1c0+(n?0x160:0x15c),0);CK(pbc_current(&p,&s,&e)==PBC_CALLBACK);}
 for(unsigned n=0;n<3;n++){cases++;reset();put(n==0?0x590:n==1?0x594:0x5a0,0x130d3030);CK(pbc_current(&p,&s,&e)==PBC_PARENT_EVENTS);}
 for(unsigned n=0;n<21;n++){cases++;reset();put(0x1c0+0x2a4+8*n,0x30);CK(pbc_current(&p,&s,&e)==PBC_SLOTS);cases++;reset();put(0x1c0+0x2a4+8*n+4,((PBC_NATIVE_MASK>>n)&1u)^1u);CK(pbc_current(&p,&s,&e)==PBC_SLOTS);}
 for(unsigned n=1;n<=26;n++){cases++;reset();f.fail_read=n;CK(pbc_current(&p,&s,&e)==PBC_READ);}
 for(unsigned n=1;n<=6;n++){cases++;reset();f.scope_bad=n;CK(pbc_current(&p,&s,&e)==PBC_SCOPE);if(n>=2&&n<=5)CK(f.hits==n-1&&f.getters==1);}
 cases++;reset();f.getter_change=1;CK(pbc_current(&p,&s,&e)==PBC_CHANGED&&f.getters==2);
 cases++;reset();f.change_read=14;CK(pbc_current(&p,&s,&e)==PBC_CHANGED);
 for(unsigned n=1;n<=2;n++){cases++;reset();f.hit_bad=n;CK(pbc_current(&p,&s,&e)==PBC_HIT);}
 return 0;}
int capture_selftest(void){cases=checks=line=0;int rc=run();printf("pinch_capture cases=%u checks=%u error=%u\n",cases,checks,line);return rc;}
