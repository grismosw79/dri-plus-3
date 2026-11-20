/* Test27R2: complete an already published HS/JUCO acquisition exactly once.
   Run at NSD before native stage collectors, then idempotently before publication. Published Player identity is
   authoritative; interest rank, an offer, or a pending permission is not.
   No destination, array membership, influence, offer or player data is changed.
   Native collectors run before admission guards; early finalization excludes
   already-published HS identities from those stage-4/5 queues.
   Existing same-school duplicates are refused, not silently repaired here. */
typedef struct {Rec recruit;unsigned stage,owner;} HsFinalizeItem;
static int hs_finalize_published(U eval) {
 Rec franchise=0,teams=0,pool=0;
 PortalCleanupOwner *owners=0,*seen=0;
 HsFinalizeItem *pending=0;
 unsigned used=0,changed=0;
 const char *failure=0;
 if(mode!=1 || faulted)return 0;
 franchise=context_ref(eval,6);
 teams=franchise?array_field(franchise,45):0;
 pool=franchise?array_field(franchise,39):0;
 if(!teams || !pool){failure="arrays";goto done;}
 U nt=array_count(teams),nr=array_count(pool);
 if(!nt || nt>138 || nr>12000){failure="bounds";goto done;}
 owners=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,PORTAL_CLEANUP_SLOTS*sizeof(*owners));
 seen=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,PORTAL_CLEANUP_SLOTS*sizeof(*seen));
 pending=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,12000*sizeof(*pending));
 if(!owners || !seen || !pending){failure="memory";goto done;}
 /* Validate the complete owner map before the first stage write. */
 for(U i=0;i<nt;i++) {
  Rec team=array_at(teams,i),a=team?array_field(team,33):0;
  if(!team || scalar(team,383,0x100)!=i || !a){release(a);release(team);failure="team";goto done;}
  U n=array_count(a);
  if(n>192){release(a);release(team);failure="committed_bounds";goto done;}
  for(U j=0;j<n;j++) {
   Rec player=array_at(a,j);
   if(!player){release(a);release(team);failure="player";goto done;}
   PortalCleanupOwner *slot=portal_cleanup_slot(owners,player,1);release(player);
   if(!slot || slot->owner>=0){release(a);release(team);failure="duplicate_acquisition";goto done;}
   slot->owner=(int)i;
  }
  release(a);release(team);
 }
 for(U i=0;i<nr;i++) {
  Rec recruit=array_at(pool,i),player=recruit?reference(recruit,5):0;
  if(!recruit || !player || !hs15_is_recruit(recruit)) {
   release(player);release(recruit);failure="HS_record";goto done;
  }
  PortalCleanupOwner *owner=portal_cleanup_slot(owners,player,0);
  PortalCleanupOwner *unique=portal_cleanup_slot(seen,player,1);release(player);
  if(!unique || unique->stage){release(recruit);failure="duplicate_HS_identity";goto done;}
  unique->stage=1;
  unsigned stage=scalar(recruit,9,0x108);
  if(stage>6){release(recruit);failure="stage";goto done;}
  if(owner && stage<6)pending[used++]=(HsFinalizeItem){recruit,stage,(unsigned)owner->owner};
  else release(recruit);
 }
 for(unsigned i=0;i<used;i++) {
  HsFinalizeItem *item=&pending[i];
  if(scalar(item->recruit,9,0x108)!=item->stage){failure="stage_changed";goto done;}
 }
 for(unsigned i=0;i<used;i++) {
  HsFinalizeItem *item=&pending[i];
  /* Same SDF enum setter + table notification as captured native Signed write. */
  portal_recovery_scalar(item->recruit,9,6);
  if(scalar(item->recruit,9,0x108)!=6){failure="write_check";goto done;}
  changed++;
  log_line("HS_FINALIZED_EXISTING,%u,%u,%u,6",row(item->recruit),item->owner,item->stage);
 }
 log_line("HS_FINALIZATION,%u,EXISTING_ACQUISITIONS_ONLY",changed);
done:
 for(unsigned i=0;i<used;i++)release(pending[i].recruit);
 if(pending)HeapFree(GetProcessHeap(),0,pending);
 if(seen)HeapFree(GetProcessHeap(),0,seen);
 if(owners)HeapFree(GetProcessHeap(),0,owners);
 release(pool);release(teams);release(franchise);
 if(failure){log_line("HS_FINALIZATION_REFUSED,%s",failure);portal_fault("HS_finalization");return -1;}
 return (int)changed;
}

/* Test28R4: native NSD builds stage-4/5 target vectors before bulk admission.
   Finalizing after collection cannot remove retained target references. Run
   this validated identity-only batch before the first native school collector.
   The later callback remains an idempotent final consistency check. */
void DriBeforeNsdCollect(Registers *r) {
 if(mode!=1 || faulted)return;
 __try {
  U eval=r->r13;
  if(!eval || !*(unsigned char*)(eval+0x1a0))return;
  int finalized=hs_finalize_published(eval);
  if(finalized>=0 && !faulted)
   log_line("NSD_PRECOLLECT,%d,EXISTING_HS_FINALIZED",finalized);
 } __except(fail_exception("NSD_HS_precollect",GetExceptionCode())){}
}
