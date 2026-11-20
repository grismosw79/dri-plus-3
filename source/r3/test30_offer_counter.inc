/* Test30: TotalScholarshipOffers is a derived count of native board statuses
   2..4 (the same states increment/decremented by native offer/removal code).
   Reconcile only at the safe final NSD boundary, after native publication and
   rival cleanup. This repairs the derived counter, not the historical writer
   responsible for the Robinson discrepancy. No target, offer, hours, points,
   commitment stage or ownership is created or removed here. */
typedef struct {Rec recruit;unsigned before,count;} Test30OfferCounter;
static int test30_reconcile_offer_counters(U eval){
 if(mode!=1||faulted)return 1;
 Rec f=context_ref(eval,6),teams=f?array_field(f,45):0;
 PortalCleanupOwner *map=0,*owners=0;
 Test30OfferCounter *items=0;
 unsigned used=0,changed=0;const char *error=0;
 if(!f||!teams||!portal_ownership_scope(f)){error="scope";goto done;}
 U nt=array_count(teams);if(!nt||nt>138){error="team_bounds";goto done;}
 map=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,PORTAL_CLEANUP_SLOTS*sizeof(*map));
 owners=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,PORTAL_CLEANUP_SLOTS*sizeof(*owners));
 items=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,PORTAL_CLEANUP_SLOTS*sizeof(*items));
 if(!map||!owners||!items){error="memory";goto done;}
 /* Complete both pools and every board before the first counter write. */
 for(int kind=0;kind<2;kind++){
  Rec pool=array_field(f,kind?46:39);
  if(!pool){error="pool";goto done;}
  U n=array_count(pool);if(n>12000){release(pool);error="pool_bounds";goto done;}
  for(U j=0;j<n;j++){
   Rec recruit=array_at(pool,j);
   if(!recruit||used>=PORTAL_CLEANUP_SLOTS){release(recruit);release(pool);error="record";goto done;}
   PortalCleanupOwner *slot=portal_cleanup_slot(map,recruit,1);
   if(!slot||slot->owner>=0||scalar(recruit,9,0x108)>6||
      (kind?!transfer_recruit(recruit):!hs15_is_recruit(recruit))){release(recruit);release(pool);error="pool_identity";goto done;}
   slot->owner=(int)used;
   items[used++]=(Test30OfferCounter){recruit,scalar(recruit,14,0x100),0};
  }
  release(pool);
 }
 for(U tid=0;tid<nt;tid++){
  Rec team=array_at(teams,tid),board=team?board_array(team):0,committed=team?array_field(team,33):0;
  if(!team||scalar(team,383,0x100)!=tid||!board||!committed){release(committed);release(board);release(team);error="team";goto done;}
  U n=array_count(board),nc=array_count(committed);
  if(n>MAX_BOARD||nc>192){release(committed);release(board);release(team);error="board_bounds";goto done;}
  for(U j=0;j<nc;j++){
   Rec player=array_at(committed,j);PortalCleanupOwner *slot=player?portal_cleanup_slot(owners,player,1):0;
   release(player);
   if(!slot||slot->owner>=0){release(committed);release(board);release(team);error="duplicate_owner";goto done;}
   slot->owner=(int)tid;
  }
  release(committed);
  unsigned seen[MAX_BOARD],seen_count=0;
  for(U j=0;j<n;j++){
   Rec target=array_at(board,j),recruit=target?reference(target,10):0;
   PortalCleanupOwner *slot=recruit?portal_cleanup_slot(map,recruit,0):0;
   unsigned status=target?scalar(target,12,0x108):99;
   /* User boards can retain a valid recruit outside the current pool. Count
      its actual targets too; pool membership is not ownership or offer state. */
   if(recruit&&!slot&&used<PORTAL_CLEANUP_SLOTS&&scalar(recruit,9,0x108)<=6&&
      (hs15_is_recruit(recruit)||transfer_recruit(recruit))){
    slot=portal_cleanup_slot(map,recruit,1);
    if(slot){slot->owner=(int)used;items[used++]=(Test30OfferCounter){recruit,scalar(recruit,14,0x100),0};
     ((void(*)(Rec))vfunc(recruit,0))(recruit);}
   }
   release(recruit);release(target);
   if(!slot||slot->owner<0||status>4){release(board);release(team);error="target";goto done;}
   unsigned at=(unsigned)slot->owner;
   for(unsigned k=0;k<seen_count;k++)if(seen[k]==at){release(board);release(team);error="duplicate_target";goto done;}
   seen[seen_count++]=at;
   if(status>=2)items[at].count++;
  }
  release(board);release(team);
 }
 for(unsigned i=0;i<used;i++)if(scalar(items[i].recruit,14,0x100)!=items[i].before){error="snapshot_changed";goto done;}
 for(unsigned i=0;i<used;i++){
  Test30OfferCounter *x=&items[i];if(x->before==x->count)continue;
  portal_recovery_scalar(x->recruit,14,x->count);
  if(scalar(x->recruit,14,0x100)!=x->count){error="write_check";goto done;}
  log_line("TEST30_NSD_OFFER_COUNTER,%u,%u,%u,DERIVED_FROM_BOARDS",row(x->recruit),x->before,x->count);changed++;
 }
 log_line("TEST30_NSD_OFFER_COUNTER_SUMMARY,%u,%u",used,changed);
done:
 for(unsigned i=0;i<used;i++)release(items[i].recruit);
 if(items)HeapFree(GetProcessHeap(),0,items);
 if(owners)HeapFree(GetProcessHeap(),0,owners);
 if(map)HeapFree(GetProcessHeap(),0,map);
 release(teams);release(f);
 if(error){log_line("TEST30_NSD_OFFER_COUNTER_REFUSED,%s",error);portal_fault("NSD_offer_counter");return 0;}
 return 1;
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTest30OfferCounters(U eval){return test30_reconcile_offer_counters(eval);}
#endif
