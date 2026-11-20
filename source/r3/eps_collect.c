/* Read-only native record traversal through an injectable, ownership-aware API.
   Ordinals are verified against the supplied October 13 native field descriptors. */
#include "eps_collect.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define EPS_GAME_SLOTS 8192
typedef struct {EpsRec ref;int index;} EpsGame;
typedef struct {const EpsAPI *a;EpsRec teams[EPS_MAX_TEAMS];int team_count,count,year;EpsGame games[EPS_GAME_SLOTS];EpsRow *rows;const char *error;} EpsReader;
int eps_row_valid(const EpsRow *x){
 /* Validate each source independently. Team/individual equality is not a
    validity condition: native team totals drive team metrics, recorded
    individual production drives position metrics. Never redistribute a gap.
    Preserve the pre-existing finite/integer and nonnegative-count guards. */
 const double *g=x->group[EPS_G_ALL];
 double v[]={x->pass_yards,x->pass_attempts,x->rush_yards,x->rush_attempts,x->sacks*2,
             g[EPS_S_PASSYARDS],g[EPS_S_PASSATTEMPTS],g[EPS_S_RUSHYARDS],g[EPS_S_RUSHATTEMPTS],
             g[EPS_S_DLINESACKS],g[EPS_S_DLINEHALFSACK]};
 for(unsigned i=0;i<sizeof(v)/sizeof(v[0]);i++)if(!isfinite(v[i])||v[i]!=floor(v[i]))return 0;
 if(x->pass_attempts<0||x->rush_attempts<0||x->sacks<0||
    g[EPS_S_PASSATTEMPTS]<0||g[EPS_S_RUSHATTEMPTS]<0||g[EPS_S_DLINESACKS]<0||g[EPS_S_DLINEHALFSACK]<0)return 0;
 return 1;
}
int eps_reconcile_row(const EpsRow *x){
 /* Diagnostic classification only. Retain the historical small-residual
    category for comparison with older logs; neither size nor sign of a
    difference rejects an otherwise valid row. */
 if(!eps_row_valid(x))return 0;
 const double *g=x->group[EPS_G_ALL];
 if(g[EPS_S_PASSYARDS]!=x->pass_yards||g[EPS_S_PASSATTEMPTS]!=x->pass_attempts)return 0;
 double attempts=x->rush_attempts-g[EPS_S_RUSHATTEMPTS],yards=x->rush_yards-g[EPS_S_RUSHYARDS];
 double sacks=x->sacks-g[EPS_S_DLINESACKS]-.5*g[EPS_S_DLINEHALFSACK];
 if(attempts<0||attempts>2||fabs(yards)>5*attempts||sacks<0||sacks>1)return 0;
 return attempts==0&&yards==0&&sacks==0?1:2;
}
static int eps_team(EpsReader *r,EpsRec team){
 const EpsAPI *a=r->a;int tid=a->value(team,383,0x100);
 if(tid>=0&&tid<EPS_TEAMS&&a->same(team,r->teams[tid]))return tid;
 for(int i=EPS_TEAMS;i<r->team_count;i++)if(a->same(team,r->teams[i]))return i;
 return -1;
}
static int eps_group(int pos){
 switch(pos){case 0:return EPS_G_QB;case 1:return EPS_G_HB;case 3:return EPS_G_WR;case 4:return EPS_G_TE;case 10:case 11:return EPS_G_DE;case 12:return EPS_G_DT;case 13:case 15:return EPS_G_OLB;case 14:return EPS_G_MLB;default:return -1;}
}
static int eps_stats(const EpsAPI *a,EpsRec stats,int pos,EpsRow *row){
 if(pos<0||pos>21)return 0;
 static const int off_s[]={EPS_S_RUSHYARDS,EPS_S_RUSHATTEMPTS,EPS_S_PASSYARDS,EPS_S_PASSATTEMPTS,EPS_S_RECEIVEYARDS,EPS_S_RECEIVECATCHES,EPS_S_RECEIVEYARDSAFTER};
 static const int off_f[]={24,19,11,5,16,12,17};
 static const int def_s[]={EPS_S_DLINESACKS,EPS_S_DLINEHALFSACK,EPS_S_DEFTACKLESFORLOSS,EPS_S_DEFTACKLES,EPS_S_ASSDEFTACKLES},def_f[]={17,16,10,9,5};
 static const int kick_s[]={EPS_S_KICKFGATTEMPTS,EPS_S_KICKFGMADE,EPS_S_KICKFGATTEMPTS29ORLESS,EPS_S_KICKFGMADE29ORLESS,EPS_S_KICKFGATTEMPTS30TO39,EPS_S_KICKFGMADE30TO39,EPS_S_KICKFGATTEMPTS40TO49,EPS_S_KICKFGMADE40TO49,EPS_S_KICKFGATTEMPTS50ORMORE,EPS_S_KICKFGMADE50ORMORE,EPS_S_KICKNUMKICKOFFS,EPS_S_KICKTOUCHBACKS,EPS_S_PUNTATTEMPTS,EPS_S_PUNTYARDS,EPS_S_PUNTIN20,EPS_S_PUNTTOUCHBACKS};
 static const int kick_f[]={8,15,9,16,10,17,11,18,12,19,20,21,22,28,24,27};
 const int *ss=0,*ff=0;int n=0,g=eps_group(pos);
 /* Decode the historical record's actual schema, never the player's current
    position. An offseason position change must not reinterpret packed fields. */
 int fields=a->stat_fields(stats);
 if(fields==26||fields==34){ss=off_s;ff=off_f;n=7;}
 else if(fields==23||fields==31){ss=def_s;ff=def_f;n=5;}
 else if(fields==29){ss=kick_s;ff=kick_f;n=16;}
 else if(fields!=7)return 0;
 for(int i=0;i<n;i++){
  double v=a->value(stats,ff[i],0x100);if(fabs(v)>100000)return 0;
  row->group[EPS_G_ALL][ss[i]]+=v;if(g>=0)row->group[g][ss[i]]+=v;if(pos==1||pos==2)row->group[EPS_G_RB][ss[i]]+=v;
 }return 1;
}
static int eps_game(EpsReader *r,EpsRec game){
 const EpsAPI *a=r->a;uint64_t hash=a->key(game);unsigned slot=(unsigned)(hash^(hash>>32))&(EPS_GAME_SLOTS-1);
 for(int scan=0;scan<EPS_GAME_SLOTS;scan++,slot=(slot+1)&(EPS_GAME_SLOTS-1)){
  EpsGame *g=&r->games[slot];if(g->ref){if(a->same(g->ref,game))return g->index;continue;}
  if(r->count+2>EPS_MAX_ROWS){r->error="game_capacity";return -1;}
  EpsRec home=a->ref(game,42),away=a->ref(game,18),hs=a->ref(game,43),as=a->ref(game,19);
  int h=home?eps_team(r,home):-1,w=away?eps_team(r,away):-1;int good=home&&away&&hs&&as&&h>=0&&w>=0&&h!=w;
  if(good){
   int at=r->count;for(int side=0;side<2;side++){
    EpsRow *x=&r->rows[at+side];EpsRec st=side?as:hs;x->game=(int)(hash&0x7fffffff);x->team=side?w:h;x->opponent=side?h:w;
    x->pass_yards=a->value(st,27,0x100);x->pass_attempts=a->value(st,33,0x100);x->rush_yards=a->value(st,31,0x100);x->rush_attempts=a->value(st,45,0x100);x->sacks=a->value(st,47,0x100);
    if(x->pass_attempts<0||x->rush_attempts<0||x->sacks<0||x->pass_attempts>300||x->rush_attempts>300)good=0;
   }
   if(good){a->keep(game);g->ref=game;g->index=at;r->count+=2;}
  }
  a->drop(home);a->drop(away);a->drop(hs);a->drop(as);
  if(!good){r->error="game_team_or_cache";return -1;}return g->index;
 }r->error="game_hash_capacity";return -1;
}
EpsReadResult eps_collect_through(const EpsAPI *a,EpsRec franchise,int year_index,EpsRow *rows,int cutoff){
 EpsReadResult result={0};EpsReader *r=calloc(1,sizeof(*r));if(!r){result.error="reader_allocation";return result;}
 r->a=a;r->rows=rows;r->year=year_index;memset(rows,0,sizeof(*rows)*EPS_MAX_ROWS);
 EpsRec teams=a->array(franchise,45),fcs=a->array(franchise,11);unsigned count=teams?a->count(teams):0,nfcs=fcs?a->count(fcs):0;
 if(count!=EPS_TEAMS||nfcs>EPS_MAX_TEAMS-EPS_TEAMS){r->error="school_count";goto done;}
 for(unsigned i=0;i<count;i++){EpsRec t=a->at(teams,i);int tid=t?a->value(t,383,0x100):-1;if(tid<0||tid>=EPS_TEAMS||r->teams[tid]){a->drop(t);r->error="school_identity";goto done;}r->teams[tid]=t;}
 r->team_count=EPS_TEAMS;
 for(unsigned i=0;i<nfcs;i++){EpsRec t=a->at(fcs,i);if(!t){r->error="fcs_identity";goto done;}for(int j=0;j<r->team_count;j++)if(a->same(t,r->teams[j])){a->drop(t);r->error="duplicate_fcs";goto done;}r->teams[r->team_count++]=t;}
 for(int tid=0;tid<EPS_TEAMS;tid++){
  EpsRec roster=a->array(r->teams[tid],239);if(!roster){r->error="roster_missing";goto done;}
  unsigned players=a->count(roster);if(players>192){a->drop(roster);r->error="roster_capacity";goto done;}
  for(unsigned pi=0;pi<players;pi++){
   EpsRec player=a->at(roster,pi);if(!player){r->error="roster_player";break;}result.players++;
   int pos=a->value(player,3,0x108);EpsRec history=a->array(player,146);unsigned games=history?a->count(history):0;
   if(pos<0||pos>21||games>64){r->error="player_position_or_games";a->drop(history);a->drop(player);break;}
   for(unsigned si=0;si<games;si++){
    EpsRec stats=a->at(history,si);if(!stats)continue;EpsRec game=a->ref(stats,4);
    if(!game){a->drop(stats);r->error="player_game_link";break;}
    int use=a->value(game,63,0x100)==year_index&&!a->value(game,47,0xf8)&&!a->value(game,51,0xf8);
    if(use && cutoff>=0){
     /* Verified SeasonGame descriptors: SeasonWeek61, SeasonWeekType62.
        Challenge/practice exclusion above happens before reading statistics. */
     int week=a->value(game,61,0x100),type=a->value(game,62,0x108);
     if(week<0 || week>60 || type<0 || type>6){a->drop(game);a->drop(stats);r->error="game_calendar";break;}
     use=type==1 && week<cutoff;
    }
    if(use){
     int gi=eps_game(r,game);
     if(gi>=0){
      /* OpposingTeam records ownership at the time of the game. Current
         rosters change during the portal; they are not historical ownership. */
      EpsRec opponent=a->ref(stats,3);int opp=opponent?eps_team(r,opponent):-1;
      a->drop(opponent);int side=rows[gi].opponent==opp?0:rows[gi+1].opponent==opp?1:-1;
      if(side<0)r->error="player_game_opponent";
      else if(!eps_stats(a,stats,pos,rows+gi+side))r->error="player_stat_range";
      else result.player_games++;
     }
    }
    a->drop(game);a->drop(stats);if(r->error)break;
   }
   a->drop(history);a->drop(player);if(r->error)break;
  }
  a->drop(roster);if(r->error)goto done;
 }
 for(int i=0;i<EPS_GAME_SLOTS;i++)if(r->games[i].ref){
  EpsGame *g=&r->games[i];
  for(int side=0;side<2;side++){
   EpsRow *row=&rows[g->index+side];if(row->team<EPS_TEAMS)continue;result.fcs_games++;
   EpsRec cache=a->array(g->ref,side?10:34);if(!cache){r->error="fcs_stats_missing";goto done;}
   unsigned n=a->count(cache);if(n>192){a->drop(cache);r->error="fcs_stats_capacity";goto done;}
   for(unsigned j=0;j<n;j++){
    EpsRec item=a->at(cache,j),player=item?a->ref(item,0):0,stats=item?a->ref(item,1):0;
    if(!item||!player||!stats||!eps_stats(a,stats,a->value(player,3,0x108),row))r->error="fcs_stat_record";
    a->drop(stats);a->drop(player);a->drop(item);if(r->error)break;
   }
   a->drop(cache);if(r->error)goto done;
  }
 }
 /* Order-independent signature of completed games and team caches. It stays
    stable when players change schools, but rejects a different season branch.
    Native key() is the stable row id, not a process address. */
 result.rows=r->count;result.teams=r->team_count;
 result.schedule_hash=UINT64_C(1469598103934665603)^(uint64_t)r->count;
 for(int i=0;i<r->count;i++){
  EpsRow *x=&rows[i];uint64_t h=UINT64_C(1469598103934665603);
  int64_t values[]={x->game,x->team,x->opponent,(int64_t)x->pass_yards,(int64_t)x->pass_attempts,(int64_t)x->rush_yards,(int64_t)x->rush_attempts,(int64_t)(x->sacks*2)};
  for(unsigned j=0;j<sizeof(values)/sizeof(values[0]);j++){h^=(uint64_t)values[j];h*=UINT64_C(1099511628211);}
  result.schedule_hash+=h;
 }
 for(int i=0;i<r->count;i++){
  if(!eps_row_valid(&rows[i])){r->error="team_game_values";goto done;}result.reconciliations+=5;
 }
 result.success=1;result.rows=r->count;result.teams=r->team_count;
done:
 result.error=r->error;for(int i=0;i<EPS_GAME_SLOTS;i++)if(r->games[i].ref)a->drop(r->games[i].ref);
 for(int i=0;i<EPS_MAX_TEAMS;i++){a->drop(r->teams[i]);}
 a->drop(teams);a->drop(fcs);free(r);return result;
}

EpsReadResult eps_collect(const EpsAPI *a,EpsRec f,int year,EpsRow *rows){
 return eps_collect_through(a,f,year,rows,-1);
}
