"""Differential property test for the roster pass ownership index (source/roster/roster_owner_index.inc).

Compiles the production text of rm_owned (unchanged, from source/roster/native_actions.inc) and the
whole of roster_owner_index.inc against a modelled league: 138 teams with roster (239) and arrivals
(33) arrays, empty slots, oversize arrays, unreadable arrays and entries, unreadable identities and
duplicates. No game or Windows API is needed.

  P1  Random transaction passes whose actions change only their own source and destination arrays
      (moves, cuts, and anomalies placed in those two teams: duplicates, copies in arrivals,
      removals, oversize arrays, unreadable arrays or entries), each followed by
      rm_owner_index_touch(src/dst) as rm_run_plan does. Every query (moved players, untouched
      players, unreadable identities, the expected team or no owner) is answered by
      rm_owned_before (the index) and by rm_owned (the original full-league check) on the same
      league state: 0 differences and 0 fallbacks.
  P2  Control: actions that also change an unmarked third team (a copy of a source-team player
      added to its roster, a member removed, an array made unreadable). The index can then be
      stale and the comparison must report it (differences > 0).
  OUT Any query outside rm_owner_index_begin/end uses rm_owned only: no index is allocated and
      nothing is counted.
  Mutants: four deliberately broken copies of the index (dirty marking off, team/slot check
      dropped, unreadable-array rule dropped, stale entries kept on a rescan) must each be caught
      by the comparison on P1 passes.

Usage: python3 tests/test_roster_owner_index.py [--cc gcc] [--out DIR] [--passes N]
"""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'source'

HARNESS = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uintptr_t U;typedef void *Rec;
#include "roster/rm_policy.h"
static Rec rm_teams[RM_TEAMS];
typedef struct {U store;uint32_t row;int unreadable;} PObj;
typedef struct {int team;} TeamObj;
typedef struct {int t,slot;} ArrObj;
#define MAXL 230
static TeamObj team_objs[RM_TEAMS];static ArrObj arrs[RM_TEAMS][2];
static PObj *lists[RM_TEAMS][2][MAXL];static int lens[RM_TEAMS][2];
static int bad_field[RM_TEAMS][2],bad_entry[RM_TEAMS][2];
static int identity(Rec r,U *s,uint32_t *w){if(!r)return 0;const PObj *o=r;if(o->unreadable)return 0;*s=o->store;*w=o->row;return 1;}
static int pilot_same(Rec a,Rec b){U sa,sb;uint32_t ra,rb;return identity(a,&sa,&ra)&&identity(b,&sb,&rb)&&sa==sb&&ra==rb;}
static Rec array_field(Rec team,unsigned field,int *ok){const TeamObj *o=team;assert(field==33||field==239);int slot=field==33;*ok=!bad_field[o->team][slot];return &arrs[o->team][slot];}
static U array_count(Rec a){const ArrObj *x=a;return (U)lens[x->t][x->slot];}
static Rec array_at(Rec a,U i,int *ok){const ArrObj *x=a;assert((int)i<lens[x->t][x->slot]);*ok=!(bad_entry[x->t][x->slot]&&(int)i==lens[x->t][x->slot]-1);return lists[x->t][x->slot][i];}
static void release(Rec r){(void)r;}
static long logged;
static void log_line(const char *f,...){(void)f;logged++;}
'''

DRIVER = r'''
static uint64_t rng=88172645463325252ULL;
static unsigned rnd(unsigned n){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)(rng%n);}
#define NP 16000
static PObj players[NP];static int nextp;
static PObj *third_dup;static int third_dup_team=-1;
static void add(int t,int s,PObj *p){if(lens[t][s]<MAXL)lists[t][s][lens[t][s]++]=p;}
static int find(int t,int s,PObj *p){for(int i=0;i<lens[t][s];i++)if(lists[t][s][i]==p)return i;return -1;}
static void del(int t,int s,int i){for(int k=i;k+1<lens[t][s];k++)lists[t][s][k]=lists[t][s][k+1];lens[t][s]--;}
static void league(void){
 memset(lens,0,sizeof(lens));memset(bad_field,0,sizeof(bad_field));memset(bad_entry,0,sizeof(bad_entry));nextp=0;
 for(int i=0;i<NP;i++){players[i].store=0x1000+(U)(i%7)*0x100;players[i].row=(uint32_t)(i/7);players[i].unreadable=!rnd(400);}
 for(int t=0;t<RM_TEAMS;t++){int n=60+rnd(30);for(int i=0;i<n;i++)add(t,0,rnd(40)?&players[nextp++]:0);int m=rnd(4);for(int i=0;i<m;i++)add(t,1,&players[nextp++]);}
 /* pre-existing anomalies anywhere (the original check fails on the affected queries; the index must agree) */
 if(!rnd(4)){int t=rnd(RM_TEAMS);switch(rnd(4)){case 0:add(t,0,&players[rnd(nextp)]);break;case 1:add(t,1,&players[rnd(nextp)]);break;
  case 2:bad_field[t][rnd(2)]=1;break;case 3:{int s=rnd(2);if(lens[t][s])bad_entry[t][s]=1;}break;}}
}
static PObj *member(int t){if(!lens[t][0])return 0;return lists[t][0][rnd(lens[t][0])];}
/* one action: changes only teams s and d (d<0: cut); third>=0 additionally changes that team (not marked) */
static void act(int s,int d,int third){
 PObj *p=member(s);
 if(p){int i=find(s,0,p);if(i>=0)del(s,0,i);if(d>=0)add(d,0,p);}
 if(!rnd(6)){int t=rnd(2)?s:(d>=0?d:s);switch(rnd(6)){
  case 0:add(t,0,&players[rnd(nextp)]);break;
  case 1:add(t,1,&players[rnd(nextp)]);break;
  case 2:if(lens[t][0])del(t,0,rnd(lens[t][0]));break;
  case 3:while(lens[t][0]<RM_ROOM+1+(int)rnd(3))add(t,0,0);break;
  case 4:bad_field[t][rnd(2)]=!bad_field[t][0];break;
  case 5:{int sl=rnd(2);if(lens[t][sl])bad_entry[t][sl]=!bad_entry[t][sl];}break;}}
 if(third>=0){switch(rnd(3)){case 0:{PObj *q=member(third);if(q)del(third,0,find(third,0,q));}break;
  case 1:if(lens[s][0]){third_dup=lists[s][0][rnd(lens[s][0])];third_dup_team=s;add(third,0,third_dup);}break;
  case 2:bad_field[third][rnd(2)]=1;break;}}
}
static long queries,diffs;
/* the index verdict (rm_owned_before) against the original check on the same league state */
static void check(PObj *p,int e){
 int v=rm_owned_before(p,e);int live=rm_owned(p,e);
 if(v!=live)diffs++;queries++;
}
static void query_some(int k,int ta,int tb){
 for(int z=0;z<k;z++){
  int t=rnd(3)?(rnd(2)?ta:tb):(int)rnd(RM_TEAMS);if(t<0)t=rnd(RM_TEAMS);
  PObj *p=rnd(8)?member(t):&players[rnd(NP)];if(!p)p=&players[rnd(NP)];
  int e=rnd(5)?t:(rnd(2)?-1:(int)rnd(RM_TEAMS));
  check(p,e);
 }
}
/* one transaction pass; third=1: some actions also change an unmarked third team */
static void pass(int third_mode){
 league();rm_owner_index_begin();
 int na=5+rnd(40);
 for(int j=0;j<na;j++){
  int s=rnd(RM_TEAMS),d=rnd(5)?(int)rnd(RM_TEAMS):-1;if(d==s)d=(d+1)%RM_TEAMS;
  query_some(1+rnd(4),s,d);
  int third=-1;if(third_mode&&!rnd(3)){third=rnd(RM_TEAMS);if(third==s||third==d)third=-1;}
  third_dup=0;act(s,d,third);rm_owner_index_touch(s);rm_owner_index_touch(d);
  if(third_dup)check(third_dup,third_dup_team);
 }
 query_some(4,0,1);
 rm_owner_index_end();
}
int main(int argc,char **argv){
 int n=argc>1?atoi(argv[1]):600,only_p1=argc>2&&!strcmp(argv[2],"p1");
 for(int t=0;t<RM_TEAMS;t++){team_objs[t].team=t;rm_teams[t]=&team_objs[t];arrs[t][0]=(ArrObj){t,0};arrs[t][1]=(ArrObj){t,1};}
 long q1,d1,f1=0,q2=0,d2=0,qo=0,oo=0;
 /* P1: the index must equal rm_owned on every query */
 queries=diffs=0;
 for(int k=0;k<n;k++){pass(0);f1+=rm_oi.fallback;}
 q1=queries;d1=diffs;
 if(only_p1){printf("{\"p1_queries\":%ld,\"p1_differences\":%ld}\n",q1,d1);return d1>0?3:0;}
 /* P2 control: an unmarked third-team change must be reported */
 queries=diffs=0;
 for(int k=0;k<n/3+1;k++)pass(1);
 q2=queries;d2=diffs;
 /* outside a pass: rm_owned only, nothing allocated or counted */
 queries=diffs=0;long counted=rm_oi.queries;
 league();for(int k=0;k<200;k++){PObj *p=member(rnd(RM_TEAMS));if(!p)continue;check(p,rnd(RM_TEAMS));if(rm_oi.map||rm_oi.arr||rm_oi.queries!=counted)oo++;}
 qo=queries;oo+=diffs;
 int ok=d1==0&&f1==0&&d2>0&&oo==0;
 printf("{\"p1_passes\":%d,\"p1_queries\":%ld,\"p1_differences\":%ld,\"p1_fallback\":%ld,"
        "\"p2_third_team_queries\":%ld,\"p2_differences\":%ld,\"outside_pass_queries\":%ld,\"outside_pass_violations\":%ld,\"pass\":%d}\n",
        n,q1,d1,f1,q2,d2,qo,oo,ok);
 return ok?0:1;
}
'''

MUTANTS = {
    'dirty_marking_off': ('static void rm_owner_index_touch(int t){if(rm_oi.active&&t>=0&&t<RM_TEAMS)rm_oi.dirty[t]=1;}',
                          'static void rm_owner_index_touch(int t){(void)t;}'),
    'team_and_slot_ignored': ('return k==1&&e->code_sum==expected*2;', 'return k==1;'),
    'unreadable_arrays_ignored': (' if(rm_oi.bad)return 0;\n', '\n'),
    'rescan_keeps_old_entries': ('if(x->state==1)for(int i=0;i<x->n;i++){RMOwnerEntry *e=rm_oi_slot(x->store[i],x->row[i],0);if(!e)return 0;e->count--;e->code_sum-=code;}',
                                 ''),
}


def build(cc, out, name, index_text, owned):
    c = out / (name + '.c')
    exe = out / name
    c.write_text(HARNESS + owned + index_text + DRIVER)
    subprocess.run([cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function',
                    '-Wno-misleading-indentation', '-I' + str(SRC), str(c), '-o', str(exe)], check=True)
    return exe


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--cc', default='gcc')
    ap.add_argument('--out', default=str(ROOT / 'build' / 'portable-checks' / 'roster_owner_index'))
    ap.add_argument('--passes', type=int, default=600)
    a = ap.parse_args()
    out = Path(a.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    actions = (SRC / 'roster' / 'native_actions.inc').read_text()
    index_text = (SRC / 'roster' / 'roster_owner_index.inc').read_text()
    assert '#include "roster_owner_index.inc"' in actions
    i = actions.index('static int rm_owned(Rec p,int expected)')
    owned = actions[i:actions.index('\n', i) + 1]
    exe = build(a.cc, out, 'roster_owner_index', index_text, owned)
    p = subprocess.run([str(exe), str(a.passes)], capture_output=True, text=True)
    result = json.loads(p.stdout)
    result['mutants'] = {}
    for name, (old, new) in MUTANTS.items():
        assert index_text.count(old) == 1, name
        m = build(a.cc, out, 'mutant_' + name, index_text.replace(old, new), owned)
        q = subprocess.run([str(m), str(max(60, a.passes // 5)), 'p1'], capture_output=True, text=True)
        result['mutants'][name] = {'caught': q.returncode == 3, **json.loads(q.stdout)}
    caught = all(v['caught'] for v in result['mutants'].values())
    result['passed'] = p.returncode == 0 and bool(result['pass']) and caught
    (out / 'roster_owner_index_results.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))
    if not result['passed']:
        raise SystemExit('roster owner index property test FAILED')
    print('PASS: roster owner index matches rm_owned; third-team control and %d mutants caught.' % len(MUTANTS))


if __name__ == '__main__':
    main()
