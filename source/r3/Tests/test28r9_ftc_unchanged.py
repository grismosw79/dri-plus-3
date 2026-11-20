from pathlib import Path
import sys,hashlib,json
old,new=map(Path,sys.argv[1:]);out=[]
for i in range(5):
 rel=f'frantkdata/{i}/dynasty-tuning-binary.FTC';a=(old/rel).read_bytes();b=(new/rel).read_bytes();assert a==b
 out.append({'route':i,'sha256':hashlib.sha256(b).hexdigest(),'unchanged':True})
assert (old/'Prepared-Save/DYNASTY-TEST28R8START').read_bytes()==(new/'Prepared-Save/DYNASTY-TEST28R8START').read_bytes()
print(json.dumps(out,indent=2))
