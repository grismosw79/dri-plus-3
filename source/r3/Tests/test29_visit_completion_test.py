"""Native visit readiness, scheduled-date completion and influence write.
Captured completion+apply+target writer execute with compiled observation hooks.
Calendar service, zero coach modifiers, base visit influence and SDF services
are controlled fixtures. No live completion or unchanged numeric season claim.
"""
from pathlib import Path
src=(Path(__file__).parent/'runtime_fixture_test.py').read_text()
exec(compile(src.split("prefix=bytes.fromhex")[0],'visit completion fixture','exec'))
import hashlib
assert hashlib.sha256(raw).hexdigest()=='7690a69b26b19a483f847abd6c7e12c9fece11717a5546fa6f467bcc8a48016e'

def map_game(rva,size=0x1000):
 for page in range(GAME+(rva&~4095),(GAME+rva+size+4095)&~4095,4096):
  if not any(lo<=page<=hi for lo,hi,_ in uc.mem_regions()):uc.mem_map(page,4096)
def capture(lo,hi):map_game(lo,hi-lo);uc.mem_write(GAME+lo,raw[lo-0x1000:hi-0x1000])
def game_api(rva,fn):map_game(rva,1);api(fn,GAME+rva)
def patch(rva,n,entry):uc.mem_write(GAME+rva,b'\xff\x25'+bytes(4)+struct.pack('<Q',entry)+b'\x90'*(n-14));uc.ctl_remove_cache(GAME+rva,GAME+rva+n)
def configure(mode):call(exports['DriTestConfigure'],GAME,mode,7)
configure(1);hp=alloc(16);call(exports['DriTestVisit29ObserveHooks'],hp);eligible_hook,completed_hook=rq(hp),rq(hp+8)
observed=alloc(28)
def observations():call(exports['DriTestVisit29Observation'],observed);return [ri(observed+n*4)for n in range(7)]
def reset_observations():call(exports['DriTestVisit29ObservationReset'])
coach=record('coach');team=record('school',{383:67},ref={32:coach});recruit=record('HS recruit',{9:1});visit=record('ActiveVisitInfo',{1:5,2:1})
target=record('target',{6:40,7:0,8:200,12:3},ref={10:recruit})
team_ref=alloc(8);target_ref=alloc(8);wq(team_ref,team);wq(target_ref,target)
wrapper=alloc(32);wq(wrapper+8,target);wq(wrapper+16,0xabcdef)
readiness=[]
for stage,offer,booked in [(0,3,False),(1,3,False),(2,3,False),(3,3,False),(4,3,False),(5,3,False),(6,3,False),(1,0,False),(1,1,False),(1,2,False),(1,4,False),(1,3,True)]:
 records[recruit]['scalar'][9]=stage;records[target]['scalar'][12]=offer
 records[target]['ref'][11]=visit if booked else 0
 actual=call(GAME+0x862b3f0,wrapper)&255;expected=int(1<=stage<=4 and 2<=offer<=4 and not booked)
 assert actual==expected and all(v==1 for v in refs.values())
 readiness.append({'stage':stage,'offer_status':offer,'already_booked':booked,'native_ready':bool(actual)})

# The CPU eligibility observer preserves both native branches before any cost
# query or legal-calendar search. "First" means earliest observed CSV event.
capture(0x8177ad4,0x8177ae3);patch(0x8177ad4,15,eligible_hook)
for rva in (0x8177ae3,0x8177c16):map_game(rva,1);uc.mem_write(GAME+rva,b'\xc3')
eval_ptr=alloc(0x400);manager=alloc(0x300);wq(eval_ptr+0xc8,manager);wi(eval_ptr+0x1a4,2);wi(eval_ptr+0x1a8,1)
bp=alloc(0x200)+0x80;wi(bp+0x60,39);stops=[]
def stop_eligible(u,address,size,data):
 if address in (GAME+0x8177ae3,GAME+0x8177c16):stops.append(address-GAME);u.emu_stop()
eh=uc.hook_add(unicorn.UC_HOOK_CODE,stop_eligible)
for allowed in [0,1]:
 reset_observations();stops.clear();before={r:dict(x['scalar'])for r,x in records.items()}
 for name,value in [('RSP',STACK+0xd0000),('RBP',bp),('RBX',eval_ptr),('RSI',target_ref),('R15',team_ref),('RAX',allowed)]:uc.reg_write(globals()['UC_X86_REG_'+name],value)
 uc.emu_start(GAME+0x8177ad4,STOP+1,count=10000)
 assert stops==[0x8177ae3 if allowed else 0x8177c16] and reg('RAX')==allowed
 assert observations()[0]==allowed and before=={r:dict(x['scalar'])for r,x in records.items()}
uc.hook_del(eh)

# Actual native scheduled visit completion and influence application. The
# native VisitEval numerical base calculation is modeled; no native decision
# about date matching, final influence write/clamp or signed/locked refusal is.
capture(0x81a3890,0x81a40cc);capture(0x8147c10,0x81485e2)
patch(0x81a3fac,14,completed_hook)
tuning=record('tunables');wq(manager+0x188,tuning);wq(eval_ptr+0x88,alloc(0x200));wq(eval_ptr+0x90,alloc(0x200));wq(eval_ptr+0xe0,0)
player=record('player');records[recruit]['ref'][5]=player
zero=record('empty coach reference');records[team]['ref'][111]=zero
writes=[];feedback=[];locked=False;base_gain=150;base_calls=[]
def setter():
 store,idx,f,value=args();r=tables[store][idx];field=f-0x100000
 records[r]['scalar'][field]=value&0xffffffff;writes.append((r,field,value&0xffffffff));return 0
wq(TABLEVT+0x148,api(setter));game_api(0x8059580,lambda:0)
def prior_date():
 out=reg('RDX');wq(out+8,0);wi(out+16,8);return out
def action_array():out=reg('RDX');wq(out,0);return out
def influence():
 sp=reg('RSP');base_calls.append((ri(eval_ptr+0x1a8),ri(eval_ptr+0x1a4)))
 wi(rq(sp+0x40),min(base_gain,0));wi(rq(sp+0x48),max(base_gain,0));wi(rq(sp+0x50),base_gain);return 0
def feedback_fn():
 sp=reg('RSP');m,tar,action,delta=args();assert m==manager and tar==target_ref and action==10
 feedback.append({'input_gain':delta,'cost_argument':ri(sp+0x40),'category':ri(sp+0x38)});return 0
for addr,fn in [(0x830bc10,prior_date),(0x830cbf0,prior_date),(0x81aef90,lambda:0),(0x844a120,lambda:0),(0x8165c30,action_array),(0x84483c0,influence),(0x8183410,lambda:0),(0x82a2690,lambda:0),(0x82a2810,lambda:0),(0x82d1dd0,lambda:1),(0x819a320,lambda:0),(0x862cf00,lambda:1),(0x814a190,feedback_fn),(0x862d990,lambda:int(locked))]:game_api(addr,fn)
def avx_zero(u,address,size,data):
 if address==GAME+0x81a3b03:u.reg_write(UC_X86_REG_XMM0,0);u.reg_write(UC_X86_REG_RIP,address+4)
 elif address==GAME+0x81a3b18:u.mem_write(reg('RBP')-0x40,bytes(16));u.reg_write(UC_X86_REG_RIP,address+5)
ah=uc.hook_add(unicorn.UC_HOOK_CODE,avx_zero)
# Check original direct branch targets do not enter either replacement span.
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
for lo,hi,span,n in [(0x8177900,0x8177eac,0x8177ad4,15),(0x81a3890,0x81a40cc,0x81a3fac,14)]:
 for ins in cs.disasm(raw[lo-0x1000:hi-0x1000],lo):
  if (ins.mnemonic.startswith('j') or ins.mnemonic=='call') and ins.operands and ins.operands[0].type==capstone.x86.X86_OP_IMM:assert not span<ins.operands[0].imm<span+n

completion=[]
for label,week,kind,scheduled_week,scheduled_type,stage,is_locked,gain,start,expect_apply,expected in [
 ('scheduled day',5,1,5,1,1,False,150,200,True,350),
 ('future visit',4,1,5,1,1,False,150,200,False,200),
 ('past date',6,1,5,1,1,False,150,200,False,200),
 ('different type',5,6,5,1,1,False,150,200,False,200),
 ('offseason matched',3,6,3,6,1,False,150,200,True,350),
 ('hard commitment',5,1,5,1,5,False,150,200,False,200),
 ('signed',5,1,5,1,6,False,150,200,False,200),
 ('native lockout',5,1,5,1,1,True,150,200,False,200),
 ('upper influence clamp',5,1,5,1,1,False,500,900,True,1000),
 ('zero native award',5,1,5,1,1,False,0,200,True,200)]:
 locked=is_locked;base_gain=gain;wi(eval_ptr+0x1a4,week);wi(eval_ptr+0x1a8,kind)
 records[visit]['scalar'].update({1:scheduled_week,2:scheduled_type});records[target]['ref'][11]=visit
 records[recruit]['scalar'][9]=stage;records[target]['scalar'].update({6:40,7:0,8:start,12:3})
 outcomes=[]
 for enabled in [False,True]:
  records[target]['scalar'][8]=start;writes.clear();feedback.clear();base_calls.clear();reset_observations()
  if enabled:patch(0x81a3fac,14,completed_hook)
  else:uc.mem_write(GAME+0x81a3fac,raw[0x81a3fac-0x1000:0x81a3fba-0x1000]);uc.ctl_remove_cache(GAME+0x81a3fac,GAME+0x81a3fba)
  result=call(GAME+0x81a3890,eval_ptr,team_ref,target_ref);obs=observations()
  assert records[target]['scalar'][8]==expected,(label,records[target]['scalar'],obs)
  assert records[target]['scalar'][6]==40 and records[target]['scalar'][7]==0
  assert bool(writes)==expect_apply and all(r==target and f==8 for r,f,v in writes)
  assert bool(feedback)==expect_apply and all(x['cost_argument']==0 and x['category']==4 for x in feedback)
  matched=(week==scheduled_week and kind==scheduled_type)
  assert bool(base_calls)==matched
  assert obs[1]==int(enabled and matched) and (not obs[1] or (obs[2]==int(expect_apply) and obs[4]==expected))
  assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
  outcomes.append((result,list(writes),list(feedback),records[target]['scalar'].copy()))
 assert outcomes[0]==outcomes[1],('observation changed native result',label)
 completion.append({'case':label,'scheduled_date_matched':matched,'native_influence_written':expect_apply,'resulting_influence':expected,'target_field6_unchanged':True,'native_feedback_cost_argument':0,'observational_equivalence':True})
uc.hook_del(ah)
assert call(exports['DriTestQualityFault'])==0
(OUTPUT/'test29_visit_completion_results.json').write_text(json.dumps({'native_readiness':readiness,'completion':completion,'compiled_observation_hooks':True,'captured_completion':'0x081a3890','captured_apply':'0x08147c10','captured_influence_writer':'0x08148340','modeled_services':['SDF storage','calendar service','base VisitEval influence','zero coach modifiers','feedback storage'],'live_game_verified':False},indent=2)+'\n')
print('PASS Test29 native visit eligibility/date matching/completion/actual influence setter, read-only compiled observations, unchanged target field6 and zero completion feedback cost')
