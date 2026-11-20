'use strict';
// node build_test28_ftc.cjs ROUTE BASELINE_PACKAGE OUTPUT_PACKAGE
// Build a category-isolated Test28 FTC from the untouched Test27R2 route.
// College roster and portal OVR requirements, staff-specific grades, ProPotential score
// policy, PlayingTime grades and Alaska geography require the matching DLL.
const fs=require('fs'),path=require('path'),crypto=require('crypto'),assert=require('assert/strict');
const api=require('../Audit/EPS1/Parser/save_adapter.cjs');
const [route,baseArg,outArg]=process.argv.slice(2);
assert(/^[0-4]$/.test(route));assert(baseArg&&outArg);
const base=path.resolve(baseArg),out=path.resolve(outArg);assert.notEqual(base,out);
const rel=`frantkdata/${route}/dynasty-tuning-binary.FTC`,source=path.join(base,rel),dest=path.join(out,rel);
const num=x=>typeof x==='string'?parseInt(x,2):x;
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const fields=r=>Object.fromEntries(r.fieldsArray.map(f=>[f.key,r[f.key]]));
const open=p=>api.create(p,{gameYearOverride:27,gameTypeOverride:'college',autoUnempty:false,saveOnChange:false});
const gradeKeys=['APlus','A','AMinus','BPlus','B','BMinus','CPlus','C','CMinus','DPlus','D','DMinus','F'];
const edits=[],growths=[];
async function append(t,count,sample){
 const old=Buffer.from(t.data),cap=t.header.recordCapacity,size=t.header.record1Size,growth=size*count;
 assert(!t.header.hasSecondTable&&!t.header.hasThirdTable&&!t.header.tableStoreLength);
 assert.equal(t.header.nextRecordToUse,cap);assert.equal(t.header.data1RecordCount,cap);assert.equal(old.length,t.header.table2StartIndex);
 const data=Buffer.concat([old,Buffer.alloc(growth)]),headers=[];
 for(const [offset,before,after] of [[176,cap,cap+count],[184,t.header.table1Length,t.header.table1Length+growth],[204,t.header.table1Length2,t.header.table1Length2+growth],[208,t.header.tableTotalLength,t.header.tableTotalLength+growth],[216,cap,cap+count],[228,cap,cap+count]]){
  assert.equal(data.readUInt32BE(offset),before);data.writeUInt32BE(after,offset);headers.push({offset,before,after});
 }
 const bytes=Buffer.from(t.records[sample].data);for(let i=0;i<count;i++)bytes.copy(data,old.length+i*size);
 await t.replaceRawData(api.Buffer.from(data),true);
 const result={table:t.name,id:t.header.tableId,start:cap,count,size,growth,headers,sample,expectedData:Buffer.from(data)};
 growths.push(result);return result;
}
function set(t,row,key,value){
 const r=t.records[row];assert(r.fields[key],`${t.name}.${key} missing`);
 const old=r[key];if(old===value)return;
 edits.push({table:t.name,id:t.header.tableId,row,key,before:old,after:value,offset:r.fields[key].offset.offset,length:r.fields[key].offset.length});r[key]=value;
}
function threshold(t,row,values){for(const g of gradeKeys)set(t,row,g,Object.hasOwn(values,g)?values[g]:-100);set(t,row,'Incomplete',0);}
function lookup(r,x){let best=-Infinity,grade=12;for(let i=0;i<gradeKeys.length;i++){const v=r[gradeKeys[i]];if(v>=0&&v<=x&&v>best){best=v;grade=i;}}return grade;}
function nativeNearest(r,x){let best=200,grade=0;for(let i=0;i<gradeKeys.length;i++){const d=Math.abs(r[gradeKeys[i]]-x);if(d===0)return i;if(d<best){best=d;grade=i;}}return grade;}
function applyAllowedBits(expected,actual,offset,length){
 // Table scalar packing is MSB-first. Copy only explicitly changed field bits.
 for(let bit=offset;bit<offset+length;bit++){const byte=bit>>>3,mask=1<<(7-(bit&7));expected[byte]=(expected[byte]&~mask)|(actual[byte]&mask);}
}
(async()=>{
 const original=fs.readFileSync(source),f=await open(source);
 const before=new Map(f.tables.map(t=>[t.header.tableId,{id:t.header.tableId,name:t.name,uid:t.header.uniqueId,data:Buffer.from(t.data),header:{...t.header}}]));
 const assets=JSON.stringify(f.assetTable);
 const db=f.getTableByUniqueId(4118070883),values=f.getTableByUniqueId(4290750322),ranges=f.getTableByUniqueId(1398169845),acq=f.getTableByUniqueId(954733288),cc=f.getTableByUniqueId(3404461792),brand=f.getTableByUniqueId(3069400609),pro=f.getTableByUniqueId(1150011103),leaving=f.getTableByUniqueId(2910765890);
 for(const t of[db,values,ranges,acq,cc,brand,pro,leaving]){assert(t);await t.readRecords();}
 const byMotivation=Object.fromEntries(db.records.filter(r=>!r.isEmpty).map(r=>[r.Motivation,r]));assert.equal(Object.keys(byMotivation).length,8);
 const dbBefore=Object.fromEntries(Object.entries(byMotivation).map(([k,r])=>[k,fields(r)]));
 const initialValues=values.records.map(fields);
 const v=await append(values,6,num(byMotivation.BrandExposure.OVRThresholdToLetterGradeTable)&131071);
 const rows={proximity:v.start,conferenceHS:v.start+1,conferenceCollege:v.start+2,championshipCollege:v.start+3,brandCollege:v.start+4,proCollege:v.start+5};
 threshold(values,rows.proximity,{B:0});
 threshold(values,rows.conferenceHS,{F:0,DMinus:1,CMinus:2,AMinus:3});
 threshold(values,rows.conferenceCollege,{CMinus:0,B:80,AMinus:85});
 threshold(values,rows.championshipCollege,{D:0,DPlus:70,C:75,BMinus:80,B:85,AMinus:90});
 threshold(values,rows.brandCollege,{D:0,DPlus:70,C:75,BMinus:80,B:85,AMinus:90});
 threshold(values,rows.proCollege,{D:0,DPlus:70,C:75,BMinus:82,AMinus:90});
 const pointer=(mot,key,row)=>set(db,byMotivation[mot].index,key,values.getBinaryReferenceToRecord(row));
 for(const key of['OVRThresholdToLetterGradeTable','RecruitStarLevelThresholdToLetterGradeTable','TransferStarLevelThresholdToLetterGradeTable'])pointer('ProximityToHome',key,rows.proximity);
 set(db,byMotivation.ProximityToHome.index,'DealbreakerLetterGradeThresholdForCPU','B');
 set(db,byMotivation.ProximityToHome.index,'DealbreakerLetterGradeThresholdForUser','B');
 pointer('ConferencePrestige','RecruitStarLevelThresholdToLetterGradeTable',rows.conferenceHS);
 for(const [mot,row]of[['ConferencePrestige',rows.conferenceCollege],['ChampionshipContender',rows.championshipCollege],['BrandExposure',rows.brandCollege],['ProPotential',rows.proCollege]])pointer(mot,'OVRThresholdToLetterGradeTable',row);
 // All existing HS thresholds other than Conference/PTH, all PlayingStyle
 // thresholds, and all shared original rows are preserved byte-for-byte.
 assert.equal(cc.records[0].CurrentYearOVRMod+cc.records[0].CurrentYearPollRankMod,100);
 set(cc,0,'CurrentYearOVRMod',50);set(cc,0,'CurrentYearPollRankMod',50);
 const streamingSupported=!!brand.records[0].fields.StreamingGamesPlayedMod;
 if(streamingSupported){assert(brand.records[0].fields.StreamingGamesWonMod);set(brand,0,'StreamingGamesPlayedMod',1);set(brand,0,'StreamingGamesWonMod',3);}
 else{assert.equal(Number(route),1);assert.equal(brand.records[0].fieldsArray.length,6);}
 // Give Pro Potential a private copy of its existing ranking curve, without
 // altering any grade cuts shared with Coach Prestige or Brand Exposure.
 const proRangeKey=pro.records[0].fields.DraftPicksToGradeRange?'DraftPicksToGradeRange':'Field_0';
 const proRangeRef=num(pro.records[0][proRangeKey]);assert.equal(proRangeRef>>>17,ranges.header.tableId);
 const priorProRange=fields(ranges.records[proRangeRef&131071]);
 const rg=await append(ranges,1,proRangeRef&131071);
 set(pro,0,proRangeKey,typeof pro.records[0][proRangeKey]==='string'?ranges.getBinaryReferenceToRecord(rg.start):num(ranges.getBinaryReferenceToRecord(rg.start)));
 // LS is a native-supported lookup group10. Its formerly null field can
 // reference a normal row; no new native enum or eligibility branch needed.
 const lookupRef=num(leaving.records[0].PlayingTimePositionDealbreakerTable??leaving.records[0].Field_33);
 const positionLookup=f.getTableById(lookupRef>>>17);await positionLookup.readRecords();
 const lookupRow=lookupRef&131071,lookupBefore=fields(positionLookup.records[lookupRow]);
 assert.equal(num(lookupBefore.LS),0);assert.equal(acq.header.recordCapacity,10);
 const qbRow=num(lookupBefore.QB)&131071;assert.equal(num(lookupBefore.QB)>>>17,acq.header.tableId);
 const ag=await append(acq,1,qbRow);
 for(let row=0;row<acq.header.recordCapacity;row++){set(acq,row,'minOVR',75);set(acq,row,'shouldCheckOVR',true);set(acq,row,'shouldCheckStarRating',false);}
 set(positionLookup,lookupRow,'LS',acq.getBinaryReferenceToRecord(ag.start));
 await f.save(dest,{sync:true});
 const saved=await open(dest);assert.equal(saved.tables.length,before.size);assert.equal(JSON.stringify(saved.assetTable),assets);
 const touched=new Set(edits.map(z=>z.id).concat(growths.map(z=>z.id)));let unchanged=0;
 for(const t of saved.tables){
  const b=before.get(t.header.tableId);assert(b);assert.equal(t.name,b.name);assert.equal(t.header.uniqueId,b.uid);
  if(!touched.has(t.header.tableId)){assert(Buffer.from(t.data).equals(b.data),`Unexpected table change: ${t.name}`);unchanged++;continue;}
  await t.readRecords();const g=growths.find(g=>g.id===t.header.tableId),expected=Buffer.from(g?g.expectedData:b.data),actual=Buffer.from(t.data);
  assert.equal(actual.length,expected.length);
  for(const edit of edits.filter(z=>z.id===t.header.tableId)){
   const r=t.records[edit.row];assert.deepEqual(r[edit.key],edit.after,`${t.name}[${edit.row}].${edit.key}`);
   applyAllowedBits(expected,actual,8*(t.header.table1StartIndex+edit.row*t.header.record1Size)+edit.offset,edit.length);
  }
  assert(expected.equals(actual),`Unexpected bits or headers changed: ${t.name}`);
  if(g)for(const h of g.headers)assert.equal(actual.readUInt32BE(h.offset),h.after);
 }
 const outValues=saved.getTableByUniqueId(4290750322);await outValues.readRecords();
 for(let row=0;row<initialValues.length;row++)assert.deepEqual(fields(outValues.records[row]),initialValues[row]);
 let cases=0;
 for(let ovr=0;ovr<=99;ovr++){
  assert.equal(lookup(outValues.records[rows.proximity],ovr),4);cases++;
  assert.equal(lookup(outValues.records[rows.conferenceCollege],ovr),ovr<80?8:ovr<85?4:2);cases++;
  for(const row of[rows.championshipCollege,rows.brandCollege]){assert.equal(lookup(outValues.records[row],ovr),ovr<70?10:ovr<75?9:ovr<80?7:ovr<85?5:ovr<90?4:2);cases++;}
  assert.equal(lookup(outValues.records[rows.proCollege],ovr),ovr<70?10:ovr<75?9:ovr<82?7:ovr<90?5:2);cases++;
 }
 for(let stars=0;stars<5;stars++){assert.equal(lookup(outValues.records[rows.conferenceHS],stars),[12,11,8,2,2][stars]);assert.equal(lookup(outValues.records[rows.proximity],stars),4);cases+=2;}
 const outAcq=saved.getTableByUniqueId(954733288);await outAcq.readRecords();
 for(const r of outAcq.records)for(let ovr=0;ovr<100;ovr++)for(let star=0;star<5;star++){
  assert.equal((r.shouldCheckOVR&&ovr>=r.minOVR)||(r.shouldCheckStarRating&&star>=4),ovr>=75);cases++;
 }
 const outDb=saved.getTableByUniqueId(4118070883);await outDb.readRecords();
 for(const r of outDb.records){const old=dbBefore[r.Motivation];if(!['ProximityToHome','ConferencePrestige'].includes(r.Motivation))assert.equal(r.RecruitStarLevelThresholdToLetterGradeTable,old.RecruitStarLevelThresholdToLetterGradeTable);if(r.Motivation==='PlayingStyle')assert.deepEqual(fields(r),old);}
 const nativeCurves=[];let nativeStarCases=0;
 const normalHS=[1,2].includes(Number(route))?[11,10,8,6,4]:[12,11,8,6,4];
 for(const r of outDb.records){
  const old=dbBefore[r.Motivation];
  for(const [phase,key]of[['HS','RecruitStarLevelThresholdToLetterGradeTable'],['portal','TransferStarLevelThresholdToLetterGradeTable'],['roster','OVRThresholdToLetterGradeTable']]){
   const ref=num(r[key]),row=ref&131071,curve=fields(outValues.records[row]),oldCurve=initialValues[num(old[key])&131071];
   assert.equal(ref>>>17,outValues.header.tableId);
   const inputs=phase==='roster'?Array.from({length:100},(_,i)=>i):[0,1,2,3,4];
   const nativeResults=inputs.map(x=>nativeNearest(curve,x));
   if(phase!=='roster')for(let stars=0;stars<5;stars++){
    const expected=r.Motivation==='ProximityToHome'?4:phase==='HS'&&r.Motivation==='ConferencePrestige'?[12,11,8,2,2][stars]:nativeNearest(oldCurve,stars);
    assert.equal(nativeResults[stars],expected,`Native nearest ${r.Motivation} ${phase} ${stars}`);
    assert(curve[gradeKeys[nativeResults[stars]]]>=0,`Disabled anchor selected: ${r.Motivation} ${phase} ${stars}`);
    if(phase==='HS'&&!['ProximityToHome','ConferencePrestige'].includes(r.Motivation))assert.equal(nativeResults[stars],normalHS[stars]);
    nativeStarCases++;
   }
   nativeCurves.push({motivation:r.Motivation,phase,ref,row,values:gradeKeys.map(k=>curve[k]),inputs,nativeResults,explicitFloorResults:inputs.map(x=>lookup(curve,x))});
  }
 }
 const outRanges=saved.getTableByUniqueId(1398169845);await outRanges.readRecords();assert.deepEqual(fields(outRanges.records[rg.start]),priorProRange);
 const outLookup=saved.getTableById(positionLookup.header.tableId);await outLookup.readRecords();
 for(const [key,reference]of Object.entries(lookupBefore)){const actual=num(outLookup.records[lookupRow][key]);assert.equal(actual,key==='LS'?num(outAcq.getBinaryReferenceToRecord(ag.start)):num(reference));}
 assert.equal(sha(fs.readFileSync(source)),sha(original),'Baseline was modified');
 const result={release:'Test28',route:Number(route),status:'PASS_OFFLINE_LIVE_PENDING',sourceSHA256:sha(original),sha256:sha(fs.readFileSync(dest)),tables:before.size,unchangedTables:unchanged,editedTables:[...touched].map(id=>({id,name:before.get(id).name})),edits,growths:growths.map(({expectedData,...g})=>g),privateRequirementRows:Object.fromEntries(Object.entries(rows).map(([key,row])=>[key,{row,ref:num(values.getBinaryReferenceToRecord(row)),values:fields(outValues.records[row])}])),proPotentialPrivateRange:{row:rg.start,reference:num(outRanges.getBinaryReferenceToRecord(rg.start)),nativeShape:pro.records[0].fieldsArray.length},playingTimeAcquisition:{groups:11,minOVR:75,starCheck:false,LSAddedRow:ag.start,LSReference:num(outAcq.getBinaryReferenceToRecord(ag.start)),lookupReference:lookupRef,nativeLSGroup:10,nativeLookupFieldOrdinal:4},streaming:{supported:streamingSupported,appearance:streamingSupported?1:null,win:streamingSupported?3:null},routeNotes:[...(!streamingSupported?['Legacy route1 has no streaming coefficients. Its Brand Exposure scoring table is preserved; the matched Test28 DLL targets the verified modern game executable.']:[]),...([1,2].includes(Number(route))?['Legacy routes1/2 retain their existing ordinary HS1/2-star D−/D requirements; modern routes retain F/D−. All ordinary HS4/5-star requirements remain C+/B.']:[]),...(pro.records[0].fieldsArray.length===3?['Legacy ProPotential schema has three fields; its existing weights and private curve are preserved. The Test28 runtime supports the verified modern five-field game schema.']:[])],intendedFloorPolicyAndAcquisitionCases:cases,nativeStarCases,nativeRequirementSemantics:{helperRVA:'0x08609fd0',rosterCallRVA:'0x0842fa45',recruitCallRVA:'0x0842f943',rule:'Minimum absolute distance to anchor; first native grade ordinal wins exact/tied values; disabled negative values are not skipped.',gradeKeys,FTCOnlyBandEquivalent:false,exactCollegeBandsRequire:'Test28 DLL replaces native roster and portal base requirements with explicit floor of OVR field4, preserving native contextual adjustment.'},nativeCurves,originalSharedGradeRowsPreserved:true,playingStyleAllTablesAndPointersPreserved:true,assetsUnchanged:true,unlistedTableBytesUnchanged:true,live_game_verified:false,requiredRuntime:['Shared explicit-floor OVR college roster and portal requirement override','Player-specific75/25 CoachPrestige','PlayingTime opportunity score and class timing','ProPotential history/tie/no-evidence grading','Alaska-to-Seattle proximity grade rule']};
 const audit=path.join(out,'Audit/Test28');fs.mkdirSync(audit,{recursive:true});fs.writeFileSync(path.join(audit,`ftc-route-${route}.json`),JSON.stringify(result,null,2)+'\n');
 console.log(JSON.stringify({route:Number(route),status:result.status,sha256:result.sha256,unchangedTables:unchanged,intendedFloorPolicyAndAcquisitionCases:cases,nativeStarCases,streamingSupported,LSAdded:true}));
})().catch(e=>{console.error(e.stack);process.exit(1)});
