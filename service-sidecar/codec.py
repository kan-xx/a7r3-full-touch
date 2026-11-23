"""Pure128B F910 framing. No USB/file I/O or mutation on import."""
import struct,re
BUILD=0x3111fe07
REPORT_BYTES=26412
REPLY_BYTES=26476
OPS={1:'prepare_ui_readonly',2:'query',3:'install_passthrough_hook',4:'detach_restore_original',5:'stop_retired',6:'snapshot_current_menu_rows',7:'synthetic_focus_only',8:'fixed_empty_tp_marker',9:'choice_popup_only',10:'original_menu_back',11:'native_pas_leases_gate',12:'discover_resident_identity'}
META='version bytes build pid ui_tid lib_base obj_base manager phase callbacks submit_tid callback_tid callback_pid dispatch_flag return_matches callback_return safe_to_unload owns_reference attempted identity_valid bind_stage close_uncertain last_error callback_operation'.split()
RUNTIME_NAMES=['abi', 'bytes', 'state', 'installed', 'gate', 'inflight', 'pointer_uncertain', 'last_error', 'viewsystem', 'callback_cell', 'original_callback', 'hook_callback', 'pending_timer', 'retired', 'serial', 'fence_attempts', 'raw_callbacks', 'original_callbacks', 'bad_source', 'copied_blobs', 'armed', 'cancelled', 'committed', 'focus_count', 'centers_queued', 'center_failures', 'synthetic_count', 'last_focus_status', 'gui_count', 'gui_head', 'gui_tail', 'mouse_buttons', 'gui_token', 'cookie', 'eligible_flags', 'view', 'root', 'view_vptr', 'root_vptr', 'view_phase', 'root_mode', 'tab', 'page', 'row', 'row_count', 'root_runtime_id', 'resource_group_id', 'layout_id', 'header_focus', 'last_x', 'last_y', 'last_focus_row', 'timers_scheduled', 'detach_count', 'install_count', 'sticky_error', 'callback_readback', 'source_lr', 'source_tid', 'tap_phase', 'power_request', 'power_state', 'power_error', 'packed_provider_phases']
PATCH='abi bytes op phase result detail base page original_perm final_perm reads writes flush_calls protect_calls rollback_attempted rollback_completed uncertain'.split()
INSPECT='abi bytes status selected_count selected_node top_id top_obj active_count reads read_bytes selected_id sequence'.split()
NAV='version status prepared submitted event_id destination category target_view mode allocation_count release_count push_return caught_exception exception_unlock precheck_busy reserved'.split()
NODE='address vptr class_id role depth parent last_child prev_sibling mode flags runtime_widget_id on_mouse hook is_hit default_present default_mode aux_kind aux1 aux2 geometry_present'.split()
GEOMETRY='local_x local_y local_scale_x local_scale_y world_x world_y world_scale_x world_scale_y width height world_bound_left world_bound_top world_bound_width world_bound_height'.split()
def request(op,nonce,epoch,ui_tid,metadata,row=None,enable=None):
 boot,pid,start=epoch
 if op==12:raise ValueError('use discovery_request')
 if op not in OPS or len(nonce)!=16 or not any(nonce) or not re.fullmatch(r'[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}',boot):raise ValueError('op/nonce/boot')
 if not 1<pid<2**32 or not 1<ui_tid<2**32 or pid==ui_tid or not 0<start<2**64:raise ValueError('process epoch/UI')
 maj,minor,inode,base,uid,gid,mode=(metadata[k] for k in ('major','minor','inode','base','uid','gid','mode'))
 if (maj,minor)!=(252,15) or not 0<inode<2**64 or not 0<=base<=2**32-0xf2000 or base&4095 or not 0<=uid<2**32 or not 0<=gid<2**32 or not 0<=mode<=0o7777:raise ValueError('view4 file identity; base may be zero/unmapped')
 arg=0
 if op in (7,9):
  if not isinstance(row,int) or not 0<=row<6:raise ValueError('synthetic focus row0..5 required')
  arg=row
 elif row is not None:raise ValueError('row only accepted by op7/9')
 if op==11:
  if not isinstance(enable,bool):raise ValueError('op11 requires explicit enable bool')
  arg=int(enable)
 elif enable is not None:raise ValueError('enable only op11')
 body=b'A7TRQ007'+struct.pack('<II16s40sIIQIIQ6I',7,op,nonce,boot.encode()+bytes(4),pid,ui_tid,start,maj,minor,inode,base,uid,gid,mode,arg,0)
 assert len(body)==128
 return struct.pack('<HH',0,0xf910)+body

def reply(data,op,nonce):
 if len(data)!=REPLY_BYTES or data[:8]!=b'A7TRR007' or data[48:64]!=nonce:raise ValueError('reply framing')
 h=dict(zip('version op result flags pid tid entry report_bytes total_bytes build'.split(),struct.unpack_from('<10I',data,8)))
 if h['version']!=7 or h['op']!=op or h['flags']&~7 or h['report_bytes']!=REPORT_BYTES or h['total_bytes']!=REPLY_BYTES or h['build']!=BUILD:raise ValueError('reply identity')
 r=dict(zip(META,struct.unpack_from('<24I',data,64)));r['nonce']=data[160:176].hex()
 layout=list(struct.unpack_from('<27I',data,176));r['layout']=dict(abi=layout[0],bytes=layout[1],status=layout[2],resource_group_id=layout[3],layout_ids=layout[4:10],active_layout_count=layout[10],primary_view=layout[11],root_id_mask=layout[12],get_widget_calls=layout[13],returned_roots=layout[14:16],sequence=layout[16],actual_runtime_ids=layout[17:19],reserved=layout[19:])
 i=dict(zip(INSPECT,struct.unpack_from('<12I',data,284)));i['module']=data[332:396].split(b'\0')[0].decode('ascii');i['factory']=data[396:460].split(b'\0')[0].decode('ascii');r['inspection']=i
 r['navigation']=dict(zip(NAV,struct.unpack_from('<16I',data,460)))
 values=struct.unpack_from('<24I',data,524)
 t=dict(zip('abi bytes status sequence view_id view_obj view_vptr root0 root1 root_id0 root_id1 root_count node_count button_count unknown_count truncated reads read_bytes failed_address reserved0 reserved1 reserved2 reserved3 reserved4'.split(),values))
 if t['root_count']>2 or t['node_count']>128 or t['button_count']>24 or t['reads']>2048 or t['read_bytes']>262144:raise ValueError('tree bounds')
 t['requested_resource_ids']=[t['root_id0'],t['root_id1']];t['nodes']=[];t['buttons']=[]
 for ix in range(t['node_count']):
  at=620+152*ix;n=dict(zip(NODE,struct.unpack_from('<20I',data,at)));n['geometry_raw']=list(struct.unpack_from('<14I',data,at+80));n['geometry_cache']=dict(zip(GEOMETRY,struct.unpack_from('<14f',data,at+80)));n['margin']=list(struct.unpack_from('<4i',data,at+136));t['nodes'].append(n)
 for ix in range(t['button_count']):
  at=620+128*152+96*ix;v=struct.unpack_from('<24I',data,at);t['buttons'].append(dict(address=v[0],enabled_mask=v[1],event_ids=list(v[2:23]),reserved=v[23]))
 r['tree']=t
 words=list(struct.unpack_from('<1024I',data,22380));rt=dict(zip(RUNTIME_NAMES,words[:64]));rt['raw_words']=words;rt['diagnostics']=dict(zip('timer_lr timer_tid timer_dispatch_flag scheduled_cookie fence_now_ms fence_age_ms fence_result post_commit_serial candidate_serial commit_power_request center_event_id last_blob_copy_valid last_sample_state reserved source_pid markers_queued marker_failures'.split(),words[208:225]));rt['rows']=[]
 for index in range(6):
  a=64+index*24;v=words[a:a+24];rt['rows'].append(dict(index=index,row_address=v[0],row_vptr=v[1],button_address=v[2],button_vptr=v[3],row_mode=v[4],button_mode=v[5],runtime_widget_id=v[6],enabled_mask=v[7],world_bounds=dict(zip(['left','top','width','height'],struct.unpack('<4f',struct.pack('<4I',*v[8:12])))),eligible_geometry=bool(v[12]),hit=bool(v[13])))
 if rt['abi'] and (rt['abi']!=7 or rt['bytes']!=4096):raise ValueError('runtime ABI')
 identity_names='power view_id view root runtime_id kind layout phase tab page popup popup_node first'.split()
 rt['surface']=dict(zip(identity_names,words[256:269]));rt['surface'].update(status=words[269],target_count=words[270],row_count=words[271],tab_count=words[272],page_count=words[273])
 if words[270]>48:raise ValueError('target count')
 rt['surface']['targets']=[dict(zip('kind index node vptr runtime_id flags'.split(),words[274+i*6:280+i*6])) for i in range(words[270])]
 rt['native_pas_leases_enabled']=bool(words[240]);rt['current_snapshot_valid']=bool(words[239]);rt['pending_envelope']=dict(active=words[225],sequence=words[226],queued_count=words[237],received_count=words[235],last_receiver_result=words[236])
 rt['touch_diagnostic']=dict(zip('abi bytes status vm model_manager element model model_vptr initialized touch_disp touch_range user_device desired range forced'.split(),words[600:615]))
 for key in ['touch_disp','touch_range','user_device','range']:
  v=rt['touch_diagnostic'][key];rt['touch_diagnostic'][key]=v if v<2**31 else v-2**32
 rt['leases']=[dict(active=words[620+i*10],uncertain=words[621+i*10],identity=dict(zip('view widget runtime_id resource_id class_kind current_mode default_mode'.split(),words[622+i*10:629+i*10]))) for i in range(3)]
 rt['last_sync']=dict(zip('abi status constructed called returned retained allocations releases event_id view_id retained_event'.split(),words[740:751]))
 r['runtime']=rt
 if r['version'] and (r['version']!=7 or r['bytes']!=REPORT_BYTES or r['build']!=BUILD or r['nonce']!=nonce.hex() or r['callback_operation'] not in OPS):raise ValueError('core report identity')
 h['report']=r;return h

def callback_completed(d,epoch,ui_tid,operation,sequence=None):
 r=d['report']
 return d['result']==0 and d['pid']==r['pid']==r['callback_pid']==epoch[1] and not d['flags']&2 and r['ui_tid']==r['callback_tid']==ui_tid and r['phase']==2 and r['callback_operation']==operation and r['dispatch_flag']==r['return_matches']==r['safe_to_unload']==r['owns_reference']==r['identity_valid']==1 and not r['close_uncertain'] and r['last_error']==0 and (sequence is None or r['attempted']>>8==sequence)

DISCOVERY_BYTES=424
DISCOVERY_REPLY_BYTES=488
DISCOVERY_META='abi bytes build status origin flags core_base api_entry autoload_entry callbacks phase last_error'.split()
AUTOLOAD_REPORT='abi bytes stage result ui_tid adopted independent_ref close_attempted close_uncertain retryable fresh prepare_result exception_caught borrow_valid existing_accepted attempt_token'.split()
def discovery_request(epoch,ui_tid=0):
 boot,pid,start=epoch
 if not re.fullmatch(r'[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}',boot) or not 1<pid<2**32 or not 0<start<2**64 or not 0<=ui_tid<2**32 or ui_tid==pid:raise ValueError('discovery process probe')
 body=b'A7TRQ007'+struct.pack('<II16s40sIIQIIQ6I',7,12,bytes(16),boot.encode()+bytes(4),pid,ui_tid,start,0,0,0,0,0,0,0,0,0)
 assert len(body)==128
 return struct.pack('<HH',0,0xf910)+body

def _progress_file_phase(p):
 # Preserve every original index0..5 profile. Only new slot6 is admitted,
 # and only for the proved seven-file V3/complete/cancel/failure phases.
 i=p['file_index']
 return i<=5 or (i==6 and p['file_count']==7 and p['phase'] in (2,3,4,5,6,8))

def discovery_reply(data,epoch):
 if len(data)!=DISCOVERY_REPLY_BYTES or data[:8]!=b'A7TRD007' or any(data[48:64]):raise ValueError('discovery frame')
 h=dict(zip('version op result flags pid tid entry report_bytes total_bytes build'.split(),struct.unpack_from('<10I',data,8)))
 if h['version']!=7 or h['op']!=12 or h['flags']&~7 or h['flags']&1 or h['report_bytes']!=DISCOVERY_BYTES or h['total_bytes']!=DISCOVERY_REPLY_BYTES or h['build']!=BUILD or h['pid']!=epoch[1]:raise ValueError('discovery header')
 if not any(data[64:]):
  if not h['result']:raise ValueError('empty successful discovery')
  h['discovery']=None;return h
 d=dict(zip(DISCOVERY_META,struct.unpack_from('<12I',data,64)))
 if d['abi']!=7 or d['bytes']!=DISCOVERY_BYTES or d['build']!=BUILD or d['status']!=h['result'] or d['flags']&~63:raise ValueError('discovery identity')
 if h['result']:
  h['discovery']=d;return h
 values=struct.unpack_from('<II16s40sIIQIIQ6I',data,112)
 names=['version','op','nonce','boot','pid','ui_tid','start','major','minor','inode','base','uid','gid','mode','arg0','arg1']
 q=dict(zip(names,values));boot=q['boot']
 if q['version']!=7 or q['op']!=1 or not any(q['nonce']) or boot!=epoch[0].encode()+bytes(4) or q['pid']!=epoch[1] or q['start']!=epoch[2] or not 1<q['ui_tid']<2**32 or q['pid']==q['ui_tid'] or (q['major'],q['minor'])!=(252,15) or not q['inode'] or q['base']&4095 or q['mode']>0o7777 or q['arg0'] or q['arg1'] or d['origin'] not in (1,2) or not d['flags']&1 or bool(d['flags']&16)!=(d['origin']==2):raise ValueError('provider process/request identity')
 q['nonce']=q['nonce'].hex();q['boot']=boot[:36].decode('ascii');d['identity']=q
 for key in ['api_entry','autoload_entry']:
  if not d['core_base'] or d['core_base']&4095 or not 0<d[key]-d['core_base']<0x1000000:raise ValueError('module entry')
 path=data[232:328].split(b'\0')[0].decode('ascii')
 if path not in ('/usr/lib/liba7r_touch_full_runtime_v7.so','//usr/lib/liba7r_touch_full_runtime_v7.so','/tmp_bt/a7r-touch-full-v7/liba7r_touch_full_runtime_v7.so','//tmp_bt/a7r-touch-full-v7/liba7r_touch_full_runtime_v7.so'):raise ValueError('module path')
 d['path']=path;d['autostart']=dict(zip(AUTOLOAD_REPORT,struct.unpack_from('<16I',data,328)));d['bootstrap']=dict(zip('bound callbacks busy_count result'.split(),struct.unpack_from('<4I',data,392)))
 if d['autostart']['abi'] and (d['autostart']['abi']!=1 or d['autostart']['bytes']!=64):raise ValueError('autoload report ABI')
 if d['bootstrap']['busy_count']>8:raise ValueError('autoload retry bound')
 d['progress']=dict(zip('abi bytes active phase file_index file_count steps chunk_limit last_io_bytes total_file_bytes file_size file_done owner_qualified last_power_error result queued selfpin open_file worker_tid generation'.split(),struct.unpack_from('<20I',data,408)))
 if d['progress']['abi'] and (d['progress']['abi']!=1 or d['progress']['bytes']!=80 or d['progress']['chunk_limit']!=49152 or d['progress']['last_io_bytes']>49152 or not _progress_file_phase(d['progress']) or d['progress']['queued'] and d['progress']['active']):raise ValueError('preparation progress ABI/budget')
 h['discovery']=d;return h

def identity_arguments(d):
 """Return fresh typed control arguments; never a saved callable pointer."""
 q=d['discovery']['identity']
 return dict(nonce=bytes.fromhex(q['nonce']),epoch=(q['boot'],q['pid'],q['start']),ui_tid=q['ui_tid'],metadata={k:q[k] for k in ['major','minor','inode','base','uid','gid','mode']})
