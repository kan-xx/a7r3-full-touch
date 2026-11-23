"""Compare same-boot V17 read-only reports saved before and after a real test."""
from pathlib import Path
import json
import struct
import sys

NAMES='epoch maps inspect catalog power pre stock post fence receive apply lease hit publish fast native_wheel_return'.split()
def decode(before,after):
    def words(p):
        b=Path(p).read_bytes()
        if len(b)==26476:b=b[-4096:]
        if len(b)!=4096:raise ValueError('Runtime4096 or a full V17 op2 reply required')
        w=struct.unpack('<1024I',b)
        if w[:2]!=(7,4096) or w[668:670]!=(0x41375042,2):raise ValueError('V17 runtime and playback layout required')
        return w
    a,b=map(words,(before,after))
    layout=Path(__file__).resolve().parent/'telemetry-pinch-layout.json'
    groups=json.loads(layout.read_text())['groups']
    def val(w,group):
        g=groups[group];i=g['start'];return dict(zip(g['fields'],w[i:i+len(g['fields'])]))
    perf={}
    for i,name in enumerate(NAMES):
        off=840+i*3
        if b[off]<a[off]:raise ValueError('Counter decreased; verify same boot, V17 core and installation')
        perf[name]=dict(count_delta=b[off]-a[off],total_count=b[off],last_us=b[off+1],max_since_install_us=b[off+2])
    return dict(perf=perf,playback_before=val(a,'pb'),playback_after=val(b,'pb'),gui_before=val(a,'pb_gui_fence'),
                gui_after=val(b,'pb_gui_fence'),limits=['Establish same boot, core hash and nonce independently before comparing.',
                'Timing groups overlap. Last and maximum are not duration averages. Native wheel return excludes downstream image rendering.',
                'Counter movement does not establish physical latency or visible zoom.'])
if __name__=='__main__':
    if len(sys.argv)!=3:raise SystemExit('Usage: decode_v17_performance.py BEFORE AFTER')
    print(json.dumps(decode(*sys.argv[1:]),indent=2))
