"""Exercise the missed native soft/hard -> Signed path and Test07 observer.

Captured leader/team comparison, stage branch, and Signed write execute with
the compiled hook and callback. Windows, SDF storage and map services remain
fixtures. The collector's stage-4/stage-5 calls are checked in captured bytes;
this is not a live game or a complete native recruiting simulation.
"""
exec(compile((__import__('pathlib').Path(__file__).parent/'signing_ownership_test.py').read_text(),'signing_ownership_test.py','exec'))
bulk_cb=hooks[17];bulk_checks=[]
offsets=[];at=0
while (at:=image.find(prefix,at))>=0:offsets.append(at);at+=1
hook_address=BASE+offsets[17]
ins=list(cs.disasm(image[offsets[17]:offsets[17]+400],hook_address))
native_call=next(x for x in ins if x.mnemonic=='call' and 'rip' in x.op_str)
native_jump=next(x for x in ins if x.mnemonic=='jmp' and 'rip' in x.op_str)
wq(native_call.address+native_call.size+native_call.operands[0].mem.disp,GAME+0x08055d00)
wq(native_jump.address+native_jump.size+native_jump.operands[0].mem.disp,GAME+0x081630d3)
START,JOIN,RESUME,END,SKIP=0x08162eda,0x081630c1,0x081630d3,0x081630f8,0x08163108
map_game(START,SKIP-START+1)
native_bytes=raw[START-0x1000:END-0x1000]
uc.mem_write(GAME+START,native_bytes)
game_api(END,lambda:0);game_api(SKIP,lambda:0)
book=alloc(0x60);buckets=alloc(0x18);wq(book+8,buckets);wi(book+0x10,1)
game_api(0x08057470,lambda:ri(reg('RCX')+0x18))
game_api(0x019ea660,lambda:alloc(reg('RDX')))
game_api(0x0212a5f0,lambda:0)
def unexpected_duplicate():raise AssertionError('Unexpected duplicate in fresh bookkeeping fixture')
game_api(0x037f4530,unexpected_duplicate)
bulk_bp=STACK+0x2c000

# Guard the provenance of the path: collect both stages, compare ownership,
# branch hard commits directly to the shared writer, write Signed=6.
assert raw[0x08162d56-0x1000:0x08162d5c-0x1000]==bytes.fromhex('41b804000000')
assert raw[0x08162d73-0x1000:0x08162d79-0x1000]==bytes.fromhex('41b805000000')
for rva in [0x08162d65,0x08162d84]:
 i=next(cs.disasm(raw[rva-0x1000:rva-0x1000+5],rva))
 assert i.mnemonic=='call' and i.operands[0].imm==0x08187730
assert raw[0x08162f98-0x1000:0x08162fa1-0x1000]==bytes.fromhex('83f8050f8420010000')
assert raw[JOIN-0x1000:RESUME-0x1000]==bytes.fromhex('488b4defba09000000488b7de7e82d2cefff')
bulk_checks.append('Captured collector calls select stage 4 and stage 5; both converge at the exact Signed writer')

def bulk_signed(rec,observe=True,leader_tid=None):
 stage=records[rec]['scalar'][9];assert stage in [4,5]
 tid=records[team]['scalar'][383]
 leader=record('bulk leading school',{0:tid if leader_tid is None else leader_tid,1:100})
 wq(bulk_bp-1,0xabcdef);wq(bulk_bp-0x11,0xabcdef)
 wq(bulk_bp-0x19,rec);wq(bulk_bp-0x49,rec);wq(bulk_bp+0x77,book)
 uc.mem_write(GAME+JOIN,(bytes.fromhex('ff2500000000')+struct.pack('<Q',hook_address)+b'\x90'*4) if observe else native_bytes[JOIN-START:RESUME-START])
 uc.ctl_remove_cache(GAME+START,GAME+SKIP+1)
 rsp=STACK+0x32000;wq(rsp,STOP)
 for n,v in [('RSP',rsp),('RBP',bulk_bp),('R12',team),('R13',context),('R14',leader),('R15',book),('RCX',leader)]:
  uc.reg_write(globals()['UC_X86_REG_'+n],v)
 uc.emu_start(GAME+START,STOP+1,count=2000000)
 assert reg('RIP')==STOP
 assert records[rec]['scalar'][9]==(6 if leader_tid is None or leader_tid==tid else stage)

# Test06's HS weekly observer intentionally ignores stage-4 transitions.
# Without the new bulk observer, native signing hides this HS destination.
reset();records[roster]['array']=[pp('returner',74)]
r,_,p=pr(9000,72,stage=4,portal=False);listed(r,TEST_TEAM_ID)
records[transition]['scalar'][1]=4
wq(osp+0x38,transition);wq(obp+0x68,r);wq(obp+0x60,context)
call(hooks[16],of);bulk_signed(r,observe=False)
assert quality()[:6]==(0,4,1,1,1,-1)
bulk_checks.append('Negative control: existing Test06 HS observer plus native soft signing still omits the new HS player before publication')

# Pending permission and an unresolved soft commit never count as a signing.
reset();records[roster]['array']=[pp('returner',74)]
r,_,p=pr(9010,72,stage=4,portal=False);listed(r,TEST_TEAM_ID)
wq(of+3*8,team);wq(of+2*8,context);wq(obp-0x19,r)
before=bytes(uc.mem_read(of,128));state=dict(records[r]['scalar'])
call(bulk_cb,of)
assert bytes(uc.mem_read(of,128))==before and records[r]['scalar']==state
assert quality()[:6]==(0,4,1,1,1,-1)
bulk_signed(r);assert quality()[:6]==(0,4,1,2,2,-1)
bulk_checks.append('Observer alone changes neither registers nor recruit fields and does not count a soft commit; native Signed state activates ownership')

for stage in [4,5]:
 for is_portal in [False,True]:
  reset();records[roster]['array']=[pp('returner',74)]
  r,_,p=pr(9020+stage*2+int(is_portal),80,stage=stage,portal=is_portal);listed(r,TEST_TEAM_ID)
  # No preceding roster scan, hard-commit observation or published list.
  bulk_signed(r)
  q=quality();assert q[:6]==(0,4,1,2,1 if is_portal else 2,80 if is_portal else -1)
  assert p not in records[committed]['array']
  records[committed]['array']=[p];assert quality()[:6]==q[:6]
  call(exports['DriTestOwnershipReset']);assert quality()[:6]==q[:6]
bulk_checks.append('Compiled hook with captured owner comparison, soft/hard branch and Signed writer preserves HS/portal classification, deduplicates publication and survives ledger reset after publication')

for mode_value in [0,-1]:
 reset();configure(mode_value)
 r,_,p=pr(9040+mode_value,80,stage=4,portal=False);listed(r,TEST_TEAM_ID)
 bulk_signed(r);configure(1);assert quality()[3:6]==(0,0,-1)
reset();r,_,p=pr(9050,80,stage=4,portal=False);listed(r,1)
bulk_signed(r,leader_tid=1);assert records[r]['scalar'][9]==4
# A later unobserved Signed write must not reveal a spurious pending owner.
native_signed(r);assert quality()[3:6]==(0,0,-1)
reset();records[team]['ref'][407]=record('user character')
r,_,p=pr(9051,80,stage=4,portal=False);listed(r,TEST_TEAM_ID);bulk_signed(r)
assert quality()[3:6]==(1,1,-1)
bulk_checks.append('Native leader/team mismatch bypasses the observer; diagnostic and disabled modes do not record owners; user HS signing is observed without a recruiting veto')

case_results=[]
for ci,event in enumerate(json.loads((ROOT.parent/'Audit/test06_nsd_false_admission_cases.json').read_text())):
 pos=POS.index(event['position']);reset(event['tid'],event['prestige'],pos,event['ideal'],event['starters']);isolate_archived_room(pos)
 missing={p['pid'] for p in event['omitted_HS']}
 for p in event['actual_before']:
  if not p['incoming'] and p['pid'] not in missing:
   records[roster]['array'].append(pp(p['name'],p['ovr'],pos=pos,tid=event['tid']))
 for j,p in enumerate(event['actual_before']):
  if p['pid'] in missing or p['incoming']:
   r,_,_=pr(9100+ci*30+j,p['ovr'],pos=pos,stage=4 if p['pid'] in missing else 5,portal=p['incoming'])
   listed(r,event['tid']);bulk_signed(r)
 candidate,_,_=pr(9129+ci*30,event['ovr'],pos=pos)
 got=decide(candidate);assert got==event['actual_reason'],(event['team'],event['name'],got)
 assert quality(pos)[3]==event['actual_count']
 case_results.append({'team':event['team'],'player':event['name'],'count':quality(pos)[3],'reason':got})
bulk_checks.append('All four actual Test06 false-admission snapshots reject under unchanged policy, with missing HS players signed via the captured soft branch')
call(exports['DriTestOwnershipReset']);assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[r]['name'],v) for r,v in refs.items() if v!=1]
bulk_checks.append('All retained references released and no runtime fixture faults')
result={'checks':bulk_checks,'actual_test06_cases':case_results,'native_executed':'0815F1CA through 0815F3E8 / mismatch exit 0815F3F8','hook':'0815F3B1, 18 bytes, resume 0815F3C3','fixture_services':['Windows','SDF storage','bookkeeping allocation/insertion'],'live_game_verified':False}
(OUTPUT/'bulk_ownership_results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
