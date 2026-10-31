#!/usr/bin/env python3
import argparse,bisect,json,statistics
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('capture');p.add_argument('--output',required=True);a=p.parse_args()
names={};events=[];channels={};t=0
for line in Path(a.capture).read_text().splitlines():
    if line.startswith('$var'):
        fields=line.split();names[fields[3]]=fields[4];channels[fields[4]]=[]
    elif line.startswith('#'):t=int(line[1:])
    elif line and line[0] in '01' and line[1:] in names:
        name=names[line[1:]];v=int(line[0]);events.append((t,name,v));channels[name].append((t,v))
# Use the same 2MHz time grid as sigrok. Resolve simultaneous pin changes
# together so GPIO mode transitions do not become false START conditions.
state={name:0 for name in channels};packets=[];frame=None;groups_by_time={}
for t,name,value in events:groups_by_time.setdefault((t//500)*500,{})[name]=value
for t,updates in groups_by_time.items():
    new=dict(state);new.update(updates)
    if state['D0']==new['D0']==1 and state['D1']!=new['D1']:
        if new['D1']==0:
            if frame:frame['end_ns']=t;frame['end']='RESTART';packets.append(frame)
            frame={'start_ns':t,'bytes':[],'bits':[],'clock_rises_ns':[]}
        elif frame:
            frame['end_ns']=t;frame['end']='STOP';packets.append(frame);frame=None
    if new['D0']==1 and state['D0']==0 and frame:
        frame['clock_rises_ns'].append(t);frame['bits'].append(new['D1'])
        if len(frame['bits'])==9:
            bits=frame['bits'];byte=0
            for bit in bits[:8]:byte=(byte<<1)|bit
            frame['bytes'].append({'value':byte,'hex':f'{byte:02x}','bits':''.join(map(str,bits[:8])),'ack':bits[8]==0});frame['bits']=[]
    state=new
valid=[f for f in packets if f['bytes']]
for f in valid:f['duration_us']=(f['end_ns']-f['start_ns'])/1000
read_frames=[f for f in valid if f['bytes'][0]['value']==0xd1 and len(f['bytes'])==15]
periods=[]
for f in valid:
    times=f['clock_rises_ns'];periods.extend((b-a)/1000 for a,b in zip(times,times[1:]) if b-a<1000000)
adc=channels.get('D4',[]);widths=[];rises=[];previous=None
for t,v in adc:
    if v:previous=t;rises.append(t)
    elif previous is not None:widths.append((t-previous)/1000);previous=None
hxclk=[t for t,v in channels.get('D2',[]) if v];groups=[]
for t in hxclk:
    if not groups or t-groups[-1][-1]>1000000:groups.append([])
    groups[-1].append(t)
hxdata=channels.get('D3',[]);data_times=[t for t,v in hxdata]
def hx_value(t):
    i=bisect.bisect_right(data_times,t)-1;return hxdata[i][1] if i>=0 else 0
words=[]
for group in groups:
    if len(group)==25:
        bits=[hx_value(t+1000) for t in group[:24]];value=0
        for b in bits:value=value*2+b
        words.append({'start_ns':group[0],'clocks':25,'bits':''.join(map(str,bits)),'raw':value if value<0x800000 else value-0x1000000,'duration_us':(group[-1]-group[0])/1000})
def stats(values):return {'n':len(values),'min':min(values),'mean':statistics.mean(values),'max':max(values)} if values else None
result={'capture':a.capture,'i2c_frame_count':len(valid),'i2c_first_frames':valid[:10],'i2c_nack_addresses':[f for f in valid if not f['bytes'][0]['ack']][:4],'i2c_clock_period_us':stats(periods),'i2c_14byte_read_us':stats([f['duration_us'] for f in read_frames]),'adc_marker_width_us':stats(widths),'adc_marker_period_us':stats([(b-a)/1000 for a,b in zip(rises,rises[1:])]),'hx_full_words':len(words),'hx_examples':words[:5]}
Path(a.output).write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['i2c_first_frames','i2c_nack_addresses']},indent=2))
