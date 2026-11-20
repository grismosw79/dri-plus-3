/* Runs on the detached ENC plan. Give a school missing a natural K or P
   first access to legal specialists before general placements fill its room. */
static int rm_specialist_donor(RMState *s,int i) {
 RMPlayer *p=&s->p[i];int t=p->original,other=p->pos==RM_K_POS?RM_P_POS:RM_K_POS;
 if(t<0 || p->team!=t || !rm_open(s,t) || s->t[t].count<=85)return 0;
 int same=rm_count(s,t,p->pos,0),opposite=rm_count(s,t,other,0);
 if(same+opposite<4 || same-1<rm_max(1,s->t[t].config[p->pos].minimum) ||
    opposite<rm_max(1,s->t[t].config[other].minimum))return 0;
 int healthy_same=0,healthy_other=0;
 for(int j=s->t[t].head;j>=0;j=s->p[j].next)if(j!=i&&!s->p[j].injured) {
  healthy_same+=s->p[j].pos==p->pos;healthy_other+=s->p[j].pos==other;
 }
 return healthy_same && healthy_other;
}
static int rm_find_specialist(RMState *s,int t,int pos,RMAction *best) {
 int64_t best_cost=INT64_MAX;
 int phase=s->t[t].academy?RM_ACADEMY_ROOM_REPAIR:RM_ROOM_REPAIR;
 int kickers=rm_count(s,t,RM_K_POS,0),punters=rm_count(s,t,RM_P_POS,0);
 for(int i=0;i<s->n&&!s->fault;i++) {
  RMPlayer *p=&s->p[i];
  if(!rm_kicking_unit(p->pos) || (pos>=0 && p->pos!=pos) || p->moved || p->held ||
     p->injured || p->user || p->arrival || (!p->pool&&!rm_specialist_donor(s,i)))continue;
  int count=p->pos==RM_K_POS?kickers:punters;
  if(count>=rm_min(2,s->t[t].config[p->pos].ideal))continue;
  int role,price;
  if(!rm_edge(s,i,t,phase,&role,&price))continue;
  /* Prefer a qualified donor above the ceiling, then another surplus donor,
     then unsigned depth. Compare job, price and ability within each group. */
  int group=p->pool?2:s->t[p->original].count>90?0:1;
  int64_t cost=(int64_t)group*INT64_C(1000000000000)+(int64_t)role*INT64_C(10000000000)+
               (int64_t)price*100000+(100-p->ovr)*100+(3-p->dev);
  if(cost<best_cost || (cost==best_cost && p->id<s->p[best->player].id)) {
   best_cost=cost;*best=(RMAction){i,p->team,t,phase,role,price};
  }
 }
 return !s->fault && best_cost<INT64_MAX;
}
static int rm_place_room_action(RMState *s,RMAction a) {
 if(s->na>=RM_ACTIONS){s->fault=1;return 0;}
 if(a.src>=0 && !rm_unlink(s,a.player,a.src))return 0;
 rm_link(s,a.player,a.dst);s->p[a.player].moved=1;s->t[a.dst].reserved+=a.quote;
 s->a[s->na++]=a;return 1;
}
static int rm_make_specialist_room(RMState *s,int t,int pos) {
 /* Try the entire two-move chain before keeping either proposal. A full club
    may rehome a true field-position surplus to an open, qualifying role. */
 if(!rm_open(s,t))return 0;
 for(int k=0;k<s->nq&&!s->fault;k++) {
  int i=s->queue[k];RMPlayer *p=&s->p[i];
  if(p->original!=t || p->team!=t || p->moved || p->held || p->pos>=RM_K_POS ||
     rm_count(s,t,p->pos,0)<=s->t[t].config[p->pos].ideal || !rm_source_safe(s,i))continue;
  int dest=-1,role=RM_NO_ROLE,price=0;
  for(int d=0;d<RM_TEAMS&&!s->fault;d++) {
   int r,q;if(!rm_edge(s,i,d,RM_CAPACITY,&r,&q))continue;
   if(dest<0 || r<role || (r==role && q<price)){dest=d;role=r;price=q;}
  }
  if(dest<0 || s->fault)continue;
  RMAction out={i,t,dest,RM_SPECIALIST_REHOME,role,price},in;
  if(!rm_place_room_action(s,out))return 0;
  if(rm_find_specialist(s,t,pos,&in))return rm_place_room_action(s,in);
  /* Quotes are cached, but this tentative move has never touched a live player. */
  s->na--;s->p[i].moved=0;s->t[dest].reserved-=price;
  if(!rm_unlink(s,i,dest))return 0;
  rm_link(s,i,t);
 }
 return 0;
}
static void rm_repair_specialist_rooms(RMState *s) {
 int entry[RM_TEAMS][2]={{0}};
 for(int i=0;i<s->n;i++) {
  RMPlayer *p=&s->p[i];
  if(p->original>=0 && rm_kicking_unit(p->pos))entry[p->original][p->pos-RM_K_POS]++;
 }
 /* A missing listed job goes first leaguewide. An initially thin room can
    then take one more specialist up to three, if it has an open spot. */
 for(int pass=0;pass<3&&!s->fault;pass++)for(int t=0;t<RM_TEAMS&&!s->fault;t++) {
  if(!rm_managed(s,t) || !s->t[t].config[RM_K_POS].minimum || !s->t[t].config[RM_P_POS].minimum)continue;
  int kickers=rm_count(s,t,RM_K_POS,0),punters=rm_count(s,t,RM_P_POS,0);
  int pos=pass==0?RM_P_POS:pass==1?RM_K_POS:-1;
  if(pos==RM_P_POS && punters)continue;
  if(pos==RM_K_POS && kickers)continue;
  if(pos<0 && (kickers+punters>=3 || (entry[t][0]&&entry[t][1])))continue;
  if(s->t[t].count<90) {
   RMAction in;if(rm_find_specialist(s,t,pos,&in))rm_place_room_action(s,in);
  } else if(s->t[t].count==90 && pos>=0)rm_make_specialist_room(s,t,pos);
 }
}
