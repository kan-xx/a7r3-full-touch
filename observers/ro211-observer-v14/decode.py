#!/usr/bin/env python3
"""Decode only the bounded A7RO14B1 diagnostic log; never grants action authority."""
import json, struct, sys, zlib
from pathlib import Path
FILE=524288; REC=2048; BODY=964
U=lambda b,o,n=1:struct.unpack_from('<'+'I'*n,b,o)
def widget(b):
 w=U(b,0,40)
 return dict(address=w[0],ok=w[1],typed=w[2],header=list(w[3:13]),parent_node=w[13],parent_parent=w[14],parent_widget=w[15],owner_valid=w[16],owner=w[17],event_valid=w[18],event_id=list(w[19:40]),event_enable=list(b[160:181]),type=b[184:248].split(b'\0',1)[0].decode('ascii','replace'))
def body(b):
 w=U(b,0,36); e=U(b,144,72); stream=b[208:412]
 return dict(status=w[0],flags=w[1],vm=w[2],mm=w[3],top_id=w[4],top_view=w[5],top_index=w[6],view_vptr=w[7],view_phase=w[8],ws=w[9],scope_generation=w[10],mouse=list(w[12:17]),gui=list(w[17:21]),model=dict(zip(('id','status','node','element','model','vptr','depth'),w[21:28])),model_element_header=list(w[28:36]),e9=dict(typed=e[0],complete=e[1],offset_to_top=e[2],primary_vptr=e[3],stream_vptr=e[4],head=list(e[5:14]),common_ref=list(e[14:16]),stream_words=list(e[16:67]),stream_hex=stream.hex(),raw_type=U(stream,0x68)[0],original_width=U(stream,0x78)[0],original_height=U(stream,0x7c)[0],file_size=U(stream,0xa0)[0],directory_hex=stream[0xb0:0xba].hex(),filename_hex=stream[0xba:0xc8].hex(),directory=stream[0xb0:0xba].split(b'\0',1)[0].decode('ascii','replace'),filename=stream[0xba:0xc8].split(b'\0',1)[0].decode('ascii','replace'),tail=list(e[67:71]),still_status=e[71],busy=list(b[432:434]),cmd3_valid=b[434],still=list(U(b,436,8)),raw_hex=b[144:468].hex()),hit=widget(b[468:716]),capture=widget(b[716:964]))
def decode(data):
 if len(data)!=FILE or data[:8]!=b'A7RO14B1':raise ValueError('file size/magic')
 if U(data,2044)[0]!=zlib.crc32(data[:2044]):raise ValueError('header CRC; stop/close observer before final copy')
 w=U(data,8,32)
 if w[:4]!=(1,FILE,REC,BODY) or w[24:28]!=(6,192,198,58):raise ValueError('layout')
 header=dict(abi=w[0],bytes=w[1],record_bytes=w[2],body_bytes=w[3],pid=w[4],start=w[5]+(w[6]<<32),observer_pid=w[7],sequence=w[8],samples=w[9],stored=w[10],active_writes=w[11],post_writes=w[12],first211=w[13],first_ready211=w[14],first_return175=w[15],first_capture211=w[16],last_capture211=w[17],metadata_checks=w[18],reason=w[19],started_ms=w[20]+(w[21]<<32),last_header_ms=w[22]+(w[23]<<32),header_serial=w[28],sample_ms=w[29],metadata_ms=w[30],max_ms=w[31],boot=data[136:173].split(b'\0',1)[0].decode('ascii'),bases=list(U(data,176,3)))
 records=[];bad=[];fixed={1:'first211',2:'first_ready211',3:'first_return175',4:'first_capture211',5:'last_capture211'}
 for slot in range(1,256):
  raw=data[slot*REC:(slot+1)*REC]
  if not any(raw):continue
  if U(raw,2044)[0]!=zlib.crc32(raw[:2044]) or U(raw,0,2)!=(0x31524f52,1):bad.append(slot);continue
  m=U(raw,0,16);a=raw[64:64+BODY];b=raw[64+BODY:64+2*BODY]
  if bool(m[7]&1)!=(a==b):raise ValueError('stable bit does not describe both raw payloads')
  records.append(dict(slot=slot,fixed=fixed.get(slot),sequence=m[2],monotonic_ms=m[3]+(m[4]<<32),duration_ms=m[6],flags=m[7],stable=bool(m[7]&1),e9_full_equal=a[144:468]==b[144:468],capture_observed_while_top211=bool(m[7]&16),metadata_qualified=bool(m[7]&2),status_a=m[8],status_b=m[9],reads=m[10],bytes=m[11],read_failures=m[12],metadata_checks=m[13],metadata_age_ms=m[14],a=body(a),b=body(b)))
 records.sort(key=lambda r:(r['sequence'],r['slot']))
 return dict(header=header,records=records,invalid_or_inflight_slots=bad,no_action_authority=True,source='O_RDONLY process observations; adjacent pair is not atomic; preserve both payloads even when unstable')
if __name__=='__main__':
 result=decode(Path(sys.argv[1]).read_bytes());text=json.dumps(result,ensure_ascii=False,indent=2)+'\n'
 if len(sys.argv)>2:Path(sys.argv[2]).write_text(text)
 else:print(text,end='')
