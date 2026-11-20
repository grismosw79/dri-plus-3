'use strict';
// Guarded Test30 -> Test31 generator/pool-only transformation.
const fs=require('fs'),path=require('path'),crypto=require('crypto'),assert=require('assert/strict');
const G=require('./test29_supply_graph.cjs'),api=require(path.resolve(process.env.DRI_SAVE_PARSER));
const [routeArg,base,out,auditArg]=process.argv.slice(2),route=Number(routeArg);assert(Number.isInteger(route)&&route>=0&&route<=4);assert(base&&out&&path.resolve(base)!==path.resolve(out));
const audit=auditArg||path.join(out,'Runtime/Portal/Audit/Test31/Generator'),solution=JSON.parse(fs.readFileSync(path.join(audit,`route${route}_solution.json`)));
const hash=b=>crypto.createHash('sha256').update(b).digest('hex'),num=x=>typeof x==='string'?parseInt(x,2):x;
const open=p=>api.create(p,{gameYearOverride:27,gameTypeOverride:'college',autoUnempty:false,saveOnChange:false});
const records=f=>new Map(f.tables.map(t=>[t.header.tableId,{name:t.name,uid:t.header.uniqueId,data:Buffer.from(t.data),header:{...t.header}}]));
function writeField(data,t,r,key,value,edits){const d=r.fields[key].offset;assert(!d.valueInSecondTable&&!d.valueInThirdTable);const start=8*(t.header.table1StartIndex+r.index*t.header.record1Size)+d.offset;const before=num(r[key]);let encoded=0;for(let j=0;j<d.length;j++)encoded=encoded*2+((data[(start+j)>>3]>>(7-((start+j)&7)))&1);const bias=encoded-before;assert([0,2147483648,-d.minValue].includes(bias));const v=value+bias;assert(v>=0&&v<2**d.length);for(let j=0;j<d.length;j++){const bit=start+j,b=bit>>3,m=1<<(7-(bit&7));data[b]=(data[b]&~m)|((Math.floor(v/2**(d.length-1-j))%2)?m:0);}edits.push({table:t.name,table_id:t.header.tableId,row:r.index,field:key,before,after:value,table_bit_offset:start,bit_length:d.length,encoding_bias:bias});}
async function appendRows(t,rows,counts,log){
 const old=Buffer.from(t.data),h=t.header,cap=h.recordCapacity,size=h.record1Size,n=rows.length;assert(n&&rows.every(x=>x.length===size));assert(!h.hasThirdTable);assert.equal(h.data1RecordCount,cap);assert.equal(h.table2StartIndex,h.table1StartIndex+cap*size);
 const countBytes=h.isArray?4*n:0,growth=size*n+countBytes;let data;
 if(h.isArray){assert(counts&&counts.length===n);assert.equal(h.table1StartIndex,h.headerSize+cap*4);const c=Buffer.alloc(countBytes);counts.forEach((v,i)=>c.writeUInt32BE(v,i*4));data=Buffer.concat([old.subarray(0,h.table1StartIndex),c,old.subarray(h.table1StartIndex,h.table2StartIndex),...rows,old.subarray(h.table2StartIndex)]);}
 else data=Buffer.concat([old.subarray(0,h.table2StartIndex),...rows,old.subarray(h.table2StartIndex)]);
 const stringCapacity=h.hasSecondTable?t.records.find(r=>!r.isEmpty).fieldsArray.reduce((v,f)=>v+(f.offset.valueInSecondTable?(f.offset.maxLength||0):0),0):0;
 const emptyTailEdits=[];for(const [index,entry]of t.emptyRecords)if(entry.next===cap){const offset=h.table1StartIndex+countBytes+index*size;assert.equal(data.readUInt32BE(offset),cap);data.writeUInt32BE(cap+n,offset);emptyTailEdits.push({row:index,offset,before:cap,after:cap+n});}
 const nativeRowBaseOffset=h.offsetStart-8,nativeRowBaseWords=old.readUInt32BE(nativeRowBaseOffset);assert.equal(nativeRowBaseWords,(h.table1StartIndex-(h.offsetStart-32))/4,'Native stored row-data base disagrees with physical row start');
 const rowBaseHeader=h.isArray?[[224,nativeRowBaseWords,nativeRowBaseWords+n]]:[];
 const headers=[];for(const [off,before,after]of [[176,cap,cap+n],[184,h.table1Length,h.table1Length+growth],[204,h.table1Length2,h.table1Length2+growth],[208,h.tableTotalLength,h.tableTotalLength+growth+stringCapacity*n],[216,cap,cap+n],[228,h.nextRecordToUse,h.nextRecordToUse===cap?cap+n:h.nextRecordToUse],...rowBaseHeader]){const offset=off+h.tableStoreLength;assert.equal(data.readUInt32BE(offset),before,`${t.name} header${offset}`);data.writeUInt32BE(after,offset);headers.push({offset,before,after});}
 await t.replaceRawData(api.Buffer.from(data),true);assert.equal(t.header.recordCapacity,cap+n);assert.equal(t.data.length,old.length+growth);log.push({table_id:h.tableId,table:t.name,start:cap,count:n,record_size:size,count_bytes:countBytes,old_table1_start:h.table1StartIndex,old_table2_start:h.table2StartIndex,growth,headers,emptyTailEdits,rows_sha256:rows.map(hash),new_counts:counts||null});return cap;
}
(async()=>{
 fs.mkdirSync(audit,{recursive:true});const rel=`frantkdata/${route}/dynasty-generatordata-binary.FTC`,source=path.join(base,rel),dest=path.join(out,rel);assert.equal(hash(fs.readFileSync(source)),solution.baseline_sha256);const m=await G.inspect(source,api),f=m.f,before=records(f),assets=JSON.stringify(f.assetTable),edits=[],appends=[];const expected=new Map([...before].map(([id,x])=>[id,Buffer.from(x.data)]));
 // Weight selectors only: zero entries remain zero; schema bounded at1000.
 const weightTables=new Set();for(const [ref,newWeight]of Object.entries(solution.weight_changes)){const x=await m.ref(Number(ref));assert.equal(x.t.name,'WeightedGenerator');assert(x.r.Weight>0&&newWeight>0&&newWeight<=1000);writeField(expected.get(x.t.header.tableId),x.t,x.r,'Weight',newWeight,edits);weightTables.add(x.t.header.tableId);}
 for(const id of weightTables)await f.getTableById(id).replaceRawData(api.Buffer.from(expected.get(id)),true);
 // Two private Position setters for nonelite OT/OLB. Original elite recipes untouched.
 const templates=[];for(const [key,spec]of Object.entries(solution.nonelite_position_overrides)){
  const c=m.branches.flatMap(b=>b.children).find(c=>Object.keys(c.position.distribution).sort().join('/')===spec.positions.slice().sort().join('/'));assert(c);const p=c.position.property,s=c.position.setter,a=await m.ref(s.r.EnumValues),w=await m.array(s.r.EnumValues);assert.equal(w.length,2);assert.equal(p.r.SetOnTarget,false);templates.push({key,spec,p,s,a,w});
 }
 const first=templates[0],pt=first.p.t,st=first.s.t,at=first.a.t,wt=first.w[0].t;assert(templates.every(x=>x.p.t===pt&&x.s.t===st&&x.a.t===at&&x.w.every(w=>w.t===wt)));
 const wstart=await appendRows(wt,templates.flatMap(x=>x.spec.positions.map(p=>Buffer.from(x.w.find(w=>Number(w.r.Name)===G.POS.indexOf(p)).r.data))),null,appends);
 const astart=await appendRows(at,templates.map(x=>Buffer.alloc(at.header.record1Size)),[2,2],appends);
 const sstart=await appendRows(st,templates.map(x=>Buffer.from(x.s.r.data)),null,appends);
 const pstart=await appendRows(pt,templates.map(x=>Buffer.from(x.p.r.data)),null,appends);
 for(const t of [wt,at,st,pt])expected.set(t.header.tableId,Buffer.from(t.data));
 const overrideLeaves=new Map();for(let i=0;i<templates.length;i++){
  const x=templates[i];for(let j=0;j<2;j++)writeField(expected.get(wt.header.tableId),wt,wt.records[wstart+i*2+j],'Weight',x.spec.weights[j],edits);
  for(let j=0;j<2;j++)writeField(expected.get(at.header.tableId),at,at.records[astart+i],at.records[astart+i].fieldsArray[j].key,num(wt.getBinaryReferenceToRecord(wstart+i*2+j)),edits);
  writeField(expected.get(st.header.tableId),st,st.records[sstart+i],'EnumValues',num(at.getBinaryReferenceToRecord(astart+i)),edits);
  writeField(expected.get(pt.header.tableId),pt,pt.records[pstart+i],'Setter',num(st.getBinaryReferenceToRecord(sstart+i)),edits);
  for(const b of m.branches)for(const c of b.children)for(const s of c.starChildren)if([2,3].includes(s.tier)&&Object.keys(s.position.distribution).sort().join('/')===x.spec.positions.slice().sort().join('/')){assert(!s.override);overrideLeaves.set(s.generator.ref,{s,property:num(pt.getBinaryReferenceToRecord(pstart+i)),key:x.key});}
 }
 const propertyArrays=new Set();for(const [ref,{s,property,key}]of overrideLeaves){const a=await m.ref(s.generator.r.Properties),n=a.t.data.readUInt32BE(a.t.header.headerSize+a.r.index*4);assert(n<a.r.fieldsArray.length);assert.equal(num(a.r[a.r.fieldsArray[n].key]),0);const d=expected.get(a.t.header.tableId);writeField(d,a.t,a.r,a.r.fieldsArray[n].key,property,edits);const countOffset=a.t.header.headerSize+a.r.index*4;d.writeUInt32BE(n+1,countOffset);edits.push({table:a.t.name,table_id:a.t.header.tableId,row:a.r.index,field:'native_array_count',before:n,after:n+1,table_bit_offset:countOffset*8,bit_length:32,recipe_ref:ref,reason:key});propertyArrays.add(a.t.header.tableId);}
 assert.equal(overrideLeaves.size,16);
 for(const id of new Set([wt,at,st,pt].map(t=>t.header.tableId).concat([...propertyArrays])))await f.getTableById(id).replaceRawData(api.Buffer.from(expected.get(id)),true);
 await f.save(dest,{sync:true});const v=await G.inspect(dest,api);assert.equal(JSON.stringify(v.f.assetTable),assets);assert.equal(v.f.tables.length,before.size);
 for(const [id,e]of expected){const t=v.f.getTableById(id);assert(Buffer.from(t.data).equals(e),`Unapproved serialized delta ${t.name}`);}
 const model={pool:3000,positions:Object.fromEntries(G.POS.map(p=>[p,v.distribution[p]*3000])),stars:[1,2,3,4,5].map(t=>G.POS.reduce((n,p)=>n+v.joint[p][t-1]*3000,0)),joint:Object.fromEntries(G.POS.map(p=>[p,v.joint[p].map(v=>v*3000)]))};
 const report={route,source_sha256:solution.baseline_sha256,sha256:hash(fs.readFileSync(dest)),status:'PASS_SERIALIZATION_AND_MODEL',assets_unchanged:true,table_count:before.size,all_unlisted_bytes_unchanged:true,edits,appends,nonelite_override_leaf_count:overrideLeaves.size,model};fs.writeFileSync(path.join(audit,`route${route}_generator_build.json`),JSON.stringify(report,null,2)+'\n');
 // Native pool count; no hours/visits/star cap or other tuning edit.
 const trel=`frantkdata/${route}/dynasty-tuning-binary.FTC`,tf=await open(path.join(base,trel)),tb=records(tf),ta=JSON.stringify(tf.assetTable),tt=tf.getTableByUniqueId(3880326652);await tt.readRecords();const tr=tt.records[0],td=Buffer.from(tt.data),te=[];assert.equal(tr.MaxFiveStarsInClass,32);for(const key of ['RecruitsToGenerateMinimum','RecruitsToGenerateMaximum']){assert.equal(tr[key],3200);writeField(td,tt,tr,key,3000,te);}await tt.replaceRawData(api.Buffer.from(td),true);await tf.save(path.join(out,trel),{sync:true});const tv=await open(path.join(out,trel));assert.equal(JSON.stringify(tv.assetTable),ta);for(const [id,b]of tb){const t=tv.getTableById(id);assert(Buffer.from(t.data).equals(id===tt.header.tableId?td:b.data));}const tvr=tv.getTableByUniqueId(3880326652);await tvr.readRecords();assert.equal(tvr.records[0].MaxFiveStarsInClass,32);const tuning={route,source_sha256:hash(fs.readFileSync(path.join(base,trel))),sha256:hash(fs.readFileSync(path.join(out,trel))),all_unlisted_bytes_unchanged:true,edits:te,MaxFiveStarsInClass:32};fs.writeFileSync(path.join(audit,`route${route}_tuning_build.json`),JSON.stringify(tuning,null,2)+'\n');
 console.log(JSON.stringify({route,generator_sha256:report.sha256,tuning_sha256:tuning.sha256,edits:edits.length,appended_rows:appends.reduce((n,a)=>n+a.count,0),model}));
})().catch(e=>{console.error(e.stack);process.exit(1)});
