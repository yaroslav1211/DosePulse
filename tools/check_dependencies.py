#!/usr/bin/env python3
from pathlib import Path
import re
for directory in ['firmware/App','firmware/Devices']:
    for p in Path(directory).glob('*.[ch]'):
        assert not re.search(r'#\s*include\s*[<"](?:main|stm32|dp_bench)',p.read_text()),f'Forbidden platform dependency: {p}'
print('PASS App/Devices do not include HAL, Core main or Platform')
