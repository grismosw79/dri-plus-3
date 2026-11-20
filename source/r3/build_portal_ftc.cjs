'use strict';
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),crypto=require('crypto');
// Usage: node build_portal_ftc.cjs ROUTE OFFER_COUNT_FIX_ROOT OUTPUT_ROOT
// OFFER_COUNT_FIX_ROOT must contain the original Test01 frantkdata/ directory.
const api=require('./save_adapter.cjs');
const route=Number(process.argv[2]);assert(route>=0&&route<5);
assert(process.argv[3]&&process.argv[4],'Supply baseline and output directories');
const base=path.resolve(process.argv[3]),out=path.resolve(process.argv[4]);
assert(base!==out,'Keep the baseline separate from the output');
const rel=`frantkdata/${route}/dynasty-tuning-binary.FTC`,source=path.join(base,rel),dest=path.join(out,rel);
const open=p=>api.create(p,{gameYearOverride:27,gameTypeOverride:'college',autoUnempty:false,saveOnChange:false});
const number=v=>typeof v==='string'?parseInt(v,2):v;
const hash=b=>crypto.createHash('sha256').update(b).digest('hex');
async function array(f,v){const n=number(v),t=f.getTableById(n>>>17);assert(t&&t.header.isArray);await t.readRecords();const row=n&131071,count=t.data.readUInt32BE(t.header.headerSize+row*4);assert(!t.records[row].isEmpty);return t.records[row].fieldsArray.slice(0,count).map(x=>t.records[row][x.key]);}
async function curve(f,v){const n=number(v),t=f.getTableById(n>>>17);assert.equal(t.header.uniqueId,1100607787);await t.readRecords();const r=t.records[n&131071];assert(!r.isEmpty);const x=await array(f,r.CalculateY),y=await array(f,r.Y);assert.equal(x.length,y.length);assert(x.length>1&&x.every((v,i)=>Number.isInteger(v)&&(i===0||v>x[i-1])));return {reference:n,table:t.header.tableId,row:r.index,x,y};}
function interp(c,x){if(x<=c.x[0])return c.y[0];for(let i=1;i<c.x.length;i++)if(x<=c.x[i])return Math.trunc(c.y[i-1]+(x-c.x[i-1])*(c.y[i]-c.y[i-1])/(c.x[i]-c.x[i-1]));return c.y.at(-1);}
(async()=>{
 const f=await open(source),before=new Map(f.tables.map(t=>[t.header.uniqueId,{data:api.Buffer.from(t.data),header:{...t.header},id:t.header.tableId}])),assets=JSON.stringify(f.assetTable);
 const rt=f.getTableByUniqueId(3880326652);await rt.readRecords();const hs=f.getTableByUniqueId(3747201733);await hs.readRecords();
 const active=number(rt.records[0].HighSchoolRecruitScoreTunable);assert.equal(active>>>17,hs.header.tableId);assert.equal(active&131071,0);
 const portal=number(rt.records[0].TransferRecruitScoreTunable),pt=f.getTableById(portal>>>17);assert.equal(pt.header.uniqueId,195834476);
 await pt.readRecords();assert.equal(portal&131071,1);const r=pt.records[1],field=r.fieldsArray.find(x=>x.key==='OfferCountScoreWeightSpline'||x.key==='Field_5');
 const result={route,source:rel,source_sha256:hash(fs.readFileSync(source)),activeHSReference:active,activePortalReference:portal,scope:'Portal offer-count score contribution only; HS offer-count fix retained',changed:false};
 let expected=before.get(pt.header.uniqueId).data;
 if(field){
  assert([0,3,4].includes(route));assert.equal(r.fieldsArray.length,11);assert.equal(field.offset.index,5);assert.equal(field.offset.offset,96);assert.equal(field.offset.length,32);assert(!field.offset.valueInSecondTable&&!field.offset.valueInThirdTable);
  const original=number(r[field.key]);const oldCurve=await curve(f,original);assert.deepEqual(oldCurve.x,[0,1,2,8,12]);assert.deepEqual(oldCurve.y,[0,50,100,125,200]);
  const st=f.getTableByUniqueId(1100607787),zeroRef=(st.header.tableId<<17)+660,zeroCurve=await curve(f,zeroRef);assert.deepEqual(zeroCurve.x,[0,2,6,8,10,12,14,16,18,20,24]);assert(zeroCurve.y.every(v=>v===0));
  const offset=pt.header.table1StartIndex+pt.header.record1Size+field.offset.offset/8;expected=api.Buffer.from(pt.data);assert.equal(expected.readUInt32BE(offset),original);expected.writeUInt32BE(zeroRef,offset);
  await pt.replaceRawData(expected,true);await pt.readRecords();assert.equal(number(pt.records[1][field.key]),zeroRef);
  result.changed=true;result.edit={table:pt.name,uid:pt.header.uniqueId,row:1,field:field.key,nativeField:'OfferCountScoreWeightSpline',byteOffset:offset,before:original,after:zeroRef};result.oldCurve=oldCurve;result.zeroCurve=zeroCurve;
  const countCurve=await curve(f,pt.records[1].Field_4);let checks=0;for(let calendar=0;calendar<=40;calendar++)for(let offers=0;offers<=138;offers++){assert.equal(Math.trunc(interp(countCurve,offers)*interp(zeroCurve,calendar)/100),0);checks++;}result.zeroContributionCases=checks;
  fs.mkdirSync(path.dirname(dest),{recursive:true});await f.save(dest,{sync:true});
 }else{
  assert([1,2].includes(route));assert.equal(r.fieldsArray.length,9);assert(!r.fieldsArray.some(x=>/OfferCount/.test(x.key)));result.reason='This route has the older nine-field portal scoring schema and no offer-count score fields; copied byte-for-byte.';fs.mkdirSync(path.dirname(dest),{recursive:true});fs.copyFileSync(source,dest);
 }
 const v=await open(dest);assert.equal(v.tables.length,before.size);assert.equal(JSON.stringify(v.assetTable),assets);let same=0;
 for(const t of v.tables){const b=before.get(t.header.uniqueId);assert(b);assert.equal(t.header.tableId,b.id);assert.deepEqual(t.header,b.header);assert(t.data.equals(t.header.uniqueId===195834476?expected:b.data),`Unexpected table change: ${t.name}`);if(t.data.equals(b.data))same++;}
 const vh=v.getTableByUniqueId(3747201733);await vh.readRecords();for(let row=0;row<hs.records.length;row++)for(const fld of hs.records[row].fieldsArray)assert.deepEqual(vh.records[row][fld.key],hs.records[row][fld.key]);
 if(result.changed){const vp=v.getTableByUniqueId(195834476);await vp.readRecords();const z=await curve(v,vp.records[1][field.key]);assert.deepEqual(z,result.zeroCurve);}else assert(fs.readFileSync(source).equals(fs.readFileSync(dest)));
 const vr=v.getTableByUniqueId(3880326652);await vr.readRecords();for(const [key,value] of Object.entries({MaxRecruitingBoardTargets:50,MaxTeamScholarshipOffers:50,RecruitsToGenerateMinimum:3200,RecruitsToGenerateMaximum:3200,MaxFiveStarsInClass:32}))assert.equal(vr.records[0][key],value);
 assert.equal(vh.records[0].TeamNeedsBoost??vh.records[0].Field_10,5);assert(vh.data.equals(before.get(3747201733).data));const vp=v.getTableByUniqueId(195834476);await vp.readRecords();assert.equal(vp.records[1].TeamNeedsBoost??vp.records[1].Field_10,100);
 result.status='STATIC_PASS_LIVE_TEST_PENDING';result.sha256=hash(fs.readFileSync(dest));result.tables=before.size;result.unchangedTables=same;result.assetDirectoryUnchanged=true;result.tableLayoutsUnchanged=true;result.highSchoolTableUnchanged=true;result.onlyIntendedFieldChanged=result.changed;result.allExistingCurvesByteIdentical=true;
 fs.mkdirSync(path.join(out,'Audit'),{recursive:true});fs.writeFileSync(path.join(out,`Audit/route_${route}_validation.json`),JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result));
})().catch(e=>{console.error(e.stack);process.exit(1)});
