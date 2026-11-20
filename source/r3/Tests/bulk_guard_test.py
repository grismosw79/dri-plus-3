"""Test15 shared acceptance guard in the captured native NSD bulk-signing path.

Executes the compiled guard and observer with the captured native leader check,
soft-commit map preparation, and Signed write. SDF/Windows and map services are
controlled fixtures; this is not a live-game result or a season simulation.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'bulk_ownership_test.py').read_text(),'bulk_ownership_test.py','exec'))
import re

guard_checks=[]
hook_names=re.findall(r'^\.global\s+(Hook\w+)',(ROOT/'hooks.S').read_text(),re.M)
hook_names=[name for name in hook_names if not name.startswith('HookEps')]
guard_names=[name for name in hook_names if 'BulkGuard' in name]
assert len(guard_names)==1,guard_names
guard_index=hook_names.index(guard_names[0])
guard_address=BASE+offsets[guard_index]
guard_end=offsets[guard_index+1] if guard_index+1<len(offsets) else offsets[guard_index]+600
guard_ins=list(cs.disasm(image[offsets[guard_index]:guard_end],guard_address))
guard_call=next(i for i in guard_ins if i.mnemonic=='call' and 'rip' in i.op_str)
guard_jumps=[i for i in guard_ins if i.mnemonic=='jmp' and 'rip' in i.op_str]
assert len(guard_jumps)>=2
GUARD,GUARD_RESUME=0x08162f6f,0x08162f81
assert raw[GUARD-0x1000:GUARD_RESUME-0x1000]==bytes.fromhex('488b4defba09000000488b5de7e87f2defff')
wq(guard_call.address+guard_call.size+guard_call.operands[0].mem.disp,GAME+0x08055d00)
for instruction,destination in zip(guard_jumps[:2],[GUARD_RESUME,SKIP]):
 wq(instruction.address+instruction.size+instruction.operands[0].mem.disp,GAME+destination)
uc.mem_write(GAME+GUARD,bytes.fromhex('ff2500000000')+struct.pack('<Q',guard_address)+b'\x90'*4)
uc.mem_write(GAME+JOIN,bytes.fromhex('ff2500000000')+struct.pack('<Q',hook_address)+b'\x90'*4)
uc.ctl_remove_cache(GAME+START,GAME+SKIP+1)

map_calls={'allocate':0,'insert':0}
def bulk_allocate():
 map_calls['allocate']+=1
 return alloc(reg('RDX'))
def bulk_insert():
 map_calls['insert']+=1
 return 0
game_api(0x019ea660,bulk_allocate)
game_api(0x0212a5f0,bulk_insert)

# Match the captured parent frame, not just its individual field offsets.
# LEA RBP,[RSP-2F] followed by SUB RSP,A8 gives RBP=active RSP+79.
# The inherited observer-only fixture put RBP 24 KiB *below* active RSP;
# Test15's legitimate HS snapshot stack then overlapped that synthetic frame.
assert raw[0x08162c73-0x1000:0x08162c7f-0x1000]==bytes.fromhex('488d6c24d14881eca8000000')
bulk_rsp=STACK+0x32000
bulk_bp=bulk_rsp+0x79

def guarded_bulk(rec,accepted,leader_tid=None):
 stage=records[rec]['scalar'][9]
 assert stage in [4,5]
 tid=records[team]['scalar'][383]
 leader=record('guarded native leading school',{0:tid if leader_tid is None else leader_tid,1:100})
 wq(bulk_bp-1,0xabcdef);wq(bulk_bp-0x11,0xabcdef)
 wq(bulk_bp-0x19,rec);wq(bulk_bp-0x49,rec);wq(bulk_bp+0x77,book)
 map_calls.update(allocate=0,insert=0)
 rsp=bulk_rsp;wq(rsp,STOP)
 for name,value in [('RSP',rsp),('RBP',bulk_bp),('R12',team),('R13',context),('R14',leader),('R15',book),('RCX',leader)]:
  uc.reg_write(globals()['UC_X86_REG_'+name],value)
 uc.emu_start(GAME+START,STOP+1,count=2000000)
 assert reg('RIP')==STOP
 assert rq(bulk_bp-0x19)==rec,('native parent recruit frame overwritten',hex(rq(bulk_bp-0x19)))
 assert records[rec]['scalar'][9]==(6 if accepted else stage),(tid,stage,accepted,records[rec]['scalar'][9])
 assert map_calls==({'allocate':1,'insert':1} if accepted and stage==4 else {'allocate':0,'insert':0}),map_calls
 assert call(exports['DriTestQualityFault'])==0

# The new guard runs before the soft-commit path allocates and inserts its
# destination record. Blocking only the old shared writer would be too late.
for academy in [0,7,55]:
 for user in [False,True]:
  for stage in [4,5]:
   reset(tid=academy)
   if user:records[team]['ref'][407]=record('academy user coach')
   rec,target,player=pr(9500+academy*4+stage,85,stage=stage)
   listed(rec,academy)
   assert call(exports['DriTestActiveOffer'],rec,team)==1
   guarded_bulk(rec,False)
   assert player not in records[committed]['array']
guard_checks.append('Air Force, Army and Navy are blocked for CPU and user teams at both soft/hard native bulk-signing stages, before destination allocation or insertion')

for stage in [4,5]:
 for status in [2,3,4]:
  reset()
  rec,target,player=pr(9800+stage,80,stage=stage);listed(rec,TEST_TEAM_ID)
  records[target]['scalar'][12]=status
  guarded_bulk(rec,stage==5)
  assert player not in records[committed]['array']
  assert quality()[3:6]==((1,0,80) if stage==5 else (0,0,-1))
guard_checks.append('New, offered and committed matching scholarships let nonacademy soft/hard commitments reach the captured Signed writer and count before publication')

for stage in [4,5]:
 for status in [None,0,1]:
  reset()
  rec,target,player=pr(9810+stage,91,stage=stage);listed(rec,TEST_TEAM_ID)
  if status is None:records[arr]['array']=[]
  else:records[target]['scalar'][12]=status
  records[rec]['scalar'][14]=0
  guarded_bulk(rec,False)
  assert quality()[3:6]==(0,0,-1)
guard_checks.append('Offerless, unoffered and revoked soft/hard portal commitments cannot become Signed merely because the native leader matches the destination')

reset()
rec,target,player=pr(9820,91,stage=5);listed(rec,TEST_TEAM_ID)
other,other_target,other_player=pr(9820,70,stage=1)
records[arr]['array']=[other_target]
guarded_bulk(rec,False)
guard_checks.append('A live scholarship for another recruit with the same row number in a different table does not validate commitment ownership')

# Academy portal exclusion must not veto otherwise legal HS/JUCO signings.
# These empty rooms also satisfy the new ordinary CPU acceptance policy.
for academy in [0,7,55]:
 for category in [0,1,2,3]:
  for stage in [4,5]:
   reset(tid=academy)
   rec,target,player=pr(9830+category*2+stage,75,stage=stage,portal=False)
   records[rec]['scalar'][2]=category;listed(rec,academy)
   guarded_bulk(rec,stage==5)
   assert quality()[3:6]==((1,1,-1) if stage==5 else (0,0,-1))
guard_checks.append('All four nonportal recruit categories retain legal native soft/hard bulk signing for all three academies; the captured RBP/RSP relationship preserves parent frame fields during the larger HS snapshot')

for mode_value in [0,-1]:
 for stage in [4,5]:
  reset(tid=7)
  rec,target,player=pr(9870+stage,80,stage=stage);listed(rec,7)
  configure(mode_value);guarded_bulk(rec,True)
  configure(1)
  assert quality()[3:6]==(0,0,-1)
guard_checks.append('Diagnostic and disabled modes preserve native academy signing and do not create an ownership ledger entry')

reset()
rec,target,player=pr(9880,80,stage=5);listed(rec,TEST_TEAM_ID)
guarded_bulk(rec,False,leader_tid=1)
native_signed(rec)
assert quality()[3:6]==(0,0,-1)
guard_checks.append('Native leader/destination mismatch bypasses the guard and observer without inventing ownership')

# Real acceptance can outlive a changed interest leader. The pending ledger is
# activated only by an actual hard commit and then preserves its destination.
reset()
rec,target,player=pr(9890,80,stage=4);listed(rec,TEST_TEAM_ID)
assert pending(rec)==0
records[rec]['scalar'][9]=5
records[arr]['array']=[]
records[records[rec]['ref'][13]]['array']=[record('later stale leader',{0:1,1:100})]
owner_out=alloc(4)
assert call(exports['DriTestHardOwner'],rec,context,owner_out)==1 and ri(owner_out)==TEST_TEAM_ID
# Native bulk collection still requires its own leader/destination comparison.
# Restore that native precondition while leaving the offer absent.
records[records[rec]['ref'][13]]['array']=[record('native accepted leader',{0:TEST_TEAM_ID,1:100})]
guarded_bulk(rec,True)
assert quality()[3:6]==(1,0,80)
guard_checks.append('An accepted native destination plus actual HardCommitted state remains authoritative after offer removal or stale interest-list changes')

reset()
rec,target,player=pr(9891,80,stage=5);listed(rec,TEST_TEAM_ID)
rival=records[teams]['array'][1]
rival_commits=records[rival]['ref'][33]
records[rival_commits]['array']=[player]
assert call(exports['DriTestHardOwner'],rec,context,owner_out)==1 and ri(owner_out)==1
guarded_bulk(rec,False)
records[rival_commits]['array']=[]
guard_checks.append('An already published destination overrides a stale leading-school entry and a rival scholarship')

# Exercise every acceptance callback carrying a native commitment/signing
# opportunity, with the user's academy receiving the same exclusion as a CPU.
for academy in [0,7,55]:
 for user in [False,True]:
  reset(tid=academy)
  if user:records[team]['ref'][407]=record('academy user for acceptance callbacks')
  rec,target,player=pr(9893,85,stage=1);listed(rec,academy);valid_offer(rec,target)
  wq(obp-0x41,rec);wq(obp+0x77,team_ref);wq(obp+0x67,context)
  assert call(instant_cb,of)==5
  assert pending(rec,board=True)==5
  assert pending(rec,board=False)==5
  records[transition]['scalar'][1]=5
  wq(osp+0x38,transition);wq(obp+0x68,rec);wq(obp+0x60,context);wq(of+14*8,1)
  call(weekly_cb,of)
  assert rq(of+14*8)==0 and records[rec]['scalar'][9]==1
guard_checks.append('All three academies, including user teams, are excluded at weekly, instant, board-signing and portal-deadline acceptance callbacks')

# The native instant caller writes New=2 before calling its instant check.
# A valid first offer must be accepted, but only an actual stage write can
# activate the remembered destination.
reset()
rec,target,player=pr(9894,80,stage=1);listed(rec,TEST_TEAM_ID);valid_offer(rec,target)
records[target]['scalar'][12]=2
wq(obp-0x41,rec);wq(obp+0x77,team_ref);wq(obp+0x67,context)
assert call(instant_cb,of)==0
assert call(exports['DriTestHardOwner'],rec,context,owner_out)==0
instant_write,instant_written=0x08156750,0x08156776
map_game(instant_write,instant_written-instant_write+1)
uc.mem_write(GAME+instant_write,raw[instant_write-0x1000:instant_written-0x1000])
game_api(instant_written,lambda:0)
uc.reg_write(UC_X86_REG_R14,rec);uc.reg_write(UC_X86_REG_RAX,0x100009)
call(GAME+instant_write)
assert records[rec]['scalar'][9]==5
records[arr]['array']=[]
assert call(exports['DriTestHardOwner'],rec,context,owner_out)==1 and ri(owner_out)==TEST_TEAM_ID
guard_checks.append('A matching New scholarship remains eligible for an instant commitment; the accepted destination activates only after the captured native HardCommitted write')

reset()
rec,target,player=pr(9892,80,stage=5);listed(rec,TEST_TEAM_ID)
duplicate=record('duplicate target for same recruit',{12:0},ref={10:rec})
records[arr]['array'].append(duplicate)
before=dict(records[rec]['scalar'])
assert call(exports['DriTestActiveOffer'],rec,team)==0xffffffff
assert call(exports['DriTestHardOwner'],rec,context,owner_out)==0xffffffff
assert records[rec]['scalar']==before and records[arr]['array']==[target,duplicate]
guard_checks.append('Duplicate matching targets fail ownership validation before any recruit or board mutation, even if the duplicate has no scholarship')

reset()
rec,target,player=pr(9895,65,stage=4,portal=False);listed(rec,TEST_TEAM_ID)
records[committed]['array']=[player]
guarded_bulk(rec,False)
assert records[committed]['array']==[player]
guard_checks.append('A published HS acquisition skips native bulk queueing and Signed write; existing acquisition is finalized once at the NSD boundary')

call(exports['DriTestOwnershipReset'])
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
guard_checks.append('All retained fixture references are released and every case leaves the runtime enabled')
result={'checks':guard_checks,'native_executed':'0815F1CA through 0815F3E8 / rejection exit 0815F3F8','guard':'0815F25F, 18 bytes, resume 0815F271, reject 0815F3F8','observer':'0815F3B1, 18 bytes, resume 0815F3C3','fixture_services':['Windows','SDF storage','bookkeeping allocation/insertion'],'live_game_verified':False}
(OUTPUT/'bulk_guard_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
