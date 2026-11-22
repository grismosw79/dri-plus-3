/* Test29R2: a conditional portal offer can become usable after a later entry
   in the native portal array signs elsewhere. The native finalizer makes one
   forward pass. Retry its verified complete pass only while it signs players,
   at the existing pre-publication boundary with the original live maps.
   No destination, offer, influence, stage or roster is synthesized here. */
static int test29r2_portal_pending(U eval,int *remaining,int *pool_count) {
 *remaining=0;*pool_count=0;
 Rec franchise=context_ref(eval,6),pool=franchise?array_field(franchise,46):0;
 release(franchise);
 if(!pool){portal_fault("R2_NSD_portal_pool");return -1;}
 U count=array_count(pool);int pending=0;
 if(count>12000){release(pool);portal_fault("R2_NSD_portal_bounds");return -1;}
 *pool_count=(int)count;
 for(U i=0;i<count && !faulted;i++) {
  Rec recruit=array_at(pool,i);
  if(!recruit){portal_fault("R2_NSD_portal_record");break;}
  unsigned stage=scalar(recruit,9,0x108);
  if(stage>6 || !transfer_recruit(recruit)) {
   release(recruit);portal_fault("R2_NSD_portal_invalid_entry");break;
  }
  if(stage<6) {
   (*remaining)++;
   /* A hard commitment still needs native finalization. Other candidates
      need a displayed offer before another native winner pass can help. */
   if(stage==5 || scalar(recruit,14,0x100)>0)pending++;
  }
  release(recruit);
 }
 release(pool);return faulted?-1:pending;
}
static int test29r2_portal_retry(Registers *r) {
 if(mode!=1 || faulted)return 0;
 r3_closed_recovery(r->r13,"NSD_BEFORE_NATIVE_RETRY");
 if(faulted)return -1;
 int remaining=0,total=0;
 int pending=test29r2_portal_pending(r->r13,&remaining,&total);
 if(pending<0)return -1;
 int signed_count=0,initial=remaining;
 for(int pass=0;pending>0 && pass<initial && !faulted && mode==1;pass++) {
  int before=remaining;
  log_line("TEST29R2_PORTAL_FINAL_PASS,%d,%d,%d,NATIVE_SIGNING_PENDING",pass+1,pending,remaining);
  /* Verified native coordinator call at 0819f268..273: RCX evaluator,
     RDX destination map, R8 newly-signed map. Signed6 entries are skipped
     before selectors or side effects. Both maps remain owned by the caller. */
  ((void(*)(U,U,U))(game_base+0x081b5670))(r->r13,r->rbp-0x78,r->rbp-0x18);cache_note_write();
  if(faulted || mode!=1)return -1;
  /* A later signing may close the sole conditional destination of another
     recruit. Recheck at this completed-pass boundary, with no board iterator
     alive. Native code still earns influence and chooses every destination. */
  r3_closed_recovery(r->r13,"NSD_AFTER_NATIVE_RETRY");
  if(faulted || mode!=1)return -1;
  int after_total=0;
  pending=test29r2_portal_pending(r->r13,&remaining,&after_total);
  if(pending<0)return -1;
  if(after_total!=total || remaining>before) {
   portal_fault("R2_NSD_portal_population_changed");return -1;
  }
  signed_count+=before-remaining;
  log_line("TEST29R2_PORTAL_FINAL_RESULT,%d,%d,%d,NATIVE_SIGNED_DELTA",pass+1,before-remaining,remaining);
  if(remaining==before) {
   log_line("TEST29R2_PORTAL_UNRESOLVED,%d,%d,NO_NATIVE_PROGRESS",remaining,pending);break;
  }
 }
 return signed_count;
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) int DriTest29R2PortalRetry(Registers *r){return test29r2_portal_retry(r);}
#endif
