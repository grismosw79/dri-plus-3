/* Test15 shared recruiting accounting for HS/JUCO policy. Included after
   portal_roster so ownership/departure helpers are the same as portal policy.
   This performs one bounded pass over each source rather than one per position.
   It never writes a player, offer, recruiting stage, influence, or destination. */
#include "class31_policy.h"
#include "class34_policy.h"
#include "class34r2_plan.h"
static int hs15_is_recruit(Rec r) {return r && scalar(r,2,0x108)<=3;}
/* Test30: academies may pursue four-star HS under ordinary gates; five-star
   HS remains prohibited. Existing secured commitments and JUCO classes are
   preserved. Stars are native zero-based (3=4-star, 4=5-star). */
static int academy29_hs_block(Rec recruit,Rec team) {
 if(!recruit || !team || !portal_academy(scalar(team,383,0x100)) ||
    scalar(recruit,2,0x108)!=0 || scalar(recruit,9,0x108)>=5)return 0;
 Rec player=reference(recruit,5);if(!player)return 0;
 unsigned stars=scalar(player,231,0x108);release(player);
 return stars==4;
}
static int hs15_window(U eval) {
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 if(!season){release(f);return -1;}
 unsigned week=scalar(season,4,0x100),type=scalar(season,5,0x108);
 release(season);release(f);
 return type<=1 || (type==6 && week>=2 && week<=6);
}
/* Shared deterministic class authorization for portal projections as well as
   HS snapshots. It depends on the current scheme and calendar, never mutable
   offers or already selected candidates; no native record is retained. */
static int class31_team_annual(Rec team,U eval,int annual[22]) {
 static const int fields[22]={5,2,1,9,8,4,3,0,6,7,3,6,1,4,5,7,0,2,8,1,6,4};
 int ideal[22],ok=1;
 Rec cfg=reference(team,64),offense=cfg?reference(cfg,5):0,defense=cfg?reference(cfg,0):0;
 if(!cfg || !offense || !defense)ok=0;
 for(int p=0;p<22 && ok;p++) {
  Rec e=reference(p<10?offense:p<19?defense:cfg,fields[p]);
  if(!e){ok=0;break;}ideal[p]=(int)scalar(e,2,0x100);release(e);
 }
 release(defense);release(offense);release(cfg);
 Rec f=context_ref(eval,6),season=f?reference(f,44):0;
 unsigned year=season?scalar(season,2,0x100):10001;release(season);release(f);
 return ok?class31_annual(ideal,scalar(team,383,0x100),year,annual):-1;
}

static int hs15_add(Hs15Snapshot *s,Rec p,int returning,int incoming,int hs,int published) {
 int pos=(int)scalar(p,3,0x108),ovr=(int)scalar(p,191,0x100);
 int stars=(int)scalar(p,231,0x108);
 /* Historical prospect stars may legitimately be unset on a college player.
    Only a secured current HS/JUCO recruit contributes to the class star caps.
    Preserve the stored value for returners/portal arrivals; never invent one. */
 if(pos<0 || pos>=POSITIONS || ovr<0 || ovr>99 || (hs && (stars<0 || stars>4))) {
  log_line("HS15_DATA_ERROR,PLAYER,%u,%d,%d,%d,%d,%d,%d,%d",row(p),pos,ovr,stars,returning,incoming,hs,published);
  return 0;
 }
 U table=q((U)p+0x10);uint32_t rid=row(p);
 for(int i=0;i<s->used;i++)if(s->players[i].table==table && s->players[i].rid==rid) {
  Hs15Player *v=&s->players[i];
  if(v->pos!=pos || v->ovr!=ovr || v->stars!=stars) {
   log_line("HS15_DATA_ERROR,DUPLICATE,%u,%d,%d,%d,%d,%d,%d,%d",rid,pos,ovr,stars,returning,incoming,hs,published);
   return 0;
  }
  v->returning|=returning;v->incoming|=incoming;v->hs|=hs;v->published|=published;return 1;
 }
 if(s->used>=HS15_MAX_PLAYERS) {
  log_line("HS15_DATA_ERROR,CAPACITY,%u,%d,%d,%d,%d,%d,%d,%d",rid,pos,ovr,stars,returning,incoming,hs,published);
  return 0;
 }
 int young=pos==0 && dri_qb_young(ovr,(int)scalar(p,242,0x108),(int)scalar(p,271,0x108),returning && !incoming && !hs);
 s->players[s->used++]=(Hs15Player){table,rid,pos,ovr,returning,incoming,hs,stars,published,young,
   pos==0 && returning?r3_qb_priority(p)+young:0};
 return 1;
}
static int hs15_has(const Hs15Snapshot *s,Rec p) {
 for(int i=0;i<s->used;i++)if(s->players[i].table==q((U)p+0x10) && s->players[i].rid==row(p))return s->players[i].published;
 return 0;
}
static int hs15_team_players(Hs15Snapshot *s,Rec team,int ordinal,int returning,
                             const PortalDepartures *d) {
 Rec a=array_field(team,ordinal);if(!a)return 0;
 U n=array_count(a);if(n>192){release(a);return 0;}
 if(returning)s->roster_count=(int)n;
 unsigned tid=scalar(team,383,0x100);int ok=1;
 for(U j=0;j<n;j++) {
  Rec p=array_at(a,j);if(!p){ok=0;break;}
  int year=(int)scalar(p,242,0x108),pos=(int)scalar(p,3,0x108);
  if(year<0 || year>3 || pos<0 || pos>=POSITIONS)ok=0;
  if(ok && returning && year==3 && (pos==19 || pos==20))s->departing_specialists++;
  if(ok && (!returning || (year<3 && scalar(p,261,0x100)==tid && !portal_departed(p,d))))
   ok=hs15_add(s,p,returning,0,0,!returning);
  release(p);if(!ok)break;
 }
 release(a);return ok;
}
static int hs15_secured(Hs15Snapshot *s,Rec franchise,Rec team,U eval,int ordinal,int portal) {
 Rec a=array_field(franchise,ordinal);if(!a)return 0;
 U n=array_count(a);if(n>12000){release(a);return 0;}
 unsigned tid=scalar(team,383,0x100);int ok=1;
 for(U j=0;j<n;j++) {
  Rec r=array_at(a,j);if(!r){ok=0;break;}
  unsigned stage=scalar(r,9,0x108);
  if(stage==5 || stage==6) {
   Rec p=reference(r,5);if(!p)ok=0;
   else {
    int owns=0,owner=-1;
    if(stage==5) {
     if(portal) {
      int valid=portal_hard_owner(r,eval,&owner);
      if(valid<0)ok=0;
      owns=valid>0 && owner==(int)tid;
     } else {
      owns=own_commit(r,team);
      if(owns && mode==1 && !portal_store_owner(r,p,tid))ok=0;
     }
    } else if(portal_signed_destination(r,p,&owner))owns=owner==(int)tid;
    else owns=hs15_has(s,p);
    if(ok && owns)ok=hs15_add(s,p,0,portal,!portal,0);
    release(p);
   }
  }
  release(r);if(!ok)break;
 }
 release(a);return ok;
}
static int hs15_collect(Hs15Snapshot *s,Rec team,U eval) {
 static const int fields[POSITIONS]={5,2,1,9,8,4,3,0,6,7,3,6,1,4,5,7,0,2,8,1,6,4};
 memset(s,0,sizeof(*s));
 s->prestige=(int)scalar(team,384,0x100);
 if(s->prestige<0 || s->prestige>10)return 0;
 Rec cfg=reference(team,64);if(!cfg)return 0;
 Rec offense=reference(cfg,5),defense=reference(cfg,0);
 if(!offense || !defense){release(offense);release(defense);release(cfg);return 0;}
 int ok=1;
 for(int pos=0;pos<POSITIONS;pos++) {
  Rec e=reference(pos<10?offense:pos<19?defense:cfg,fields[pos]);
  if(!e){ok=0;break;}
  int ideal=(int)scalar(e,2,0x100);
  int starters=(int)scalar(e,4,0x100)+(int)scalar(e,0,0x100);release(e);
  if(ideal<0 || ideal>30 || starters<0 || starters>11){ok=0;break;}
  s->starters[pos]=starters?starters:1;
  s->ideal[pos]=ideal;s->total_ideal+=ideal;
 }
 release(offense);release(defense);release(cfg);
 if(!ok || s->total_ideal<40 || s->total_ideal>110 || s->ideal[0]<1)return 0;
 Rec f=context_ref(eval,6);if(!f)return 0;
 Rec season=reference(f,44);unsigned year=season?scalar(season,2,0x100):10001;release(season);
 s->annual_target=class31_annual(s->ideal,scalar(team,383,0x100),year,s->annual);
 if(s->annual_target<0){release(f);return 0;}s->annual_valid=1;
 PortalDepartures d;
 ok=portal_ownership_scope(f) && portal_departures(f,&d) &&
    hs15_team_players(s,team,239,1,&d) && hs15_team_players(s,team,33,0,&d) &&
    hs15_secured(s,f,team,eval,39,0) && hs15_secured(s,f,team,eval,46,1);
 release(f);if(!ok)return 0;
 int step=portal_projection_step(eval);if(faulted)return 0;
 s->projection_step=step;
 for(int p=0;p<POSITIONS;p++)s->flex_floor[p]=100;
 for(int j=0;j<s->used;j++) {
  Hs15Player *p=&s->players[j];int pos=p->pos;
  int projected=portal_projected_ovr(p->ovr,(p->returning || p->incoming)?step:0);
  s->count[pos]++;s->total++;
  if(p->returning && !p->incoming && !p->hs)s->returning[pos]++;
  if(p->hs){s->hs[pos]++;s->four[pos]+=p->stars==3;s->five[pos]+=p->stars==4;}
  if(projected<s->flex_floor[pos])s->flex_floor[pos]=projected;
  if(pos==0 && projected>=85)s->qb85++;
 }
 int reserved=0;
 for(int p=0;p<POSITIONS;p++)reserved+=s->count[p]>s->ideal[p]?s->count[p]:s->ideal[p];
 if((s->total_ideal==83 || s->total_ideal==84) && reserved<85)
  for(int p=1;p<POSITIONS;p++)if(p!=19 && p!=20 && s->ideal[p]>0 && s->count[p]==s->ideal[p])s->flex[p]=1;
 return 1;
}
/* Same projected players as portal_roster, collected once for the entire
   school. Secured HS players are already included in the portal baseline. */
static int hs15_portal_rooms(const Hs15Snapshot *s,PortalQuality rooms[22],int prestige,int step) {
 memset(rooms,0,22*sizeof(*rooms));
 for(int p=0;p<22;p++) {
  rooms[p].pos=p;rooms[p].ideal=s->ideal[p];rooms[p].starters=s->starters[p];rooms[p].incoming_best=-1;
  rooms[p].hs_normal_limit=s->annual_valid?s->annual[p]:0;
  rooms[p].hs_annual_valid=s->annual_valid;
 }
 for(int j=0;j<s->used;j++) {
  const Hs15Player *p=&s->players[j];PortalQuality *q=&rooms[p->pos];
  if(q->count>=192)return 0;
  int ovr=portal_projected_ovr(p->ovr,(p->returning || p->incoming)?step:0),k=q->count++;
  while(k>0 && (q->rating[k-1]<ovr || (q->rating[k-1]==ovr && q->incoming[k-1]<p->incoming))) {
   q->rating[k]=q->rating[k-1];q->incoming[k]=q->incoming[k-1];k--;
  }
  q->rating[k]=ovr;q->incoming[k]=p->incoming;
  dri_qb_observe(q,p->ovr,ovr,p->qb_young && p->returning && !p->incoming && !p->hs,p->qb_priority);
  if(!p->incoming) {
   q->base_count++;
   if(!p->returning)q->hs_count++;
  } else if(ovr>q->incoming_best)q->incoming_best=ovr;
 }
 dri_portal_refresh_budget(rooms,prestige);return 1;
}
static int hs15_refresh(Session *s,Rec team,U eval) {
 Hs15Snapshot shot;if(!hs15_collect(&shot,team,eval))return 0;
 s->hs_snapshot=shot;
 for(int p=0;p<POSITIONS;p++) {
  s->ideal[p]=shot.ideal[p];s->limit[p]=shot.annual_valid?shot.annual[p]:dri_annual_limit(p,shot.ideal[p]);
  s->returning[p]=shot.returning[p];s->projected[p]=shot.count[p];
  s->commits[p]=shot.hs[p];s->hs_four[p]=shot.four[p];s->hs_five[p]=shot.five[p];
  s->flex[p]=shot.flex[p];s->flex_floor[p]=shot.flex_floor[p];
 }
 s->qb85=shot.qb85;s->roster_count=shot.roster_count;s->departing_specialists=shot.departing_specialists;
 PortalQuality rooms[22];
 if(!hs15_portal_rooms(&shot,rooms,shot.prestige,shot.projection_step))return 0;
 int inherited_excess=0,hs_extra=0,portal_extra=0;
 for(int p=0;p<22;p++) {
  int inherited=rooms[p].base_count-rooms[p].hs_count;
  inherited_excess+=inherited>rooms[p].ideal?inherited-rooms[p].ideal:0;
  hs_extra+=dri_hs_extra_count(&rooms[p]);
  int ceiling=rooms[p].base_count>rooms[p].ideal?rooms[p].base_count:rooms[p].ideal;
  portal_extra+=rooms[p].count>ceiling?rooms[p].count-ceiling:0;
 }
 log_line("HS28R5_BUDGET,%llu,%d,%d,%d,%d,%u,%s",s->pass,s->team,
  inherited_excess,hs_extra,portal_extra,rooms[0].exception_states & ~DRI_EXCEPTION_READY,s->mode?"ACTIVE":"DIAGNOSTIC");
 return 1;
}
static int hs15_snapshot_block(const Hs15Snapshot *s,Rec recruit);
static int hs15_cached_block(Rec recruit,const Session *s) {
 if(!hs15_is_recruit(recruit))return 0;
 return hs15_snapshot_block(&s->hs_snapshot,recruit);
}
static int hs15_snapshot_block(const Hs15Snapshot *s,Rec recruit) {
 if(!hs15_is_recruit(recruit))return -1;
 Rec p=reference(recruit,5);if(!p)return -1;
 int pos=(int)scalar(p,3,0x108),stars=(int)scalar(p,231,0x108),ovr=(int)scalar(p,191,0x100);release(p);
 if(pos<0 || pos>=POSITIONS || stars<0 || stars>4 || ovr<0 || ovr>99)return -1;
 return hs15_snapshot_values(s,pos,stars,ovr);
}
/* R5: one material elite upgrade to a numerically full HS room may compete
   for an existing shared general place. No QB/specialist override, inherited
   overstack expansion, second-five-star waiver, or new allowance. Compare
   against both the secured HS class and a bounded developmental depth seat. */
static int hs29_quality_extra(const Hs15Snapshot *s,int pos,int stars,int ovr) {
 if(pos<1 || pos>18 || stars<3 || s->ideal[pos]<=0 ||
    s->count[pos]!=s->ideal[pos] || s->hs[pos]<=0)return 0;
 int depth=2*s->starters[pos]+1;if(depth>s->ideal[pos])depth=s->ideal[pos];
 int ratings[192],n=0,best_hs=-1;
 for(int i=0;i<s->used;i++)if(s->players[i].pos==pos) {
  const Hs15Player *p=&s->players[i];
  int rating=portal_projected_ovr(p->ovr,(p->returning || p->incoming)?s->projection_step:0);
  if(n>=192)return 0;
  int k=n++;while(k>0 && ratings[k-1]<rating){ratings[k]=ratings[k-1];k--;}
  ratings[k]=rating;
  if(p->hs && p->ovr>best_hs)best_hs=p->ovr;
 }
 return depth>0 && depth<=n && best_hs>=0 &&
  ovr>=best_hs+4 && ovr>=ratings[depth-1]+4;
}
static int hs34_vacancy_extra(const Hs15Snapshot *s,int pos) {
 if(pos<0 || pos>=22)return 0;
 int secured=0;for(int p=0;p<22;p++)secured+=s->hs[p];
 return dri_hs_vacancy_extra(s->annual_valid,
  class34r2_capacity(s->annual_valid,s->annual_target,0,2),secured,s->total,
  pos,s->ideal[pos],s->annual[pos],s->hs[pos],s->count[pos]);
}
static int hs15_snapshot_values(const Hs15Snapshot *s,int pos,int stars,int ovr) {
 int normal=s->annual_valid && class31_remaining(s->annual,s->hs,pos)>0;
 int vacancy=hs34_vacancy_extra(s,pos);
 int limit=s->annual_valid?s->annual[pos]:dri_annual_limit(pos,s->ideal[pos]);
 int flex=s->flex[pos] && ovr>=s->flex_floor[pos];
 int reason=dri_hs_candidate_block(pos,s->ideal[pos],limit,s->hs[pos],
  s->count[pos],flex,stars,s->four[pos],s->five[pos],ovr,s->qb85);
 /* Ordinary annual class slots are independent of inherited room size and
    the two discretionary general places. Retain the elite-QB and five-star
    stacking rules; multiple four-stars may fill distinct normal class slots.
    After the annual quota is filled, only the bounded Test34 vacancy or
    existing quality exceptions can create an optional class seat. */
 if(s->annual_valid) {
  if(normal && (reason==1 || reason==3))reason=0;
  else if(!normal && !reason)reason=1;
 }
 /* One second five-star in a developmental position per school/class.
    Reconstruct use from secured class identities: reloads cannot replenish it.
    This relaxes the star cap only; actual position and team space are required. */
 if(reason==2 && stars==4 && pos>=5 && pos<=18 && s->five[pos]==1 &&
    s->count[pos]<s->ideal[pos] && s->total<85) {
  int used=0,lo=pos<=9?5:pos<=12?10:pos<=15?13:16;
  int hi=pos<=9?9:pos<=12?12:pos<=15?15:18,cohort=0,ideal=0;
  for(int p=5;p<=18;p++)if(s->five[p]>1)used+=s->five[p]-1;
  for(int p=lo;p<=hi;p++){cohort+=s->hs[p];ideal+=s->ideal[p];}
  if(!used && cohort<(ideal+1)/3+1)
   reason=dri_hs_candidate_block(pos,s->ideal[pos],limit,
    s->hs[pos],s->count[pos],flex,stars,s->four[pos],0,ovr,s->qb85);
 }
 if((reason==1 || reason==3) && hs29_quality_extra(s,pos,stars,ovr))reason=0;
 if((reason==1 || reason==3) && vacancy)reason=0;
 if(!reason && normal)return 0;
 if(reason || (s->count[pos]<s->ideal[pos] && !vacancy))return reason;
 /* Evaluate the complete prospective class. Adding an HS player can change
    how a secured portal extra fits the general/emergency/RB buckets. Testing
    the post-addition state prevents consuming the last general place twice. */
 PortalQuality rooms[22];
 if(!hs15_portal_rooms(s,rooms,s->prestige,s->projection_step))return -1;
 PortalQuality *q=&rooms[pos];if(q->count>=192)return -1;
 int k=q->count++;
 while(k>0 && q->rating[k-1]<ovr) {
  q->rating[k]=q->rating[k-1];q->incoming[k]=q->incoming[k-1];k--;
 }
 q->rating[k]=ovr;q->incoming[k]=0;q->base_count++;q->hs_count++;
 dri_portal_refresh_budget(rooms,s->prestige);
 return (rooms[0].exception_states & ~DRI_EXCEPTION_READY)?0:8;
}
#ifdef DRI_EMULATION_TEST
/* Numeric projection only. This uses the same collector and shared budget
   as production acceptance; it cannot write players, offers or funding. */
__declspec(dllexport) int DriTest31ClassAnnual(Rec team,U eval,int *out) {
 Hs15Snapshot s;PortalQuality rooms[22];
 if(!hs15_collect(&s,team,eval) || !hs15_portal_rooms(&s,rooms,s.prestige,s.projection_step))return 0;
 out[0]=s.annual_target;
 for(int p=0;p<22;p++){out[1+p]=s.annual[p];out[23+p]=s.hs[p];}
 out[45]=(int)(rooms[0].exception_states & ~DRI_EXCEPTION_READY);return 1;
}
#endif
#ifdef DRI_EMULATION_TEST
static int hs15_snapshot_recovery(const Hs15Snapshot *s,Rec recruit) {
 int pos=position(recruit);if(pos<0 || pos>=POSITIONS)return -1;
 int target=pos==19 || pos==20?2:s->ideal[pos]+s->flex[pos];
 if(s->ideal[pos]<=0 || s->count[pos]>=target)return 0;
 int blocked=hs15_snapshot_block(s,recruit);return blocked<0?-1:!blocked;
}
#endif
/* Returns the policy reason; only active mode supplies a veto. A native already
   secured commitment is finalized normally. Merely being a board target or
   receiving permission from an earlier callback does not reserve a roster spot. */
static int hs15_decision(Rec recruit,Rec team,U eval,const char *site,int accepting) {
 if(mode<0 || faulted || !hs15_is_recruit(recruit) || !team)return 0;
 if(academy29_hs_block(recruit,team))return mode==1?20:0;
 Rec user=reference(team,407);if(user){release(user);return 0;}
 unsigned stage=scalar(recruit,9,0x108);int active=mode==1;
 /* Native HS transitions also run in bowls and offseason week0/1.
    Admission applies whenever an actual outcome is proposed; only allocation
    and market maintenance use hs15_window. */
 if(!accepting) {
  int window=hs15_window(eval);
  if(window<0){portal_fault("HS_window");return active?7:0;}
  if(!window)return 0;
 }
 if(stage>=6)return 0;
 if(stage==5) {
  Rec f=context_ref(eval,6),p=reference(recruit,5);
  if(!f || !p || !portal_ownership_scope(f)) {
   release(f);release(p);portal_fault("HS_hard_owner");return active?7:0;
  }
  release(f);int owner=-1,known=portal_signed_destination(recruit,p,&owner);release(p);
  unsigned tid=scalar(team,383,0x100);
  int reason=known?(owner==(int)tid?0:6):(own_commit(recruit,team)?0:7);
  if(reason)log_line("HS15_COMMIT_BLOCK,%s,%u,%u,%d,%d,%s",site,tid,row(recruit),owner,reason,mode?"ACTIVE":"DIAGNOSTIC");
  return active?reason:0;
 }
 Hs15Snapshot s;
 if(!hs15_collect(&s,team,eval)) {
  log_line("HS15_DATA_ERROR,PROJECTION,%s,%u,%u",site,scalar(team,383,0x100),row(recruit));
  portal_fault("HS_projection");return active?7:0;
 }
 int reason=hs15_snapshot_block(&s,recruit);
 if(reason<0){portal_fault("HS_candidate");return active?7:0;}
 int pos=position(recruit);
 log_line("HS15_DECISION,%s,%u,%u,%d,%d,%d,%d,%d,%d,%d,%s",site,
  scalar(team,383,0x100),row(recruit),pos,s.ideal[pos],s.count[pos],s.hs[pos],
  s.four[pos],s.five[pos],reason,mode?"ACTIVE":"DIAGNOSTIC");
 return mode==1?reason:0;
}
