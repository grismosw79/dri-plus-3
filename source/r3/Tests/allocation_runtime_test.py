"""Compiled allocator/retention callbacks and captured final-consumption loop.
SDF storage and add-to-board operation are fixtures; removal uses captured code.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'quality_runtime_test.py').read_text(),'quality_runtime_test.py','exec'))
retain_cb,allocate_cb=hooks[14:16]
configure(1);records[team]['scalar'][384]=8
# Test14 reads live native stage/rank eligibility for existing offered targets.
wq(context+0xd0,tuning)
for r in [departures,hslist,tplist,committed,arr]:records[r]['array']=[]
records[roster]['array']=[pp('QB'+str(i),v) for i,v in enumerate([74,74,73,69])]+[pp('WR',70,pos=3),pp('P',77,pos=20),pp('LT1',85,pos=5),pp('LT2',80,pos=5)]
records[ideal_entries[0]]['scalar'].update({2:4,4:1})
records[ideal_entries[3]]['scalar'].update({2:10,4:2})
records[ideal_entries[5]]['scalar'].update({2:3,4:1})
records[ideal_entries[20]]['scalar'].update({2:2,4:1})
pairs=[pr(5000,86),pr(5001,92),pr(5002,86,pos=3),pr(5003,82,pos=20),pr(5004,75,pos=20),pr(5005,70,pos=5)]
bases=[370,350,200,517,467,900]
hs,hst,hsp=pr(5010,70,pos=6,stage=6,portal=False)
original_targets={r:t for r,t,p in pairs};old_qb,old_p,old_depth=[pairs[i] for i in [0,3,5]]
records[arr]['array']=[hst,old_qb[1],old_p[1],old_depth[1]]
records[old_p[1]]['scalar'][2]=55
protected_before=dict(records[old_p[1]]['scalar'])
af=STACK+0x23000;asp=af+128;abp=asp+0x200
wq(af+10*8,abp);wq(af+2*8,context);wq(af+3*8,team_ref)
wq(abp+0x28,team_ref);wq(asp+0x58,context);uc.mem_write(asp+0x42,b'\1')
call(prepare_cb,af)
assert call(exports['DriTestQualityFault'])==0
sf2=STACK+0x25000;ss2=sf2+128
holder=alloc(8);wq(sf2,holder);wq(ss2+0xa8,team_ref);wq(ss2+0x88,GAME+0x0818c282)
def fresh_score(rec,base):
 wq(holder,rec);wq(sf2+1*8,base);wq(sf2+3*8,0);call(score_cb,sf2);return rq(sf2+1*8)
encoded=[fresh_score(r,base) for (r,t,p),base in zip(pairs,bases)]
assert encoded==[3000750,3000820,3000640,3000692,2000537,1000900],encoded

# Retention override occurs after native flags: all unsecured portal candidates
# are reconsidered. Secured HS/portal and the user team are not changed.
for r,_,_ in pairs:
 wq(abp-0x58,r);uc.mem_write(asp+0x40,b'\0');uc.mem_write(asp+0x44,b'\0')
 call(retain_cb,af);assert bytes(uc.mem_read(asp+0x40,1))==b'\1' and bytes(uc.mem_read(asp+0x44,1))==b'\1'
wq(abp-0x58,hs);uc.mem_write(asp+0x40,b'\0');uc.mem_write(asp+0x44,b'\0');call(retain_cb,af)
assert bytes(uc.mem_read(asp+0x40,1))==b'\0'
records[pairs[0][0]]['scalar'][9]=5;wq(abp-0x58,pairs[0][0]);call(retain_cb,af);assert bytes(uc.mem_read(asp+0x44,1))==b'\0'
records[pairs[0][0]]['scalar'][9]=1

vec2=alloc(24);main2=alloc(len(pairs)*48);rem2=alloc(3*48)
wq(vec2,main2);wq(vec2+8,main2+len(pairs)*48);wq(vec2+16,main2+len(pairs)*48)
wq(abp-0x68,vec2);wq(asp+0x68,len(pairs));wq(asp+0x50,4);wi(abp+0x30,4)
wq(asp+0x78,rem2);wq(abp-0x80,rem2+3*48)
for i,((r,t,p),v) in enumerate(zip(pairs,encoded)):
 wq(main2+i*48,r);wi(main2+i*48+40,v);incref(r)
for i,(r,t,p) in enumerate([old_qb,old_p,old_depth]):
 wq(rem2+i*48,r);wi(rem2+i*48+40,10+i);incref(r)
call(allocate_cb,af)
assert call(exports['DriTestQualityFault'])==0
ordered=[rq(main2+i*48) for i in range(len(pairs))]
assert ordered==[pairs[i][0] for i in [2,3,5,4,1,0]],ordered
assert [ri(main2+i*48+40) for i in range(len(pairs))]==[640,692,900,537,820,750]
assert rq(abp-0x80)==rem2+1*48
assert {rq(rem2+i*48) for i in range(1)}=={old_qb[0]}
assert records[arr]['array']==[hst,old_qb[1],old_p[1],old_depth[1]]
assert records[old_p[1]]['scalar']==protected_before
allocationchecks=['Genuine WR, P and LT vacancies precede full QB room upgrade alternatives',
 'Distinct vacancies covered before alternative candidates',
 'Full-board refresh protects selected punter and depth targets in place',
 'HS and secured portal commitments retain native protection; transient scores restored']

# Execute captured native loop against the actual reordered arrays. Native
# removal handles offers/visits/hours; addition is mocked and independently
# checks the live roster policy before creating a target.
map_game(0x08172d10,0x1000);uc.mem_write(GAME+0x08172d10,raw[0x08172d10-0x1000:0x08173d10-0x1000])
def is_on_board():
 rec=rq(reg('R8'));return any(records[t]['ref'].get(10)==rec for t in records[arr]['array'])
admitted=[]
def add_target():
 rec=rq(reg('R8'));assert decide_saved[rec]==0
 t=original_targets[rec];assert t not in records[arr]['array']
 records[arr]['array'].append(t);records[t]['ref'][10]=rec;admitted.append(rec);return 0
decide_saved={r:decide(r) for r,t,p in pairs}
game_api(0x081b7e60,is_on_board);game_api(0x08149660,add_target)
uc.reg_write(UC_X86_REG_RSP,asp);uc.reg_write(UC_X86_REG_RBP,abp)
uc.reg_write(UC_X86_REG_R12,team_ref);uc.reg_write(UC_X86_REG_R13,0);uc.reg_write(UC_X86_REG_R15,vec2);uc.reg_write(UC_X86_REG_RSI,0)
uc.emu_start(GAME+0x081734f0,GAME+0x0817363b,count=500000)
assert reg('RIP')==GAME+0x0817363b
assert set(admitted)=={pairs[2][0]}
assert set(records[arr]['array'])=={hst,pairs[2][1],pairs[3][1],pairs[5][1]}
assert len(records[arr]['array'])==rq(asp+0x50)==4
assert records[old_p[1]]['scalar']==protected_before
assert not records[old_qb[1]]['ref'][10]
assert rq(abp-0x80)==rem2
for i in range(len(pairs)):decref(rq(main2+i*48))
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
allocationchecks.append('Captured native consumption replaces full-room QB target, preserves selected punter/depth target identity/hours and HS, balances every reference, respects capacity')

# Vacancies precede the full-room upgrade; the better QB now leads that race.
# No artificial minimum board size or forced filling.
for capacity,expected in [(k,[2,3,5,4,1,0][:k]) for k in range(1,7)]:
 wq(af+10*8,abp);wq(af+2*8,context);wq(af+3*8,team_ref)
 call(prepare_cb,af)
 for (r,t,p),v in zip(pairs,bases):fresh_score(r,v)
 for i,(r,t,p) in enumerate(pairs):wq(main2+i*48,r);wi(main2+i*48+40,encoded[i]);incref(r)
 wq(asp+0x50,0);wi(abp+0x30,capacity);wq(asp+0x78,rem2);wq(abp-0x80,rem2)
 call(allocate_cb,af)
 assert [rq(main2+i*48) for i in range(capacity)]==[pairs[i][0] for i in expected]
 for i in range(len(pairs)):decref(rq(main2+i*48))
allocationchecks.append('Capacity 1–6: physical vacancies and their first fallback precede optional full-room quality in exact order')

# Disabled after scoring must still restore the transient shortlist values;
# diagnostic mode never encodes, reorders or edits retention flags.
configure(1);call(prepare_cb,af)
for (r,t,p),v in zip(pairs,bases):fresh_score(r,v)
for i,(r,t,p) in enumerate(pairs):wq(main2+i*48,r);wi(main2+i*48+40,encoded[i])
configure(-1);call(allocate_cb,af)
assert [rq(main2+i*48) for i in range(len(pairs))]==[r for r,t,p in pairs]
assert [ri(main2+i*48+40) for i in range(len(pairs))]==[750,820,640,692,537,900]
configure(0);call(prepare_cb,af)
assert [fresh_score(r,v) for (r,t,p),v in zip(pairs,bases)]==bases
before=bytes(uc.mem_read(main2,len(pairs)*48));call(allocate_cb,af)
assert bytes(uc.mem_read(main2,len(pairs)*48))==before
wq(abp-0x58,pairs[0][0]);uc.mem_write(asp+0x40,b'\0');uc.mem_write(asp+0x44,b'\0');call(retain_cb,af)
assert bytes(uc.mem_read(asp+0x40,1))==b'\0' and bytes(uc.mem_read(asp+0x44,1))==b'\0'
configure(1);records[team]['ref'][407]=record('allocation user team');call(prepare_cb,af)
assert fresh_score(pairs[0][0],370)==370
call(allocate_cb,af);assert bytes(uc.mem_read(main2,len(pairs)*48))==before
records[team]['ref'].pop(407)
allocationchecks.append('Diagnostic/user paths do not modify allocation; mid-pass disable restores transient scores without reordering')

# Signing guards still act on current records, independently of planning cache.
records[pairs[0][0]]['scalar'][9]=5
valid_offer(pairs[0][0],pairs[0][1])
assert decide(pairs[1][0])==2 # New projected85 elite-QB guard precedes the room cap.
records[pairs[0][0]]['scalar'][9]=1
assert decide(pairs[1][0])==0
allocationchecks.append('Same-advance86QB commitment blocks a second92QB under the projected85 elite guard; decommitment reopens')
assert call(exports['DriTestQualityFault'])==0
call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values())
(OUTPUT/'allocation_runtime_results.json').write_text(json.dumps({'checks':allocationchecks,'live_game_verified':False,'mocked':['SDF storage','Windows APIs','native add operation']},indent=2)+'\n')
print(json.dumps(allocationchecks,indent=2))
