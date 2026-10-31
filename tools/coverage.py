#!/usr/bin/env python3
import subprocess,sys,json,re,shutil
from pathlib import Path
build=Path('build-host/coverage');build.mkdir(parents=True,exist_ok=True)
out=Path('docs/evidence/lab2/coverage');out.mkdir(parents=True,exist_ok=True)
cc='cc';common=['-std=c11','-Wall','-Wextra','-Werror','-Ifirmware/Devices','-Ifirmware/App','--coverage','-O0']
sources=['firmware/Devices/dp_sensors.c','firmware/Devices/dp_sd.c','firmware/App/dp_quality.c']
for p in build.glob('*.gcda'):p.unlink()
for source in sources:
    subprocess.run([cc,*common,'-c',source,'-o',str(build/(Path(source).stem+'.o'))],check=True)
for name,objects in [('sensors',['dp_sensors','dp_quality']),('sd',['dp_sd'])]:
    binary=build/('test-'+name)
    subprocess.run([cc,*common,'tests/test_'+name+'.c',*[str(build/(o+'.o')) for o in objects],'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
gcov=['xcrun','llvm-cov','gcov'] if sys.platform=='darwin' else ['gcov']
results={}
for source in sources:
    log=subprocess.check_output([*gcov,'-b','-c','-o',str(build),source],text=True);(out/(Path(source).stem+'-summary.txt')).write_text(log)
    p=Path(Path(source).name+'.gcov');text=p.read_text();shutil.move(str(p),out/p.name)
    executable=covered=0
    for line in text.splitlines():
        match=re.match(r'\s*([\d#=*-]+):\s*(\d+):',line)
        if match and match[1]!='-':
            executable+=1
            if match[1].rstrip('*').isdigit() and int(match[1].rstrip('*'))>0:covered+=1
    branches=[int(v) for v in re.findall(r'^branch\s+\d+ taken (\d+)',text,re.M)]
    lp=100*covered/executable if executable else 0;bp=100*sum(v>0 for v in branches)/len(branches) if branches else 100
    results[source]={'lines':executable,'lines_covered':covered,'line_percent':lp,'branches':len(branches),'branches_covered':sum(v>0 for v in branches),'branch_percent':bp}
    print(source,f'lines={lp:.2f}% branches={bp:.2f}%')
(out/'metrics.json').write_text(json.dumps(results,indent=2)+'\n')
assert all(r['line_percent']>=80 and r['branch_percent']>=80 for r in results.values()),'Coverage below declared 80% per module'
