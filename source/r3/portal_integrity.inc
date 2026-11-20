/* Test11 portal acceptance and ownership. Include after the ownership ledger.
   A native stage+TopSchoolsList leader is not proof of an accepted transfer.
   All returned Rec handles are retained; callers release them normally. */
static int portal_academy(unsigned tid) { return tid==0 || tid==7 || tid==55; }
/* Native Player PrevTeamIndex is ordinal230 (captured getter08590be0).
   The save keeps it when TeamIndex becomes255. Class4..7 proves transfer
   status, so default/HS previous-team values cannot authorize a return. */
static int portal_academy_allowed(Rec recruit,unsigned tid) {
 if(!portal_academy(tid))return 1;
 if(!recruit || !transfer_recruit(recruit))return 0;
 Rec p=reference(recruit,5);if(!p)return 0;
 unsigned origin=scalar(p,230,0x100);release(p);
 return origin==tid;
}
static int portal_same_record(Rec a,Rec b) {
 return a && b && q((U)a+0x10)==q((U)b+0x10) && row(a)==row(b);
}
static Rec portal_team_by_id(U eval,unsigned tid) {
 if(tid>=138)return 0;
 Rec franchise=context_ref(eval,6);if(!franchise)return 0;
 Rec teams=array_field(franchise,45);release(franchise);if(!teams)return 0;
 U n=array_count(teams);if(n>138){release(teams);return 0;}
 /* Real franchise arrays are team-index ordered. The bounded fallback also
    permits compact test fixtures and refuses a mismatched index identity. */
 if(tid<n) {
  Rec t=array_at(teams,tid);
  if(t && scalar(t,383,0x100)==tid){release(teams);return t;}
  release(t);
 }
 for(U i=0;i<n;i++) {
  if(i==tid)continue;
  Rec t=array_at(teams,i);
  if(t && scalar(t,383,0x100)==tid){release(teams);return t;}
  release(t);
 }
 release(teams);return 0;
}
/* 1: this exact recruit has a live offer; 0: no offer; -1: malformed board.
   TotalScholarshipOffers alone cannot identify the offering destination. */
static int portal_active_offer(Rec recruit,Rec team) {
 if(!recruit || !team)return -1;
 Rec board=reference(team,224);if(!board)return 0;
 Rec a=array_field(board,0);release(board);if(!a)return 0;
 U n=array_count(a);if(n>MAX_BOARD){release(a);return -1;}
 int found=0,matched=0;
 for(U i=0;i<n;i++) {
  Rec target=array_at(a,i);if(!target){found=-1;break;}
  Rec r=reference(target,10);
  if(!r){release(target);found=-1;break;}
  if(portal_same_record(r,recruit)) {
   if(matched++){release(r);release(target);found=-1;break;}
   unsigned status=scalar(target,12,0x108);
   if(status>4){release(r);release(target);found=-1;break;}
   if(status>=2)found=1;
  }
  release(r);release(target);
 }
 release(a);return found;
}
/* Published ownership is authoritative even if the saved interest list is
   stale. Return -1 on duplicate ownership or unreadable records. */
static int portal_published_owner(Rec recruit,U eval,int *tid) {
 if(tid)*tid=-1;
 Rec player=reference(recruit,5),franchise=context_ref(eval,6);
 if(!player || !franchise){release(player);release(franchise);return -1;}
 Rec teams=array_field(franchise,45);release(franchise);
 if(!teams){release(player);return -1;}
 U n=array_count(teams);if(n>138){release(teams);release(player);return -1;}
 int found=-1,ok=1;
 for(U i=0;i<n && ok;i++) {
  Rec team=array_at(teams,i);if(!team){ok=0;break;}
  unsigned id=scalar(team,383,0x100);
  Rec a=array_field(team,33);
  if(id>=138 || !a){release(a);release(team);ok=0;break;}
  U count=array_count(a);
  if(count>192){release(a);release(team);ok=0;break;}
  for(U j=0;j<count;j++) {
   Rec p=array_at(a,j);if(!p){ok=0;break;}
   int same=portal_same_record(p,player);release(p);
   if(same) {
    if(found>=0 && found!=(int)id){ok=0;break;}
    found=(int)id;
   }
  }
  release(a);release(team);
 }
 release(teams);release(player);
 if(!ok)return -1;
 if(found<0)return 0;
 if(tid)*tid=found;return 1;
}
/* Keep the recruit key occupied so collided signings remain reachable. */
static void portal_forget_owner(Rec recruit) {
 if(!recruit)return;
 U table=q((U)recruit+0x10);uint32_t rid=row(recruit);
 AcquireSRWLockExclusive(&ownership_lock);
 PortalSign *s=portal_sign_slot(table,rid,0);
 int changed=(!portal_callback_generation||portal_callback_generation==portal_scope_generation)&&s&&s->player_table;
 if(changed){s->player_table=0;s->pid=0;s->tid=255;pt28_invalidate();}
 ReleaseSRWLockExclusive(&ownership_lock);
}
/* A fresh process has no callback ledger. Resolve the target's native
   commitment week across ALL boards before interpreting any preference rank.
   A plain displayed offer with no commitment marker cannot defeat a saved
   acceptance. Multiple markers are inconsistent and must never cause cleanup. */
static int r2_saved_owner(Rec recruit,U eval,int *owner) {
 Rec f=context_ref(eval,6),teams=f?array_field(f,45):0,season=f?reference(f,44):0;
 unsigned week=season?scalar(season,4,0x100):0,type=season?scalar(season,5,0x108):99;
 release(season);release(f);
 U n=teams?array_count(teams):0;int result=0,marked=-1,offers=0;
 unsigned chosen_week=0;int price=0;
 if(!teams || !n || n>138){release(teams);return -1;}
 for(U i=0;i<n && result>=0;i++) {
  Rec team=array_at(teams,i);unsigned tid=team?scalar(team,383,0x100):138;
  Rec b=team?reference(team,224):0,a=b?array_field(b,0):0;release(b);
  U m=a?array_count(a):0;
  if(tid>=138 || m>MAX_BOARD)result=-1;
  int matched=0;
  for(U j=0;j<m && result>=0;j++) {
   Rec t=array_at(a,j),r=t?reference(t,10):0;
   if(!r){result=-1;release(t);break;}
   if(portal_same_record(r,recruit)){
    unsigned status=scalar(t,12,0x108),w=scalar(t,1,0x100);
    if(matched++ || status>4)result=-1;
    else if(status>=2 && portal_academy_allowed(recruit,tid)) {
     offers++;
     if(w>=1 && w<=6 && type==6 && w<=week){
      if(marked>=0){
       log_line("R2_OWNER_CONFLICT,%u,%d,%u,%u",row(recruit),marked,tid,w);result=-1;
      }else {marked=(int)tid;chosen_week=w;price=(int)scalar(t,2,0x100);}
     }
    }
   }
   release(r);release(t);
  }
  release(a);release(team);
 }
 release(teams);
 if(result<0)return -1;
 if(marked>=0){*owner=marked;log_line("R2_SAVED_OWNER,%u,%d,%u,%d",row(recruit),marked,chosen_week,price);return 1;}
 if(offers>1){log_line("R2_OWNER_CONFLICT,%u,%d,NO_DATED_OWNER",row(recruit),offers);return -1;}
 return 0;
}
/* 1: valid accepted hard commitment, 0: unowned/orphan, -1: unreadable state.
   The fast ledger path is safe only because Test11 stores portal owners from
   accepted native callbacks or this resolver, never from own_commit alone.
   Pending permission contributes no player until the native stage is 5. */
static int portal_hard_owner(Rec recruit,U eval,int *tid) {
 if(tid)*tid=-1;
 if(!recruit || !transfer_recruit(recruit) || scalar(recruit,9,0x108)!=5)return 0;
 Rec franchise=context_ref(eval,6);
 if(!franchise)return -1;
 int scoped=portal_ownership_scope(franchise);release(franchise);if(!scoped)return -1;
 Rec player=reference(recruit,5);if(!player)return -1;
 int owner=-1;
 if(portal_signed_destination(recruit,player,&owner) && owner>=0 && owner<138) {
  release(player);
  if(!portal_academy_allowed(recruit,(unsigned)owner))return 0;
  if(tid)*tid=owner;return 1;
 }
 int published=portal_published_owner(recruit,eval,&owner);
 if(published<0){release(player);return -1;}
 if(published) {
  if(!portal_academy_allowed(recruit,(unsigned)owner)){release(player);return 0;}
  int stored=mode!=1 || portal_store_owner(recruit,player,(unsigned)owner);
  release(player);if(!stored)return -1;
  if(tid)*tid=owner;return 1;
 }
 int saved=r2_saved_owner(recruit,eval,&owner);
 if(saved<0){release(player);return -1;}
 if(saved) {
  int stored=mode!=1 || portal_store_owner(recruit,player,(unsigned)owner);
  release(player);if(!stored)return -1;if(tid)*tid=owner;return 1;
 }
 Rec top=array_field(recruit,13);
 if(!top){release(player);return -1;}
 U count=array_count(top);
 if(!count){release(top);release(player);return 0;}
 if(count>64){release(top);release(player);return -1;}
 Rec leading=array_at(top,0);release(top);
 if(!leading){release(player);return -1;}
 unsigned leader=scalar(leading,0,0x100);release(leading);
 if(leader>=138){release(player);return -1;}
 if(!portal_academy_allowed(recruit,leader)){release(player);return 0;}
 Rec team=portal_team_by_id(eval,leader);
 if(!team){release(player);return -1;}
 int offered=portal_active_offer(recruit,team);release(team);
 if(offered!=1){release(player);return offered;}
 int stored=mode!=1 || portal_store_owner(recruit,player,leader);
 release(player);if(!stored)return -1;
 if(tid)*tid=(int)leader;return 1;
}
