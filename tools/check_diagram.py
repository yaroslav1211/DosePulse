#!/usr/bin/env python3
import json
from pathlib import Path
d=json.loads(Path('diagram.json').read_text());parts={p['id'] for p in d['parts']}
for connection in d['connections']:
    for endpoint in connection[:2]:
        part,_,pin=endpoint.partition(':')
        assert part.startswith('$') or part in parts,endpoint
        if part=='nucleo':assert pin not in {'3V3','5V','GND'},f'Rail needs explicit board pin suffix: {endpoint}'
required=[('scale:DT','nucleo:PA1'),('scale:SCK','nucleo:PA8'),('imu:SCL','nucleo:PB8'),('imu:SDA','nucleo:PB9'),('target:SIG','nucleo:PA0')]
nets=[set(c[:2]) for c in d['connections']]
for pair in required:assert set(pair) in nets,pair
print('PASS DosePulse diagram: component IDs, explicit supply rails, sensor pinout')
