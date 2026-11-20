"""Reproduce Test30's seven accounting discrepancies from its actual CSV.
Read-only evidence extraction; this is not a replay or proof of live Test31.
Usage: python test31_funding_csv_audit.py CSV OUTPUT_JSON
"""
from pathlib import Path
import sys,json,hashlib
path=Path(sys.argv[1]);out=Path(sys.argv[2]);plans={};witnesses=[];cases=[]
for number,line in enumerate(path.open(errors='replace'),1):
 a=line.strip().split(',')
 if a[:2]==['PORTAL_PLAN','FUNDING']:plans.setdefault(a[2],set()).add(a[3])
 if a[:2]==['PORTAL29_FUNDING_WITNESS','POST_FUNDING']:witnesses.append(a)
 if a[:2]!=['PORTAL29_FUNDING_SUMMARY','POST_FUNDING']:continue
 tid=a[5]
 if a[-1]=='0':
  orphan=[{'recruit':int(x[6]),'raw_allocated':int(x[10]),'mask':int(x[11]),'slot':int(x[13])}
   for x in witnesses if x[5]==tid and x[6] not in plans.get(tid,set()) and int(x[10])>0]
  total,assigned,raw=map(int,a[6:9]);adjusted=raw-sum(x['raw_allocated']for x in orphan)
  cases.append({'line':number,'year':int(a[2]),'type':int(a[3]),'week':int(a[4]),'team':int(tid),
   'total':total,'assigned':assigned,'raw_flag_sum':raw,'retained_nonvector_pitches':orphan,
   'current_vector_flag_sum_upper_bound':adjusted,'within_native_budget':adjusted<=assigned<=total})
 plans.pop(tid,None);witnesses=[]
assert len(cases)==7 and all(x['within_native_budget']for x in cases)
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps({'input':path.name,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
 'evidence':'Actual Test30 CSV; rows show retained40h pitch flags absent from PORTAL_PLAN,FUNDING with slot0. Those rows are a DLL funding pool, not a complete raw native-vector serialization. The subtraction is an accounting reconstruction under the historical-flag interpretation, not proof of raw vector membership or replay of all HS/native allocations. Test31 records actual positive native-vector identity to test this interpretation live.',
 'cases':cases,'live_test31_verified':False},indent=2)+'\n')
print('PASS seven observed Test30 discrepancies admit retained-pitch accounting reconstruction; native assigned never exceeded total')
