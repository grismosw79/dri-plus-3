"""Test30 compiled HS callback and captured native condition evaluator.
The SDF expression leaf has controlled threshold/boolean semantics; the real
captured evaluator builds arguments, dispatches the expression and returns it.
This proves ABI, choice/restore and freshness, not live FTC-expression results.
"""
from pathlib import Path
import json,hashlib,re
src=(Path(__file__).parent/'test28r11_committable_test.py').read_text()
exec(compile(src.split('reset29()\nr,p=make(91000')[0],'HS fixture','exec'))
checks=[];evaluations=[];extra_condition=True;withdraw_during_condition=False;fault_during_condition=False
# Captured evaluator, unchanged bytes; mock only SDF expression/value services.
start,end=0x08627910,0x08627ba0
uc.mem_map(GAME+start,end-start)
uc.mem_write(GAME+start,raw[start-0x1000:end-0x1000])
uc.mem_map(GAME+0x0da92840,8)
game_api(0x078cb430,lambda:reg('RAX')) # native cookie check preserves the return value
def variant_type():
    wi(reg('RCX'),reg('RDX'));return reg('RCX')
def copy_variant():
    dest,src,_,_=args();uc.mem_write(dest,bytes(uc.mem_read(src,24)))
    if ri(src) in [7,9]:incref(rq(src+8))
    return dest
def expression():
    out,ref,field,other=args();assert field==0x100002 and rq(ref)==rq(other)
    transition=rq(ref);vector=rq(reg('RSP')+0x28);begin=rq(vector)
    r=rq(begin+8);tun=rq(begin+24+8)
    assert tun==tuning and records[transition]['scalar'][1]==4
    leader,points=values(r)[0];evaluations.append((ri(r+0x18),leader,points))
    if withdraw_during_condition:
        for target in records[board_targets(allteams[leader])]['array']:
            if records[target]['ref'].get(10)==r:records[target]['scalar'][12]=1
    if fault_during_condition:records[allteams[leader]]['ref'][64]=0
    result=int(points>=records[r]['scalar'].get(3,1000) and extra_condition)
    wi(out,1);wq(out+8,result);return out
game_api(0x0806cf90,variant_type);game_api(0x037eec00,copy_variant);game_api(0x08069b20,expression)
def own(r,tid):
    wr=alloc(32);wq(wr,GAME+0x0bd2e980);wq(wr+8,r);wq(wr+16,0xabcdef);wq(wr+24,1);tr=alloc(8);wq(tr,allteams[tid])
    return call(GAME+0x0862d6d0,wr,tr)&255
def native_attempt(r,next_stage=4):
    frame=alloc(0x180);bp=alloc(0x200)+0x80;transition=record('HS native transition',{1:next_stage})
    wq(frame+80,bp);wq(frame+128+0x30,GAME+0x0bd2eea0);wq(frame+128+0x38,transition);wq(frame+128+0x40,0xabcdef)
    wq(bp+0x68,r);wq(bp+0x60,context)
    registers=bytes(uc.mem_read(frame,128));stage=records[r]['scalar'][9];before=dict(values(r))
    result=call(hooks[16],frame)
    assert bytes(uc.mem_read(frame,128))==registers
    assert records[r]['scalar'][9]==stage
    return result

negative=os.environ.get('DRI31_SIGN_NEGATIVE')=='1'
# Full native stage writer and its influence adjustment. Only storage lookup
# services are fixtures; the selected leader, bonuses and Signed enum are native.
for start,end in [(0x0814c3c0,0x0814c9b8),(0x081b2730,0x081b2e10)]:
 map_game(start,end-start);uc.mem_write(GAME+start,raw[start-0x1000:end-0x1000])
wq(context+0xa8,franchise)
def lookup_target():
 mgr,out,board_ref,player_ref=args();player=rq(player_ref)
 board=records[rq(board_ref)]['ref'][0]
 found=next((t for t in records[board]['array'] if records[records[t]['ref'][10]]['ref'][5]==player),0)
 if found:incref(found)
 wq(out,found);return out
game_api(0x081790c0,lookup_target)
def stage_write(r,stage):
 holder=alloc(8);wq(holder,r);call(GAME+0x0814c3c0,context,holder,stage)
def prepare(rid=31001,value=1100):
 reset29();records[tuning]['scalar'][13]=10;records[ideal_entries[0]]['scalar'][2]=4
 r,p=make(rid,((10,1200),(11,900),(12,700)),[13],78,3)
 target=next(t for t in records[board_targets(allteams[13])]['array'] if records[t]['ref'][10]==r)
 records[target]['scalar'][8]=value;records[target]['scalar'][9]=value;records[target]['scalar'][7]=100
 assert call(exports['DriTestHs29Offer'],r,allteams[13],context)==2, (call(exports['DriTestHs29Offer'],r,allteams[13],context),call(exports['DriTestHsPairDecision'],r,allteams[13],context,0),call(exports['DriTestQualityFault']))
 return r,p,target
checks=[]
r,p,target=prepare();before=values(r);result=native_attempt(r)
if negative:
 assert result==1 and values(r)==before
 checks.append('Test30 negative: real firm outside-list offer with1100 earned influence cannot be selected despite native1000 threshold')
else:
 assert result==0 and values(r)==[(13,1100),(10,1200),(11,900)],values(r)
 assert evaluations[-1]==(31001,13,1100)
 stage_write(r,4)
 assert records[r]['scalar'][9]==4
 assert values(r)==[(13,1000),(10,1100),(11,800)],values(r)
 assert records[target]['scalar'][8]==1000
 checks.append('Actual outside offer passes captured native condition; full native stage writer setsSoftCommitted and applies its native100-point deduction to list and real board Target')
 for value in (0,999):
  r,p,t=prepare(31002+value,value);before=values(r)
  assert native_attempt(r)==1 and values(r)==before and records[r]['scalar'][9]==3
 checks.append('Zero/below-threshold real influence cannot borrow displayed leader points; every original pair is restored exactly')
 r,p,t=prepare(31004);extra_condition=False;before=values(r)
 assert native_attempt(r)==1 and values(r)==before;extra_condition=True
 checks.append('Other native expression rejection remains authoritative for outside-list offers')
 r,p,t=prepare(31005);withdraw_during_condition=True;before=values(r)
 assert native_attempt(r)==1 and values(r)==before;withdraw_during_condition=False
 checks.append('Offer withdrawn during expression dispatch cannot commit; temporary list fully restored')
 r,p,t=prepare(31006);records[allteams[13]]['ref'][407]=record('actual user')
 assert native_attempt(r)==0 and values(r)[0]==(13,1100)
 stage_write(r,4);assert records[r]['scalar'][9]==4
 checks.append('Native-qualified actual user pursuit remains selectable outside the displayed list')
# Execute the new original-condition boundary, actual native false branch,
# existing post-condition ownership trampoline and full native stage writer.
start,end=0x081b98d6,0x081b9928
map_game(start,0x200);uc.mem_write(GAME+start,raw[start-0x1000:end-0x1000])
assert raw[start-0x1000:0x081b98e5-0x1000].hex()=='4889442430488d4c2430e82be04600'
if not negative:
 condition_hook=call(exports['DriTest31ConditionHook'])
 offsets=[];at=0
 while (at:=image.find(prefix,at))>=0:offsets.append(at);at+=1
 ownership_hook=BASE+offsets[16]
 uc.mem_write(GAME+start,b'\xff\x25\0\0\0\0'+struct.pack('<Q',condition_hook)+b'\x90')
 uc.mem_write(GAME+0x081b98ed,b'\xff\x25\0\0\0\0'+struct.pack('<Q',ownership_hook)+b'\x90')
uc.ctl_remove_cache(GAME+start,GAME+0x081b9a02)
def boundary_stop(u,address,size,data):
 if address in (GAME+0x081b9928,GAME+0x081b9a01):u.emu_stop()
stop_hook=uc.hook_add(unicorn.UC_HOOK_CODE,boundary_stop)
def full_attempt(r):
 bp=STACK+0xe0000;sp=STACK+0xd0000;transition=record('native boundary soft transition',{1:4})
 wq(bp+0x68,r);wq(bp+0x60,context);wq(sp+0x38,transition);wq(sp+0x40,0xabcdef)
 for regname,value in [('RSP',sp),('RBP',bp),('RAX',GAME+0x0bd2eea0),('RDX',bp+0x68),('R8',context+0xd0)]:
  uc.reg_write(globals()['UC_X86_REG_'+regname],value)
 try:uc.emu_start(GAME+start,STOP+1,count=250000000)
 except unicorn.UcError:
  print('BOUNDARY_FAULT',hex(reg('RIP')),list(recent),{k:hex(reg(k)) for k in ['RSP','RBP','RCX','RDX','R8','RAX']});raise
 assert reg('RIP') in (GAME+0x081b9928,GAME+0x081b9a01),hex(reg('RIP'))
 return reg('RIP')==GAME+0x081b9928
def low_displayed(r):
 for e,value in zip(records[records[r]['ref'][13]]['array'],[300,200,100]):records[e]['scalar'][1]=value
r,p,t=prepare(31020);low_displayed(r);before=values(r);n=len(evaluations)
if negative:
 assert not full_attempt(r) and records[r]['scalar'][9]==3 and values(r)==before
 assert evaluations[n:]==[(31020,10,300)]
 checks.append('Test30 negative full native branch: displayed leader300 fails, so outside funded1100 is never evaluated and stays unsigned')
else:
 assert full_attempt(r) and records[r]['scalar'][9]==4
 assert evaluations[n:]==[(31020,10,300),(31020,13,1100)],evaluations[n:]
 assert values(r)==[(13,1000),(10,200),(11,100)]
 checks.append('Full native branch: original leader300 fails, actual outside1100 passes the same expression; existing ownership hook and native stage/influence writer then complete soft commitment')
 r,p,t=prepare(31021,999);low_displayed(r);before=values(r)
 assert not full_attempt(r) and values(r)==before and records[r]['scalar'][9]==3
 r,p,t=prepare(31022);low_displayed(r);before=values(r);extra_condition=False
 assert not full_attempt(r) and values(r)==before and records[r]['scalar'][9]==3;extra_condition=True
 checks.append('Actual false branch is retained when outside influence or any other native expression requirement fails; original pairs are restored exactly')
 r,p,t=prepare(31023);low_displayed(r);before=values(r);withdraw_during_condition=True
 assert not full_attempt(r) and values(r)==before and records[r]['scalar'][9]==3;withdraw_during_condition=False
 checks.append('Withdrawal during outside evaluation preserves native false branch and restores original list')
 r,p,t=prepare(31024);low_displayed(r);records[allteams[13]]['ref'][407]=record('user outside boundary')
 assert full_attempt(r) and records[r]['scalar'][9]==4
 checks.append('A real qualified user offer outside a low-influence displayed list can complete the same native transition')
 for m in [0,-1]:
  r,p,t=prepare(31025+m);low_displayed(r);before=values(r);configure(m)
  assert not full_attempt(r) and values(r)==before and records[r]['scalar'][9]==3
 configure(1)
 checks.append('Diagnostic and disabled modes leave the original native false decision unchanged')
if not negative:
 # Build the numeric index before the new scholarship exists, as PRE_PLAN
 # does before native funding. Authorized action only invalidates the index;
 # the modeled native status publication must still happen to create an offer.
 def action_frame(target,tid=13):
  f=alloc(0x200);th=alloc(8);rh=alloc(8);wq(th,allteams[tid]);wq(rh,target)
  wq(f+104,manager);wq(f+96,th);wq(f+56,rh);wi(f+128+0x28,3);return f
 for human in [False,True]:
  r,p,t=prepare(31040+human);records[t]['scalar'][12]=1;records[r]['scalar'][14]=0
  if human:records[allteams[13]]['ref'][407]=record('new actual user scholarship')
  else:
   # A new normal CPU pursuit must be viable in the frozen market too.
   # The separate user/post-funding cases retain an outside-list destination.
   first=records[records[r]['ref'][13]]['array'][0]
   records[first]['scalar'].update({0:13,1:1100})
   for tid in range(138):records[records[allteams[tid]]['ref'][224]]['scalar'][3]=1500 if tid==13 else 0
   call(exports['DriTestHs32Reset']);call(exports['DriTestHs32Begin'],context)
   assert call(exports['DriTestHs30Authorized'],r,13)==1
  assert native_attempt(r)==1 # cached snapshot excludes the unoffered Target
  call(exports['DriTest31OfferRegister'],context)
  assert call(exports['DriTest31OfferAction'],action_frame(t))==0
  assert records[t]['scalar'][12]==1 and records[r]['scalar'][14]==0
  records[t]['scalar'][12]=3;records[r]['scalar'][14]=1 # modeled native funded publication
  assert call(exports['DriTestHs29Offer'],r,allteams[13],context)==2
  if human:low_displayed(r)
  assert full_attempt(r) and records[r]['scalar'][9]==4
 checks.append('Index built before new offer: both authorized CPU action and exempt user action expose the subsequently native-published scholarship in the same week, without a reload or fabricated offer')
 # The existing post-funding boundary also invalidates a snapshot when native
 # funding has changed actual boards; its real observer callback still runs.
 r,p,t=prepare(31042);records[t]['scalar'][12]=1;records[r]['scalar'][14]=0
 assert native_attempt(r)==1
 records[t]['scalar'][12]=3;records[r]['scalar'][14]=1
 names=re.findall(r'^\.global\s+(Hook\w+)',(ROOT/'hooks.S').read_text(),re.M)
 wrappers={'HookEpsGrade','HookEpsGeneric','HookEpsRequired','HookEpsUpdate','HookStaff28Update','HookPt28Grade','HookProximity28Grade','HookPPGroupGrade'}
 names=[n for n in names if n not in wrappers]
 post=alloc(128);post_bp=alloc(256)+128;holder=alloc(8);wq(holder,allteams[13]);wq(post_bp-0x60,holder)
 wq(post+72,context);wq(post+80,post_bp)
 call(hooks[names.index('HookHsPostFunding')],post)
 assert call(exports['DriTestQualityFault'])==0
 low_displayed(r);assert full_attempt(r) and records[r]['scalar'][9]==4
 checks.append('Actual compiled post-funding callback invalidates a pre-funding offer index; newly funded outsider can pass the native false-condition boundary immediately')
uc.hook_del(stop_hook)
call(exports['DriTestHs29Reset']);call(exports['DriTestOwnershipReset'])
assert call(exports['DriTestQualityFault'])==0
assert all(v==1 for v in refs.values()),[(records[k]['name'],v) for k,v in refs.items() if v!=1]
report={'passed':True,'negative_control':negative,'checks':checks,'native_executed':['08627910 condition evaluator','0814c3c0 full stage writer','081b2730 native influence adjustment'],'controlled_expression_leaf':True,'live_game_verified':False}
report.update(fixture_sha256=hashlib.sha256((OUTPUT/'retention_fixture.dll').read_bytes()).hexdigest(),capture_code_sha256=hashlib.sha256(raw).hexdigest())
(OUTPUT/'test31_weekly_destination_results.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
