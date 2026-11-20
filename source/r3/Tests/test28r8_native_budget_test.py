import gc
"""Execute captured native remaining-hours helper with mocked record access.

Proves that processed hours are not subtracted twice from assigned hours.
No DLL or live game is run; field accessor services return controlled records.
"""
from pathlib import Path
import json, os, struct
import unicorn
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = Path(os.environ.get('DRI_TEST_OUTPUT_DIR', ROOT/'build'))
CAPTURE = Path(os.environ['DRI_CAPTURE_DIR'])
raw = (CAPTURE/'section_00_00001000.bin').read_bytes()
GAME, STACK, HEAP, API = 0x140000000, 0x20000000, 0x30000000, 0x40000000
ENTRY, END = 0x08628f30, 0x08629001
results = []
for total, assigned, processed in [(1500,0,0),(1500,5,5),(1500,500,75),(1500,1490,75),(1500,1500,1500),(0,0,0),(100,101,0)]:
    u = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_64)
    for address, size in [(GAME+0x08628000,0x2000),(GAME+0x08055000,0x1000),(GAME+0x037f4000,0x1000),(STACK,0x10000),(HEAP,0x10000),(API,0x1000)]:
        u.mem_map(address,size)
    u.mem_write(GAME+ENTRY,raw[ENTRY-0x1000:END-0x1000])
    for callback in [GAME+0x08055d00,GAME+0x037f4530,API,API+0x100]:u.mem_write(callback,b"\xc3")
    q=lambda address,value:u.mem_write(address,struct.pack('<Q',value))
    wrapper, record, schema, store, vtable = [HEAP+i*0x100 for i in range(1,6)]
    q(wrapper+8,record);q(wrapper+16,schema);q(record+16,store);q(store,vtable);q(vtable+0x100,API)
    sp=STACK+0x8008; q(sp,API+0x100);u.reg_write(UC_X86_REG_RSP,sp);u.reg_write(UC_X86_REG_RCX,wrapper)
    fields={1:assigned,2:processed,3:total};reads=[];logged=[]
    def hook(uc,address,size,user):
        if address==API+0x100:uc.emu_stop();return
        value=None
        if address==GAME+0x08055d00:
            assert uc.reg_read(UC_X86_REG_RCX)==schema
            value=uc.reg_read(UC_X86_REG_EDX)
        elif address==API:
            field=uc.reg_read(UC_X86_REG_R8);reads.append(field);value=fields[field]
        elif address==GAME+0x037f4530:logged.append(True);value=0
        if value is not None:
            rsp=uc.reg_read(UC_X86_REG_RSP);ret=struct.unpack('<Q',uc.mem_read(rsp,8))[0]
            uc.reg_write(UC_X86_REG_RAX,value);uc.reg_write(UC_X86_REG_RSP,rsp+8);uc.reg_write(UC_X86_REG_RIP,ret)
    emulator_hook=u.hook_add(unicorn.UC_HOOK_CODE,hook)
    try:u.emu_start(GAME+ENTRY,API+0x101,count=500)
    except unicorn.UcError:
        raise AssertionError((hex(u.reg_read(UC_X86_REG_RIP)), reads, total, assigned))
    actual=u.reg_read(UC_X86_REG_EAX);expected=max(0,total-assigned)
    assert u.reg_read(UC_X86_REG_RIP)==API+0x100 and actual==expected
    assert reads==[3,1] and bool(logged)==(assigned>total)
    results.append({'total':total,'assigned':assigned,'processed':processed,'remaining':actual,'reads':reads,'passed':True})
    u.hook_del(emulator_hook);del u;gc.collect()
OUTPUT.mkdir(parents=True,exist_ok=True)
(OUTPUT/'test28r8_native_budget_results.json').write_text(json.dumps({'helperRVA':hex(ENTRY),'cases':results,'formula':'max(0, total - assigned); processed is not separately subtracted','live_game_verified':False},indent=2)+'\n')
print('PASS captured native remaining-hours helper: seven budget cases; processed hours never read or double-counted')
