#!/usr/bin/env python3
"""Short sensor bring-up scenario, scheduled on MCU DATA timestamps."""
import serial,time,re,json,hashlib,datetime,argparse
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--output',default='docs/evidence/lab2/integration');a=p.parse_args();out=Path(a.output);out.parent.mkdir(parents=True,exist_ok=True)
schedule=[(4000,'selftest'),(5000,'anomaly'),(6000,'tare'),(6500,'fault'),(9000,'recover'),(18000,'sdprobe'),(21000,'invalid'),(21500,'x'*80),(24000,'sdread'),(29000,'counts')]
raw=bytearray();pending=bytearray();sent=[];started=time.monotonic();last_t=0;finished=False;base=None
uart=None
for attempt in range(12):
    try:
        uart=serial.serial_for_url('rfc2217://127.0.0.1:4192',baudrate=115200,timeout=.2);break
    except serial.SerialException as error:
        print('RFC2217 retry',attempt,str(error),flush=True);time.sleep(1)
if uart is None:raise RuntimeError('Wokwi UART unavailable after bounded retries')
with uart:
    uart.write(b'status\n')
    while time.monotonic()-started<150:
        block=uart.read(4096);raw.extend(block);pending.extend(block)
        while b'\n' in pending:
            line,_,rest=pending.partition(b'\n');pending=bytearray(rest);text=line.decode(errors='replace');print(text,flush=True)
            match=re.search(r'DATA t=(\d+)',text)
            if match:
                last_t=int(match[1])
                if base is None:base=last_t
                elapsed=last_t-base
                while schedule and elapsed>=schedule[0][0]:
                    threshold,command=schedule.pop(0);uart.write((command+'\n').encode());sent.append({'after_DATA_t_ms':last_t,'threshold':threshold,'command':command})
                if elapsed>=32000:finished=True;break
        if finished:break
out.with_suffix('.raw').write_bytes(raw)
meta={'actual_time_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'last_mcu_ms':last_t,'finished':finished,'commands':sent,'firmware_sha256':hashlib.sha256(Path('firmware/build/firmware.elf').read_bytes()).hexdigest(),'host_elapsed_s':time.monotonic()-started}
out.with_suffix('.json').write_text(json.dumps(meta,indent=2)+'\n')
assert finished,'No complete short MCU interval; preserve failed run'
text=raw.decode(errors='replace')
for expected in ['SELF device=MPU6050 addr=68 result=0','ANOMALY addr=69 reg=75 result=1','ACK fault','ACK recover','ERR command','ERR length','SD sector=4096 write=1 result=0','SD sector=4096 write=0 result=0']:
    assert expected in text,expected
last_data=[line for line in text.splitlines() if line.startswith('DATA t=')][-1]
for field in ['hx_ok=1','imu=0','stable=1']:assert field in last_data,(field,last_data)
print('PASS short sensor integration, actual I2C recovery, final sensor validity, command errors, actual SD read/write')
