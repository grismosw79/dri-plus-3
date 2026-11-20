/* Test34 ordinary class replenishment. Ranked search resumes around each
   position's numeric cursor, checks at most44 native candidates per team/pass,
   and never mistakes its work limit for proof that no candidate exists.
   Post-week work queues numeric identities for NEXT allocation only. */
#define CLASS34_CHECKS 44
#define CLASS34_PICK 44
typedef struct {U table;uint32_t rid;uint64_t seen[2][3],queued[3];} Class34Identity;
typedef struct {int valid,value;U table;uint32_t rid;} Class34Cursor;
typedef struct {int index,value,identity,tier;} Class34Pick;
static struct {
 int valid,count;U table,handle;uint32_t row,year,week,type;
 int hash[HS28_MARKET_HASH];Class34Identity identity[HS28_MARKET_LIMIT];
 Class34Cursor cursor[138][22];unsigned next_position[138];
} class34_search;
static void test34_class_market_reset(int previous_valid) {
 if(!previous_valid)memset(&class34_search,0,sizeof(class34_search));
}
static void class34_scope(void) {
 int same=class34_search.valid && class34_search.table==hs28_market.table &&
  class34_search.handle==hs28_market.handle && class34_search.row==hs28_market.row &&
  class34_search.year==hs28_market.year && hs28_market.type>=class34_search.type &&
  (hs28_market.type!=class34_search.type || hs28_market.week>=class34_search.week);
 if(!same)memset(&class34_search,0,sizeof(class34_search));
 if(!same || class34_search.week!=hs28_market.week || class34_search.type!=hs28_market.type)
  for(int i=0;i<class34_search.count;i++)memset(class34_search.identity[i].seen,0,sizeof(class34_search.identity[i].seen));
 class34_search.valid=1;class34_search.table=hs28_market.table;class34_search.handle=hs28_market.handle;
 class34_search.row=hs28_market.row;class34_search.year=hs28_market.year;
 class34_search.week=hs28_market.week;class34_search.type=hs28_market.type;
}
static int class34_identity(U table,uint32_t rid) {
 unsigned h=(unsigned)((table>>4)^((U)rid*2654435761u))&(HS28_MARKET_HASH-1);
 for(int probe=0;probe<HS28_MARKET_HASH;probe++,h=(h+1)&(HS28_MARKET_HASH-1)) {
  int i=class34_search.hash[h];
  if(i) {Class34Identity *v=&class34_search.identity[i-1];if(v->table==table && v->rid==rid)return i-1;}
  else {
   if(class34_search.count>=HS28_MARKET_LIMIT)return -1;
   i=class34_search.count++;class34_search.identity[i].table=table;class34_search.identity[i].rid=rid;
   class34_search.hash[h]=i+1;return i;
  }
 }
 return -1;
}
static int class34_bit(const uint64_t bits[3],unsigned tid) {return !!(bits[tid/64]&(UINT64_C(1)<<(tid%64)));}
static void class34_set(uint64_t bits[3],unsigned tid,int value) {
 uint64_t mask=UINT64_C(1)<<(tid%64);if(value)bits[tid/64]|=mask;else bits[tid/64]&=~mask;
}
static int class34_after(const Class34Cursor *c,int value,const Class34Identity *v) {
 return !c->valid || value<c->value || (value==c->value &&
  (v->rid>c->rid || (v->rid==c->rid && v->table>c->table)));
}
static int class34_better(const Class34Pick *a,const Class34Pick *b) {
 if(b->index<0)return 1;
 if(a->tier!=b->tier)return a->tier<b->tier;
 if(a->value!=b->value)return a->value>b->value;
 const Class34Identity *x=&class34_search.identity[a->identity],*y=&class34_search.identity[b->identity];
 return x->rid<y->rid || (x->rid==y->rid && x->table<y->table);
}
static void class34_seed(U eval,int future) {
 if(mode!=1 || faulted || !hs28_pair_window(eval) || !hs28_market_scope(eval,0) || !hs28_market.ready)return;
 class34_scope();
 Hs15RecoveryCache shots={0};int need[138][22]={0},active[138]={0},left[138]={0};
 int count[138][22]={0};
 Class34Pick (*pick)[22][CLASS34_PICK]=HeapAlloc(GetProcessHeap(),0,138*sizeof(*pick));
 if(!pick){log_line("TEST34_CLASS_SEARCH_DEFERRED,ALLOCATION_FAILED");return;}
 for(int t=0;t<138;t++)for(int p=0;p<22;p++)for(int k=0;k<CLASS34_PICK;k++)pick[t][p][k]=(Class34Pick){-1,0,0,0};
 int teams=0,checked=0,reserved=0,queued=0;
 for(unsigned tid=0;tid<138;tid++) {
  Rec team=portal_team_by_id(eval,tid);if(!team)continue;
  Rec user=reference(team,407);if(user){release(user);release(team);continue;}
  Rec budget=reference(team,224);unsigned total=budget?scalar(budget,3,0x100):0,assigned=budget?scalar(budget,1,0x100):0;release(budget);
  /* End-of-week spent hours cannot fund a new pursuit. They also must not
     prevent preparing a recheck queue for the next allocation boundary. */
  if(!future && assigned>=total){release(team);continue;}
  Hs15Snapshot *s=hs15_recovery_snapshot(&shots,team,eval);release(team);if(!s)continue;
  int secured=0;for(int p=0;p<22;p++)secured+=s->hs[p];
  /* Acquired extras retain their own bounded capacity. They cannot erase
     unfilled ordinary slots merely by bringing total intake up to base. */
  int target=class34r2_capacity(s->annual_valid,s->annual_valid?s->annual_target:test29_class_floor(tid),
   class34r2_secured_extra(s->annual_valid,s->annual,s->hs),0);
  left[tid]=target-secured;if(left[tid]<=0)continue;
  for(int p=0;p<22;p++)if((s->annual_valid?class31_remaining(s->annual,s->hs,p)>0:s->count[p]<s->ideal[p]) || hs34_vacancy_extra(s,p)){need[tid][p]=1;active[tid]=1;}
  teams+=active[tid];
 }
 if(!teams){HeapFree(GetProcessHeap(),0,pick);hs15_recovery_cache_free(&shots);return;}
 Rec f=context_ref(eval,6),pool=f?array_field(f,39):0;release(f);
 if(!pool || array_count(pool)>HS28_MARKET_LIMIT){release(pool);HeapFree(GetProcessHeap(),0,pick);hs15_recovery_cache_free(&shots);return;}
 U n=array_count(pool);
 for(U i=0;i<n && !faulted;i++) {
  Rec r=array_at(pool,i);if(!r)continue;
  unsigned stage=scalar(r,9,0x108);
  if(!hs15_is_recruit(r) || stage>=5 || (!future && hs28_recovery_pending(r,eval))){release(r);continue;}
  Rec p=reference(r,5);int pos=p?(int)scalar(p,3,0x108):-1,stars=p?(int)scalar(p,231,0x108):-1,ovr=p?(int)scalar(p,191,0x100):-1;release(p);
  if(pos<0 || pos>=22 || stars<0 || stars>2 || ovr<0 || ovr>99){release(r);continue;}
  int rank[138]={0},points[138]={0},leader=0,valid=1;
  Rec top;U nt;
  if(!hs_read_school_list(r,stage,&top,&nt) || nt>64)valid=0;
  for(U j=0;j<nt && valid;j++) {
   Rec e=array_at(top,j);unsigned tid=e?scalar(e,0,0x100):138,value=e?scalar(e,1,0x100):0;release(e);
   if(tid>=138 || value>65535 || rank[tid]){valid=0;break;}
   rank[tid]=(int)j+1;points[tid]=(int)value;if((int)value>leader)leader=(int)value;
  }
  release(top);int threshold=(int)scalar(r,3,0x100),id=valid?class34_identity(q((U)r+0x10),row(r)):-1;
  if(id>=0)for(unsigned tid=0;tid<138;tid++)if(active[tid] && need[tid][pos]) {
   Class34Identity *v=&class34_search.identity[id];if(class34_bit(v->seen[future],tid))continue;
   int early=stage==0 && (!leader || (threshold>0 && (int64_t)leader*100<(int64_t)threshold*35));
   int plausible=early || (rank[tid] && (rank[tid]<=(stage==0?5:3) || (int64_t)points[tid]*4>=(int64_t)leader*3));
   if(!plausible)continue;
   int value=(rank[tid]?100000:0)+points[tid]*2+ovr;
   Class34Pick candidate={(int)i,value,id,(!future && class34_bit(v->queued,tid))?0:class34_after(&class34_search.cursor[tid][pos],value,v)?1:2};
   count[tid][pos]++;
   for(int k=0;k<CLASS34_PICK;k++)if(class34_better(&candidate,&pick[tid][pos][k])) {
    for(int j=CLASS34_PICK-1;j>k;j--)pick[tid][pos][j]=pick[tid][pos][j-1];
    pick[tid][pos][k]=candidate;break;
   }
  }
  release(r);
 }
 unsigned offset=(hs28_market.year+hs28_market.week)%138;
 hs28_recovery_cache_invalidate();
 for(unsigned turn=0;turn<138 && !faulted;turn++) {
  unsigned tid=(offset+turn)%138;if(!active[tid])continue;
  Rec team=portal_team_by_id(eval,tid);if(!team)continue;
  Hs15Snapshot *s=shots.team[tid];int attempts=0,added=0,staged=0,consumed=0,k[22]={0},staged_pos[22]={0};
  unsigned first=class34_search.next_position[tid]%22;
  /* One candidate per position per round prevents a long QB list from
     consuming every check before a needy specialist is considered. */
  for(unsigned visit=0;visit<22*CLASS34_PICK && attempts<CLASS34_CHECKS && left[tid]>0;visit++) {
   unsigned pos=(first+visit)%22;
   if(!need[tid][pos] || k[pos]>=CLASS34_PICK || (future && staged_pos[pos]>=2))continue;
   Class34Pick *candidate=&pick[tid][pos][k[pos]++];if(candidate->index<0)continue;
   Class34Identity *v=&class34_search.identity[candidate->identity];
   class34_set(v->seen[future],tid,1);if(!future)class34_set(v->queued,tid,0);consumed++;
   class34_search.cursor[tid][pos]=(Class34Cursor){1,candidate->value,v->table,v->rid};
   Rec r=array_at(pool,(U)candidate->index);if(!r)continue;
   if(q((U)r+0x10)!=v->table || row(r)!=v->rid){release(r);portal_fault("class34_pool_identity");break;}
   if((!future && hs28_recovery_pending(r,eval)) || locked_out(r,team,eval)){release(r);continue;}
   int owner=-1,published=portal_published_owner(r,eval,&owner);
   if(published){release(r);if(published<0)portal_fault("class34_owner");continue;}
   Hs28Choice choice;int score=0,slot=0,role=0;attempts++;checked++;
   class34_search.next_position[tid]=(pos+1)%22;
   hs34_trace_begin(team,r);
   int legal=hs28_choice(&choice,r,0,team,eval,s,0) && choice.legal && choice.viable;
   if(choice.legal && !choice.viable)hs34_trace_note(team,r,HS34_VIABILITY);
   if(future) {
    /* Completed native turns are never reopened. Queue only identity and
       ranking intent; all quota/admission/budget checks run again next scope. */
    if(legal && hs28_native_score(r,team,eval,&score) && score>0) {
     class34_set(v->queued,tid,1);staged++;queued++;staged_pos[pos]++;
     hs34_trace_note(team,r,HS34_NEXT_SCOPE);
     log_line("TEST34_CLASS_QUEUE,%u,%u,%d,NEXT_ALLOCATION_RECHECK_REQUIRED",tid,row(r),score);
    } else if(legal)hs34_trace_note(team,r,HS34_SCORE);
   } else {
    int ok=legal?hs28_recovery_destination(r,team,eval,s,&score,&slot,&role):0;
    int normal=s->annual_valid?class31_remaining(s->annual,s->hs,pos)>0:s->count[pos]<s->ideal[pos];
    if(ok>0 && role!=1)hs34_trace_note(team,r,HS34_BACKUP_ONLY);
    if(ok>0 && role==1 && ((slot%32!=31 && normal) || (slot%32==31 && hs34_vacancy_extra(s,pos))) && hs28_recovery_reserve(team,r,eval,slot,role)>0) {
     added++;reserved++;left[tid]--;hs34_trace_note(team,r,HS34_ACCEPTED);
     log_line("TEST29R2_CLASS_RECOVERY,%u,%u,%d,%d,RESERVED_NATIVE_PURSUIT",tid,row(r),score,slot);
    }
   }
   hs34_trace.score=score;hs34_trace.slot=slot;hs34_trace.role=role;
   hs34_trace_emit(&hs34_trace,future?"CLASS_NEXT_SCOPE_CANDIDATE":"CLASS_SEARCH_RESERVATION_ONLY");hs34_trace_end();
   release(r);
  }
  int remaining=0;for(int pos=0;pos<22;pos++)remaining+=count[tid][pos];remaining-=consumed;
  log_line("TEST29R2_CLASS_SEARCH,%u,%d,%d,%d,NATIVE_OFFERS_PENDING",tid,attempts,added,left[tid]);
  log_line("TEST34_CLASS_SEARCH,%u,%d,%d,%d,%d,%s,%s",tid,attempts,added,staged,remaining,
   future?"NEXT_SCOPE_QUEUE_ONLY":"CURRENT_SCOPE_RESERVATIONS_ONLY",remaining>0?"SEARCH_PENDING":"CURRENT_PLAUSIBLE_SET_EXHAUSTED");release(team);
 }
 release(pool);HeapFree(GetProcessHeap(),0,pick);hs15_recovery_cache_free(&shots);
 log_line("TEST29R2_CLASS_SEARCH_TOTAL,%d,%d,%d,BOUNDED44_PER_TEAM",teams,checked,reserved);
 log_line("TEST34_CLASS_SEARCH_TOTAL,%d,%d,%d,%d,%s",teams,checked,reserved,queued,future?"NEXT_ALLOCATION_RECHECK_REQUIRED":"NATIVE_OFFERS_PENDING");
}
static void test29r2_class_seed(U eval){class34_seed(eval,0);}
static void test34_class_refill(U eval){class34_seed(eval,1);}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTest29R2ClassSeed(U eval){test29r2_class_seed(eval);}
__declspec(dllexport) void DriTest34ClassRefill(U eval){test34_class_refill(eval);}
__declspec(dllexport) int DriTest34ClassQueued(Rec r,unsigned tid) {
 if(tid>=138)return 0;int id=class34_identity(q((U)r+0x10),row(r));return id>=0 && class34_bit(class34_search.identity[id].queued,tid);
}
#endif
