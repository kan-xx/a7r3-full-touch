struct result {uint32_t id,status,node,element,model,vptr,depth;uint32_t element_header[8];};

static int lookup(uint32_t mm,uint32_t id,struct result*out){
 memset(out,0,sizeof*out);out->id=id;uint32_t root=0;if(!word(mm+0x90,&root))return 0;uint32_t end=mm+0x8c,parent=end,seen[64];unsigned used=0;
 while(root){if(root==end||used==64||(root&3))return 0;for(unsigned j=0;j<used;j++)if(seen[j]==root)return 0;seen[used++]=root;uint32_t n[6];if(!rd(root,n,sizeof n)||n[1]!=parent)return 0;int32_t key=(int32_t)n[4];out->depth=used;if(key==(int32_t)id){out->node=root;out->element=n[5];if(!out->element){out->status=1;return 1;}if(!rd(out->element,out->element_header,sizeof out->element_header))return 0;out->model=out->element_header[7];if(!out->model){out->status=2;return 1;}if(!word(out->model,&out->vptr))return 0;if(!mapped(out->vptr,4))return 0;out->status=3;return 1;}parent=root;root=key<(int32_t)id?n[3]:n[2];}
 return 1;
}
