#include "surface_actions.h"
int surface_identity_equal(const struct surface_identity*a,const struct surface_identity*b){return a&&b&&a->power==b->power&&a->view_id==b->view_id&&a->view==b->view&&a->root==b->root&&a->runtime_id==b->runtime_id&&a->kind==b->kind&&a->layout==b->layout&&a->phase==b->phase&&a->tab==b->tab&&a->page==b->page&&a->popup==b->popup&&a->popup_node==b->popup_node&&a->first==b->first;}
static int valid(const struct surface_ports*p,const struct surface_identity*s){return p&&s&&p->validate&&s->power&&s->view&&s->root&&p->validate(p->context,s);}
int surface_apply(const struct surface_ports*p,const struct surface_identity*s,const struct surface_target*t,int activate){
 if(!valid(p,s)||!t||activate<0||activate>1)return SURFACE_REJECT;
 if(t->kind==TARGET_BACK){if(!activate||s->view_id!=0xaa||(s->kind!=SURFACE_MENU&&s->kind!=SURFACE_POPUP)||!p->submit)return SURFACE_REJECT;return p->submit(p->context,0xaa,1)?SURFACE_DISPATCHED:SURFACE_REJECT;}
 if(s->kind==SURFACE_MENU&&s->view_id==0xaa&&s->phase==1){
  if(t->kind==TARGET_ROW){if(!p->focus_row||!p->focus_row(p->context,s,t->index)||!valid(p,s))return SURFACE_REJECT;if(!activate)return SURFACE_FOCUSED;return p->submit&&p->submit(p->context,0xaa,0)?SURFACE_DISPATCHED:SURFACE_REJECT;}
  if(t->kind==TARGET_TAB||t->kind==TARGET_PAGE){if(!p->select_path||!p->refresh||!p->tab_count||!p->page_count)return SURFACE_REJECT;int tabs=p->tab_count(p->context,s->root);int tab=t->kind==TARGET_TAB?(int)t->index+1:s->tab;if(tabs<1||tabs>7||tab<1||tab>tabs)return SURFACE_REJECT;int pages=p->page_count(p->context,s->root,tab);int page=t->kind==TARGET_TAB?1:(int)t->index+1;if(pages<0||pages>15||page<1||(t->kind==TARGET_PAGE&&page>pages))return SURFACE_REJECT;
   /* A target category may be an empty MyMenu. Native row=-1 selection owns
    * its empty-node semantics; geometry/header/back remain independent of rows.
    * Port must prove native selector handles its empty-category page1 contract. */
   if(!p->select_path(p->context,s->view,tab,page,-1)||!p->refresh(p->context,s->view))return SURFACE_REJECT;return SURFACE_FOCUSED;
  }
  if(t->kind==TARGET_PAGE_PREV||t->kind==TARGET_PAGE_NEXT){if(!p->direction||!p->direction(p->context,s->view,t->kind==TARGET_PAGE_PREV?2:3))return SURFACE_REJECT;return SURFACE_FOCUSED;}
 }
 if(s->kind==SURFACE_POPUP&&s->view_id==0xaa&&s->phase==2&&t->kind==TARGET_POPUP_ROW){
  if(!p->popup_count||!p->popup_first||!p->popup_position||!p->popup_style||!p->popup_move)return SURFACE_REJECT;int visible=p->popup_count(p->context,s->popup,1),total=p->popup_count(p->context,s->popup,0),first=p->popup_first(p->context,s->popup);if(visible<1||visible>6||first<0||(uint32_t)first!=s->first||t->index>=(uint32_t)visible||first+(int)t->index>=total)return SURFACE_REJECT;int target=first+t->index;int style=p->popup_style(p->context,s->popup,target);if(style<0||style==3||style==4)return SURFACE_REJECT;
  for(unsigned i=0;i<6;i++){if(!valid(p,s)||p->popup_first(p->context,s->popup)!=first)return SURFACE_REJECT;int h=p->popup_position(p->context,s->popup);if(h<0||h>=visible)return SURFACE_REJECT;int current=first+h;if(current==target){if(!activate)return SURFACE_FOCUSED;return p->submit&&p->submit(p->context,0xaa,0)?SURFACE_DISPATCHED:SURFACE_REJECT;}if(!p->popup_move(p->context,s->root,target>current))return SURFACE_REJECT;int next=p->popup_position(p->context,s->popup);if(next==h||next<0||next>=visible||(target>current?next<h:next>h))return SURFACE_REJECT;}
  return SURFACE_REJECT;
 }
 if(s->kind==SURFACE_FN&&s->view_id==0x44&&s->layout==2&&s->phase==1&&t->kind==TARGET_FN_CELL){if(t->index>=12||!p->fn_focus||!p->fn_focus(p->context,s,t->index)||!valid(p,s))return SURFACE_REJECT;if(!activate)return SURFACE_FOCUSED;return p->submit&&p->submit(p->context,0x44,0)?SURFACE_DISPATCHED:SURFACE_REJECT;}
 return SURFACE_REJECT;
}
