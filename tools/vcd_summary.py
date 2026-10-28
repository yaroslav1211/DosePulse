#!/usr/bin/env python3
"""Summarize recorded digital edges; never synthesize a capture."""
import argparse,json,statistics
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('capture');p.add_argument('--out');a=p.parse_args()
names={};changes={};timestamp=0
for line in Path(a.capture).read_text().splitlines():
    bits=line.split()
    if line.startswith('$var'):names[bits[3]]=bits[4];changes[bits[3]]=[]
    elif line.startswith('#'):timestamp=int(line[1:])
    elif line and line[0] in '01' and line[1:] in names:changes[line[1:]].append((timestamp,int(line[0])))
result={'file':a.capture,'duration_ns':timestamp,'channels':{}}
for code,name in names.items():
    rising=[t for t,v in changes[code] if v];intervals=[(b-a)/1000 for a,b in zip(rising,rising[1:])]
    result['channels'][name]={'transitions':len(changes[code]),'rising':len(rising),'interval_us':{'min':min(intervals),'mean':statistics.mean(intervals),'max':max(intervals)} if intervals else None}
text=json.dumps(result,indent=2);print(text)
if a.out:Path(a.out).write_text(text+'\n')
