#!/usr/bin/env python3
"""Captured native generator selection boundaries, not a full game simulation.

Usage: PYTHONPATH=DEPS python3 test29_supply_native.py NATIVE_CAPTURE [OUTPUT]
The original code section hash is mandatory. The tests stub record access and
the RNG result; the root/list/generator chain and weighted-selection branch are
the captured instructions. No generated Player records are edited.
"""
import hashlib, json, pathlib, struct, sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_64, UC_HOOK_CODE
from unicorn.x86_const import *

CAPTURE_HASH = '7690a69b26b19a483f847abd6c7e12c9fece11717a5546fa6f467bcc8a48016e'
code = (pathlib.Path(sys.argv[1]) / 'section_00_00001000.bin').read_bytes()
assert hashlib.sha256(code).hexdigest() == CAPTURE_HASH, 'Unsupported native capture'

def machine():
    u=Uc(UC_ARCH_X86, UC_MODE_64)
    u.mem_map(0x8103000,0x101000)
    u.mem_write(0x8103d10,code[0x8103d10-0x1000:0x8203d10-0x1000])
    u.mem_map(0x10000000,0x20000)
    u.reg_write(UC_X86_REG_RSP,0x10018000)
    return u
def q(u,a,v):u.mem_write(a,struct.pack('<Q',v))
def d(u,a,v):u.mem_write(a,struct.pack('<I',v))
def rq(u,a):return struct.unpack('<Q',u.mem_read(a,8))[0]
def rr(u,r):return u.reg_read(r)

def root_chain(index,count):
    u=machine(); manager=0x10001000; root=0x10004000; array=0x10005000
    tuning=0x10006000+index*0x100; generator=0x10007000+index*0x100
    vtable=0x10008000; table=0x10009000; stack=rr(u,UC_X86_REG_RSP)
    for obj in [root,array,tuning,generator]:q(u,obj,vtable);q(u,obj+0x10,table)
    q(u,table,vtable);q(u,manager+0x11e0,root);q(u,manager+0x1280,index)
    u.reg_write(UC_X86_REG_RSI,manager);u.reg_write(UC_X86_REG_R12,0)
    fields=[]
    noop={0x817b0ee:2,0x817b15d:3,0x817b170:3,0x817b1a6:2,0x817b209:3,0x817b21c:3}
    schemas={0x817b0fa:3,0x817b1b3:3}
    def hook(u,a,size,data):
        if a in noop:u.reg_write(UC_X86_REG_RIP,a+noop[a])
        elif a in schemas:u.reg_write(UC_X86_REG_RAX,0x1000a000);u.reg_write(UC_X86_REG_RIP,a+schemas[a])
        elif a in [0x817b105,0x817b1bb]:
            field=rr(u,UC_X86_REG_RDX);fields.append(field)
            u.reg_write(UC_X86_REG_RAX,field);u.reg_write(UC_X86_REG_RIP,a+5)
        elif a==0x817b120:
            assert rr(u,UC_X86_REG_R9)==2
            q(u,rr(u,UC_X86_REG_RDX),array);u.reg_write(UC_X86_REG_RIP,a+7)
        elif a==0x817b147:
            assert rr(u,UC_X86_REG_R8)==0x41525259
            out=rr(u,UC_X86_REG_RDX);q(u,out,array)
            u.reg_write(UC_X86_REG_RAX,out);u.reg_write(UC_X86_REG_RIP,a+3)
        elif a==0x817b18d:
            assert rr(u,UC_X86_REG_R9)==index
            q(u,rr(u,UC_X86_REG_RDX),tuning);u.reg_write(UC_X86_REG_RIP,a+6)
        elif a==0x817b1d6:
            assert rr(u,UC_X86_REG_R9)==0
            q(u,rr(u,UC_X86_REG_RDX),generator);u.reg_write(UC_X86_REG_RIP,a+7)
    h=u.hook_add(UC_HOOK_CODE,hook)
    u.emu_start(0x817b0d1,0x817b21f,count=300)
    assert fields==[2,0] and rq(u,manager+0x11e8)==generator
    u.hook_del(h)
    u.reg_write(UC_X86_REG_R14,count);u.reg_write(UC_X86_REG_R15,manager+0x1230)
    q(u,manager+0x68,0x1000b000);q(u,manager+0x11a0,0x1000c000)
    u.emu_start(0x817b2f4,0x817b32f,count=30)
    assert rr(u,UC_X86_REG_R8)==manager+0x11e8
    assert rq(u,rr(u,UC_X86_REG_R8))==generator
    assert struct.unpack('<I',u.mem_read(stack+0x20,4))[0]==count
    assert rq(u,stack+0x28)==manager+0x1210 and rq(u,stack+0x30)==manager+0x1230
    return {'generator_index':index,'requested_batch':count,'root_field':2,'tuning_generator_field':0}

def weighted_selection(weights):
    assert len(weights)>1 and all(w>=0 for w in weights) and sum(weights)>0
    u=machine();frame=0x1000c000;cdf=0x10002000;entries=0x10004000;total=0
    for i,w in enumerate(weights):
        total+=w;d(u,cdf+i*4,total);q(u,entries+i*16,0x10008000+i*0x100)
    q(u,frame-0x49,entries);q(u,frame-0x41,entries+len(weights)*16)
    q(u,frame-0x29,cdf);q(u,frame-0x21,cdf+len(weights)*4)
    u.reg_write(UC_X86_REG_RBP,frame)
    current=[0]
    def hook(u,a,size,data):
        if a==0x81237ea:
            assert rr(u,UC_X86_REG_RDX)==0 and rr(u,UC_X86_REG_R8)==total-1
            u.reg_write(UC_X86_REG_RAX,current[0]);u.reg_write(UC_X86_REG_RIP,a+5)
    u.hook_add(UC_HOOK_CODE,hook)
    observed=[0]*len(weights)
    for draw in range(total):
        current[0]=draw;u.reg_write(UC_X86_REG_R11,cdf+len(weights)*4);u.reg_write(UC_X86_REG_RBX,0)
        u.emu_start(0x81237dd,0x8123863,count=300)
        i=(rr(u,UC_X86_REG_RBX)-0x10008000)//0x100
        assert 0<=i<len(weights);observed[i]+=1
    assert observed==weights,(weights,observed)
    return {'weights':weights,'all_rng_draws':total,'native_selected_counts':observed}

result={'release':'Test29','status':'PASS_CAPTURED_NATIVE','native_code_sha256':CAPTURE_HASH,
 'root_to_generator':[root_chain(i,n) for i,n in [(0,0),(0,20),(1,7),(2,20),(3,1),(4,20)]],
 'weighted_selection':[weighted_selection(w) for w in [[1,1],[1,7,2],[0,3,0,2],[20,0,1,40,7,3,0,8]]],
 'limitations':['Record APIs and RNG draw are fixtures.','Native player attribute creation and live recruit cohort publication are not executed.','This proves the external generator entry and integer-weight selection consumer, not live seasonal outcomes.']}
if len(sys.argv)>2:pathlib.Path(sys.argv[2]).write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
