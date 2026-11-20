"""Real HS prepare/quota/allocation callbacks; native candidate scores preserved."""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
configure(1);call(exports['DriTestCommitReset']);records[season]['scalar'].update({2:2040,4:8,5:1})
for a in [departures,roster,hslist,tplist,committed,arr]:records[a]['array']=[]
for pos,(e,v)in enumerate(zip(ideal_entries,ideals)):
 records[e]['scalar'].update({2:v,4:1})
 count=max(0,v-(1 if pos in [3,7,13]else 0))
 records[roster]['array'] += [pp('returner '+str(pos),65,pos=pos)for j in range(count)]
# One uncovered WR, C and LOLB. Native fit puts all WR candidates first.
pairs=[]
for i,(pos,value)in enumerate([(3,900),(3,850),(3,800),(7,600),(7,500),(13,400),(13,300),(0,1000)]):
 r,t,p=pr(46000+i,65,pos=pos,portal=False,stage=0);records[p]['scalar'][231]=1;pairs.append((r,t,p,value))
af=STACK+0x23000;asp=af+128;abp=asp+0x200
wq(af+10*8,abp);wq(af+2*8,context);wq(af+3*8,team_ref)
wq(abp+0x28,team_ref);wq(abp+0x58,season);wq(abp+0x180,need);wq(asp+0x58,context);uc.mem_write(asp+0x42,b'\0')
# The actual evaluation structure also holds per-position native quotas.
# Make the fixture context sufficiently large for its +9a4 quota fields.
ctx=alloc(0xc00);uc.mem_write(ctx,bytes(uc.mem_read(context,0x300)))
for pos in range(22):wi(ctx+0x9a4+4*pos,1)
wq(af+2*8,ctx);wq(asp+0x58,ctx)
call(hooks[8],af);assert call(exports['DriTestQualityFault'])==0
qf=STACK+0x26000;qb=qf+0x200;wq(qf+10*8,qb);wq(qb+0x38,team_ref);wq(qb+0x30,ctx);wq(qf+7*8,7);wq(qf+13*8,2)
call(hooks[10],qf);assert rq(qf+13*8)==12000
wq(qf+7*8,0);wq(qf+13*8,2);call(hooks[10],qf);assert rq(qf+13*8)==2
call(start,af);assert call(exports['DriTestQualityFault'])==0
n=len(pairs);vec=alloc(24);main=alloc(n*48);wq(vec,main);wq(vec+8,main+n*48);wq(vec+16,main+n*48)
wq(abp-0x68,vec);wq(asp+0x68,n);wi(abp+0x30,50);wq(asp+0x50,0)
for i,(r,t,p,v)in enumerate(pairs):wq(main+i*48,r);wi(main+i*48+40,v)
original={r:bytes(uc.mem_read(main+i*48,48))for i,(r,t,p,v)in enumerate(pairs)}
call(hooks[15],af)
assert call(exports['DriTestQualityFault'])==0
order=[rq(main+i*48)for i in range(n)]
assert order[:3]==[pairs[i][0]for i in [0,3,5]],order
assert order[3:7]==[pairs[i][0]for i in [7,1,4,6]],order
assert len(set(order))==n and all(bytes(uc.mem_read(main+i*48,48))==original[r]for i,r in enumerate(order))
assert not records[arr]['array'] # Reordering is not a fabricated offer or addition.
checks=['Vacant C and LOLB receive first-pass candidate coverage alongside WR before extra WR alternatives',
 'Vacancy shortlist expanded before native collection; full QB room quota unchanged',
 'Exact candidate identity/score/component permutation; no board membership, offer, budget or roster mutation']
# Already covered C gets no new primary before truly uncovered roles.
records[arr]['array']=[pairs[3][1]];records[pairs[3][0]]['scalar'][9]=0
call(hooks[8],af);call(start,af)
for i,(r,t,p,v)in enumerate(pairs):uc.mem_write(main+i*48,original[r])
call(hooks[15],af);order=[rq(main+i*48)for i in range(n)]
assert order[:3]==[pairs[i][0]for i in [0,3,5]],order
checks.append('Existing C pursuit occupies its original primary; all uncovered positions precede backups')
# Same native HS board pass during the portal calendar. Protected existing
# entries leave the replacement vector with exactly one native release.
records[season]['scalar'].update({4:3,5:6})
call(hooks[8],af);call(start,af)
for i,(r,t,p,v)in enumerate(pairs):uc.mem_write(main+i*48,original[r])
rem=alloc(48);uc.mem_write(rem,original[pairs[3][0]]);incref(pairs[3][0])
wq(asp+0x78,rem);wq(abp-0x80,rem+48);wq(asp+0x50,1)
call(hooks[15],af)
assert rq(abp-0x80)==rem and refs[pairs[3][0]]==1
assert [rq(main+i*48)for i in range(3)]==[pairs[i][0]for i in [0,3,5]]
checks.append('Offseason HS planning preserves the pair and releases exactly one protected replacement-vector reference')

# Regression: keep every physical native record exactly once, including
# selected existing entries whose score is nonpositive and alias entries.
from collections import Counter
negative=os.environ.get('DRI_R6_NEGATIVE')=='1'
regressions=[]
for label,score,alias,include_existing in [
 ('existing_zero',0,False,True),('existing_negative',-25,False,True),
 ('positive_with_zero_alias',600,True,True),('absent_from_main',600,False,False),
 ('all_nonpositive',0,False,True)]:
 records[season]['scalar'].update({4:1,5:1})
 records[arr]['array']=[pairs[3][1]]
 call(hooks[8],af);call(start,af)
 entries=[original[r] for i,(r,t,p,v)in enumerate(pairs) if include_existing or i!=3]
 if include_existing:
  entry=bytearray(entries[3]);struct.pack_into('<i',entry,40,score);entries[3]=bytes(entry)
 if alias:
  entry=bytearray(entries[3]);struct.pack_into('<i',entry,40,0);struct.pack_into('<i',entry,8,9876);entries.append(bytes(entry))
 if label=='all_nonpositive':
  entries=[e[:40]+struct.pack('<i',0)+e[44:] for e in entries]
 nn=len(entries);mem=alloc((nn+2)*48);canary=b'\xa5'*48;uc.mem_write(mem,canary);main=mem+48
 uc.mem_write(main,b''.join(entries)+canary)
 wq(vec,main);wq(vec+8,main+nn*48);wq(vec+16,main+nn*48);wq(asp+0x68,nn)
 # Retained existing C is protected in replacement vector even with zero score.
 rmem=alloc(48);uc.mem_write(rmem,original[pairs[3][0]]);wi(rmem+40,0);incref(pairs[3][0])
 wq(asp+0x78,rmem);wq(abp-0x80,rmem+48);wq(asp+0x50,1)
 before=bytes(uc.mem_read(main,nn*48));before_refs=dict(refs)
 call(hooks[15],af);fault=call(exports['DriTestQualityFault'])
 if negative:
  assert label=='existing_zero' and fault==1
  assert bytes(uc.mem_read(main,nn*48))==before
  assert rq(abp-0x80)==rmem+48 and refs==before_refs
  print('R6 NEGATIVE CONTROL: selected existing zero-score record triggers guard before native writes')
  break
 assert fault==0,(label,'unexpected fault')
 after=[bytes(uc.mem_read(main+i*48,48))for i in range(nn)]
 assert Counter(after)==Counter(entries),(label,'not an exact physical-entry permutation')
 assert bytes(uc.mem_read(mem,48))==canary and bytes(uc.mem_read(main+nn*48,48))==canary
 assert rq(abp-0x80)==rmem and refs[pairs[3][0]]==1
 assert records[arr]['array']==[pairs[3][1]],'No fabricated board/offer changes'
 # Unscored entries remain in native relative order in the tail.
 def score_of(e):return struct.unpack_from('<i',e,40)[0]
 assert [e for e in after if score_of(e)<=0]==[e for e in entries if score_of(e)<=0]
 # Re-entry does not release a second reference or reshuffle a completed pass.
 snapshot=bytes(uc.mem_read(main,nn*48));call(hooks[15],af)
 assert bytes(uc.mem_read(main,nn*48))==snapshot and refs[pairs[3][0]]==1
 regressions.append(label)
if not negative:
 # Truly duplicated positive candidate entries still fail the existing guard.
 call(hooks[8],af);call(start,af)
 nn=2;main=alloc(96);uc.mem_write(main,original[pairs[0][0]]*2)
 wq(vec,main);wq(vec+8,main+96);wq(vec+16,main+96);wq(asp+0x68,nn)
 wq(asp+0x78,0);wq(abp-0x80,0);wq(asp+0x50,1)
 before=bytes(uc.mem_read(main,96));call(hooks[15],af)
 assert call(exports['DriTestQualityFault'])==1 and bytes(uc.mem_read(main,96))==before
 regressions.append('positive_duplicate_still_rejected_before_write')
 # Run this same input against a freshly compiled frozen R6 source.
 import subprocess,shutil
 control=Path(os.environ['DRI_TEST28R6_CONTROL_DLL']);out=OUTPUT/'r6_negative';out.mkdir(exist_ok=True)
 shutil.copy2(control,out/'retention_fixture.dll')
 result=subprocess.run([sys.executable,__file__],env=dict(os.environ,DRI_R6_NEGATIVE='1',DRI_TEST_OUTPUT_DIR=str(out)),capture_output=True,text=True)
 (OUTPUT/'r6_negative.log').write_text(result.stdout+result.stderr)
 assert result.returncode==0,result.stdout[-1000:]+result.stderr
 assert 'R6 NEGATIVE CONTROL:' in result.stdout
 call(exports['DriTestOwnershipReset'])
 assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
 (OUTPUT/'test28r7_permutation_results.json').write_text(json.dumps({'checks':regressions,'r6_negative_control_reproduced':True,'r6_control_dll_sha256':__import__('hashlib').sha256(control.read_bytes()).hexdigest(),'native_record_bytes_and_reference_counts_preserved':True,'live_game_verified':False},indent=2)+'\n')
 print(json.dumps(regressions))
