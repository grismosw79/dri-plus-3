/* Test31: collection is not acceptance. Native 08187730 retains Target refs
   before bulk admission; 08164d40 later publishes every retained target without
   rechecking that admission. Keep only the destination whose native signing
   actually completed. No owner is chosen or repaired after publication.

   Captured 081689e0 map ABI: buckets+8, bucket count+0x10; node team+0,
   retained Target vector+8, next+0x28. The existing 081a2f90 hook executes after
   all normal/fallback signing and before the publication loop. */
typedef struct {U vector,begin,end;unsigned tid,count;unsigned char keep[192];} Test31NsdQueue;
static int test31_nsd_queue_validate(U map,U eval) {
 if(mode!=1 || faulted)return 0;
 Test31NsdQueue *plan=0;PortalCleanupOwner *destinations=0,*published_owners=0;
 unsigned used=0,removed=0,kept=0,teams[138]={0};const char *failure=0;
 U buckets=q(map+8);unsigned count=*(unsigned *)(map+0x10);
 if(!count && !buckets)return 1;
 if(!buckets || !count || count>4096){failure="map_bounds";goto done;}
 plan=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,138*sizeof(*plan));
 destinations=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,PORTAL_CLEANUP_SLOTS*sizeof(*destinations));
 published_owners=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,PORTAL_CLEANUP_SLOTS*sizeof(*published_owners));
 if(!plan || !destinations || !published_owners){failure="memory";goto done;}
 for(unsigned tid=0;tid<138;tid++) {
  Rec team=portal_team_by_id(eval,tid),incoming=team?array_field(team,33):0;
  if(!team || !incoming || array_count(incoming)>192){release(incoming);release(team);failure="published_array";goto done;}
  U n=array_count(incoming);
  for(U j=0;j<n;j++) {
   Rec player=array_at(incoming,j);PortalCleanupOwner *owner=player?portal_cleanup_slot(published_owners,player,1):0;release(player);
   if(!owner || owner->owner>=0){release(incoming);release(team);failure="published_owner_conflict";goto done;}
   owner->owner=(int)tid;
  }
  release(incoming);release(team);
 }
 /* Validate all queues and player identities before releasing a single ref. */
 for(unsigned b=0;b<count;b++)for(U node=q(buckets+b*8);node;node=q(node+0x28)) {
  unsigned tid=*(unsigned *)node;
  if(used>=138 || tid>=138 || teams[tid]++){failure="team_identity";goto done;}
  Test31NsdQueue *p=&plan[used++];p->vector=node+8;p->tid=tid;
  p->begin=q(p->vector);p->end=q(p->vector+8);U cap=q(p->vector+16);
  if((p->begin|p->end|cap)&7 || p->end<p->begin || cap<p->end ||
     p->end-p->begin>192*8 || ((!p->begin) && (p->end || cap))){failure="vector_bounds";goto done;}
  p->count=(unsigned)((p->end-p->begin)/8);
  for(unsigned i=0;i<p->count;i++) {
   Rec target=(Rec)q(p->begin+i*8),recruit=target?reference(target,10):0,player=recruit?reference(recruit,5):0;
   if(!target || !recruit || !player || (!hs15_is_recruit(recruit) && !transfer_recruit(recruit))) {
    release(player);release(recruit);failure="target_identity";goto done;
   }
   unsigned stage=scalar(recruit,9,0x108);int accepted=-1,published=-1;
   int has=portal_signed_destination(recruit,player,&accepted);
   PortalCleanupOwner *saved=portal_cleanup_slot(published_owners,player,0);
   int acquired=saved!=0;if(saved)published=saved->owner;
   if(stage>6 || (acquired && has && accepted!=published)) {
    release(player);release(recruit);failure="owner_conflict";goto done;
   }
   if(stage==6 && !acquired) {
    if(!has || accepted<0 || accepted>=138){release(player);release(recruit);failure="signed_without_destination";goto done;}
    PortalCleanupOwner *d=portal_cleanup_slot(destinations,player,1);
    if(!d || (d->owner>=0 && d->owner!=accepted)){release(player);release(recruit);failure="accepted_identity_conflict";goto done;}
    d->owner=accepted;
    /* Native consumer de-duplicates Target identity, not Player identity.
       Two different Target wrappers must still publish this player once. */
    if(accepted==(int)tid && !d->stage){p->keep[i]=1;d->stage=1;kept++;}
   }
   if(!p->keep[i])removed++;
   release(player);release(recruit);
  }
 }
 for(unsigned i=0;i<PORTAL_CLEANUP_SLOTS;i++)if(destinations[i].used && !destinations[i].stage){failure="accepted_destination_not_queued";goto done;}
 for(unsigned i=0;i<used;i++)if(q(plan[i].vector)!=plan[i].begin || q(plan[i].vector+8)!=plan[i].end){failure="vector_changed";goto done;}
 for(unsigned i=0;i<used;i++) {
  Test31NsdQueue *p=&plan[i];U out=p->begin;
  for(unsigned j=0;j<p->count;j++) {
   Rec target=(Rec)q(p->begin+j*8);
   if(p->keep[j]){*(Rec *)out=target;out+=8;}
   else release(target);
  }
  /* Retained refs are moved, not duplicated. Native vector owns survivors. */
  for(U tail=out;tail<p->end;tail+=8)*(U *)tail=0;
  *(U *)(p->vector+8)=out;
 }
 log_line("TEST31_NSD_QUEUE,%u,%u,%u,VALIDATED_NATIVE_SIGNINGS",used,kept,removed);
done:
 if(published_owners)HeapFree(GetProcessHeap(),0,published_owners);
 if(destinations)HeapFree(GetProcessHeap(),0,destinations);
 if(plan)HeapFree(GetProcessHeap(),0,plan);
 if(failure){log_line("TEST31_NSD_QUEUE_REFUSED,%s",failure);portal_fault("NSD_queue_validation");return 0;}
 return 1+(int)removed; /* Positive success; >1 means queued publication changed. */
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTest31NsdQueue(U map,U eval){return test31_nsd_queue_validate(map,eval)>0;}
__declspec(dllexport) int DriTest31Finalize(U eval){return hs_finalize_published(eval);}
#endif
