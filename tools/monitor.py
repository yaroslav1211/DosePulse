#!/usr/bin/env python3
"""Capture the actual Wokwi RFC2217 stream, preserving received bytes."""
import argparse
import serial
import time
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--seconds',type=float,default=8);p.add_argument('--output',default='docs/evidence/uart.raw');p.add_argument('--command',default='');a=p.parse_args()
with serial.serial_for_url('rfc2217://127.0.0.1:4192',baudrate=115200,timeout=.2) as uart:
    if a.command:uart.write((a.command+'\n').encode())
    end=time.monotonic()+a.seconds;raw=bytearray()
    while time.monotonic()<end:raw.extend(uart.read(4096))
Path(a.output).parent.mkdir(parents=True,exist_ok=True);Path(a.output).write_bytes(raw)
print(raw.decode(errors='replace'),end='')
