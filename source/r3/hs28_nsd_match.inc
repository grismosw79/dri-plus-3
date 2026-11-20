/* R8 deadline matching. Recruits propose to existing eligible offers in earned
   influence order; each authorized opening retains its primary before its
   backup. A rejected recruit tries its next offer. Every proposal consumes an
   edge, so crossed priorities cannot recurse or wait forever. Only numeric
   resolved identities survive this read-only calculation. The verified native
   recruiting/NSD loop serializes these callbacks; no native signing service
   is invoked from this solver and its shared workspace is not reentrant. */
#define HS28_NSD_MAX (138*50)
#define HS28_NSD_SLOTS (22*32)
typedef struct { Rec rec;U table;unsigned rid;int first,count,next,match; } Hs28NsdNode;
typedef struct { int node,team,slot,priority;unsigned points; } Hs28NsdEdge;
typedef struct { int loaded,n;Hs28Choice choices[MAX_BOARD]; } Hs28NsdTeam;
typedef struct { U table;unsigned rid;int team; } Hs28NsdResult;
static struct {
 Hs28NsdNode nodes[HS28_NSD_MAX];Hs28NsdEdge edges[HS28_NSD_MAX];
 Hs28NsdTeam teams[138];int holds[138][HS28_NSD_SLOTS];int n,ne;
} hs28_nsd_work;
static struct {
 U eval,table;unsigned rid,year,week,type;int n,valid;
 Hs28NsdResult results[HS28_NSD_MAX];
} hs28_nsd_cache;
static void hs28_nsd_reset(void){hs28_nsd_cache.valid=0;hs28_nsd_cache.n=0;}
static int hs28_nsd_scope(U eval) {
 Rec f=context_ref(eval,6),s=f?reference(f,44):0;
 if(!f || !s){release(s);release(f);return 0;}
 U table=q((U)f+0x10);unsigned rid=row(f),year=scalar(s,2,0x100),week=scalar(s,4,0x100),type=scalar(s,5,0x108);
 release(s);release(f);
 if(!hs28_nsd_cache.valid || hs28_nsd_cache.eval!=eval || hs28_nsd_cache.table!=table ||
    hs28_nsd_cache.rid!=rid || hs28_nsd_cache.year!=year || hs28_nsd_cache.week!=week || hs28_nsd_cache.type!=type) {
  hs28_nsd_reset();hs28_nsd_cache.eval=eval;hs28_nsd_cache.table=table;hs28_nsd_cache.rid=rid;
  hs28_nsd_cache.year=year;hs28_nsd_cache.week=week;hs28_nsd_cache.type=type;hs28_nsd_cache.valid=1;
 }
 return 1;
}
static int hs28_nsd_offer_allowed(Rec recruit,Rec team,U eval,unsigned *points) {
 int offered=portal_offer_total(recruit,team,points);
 if(offered<0){portal_fault("HS_NSD_offer");return 0;}
 if(!offered)return 0;
 int blocked=cpu_recruit_decision(recruit,team,eval,"NSD_OFFER",0);
 if(blocked || faulted){log_line("TEST29R2_NSD_EDGE,%u,%u,%d,ROSTER_POLICY",row(recruit),scalar(team,383,0x100),blocked);return 0;}
 int eligible=native_recruit_eligible(eval,team,recruit);
 if(!eligible)log_line("TEST29R2_NSD_EDGE,%u,%u,0,NATIVE_ELIGIBILITY",row(recruit),scalar(team,383,0x100));
 return eligible;
}
static int hs28_nsd_add(Rec recruit) {
 U table=q((U)recruit+0x10);unsigned rid=row(recruit);
 for(int i=0;i<hs28_nsd_work.n;i++)if(hs28_nsd_work.nodes[i].table==table && hs28_nsd_work.nodes[i].rid==rid)return i;
 if(hs28_nsd_work.n>=HS28_NSD_MAX){portal_fault("HS_NSD_component_bounds");return -1;}
 int i=hs28_nsd_work.n++;((void(*)(Rec))vfunc(recruit,0))(recruit);
 hs28_nsd_work.nodes[i]=(Hs28NsdNode){.rec=recruit,.table=table,.rid=rid,.match=-1};return i;
}
static int hs28_nsd_plan(Rec team,U eval,int tid) {
 Hs28NsdTeam *p=&hs28_nsd_work.teams[tid];if(p->loaded)return 1;
 Hs15Snapshot shot;
 if(!hs15_collect(&shot,team,eval) || !hs28_make_board_plan(team,eval,&shot,p->choices,&p->n,1))return 0;
 for(int i=0;i<p->n;i++)if(hs28_opening_block(&shot,p->choices[i].pos,p->choices[i].stars,p->choices[i].ovr))p->choices[i].selected=0;
 p->loaded=1;return 1;
}
static int hs28_nsd_partners(Rec team,const Hs28NsdTeam *p,int slot) {
 Rec a=board_array(team);if(!a)return 0;U n=array_count(a);int ok=n<=MAX_BOARD;
 for(U j=0;j<n && ok;j++) {
  Rec t=array_at(a,j),r=t?reference(t,10):0;
  if(!t || !r)ok=0;
  for(int k=0;k<p->n && r && ok;k++)if(p->choices[k].selected && p->choices[k].legal && p->choices[k].offered &&
      p->choices[k].slot==slot && hs28_identity(&p->choices[k],r))ok=hs28_nsd_add(r)>=0;
  release(r);release(t);
 }
 release(a);return ok;
}
static int hs28_nsd_build(Rec recruit,U eval) {
 memset(&hs28_nsd_work,0,sizeof(hs28_nsd_work));memset(hs28_nsd_work.holds,0xff,sizeof(hs28_nsd_work.holds));
 int ok=hs28_nsd_add(recruit)>=0;
 for(int i=0;i<hs28_nsd_work.n && ok && !faulted;i++) {
  Hs28NsdNode *node=&hs28_nsd_work.nodes[i];Rec r=node->rec;
  node->first=hs28_nsd_work.ne;
  int owner=-1,published=portal_published_owner(r,eval,&owner);
  if(published<0){ok=0;break;}
  if(published || scalar(r,9,0x108)>=5)continue;
  unsigned points[138]={0},listed[138]={0};Rec top;U nt;
  if(!hs_read_school_list(r,scalar(r,9,0x108),&top,&nt)){ok=0;break;}
  ok=nt<=138;
  for(U j=0;j<nt && ok;j++) {
   Rec e=array_at(top,j);if(!e){ok=0;break;}
   unsigned tid=scalar(e,0,0x100),value=scalar(e,1,0x100);release(e);
   if(tid>=138 || value>65535 || listed[tid]++)ok=0;else points[tid]=value;
  }
  release(top);
  for(int tid=0;tid<138 && ok && !faulted;tid++) {
   Rec team=portal_team_by_id(eval,(unsigned)tid);if(!team){ok=0;break;}
   unsigned value=0;int allowed=hs28_nsd_offer_allowed(r,team,eval,&value),slot=-1,priority=1;
   if(allowed) {
    Rec user=reference(team,407);int cpu=!user;release(user);
    if(cpu) {
     if(!hs28_nsd_plan(team,eval,tid)){release(team);ok=0;break;}
     Hs28NsdTeam *p=&hs28_nsd_work.teams[tid];allowed=0;
     for(int k=0;k<p->n;k++)if(hs28_identity(&p->choices[k],r) && p->choices[k].selected && p->choices[k].legal && p->choices[k].offered) {
      slot=p->choices[k].slot;priority=p->choices[k].choice;allowed=slot>0 && slot<HS28_NSD_SLOTS;break;
     }
     if(!allowed) {
      int found=0;
      for(int k=0;k<p->n;k++)if(hs28_identity(&p->choices[k],r)) {
       const Hs28Choice *c=&p->choices[k];found=1;
       log_line("TEST29R2_NSD_PLAN_EDGE,%u,%d,%d,%d,%d,%d,%d,%d,NO_SELECTED_OPENING",row(r),tid,c->native_score,c->legal,c->viable,c->selected,c->slot,c->choice);break;
      }
      if(!found)log_line("TEST29R2_NSD_EDGE,%u,%d,0,MISSING_BOARD_CHOICE",row(r),tid);
     }
     if(allowed && !hs28_nsd_partners(team,p,slot)){release(team);ok=0;break;}
    }
   }
   if(allowed) {
    if(hs28_nsd_work.ne>=HS28_NSD_MAX){release(team);ok=0;break;}
    hs28_nsd_work.edges[hs28_nsd_work.ne++]=(Hs28NsdEdge){i,tid,slot,priority,listed[tid]?points[tid]:value};node->count++;
   }
   release(team);
  }
  for(int j=1;j<node->count;j++) {
   Hs28NsdEdge e=hs28_nsd_work.edges[node->first+j];int k=j;
   while(k>0){Hs28NsdEdge *prior=&hs28_nsd_work.edges[node->first+k-1];
    if(prior->points>e.points || (prior->points==e.points && prior->team<e.team))break;
    hs28_nsd_work.edges[node->first+k]=*prior;k--;
   }
   hs28_nsd_work.edges[node->first+k]=e;
  }
 }
 if(ok && !faulted) {
  int progress=1,proposals=0;
  while(progress){progress=0;
   for(int i=0;i<hs28_nsd_work.n;i++) {
    Hs28NsdNode *node=&hs28_nsd_work.nodes[i];if(node->match>=0 || node->next>=node->count)continue;
    progress=1;int edge=node->first+node->next++;Hs28NsdEdge *e=&hs28_nsd_work.edges[edge];proposals++;
    if(e->slot<0){node->match=edge;continue;} /* User offers keep manual roster policy. */
    int previous=hs28_nsd_work.holds[e->team][e->slot];
    if(previous<0 || e->priority<hs28_nsd_work.edges[previous].priority ||
       (e->priority==hs28_nsd_work.edges[previous].priority && node->rid<hs28_nsd_work.nodes[hs28_nsd_work.edges[previous].node].rid)) {
     if(previous>=0)hs28_nsd_work.nodes[hs28_nsd_work.edges[previous].node].match=-1;
     hs28_nsd_work.holds[e->team][e->slot]=edge;node->match=edge;
    }
   }
  }
  if(proposals>hs28_nsd_work.ne){ok=0;} /* Defensive bound; every edge is proposed at most once. */
  if(ok) {
   if(hs28_nsd_cache.n+hs28_nsd_work.n>HS28_NSD_MAX)hs28_nsd_cache.n=0;
   for(int i=0;i<hs28_nsd_work.n;i++) {
    Hs28NsdNode *node=&hs28_nsd_work.nodes[i];int tid=node->match>=0?hs28_nsd_work.edges[node->match].team:-1;
    hs28_nsd_cache.results[hs28_nsd_cache.n++]=(Hs28NsdResult){node->table,node->rid,tid};
   }
   log_line("HS28_NSD_MATCH,%u,%d,%d,%d",row(recruit),hs28_nsd_work.n,hs28_nsd_work.ne,proposals);
  }
 }
 for(int i=0;i<hs28_nsd_work.n;i++){release(hs28_nsd_work.nodes[i].rec);hs28_nsd_work.nodes[i].rec=0;}
 if(!ok && !faulted)portal_fault("HS_NSD_match_validation");
 return ok && !faulted;
}
static Rec hs28_nsd_destination(Rec recruit,U eval) {
 if(mode!=1 || faulted || !recruit || !hs15_is_recruit(recruit) || scalar(recruit,9,0x108)>=5 || !hs28_nsd_scope(eval))return 0;
 U table=q((U)recruit+0x10);unsigned rid=row(recruit);
 for(int attempt=0;attempt<2 && !faulted;attempt++) {
  int found=-1;
  for(int i=hs28_nsd_cache.n-1;i>=0;i--)if(hs28_nsd_cache.results[i].table==table && hs28_nsd_cache.results[i].rid==rid){found=i;break;}
  if(found<0){if(!hs28_nsd_build(recruit,eval))return 0;
   for(int i=hs28_nsd_cache.n-1;i>=0;i--)if(hs28_nsd_cache.results[i].table==table && hs28_nsd_cache.results[i].rid==rid){found=i;break;}
  }
  if(found<0)return 0;int tid=hs28_nsd_cache.results[found].team;if(tid<0)return 0;
  Rec team=portal_team_by_id(eval,(unsigned)tid);unsigned points=0;
  if(team && hs28_nsd_offer_allowed(recruit,team,eval,&points))return team;
  release(team);hs28_nsd_reset();if(!hs28_nsd_scope(eval))return 0;
 }
 return 0;
}
#ifdef DRI_EMULATION_TEST
__declspec(dllexport) void DriTestNsdReset(void){hs28_nsd_reset();}
#endif
