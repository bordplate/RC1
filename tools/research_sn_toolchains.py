"""Research-only SN toolchain matrix; see notes/sn_toolchain_assemblers.md.

Requires extracted decomp.me archives and the symbolic-address baseline under
--out. Does not modify production sources, toolchain, or match status.
"""
import sys, os, json, hashlib, subprocess, re, argparse
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT/'tools'))
from research_symbolic_addresses import CASES, function_bytes
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', required=True, type=Path)
parser.add_argument('--builds', nargs='+', default=['273a','274','107','114','136'])
args=parser.parse_args()
BASE=args.out.resolve()
env=dict(os.environ,WINEPREFIX=str(ROOT/'tools/wineprefix'),WINEDEBUG='-all')
wine=str(ROOT/'tools/wine/bin/wine')
def run(cmd, log):
    p=subprocess.run(list(map(str,cmd)),cwd=ROOT,env=env,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
    text=p.stdout.decode(errors='replace')
    with log.open('a') as f: f.write(json.dumps(list(map(str,cmd)))+'\n'+text+'\n')
    return p.returncode,text
builds={'273a':BASE/'273a','274':BASE/'ee-gcc2.95.2-274','107':BASE/'ee-gcc2.95.3-107','114':BASE/'114','136':BASE/'136'}
results=[]; fingerprints=[]
for name,folder in builds.items():
    if name not in args.builds: continue
    lib=next((folder/'lib/gcc-lib/ee').iterdir())
    log=folder/'test.log'; log.write_text('')
    for exe in [folder/'bin/ee-gcc.exe',lib/'cc1plus.exe',*lib.glob('*as.exe'),*lib.glob('*As.exe')]:
        rc,text=run([wine,exe, '-v' if exe.name=='cc1plus.exe' else '--version'] if exe.name.lower()!='ps2eeas.exe' else [wine,exe],log)
        fingerprints.append(dict(build=name,path=str(exe.relative_to(BASE)),sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),version=text))
    for case,ref,symbol,defs in CASES:
        baseline=BASE/'baseline'/case
        expected=bytes.fromhex(''.join(re.findall(r'/\*\s*[0-9a-fA-F]+\s+[0-9a-fA-F]+\s+([0-9a-fA-F]{8})\s*\*/',(baseline/'reference.s').read_text().split('endlabel',1)[0])))
        link=json.loads((baseline/'candidate.log').read_text().splitlines()[-1])
        for mode in ['native','snas','fixed-gas','fixed-snas', 'borrowed-snas']:
            if mode=='borrowed-snas' and name not in ('274','107'): continue
            out=folder/(case+'-'+mode); obj=out.with_suffix('.o'); elf=out.with_suffix('.elf')
            row=dict(build=name,case=case,mode=mode)
            obj.unlink(missing_ok=True)
            elf.unlink(missing_ok=True)
            if mode.startswith('fixed'):
                aspath=lib/'as.exe' if mode=='fixed-gas' else next(iter(list(lib.glob('*eeas.exe'))+list(lib.glob('*EeAs.exe'))),None)
                if aspath is None:
                    row['error']='no SN assembler in archive'; results.append(row); continue
                flags=['-EL','-m5900','-G8'] if mode=='fixed-gas' else ['-G8']
                cmd=[wine,aspath,*flags,'-o',obj,baseline/'candidate.s']
            else:
                cmd=[wine,folder/'bin/ee-gcc.exe','-B'+str(lib)+'/', '-c','-G8','-O2','-ffast-math','-fno-exceptions','-Icode/include','-Itools/cc/lib/gcc-lib/ee/2.95.2/include',*( ['-snas'] if mode=='snas' else ['-Wa,-EL']),ROOT/'decomp_state/probes/symbolic_addresses'/(case+'.cpp'),'-o',obj]
                if mode=='borrowed-snas':
                    asm=out.with_suffix('.s')
                    cmd=[str(asm) if x==obj else '-S' if x=='-c' else x for x in cmd]
                    rc,text=run(cmd,log)
                    if rc:
                        row['error']=text; results.append(row); continue
                    cmd=[wine,BASE/'273a/lib/gcc-lib/ee/2.95.2/ps2eeas.exe','-G8','-o',obj,asm]
            rc,text=run(cmd,log)
            if rc or not obj.exists(): row['error']=text
            else:
                lc=[str(obj) if x==str(baseline/'candidate.o') else str(elf) if x==str(baseline/'candidate.elf') else x for x in link]
                rc,text=run(lc,log)
                if rc: row['error']=text
                else:
                    try:
                        actual=function_bytes(elf,symbol)
                        row.update(size=len(actual),reference_size=len(expected),match=actual==expected,diff_words=[i//4 for i in range(0,max(len(actual),len(expected)),4) if actual[i:i+4]!=expected[i:i+4]])
                    except Exception as e: row['error']=str(e)
            results.append(row)
            print(name,case,mode, 'ERROR '+row['error'][:160] if 'error' in row else (row['size'],row['match']),flush=True)
suffix='-'+'-'.join(args.builds)
(BASE/('fingerprints'+suffix+'.json')).write_text(json.dumps(fingerprints,indent=2))
(BASE/('results'+suffix+'.json')).write_text(json.dumps(results,indent=2))
