#include "tree.h"
struct cursor{const struct tw_ports*p;struct tw_report*r;};
static int get(struct cursor*c,uint32_t a,void*out,uint32_t n){struct tw_report*r=c->r;if(r->reads>=2048||n>262144-r->read_bytes){r->status=TW_BUDGET;r->failed_address=a;return 0;}if(!a||a>UINT32_MAX-n||!c->p->read(c->p->context,a,out,n)){r->status=TW_READ;r->failed_address=a;return 0;}r->reads++;r->read_bytes+=n;return 1;}
static const struct tw_class*known(struct cursor*c,uint32_t v){for(uint32_t i=0;i<c->p->class_count;i++)if(c->p->classes[i].vptr==v)return &c->p->classes[i];return 0;}
static struct tw_node*seen(struct cursor*c,uint32_t a){for(uint32_t i=0;i<c->r->node_count;i++)if(c->r->nodes[i].address==a)return &c->r->nodes[i];return 0;}
/* Every call through links is preceded by exact qualified class/vtable checks.
 * Unknown classes are reported with address/vptr only; no assumed base layout,
 * no methods, default mode, coordinates, or descendant access are attempted. */
static struct tw_node*node(struct cursor*c,uint32_t a,uint32_t role,uint32_t depth){
 if(!a||(a&3)){c->r->status=TW_LINK;return 0;}if(seen(c,a)){c->r->status=TW_LINK;return 0;}
 if(c->r->node_count==TW_NODES){c->r->truncated=1;c->r->status=TW_LIMIT;return 0;}
 struct tw_node*n=&c->r->nodes[c->r->node_count++];n->address=a;n->role=role;n->depth=depth;
 if(!get(c,a,&n->vptr,4))return 0;const struct tw_class*k=known(c,n->vptr);if(!k){c->r->unknown_count++;return n;}n->class_id=k->id;
 uint8_t base[40];uint32_t last=0,mouse[2],hit=0,def=0;
 if(!get(c,a,base,sizeof base)||!get(c,n->vptr+0x14,&last,4)||!get(c,n->vptr+0x68,mouse,8)||!get(c,n->vptr+0x90,&hit,4))return 0;
 uint32_t v=(uint32_t)base[0]|(uint32_t)base[1]<<8|(uint32_t)base[2]<<16|(uint32_t)base[3]<<24;
 if(v!=k->vptr||last!=k->last||mouse[0]!=k->on_mouse||mouse[1]!=k->hook||hit!=k->is_hit){c->r->status=TW_VTABLE;return 0;}
 n->mode=base[0x21];n->flags=base[0x20];n->runtime_widget_id=(uint32_t)base[0x24]|(uint32_t)base[0x25]<<8|(uint32_t)base[0x26]<<16|(uint32_t)base[0x27]<<24;n->on_mouse=mouse[0];n->hook=mouse[1];n->is_hit=hit;
 uint8_t margins[8];if(!get(c,a+0x4c,n->geometry,32)||!get(c,a+0x88,n->geometry+8,24)||!get(c,a+0x130,margins,8))return 0;for(unsigned i=0;i<4;i++){uint32_t v=(uint32_t)margins[2*i]|(uint32_t)margins[2*i+1]<<8;n->margin[i]=v&0x8000?(int32_t)v-65536:(int32_t)v;}n->geometry_present=1;
 if(k->flags&TW_DEFAULT){if(!get(c,n->vptr+0x108,&def,4)||!get(c,a+0x180,&n->default_mode,4))return 0;if(def!=k->def_setter){c->r->status=TW_VTABLE;return 0;}n->default_present=1;}
 if(k->flags&TW_POPUP){uint32_t x[2];if(!get(c,a+0x1c4,x,8))return 0;n->aux_kind=1;n->aux1=x[0];n->aux2=x[1];}
 if(k->flags&TW_BUTTON){if(c->r->button_count==TW_BUTTONS){c->r->truncated=1;}else{struct tw_button*b=&c->r->buttons[c->r->button_count++];uint8_t slots[168];if(!get(c,a+0x2a4,slots,sizeof slots))return 0;b->address=a;for(unsigned i=0;i<21;i++){const uint8_t*s=slots+8*i;b->event_ids[i]=(uint32_t)s[0]|(uint32_t)s[1]<<8|(uint32_t)s[2]<<16|(uint32_t)s[3]<<24;if(s[4])b->enabled_mask|=1u<<i;}}}
 if(!c->p->links(c->p->context,a,&n->parent,&n->last_child,&n->prev_sibling)){c->r->status=TW_LINK;c->r->failed_address=a;return 0;}
 if((n->parent&3)||(n->last_child&3)||(n->prev_sibling&3)||n->parent==a||n->last_child==a||n->prev_sibling==a){c->r->status=TW_LINK;c->r->failed_address=a;return 0;}return n;
}
static int descend(struct cursor*c,struct tw_node*p,unsigned depth){
 if(!p->class_id)return 1;if(depth>32){c->r->truncated=1;c->r->status=TW_LIMIT;return 0;}
 uint32_t at=p->last_child;for(unsigned siblings=0;at;siblings++){
  if(siblings==TW_NODES){c->r->status=TW_LINK;return 0;}struct tw_node*n=node(c,at,2,depth);if(!n)return 0;if(!n->class_id)return 1;
  if(n->parent!=p->address){c->r->status=TW_LINK;c->r->failed_address=at;return 0;}uint32_t prev=n->prev_sibling;if(!descend(c,n,depth+1))return 0;at=prev;
 }return 1;
}
int tw_capture(const struct tw_ports*p,struct tw_report*r){
 if(!p||!r||!p->read||!p->links||!p->classes||!p->class_count||r->root_count>2)return TW_BADARG;
 r->abi=2;r->bytes=sizeof*r;r->status=0;r->node_count=r->button_count=r->unknown_count=r->truncated=r->reads=r->read_bytes=r->failed_address=0;
 for(unsigned i=0;i<sizeof r->nodes;i++)((uint8_t*)r->nodes)[i]=0;for(unsigned i=0;i<sizeof r->buttons;i++)((uint8_t*)r->buttons)[i]=0;
 if(!r->root_count)return r->status=TW_NO_ROOT;struct cursor c={p,r};
 for(unsigned ri=0;ri<r->root_count;ri++){
  if(!r->roots[ri])continue;if(seen(&c,r->roots[ri]))continue;uint32_t first=r->node_count;struct tw_node*root=node(&c,r->roots[ri],0,0);if(!root)return r->status;
  uint32_t parent=root->parent;for(unsigned depth=1;parent;depth++){
   if(depth>TW_ANCESTORS){r->truncated=1;return r->status=TW_LIMIT;}struct tw_node*existing=seen(&c,parent);if(existing){if((uint32_t)(existing-r->nodes)>=first)return r->status=TW_LINK;break;}
   struct tw_node*a=node(&c,parent,1,depth);if(!a)return r->status;parent=a->parent;
  }
  if(!descend(&c,root,1))return r->status;
 }return r->status;
}
