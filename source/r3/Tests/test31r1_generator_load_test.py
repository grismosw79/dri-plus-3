#!/usr/bin/env python3
"""Test31R1 native Generator layout regression, independent of the JS parser.

Requires Python 3 and unicorn. Supply the two immutable title-update captures:
  python test31r1_generator_load_test.py --native-original CAPTURE_DIR \
    --native-modmanager CAPTURE_DIR [--candidate PACKAGE_DIR]

Default fixtures under Audit/Test31R1/Generator reproduce Test30 success and
released Test31 nontermination without reconstructing either earlier release.
--seed-fixtures --baseline PACKAGE_DIR --released PACKAGE_DIR creates those
fixtures from immutable packages; it never modifies a package FTC.

Scope: raw sequential FTC/table layout plus captured native active-row/free-list
traversal. It does not claim a complete live dynasty initialization test.
"""
import argparse, hashlib, json, struct, zlib
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_64
from unicorn.x86_const import UC_X86_REG_RCX,UC_X86_REG_RSP,UC_X86_REG_RAX,UC_X86_REG_RIP
ENTRY=0x08069610
CODE_LENGTH=0x3a
EXPECTED_CODE_SHA='d12b1320058b6bd8d2194087b90b35898780f197f7a66217d31a482e376a7195'
TABLE_UID=3981495127
LIMIT=5000
U=lambda b,o:struct.unpack_from('>I',b,o)[0]
SHA=lambda b:hashlib.sha256(b).hexdigest()

def read_ftc(path):
    """Read sequential native block sizes, not schema/parser-derived offsets."""
    packed=path.read_bytes();raw=zlib.decompress(packed)
    assert raw[:4]==b'FrTk' and U(raw,4)==128
    count=U(raw,20);assert count==98 and U(raw,28)==count
    assert U(raw,36)==0,'Unexpected prefix index table requires explicit extension'
    cursor=128;tables=[]
    for index in range(count):
        base=cursor;name=raw[base:base+128].split(b'\0',1)[0].decode('ascii')
        assert raw[base+148:base+152] in (b'SPBF',b'ASTO'),(path,index,'store tag')
        store_length=U(raw,base+164);outer=base+168+store_length;block_start=outer+32
        tag=raw[block_start:block_start+4];assert tag in (b'BSFT',b'CMPC'),(path,index,tag)
        outer_capacity=U(raw,outer+8);physical_length=U(raw,outer+16);string_length=U(raw,outer+20);third_length=U(raw,outer+24)
        inner_length=U(raw,block_start+4);expanded_length=U(raw,block_start+8);words=U(raw,block_start+12);capacity=U(raw,block_start+16);members=U(raw,block_start+20);rowbase=U(raw,block_start+24);freehead=U(raw,block_start+28)
        assert inner_length==physical_length and capacity==outer_capacity
        assert inner_length%4==0 and words>0 and capacity<131072
        assert third_length==0,'Unexpected third-table layout requires explicit extension'
        array=tag==b'CMPC';actual_base=8+(capacity if array else members)
        actual_length=(actual_base+capacity*words)*4
        assert actual_length==physical_length,(path,name,'row bytes do not match physical block')
        assert expanded_length>=physical_length and U(raw,base+136)==string_length
        block=raw[block_start:block_start+physical_length]
        assert len(block)==physical_length
        table={'index':index,'table_id':U(raw,base+128),'uid':U(raw,base+132),'name':name,'array':array,'capacity':capacity,'members':members,'words':words,'stored_rowbase_words':rowbase,'actual_rowbase_words':actual_base,'freehead':freehead,'raw_block_offset':block_start,'raw_table_offset':base,'block':block}
        if array:
            # Counts are the consecutive capacity words after the CMPC header.
            counts=[U(block,32+4*r) for r in range(capacity)]
            assert all(n<=members for n in counts),(path,name,'array count over member capacity')
        tables.append(table);cursor=block_start+physical_length+string_length+third_length
    assert cursor==len(raw)-8,'Unexpected trailing asset-table size'
    return {'file_sha256':SHA(packed),'unpacked_sha256':SHA(raw),'tables':tables,'table_end':cursor,'trailing_asset_bytes':len(raw)-cursor}

def walk_native(block):
    cap=U(block,16);words=U(block,12);base=U(block,24);cur=U(block,28);chain=[];seen=set()
    while cur<cap:
        if cur in seen:return {'returns':False,'cycle':chain[chain.index(cur):]+[cur],'chain':chain,'active_rows':None}
        seen.add(cur);chain.append(cur);off=4*(base+cur*words)
        assert off>=32 and off+4<=len(block),'Native free pointer outside primary block'
        cur=U(block,off)
    assert cur==cap,('Native free-list terminal differs from capacity',cur,cap)
    return {'returns':True,'cycle':None,'chain':chain,'active_rows':cap-len(chain)}

def export_table(t):
    return {k:v for k,v in t.items() if k!='block'}

def capture_bytes(directory):
    p=directory/'section_00_00001000.bin'
    with p.open('rb') as f:f.seek(ENTRY-0x1000);code=f.read(CODE_LENGTH)
    assert len(code)==CODE_LENGTH and SHA(code)==EXPECTED_CODE_SHA,('Native signature mismatch',p,SHA(code))
    return code

def replay(code,block):
    assert len(block)%4==0
    values=struct.unpack('>'+str(len(block)//4)+'I',block)
    decoded=struct.pack('<'+str(len(values))+'I',*values)
    u=Uc(UC_ARCH_X86,UC_MODE_64);page=ENTRY&~0xfff;u.mem_map(page,0x1000);u.mem_write(ENTRY,code)
    u.mem_map(0x50000000,(len(decoded)+4095)&~4095);u.mem_write(0x50000000,decoded)
    u.mem_map(0x70000000,0x10000);stop=page+0xff0;sp=0x7000ff00
    u.mem_write(sp,struct.pack('<Q',stop));u.reg_write(UC_X86_REG_RSP,sp);u.reg_write(UC_X86_REG_RCX,0x50000000)
    u.emu_start(ENTRY,stop,count=LIMIT)
    ip=u.reg_read(UC_X86_REG_RIP);returned=ip==stop
    return {'returned':returned,'return_value':u.reg_read(UC_X86_REG_RAX) if returned else None,'end_rip':hex(ip),'instruction_limit':LIMIT}

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--candidate',type=Path,default=Path(__file__).resolve().parents[4])
    ap.add_argument('--native-original',type=Path,required=True);ap.add_argument('--native-modmanager',type=Path,required=True)
    ap.add_argument('--baseline',type=Path);ap.add_argument('--released',type=Path);ap.add_argument('--seed-fixtures',action='store_true')
    ap.add_argument('--output',type=Path);args=ap.parse_args()
    audit=args.candidate/'Runtime/Portal/Audit/Test31R1/Generator';fixtures=audit/'fixtures';fixtures.mkdir(parents=True,exist_ok=True)
    original=capture_bytes(args.native_original);modded=capture_bytes(args.native_modmanager);assert original==modded
    manifest_path=fixtures/'manifest.json'
    if args.seed_fixtures:
        assert args.baseline and args.released
        entries=[]
        for label,package in [('Test30',args.baseline),('Test31',args.released)]:
            for route in range(5):
                ftc=read_ftc(package/f'frantkdata/{route}/dynasty-generatordata-binary.FTC')
                matches=[t for t in ftc['tables'] if t['uid']==TABLE_UID];assert len(matches)==1;t=matches[0]
                filename=f'route{route}_{label.lower()}_cmpc.bin';(fixtures/filename).write_bytes(t['block'])
                mismatches=[export_table(x) for x in ftc['tables'] if x['stored_rowbase_words']!=x['actual_rowbase_words']]
                cycles=[export_table(x) for x in ftc['tables'] if not walk_native(x['block'])['returns']]
                if label=='Test30':assert not mismatches and not cycles
                else:assert [x['uid'] for x in mismatches]==[TABLE_UID] and [x['uid'] for x in cycles]==[TABLE_UID]
                entries.append({'package':label,'route':route,'file':filename,'block_sha256':SHA(t['block']),'source_ftc_sha256':ftc['file_sha256'],'table':export_table(t),'all_table_count':len(ftc['tables']),'all_array_count':sum(x['array'] for x in ftc['tables']),'native_rowbase_mismatches':mismatches,'native_free_cycles':cycles})
        manifest_path.write_text(json.dumps({'fixtures':entries},indent=2)+'\n')
    manifest=json.loads(manifest_path.read_text());results=[];layout=[]
    assert sorted((e['package'],e['route']) for e in manifest['fixtures'])==[(v,r) for v in ('Test30','Test31') for r in range(5)]
    for entry in manifest['fixtures']:
        block=(fixtures/entry['file']).read_bytes();assert SHA(block)==entry['block_sha256'];case=entry['package'];walk=walk_native(block)
        for capture,code in [('original',original),('mod_manager',modded)]:
            result=replay(code,block)
            if case=='Test30':assert result['returned'] and result['return_value']==244 and walk['active_rows']==244
            else:
                assert case=='Test31' and not result['returned'] and walk['cycle']==[0,2,0]
                assert 0x08069630<=int(result['end_rip'],16)<=0x08069644
            results.append({'package':case,'route':entry['route'],'native_capture':capture,'block_sha256':SHA(block),'table_id':entry['table']['table_id'],'stored_rowbase_words':U(block,24),'actual_rowbase_words':entry['table']['actual_rowbase_words'],'native_chain':walk,**result})
    for route in range(5):
        ftc=read_ftc(args.candidate/f'frantkdata/{route}/dynasty-generatordata-binary.FTC')
        for t in ftc['tables']:
            assert t['stored_rowbase_words']==t['actual_rowbase_words'],('Native rowbase mismatch',route,t['name'],t['stored_rowbase_words'],t['actual_rowbase_words'])
            assert walk_native(t['block'])['returns'],('Native free-chain cycle',route,t['name'])
        matches=[t for t in ftc['tables'] if t['uid']==TABLE_UID];assert len(matches)==1;t=matches[0];block=t['block'];walk=walk_native(block)
        for capture,code in [('original',original),('mod_manager',modded)]:
            result=replay(code,block);assert result['returned'] and result['return_value']==246 and walk['active_rows']==246
            results.append({'package':'Test31R1','route':route,'native_capture':capture,'block_sha256':SHA(block),'table_id':t['table_id'],'stored_rowbase_words':U(block,24),'actual_rowbase_words':t['actual_rowbase_words'],'native_chain':walk,**result})
        (audit/f'route{route}_test31r1_cmpc.bin').write_bytes(block)
        layout.append({'route':route,'ftc_sha256':ftc['file_sha256'],'tables_checked':len(ftc['tables']),'array_tables_checked':sum(x['array'] for x in ftc['tables']),'native_rowbase_mismatches':0,'native_free_chain_cycles':0,'array_counts_valid':True})
    assert len(results)==30
    report={'status':'PASS_NATIVE_GENERATOR_LOAD_REGRESSION','scope':'Sequential FTC/table layout and captured native active-row/free-chain traversal; not a live full-dynasty test.','native_function_rva':hex(ENTRY),'native_function_bytes':CODE_LENGTH,'native_function_sha256':SHA(original),'original_modmanager_bytes_identical':True,'baseline_active_rows':244,'released_failure':'Stored row-data base257 instead of259 makes free chain8->0->2->0 and native traversal does not return.','repaired_active_rows':246,'candidate_layout':layout,'native_replays':results,'total_native_replays':len(results),'expected_predecessor_nonterminations':10,'expected_successful_returns':20}
    out=args.output or audit/'native_generator_load_verification.json';out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ['native_replays','candidate_layout']},indent=2))
if __name__=='__main__':main()
