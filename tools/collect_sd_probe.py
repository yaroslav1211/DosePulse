#!/usr/bin/env python3
"""Capture actual SD probe UART, waiting for startup and both block results."""
import serial,time,json,hashlib
from pathlib import Path
out=Path('docs/evidence/lab2/sd-probe');raw=bytearray();pending=bytearray();commands=[];uart=None
for attempt in range(12):
    try:uart=serial.serial_for_url('rfc2217://127.0.0.1:4192',baudrate=115200,timeout=.2);break
    except serial.SerialException:time.sleep(1)
if uart is None:raise RuntimeError('No Wokwi UART')
with uart:
    end=time.monotonic()+30;sent=False;write_ok=False;read_sent=False;read_ok=False
    while time.monotonic()<end:
        data=uart.read(4096);raw.extend(data);pending.extend(data)
        while b'\n' in pending:
            line,_,remaining=pending.partition(b'\n');pending=bytearray(remaining);text=line.decode(errors='replace');print(text,flush=True)
            if text.startswith('DATA') and not sent:uart.write(b'sdprobe\n');commands.append('sdprobe');sent=True
            if 'SD sector=4096 write=1 result=0' in text:write_ok=True
            if write_ok and not read_sent:uart.write(b'sdread\n');commands.append('sdread');read_sent=True
            if 'SD sector=4096 write=0 result=0' in text:read_ok=True
        if read_ok:break
out.with_suffix('.raw').write_bytes(raw)
out.with_suffix('.json').write_text(json.dumps({'commands':commands,'write_ok':write_ok,'read_ok':read_ok,'ELF_sha256':hashlib.sha256(Path('firmware/build/firmware.elf').read_bytes()).hexdigest()},indent=2)+'\n')
assert write_ok and read_ok,'Probe did not complete; raw evidence retained'
