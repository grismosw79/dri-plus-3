"""Test34 keeps all Test33 capture/header corruption checks and exact guard inventory."""
from pathlib import Path
exec(compile((Path(__file__).parent/'test33_compat_test.py').read_text(),__file__,'exec'))
baseline33=Path(os.environ['DRI_TEST33_BASELINE_DIR'])/'Runtime/Portal/Source'
old33=[]
for file in ['signatures.h','retention.c']:
 for a,n,b in re.findall(r'\{(0x[\da-f]+),(\d+),\{([^}]+)\}',(baseline33/file).read_text()):
  old33.append((int(a,16),bytes(int(v.strip(),0)for v in b.split(','))[:int(n)],file))
assert set(old33)==set(guards) and len(old33)==276, 'Test34 must preserve all delivered Test33 guard bytes'
result['all_test33_guards_preserved']=True
(OUTPUT/'test34_compat_results.json').write_text(json.dumps(result,indent=2)+'\n')
