#include "hs34r3_continuity.h"
/* Called only at a safe end-of-batch boundary. Reads at first reconstruction
   are optional: unavailable or mismatched sidecars never disable recruiting. */
#ifdef DRI_EMULATION_TEST
static int hs34r3_test_io;static Hs34R3Disk hs34r3_test_disk;
#endif
static int hs34r3_key(Rec team,U eval,Hs34R3Disk *d) {
 Rec f=context_ref(eval,6),s=f?reference(f,44):0,a=board_array(team);
 if(!f||!s||!a){release(a);release(s);release(f);return 0;}
 memset(d,0,sizeof(*d));d->magic=UINT32_C(0x33505348);d->version=1;
 d->league=scalar(f,28,0x100);d->year=scalar(s,2,0x100);d->type=scalar(s,5,0x108);d->week=scalar(s,4,0x100);d->tid=scalar(team,383,0x100);
 release(s);release(f);unsigned n=(unsigned)array_count(a);
 if(!d->league||d->year<1900||d->year>2200||d->type>6||d->week>60||d->tid>=138||n>50){release(a);return 0;}
 /* Sort identity hashes so a native board permutation is not a reload loss.
    Hours/actions are excluded: native reset may precede the first new plan.
    The complete school-interest list catches changed competitive outcomes. */
 uint64_t rows[50];int ok=1;
 for(unsigned i=0;i<n&&ok;i++) {
  Rec t=array_at(a,i),r=t?reference(t,10):0,p=r?reference(r,5):0,top=0;
  U count=0;
  if(!t||!r||!p||!hs_read_school_list(r,scalar(r,9,0x108),&top,&count)||count>64){ok=0;release(top);release(p);release(r);release(t);break;}
  uint64_t h=UINT64_C(1469598103934665603);
  h=hs34r3_hash(h,row(r));h=hs34r3_hash(h,row(p));h=hs34r3_hash(h,scalar(p,3,0x108));
  h=hs34r3_hash(h,scalar(p,191,0x100));h=hs34r3_hash(h,scalar(p,231,0x108));
  h=hs34r3_hash(h,scalar(r,2,0x108));h=hs34r3_hash(h,scalar(r,3,0x100));h=hs34r3_hash(h,scalar(r,9,0x108));
  h=hs34r3_hash(h,scalar(t,12,0x108));h=hs34r3_hash(h,count);
  for(unsigned j=0;j<count;j++) {Rec e=array_at(top,j);if(!e){ok=0;break;}h=hs34r3_hash(h,scalar(e,0,0x100));h=hs34r3_hash(h,scalar(e,1,0x100));release(e);}
  release(top);release(p);release(r);release(t);rows[i]=h;
 }
 release(a);if(!ok)return 0;
 for(unsigned i=1;i<n;i++){uint64_t v=rows[i];unsigned j=i;while(j&&rows[j-1]>v){rows[j]=rows[j-1];j--;}rows[j]=v;}
 d->board_hash=hs34r3_hash(UINT64_C(1469598103934665603),n);
 for(unsigned i=0;i<n;i++)d->board_hash=hs34r3_hash(d->board_hash,rows[i]);return 1;
}
static int hs34r3_read(Hs34R3Disk *out,const Hs34R3Disk *key);
/* R4: validate every cold CPU school's disk before maintenance changes any
   recruit on any board. Retain numeric roles, not a selected/funded plan.
   The ordinary planner still rebinds current records and revalidates every
   prior. This staging is confined to the exact current session/week/phase;
   it neither relaxes the on-disk fingerprint nor searches older files. */
typedef struct {Hs28TeamPlan scope;Hs34R3Disk disk;} Hs34R4Preloaded;
static Hs34R4Preloaded hs34r4_preloaded[138];
static void hs34r4_preload_reset(void){memset(hs34r4_preloaded,0,sizeof(hs34r4_preloaded));}
static void hs34r4_preload(U eval) {
 if(mode!=1||faulted)return;
 hs34r4_preload_reset();
 unsigned loaded=0,warm=0,unavailable=0;
 for(unsigned tid=0;tid<138&&!faulted;tid++) {
  Rec team=portal_team_by_id(eval,tid);if(!team)continue;
  Rec user=reference(team,407);if(user){release(user);release(team);continue;}
  Hs28TeamPlan scope;Hs34R3Disk key,d;const char *reason=0;
  if(hs28_plan_load(&scope,team,eval)){warm++;release(team);continue;}
  if(!hs28_scope(&scope,team,eval)||!hs34r3_key(team,eval,&key))reason="KEY_UNAVAILABLE";
  else if(!hs34r3_read(&d,&key))reason="READ_UNAVAILABLE";
  else if(!hs34r3_disk_valid(&d,key.league,key.year,key.type,key.week,key.tid,key.board_hash))reason="VALIDATION_REJECTED";
  if(reason){unavailable++;log_line("TEST34R4_PLAN_PRELOAD,%u,%s",tid,reason);}
  else {
   Hs34R4Preloaded *p=&hs34r4_preloaded[tid];p->scope=scope;p->disk=d;p->scope.valid=1;loaded++;
   log_line("TEST34R4_PLAN_PRELOAD,%u,%u,%016llx,VALIDATED_BEFORE_MAINTENANCE",tid,d.count,(unsigned long long)d.board_hash);
  }
  release(team);
 }
 log_line("TEST34R4_PLAN_PRELOAD_COMPLETE,%u,%u,%u,LOADED_WARM_UNAVAILABLE",loaded,warm,unavailable);
}
static int hs34r4_preloaded_disk(Hs34R3Disk *d,Rec team,U eval) {
 unsigned tid=scalar(team,383,0x100);Hs28TeamPlan now={0};
 if(tid>=138||!hs34r4_preloaded[tid].scope.valid||!hs28_scope(&now,team,eval))return 0;
 const Hs34R4Preloaded *p=&hs34r4_preloaded[tid];
 if(!hs28_scope_matches(&p->scope,&now)||p->scope.week!=now.week||p->scope.type!=now.type)return 0;
 Rec f=context_ref(eval,6);int same=f&&scalar(f,28,0x100)==p->disk.league;release(f);
 if(!same)return 0;
 *d=p->disk;return 1;
}
static int hs34r3_path(wchar_t path[1024],const Hs34R3Disk *d) {
 DWORD n=GetModuleFileNameW(self_module,path,800);if(!n||n>=800)return 0;
 while(n&&path[n-1]!=L'\\')n--;
 if(swprintf(path+n,1024-n,L"HS-Plan-Cache")<0)return 0;
 if(!CreateDirectoryW(path,0)&&GetLastError()!=ERROR_ALREADY_EXISTS)return 0;
 n+=(DWORD)wcslen(L"HS-Plan-Cache");
 int z=swprintf(path+n,1024-n,L"\\%08x-%u-%u-%016llx.bin",d->league,d->year,d->tid,(unsigned long long)d->board_hash);
 return z>0&&(unsigned)z<1024-n;
}
static int hs34r3_read(Hs34R3Disk *out,const Hs34R3Disk *key) {
#ifdef DRI_EMULATION_TEST
 if(hs34r3_test_io){*out=hs34r3_test_disk;return 1;}
#endif
 wchar_t path[1024];if(!hs34r3_path(path,key))return 0;
 HANDLE h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return 0;
 LARGE_INTEGER size;DWORD got=0;int ok=GetFileSizeEx(h,&size)&&size.QuadPart==sizeof(*out)&&ReadFile(h,out,sizeof(*out),&got,0)&&got==sizeof(*out);CloseHandle(h);return ok;
}
static int hs34r3_write(const Hs34R3Disk *d) {
#ifdef DRI_EMULATION_TEST
 if(hs34r3_test_io){hs34r3_test_disk=*d;return 1;}
#endif
 wchar_t path[1024],tmp[1100];if(!hs34r3_path(path,d)||swprintf(tmp,1100,L"%ls.%lu.tmp",path,(unsigned long)GetCurrentProcessId())<0)return 0;
 HANDLE h=CreateFileW(tmp,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(h==INVALID_HANDLE_VALUE)return 0;
 DWORD put=0;int ok=WriteFile(h,d,sizeof(*d),&put,0)&&put==sizeof(*d);CloseHandle(h);
 if(ok)ok=MoveFileExW(tmp,path,MOVEFILE_REPLACE_EXISTING)!=0;
 if(!ok)DeleteFileW(tmp);return ok;
}
static int hs34r3_restore(Hs28TeamPlan *plan,Hs28Choice *pool,int n,Rec team,U eval,const Hs15Snapshot *s) {
 Hs34R3Disk key,d;int preloaded=hs34r4_preloaded_disk(&d,team,eval);
 if(!preloaded&&(!hs34r3_key(team,eval,&key)||!hs34r3_read(&d,&key)||
  !hs34r3_disk_valid(&d,key.league,key.year,key.type,key.week,key.tid,key.board_hash)))return 0;
 memset(plan,0,sizeof(*plan));if(!hs28_scope(plan,team,eval))return 0;
 for(unsigned j=0;j<d.count;j++) {
  const Hs34R3Entry *e=&d.entry[j];Hs28PlanEntry *p=&plan->entry[plan->count++];
  p->prior=(Hs28Prior){0,e->rid,e->pos,e->slot,e->choice,e->order,0,0,0};p->player_row=e->pid;
  for(int i=0;i<n;i++)if(pool[i].rid==e->rid&&pool[i].player_row==e->pid&&pool[i].pos==e->pos){
   p->prior.table=pool[i].table;p->prior.native_score=pool[i].native_score;p->player_table=pool[i].player_table;break;
  }
  for(int i=0;i<s->used;i++)if((s->players[i].hs||s->players[i].published)&&s->players[i].rid==e->pid)p->player_table=s->players[i].table;
 }
 plan->valid=1;log_line("TEST34R3_PLAN_RESTORE,%u,%u,%016llx,%s",d.tid,d.count,(unsigned long long)d.board_hash,
  preloaded?"VALIDATED_BEFORE_MAINTENANCE":"MATCHED_NATIVE_BOARD");return 1;
}
/* Migration/no-sidecar fallback is deliberately narrow: a real offered
   native gain, at least half the threshold, materially more progress, and
   no lower OVR than the current cold primary. Ambiguous pairs use the normal
   selector. This recognizes the observed Cutrera route without an ID rule. */
static int hs34r3_reconstruct(Hs28Choice *pool,int n,const int vacancy[22],const int optional[22],
 int general,int capacity,Hs28Prior prior[50],int np) {
 Hs28Choice cold[50];if(n>50||np)return np;memcpy(cold,pool,(size_t)n*sizeof(*cold));
 hs28_plan(cold,n,vacancy,optional,general,capacity);
 for(int i=0;i<n&&np<50;i++)if(cold[i].selected&&cold[i].choice==1) {
  int best=-1;
  for(int j=0;j<n;j++)if(j!=i&&!(cold[j].selected&&cold[j].choice==1)&&pool[j].pos==pool[i].pos&&pool[j].existing&&pool[j].offered&&pool[j].target&&
    pool[j].legal&&pool[j].viable&&pool[j].native_score>0&&hs28_market_primary_allowed(&pool[j])&&!pool[j].crowd_penalty&&
    pool[j].ovr>=pool[i].ovr&&pool[j].commit_score>0&&(int64_t)pool[j].points*2>=pool[j].commit_score&&
    pool[j].points>=pool[i].points+50+pool[j].commit_score/10&&
    (int32_t)scalar((Rec)pool[j].target,7,0x100)>0) {
   int already=0;for(int k=0;k<np;k++)if(prior[k].rid==pool[j].rid||prior[k].slot==cold[i].slot)already=1;
   if(!already&&(best<0||pool[j].points>pool[best].points||(pool[j].points==pool[best].points&&hs28_better(&pool[j],&pool[best]))))best=j;
  }
  if(best>=0){const Hs28Choice *p=&pool[best];prior[np++]=(Hs28Prior){p->table,p->rid,p->pos,cold[i].slot,1,cold[i].order,p->native_score,0,0};}
 }
 return np;
}
static void hs34r3_checkpoint(U eval) {
 if(mode!=1||faulted)return;
 for(unsigned tid=0;tid<138&&!faulted;tid++) {
  Rec team=portal_team_by_id(eval,tid);if(!team)continue;
  Hs28TeamPlan plan;Hs34R3Disk d;
  if(hs28_plan_load(&plan,team,eval)&&plan.count>0&&hs34r3_key(team,eval,&d)) {
   for(int i=0;i<plan.count;i++){const Hs28PlanEntry *p=&plan.entry[i];d.entry[d.count++]=(Hs34R3Entry){p->prior.rid,p->player_row,p->prior.pos,p->prior.slot,p->prior.choice,p->prior.order};}
   d.checksum=hs34r3_checksum(&d);
   if(hs34r3_disk_valid(&d,d.league,d.year,d.type,d.week,d.tid,d.board_hash)&&!hs34r3_write(&d))log_line("TEST34R3_PLAN_CACHE,%u,WRITE_UNAVAILABLE_NATIVE_PLAN_RETAINED",tid);
  }
  release(team);
 }
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTest34R3CacheIO(int on){hs34r3_test_io=on;memset(&hs34r3_test_disk,0,sizeof(hs34r3_test_disk));}
__declspec(dllexport) void DriTest34R3Checkpoint(U eval){hs34r3_checkpoint(eval);}
__declspec(dllexport) void DriTest34R3CacheCorrupt(int field){if(field==0)hs34r3_test_disk.checksum^=1;else if(field==1)hs34r3_test_disk.league++;else if(field==2)hs34r3_test_disk.week++;}
__declspec(dllexport) void DriTest34R4Preload(U eval){hs34r4_preload(eval);}
__declspec(dllexport) int DriTest34R4Preloaded(Rec team,U eval){Hs34R3Disk d;return hs34r4_preloaded_disk(&d,team,eval);}
#endif
