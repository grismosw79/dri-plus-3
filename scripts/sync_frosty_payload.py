"""Update the Frosty source pins after a local native build. Does not install."""
from pathlib import Path
import hashlib,json,re,struct

root=Path(__file__).resolve().parents[1]
hashes={}
for extension in ("dll","exe"):
    path=root/"Runtime"/("DRIPlus3-RM18R22."+extension)
    data=path.read_bytes()
    pe=struct.unpack_from("<I",data,0x3c)[0]
    if data[pe:pe+4]!=b"PE\0\0" or struct.unpack_from("<H",data,pe+4)[0]!=0x8664:
        raise SystemExit("Expected a Windows x64 PE: "+str(path))
    hashes[extension]=hashlib.sha256(data).hexdigest()

paths=[root/"frosty/DRIPlus3.cs",root/"frosty/BUILD.ps1"]
updates={}
for path in paths:
    text=path.read_text()
    for extension,label in [("dll","Runtime"),("exe","Launcher")]:
        pattern=(r'(internal const string '+label+r'Hash = ")[0-9a-f]{64}(";)'
                 if path.suffix==".cs" else
                 r"('DRIPlus3-RM18R22\."+extension+r"' = ')[0-9a-f]{64}(')")
        text,count=re.subn(pattern,lambda m:m[1]+hashes[extension]+m[2],text)
        if count!=1:raise SystemExit("Unexpected payload hash field: "+str(path))
    updates[path]=text
for path,text in updates.items():path.write_bytes(text.replace("\n","\r\n").encode())
print(json.dumps({"status":"source pins updated","payloads":hashes}))
