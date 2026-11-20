"""Captured visit capacity + CPU selector with actual compiled Test29 hooks.
Native record storage/calendar candidate creation/booking services are fixtures;
selector branches, coach addition, hook ABI and post-success cost are native.
No live game, UI rendering or completed visit influence claim is made.
"""
from pathlib import Path
src=(Path(__file__).parent/'runtime_fixture_test.py').read_text()
exec(compile(src.split("prefix=bytes.fromhex")[0],'visit fixture','exec'))
import hashlib
assert hashlib.sha256(raw).hexdigest()=='7690a69b26b19a483f847abd6c7e12c9fece11717a5546fa6f467bcc8a48016e'
assert 'DriTestVisit29Hooks' in exports

def map_game(rva,size=0x1000):
    for page in range(GAME+(rva&~0xfff),(GAME+rva+size+4095)&~4095,4096):
        if not any(lo<=page<=hi for lo,hi,_ in uc.mem_regions()):uc.mem_map(page,4096)
def game_api(rva,fn):map_game(rva,1);api(fn,GAME+rva)
def capture(lo,hi):
    map_game(lo,hi-lo);uc.mem_write(GAME+lo,raw[lo-0x1000:hi-0x1000])
def patch(rva,size,entry):
    uc.mem_write(GAME+rva,b'\xff\x25'+b'\0'*4+struct.pack('<Q',entry)+b'\x90'*(size-14));uc.ctl_remove_cache(GAME+rva,GAME+rva+size)
def configure(mode):call(exports['DriTestConfigure'],GAME,mode,7)
configure(1)
addresses=alloc(24);call(exports['DriTestVisit29Hooks'],addresses);capacity_hook,dates_hook,booked_hook=[rq(addresses+n*8)for n in range(3)]
capture(0x844a2c0,0x844a499)
visit_eval=alloc(0x200);tuning=record('visit tunables',{31:4});coach=record('coach effects');team=record('visit team',{383:67},ref={32:coach});holder=alloc(8);wq(holder,team);wq(visit_eval+0x70,tuning)
bonus=0;coach_reads=[]
def coach_effect():
    wrapper,effect,_,_=args();assert rq(wrapper+8)==coach and effect==0x91;coach_reads.append(effect);return bonus
game_api(0x82a2810,coach_effect)
capacity=[]
for base,add in [(4,0),(4,2),(4,4),(10,0),(10,1),(10,2),(10,4),(10,20),(10,100)]:
    records[tuning]['scalar'][31]=base;bonus=add
    # Exact native getter before intervention, including coach effect addition.
    uc.mem_write(GAME+0x844a482,raw[0x844a482-0x1000:0x844a491-0x1000]);uc.ctl_remove_cache(GAME+0x844a482,GAME+0x844a491)
    configure(1);native=call(GAME+0x844a2c0,visit_eval,holder);assert native==base+add
    patch(0x844a482,15,capacity_hook);active=call(GAME+0x844a2c0,visit_eval,holder);assert active==min(10,base+add)
    configure(0);diagnostic=call(GAME+0x844a2c0,visit_eval,holder);assert diagnostic==native
    capacity.append({'base':base,'coach_bonus':add,'native':native,'active':active,'diagnostic':diagnostic})
assert len(coach_reads)==27 and all(v==1 for v in refs.values())

# Execute the complete captured home-game + parallel-capacity admission helper.
# Its temporary vector allocator/copy/destructor are services, but native code
# performs game-home identity, week/type filtering, growth and final count<cap.
capture(0x8448d00,0x844902d)
records[tuning]['scalar'][31]=10;bonus=4;configure(1)
uc.mem_write(visit_eval+0x1d0,b'\1')
node=alloc(0x60);entries_mem=alloc(12*48);wq(visit_eval+0x180,node);wi(node+0x20,ri(team+0x18));wq(node+0x28,entries_mem)
game=record('season game',ref={42:team});game_holder=alloc(8);wq(game_holder,game)
visited_grows=[];assertions=[]
def wrap_game():
    out,holder_arg,_,_=args();r=rq(holder_arg);incref(r);wq(out,0);wq(out+8,r);wq(out+16,0xabcdef);return out
def grow_visit_vector():
    vec,source,_,_=args();begin,end,limit=rq(vec),rq(vec+8),rq(vec+16);used=(end-begin)//48
    storage=alloc((used+1)*48)
    if used:uc.mem_write(storage,bytes(uc.mem_read(begin,used*48)))
    uc.mem_write(storage+used*48,bytes(uc.mem_read(source,48)))
    wq(vec,storage);wq(vec+8,storage+(used+1)*48);wq(vec+16,storage+(used+1)*48);visited_grows.append(used+1);return storage

def copy_visit():
    dest,source,_,_=args();uc.mem_write(dest,bytes(uc.mem_read(source,48)));return dest
game_api(0x85bd780,wrap_game);game_api(0x8057470,lambda:ri(reg('RCX')+0x18));game_api(0x8444b00,grow_visit_vector);game_api(0x8445650,copy_visit);game_api(0x19ea6a0,lambda:0);game_api(0x37f4530,lambda:(assertions.append(True) or 0))
def avx_visit(u,address,size,data):
    if address==GAME+0x8448e80:u.reg_write(UC_X86_REG_XMM0,0);u.reg_write(UC_X86_REG_RIP,address+4)
    elif address==GAME+0x8448e84:u.mem_write(reg('RBP')-0x20,bytes(32));u.reg_write(UC_X86_REG_RIP,address+5)
    elif address in (GAME+0x8448e8e,GAME+0x8448ea1):u.reg_write(UC_X86_REG_RIP,address+3)
avx_handle=uc.hook_add(unicorn.UC_HOOK_CODE,avx_visit)
admission=[]
for count,home in [(0,True),(4,True),(8,True),(9,True),(10,True),(11,True),(9,False)]:
    visited_grows.clear();assertions.clear();wq(node+0x30,entries_mem+count*48)
    for n in range(count):uc.mem_write(entries_mem+n*48,struct.pack('<QQQiiQQ',0,0,0,5,1,0,0))
    records[game]['ref'][42]=team if home else coach
    rsp=STACK+0xf0000-8;wq(rsp,STOP);wq(rsp+0x28,game_holder);wq(rsp+0x30,0)
    for name,val in [('RSP',rsp),('RCX',visit_eval),('RDX',holder),('R8',5),('R9',1)]:uc.reg_write(globals()['UC_X86_REG_'+name],val)
    uc.emu_start(GAME+0x8448d00,STOP+1,count=100000)
    assert reg('RIP')==STOP
    allowed=reg('RAX')&255;assert allowed==int(home and count<10)
    assert len(visited_grows)==(count if home else 0) and bool(assertions)==(home and count>10)
    admission.append({'existing_visitors':count,'home':home,'admit_next':bool(allowed),'dynamic_vector_growth':list(visited_grows)})
assert all(v==1 for v in refs.values())
uc.hook_del(avx_handle)

# Actual CPU pre-booking conditional branches still reject unaffordable or
# ineligible visits before calling legal-date enumeration. Native eligibility,
# cost and readiness services return controlled values; no policy bypass.
capture(0x8177aac,0x8177b27)
gate_eval=alloc(0x400);gate_manager=alloc(8);wq(gate_eval+0xc8,gate_manager)
gate_bp=alloc(0x100)+0x40;gate_allowed=True;gate_school=True;gate_ready=True;gate_events=[]
def gate_service(name,value):
    gate_events.append(name);return int(value)
game_api(0x819adf0,lambda:gate_service('native action eligibility',gate_allowed))
game_api(0x817da90,lambda:gate_service('native discounted visit cost',40))
game_api(0x816b870,lambda:gate_service('native school remaining hours',gate_school))
game_api(0x815f050,lambda:gate_service('native readiness',gate_ready))
gate_stops=[]
def stop_gate(u,address,size,data):
    if address in (GAME+0x8177b27,GAME+0x8177c16,GAME+0x8177c1a):gate_stops.append(address);u.emu_stop()
map_game(0x8177c10,0x100)
for rva in (0x8177b27,0x8177c16,0x8177c1a):uc.mem_write(GAME+rva,b'\xc3')
gh=uc.hook_add(unicorn.UC_HOOK_CODE,stop_gate)
gates=[]
for available,gate_allowed,gate_school,gate_ready,expected in [(39,True,True,True,False),(40,True,True,True,True),(100,False,True,True,False),(100,True,False,True,False),(100,True,True,False,False)]:
    gate_events.clear();gate_stops.clear();wi(gate_bp+0x60,available)
    for name,val in [('RSP',STACK+0xc0000),('RBP',gate_bp),('RBX',gate_eval),('RAX',0),('R15',holder),('RSI',holder),('RDI',0)]:uc.reg_write(globals()['UC_X86_REG_'+name],val)
    try:uc.emu_start(GAME+0x8177aac,STOP+1,count=2000)
    except unicorn.UcError:
        print('gate fault',hex(reg('RIP')),list(recent),gate_events);raise
    reached=gate_stops[-1]==GAME+0x8177b27;assert reached==expected
    gates.append({'available_hours':available,'native_action_allowed':gate_allowed,'native_school_hours_allowed':gate_school,'native_readiness':gate_ready,'reaches_date_enumeration':reached})
uc.hook_del(gh)

capture(0x814d6c0,0x814daef)
patch(0x814d8b1,17,dates_hook);patch(0x814da23,14,booked_hook)
# All direct branch targets in this complete captured function must enter
# outside replaced bytes or at the first byte. The empty vector join is d29.
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
for ins in cs.disasm(raw[0x814d6c0-0x1000:0x814daef-0x1000],0x814d6c0):
    if ins.mnemonic.startswith('j') or ins.mnemonic=='call':
        if ins.operands and ins.operands[0].type==capstone.x86.X86_OP_IMM:
            target=ins.operands[0].imm
            for lo,n in [(0x814d8b1,17),(0x814da23,14)]:assert not lo<target<lo+n,(hex(ins.address),hex(target))
team_ref=holder;target=record('visit target',ref={10:record('visit recruit')});target_ref=alloc(8);wq(target_ref,target)
eval_ptr=alloc(0x400);manager=alloc(0x100);wq(eval_ptr+0xc8,manager)
frame=alloc(0x200)+0x80;vector=alloc(32*24);booking_calls=[];account_calls=[];booking_result=0;cost=40
opponents=[record('opponent '+str(n)) for n in range(32)]
def book():
    m,tr,tar,actions=args();sp=reg('RSP');assert m==manager and tr==team_ref and tar==target_ref
    booking_calls.append({'week':ri(sp+0x28),'type':ri(sp+0x30),'current_type':ri(sp+0x48),'current_week':ri(sp+0x50)})
    return booking_result
def cost_fn():
    m,tr,tar,_=args();assert (m,tr,tar)==(manager,team_ref,target_ref);return cost
def account():
    ev,tar,paid,_=args();assert ev==eval_ptr and tar==target_ref and paid==cost
    account_calls.append(paid);wi(ev+0x230,ri(ev+0x230)+paid);return 0
game_api(0x81ac050,book);game_api(0x817da90,cost_fn);game_api(0x81b84c0,account);game_api(0x37f4530,lambda:0)
stops=[]
def stop_selector(u,address,size,data):
    if address in (GAME+0x814da31,GAME+0x814da39):stops.append(address-GAME);u.emu_stop()
h=uc.hook_add(unicorn.UC_HOOK_CODE,stop_selector)

def select(entries,week=2,kind=1,active=1,result=0):
    global booking_result
    booking_result=result;booking_calls.clear();account_calls.clear();stops.clear();configure(active)
    wi(eval_ptr+0x1a4,week);wi(eval_ptr+0x1a8,kind);wi(eval_ptr+0x258,0);wi(eval_ptr+0x230,0)
    wq(frame-0x11,vector);wq(frame-9,vector+24*len(entries));wq(frame+0x5f,eval_ptr);wq(frame+0x67,team_ref);wq(frame+0x6f,target_ref);wq(frame+0x77,0)
    wi(frame-0x4d,0);wi(frame-0x49,-1);wi(frame-0x29,6 if kind==6 else 1)
    for n,(date,rank,*rest) in enumerate(entries):
        typ=rest[0] if rest else 1
        uc.mem_write(vector+n*24,struct.pack('<QiiiB3x',opponents[n],date,typ,rank,0))
    before=bytes(uc.mem_read(vector,len(entries)*24))
    for name,value in [('RBP',frame),('R15',eval_ptr),('R13',0x7fffffff),('RDI',0),('R12',0),('RSP',STACK+0xd0000)]:uc.reg_write(globals()['UC_X86_REG_'+name],value)
    uc.emu_start(GAME+0x814d8b1,STOP+1,count=100000)
    assert stops and stops[-1] in (0x814da31,0x814da39),list(recent)
    if entries:
        assert len(booking_calls)==1
        chosen=booking_calls[0]['week']
        if result==0:assert account_calls==[cost] and ri(eval_ptr+0x258)==cost and ri(eval_ptr+0x230)==cost
        else:assert not account_calls and ri(eval_ptr+0x258)==0 and ri(eval_ptr+0x230)==0
        # Native selector owns one extra reference for its chosen opponent.
        held=reg('RDI');decref(held)
    else:
        assert not booking_calls and not account_calls and stops[-1]==0x814da39;chosen=None
    after=bytes(uc.mem_read(vector,len(entries)*24))
    for n in range(len(entries)):
        assert before[n*24:n*24+16]==after[n*24:n*24+16] and before[n*24+20:n*24+24]==after[n*24+20:n*24+24]
    assert all(v==1 for v in refs.values()),[(records[r]['name'],v)for r,v in refs.items()if v!=1]
    return chosen

cases=[]
for entries,expected,week,kind,active,result,label in [
 ([(3,80),(5,40),(9,1)],5,2,1,1,0,'best opponent within three weeks'),
 ([(7,80),(9,1)],7,2,1,1,0,'no close date: earliest legal future'),
 ([(3,80),(5,40),(9,1)],9,2,1,0,0,'diagnostic preserves native opponent preference'),
 ([(7,80,6),(9,1,6)],9,6,6,1,0,'offseason selection untouched'),
 ([(3,80),(5,40),(9,1)],5,2,1,1,7,'native booking refusal no cost'),
 ([],None,2,1,1,0,'empty legal vector safe native join'),
 ([(2,80),(9,1)],9,2,1,1,0,'unexpected same-week list fails closed'),
 ([(1,80),(3,40),(9,1)],3,0,0,1,0,'preseason future calendar'),
 ([(3,40),(5,40),(9,1)],3,2,1,1,0,'native stable tied opponent ordering')]:
    actual=select(entries,week,kind,active,result);assert actual==expected,(label,actual,expected);cases.append({'case':label,'selected_week':actual,'passed':True})
uc.hook_del(h)
# Captured UI model publication and week-lock comparison support capacity10.
# This executes its data path only; it is not a rendered UI test.
capture(0x88ab0ba,0x88ab0c8);capture(0x88ab527,0x88ab531)
ui_model=alloc(0x200);ui_stops=[]
for rva in (0x88ab0c8,0x88ab531,0x88ab62f):map_game(rva,1);uc.mem_write(GAME+rva,b'\xc3')
def ui_stop(u,address,size,data):
    if address in (GAME+0x88ab0c8,GAME+0x88ab531,GAME+0x88ab62f):ui_stops.append(address-GAME);u.emu_stop()
uh=uc.hook_add(unicorn.UC_HOOK_CODE,ui_stop)
ui_cases=[]
for count in [8,9,10,11]:
    ui_stops.clear();sp=STACK+0xb0000;wi(sp+0x78,10)
    for name,val in [('RSP',sp),('RBX',ui_model),('RDI',count)]:uc.reg_write(globals()['UC_X86_REG_'+name],val)
    uc.emu_start(GAME+0x88ab0ba,STOP+1,count=50)
    assert uc.mem_read(ui_model+0x12,1)==bytes([count]) and uc.mem_read(ui_model+0x130,1)==b'\x0a'
    uc.emu_start(GAME+0x88ab527,STOP+1,count=50)
    locked=ui_stops[-1]==0x88ab531;assert locked==(count>=10)
    ui_cases.append({'count':count,'published_capacity':10,'week_locked':locked})
uc.hook_del(uh)
assert call(exports['DriTestQualityFault'])==0
OUTPUT.mkdir(parents=True,exist_ok=True)
(OUTPUT/'test29_visit_native_results.json').write_text(json.dumps({'capacity':capacity,'native_admission':admission,'native_cpu_funding_gates':gates,'native_ui_model':ui_cases,'selector_cases':cases,'compiled_hooks':True,'native_machine_code':True,'storage_calendar_and_booking_services':'modeled','native_cost':40,'diagnostic_no_mutation':True,'no_hook_interior_branch_targets':True,'ui_rendering_verified':False,'live_game_verified':False},indent=2)+'\n')
print('PASS Test29 captured native visit capacity/coach bonuses + compiled hook selector, empty/refused/offseason paths and native post-success cost')
