'use strict';
// node build_eps_ftc.cjs ROUTE BASELINE_PACKAGE OUTPUT_PACKAGE
const fs=require('fs'),path=require('path'),crypto=require('crypto'),assert=require('assert/strict');
const api=require('../Audit/EPS1/Parser/save_adapter.cjs');
const [route,base,out]=process.argv.slice(2);assert(/^[0-4]$/.test(route));assert(base&&out&&path.resolve(base)!==path.resolve(out));
const rel=`frantkdata/${route}/dynasty-tuning-binary.FTC`,source=path.join(base,rel),dest=path.join(out,rel);
const hash=b=>crypto.createHash('sha256').update(b).digest('hex'),num=x=>typeof x==='string'?parseInt(x,2):x;
const grades=['APlus','A','AMinus','BPlus','B','BMinus','CPlus','C','CMinus','DPlus','D','DMinus','F'];
const cuts=[10,25,35,40,50,60,65,75,85,95,110,125,138];
const open=p=>api.create(p,{gameYearOverride:27,gameTypeOverride:'college',autoUnempty:false,saveOnChange:false});
async function append(t,count,sample){
 const old=Buffer.from(t.data),cap=t.header.recordCapacity,size=t.header.record1Size,growth=size*count;
 assert(!t.header.hasSecondTable&&!t.header.hasThirdTable&&!t.header.tableStoreLength);
 assert.equal(t.header.nextRecordToUse,cap);assert.equal(t.header.data1RecordCount,cap);assert.equal(old.length,t.header.table2StartIndex);
 const data=Buffer.concat([old,Buffer.alloc(growth)]),headers=[];
 for(const [offset,before,after] of [[176,cap,cap+count],[184,t.header.table1Length,t.header.table1Length+growth],[204,t.header.table1Length2,t.header.table1Length2+growth],[208,t.header.tableTotalLength,t.header.tableTotalLength+growth],[216,cap,cap+count],[228,cap,cap+count]]){
  assert.equal(data.readUInt32BE(offset),before);data.writeUInt32BE(after,offset);headers.push({offset,before,after});
 }
 const bytes=Buffer.from(t.records[sample].data);for(let i=0;i<count;i++)bytes.copy(data,old.length+i*size);
 await t.replaceRawData(api.Buffer.from(data),true);return {start:cap,count,growth,headers,old};
}
(async()=>{
 const original=fs.readFileSync(source),f=await open(source),before=f.tables.map(t=>({id:t.header.tableId,uid:t.header.uniqueId,name:t.name,data:Buffer.from(t.data),header:{...t.header}})),assets=JSON.stringify(f.assetTable);
 const tune=f.getTableByUniqueId(3269140088),ranges=f.getTableByUniqueId(1398169845),gradeMap=f.getTableByUniqueId(938761091);
 await tune.readRecords();await ranges.readRecords();await gradeMap.readRecords();
 const gradeEnum=gradeMap.records[0].fieldsArray.find(k=>k.key==='MySchoolGrade').offset.enum;
 assert.equal(gradeEnum._members.find(m=>m._name==='PlayingStyle')._value,0);
 const oldref=num(tune.records[0].PlayingStyleRankingRangeTable),map=f.getTableById(oldref>>>17);await map.readRecords();const oldMap=map.records[oldref&131071];
 const originalMap=Object.fromEntries(oldMap.fieldsArray.map(k=>[k.key,oldMap[k.key]]));
 const originals=Object.fromEntries(ranges.records.filter(r=>!r.isEmpty).map(r=>[r.index,Object.fromEntries(r.fieldsArray.map(k=>[k.key,r[k.key]]))]));
 const growRanges=await append(ranges,2,0),growMap=await append(map,1,oldref&131071);
 for(let direction=0;direction<2;direction++){
  const r=ranges.records[growRanges.start+direction];r.IsRankingRangeDecreasing=!!direction;
  grades.forEach((g,i)=>r[g]=direction?138-cuts[i]:cuts[i]-1);
 }
 const newMap=map.records[growMap.start];let mapped=0;
 for(const [key,v]of Object.entries(originalMap)){
  const n=num(v);if(!n)continue;assert.equal(n>>>17,ranges.header.tableId);
  const reverse=originals[n&131071].IsRankingRangeDecreasing;
  newMap[key]=ranges.getBinaryReferenceToRecord(growRanges.start+(reverse?1:0));mapped++;
 }
 tune.records[0].PlayingStyleRankingRangeTable=map.getBinaryReferenceToRecord(growMap.start);
 await f.save(dest,{sync:true});const v=await open(dest);assert.equal(v.tables.length,before.length);assert.equal(JSON.stringify(v.assetTable),assets);
 let unchanged=0;const modified=new Set([tune.header.tableId,map.header.tableId,ranges.header.tableId]);
 for(const b of before){const t=v.getTableById(b.id);assert.equal(t.name,b.name);assert.equal(t.header.uniqueId,b.uid);
  if(!modified.has(b.id)){assert(Buffer.from(t.data).equals(b.data),`Unexpected ${b.name}`);unchanged++;}
 }
 for(const [table,growth]of [[map,growMap],[ranges,growRanges]]){
  const t=v.getTableById(table.header.tableId);await t.readRecords();assert.equal(t.data.length,growth.old.length+growth.growth);
  const normalized=Buffer.from(t.data.subarray(0,growth.old.length));for(const h of growth.headers){assert.equal(normalized.readUInt32BE(h.offset),h.after);normalized.writeUInt32BE(h.before,h.offset);}
  assert(normalized.equals(growth.old),'An original row or unapproved header changed');
 }
 const vt=v.getTableById(tune.header.tableId);await vt.readRecords();const r=vt.records[0];assert.equal(r.StaticWeeks,4);
 const expectedTune=Buffer.from(before.find(b=>b.id===tune.header.tableId).data),d=r.fields.PlayingStyleRankingRangeTable.offset;
 assert.equal(d.length,32);assert.equal(d.offset%8,0);expectedTune.writeUInt32BE(num(r.PlayingStyleRankingRangeTable),vt.header.table1StartIndex+d.offset/8);
 assert(expectedTune.equals(Buffer.from(vt.data)),'Unexpected PlayingStyle tuning edit');
 const vr=v.getTableById(ranges.header.tableId);await vr.readRecords();let cases=0;
 for(let dir=0;dir<2;dir++)for(let rank=1;rank<=138;rank++){
  const curve=vr.records[growRanges.start+dir],native=dir?138-rank:rank-1;
  const actual=grades.findIndex(g=>dir?native>=curve[g]:native<=curve[g]);
  assert.equal(actual,cuts.findIndex(c=>rank<=c));cases++;
 }
 assert.equal(hash(fs.readFileSync(source)),hash(original));
 const result={route:Number(route),status:'PASS_OFFLINE_LIVE_PENDING',sourceSHA256:hash(original),sha256:hash(fs.readFileSync(dest)),tables:before.length,unchangedTables:unchanged,addedRangeRows:2,addedMappingRows:1,archetypesRemapped:mapped,gradeBoundaryCases:cases,allOriginalRangeAndMappingRowsPreserved:true,onlyPlayingStylePointerChanged:true,assetsUnchanged:true,requirementsAndRecruitingBudgetsUnchanged:true,gradeCuts:cuts,nativePlayingStyleGradeEnum:0,live_game_verified:false};
 fs.mkdirSync(path.join(out,'Audit/EPS1'),{recursive:true});fs.writeFileSync(path.join(out,`Audit/EPS1/ftc-route-${route}.json`),JSON.stringify(result,null,2));console.log(JSON.stringify(result));
})().catch(e=>{console.error(e.stack);process.exit(1)});
