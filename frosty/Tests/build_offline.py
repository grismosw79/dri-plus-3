"""Compile release against the verified RC3 API contract, then run offline checks.
For a Windows build against actual Frosty assemblies, use ../BUILD.ps1 instead.
Reference fixtures are never installed or included in the public Plugins folder.
"""
import argparse, json, subprocess
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--dotnet', required=True)
p.add_argument('--references', required=True, help='Framework 4.6.2 reference directory')
p.add_argument('--output', required=True)
a = p.parse_args()
dotnet = Path(a.dotnet).resolve()
sdk = sorted((dotnet.parent/'sdk').iterdir())[-1]
refs = Path(a.references).resolve()
out = Path(a.output).resolve(); out.mkdir(parents=True, exist_ok=True)
src = Path(__file__).resolve().parents[1]
root = src.parent
base = [str(dotnet), str(sdk/'Roslyn/bincore/csc.dll'), '/nologo', '/noconfig',
        '/nostdlib+', '/optimize+', '/deterministic+', '/debug-', '/langversion:6', '/platform:x64']
base += ['/reference:'+str(refs/n) for n in ('mscorlib.dll','System.dll','System.Core.dll')]
def compile(name, sources, dependencies=(), resources=False):
    cmd=base+['/target:'+('library' if name.endswith('.dll') else 'exe'), '/out:'+str(out/name)]
    cmd += ['/reference:'+str(out/n) for n in dependencies]
    if resources:
        cmd += ['/resource:'+str(root/'Runtime'/n)+',DRIPlus.'+n for n in ('DRIPlus3-RM18R22.dll','DRIPlus3-RM18R22.exe')]
    subprocess.run(cmd+[str(src/n) for n in sources],check=True)
    if name.endswith('.exe'):
        (out/Path(name).with_suffix('.runtimeconfig.json')).write_text(json.dumps({
            'runtimeOptions':{'tfm':'net8.0','framework':{'name':'Microsoft.NETCore.App','version':'8.0.0'}}}))
compile('FrostySdk.dll',['Tests/ReferenceSdk.cs'])
compile('FrostyCore.dll',['Tests/ReferenceCore.cs'],['FrostySdk.dll'])
deps=['FrostySdk.dll','FrostyCore.dll']
compile('DRIPlus3.dll',['DRIPlus3.cs','LaunchWatcher.cs'],deps,True)
compile('StartupChecks.exe',['DRIPlus3.cs','LaunchWatcher.cs','Tests/StartupChecks.cs'],deps,True)
compile('ReleaseChecks.exe',['Tests/ReleaseChecks.cs'],deps)
for exe,argv in [('StartupChecks.exe',[]),('ReleaseChecks.exe',[str(out/'DRIPlus3.dll'),str(out/'fixture'),str(root/'Runtime')])]:
    result=subprocess.run([str(dotnet),str(out/exe)]+argv,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    (out/(Path(exe).stem+'.txt')).write_text(result.stdout)
    print(result.stdout,end='')
    result.check_returncode()
