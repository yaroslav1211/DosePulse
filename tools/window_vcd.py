#!/usr/bin/env python3
"""Select real VCD transitions; record the explicit shift to a local time origin."""
import argparse
import hashlib
import json
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument('source');p.add_argument('output')
p.add_argument('--start-ms',type=float,required=True)
p.add_argument('--end-ms',type=float,required=True)
a=p.parse_args();source=Path(a.source)
start=int(round(a.start_ms*1_000_000));end=int(round(a.end_ms*1_000_000))
if not 0<=start<end:p.error('Expected 0 <= start < end')
header=[];state={};events=[];time_ns=0;in_header=True
for line in source.read_text().splitlines():
    if in_header:
        header.append(line)
        if line.startswith('$enddefinitions'):in_header=False
    elif line.startswith('#'):time_ns=int(line[1:])
    elif line and line[0] in '01':
        if time_ns<start:state[line[1:]]=line[0]
        elif time_ns<=end:events.append((time_ns,line))
out=header+['#0']+[v+k for k,v in state.items()]
out += [item for t,line in events for item in ('#'+str(t-start),line)]
Path(a.output).write_text('\n'.join(out)+'\n')
Path(a.output+'.provenance.json').write_text(json.dumps({
    'source':a.source,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
    'original_start_ns':start,'original_end_ns':end,'selected_transitions':len(events),
    'transformation':'Exact transition window; carry-forward initial state; displayed time = original time minus original_start_ns. Raw source retained.'
},indent=2)+'\n')
