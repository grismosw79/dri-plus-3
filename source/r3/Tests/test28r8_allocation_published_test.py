"""Published soft recruit in positive native candidate vector cannot consume a pair."""
import os
os.environ.setdefault('DRI_TEST_INSTRUCTION_LIMIT','50000000')
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
from collections import Counter
configure(1);call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
records[season]['scalar'].update({2:2040,4:8,5:1})
for x in [departures,roster,hslist,tplist,committed,arr]:records[x]['array']=[]
for pos,e in enumerate(ideal_entries):
 records[e]['scalar'].update({2:ideals[pos],4:1})
 count=max(0,ideals[pos]-(2 if pos==3 else 0))
 records[roster]['array'] += [pp('returner '+str(pos)+' '+str(j),65,pos=pos)for j in range(count)]
acquired,at,ap=pr(63911,80,year=0,pos=3,stage=4,portal=False)
primary,pt,ppp=pr(63912,75,year=0,pos=3,stage=0,portal=False)
backup,bt,bpp=pr(63913,74,year=0,pos=3,stage=0,portal=False)
# Three-star controls isolate the common allocation gate from elite-market seeding.
for p in [ap,ppp,bpp]:records[p]['scalar'][231]=2
valid_offer(acquired,at);records[committed]['array']=[ap]
ctx=alloc(0xc00);uc.mem_write(ctx,bytes(uc.mem_read(context,0x300)))
for pos in range(22):wi(ctx+0x9a4+4*pos,1)
af=STACK+0x23000;asp=af+128;abp=asp+0x200
wq(af+10*8,abp);wq(af+2*8,ctx);wq(af+3*8,team_ref)
wq(abp+0x28,team_ref);wq(abp+0x58,season);wq(abp+0x180,need);wq(asp+0x58,ctx);uc.mem_write(asp+0x42,b'\0')
call(hooks[8],af);call(start,af);assert call(exports['DriTestQualityFault'])==0
entries=[]
for r,value in [(acquired,900),(primary,500),(backup,400)]:
 b=bytearray(48);struct.pack_into('<Q',b,0,r);struct.pack_into('<i',b,40,value);entries.append(bytes(b))
vec=alloc(24);mem=alloc(240);main=mem+48;canary=b'\xa5'*48
uc.mem_write(mem,canary+b''.join(entries)+canary);wq(vec,main);wq(vec+8,main+144);wq(vec+16,main+144)
wq(abp-0x68,vec);wq(asp+0x68,3);wi(abp+0x30,50);wq(asp+0x50,1);wq(asp+0x78,0);wq(abp-0x80,0)
players_before={p:dict(records[p]['scalar'])for p in [ap,ppp,bpp]};recruit_before=dict(records[acquired]['scalar'])
call(hooks[15],af)
assert call(exports['DriTestQualityFault'])==0
order=[rq(main+i*48)for i in range(3)]
assert order==[primary,backup,acquired],('acquired player took active slot',order)
assert Counter(bytes(uc.mem_read(main+i*48,48))for i in range(3))==Counter(entries)
assert bytes(uc.mem_read(mem,48))==canary and bytes(uc.mem_read(main+144,48))==canary
assert records[arr]['array']==[at] and records[committed]['array']==[ap]
assert records[acquired]['scalar']==recruit_before and all(records[p]['scalar']==v for p,v in players_before.items())
# Allocation permits only the two live alternatives. Native secured ownership
# checks may also reject the acquired player; this is not fabricated removal.
for r,expected in [(primary,0),(backup,0),(acquired,1)]:
 entry=alloc(48);wq(entry,r);wq(af+11*8,entry)
 assert call(admit,af)==expected,('admission',r,expected)
call(exports['DriTestOwnershipReset']);call(exports['DriTestHsPlanReset'])
assert all(v==1 for v in refs.values())
checks=['Positive-score acquired SoftCommitted native candidate stays unselected; lower-scored live primary and backup receive the only opening','Every native candidate byte and vector canary preserved as an exact permutation; acquiring board, incoming class, recruit state and players unchanged','Actual admission callback accepts only live selected alternatives; references balanced and no runtime fault']
(OUTPUT/'test28r8_allocation_published_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n');print(json.dumps(checks,indent=2))
