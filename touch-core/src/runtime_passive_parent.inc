/* Read-only candidate parent facts. Distinct magic from native getWidget proof.
 * No return value, native methods, allocation, queue mutation or eligibility. */
static void rt_pb_passive_parent(const struct p14_diag*d){
 if(rt_pb.capture_has_nonzero||!d||!d->word[10]||d->word[11]!=s.r.obj_base+0x11ed7a0)return;
 uint32_t child=d->word[10],user[2]={0},header[10]={0},parent_header[10]={0};
 if(child&3||child>UINT32_MAX-0x164||!rt_read(child,header,40)||header[0]!=s.r.obj_base+0x11ed7a0||!rt_read(child+0x15c,user,8))return;
 uint32_t record[8]={0x41375052,user[0],0,user[0],user[1],UINT32_MAX,UINT32_MAX,UINT32_MAX};
 uint32_t parent=user[0];
 if(parent&&!(parent&3)&&parent<=UINT32_MAX-0x5a4&&rt_read(parent,parent_header,40)){
  record[2]=parent_header[0];
  if(parent_header[0]==s.r.lib_base+0x8bf418&&child==parent+0x1c0&&(header[4]&~3u)==parent+4&&parent_header[7]==parent){
   uint32_t values[3];if(rt_read(parent+0x590,values,8)){record[5]=values[0];record[6]=values[1];}if(rt_read(parent+0x5a0,values+2,4))record[7]=values[2];
  }
 }
 __builtin_memcpy(&RW(732),record,sizeof record);
}
