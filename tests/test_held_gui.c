#include <stdint.h>
#ifndef A7_ARM_MIN_DECL
#include <stdio.h>
#include <string.h>
#else
extern int printf(const char*,...);extern void*memcpy(void*,const void*,unsigned);extern void*memset(void*,int,unsigned);
#endif
#include "pb_held_gui.h"
enum {O=0x10000000,M=0x20000000};
static uint32_t table[33],manager,reads,fences,failread,failfence,mutread,mutkind,cases,checks,error;static uint8_t enabled;static struct pbg_fence fence;
static int rd(void*x,uint32_t a,void*d,uint32_t n){(void)x;reads++;if(reads==failread)return 0;
 if(mutread==reads){if(mutkind==1)table[1+2*8+3]=4;if(mutkind==2)enabled=0;if(mutkind==3)manager=M+0x100;if(mutkind==4)fence.token=0;if(mutkind==5)table[0]++;}
 if(a==O+0x129ee55&&n==1){memcpy(d,&enabled,1);return 1;}if(a==O+0x129ee60&&n==4){memcpy(d,&manager,4);return 1;}if((a==M||a==M+0x100)&&n==4){memcpy(d,table,4);return 1;}if((a==M+4||a==M+0x104)&&n==sizeof table-4){memcpy(d,table+1,n);return 1;}return 0;}
static int fs(void*x,struct pbg_fence*d){(void)x;fences++;if(fences==failfence)return 0;if(fences==2&&mutkind==4)fence.token=0;*d=fence;return 1;}
static void reset(void){memset(table,0,sizeof table);table[0]=O+0x11ed420;for(unsigned i=0;i<4;i++){table[1+8*i]=O+0x11efd38;table[2+8*i]=0xffffffffu;table[3+8*i]=0xffffffffu;}table[2]=9;table[3]=0;table[4]=3;table[18]=7;table[19]=2;table[20]=3;enabled=1;manager=M;reads=fences=failread=failfence=mutread=mutkind=0;fence=(struct pbg_fence){0,0,0,1,2};}
#define CHECK(v)do{checks++;if(!(v)){error=__LINE__;return 1;}}while(0)
static int run(void){struct pbg_ports p={0,rd,fs};struct pbg_source s={O,(1u<<7)|(1u<<9)};struct pbg_evidence d;
 cases++;reset();CHECK(pbg_check(&p,&s,&d)==PBG_OK&&d.record==M+4+2*32&&d.record_words[1]==7&&d.record_words[2]==2&&d.matching_mask==4&&d.read_mask==0x1f1f);
 cases++;reset();table[1+8+2]=2;table[1+8+3]=0;CHECK(pbg_check(&p,&s,&d)==PBG_OK);/* inactive stale token is not allocated */
 cases++;reset();table[1+8+2]=2;table[1+8+3]=4;CHECK(pbg_check(&p,&s,&d)==PBG_TOKEN&&d.reason==PBG_AMBIGUOUS);
 for(unsigned i=0;i<3;i++){cases++;reset();if(i==0)fence.count=1;if(i==1)fence.head=17;if(i==2)fence.tail=29;CHECK(pbg_check(&p,&s,&d)==PBG_QUEUE);}
 const int32_t badtokens[4]={-1,-2,4,0x7fffffff};for(unsigned i=0;i<4;i++){cases++;reset();fence.token=badtokens[i];CHECK(pbg_check(&p,&s,&d)==PBG_TOKEN&&d.reason==PBG_TOKEN_RANGE);}
 const unsigned states[3]={0,2,4};for(unsigned i=0;i<3;i++){cases++;reset();table[20]=states[i];CHECK(pbg_check(&p,&s,&d)==PBG_TOKEN&&d.reason==PBG_RECORD_STATE);}
 const uint32_t ids[3]={0,16,0xffffffffu};for(unsigned i=0;i<3;i++){cases++;reset();table[18]=ids[i];CHECK(pbg_check(&p,&s,&d)==PBG_TOKEN&&d.reason==(i?PBG_CONTACT_RANGE:PBG_CONTACT_HELD));}
 cases++;reset();fence.count=1;fence.token=-1;CHECK(pbg_check(&p,&s,&d)==PBG_TOKEN&&d.reason==PBG_TOKEN_RANGE);
 cases++;reset();table[19]=1;CHECK(pbg_check(&p,&s,&d)==PBG_TOKEN&&d.reason==PBG_RECORD_TOKEN);
 cases++;reset();table[17]++;CHECK(pbg_check(&p,&s,&d)==PBG_TOKEN&&d.reason==PBG_RECORD_CLASS);
 cases++;reset();table[0]++;CHECK(pbg_check(&p,&s,&d)==PBG_TOKEN&&d.reason==PBG_MANAGER_CLASS&&reads==6);
 const uint32_t ptrs[3]={0,M+1,0xfffffff0u};for(unsigned i=0;i<3;i++){cases++;reset();manager=ptrs[i];CHECK(pbg_check(&p,&s,&d)==PBG_TOKEN&&d.reason==PBG_MANAGER);}
 for(unsigned i=0;i<2;i++){cases++;reset();enabled=i?2:0;CHECK(pbg_check(&p,&s,&d)==PBG_TOKEN&&d.reason==PBG_DISABLED);}
 const uint32_t buttons[3]={0,2,3};for(unsigned i=0;i<3;i++){cases++;reset();fence.buttons=buttons[i];CHECK(pbg_check(&p,&s,&d)==PBG_TOKEN&&d.reason==PBG_BUTTONS);}
 for(unsigned i=1;i<=8;i++){cases++;reset();failread=i;CHECK(pbg_check(&p,&s,&d)==PBG_READ);}
 for(unsigned i=1;i<=2;i++){cases++;reset();failfence=i;CHECK(pbg_check(&p,&s,&d)==PBG_READ);}
 for(unsigned i=1;i<=5;i++){cases++;reset();mutkind=i;mutread=5;CHECK(pbg_check(&p,&s,&d)==PBG_CHANGED);}
 const uint32_t held[4]={0,1,7,0x10003};for(unsigned i=0;i<4;i++){cases++;reset();s.held=held[i];CHECK(pbg_check(&p,&s,&d)==PBG_ARGUMENT);}s.held=(1u<<7)|(1u<<9);
 cases++;reset();CHECK(pbg_check(0,&s,&d)==PBG_ARGUMENT);cases++;reset();CHECK(pbg_check(&p,0,&d)==PBG_ARGUMENT);cases++;reset();CHECK(pbg_check(&p,&s,0)==PBG_ARGUMENT);
 return 0;}
int held_gui_selftest(void){int rc=run();printf("pb_held_gui cases=%u checks=%u error=%u\n",cases,checks,error);return rc;}
