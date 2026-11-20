#!/usr/bin/env python3
"""Guarded Test31 -> Test31R1 native array-row-base header repair only.

Usage: python build_test31r1_generator.py BASE_TEST31 OUTPUT_TEST31R1
No tuning, recipe, selector weight, runtime, launcher, or save is changed.
"""
from pathlib import Path
import hashlib,json,struct,sys,zlib
EXPECTED=['532aa79551193cf31b082ac05241ebf1d8a07d3f2857d6261c7cf3aaa87c9563','e73711d4b60a26d8a35fba0df845a8c94f394e817691d065fba4d5c0e856443b','c6caa8f314b3b66e3b310e04b47f5dd1017d44a1dbb9b2f367e95b1d2881b340','587b197043fb7fb55239cb4986e0c5eb39eda95c43a00c124412854a1fb26fc3','532aa79551193cf31b082ac05241ebf1d8a07d3f2857d6261c7cf3aaa87c9563']
UID=3981495127
sha=lambda b:hashlib.sha256(b).hexdigest()
def unpack(data):
 dec=zlib.decompressobj();raw=dec.decompress(data)+dec.flush();assert dec.eof and not dec.unused_data and not dec.unconsumed_tail;return raw
def tables(raw):
 """Walk sequential native framing, not the JavaScript parser's magic scan."""
 u=lambda off:struct.unpack_from('>I',raw,off)[0]
 assert raw[:4]==b'FrTk' and u(4)==128
 offset=128+u(36)*8;rows=[]
 for _ in range(u(20)):
  name=raw[offset:offset+128].split(b'\0',1)[0].decode('ascii');is_array=name.endswith('[]');store=u(offset+164);header=offset+168+store;block=header+32
  assert raw[offset+148:offset+152] in (b'SPBF',b'ASTO',b'SPEX')
  primary,secondary,third=u(header+16),u(header+20),u(header+24)
  assert primary==u(block+4)
  capacity,words,members=u(block+16),u(block+12),u(block+20)
  assert capacity==u(header+8)
  stored=u(block+24);expected=8+(capacity if is_array else members)
  assert primary==4*(expected+capacity*words),(name,'physical primary size')
  end=block+primary+secondary+third;assert offset<end<=len(raw)
  rows.append({'name':name,'table_id':u(offset+128),'unique_id':u(offset+132),'table_offset':offset,'block_offset':block,'row_base_field_offset':block+24,'stored_row_base_words':stored,'expected_row_base_words':expected,'capacity':capacity,'record_words':words,'array':is_array,'end':end})
  offset=end
 assert len(rows)==98 and offset==len(raw)-8 and u(28)==len(rows)
 return rows

def main():
 base,out=map(lambda p:Path(p).resolve(),sys.argv[1:3]);assert base!=out
 audit=out/'Runtime/Portal/Audit/Test31R1/Generator';audit.mkdir(parents=True,exist_ok=True);reports=[]
 for route in range(5):
  rel=Path(f'frantkdata/{route}/dynasty-generatordata-binary.FTC');source=(base/rel).read_bytes();assert sha(source)==EXPECTED[route],'Unsupported input generator; no patch permitted'
  before=unpack(source);old_tables=tables(before);bad=[t for t in old_tables if t['stored_row_base_words']!=t['expected_row_base_words']]
  assert len(bad)==1 and bad[0]['unique_id']==UID and bad[0]['array']
  t=bad[0];assert(t['stored_row_base_words'],t['expected_row_base_words'],t['capacity'])==(257,259,251)
  raw=bytearray(before);struct.pack_into('>I',raw,t['row_base_field_offset'],259);new_tables=tables(raw);assert all(t['stored_row_base_words']==t['expected_row_base_words'] for t in new_tables)
  diff=[i for i,(a,b)in enumerate(zip(before,raw))if a!=b];assert len(before)==len(raw) and diff==[t['row_base_field_offset']+3] and (before[diff[0]]^raw[diff[0]])==2
  changed=zlib.compress(bytes(raw));assert unpack(changed)==raw
  dest=out/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(changed)
  archived=audit/'Baseline'/rel;archived.parent.mkdir(parents=True,exist_ok=True);archived.write_bytes(source)
  report={'release':'Test31R1','scope':'Native serialized row-data-base repair only','route':route,'source_sha256':sha(source),'sha256':sha(changed),'source_unpacked_sha256':sha(before),'unpacked_sha256':sha(raw),'compressed_bytes_before':len(source),'compressed_bytes_after':len(changed),'unpacked_bytes':len(raw),'unique_id':UID,'table_id':t['table_id'],'table_name':t['name'],'field':'CMPC.row_data_base_words','absolute_unpacked_field_offset':t['row_base_field_offset'],'table_relative_field_offset':t['row_base_field_offset']-t['table_offset'],'before':257,'after':259,'native_byte_base_before':t['block_offset']+257*4,'native_byte_base_after':t['block_offset']+259*4,'changed_unpacked_byte_offsets':diff,'all_other_unpacked_bytes_identical':True,'all98_native_table_row_bases_valid':True,'live_creation_verified':False}
  (audit/f'route{route}_header_repair.json').write_text(json.dumps(report,indent=2)+'\n');reports.append(report)
  assert (base/rel).read_bytes()==source
 result={'release':'Test31R1','status':'HEADER_REPAIR_BUILT_NATIVE_REPLAY_AND_LIVE_VALIDATION_SEPARATE','files':reports,'fixed_generator_files':5,'changed_fields_per_file':1,'changed_unpacked_bits_per_file':1,'new_policy_changes':False,'tuning_changed':False,'runtime_changed':False,'live_creation_verified':False}
 (audit/'header_repair_build.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'status':result['status'],'fixed_files':5,'all_other_unpacked_bytes_identical':True}))
if __name__=='__main__':main()
