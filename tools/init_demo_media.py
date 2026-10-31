#!/usr/bin/env python3
"""Create only the explicitly named disposable SD image, preserving existing data."""
from pathlib import Path
p=Path('media/demo-sd.img');p.parent.mkdir(exist_ok=True)
if not p.exists():
    with p.open('wb') as stream:stream.truncate(8*1024*1024)
    print('Created disposable 8MiB',p)
else:print('Preserved existing image',p)
