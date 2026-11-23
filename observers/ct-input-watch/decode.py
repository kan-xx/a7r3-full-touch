from pathlib import Path
import json,struct,sys,hashlib
REASONS={1:'FIRST',2:'CHANGE',3:'HEARTBEAT',4:'ONCE_END',5:'TIME_END',6:'FULL_END',7:'IDENTITY_END',8:'MAPS_END',9:'SIGTERM_END',10:'CLOCK_END'}
def decode(path):
 p=Path(path);b=p.read_bytes()
 if len(b)<96 or (len(b)-96)%192 or len(b)>393312:raise ValueError('incomplete or oversized binary')
 h=struct.unpack_from('<12I',b)
 if h[:7]!=(0x31575443,1,96,192,2048,100,1200000):raise ValueError('wire header mismatch')
 boot=b[48:88].split(b'\0')[0].decode('ascii')
 records=[]
 for at in range(96,len(b),192):
  w=struct.unpack_from('<48I',b,at);s=w[5:];reason=REASONS.get(w[3])
  if not reason or w[0]!=len(records)+1 or w[4]>1:raise ValueError('record sequence/reason/stability mismatch')
  cb=s[14:24];key=s[24:41];known=s[0]
  records.append(dict(seq=w[0],elapsed_ms=w[1],poll=w[2],reason=reason,stable_double_read=bool(w[4]),known=known,error=s[1],
   vm=s[2],vs=s[3],actual_callback=s[4],infra_tp=s[5],infra_tp_vptr=s[6],tp_flags_raw=s[7],
   tp_flags_known=bool(known&8),suppression_u8=s[7]&255,event_sw_u8=(s[7]>>8)&255,extra_raw_u8=(s[7]>>16)&255,
   infra_input=s[8],infra_input_vptr=s[9],infra_input_plus20_raw=s[10],view_status=s[11],tri_dial_bit9=(s[11]>>9)&1,tp_filter_raw=s[12],
   callback_record=s[13],callback_compact_words=list(cb),callback_next=cb[0],callback_owner=cb[1],callback_plus11_u8=(cb[2]>>8)&255,
   callback_id=cb[3],callback_function=cb[5],key_count=cb[8],key_pointer=cb[9],
   materialized_key_known=bool(known&128),materialized_key_68B_hex=struct.pack('<17I',*key).hex(),
   pre_queue_callback=s[41],category1_callback=s[42]))
 if not records or records[-1]['reason']not in set(REASONS.values())-{'FIRST','CHANGE','HEARTBEAT'}:raise ValueError('no terminal footer; watcher may still run or file was cut')
 return dict(abi=1,sha256=hashlib.sha256(b).hexdigest(),bytes=len(b),pid=h[7],start=h[8]|(h[9]<<32),obj_base=h[10],view2_base=h[11],boot=boot,record_count=len(records),terminal=records[-1]['reason'],records=records,
  limits=['stable means two adjacent observations agreed; it is not a lock or atomic snapshot.',
          'Unknown flags, zero-initialized incomplete fields and historical records do not authorize an action.',
          'This records existing native caches and gates, not the raw1B/17 source stream.'])
if __name__=='__main__':
 if len(sys.argv)!=2:raise SystemExit('usage: decode.py BINARY')
 print(json.dumps(decode(sys.argv[1]),indent=2))
