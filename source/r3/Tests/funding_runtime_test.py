"""Compiled funding callback; native vector/accounting layout, fixture storage.
The complete native interaction/action engine is not executed here.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'portal_runtime_test.py').read_text(),'portal_runtime_test.py','exec'))
# The shared fixture already maps and bounds a 64 MiB arena.
# Reuse it; a second mapping would overlap the existing heap.
funding_fixture_heap_capacity=0x4000000
configure(1)
wq(context+0xd0,tuning) # native finalist lockout reads the evaluator tuning reference
records[season]['scalar'].update({2:2040,4:3,5:6})
records[team]['scalar'][384]=9
for a in [departures,roster,hslist,tplist,committed,arr]:records[a]['array']=[]
for pos in range(22):
 records[ideal_entries[pos]]['scalar'].update({2:3,4:1,0:0})
 records[roster]['array'] += [pp('returner',95-k,pos=pos) for k in range(2)]
pairs=[]
for pos in range(22):
 for k in range(3):pairs.append(pr(40000+pos*3+k,75-k,pos=pos,stage=1 if k%2==0 else 0))
hs,hst,hsp=pr(41000,75,pos=0,portal=False)
valid_offer(hs,hst) # An actual native funding entry always has a live board target.
allpairs=pairs[:10]+[(hs,hst,hsp)]+pairs[10:]
n=len(allpairs);vec=alloc(n*72);idx=alloc(n*8)
for j,(rec,target,player) in enumerate(allpairs):
 wq(vec+j*72,rec);wi(vec+j*72+40,1000-j);wq(vec+j*72+48,target);wi(vec+j*72+64,records[rec]['scalar'][9]);wq(idx+j*8,n-j-1)
frame=STACK+0x17000;bp=frame+0x500
def prepare_funding(frame):
 # The 67-candidate, 138-school stress case invokes the same full-market
 # preparation as other R8 callers. Keep its legacy 8M cap for old DLLs,
 # the bounded R8 50M cap, and explicit caller limits authoritative.
 limit=int(os.environ.get('DRI_TEST_INSTRUCTION_LIMIT','50000000' if (b'DRI_PORTAL_TEST28R8' in image or (b'DRI_PORTAL_TEST28R9' in image or (b'DRI_PORTAL_TEST28R10' in image or (b'DRI_PORTAL_TEST28R11' in image or (b'DRI_PORTAL_TEST28R12' in image or (modern_r14 or b'DRI_PORTAL_TEST28R13' in image)))))) else '8000000'))
 rsp=STACK+0xf0000-8;wq(rsp,STOP)
 uc.reg_write(UC_X86_REG_RSP,rsp);uc.reg_write(UC_X86_REG_RCX,frame)
 uc.emu_start(hooks[8],STOP+1,count=limit)
 assert reg('RIP')==STOP,('preparation did not return',limit,hex(reg('RIP')))
wq(frame+9*8,context);wq(frame+10*8,bp);wq(bp-0x60,team_ref)
wq(bp-0x50,vec);wq(bp-0x48,vec+n*72);wq(bp+0x58,idx);wq(bp+0x60,idx+n*8)
callback=hooks[23]
before=bytes(uc.mem_read(vec,n*72));beforeidx=bytes(uc.mem_read(idx,n*8));original={rec:bytes(uc.mem_read(vec+j*72,72)) for j,(rec,_,_) in enumerate(allpairs)}
def totals():
 out=[0,0]
 for j in range(n):out[int(1<=ri(vec+j*72+64)<=5)]+=ri(vec+j*72+40)
 return out
# Without exact score-hook provenance the callback leaves every native byte
# and iteration index alone; it must not guess a base or disable gameplay.
old=totals();call(callback,frame)
assert bytes(uc.mem_read(vec,n*72))==before and bytes(uc.mem_read(idx,n*8))==beforeidx
assert call(exports['DriTestQualityFault'])==0
# Exercise the real score hook for every participating synthetic candidate.
wq(frame+2*8,context);wq(frame+3*8,team_ref);uc.mem_write(frame+128+0x42,b'\1')
prepare_funding(frame)
scoreframe=STACK+0x25000;holder=alloc(8)
wq(scoreframe,holder);wq(scoreframe+128+0xa8,team_ref);wq(scoreframe+128+0x88,GAME+0x081733fa)
for j,(rec,target,player) in enumerate(allpairs):
 if rec==hs:continue
 route=0x0818c282 if j%2==0 else 0x081733fa
 wq(scoreframe+128+0x88,GAME+route)
 wq(holder,rec);wq(scoreframe+8,1000-j);wq(scoreframe+24,0);call(hooks[9],scoreframe)
 # New-candidate route temporarily encodes the shortlist role; the shared
 # funding ledger must retain the exact restored total from either route.
 # A vacant third RB/receiver/front/CB slot is now useful rotation. Its
 # incumbent-free quality bonus remains zero in this strong-room fixture.
 pos=records[player]['scalar'][3]
 tier=2000000 if pos in [1,3,4,10,11,12,16] else 1000000
 assert rq(scoreframe+8)==1000-j+(tier if route==0x0818c282 else 0),(pos,j,rq(scoreframe+8))
# Simulate dispatching funding on another OS thread: every TLS slot is absent.
# The evaluator-scoped exact score ledger must still drive funding.
saved_tls=dict(tls);tls.clear()
call(callback,frame)
tls.clear();tls.update(saved_tls)
assert call(exports['DriTestQualityFault'])==0
assert totals()==old
assert bytes(uc.mem_read(vec+10*72,72))==original[hs]
assert sorted(rq(idx+j*8) for j in range(n))==list(range(n))
assert rq(idx+(n-1-10)*8)==10 # HS keeps its native first-pass ordering slot.
portal=[j for j in range(n) if j!=10]
first=[rq(vec+j*72) for j in portal[:22]]
assert len({records[records[r]['ref'][5]]['scalar'][3] for r in first})==22
weights={rq(vec+j*72):ri(vec+j*72+40) for j in range(n)}
assert all(weights[rec]>0 for rec,_,_ in pairs)
for pos in range(22):
 primary,alternative,third=pairs[pos*3:pos*3+3]
 assert weights[primary[0]]>weights[third[0]] # same stage budget: primary outranks redundant option.
for j in range(n):
 rec=rq(vec+j*72);current=bytearray(uc.mem_read(vec+j*72,72));current[40:44]=original[rec][40:44]
 assert bytes(current)==original[rec]
checks=['All 22 positions are covered before alternatives in both native iteration orders',
 'Missing score provenance preserves every native vector/index byte without a runtime fault',
 'Funding remains active after a simulated thread switch with every TLS slot empty',
 'Both native score routes publish the exact original total before shortlist encoding',
 'Exact native score sum conserved separately in Top10 and Top5-through-Hard budgets',
 'Every eligible portal target has positive weight; primaries outrank extra alternatives',
 'HS candidate bytes/order and every pointer, component, stage, target identity remain unchanged']
# A matching recruit/total cannot authorize a different team or evaluator.
other_team=record('same-shaped different team',dict(records[team]['scalar']),ref=dict(records[team]['ref']))
other_ref=alloc(8);wq(other_ref,other_team)
other_context=alloc(0x300);uc.mem_write(other_context,bytes(uc.mem_read(context,0x300)))
for probe_team,probe_eval in [(other_ref,context),(team_ref,other_context)]:
 uc.mem_write(vec,before);uc.mem_write(idx,beforeidx)
 wq(bp-0x60,probe_team);wq(frame+9*8,probe_eval);call(callback,frame)
 assert bytes(uc.mem_read(vec,n*72))==before and bytes(uc.mem_read(idx,n*8))==beforeidx
wq(bp-0x60,team_ref);wq(frame+9*8,context)
checks.append('Wrong team identity or evaluator preserves all vector/index bytes despite matching recruit scores')
# Observational modes, regular season, and user control do not reallocate.
for m,typ,user in [(0,6,False),(-1,6,False),(1,1,False),(1,6,True)]:
 configure(m);records[season]['scalar'][5]=typ
 if user:records[team]['ref'][407]=record('user')
 uc.mem_write(vec,before);uc.mem_write(idx,beforeidx);call(callback,frame)
 assert bytes(uc.mem_read(vec,n*72))==before and bytes(uc.mem_read(idx,n*8))==beforeidx
 records[team]['ref'].pop(407,None)
checks.append('Diagnostic, disabled, regular-season and user funding are byte-for-byte unchanged')
# Test13: exercise scoring and funding together. The native fit prefers the
# 82 QB by 30 points; the 88 QB's quality advantage must win primary funding.
configure(1);records[season]['scalar'][5]=6
for a in [departures,roster,hslist,tplist,committed,arr]:records[a]['array']=[]
for pos in range(22):records[ideal_entries[pos]]['scalar'].update({2:ideals[pos],4:1})
records[ideal_entries[0]]['scalar'][2]=4
records[roster]['array']=[pp('QB returner',v) for v in [79,70,66]]
lower,lt,lp=pr(42000,82);higher,ht,hp=pr(42001,88)
wq(frame+2*8,context);wq(frame+3*8,team_ref);uc.mem_write(frame+128+0x42,b'\1')
prepare_funding(frame)
holder=alloc(8);wq(scoreframe,holder)
wq(scoreframe+128+0xa8,team_ref);wq(scoreframe+128+0x88,GAME+0x081733fa)
values=[]
for rec,base in [(lower,417),(higher,387)]:
 wq(holder,rec);wq(scoreframe+8,base);wq(scoreframe+24,0);call(hooks[9],scoreframe)
 values.append(rq(scoreframe+8))
# Best single seat is the weak QB2: 100 + (82-70)*15 = 280 and
# 100 + (88-70)*15 = 370. Both exceed the starter-based bonus by 35.
assert values==[697,757],values
n=2
for j,(rec,target) in enumerate([(lower,lt),(higher,ht)]):
 uc.mem_write(vec+j*72,b'\0'*72);wq(vec+j*72,rec);wi(vec+j*72+40,values[j]);wq(vec+j*72+48,target);wi(vec+j*72+64,1);wq(idx+j*8,j)
wq(bp-0x48,vec+n*72);wq(bp+0x60,idx+n*8)
old=totals();call(callback,frame)
assert totals()==old and rq(vec)==higher and rq(vec+72)==lower
assert ri(vec+40)>ri(vec+72+40)>0
assert [rq(idx+j*8) for j in range(n)]==[0,1]
checks.append('Compiled scoring plus funding: 88 QB becomes primary ahead of native-preferred 82 QB, gets higher weight, and preserves exact stage total')
# The most recent score for a recruit controls provenance. An earlier matching
# score must not be reused after a later route records a different native fit.
for j,(rec,target) in enumerate([(lower,lt),(higher,ht)]):
 uc.mem_write(vec+j*72,b'\0'*72);wq(vec+j*72,rec);wi(vec+j*72+40,values[j]);wq(vec+j*72+48,target);wi(vec+j*72+64,1);wq(idx+j*8,j)
stale_vec=bytes(uc.mem_read(vec,n*72));stale_idx=bytes(uc.mem_read(idx,n*8))
wq(holder,higher);wq(scoreframe+8,388);wq(scoreframe+24,0);call(hooks[9],scoreframe)
call(callback,frame)
assert bytes(uc.mem_read(vec,n*72))==stale_vec and bytes(uc.mem_read(idx,n*8))==stale_idx
checks.append('Stale total mismatching latest exact score snapshot preserves native funding unchanged')
# Even a non-portal prepare invalidates the previous evaluator generation.
uc.mem_write(frame+128+0x42,b'\0');prepare_funding(frame);call(callback,frame)
assert bytes(uc.mem_read(vec,n*72))==stale_vec and bytes(uc.mem_read(idx,n*8))==stale_idx
# A subsequent valid prepare may publish only a partial set. One missing entry
# still preserves the complete native callback, including the known entry.
uc.mem_write(frame+128+0x42,b'\1');prepare_funding(frame)
wq(holder,lower);wq(scoreframe+8,417);wq(scoreframe+24,0);call(hooks[9],scoreframe)
call(callback,frame)
assert bytes(uc.mem_read(vec,n*72))==stale_vec and bytes(uc.mem_read(idx,n*8))==stale_idx
checks.append('Prepare invalidates prior generations; one missing entry preserves the entire native funding callback')
# Repeated evaluator creation cannot permanently exhaust the bounded registry.
# Keep an earlier thread's TLS alive to test a late writer after slot reuse.
old_thread_tls=dict(tls);tls.clear()
for _ in range(9):
 later_context=alloc(0x300);uc.mem_write(later_context,bytes(uc.mem_read(context,0x300)))
 wq(frame+2*8,later_context);prepare_funding(frame)
 for rec,base in [(lower,417),(higher,387)]:
  wq(holder,rec);wq(scoreframe+8,base);wq(scoreframe+24,0);call(hooks[9],scoreframe)
tls.clear();tls.update(old_thread_tls)
wq(holder,higher);wq(scoreframe+8,999);wq(scoreframe+24,0);call(hooks[9],scoreframe)
# The old evaluator was evicted; its stale writer cannot recreate provenance.
wq(frame+9*8,context);call(callback,frame)
assert bytes(uc.mem_read(vec,n*72))==stale_vec and bytes(uc.mem_read(idx,n*8))==stale_idx
# The latest evaluator still reallocates correctly, even on the old thread.
wq(frame+9*8,later_context);old=totals();call(callback,frame)
assert totals()==old and rq(vec)==higher and ri(vec+40)>ri(vec+72+40)>0
checks.append('Ninth new evaluator reuses bounded ledger storage; evicted stale writer is rejected and latest funding remains active')
call(exports['DriTestOwnershipReset']);assert all(v==1 for v in refs.values())
assert call(exports['DriTestQualityFault'])==0
checks.append('All record references balanced; no runtime fault')
(OUTPUT/'funding_runtime_results.json').write_text(json.dumps({'checks':checks,'mocked':['SDF storage','Windows APIs'],'fixture_heap':{'capacity_bytes':funding_fixture_heap_capacity,'allocated_bytes_without_free_reuse':next_heap-HEAP},'not_executed':['complete native spending engine','live game'],'live_game_verified':False},indent=2)+'\n')
print(json.dumps(checks,indent=2))
