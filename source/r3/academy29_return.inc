/* R4: restoration after ordinary native portal finalization is exhausted.
   No offers, influence, hours or ratings are manufactured. The existing native
   finalizer owns its destination maps, Signed transition and publication.
   This exception is return-only: its lifetime is one synchronous final pass.
   Queue numeric identities only until the caller completes publication. No
   SDF handles survive the finalizer, and Signed6 alone is not a return. */
typedef struct {unsigned recruit,player,origin;} Academy29Pending;
static Academy29Pending academy29_pending[12000];
static unsigned academy29_pending_count;
static U academy29_pending_eval;
static void academy29_pending_clear(void) {
 academy29_pending_count=0;academy29_pending_eval=0;
}
/* The fallback native RecruitTarget constructor leaves its Recruit reference
   empty. Native roster publication follows that reference, not the Signed
   flag. Repair only this exact own-academy return before native vector append. */
int DriOnAcademy29ReturnTarget(Registers *r) {
 cache_hook_entry(CACHE_HOOK_OTHER);
 if(!academy29_return_eval)return 0;
 if(mode!=1 || faulted)return 1;
 __try {
  U eval=q(r->rbp+0x40),sp=(U)(r+1);
  Rec target=(Rec)q(sp+0x40),recruit=(Rec)q(r->rbp-0x78),team=(Rec)r->r15;
  if(eval!=academy29_return_eval || !target || !recruit || !team) {
   portal_fault("academy_return_target_scope");return 1;
  }
  if(!transfer_recruit(recruit))return 0;
  Rec player=reference(recruit,5);
  unsigned origin=player?scalar(player,230,0x100):255;release(player);
  if(!portal_academy(origin) || scalar(team,383,0x100)!=origin)return 0;
  if(scalar(recruit,9,0x108)>=6) {
   portal_fault("academy_return_target_identity");return 1;
  }
  Rec linked=reference(target,10);int same=linked && portal_same_record(linked,recruit);
  if(linked && !same){release(linked);portal_fault("academy_return_target_conflict");return 1;}
  release(linked);
  if(!same) {
   ((void(*)(Rec,U,Rec*))(game_base+0x0805e530))(target,field(schema(target),10),&recruit);cache_note_write();
   linked=reference(target,10);same=linked && portal_same_record(linked,recruit);release(linked);
   if(!same){portal_fault("academy_return_target_write");return 1;}
  }
  return 0;
 } __except(fail_exception("academy_return_target",GetExceptionCode())){}
 return 1;
}
static int academy29_verify_publication(U eval) {
 if(!academy29_pending_count)return 0;
 unsigned expected=academy29_pending_count;int verified=0;
 Rec f=0,pool=0;
 __try {
  if(mode!=1 || faulted)return -1;
  if(eval!=academy29_pending_eval){portal_fault("academy_return_publish_scope");return -1;}
  f=context_ref(eval,6);pool=f?array_field(f,46):0;
  U n=pool?array_count(pool):12001;
  if(n>12000){portal_fault("academy_return_publish_pool");return -1;}
  for(U i=0;i<n && !faulted;i++) {
   Rec recruit=array_at(pool,i);
   if(!recruit){portal_fault("academy_return_publish_record");break;}
   for(unsigned j=0;j<expected && !faulted;j++) {
    Academy29Pending *p=&academy29_pending[j];
    if(p->origin==255 || p->recruit!=row(recruit))continue;
    Rec player=reference(recruit,5);int identity=player && row(player)==p->player;release(player);
    int owner=-1,owns=identity?portal_published_owner(recruit,eval,&owner):-1;
    if(!identity || scalar(recruit,9,0x108)!=6 || owns!=1 || owner!=(int)p->origin) {
     portal_fault("academy_return_publication_incomplete");break;
    }
    /* Confirm exactly one incoming entry at the native destination. */
    Rec team=portal_team_by_id(eval,p->origin),incoming=team?array_field(team,33):0;
    unsigned occurrences=0;U count=incoming?array_count(incoming):193;
    for(U k=0;k<count && count<=192;k++) {
     Rec member=array_at(incoming,k);if(member && row(member)==p->player)occurrences++;release(member);
    }
    release(incoming);release(team);
    if(count>192 || occurrences!=1){portal_fault("academy_return_publication_duplicate");break;}
    log_line("TEST29R4_ACADEMY_RETURNED,%u,%d,PUBLISHED_OWNER",p->recruit,owner);
    p->origin=255;verified++;
   }
   release(recruit);
  }
  if(!faulted && verified!=(int)expected)portal_fault("academy_return_publication_missing");
 } __finally {release(pool);release(f);academy29_pending_clear();}
 return faulted?-1:verified;
}
static Rec academy29_return_destination(Rec recruit,U eval) {
 if(mode!=1 || faulted || !eval || academy29_return_eval!=eval || !recruit ||
    !transfer_recruit(recruit) || scalar(recruit,9,0x108)>=5)return 0;
 Rec player=reference(recruit,5);if(!player){portal_fault("academy_return_player");return 0;}
 unsigned origin=scalar(player,230,0x100);release(player);
 if(!portal_academy(origin))return 0;
 int owner=-1,published=portal_published_owner(recruit,eval,&owner);
 if(published<0){portal_fault("academy_return_owner");return 0;}
 if(published)return 0;
 return portal_team_by_id(eval,origin);
}
static int academy29_restore(Registers *r) {
 if(mode!=1 || faulted || academy29_return_eval)return 0;
 U eval=r->r13;Rec f=context_ref(eval,6),season=f?reference(f,44):0,pool=f?array_field(f,46):0;
 int active=season && scalar(season,5,0x108)==6 && scalar(season,4,0x100)==6;
 release(season);release(f);
 if(!active){release(pool);return 0;}
 U n=pool?array_count(pool):12001;if(n>12000){release(pool);portal_fault("academy_return_pool");return -1;}
 Rec *pending=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(n+1)*sizeof(Rec));
 if(!pending){release(pool);portal_fault("academy_return_memory");return -1;}
 unsigned count=0;int restored=0;
 __try {
  academy29_return_eval=eval;
  /* Validate the complete batch before native changes; no player may occur
     twice under different recruit wrappers. Published destinations win. */
  for(U i=0;i<n && !faulted;i++) {
   Rec recruit=array_at(pool,i),origin=academy29_return_destination(recruit,eval);
   if(!recruit){portal_fault("academy_return_record");release(origin);break;}
   if(origin) {
    Rec player=reference(recruit,5);
    for(unsigned j=0;j<count && !faulted;j++) {
     Rec prior=reference(pending[j],5);
     if(portal_same_record(prior,player))portal_fault("academy_return_duplicate");
     release(prior);
    }
    release(player);
    if(!faulted){pending[count++]=recruit;recruit=0;}
   }
   release(origin);release(recruit);
  }
  if(count && !faulted) {
   if(academy29_pending_count && academy29_pending_eval!=eval) {
    portal_fault("academy_return_pending_scope");return -1;
   }
   if(count>12000-academy29_pending_count){portal_fault("academy_return_pending_limit");return -1;}
   log_line("TEST29R4_ACADEMY_RETURN_PASS,%u,AFTER_NORMAL_PORTAL_FINALIZATION",count);
   ((void(*)(U,U,U))(game_base+0x081b5670))(eval,r->rbp-0x78,r->rbp-0x18);cache_note_write();
   for(unsigned i=0;i<count && !faulted;i++) {
    int owner=-1;Rec player=reference(pending[i],5);
    unsigned origin=player?scalar(player,230,0x100):255;
    int owns=player?portal_signed_destination(pending[i],player,&owner):0;release(player);
    if(scalar(pending[i],9,0x108)!=6 || !owns || owner!=(int)origin) {
     portal_fault("academy_return_native_incomplete");break;
    }
    restored++;
    player=reference(pending[i],5);
    academy29_pending[academy29_pending_count++]=(Academy29Pending){row(pending[i]),row(player),origin};
    release(player);academy29_pending_eval=eval;
    log_line("TEST29R4_ACADEMY_RETURN_QUEUED,%u,%d,AWAITING_NATIVE_PUBLICATION",row(pending[i]),owner);
   }
  }
 } __finally {
  academy29_return_eval=0;
  for(unsigned i=0;i<count;i++)release(pending[i]);
  HeapFree(GetProcessHeap(),0,pending);release(pool);
 }
 return faulted?-1:restored;
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTest29R3Restore(Registers *r){return academy29_restore(r);}
__declspec(dllexport) void DriTest29R3ReturnScope(U eval){academy29_return_eval=eval;}
__declspec(dllexport) int DriTest29R3HsBlock(Rec r,Rec t){return academy29_hs_block(r,t);}
__declspec(dllexport) int DriTest29R4VerifyReturns(U eval){return academy29_verify_publication(eval);}
__declspec(dllexport) void DriTest29R4ClearReturns(void){academy29_pending_clear();}
#endif
