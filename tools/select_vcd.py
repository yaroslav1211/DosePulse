#!/usr/bin/env python3
"""Extract a time interval from a real VCD; retain source times and provenance."""
import argparse,json,hashlib
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('source');p.add_argument('output');p.add_argument('--end-ms',type=float,default=250);a=p.parse_args()
source=Path(a.source);lines=source.read_text().splitlines();selected=[]
for line in lines:
    if line.startswith('#') and int(line[1:])>a.end_ms*1000000:break
    selected.append(line)
Path(a.output).write_text('\n'.join(selected)+'\n')
Path(a.output+'.provenance.json').write_text(json.dumps({'source':a.source,'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'end_ns':int(a.end_ms*1000000),'transformation':'Prefix selection only, unchanged digital samples and original timestamps.'},indent=2)+'\n')
