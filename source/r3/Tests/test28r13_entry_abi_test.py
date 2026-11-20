"""Native-entry detour preserves Win64 callee-saved registers on both paths."""
from pathlib import Path
src=(Path(__file__).parent/'test28r13_open_lists_test.py').read_text()
exec(compile(src.split('\ncases=0')[0],'R13 detour fixture','exec'))
checks=[]
for cls,st,mode_value,want in [(0,2,1,0),(3,4,1,0),(5,2,1,1),(0,5,1,1),(0,2,0,1),(0,2,-1,1)]:
 reset29();r,p=make(95700+len(checks),((10,1000),(11,800)),[10],stage=st)
 records[r]['scalar'][2]=cls;configure(mode_value)
 saved={globals()['UC_X86_REG_'+n]:0x101234560+i*123 for i,n in enumerate(['RBX','RBP','RSI','RDI','R12','R13','R14','R15'])}
 saved.update({globals()['UC_X86_REG_XMM'+str(i)]:0x112233445566778899aabbccddeeff00+i for i in range(6,16)})
 for regid,value in saved.items():uc.reg_write(regid,value)
 assert locked(r,96)==want
 assert all(uc.reg_read(k)==v for k,v in saved.items())
 assert reg('RSP')==STACK+0xf0000
 checks.append({'class':cls,'stage':st,'mode':mode_value,'callee_saved_gpr_and_xmm_preserved':True,'stack_balanced':True})
configure(1)
(OUTPUT/'test28r13_entry_abi_results.json').write_text(json.dumps({'checks':checks,'live_game_verified':False},indent=2)+'\n')
print('PASS: six compiled detour/fallback Win64 ABI cases')
