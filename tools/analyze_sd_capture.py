#!/usr/bin/env python3
"""Decode actual SPI mode0 samples, with SD block and read-back assertions."""
import argparse,json,statistics,hashlib
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('capture');p.add_argument('--output',required=True);a=p.parse_args()
names={};groups={};timestamp=0
for line in Path(a.capture).read_text().splitlines():
    if line.startswith('$var'):fields=line.split();names[fields[3]]=fields[4]
    elif line.startswith('#'):timestamp=int(line[1:])
    elif line and line[0] in '01' and line[1:] in names:groups.setdefault((timestamp//500)*500,{})[names[line[1:]]]=int(line[0])
state={name:0 for name in names.values()};frames=[];frame=None
for t,updates in groups.items():
    new=dict(state);new.update(updates)
    if state['D7']==1 and new['D7']==0:frame={'start_ns':t,'mosi':[],'miso':[],'rises':[],'bits_out':[],'bits_in':[]}
    if frame and new['D7']==0 and state['D6']==0 and new['D6']==1:
        frame['rises'].append(t);frame['bits_out'].append(new['D0']);frame['bits_in'].append(new['D1'])
        if len(frame['bits_out'])==8:
            for bits,key in [('bits_out','mosi'),('bits_in','miso')]:
                value=0
                for bit in frame[bits]:value=value*2+bit
                frame[key].append(value);frame[bits]=[]
    if frame and state['D7']==0 and new['D7']==1:frame['end_ns']=t;frames.append(frame);frame=None
    state=new
summaries=[];periods=[];writes=[];reads=[]
expected=bytes((i*37+19)&255 for i in range(512))
for f in frames:
    for x,y in zip(f['rises'],f['rises'][1:]):
        if y-x<1000000:periods.append((y-x)/1000)
    mosi=f['mosi'];miso=f['miso'];indices=[i for i,v in enumerate(mosi) if 0x40<=v<0x80]
    if not indices:continue
    i=indices[0];cmd=mosi[i]&0x3f
    if i+5>=len(mosi):continue
    arg=int.from_bytes(bytes(mosi[i+1:i+5]),'big');summary={'command':cmd,'arg':arg,'command_bytes':mosi[i:i+6],'start_ns':f['start_ns'],'duration_us':(f['end_ns']-f['start_ns'])/1000,'bytes':len(mosi)}
    if cmd==24:
        token=mosi.index(0xfe,i+6);data=bytes(mosi[token+1:token+513]);assert len(data)==512 and data==expected
        summary['write_payload_sha256']=hashlib.sha256(data).hexdigest();summary['data_response_bytes']=miso[token+515:token+519];writes.append(summary)
    if cmd==17:
        token=miso.index(0xfe,i+6);data=bytes(miso[token+1:token+513]);assert len(data)==512
        summary['read_payload_sha256']=hashlib.sha256(data).hexdigest();summary['all_zero']=data==bytes(512);summary['matches_pattern']=data==expected;reads.append(summary)
    summaries.append(summary)
assert writes and any(r['all_zero'] for r in reads) and any(r['matches_pattern'] for r in reads),'Required SD write/read data absent'
result={'capture':a.capture,'mode':0,'channels':{'clock':'D6','CS':'D7','MOSI':'D0','MISO':'D1'},'commands':summaries,'clock_period_us':{'n':len(periods),'min':min(periods),'mean':statistics.mean(periods),'max':max(periods)},'write_payload_verified':True,'zero_before_and_pattern_after':True}
Path(a.output).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
