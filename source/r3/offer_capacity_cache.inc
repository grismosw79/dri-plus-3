/* Cache stack: the offer-capacity count for new CPU scholarship attempts (DriOnOfferAction only).
   test32_hs_offer_capacity_scan reads every school's whole board to count one recruit's offers, for every attempt;
   one school's consecutive attempts re-read 137 boards that have not changed.
   One league snapshot of the OTHER schools' boards (per recruit: which schools list it, which offer it, duplicates,
   board validity) serves a run of one school's attempts; the applicant's own board, where the native publishes the
   new offers, is read live on every call. Same result, same count, same TEST36_ACTUAL_CAP row.
   The snapshot is rebuilt when the applicant (id and Team record), eval, franchise or team list changes, and when
   cache.offer_epoch has moved: every hook entry advances it except the offer action itself, the read-only grade
   hooks, and the five team-scoped hooks the native enters inside one school's own action loop (lockout check, PP
   group grade, visit eligibility, the two executed-action witnesses), which leave it only when their own Team is
   the cached applicant (offer_cap_team, the first statement after their cache_hook_entry). Their DRI bodies write
   no board, target link or status, team id, user flag or team list.
   It assumes that inside one school's own action loop the native changes no other school's board without entering
   a hook that ends the run. Opening publication and pursuit code (which can publish offers between checks) always
   run the scan. shadow: the scan decides; result, count and own offer are compared (OFFER_CAPACITY). */
#define OFFER_CAP_SLOTS 16384
typedef struct {U table;uint32_t row,gen;uint64_t offered[3],found[3],dup[3];} OfferCapRecruit;
static struct {
 int valid,status,scope;unsigned epoch,gen,applicant;U eval,ftable,teams_table;uint32_t frow,teams_row;U nt;
 int applicant_seen,applicant_user;U app_table;uint32_t app_row;
 OfferCapRecruit *slots;
} offer_cap;
static OfferCapRecruit *offer_cap_slot(U table,uint32_t rid,int create) {
 unsigned k=(unsigned)((table>>4)^((U)rid*2654435761u))&(OFFER_CAP_SLOTS-1);
 for(unsigned i=0;i<OFFER_CAP_SLOTS;i++,k=(k+1)&(OFFER_CAP_SLOTS-1)) {
  OfferCapRecruit *e=&offer_cap.slots[k];
  if(e->gen==offer_cap.gen && e->table==table && e->row==rid)return e;
  if(e->gen!=offer_cap.gen) {
   if(!create)return 0;
   memset(e,0,sizeof(*e));e->gen=offer_cap.gen;e->table=table;e->row=rid;return e;
  }
 }
 return 0;
}
static int offer_cap_bit(const uint64_t a[3],unsigned t){return t<192 && ((a[t>>6]>>(t&63))&1u);}
static void offer_cap_set(uint64_t a[3],unsigned t){if(t<192)a[t>>6]|=1ull<<(t&63);}
static int offer_cap_popcount(const uint64_t a[3]){int n=0;for(unsigned t=0;t<138;t++)if(offer_cap_bit(a,t))n++;return n;}
/* A team-scoped hook stays inside the run only if DRI is active, a snapshot exists and the hook's Team record is
   the cached applicant's. slot = the pointer to the Team reference that the hook body reads (guarded read: an
   unreadable slot ends the run, logs nothing, and the body's own reads and fault handling follow unchanged). */
static void offer_cap_team(const Rec *slot) {
 if(!cache.team_pending)return;
 cache.team_pending=0;
 int same=0;
 if(mode==1 && !faulted && offer_cap.valid && slot) {
  __try{Rec t=*slot;same=t && q((U)t+0x10)==offer_cap.app_table && row(t)==offer_cap.app_row;}
  __except(EXCEPTION_EXECUTE_HANDLER){same=0;}
 }
 if(!same)cache.offer_epoch++;
}
/* The same, for a hook whose body reads the slot pointer from the native frame (where = address of that pointer). */
static void offer_cap_team_at(U where) {
 if(!cache.team_pending)return;
 const Rec *slot=0;
 __try{slot=where?(const Rec *)q(where):0;}__except(EXCEPTION_EXECUTE_HANDLER){slot=0;}
 offer_cap_team(slot);
}
static void offer_cap_build(Rec teams,U nt,unsigned applicant_id,Rec applicant) {
 if(!++offer_cap.gen)offer_cap.gen=1;
 offer_cap.status=1;offer_cap.applicant_seen=0;offer_cap.applicant_user=0;
 uint64_t seen[3]={0};
 for(U i=0;i<nt;i++) {
  Rec team=array_at(teams,i);unsigned tid=team?scalar(team,383,0x100):255;
  if(!team || tid>=138 || offer_cap_bit(seen,tid)){release(team);offer_cap.status=-1;return;}
  offer_cap_set(seen,tid);
  if(tid==applicant_id) {
   if(!portal_same_record(team,applicant)){release(team);offer_cap.status=-1;return;}
   offer_cap.applicant_seen=1;
  }
  Rec user=reference(team,407);
  if(user){release(user);release(team);if(tid==applicant_id)offer_cap.applicant_user=1;continue;}
  if(tid==applicant_id){release(team);continue;} /* the applicant's board is read live */
  Rec board=board_array(team);U n=board?array_count(board):MAX_BOARD+1;
  if(!board || n>MAX_BOARD){release(board);release(team);offer_cap.status=-1;return;}
  int valid=1;
  for(U j=0;j<n && valid>0;j++) {
   Rec target=array_at(board,j),linked=target?reference(target,10):0;
   unsigned status=target?scalar(target,12,0x108):255;
   if(!target || !linked || status>4)valid=0;
   else {
    OfferCapRecruit *e=offer_cap_slot(q((U)linked+0x10),row(linked),1);
    if(!e)valid=-1;
    else {if(offer_cap_bit(e->found,tid))offer_cap_set(e->dup,tid);offer_cap_set(e->found,tid);if(status>=2 && status<=4)offer_cap_set(e->offered,tid);}
   }
   release(linked);release(target);
  }
  release(board);release(team);
  if(!valid){offer_cap.status=-1;return;}
  if(valid<0){offer_cap.status=-2;return;} /* recruit table full: never decide from it */
 }
}
/* -3: not applicable (the caller runs the scan); otherwise the scan's exact result. */
static int offer_cap_cached(Rec recruit,Rec applicant,U eval,int emit,int *count_out,int *own_out) {
 Rec f=context_ref(eval,6),teams=f?array_field(f,45):0;
 U nt=teams?array_count(teams):0;int result=-1,count=0,own=0;
 if(!recruit || !applicant || !f || !teams || !nt || nt>138)goto done;
 unsigned applicant_id=scalar(applicant,383,0x100);
 if(applicant_id>=138)goto done;
 if(!offer_cap.slots)offer_cap.slots=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,OFFER_CAP_SLOTS*sizeof(*offer_cap.slots));
 if(!offer_cap.slots){result=-3;goto done;}
 {
  U ftable=q((U)f+0x10),ttable=q((U)teams+0x10);uint32_t frow=row(f),trow=row(teams);
  U atable=q((U)applicant+0x10);uint32_t arow=row(applicant);
  if(cache.team_pending){cache.offer_epoch++;cache.team_pending=0;} /* a team-scoped hook that never proved its school */
  int same_app=offer_cap.valid && offer_cap.eval==eval && offer_cap.ftable==ftable && offer_cap.frow==frow &&
   offer_cap.teams_table==ttable && offer_cap.teams_row==trow && offer_cap.nt==nt &&
   offer_cap.applicant==applicant_id && offer_cap.app_table==atable && offer_cap.app_row==arow;
  if(!same_app || offer_cap.epoch!=cache.offer_epoch) {
   offer_cap_build(teams,nt,applicant_id,applicant);
   offer_cap.valid=1;offer_cap.epoch=cache.offer_epoch;offer_cap.eval=eval;offer_cap.ftable=ftable;offer_cap.frow=frow;
   offer_cap.teams_table=ttable;offer_cap.teams_row=trow;offer_cap.nt=nt;offer_cap.applicant=applicant_id;
   offer_cap.app_table=atable;offer_cap.app_row=arow;
  } else cache_counts[CACHE_OFFER_CAPACITY].hits++;
 }
 if(offer_cap.status==-2){result=-3;goto done;}
 if(offer_cap.status<0)goto done;
 {
  OfferCapRecruit *e=offer_cap_slot(q((U)recruit+0x10),row(recruit),0);
  if(e && offer_cap_popcount(e->dup))goto done;
  count=e?offer_cap_popcount(e->offered):0;
 }
 if(!offer_cap.applicant_user) {
  Rec board=board_array(applicant);U n=board?array_count(board):MAX_BOARD+1;
  if(!board || n>MAX_BOARD){release(board);goto done;}
  int found=0,offered=0,valid=1;
  for(U j=0;j<n;j++) {
   Rec target=array_at(board,j),linked=target?reference(target,10):0;
   unsigned status=target?scalar(target,12,0x108):255;
   if(!target || !linked || status>4)valid=0;
   if(linked && portal_same_record(linked,recruit)) {
    if(found++)valid=0;
    if(status>=2 && status<=4)offered=1;
   }
   release(linked);release(target);if(!valid)break;
  }
  release(board);if(!valid)goto done;
  count+=offered;own=offered;
 }
 if(!offer_cap.applicant_seen)goto done;
 {
  Rec player=reference(recruit,5);if(!player)goto done;
  int portal=transfer_recruit(recruit);
  int limit=portal?market30_limit(1):hs33_limit_stars((int)scalar(player,231,0x108));release(player);
  result=own || count<limit;
  if(!result && emit)log_line("TEST36_ACTUAL_CAP,%s,%u,%u,%d,%d,NEW_OFFER_HELD_BEFORE_PAYMENT",portal?"PORTAL":"HS",applicant_id,row(recruit),count,limit);
 }
done:
 release(teams);release(f);*count_out=count;*own_out=own;return result;
}
/* The offer-action capacity check (offer_cap.scope, a one-shot flag set at its one call site). */
static int offer_capacity_answer(Rec recruit,Rec applicant,U eval) {
 int scoped=offer_cap.scope;offer_cap.scope=0;
 if(scoped && cache_on()) {
  CacheCount *k=&cache_counts[CACHE_OFFER_CAPACITY];k->queries++;
  int c=0,o=0;
  if(cache_fast()){int r=offer_cap_cached(recruit,applicant,eval,1,&c,&o);if(r!=-3)return r;}
  else {
   offer_cap_live_count=offer_cap_live_own=-1;
   int live=test32_hs_offer_capacity_scan(recruit,applicant,eval),lc=offer_cap_live_count,lo=offer_cap_live_own;
   int r=offer_cap_cached(recruit,applicant,eval,0,&c,&o);
   if(r!=-3) {
    k->compares++;
    if((r!=live || (live>=0 && (c!=lc || o!=lo))) && cache_mismatch(CACHE_OFFER_CAPACITY))
     log_line("CACHE_STACK_MISMATCH,OFFER_CAPACITY,-,team=%u,recruit=%u,live=%d,cached=%d,live_count=%d,cached_count=%d,live_own=%d,cached_own=%d",
      applicant?scalar(applicant,383,0x100):999u,recruit?row(recruit):0u,live,r,lc,c,lo,o);
   }
   return live;
  }
 }
 return test32_hs_offer_capacity_scan(recruit,applicant,eval);
}
