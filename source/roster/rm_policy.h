#ifndef RM_POLICY_H
#define RM_POLICY_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#define RM_TEAMS 138
#define RM_PLAYERS 18000
#define RM_ROOM 200
#define RM_ACTIONS 2000
#define RM_POS 22
#define RM_CHARTS 35
/* Current-stage OVR/configuration projection, conservative ties and actual
   source chart opportunity. This is NOT a replica of the native chart scorer. */
enum { RM_STARTER=0,RM_ROTATION=1,RM_SUCCESSION=2,RM_SPECIALIST=3,RM_HS_DEPTH=4,RM_NO_ROLE=9 };
enum { RM_CAPACITY,RM_SPECIALISTS,RM_FLOOR,RM_FILL_SPECIALISTS,RM_ACADEMY_FLOOR,RM_ACADEMY_SPECIALISTS,RM_MINIMUM_FILL,RM_ACADEMY_MINIMUM,RM_FLEX_FLOOR,RM_ACADEMY_FLEX_FLOOR,RM_FINAL_CUT,RM_ROOM_REPAIR,RM_ACADEMY_ROOM_REPAIR,RM_SPECIALIST_REHOME };
enum { RM_USER_UNDER85=1,RM_USER_OVER90=2,RM_USER_COVERAGE=4 };
typedef struct {int starters,depth,ideal,minimum;} RMConfig;
typedef struct {int id,original,team,pos,ovr,dev,year,injured,arrival,user,pool,hs,held,paid,moved,chart_depth;uint64_t chart;int next,fb_chart_depth;} RMPlayer;
typedef struct {int user,academy,prestige,count,initial,head,budget,reserved;RMConfig config[RM_POS];} RMTeam;
typedef struct {int player,src,dst,phase,role,quote;} RMAction;
typedef int (*RMGate)(void*,int,int,int*);
typedef struct {RMPlayer p[RM_PLAYERS];RMTeam t[RM_TEAMS];RMAction a[RM_ACTIONS];int n,na,fault;RMGate gate;void *ctx;int *quotes;int queue[RM_PLAYERS],nq;unsigned char attempted[RM_PLAYERS];int source_rank[RM_PLAYERS],source_young_rank[RM_PLAYERS];int kicking[RM_PLAYERS][3];int user_fallback[RM_TEAMS];int committed;unsigned char *stale_edges;} RMState;
static int rm_min(int a,int b){return a<b?a:b;}
static int rm_max(int a,int b){return a>b?a:b;}
static int rm_unit(int p){static const int u[22]={0,1,2,3,4,5,6,7,6,5,10,10,12,13,14,13,16,17,17,19,20,21};return u[p];}
/* Native fills tackle/guard/end/OLB pairs together, but FS and SS separately.
   Coverage may use both safeties; a promised chart job may not. */
static int rm_job_unit(int p){return p==18?18:rm_unit(p);}
static int rm_unit_positions(int p){int n=0;for(int k=0;k<RM_POS;k++)n+=rm_job_unit(k)==rm_job_unit(p);return n;}
/* One ENC pass only. Native user identity and individual player protections
   are never rewritten. Eligibility stays frozen after the entry snapshot. */
static int rm_managed(RMState*s,int t){return !s->t[t].user||s->user_fallback[t]!=0;}
static int rm_open(RMState*s,int t){return rm_managed(s,t)&&!s->t[t].academy;}
static int rm_better(RMState*s,int a,int b){RMPlayer*x=&s->p[a],*y=&s->p[b];if(x->ovr!=y->ovr)return x->ovr>y->ovr;if(x->dev!=y->dev)return x->dev>y->dev;if(x->year!=y->year)return x->year<y->year;return x->id<y->id;}
static int rm_ids(RMState*s,int t,int*out){int n=0;for(int i=s->t[t].head;i>=0;i=s->p[i].next){if(n==RM_ROOM){s->fault=1;return 0;}out[n++]=i;}return n;}
static int rm_count(RMState*s,int t,int pos,int unit){int n=0;for(int i=s->t[t].head;i>=0;i=s->p[i].next)n+=unit?rm_unit(s->p[i].pos)==rm_unit(pos):s->p[i].pos==pos;return n;}
static int rm_cfg(RMState*s,int t,int pos,int which){int n=0;for(int j=0;j<RM_POS;j++)if(rm_unit(j)==rm_unit(pos)){RMConfig*c=&s->t[t].config[j];n+=which==0?(j==21?0:c->starters):which==1?rm_min(c->depth,c->ideal):which==2?c->ideal:c->minimum;}return n;}
enum { RM_HB_POS=1, RM_FB_POS=2, RM_TE_POS=4, RM_OFFENSE_POSITIONS=10, RM_CHART_MAX_DEPTH=30 };
typedef struct {int player,rank;unsigned offensive_starts;} RMFullbackMember;
typedef struct {int count;RMFullbackMember member[RM_ROOM];} RMFullbackRoom;
typedef struct {int count;RMFullbackMember member[RM_CHART_MAX_DEPTH];} RMFullbackCover;

/* Build a small view of the two eligible source rooms. At audit time 'seen'
   contains verified current roster membership and all chart fields start
   empty. The native chart scan supplies fresh ranks for that audit. */
static void rm_fullback_room(RMState *s,int team,int exclude,const unsigned char *seen,RMFullbackRoom *room) {
 room->count=0;
 for(int i=s->t[team].head;i>=0;i=s->p[i].next) {
  RMPlayer *p=&s->p[i];
  if(i==exclude || (seen?seen[i]!=1:p->injured) || (p->pos!=RM_HB_POS && p->pos!=RM_TE_POS))continue;
  if(room->count>=RM_ROOM){s->fault=1;return;}
  RMFullbackMember *m=&room->member[room->count++];
  m->player=i;m->rank=0;m->offensive_starts=0;
  if(!seen && p->original==team && !p->arrival && !p->pool){
   m->rank=p->fb_chart_depth;m->offensive_starts=(unsigned)(p->chart & ((1u<<RM_OFFENSE_POSITIONS)-1));
  }
 }
}
static void rm_fullback_chart_entry(RMFullbackRoom *room,int player,int pos,int rank,int starters) {
 if(pos>=RM_OFFENSE_POSITIONS || (pos!=RM_FB_POS && rank>starters))return;
 for(int j=0;j<room->count;j++)if(room->member[j].player==player) {
  RMFullbackMember *m=&room->member[j];
  if(rank<=starters)m->offensive_starts|=1u<<pos;
  if(pos==RM_FB_POS && (!m->rank || rank<m->rank))m->rank=rank;
  return;
 }
}
/* A healthy incumbent must actually appear on this team's FB chart. Preserve
   the natural room minimum, distinct starting assignments and native FB order.
   Real FB recruitment, admissions, payments and role promises use their
   existing rules. The result is evidence of coverage only. */
static void rm_assign_fullback_cover(const RMState *s,int team,int missing,const RMFullbackRoom *room,RMFullbackCover *cover) {
 cover->count=0;
 if(missing<=0)return;
 const RMConfig *cfg=s->t[team].config;
 int available[RM_POS]={0},starters[RM_POS]={0},used[RM_POS]={0};
 for(int j=0;j<room->count;j++) {
  const RMFullbackMember *m=&room->member[j];int pos=s->p[m->player].pos;
  unsigned elsewhere=m->offensive_starts & ~((1u<<pos)|(1u<<RM_FB_POS));
  if(elsewhere)continue;
  available[pos]++;
  if(m->offensive_starts & (1u<<pos))starters[pos]++;
 }
 int depth=rm_min(RM_CHART_MAX_DEPTH,rm_max(cfg[RM_FB_POS].starters,cfg[RM_FB_POS].depth));
 for(int rank=1;rank<=depth && cover->count<missing;rank++) {
  for(int j=0;j<room->count;j++) {
   const RMFullbackMember *m=&room->member[j];const RMPlayer *p=&s->p[m->player];int pos=p->pos;
   if(m->rank!=rank || p->original!=team || p->team!=team || p->pool || p->arrival || p->moved)continue;
   if(m->offensive_starts & ~(1u<<RM_FB_POS))continue;
   if(starters[pos]<cfg[pos].starters || available[pos]-used[pos]<=rm_max(cfg[pos].minimum,cfg[pos].starters))continue;
   cover->member[cover->count++]=*m;used[pos]++;
   break;
  }
 }
}
static int rm_planned_fullback_cover(RMState *s,int team,int missing,int exclude,RMFullbackCover *cover) {
 cover->count=0;
 if(missing>0) {
  RMFullbackRoom room;
  rm_fullback_room(s,team,exclude,NULL,&room);
  rm_assign_fullback_cover(s,team,missing,&room,cover);
 }
 return cover->count;
}
enum { RM_K_POS=19, RM_P_POS=20 };
static int rm_kicking_unit(int pos){return pos==RM_K_POS || pos==RM_P_POS;}
/* K and P are separate jobs. A club with no player listed at one position
   may use its own healthy opposite specialist already first on that chart.
   A dual assignment covers one starting job, never an extra depth slot. */
static int rm_kicking_incumbent(const RMState *s,int t,int pos,const RMPlayer *p,int healthy) {
 return rm_kicking_unit(pos) && s->t[t].config[pos].starters==1 &&
        p->pos==(pos==RM_K_POS?RM_P_POS:RM_K_POS) && p->team==t && p->original==t &&
        !p->pool && !p->arrival && !p->moved && healthy;
}
static int rm_planned_kicking_cover(RMState *s,int t,int pos) {
 for(int i=s->t[t].head;i>=0;i=s->p[i].next) {
  const RMPlayer *p=&s->p[i];
  /* rm_charts sets this bit only for rank one when starters == 1. */
  if(rm_kicking_incumbent(s,t,pos,p,!p->injured) && (p->chart&(UINT64_C(1)<<pos)))return i;
 }
 return -1;
}
static void rm_select_user_fallback(RMState*s){
 for(int t=0;t<RM_TEAMS;t++){
  int reason=0;
  if(s->t[t].user){
   if(s->t[t].count<85)reason|=RM_USER_UNDER85;
   if(s->t[t].count>90)reason|=RM_USER_OVER90;
   for(int p=0;p<RM_POS;p++)if(rm_unit(p)==p) {
    int listed=rm_count(s,t,p,1),missing=rm_cfg(s,t,p,3)-listed;
    if(missing>0 && p==RM_FB_POS){RMFullbackCover cover;missing-=rm_planned_fullback_cover(s,t,missing,-1,&cover);}
    if(missing>0 && listed==0 && rm_kicking_unit(p) && rm_planned_kicking_cover(s,t,p)>=0)missing--;
    if(missing>0)reason|=RM_USER_COVERAGE;
   }
  }
  s->user_fallback[t]=reason;
 }
}
static int rm_job_count(RMState*s,int t,int pos){int n=0;for(int i=s->t[t].head;i>=0;i=s->p[i].next)n+=rm_job_unit(s->p[i].pos)==rm_job_unit(pos);return n;}
static int rm_job_cfg(RMState*s,int t,int pos,int which){int n=0;for(int j=0;j<RM_POS;j++)if(rm_job_unit(j)==rm_job_unit(pos)){RMConfig*c=&s->t[t].config[j];n+=which==0?(j==21?0:c->starters):which==1?rm_min(c->depth,c->ideal):which==2?c->ideal:c->minimum;}return n;}
static void rm_link(RMState*s,int p,int t){s->p[p].team=t;s->p[p].next=s->t[t].head;s->t[t].head=p;s->t[t].count++;}
static int rm_unlink(RMState*s,int p,int t){int*at=&s->t[t].head;while(*at>=0&&*at!=p)at=&s->p[*at].next;if(*at!=p){s->fault=1;return 0;}*at=s->p[p].next;s->t[t].count--;s->p[p].team=-1;s->p[p].next=-1;return 1;}
/* Distinct configured primary/additional jobs within suitable position units.
   Original chart leaders remain source-protected, but their listed OVR cannot
   stand in for an unrelated job (e.g. a kicker listed at P2 is not a punter). */
static int rm_starters(RMState*s,int t,int*selected){
 int ids[RM_ROOM],slots[RM_ROOM],n=rm_ids(s,t,ids),m=0;for(int p=0;p<21;p++)for(int k=0;k<s->t[t].config[p].starters;k++){if(m>=RM_ROOM/2){s->fault=1;return 0;}slots[m++]=p;}
 for(int i=1;i<n;i++){int p=ids[i],j=i;while(j&&s->p[ids[j-1]].id>s->p[p].id){ids[j]=ids[j-1];j--;}ids[j]=p;}
 int cols=n+m;if(cols>=RM_ROOM*2){s->fault=1;return 0;}int64_t u[RM_ROOM+1]={0},v[RM_ROOM*2+1]={0},dist[RM_ROOM*2+1];int assign[RM_ROOM*2+1]={0},way[RM_ROOM*2+1];unsigned char used[RM_ROOM*2+1];
 for(int i=1;i<=m;i++){assign[0]=i;int j0=0;memset(used,0,sizeof(used));for(int j=1;j<=cols;j++)dist[j]=INT64_MAX/4;
  do{used[j0]=1;int i0=assign[j0],j1=0;int64_t delta=INT64_MAX/4;
   for(int j=1;j<=cols;j++)if(!used[j]){int64_t cost=1000000000;int pos=slots[i0-1];
    if(j<=n){RMPlayer*p=&s->p[ids[j-1]];if(!p->injured&&rm_job_unit(p->pos)==rm_job_unit(pos))cost=-(int64_t)p->ovr*1000000-p->dev*20000-(3-p->year)*1000-(p->pos==pos?10000:0)+j;}
    int64_t cur=cost-u[i0]-v[j];if(cur<dist[j]){dist[j]=cur;way[j]=j0;}if(dist[j]<delta){delta=dist[j];j1=j;}}
   for(int j=0;j<=cols;j++)if(used[j]){u[assign[j]]+=delta;v[j]-=delta;}else if(j)dist[j]-=delta;j0=j1;
  }while(assign[j0]);do{int j1=way[j0];assign[j0]=assign[j1];j0=j1;}while(j0);
 }
 int count=0;for(int j=1;j<=n;j++)if(assign[j]){int p=ids[j-1],pos=slots[assign[j]-1];if(!s->p[p].injured&&rm_job_unit(s->p[p].pos)==rm_job_unit(pos))selected[count++]=p;}return count;
}
static int rm_rank(RMState*s,int p,int t,int returning,int source){int rank=1;RMPlayer*q=&s->p[p];for(int i=s->t[t].head;i>=0;i=s->p[i].next){RMPlayer*x=&s->p[i];if(i!=p&&rm_job_unit(x->pos)==rm_job_unit(q->pos)&&(!returning||x->year<3)&&(!source||x->original==t)&&rm_better(s,i,p))rank++;}return rank;}
/* Role gain compares with the source depth at the start of preparation.
   Other planned departures must not rewrite a previously selected player's
   source opportunity. Retained source coverage remains checked separately. */
static int rm_source_rank(RMState*s,int p,int returning){
 int cached=returning?s->source_young_rank[p]:s->source_rank[p];if(cached)return cached;
 RMPlayer*q=&s->p[p];int rank=1;
 for(int i=0;i<s->n;i++)if(i!=p&&s->p[i].original==q->original&&rm_job_unit(s->p[i].pos)==rm_job_unit(q->pos)&&(!returning||s->p[i].year<3)&&rm_better(s,i,p))rank++;
 return rank;
}
static int rm_worst_rank(RMState*s,int p,int t,int returning){int rank=1;for(int i=s->t[t].head;i>=0;i=s->p[i].next)if(i!=p&&!s->p[i].injured&&rm_job_unit(s->p[i].pos)==rm_job_unit(s->p[p].pos)&&(!returning||s->p[i].year<3)&&s->p[i].ovr>=s->p[p].ovr)rank++;return rank;}
/* Specialist starter moves must show a clear football advantage, not just a
   differently boosted/rounded OVR: power, accuracy and awareness may not regress.
   Succession compares only with players still eligible after the senior leaves. */
static int rm_kicking_advantage(RMState*s,int p,int dst,int returning){
 if(s->p[p].pos!=19&&s->p[p].pos!=20)return 1;
 for(int i=s->t[dst].head;i>=0;i=s->p[i].next)if(i!=p&&s->p[i].pos==s->p[p].pos&&(!returning||s->p[i].year<3))
  for(int k=0;k<3;k++)if(s->kicking[p][k]<s->kicking[i][k])return 0;
 return 1;
}
static int rm_role(RMState*s,int p,int dst){RMPlayer*q=&s->p[p];int pos=q->pos,slots=rm_job_cfg(s,dst,pos,0),rank=rm_worst_rank(s,p,dst,0),src=q->original,srank=src<0?INT_MAX:rm_source_rank(s,p,0),jobs[RM_ROOM];int nj=rm_starters(s,dst,jobs);for(int i=0;i<nj;i++)if(jobs[i]==p){int ties=0;for(int k=s->t[dst].head;k>=0;k=s->p[k].next)if(k!=p&&rm_job_unit(s->p[k].pos)==rm_job_unit(pos)&&!s->p[k].injured&&s->p[k].ovr>=q->ovr)ties++;if(ties<slots&&rm_kicking_advantage(s,p,dst,0))return RM_STARTER;}
 if(rm_job_count(s,dst,pos)>rm_job_cfg(s,dst,pos,2))return RM_NO_ROLE;
 int overtakes=0,next=-1;for(int i=s->t[dst].head;i>=0;i=s->p[i].next)if(i!=p&&rm_job_unit(s->p[i].pos)==rm_job_unit(pos)&&!rm_better(s,i,p)&&(next<0||rm_better(s,i,next)))next=i;if(next>=0)overtakes=q->ovr>s->p[next].ovr;
 int width=rm_unit_positions(pos),source_depth=1+(srank-1)/width;
 if(q->chart_depth&&q->chart_depth<source_depth)source_depth=q->chart_depth;
 int destination_depth=1+(rank-1)/width;
 if(pos!=0&&pos!=19&&pos!=20&&rank<=rm_min(rm_job_cfg(s,dst,pos,1),slots+width)&&overtakes&&destination_depth<source_depth)return RM_ROTATION;
 if(q->year<3&&slots){int yrank=rm_worst_rank(s,p,dst,1),sy=src<0?INT_MAX:rm_source_rank(s,p,1),senior=0,young=0,improves=0,ynext=-1;
  for(int i=s->t[dst].head;i>=0;i=s->p[i].next)if(i!=p&&rm_job_unit(s->p[i].pos)==rm_job_unit(pos)){if(s->p[i].year==3&&rm_rank(s,i,dst,0,0)<=slots)senior=1;if(s->p[i].year<3){young++;if(!rm_better(s,i,p)&&(ynext<0||rm_better(s,i,ynext)))ynext=i;}}
  if(ynext>=0)improves=q->ovr>s->p[ynext].ovr;
  if(senior&&yrank<=slots&&yrank<sy&&(improves||yrank>young)&&rm_kicking_advantage(s,p,dst,1))return RM_SUCCESSION;
 }
 /* Specialists need a proven starter or succession role above. A second
    roster slot alone is not a qualifying transfer opportunity. */
 return RM_NO_ROLE;
}
static int rm_promises(RMState*s,int dst){for(int a=0;a<s->na;a++)if(s->a[a].dst==dst&&s->a[a].role!=RM_HS_DEPTH&&rm_role(s,s->a[a].player,dst)>s->a[a].role)return 0;return 1;}
static void rm_protect(RMState*s){
 for(int t=0;t<RM_TEAMS;t++)for(int i=s->t[t].head;i>=0;i=s->p[i].next){
  s->source_rank[i]=rm_rank(s,i,t,0,1);s->source_young_rank[i]=rm_rank(s,i,t,1,1);
 }
 for(int t=0;t<RM_TEAMS;t++){int jobs[RM_ROOM],n=rm_starters(s,t,jobs);for(int j=0;j<n;j++)s->p[jobs[j]].held=1;
  for(int i=s->t[t].head;i>=0;i=s->p[i].next){RMPlayer*p=&s->p[i];if(p->chart||p->arrival||p->injured||p->user||!rm_open(s,t)||(p->year<3&&rm_rank(s,i,t,1,1)<=rm_job_cfg(s,t,p->pos,0)))p->held=1;}}
 s->nq=0;for(int i=0;i<s->n;i++)if(s->p[i].original>=0&&!s->p[i].held)s->queue[s->nq++]=i;
 /* Source-only sort; no destination, prestige, NIL quote or role is consulted. */
 for(int i=1;i<s->nq;i++){int p=s->queue[i],j=i;RMPlayer*q=&s->p[p];int out=rm_rank(s,p,q->original,0,1)>rm_job_cfg(s,q->original,q->pos,1);
  while(j){int x=s->queue[j-1];RMPlayer*r=&s->p[x];int ro=rm_rank(s,x,r->original,0,1)>rm_job_cfg(s,r->original,r->pos,1);int less=q->original!=r->original?q->original<r->original:out!=ro?out>ro:q->ovr!=r->ovr?q->ovr<r->ovr:q->dev!=r->dev?q->dev<r->dev:q->year!=r->year?q->year>r->year:q->id<r->id;if(!less)break;s->queue[j]=x;j--;}
  s->queue[j]=p;
 }
}
static int rm_source_safe(RMState*s,int p){RMPlayer*q=&s->p[p];int t=q->original,initial=0,unitinitial=0,now=0,unitnow=0;for(int i=0;i<s->n;i++){RMPlayer*r=&s->p[i];if(r->original==t){initial+=r->pos==q->pos;unitinitial+=rm_unit(r->pos)==rm_unit(q->pos);if(r->team==t){now+=r->pos==q->pos;unitnow+=rm_unit(r->pos)==rm_unit(q->pos);}}}if(now-1<rm_min(initial,s->t[t].config[q->pos].minimum)||unitnow-1<rm_min(unitinitial,rm_cfg(s,t,q->pos,1)))return 0;
 if((q->pos==RM_HB_POS || q->pos==RM_TE_POS) && s->t[t].config[RM_FB_POS].minimum) {
  int missing=s->t[t].config[RM_FB_POS].minimum-rm_count(s,t,RM_FB_POS,0);
  if(missing>0) {
   RMFullbackCover cover;
   int before=rm_planned_fullback_cover(s,t,missing,-1,&cover);
   if(before && rm_planned_fullback_cover(s,t,missing,p,&cover)<before)return 0;
  }
 }
 return 1;
}
/* Two bits per player and destination, allocated only if live qualification
   changes during execution. The first change permits a fresh price. A repeat
   retires only that pairing for the current ENC pass. */
static int rm_note_stale(RMState *s,int player,int team) {
 if(player<0 || player>=s->n || team<0 || team>=RM_TEAMS)return -1;
 if(!s->stale_edges) {
  s->stale_edges=calloc(((size_t)s->n*RM_TEAMS+3)/4,1);
  if(!s->stale_edges)return -1;
 }
 unsigned index=(unsigned)(player*RM_TEAMS+team),shift=2*(index%4);
 unsigned char *entry=&s->stale_edges[index/4];unsigned old=(*entry>>shift)&3u;
 unsigned next=old?2u:1u;
 *entry=(unsigned char)((*entry&~(3u<<shift))|(next<<shift));
 return (int)next;
}
static int rm_quote(RMState*s,int p,int t){
 unsigned index=(unsigned)(p*RM_TEAMS+t);
 if(s->stale_edges && ((s->stale_edges[index/4]>>(2*(index%4)))&2u))return -1;
 int*at=&s->quotes[p*RM_TEAMS+t];if(*at==INT_MIN){int q=0;int ok=s->gate(s->ctx,p,t,&q);*at=ok&&q>=0?q:-1;if(ok<0)s->fault=1;}return *at;}
static int rm_flex(int phase){return phase==RM_FLEX_FLOOR||phase==RM_ACADEMY_FLEX_FLOOR;}
static int rm_floor(int phase){return phase==RM_FLOOR||phase==RM_ACADEMY_FLOOR||rm_flex(phase);}
/* Final HS depth flexibility is limited to one extra player per suitable
   field-position unit, only on a team below 85. Paired labels may use each
   other's unfilled ideal capacity. QB, K/P, LS and unused units get no extra. */
static int rm_flex_limit(RMState*s,int t,int pos){
 int ideal=rm_cfg(s,t,pos,2);
 return ideal+(pos>0&&pos<19&&ideal>0&&rm_cfg(s,t,pos,0)>0);
}
/* Explicit complete edge before a proposed release. Reserve actual native units. */
static int rm_edge(RMState*s,int p,int dst,int phase,int*role,int*quote){RMPlayer*q=&s->p[p];RMTeam*t=&s->t[dst];int academy=phase==RM_ACADEMY_ROOM_REPAIR||phase==RM_ACADEMY_FLOOR||phase==RM_ACADEMY_SPECIALISTS||phase==RM_ACADEMY_MINIMUM||phase==RM_ACADEMY_FLEX_FLOOR;
 if(!rm_managed(s,dst)||!!t->academy!=academy||dst==q->original||t->count>=90||q->moved)return 0;
 if(q->original>=0&&(!rm_open(s,q->original)||(t->initial>90&&phase!=RM_ROOM_REPAIR)))return 0;
 if(academy&&(!q->pool||q->hs!=1))return 0;
 if(rm_floor(phase)&&t->count>=85)return 0;
 if(rm_flex(phase)&&(!q->pool||!q->hs||rm_count(s,dst,q->pos,1)>=rm_flex_limit(s,dst,q->pos)))return 0;
 if((phase==RM_SPECIALISTS||phase==RM_FILL_SPECIALISTS||phase==RM_ACADEMY_SPECIALISTS)&&(q->pos!=19&&q->pos!=20))return 0;
 if((phase==RM_FILL_SPECIALISTS||phase==RM_ACADEMY_SPECIALISTS)&&rm_count(s,dst,q->pos,0)>=rm_min(2,t->config[q->pos].ideal))return 0;
 if((phase==RM_MINIMUM_FILL||phase==RM_ACADEMY_MINIMUM)&&rm_count(s,dst,q->pos,1)>=rm_cfg(s,dst,q->pos,3))return 0;
 if(q->pool&&q->hs&&!rm_flex(phase)&&rm_count(s,dst,q->pos,0)>=t->config[q->pos].ideal)return 0;
 int src=q->team; if(src>=0)rm_unlink(s,p,src);rm_link(s,p,dst);
 *role=q->pool&&q->hs?RM_HS_DEPTH:rm_role(s,p,dst);int valid=*role!=RM_NO_ROLE&&rm_promises(s,dst);
 rm_unlink(s,p,dst);if(src>=0)rm_link(s,p,src);
 if(!valid)return 0;*quote=rm_quote(s,p,dst);return *quote>=0&&*quote<=t->budget-t->reserved;
}
typedef struct {int to,next,cap;int64_t cost;} RMFlowEdge;
typedef struct {int n,ne,maxe,*head;RMFlowEdge*e;} RMFlow;
static int rm_flow_add(RMFlow*f,int a,int b,int cap,int64_t cost){if(f->ne+2>f->maxe)return -1;int at=f->ne;f->e[f->ne]=(RMFlowEdge){b,f->head[a],cap,cost};f->head[a]=f->ne++;f->e[f->ne]=(RMFlowEdge){a,f->head[b],0,-cost};f->head[b]=f->ne++;return at;}
static int rm_flow_solve(RMFlow*f,int start,int end){int n=f->n,total=0;int64_t*d=malloc(n*sizeof(*d));int*pre=malloc(n*sizeof(*pre)),*queue=malloc((n+1)*sizeof(*queue));unsigned char*inq=malloc(n);if(!d||!pre||!queue||!inq){free(d);free(pre);free(queue);free(inq);return -1;}
 for(;;){for(int i=0;i<n;i++){d[i]=INT64_MAX/4;pre[i]=-1;inq[i]=0;}int h=0,t=0;queue[t++]=start;d[start]=0;inq[start]=1;
  while(h!=t){int u=queue[h++];if(h==n+1)h=0;inq[u]=0;for(int e=f->head[u];e>=0;e=f->e[e].next){RMFlowEdge*x=&f->e[e];if(x->cap&&d[x->to]>d[u]+x->cost){d[x->to]=d[u]+x->cost;pre[x->to]=e;if(!inq[x->to]){queue[t++]=x->to;if(t==n+1)t=0;inq[x->to]=1;}}}}
  if(pre[end]<0)break;int at=end,walk=0;while(at!=start){int e=pre[at];if(e<0||++walk>n){total=-1;goto done;}f->e[e].cap--;f->e[e^1].cap++;at=f->e[e^1].to;}total++;
 }
 done:free(d);free(pre);free(queue);free(inq);return total;
}
typedef struct {int p,d,role,quote,edge,blocked;int64_t cost;} RMLink;
static int rm_match(RMState*s,int*candidates,int nc,int phase){
 if(nc<0||nc>RM_ACTIONS){s->fault=1;return 0;}if(!nc)return 0;int max=nc*RM_TEAMS,nl=0;RMLink*links=malloc((size_t)max*sizeof(*links));if(!links){s->fault=1;return 0;}
 for(int i=0;i<nc;i++)for(int t=0;t<RM_TEAMS;t++){int role,q;if(rm_edge(s,candidates[i],t,phase,&role,&q)){RMPlayer*p=&s->p[candidates[i]];int64_t cost;
  /* Fill affordable depth first; OVR/dev break ties at the same native NIL
     cost. Never invent NIL or spend above the destination's actual budget. */
  if(p->pool&&p->hs)cost=(rm_count(s,t,p->pos,1)<rm_cfg(s,t,p->pos,3)?0:INT64_C(1000000000000))+(int64_t)q*100000+(100-p->ovr)*1000+(3-p->dev)*10+t;
  else cost=role*1000000+(10-s->t[t].prestige)*10000+(s->t[t].count<85?0:1000)+t;
  links[nl++]=(RMLink){i,t,role,q,-1,0,cost};}if(s->fault){free(links);return 0;}}
 int accepted=0;for(int iteration=0;iteration<=nl;iteration++){
  int pb=1+nc,ub=pb+RM_TEAMS*RM_POS,tb=ub+RM_TEAMS*RM_POS;RMFlow f={tb+RM_TEAMS+1,0,2*(nl+nc+RM_TEAMS*(1+2*RM_POS)+1),0,0};f.head=malloc(f.n*sizeof(int));f.e=malloc(f.maxe*sizeof(*f.e));if(!f.head||!f.e){free(f.head);free(f.e);s->fault=1;break;}for(int i=0;i<f.n;i++)f.head[i]=-1;int z=f.n-1;
  for(int i=0;i<nc;i++)rm_flow_add(&f,0,1+i,1,0);
  for(int t=0;t<RM_TEAMS;t++){
   rm_flow_add(&f,tb+t,z,rm_max(0,(rm_floor(phase)?85:90)-s->t[t].count),0);
   for(int pos=0;pos<RM_POS;pos++){
    rm_flow_add(&f,pb+t*RM_POS+pos,ub+t*RM_POS+rm_unit(pos),rm_max(0,s->t[t].config[pos].ideal-rm_count(s,t,pos,0)),0);
    if(rm_unit(pos)==pos){int cap=rm_flex(phase)?rm_max(0,rm_flex_limit(s,t,pos)-rm_count(s,t,pos,1)):(phase==RM_MINIMUM_FILL||phase==RM_ACADEMY_MINIMUM)?rm_max(0,rm_cfg(s,t,pos,3)-rm_count(s,t,pos,1)):90;rm_flow_add(&f,ub+t*RM_POS+pos,tb+t,cap,0);}
   }
  }
  for(int l=0;l<nl;l++){RMLink*x=&links[l];RMPlayer*p=&s->p[candidates[x->p]];int target=p->pool&&p->hs&&!rm_flex(phase)?pb+x->d*RM_POS+p->pos:ub+x->d*RM_POS+rm_unit(p->pos);x->edge=x->blocked?-1:rm_flow_add(&f,1+x->p,target,1,x->cost);}
  int flow=rm_flow_solve(&f,0,z);if(flow<0){s->fault=1;free(f.head);free(f.e);break;}
  int picked[RM_ACTIONS],np=0,oldsrc[RM_ACTIONS];
  for(int l=0;l<nl;l++)if(links[l].edge>=0&&!f.e[links[l].edge].cap){if(np==RM_ACTIONS){s->fault=1;break;}picked[np++]=l;}
  free(f.head);free(f.e);if(s->fault)break;
  for(int j=0;j<np;j++){RMLink*x=&links[picked[j]];int p=candidates[x->p];oldsrc[j]=s->p[p].team;if(oldsrc[j]>=0)rm_unlink(s,p,oldsrc[j]);rm_link(s,p,x->d);s->t[x->d].reserved+=x->quote;}
  int bad=-1;for(int j=0;j<np;j++){RMLink*x=&links[picked[j]];int p=candidates[x->p];RMPlayer*q=&s->p[p];RMTeam*t=&s->t[x->d];int valid=t->reserved<=t->budget&&rm_promises(s,x->d);
   if(phase==RM_MINIMUM_FILL||phase==RM_ACADEMY_MINIMUM)valid=valid&&rm_count(s,x->d,q->pos,1)<=rm_cfg(s,x->d,q->pos,3);
   if(q->pool&&q->hs)valid=valid&&(rm_flex(phase)?rm_count(s,x->d,q->pos,1)<=rm_flex_limit(s,x->d,q->pos):rm_count(s,x->d,q->pos,0)<=t->config[q->pos].ideal);else valid=valid&&rm_role(s,p,x->d)<=x->role;
   if(!valid&&(bad<0||x->cost>links[picked[bad]].cost||(x->cost==links[picked[bad]].cost&&q->id>s->p[candidates[links[picked[bad]].p]].id)))bad=j;
  }
  if(bad<0){if(s->na+np>RM_ACTIONS){s->fault=1;bad=0;}else{for(int j=0;j<np;j++){RMLink*x=&links[picked[j]];int p=candidates[x->p];s->a[s->na++]=(RMAction){p,oldsrc[j],x->d,phase,x->role,x->quote};s->p[p].moved=1;}accepted=np;break;}}
  for(int j=np-1;j>=0;j--){RMLink*x=&links[picked[j]];int p=candidates[x->p];rm_unlink(s,p,x->d);if(oldsrc[j]>=0)rm_link(s,p,oldsrc[j]);s->t[x->d].reserved-=x->quote;}
  if(s->fault)break;links[picked[bad]].blocked=1;
 }
 free(links);return accepted;
}
static void rm_source_phase(RMState*s,int phase){memset(s->attempted,0,sizeof(s->attempted));for(int wave=0;wave<RM_PLAYERS&&!s->fault;wave++){int c[RM_ACTIONS],n=0;
 for(int k=0;k<s->nq;k++){int p=s->queue[k];RMPlayer*q=&s->p[p];int t=q->original;if(q->moved||s->attempted[p]||s->t[t].count<=(phase==RM_CAPACITY?90:85))continue;if(phase==RM_SPECIALISTS&&((q->pos!=19&&q->pos!=20)||rm_count(s,t,q->pos,0)<=s->t[t].config[q->pos].ideal))continue;if(!rm_source_safe(s,p))continue;
  if(n==RM_ACTIONS){s->fault=1;break;}c[n++]=p;s->attempted[p]=1;rm_unlink(s,p,t);
 }
 for(int j=n-1;j>=0;j--)rm_link(s,c[j],s->p[c[j]].original);if(!n)break;rm_match(s,c,n,phase);
 }}
/* Bounded augmenting search in the detached plan only. Rehome existing
   pending capacity arrivals without changing their releasing team's choice.
   Every displaced player needs an equal/better complete role and native gate.
   A failed search restores the complete modeled state; no live rollback. */
static int rm_capacity_home(RMState*s,int p,int action,int maxrole,int depth,unsigned char*path){
 RMState*saved=malloc(sizeof(*saved));if(!saved){s->fault=1;return 0;}memcpy(saved,s,sizeof(*s));
 int best=-1,br=0,bq=0;int64_t bc=INT64_MAX;
 for(int t=0;t<RM_TEAMS&&!s->fault;t++){int r,q;if(rm_edge(s,p,t,RM_CAPACITY,&r,&q)&&r<=maxrole){
  int64_t cost=r*1000000+(10-s->t[t].prestige)*10000+(s->t[t].count<85?0:1000)+t;
  if(cost<bc){best=t;br=r;bq=q;bc=cost;}}}
 if(s->fault){free(saved);return 0;}
 if(best>=0){int src=s->p[p].team;if(src>=0)rm_unlink(s,p,src);rm_link(s,p,best);s->p[p].moved=1;s->t[best].reserved+=bq;
  if(action<0){if(s->na==RM_ACTIONS){s->fault=1;free(saved);return 0;}action=s->na++;}
  s->a[action]=(RMAction){p,s->p[p].original,best,RM_CAPACITY,br,bq};free(saved);return 1;}
 memcpy(s,saved,sizeof(*s));
 if(!depth){free(saved);return 0;}
 int count=s->na;
 for(int a=s->committed;a<count&&!s->fault;a++){
  RMAction old=s->a[a];int x=old.player,dst=old.dst;
  if(path[a]||a==action||old.phase!=RM_CAPACITY||dst<0||dst==s->p[p].original||
    (s->t[dst].count<90&&rm_unit(s->p[x].pos)!=rm_unit(s->p[p].pos)))continue;
  path[a]=1;rm_unlink(s,x,dst);rm_link(s,x,old.src);s->p[x].moved=0;s->t[dst].reserved-=old.quote;s->a[a].dst=-1;
  int role,quote;
  if(rm_edge(s,p,dst,RM_CAPACITY,&role,&quote)&&role<=maxrole){
   int at=action,src=s->p[p].team;
   if(at<0){if(s->na==RM_ACTIONS){s->fault=1;break;}at=s->na++;}
   if(src>=0)rm_unlink(s,p,src);rm_link(s,p,dst);s->p[p].moved=1;s->t[dst].reserved+=quote;
   s->a[at]=(RMAction){p,s->p[p].original,dst,RM_CAPACITY,role,quote};
   if(rm_capacity_home(s,x,a,old.role,depth-1,path)){path[a]=0;free(saved);return 1;}
  }
  path[a]=0;if(!s->fault)memcpy(s,saved,sizeof(*s));
 }
 free(saved);return 0;
}
static int rm_repair_capacity(RMState*s){
 for(int k=0;k<s->nq&&!s->fault;k++){int p=s->queue[k],src=s->p[p].original;
  if(s->p[p].moved||s->t[src].count<=90||!rm_source_safe(s,p))continue;
  unsigned char path[RM_ACTIONS]={0};rm_capacity_home(s,p,-1,RM_SUCCESSION,3,path);
 }
 return !s->fault;
}
static int rm_final_cuts(RMState*s){
 /* Refresh destinations after fills. The bounded reassignment search keeps
    every existing role promise; a cut is never used to manufacture a move. */
 if(!rm_repair_capacity(s))return 0;
 for(int k=0;k<s->nq&&!s->fault;k++){int p=s->queue[k];RMPlayer*q=&s->p[p];int src=q->original;
  if(src<0||!rm_open(s,src)||s->t[src].count<=90||q->held||q->moved||q->arrival||q->user||q->injured||!rm_source_safe(s,p))continue;
  unsigned char path[RM_ACTIONS]={0};if(rm_capacity_home(s,p,-1,RM_SUCCESSION,3,path))continue;
  if(s->fault)return 0;
  if(s->na==RM_ACTIONS){s->fault=1;return 0;}
  /* All destinations were checked before this no-home fallback; retained
     coverage and every existing source protection remain mandatory. */
  if(!rm_unlink(s,p,src))return 0;q->moved=1;
  s->a[s->na++]=(RMAction){p,src,-1,RM_FINAL_CUT,RM_NO_ROLE,0};
 }
 return !s->fault;
}
#include "specialist_rooms.h"
/* Rebuild only uncommitted work with a fresh native quote cache. Completed
   moves remain fixed, including their promises, reservations and protections. */
static int rm_plan_remaining(RMState*s){s->quotes=malloc((size_t)s->n*RM_TEAMS*sizeof(int));if(!s->quotes)return 0;for(int i=0;i<s->n*RM_TEAMS;i++)s->quotes[i]=INT_MIN;
 rm_repair_specialist_rooms(s);
 rm_source_phase(s,RM_CAPACITY);rm_repair_capacity(s);rm_repair_specialist_rooms(s);rm_source_phase(s,RM_SPECIALISTS);
 const int phases[]={RM_ACADEMY_FLOOR,RM_ACADEMY_MINIMUM,RM_ACADEMY_SPECIALISTS,RM_FLOOR,RM_MINIMUM_FILL,RM_FILL_SPECIALISTS,RM_ACADEMY_FLEX_FLOOR,RM_FLEX_FLOOR};
 for(unsigned k=0;k<sizeof(phases)/sizeof(phases[0])&&!s->fault;k++){
  int phase=phases[k];for(int wave=0;wave<RM_ACTIONS&&!s->fault;wave++){
   int c[RM_ACTIONS],n=0;for(int p=0;p<s->n;p++)if(s->p[p].pool&&!s->p[p].moved&&!s->p[p].held){if(n==RM_ACTIONS){s->fault=1;break;}c[n++]=p;}
   if(s->fault||!rm_match(s,c,n,phase))break;
  }
 }
 if(!s->fault)rm_final_cuts(s);
 free(s->quotes);s->quotes=0;return !s->fault;
}
static int rm_plan(RMState*s){rm_select_user_fallback(s);rm_protect(s);return rm_plan_remaining(s);}
/* This changes detached bookkeeping only. No native ownership, NIL or player
   state is rolled back. Source order/ranks and user eligibility stay frozen. */
static int rm_rewind_pending(RMState*s){
 if(s->committed<0||s->committed>s->na||s->quotes)return 0;
 for(int i=s->na-1;i>=s->committed;i--){RMAction*a=&s->a[i];
  if(a->player<0||a->player>=s->n||a->src< -1||a->src>=RM_TEAMS||a->dst< -1||a->dst>=RM_TEAMS||a->src==a->dst)return 0;
  RMPlayer*p=&s->p[a->player];
  if(!p->moved||p->team!=a->dst||p->original!=a->src||a->quote<0)return 0;
  if(a->dst>=0){if(s->t[a->dst].reserved<a->quote||!rm_unlink(s,a->player,a->dst))return 0;s->t[a->dst].reserved-=a->quote;}
  else if(a->phase!=RM_FINAL_CUT)return 0;
  if(a->src>=0)rm_link(s,a->player,a->src);p->moved=0;
 }
 s->na=s->committed;return !s->fault;
}
static int rm_replan_remaining(RMState*s){return rm_rewind_pending(s)&&rm_plan_remaining(s);}
#endif
