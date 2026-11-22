"""Differential property test for the cache-stack framework and the league-scan caches
(source/r3/cache_stack.inc, source/r3/cache_scans.inc).

Compiles the production text of cache_stack.h, cache_stack.inc, cache_scans.inc, portal_published_owner_live
(source/r3/portal_integrity.inc) and hs15_secured (source/r3/hs15.inc), as a verification build (DRI_SPEED_TEST_MODES, so
the test can switch between the original code, shadow and fast at run time), against a modelled league: teams with
published-player arrays (33), the franchise HS and portal recruit lists (39, 46), duplicates across teams,
unreadable teams, arrays and entries, oversize arrays, and per-entry failures. No game or Windows API is needed.

  P1  Random windows. Inside each window: owner queries (portal_published_owner) and secured-list walks
      (hs15_secured, both lists, a failing entry now and then), with legitimate changes in between: a recruit
      stage write noted as DRI does (cache_note_stage_write) and published-array changes behind a sub-phase
      boundary (cache_phase_boundary). Between windows the league changes freely.
      fast: every answer, return value and visited-entry trace equals the original code's (CACHE_OFF).
      shadow: the original decides and 0 differences are reported.
  C1  Control: a published-array change inside a window with no boundary. The owner index can then be stale and
      shadow must report it.
  C2  Control: a stage change with no stage-write note. The secured list can then be stale and shadow must
      report it.
  OFF CACHE_OFF: no cache is allocated and nothing is counted.
  HOOK cache_hook_entry: which hook kinds advance the memo generation and end an offer-capacity run.

Usage: python3 tests/test_cache_stack.py [--cc gcc] [--out DIR] [--windows N]
"""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
R3 = ROOT / 'source' / 'r3'


def cut(text, start, end):
    i = text.index(start)
    return text[i:text.index(end, i)]


HARNESS = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uintptr_t U;typedef void *Rec;typedef long LONG;typedef unsigned long DWORD;typedef void *HANDLE;
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define HEAP_ZERO_MEMORY 8
static long allocations;
static void *GetProcessHeap(void){return 0;}
static void *HeapAlloc(void *h,int f,size_t n){(void)h;(void)f;allocations++;return calloc(1,n);}
static DWORD GetCurrentThreadId(void){return 7;}
#define DRI_SPEED_TEST_MODES 1 /* verification build: cache_stack_mode switches CACHE_OFF / CACHE_SHADOW / CACHE_FAST */
static volatile LONG mode=1,faulted=0;
static HANDLE log_file=(HANDLE)1;
static long mismatch_rows,rows;
static void log_line(const char *f,...){rows++;if(!strncmp(f,"CACHE_STACK_MISMATCH",20))mismatch_rows++;}
/* the model: every record has its table at +0x10 (q) and a row */
#define MAXN 2200
typedef struct Obj {U pad0,pad1;U table;uint32_t row;int id,stage;struct Obj *player;struct Arr *a33;} Obj;
typedef struct Arr {U pad0,pad1;U table;uint32_t row;int n,bad_at;Obj *items[MAXN];} Arr;
static U q(U a){return *(U *)a;}
static uint32_t row(Rec r){return ((Obj *)r)->row;}
static void release(Rec r){(void)r;}
static Obj franchise;static Arr teams,list39,list46;
static int bad_teams_field;
static Rec context_ref(U context,int n){assert(context==123&&n==6);return &franchise;}
static Rec reference(Rec r,int f){assert(f==5);return ((Obj *)r)->player;}
static Rec array_field(Rec r,int f){
 if(r==&franchise)return f==45?(bad_teams_field?0:(Rec)&teams):f==39?(Rec)&list39:f==46?(Rec)&list46:0;
 assert(f==33);return ((Obj *)r)->a33;
}
static U array_count(Rec a){return (U)((Arr *)a)->n;}
static Rec array_at(Rec a,U i){Arr *x=a;assert((int)i<x->n);return (int)i==x->bad_at?0:x->items[i];}
static unsigned scalar(Rec r,int f,int k){(void)k;Obj *o=r;return f==383?(unsigned)o->id:f==9?(unsigned)o->stage:0;}
static int portal_same_record(Rec a,Rec b){return a && b && q((U)a+0x10)==q((U)b+0x10) && row(a)==row(b);}
typedef struct {int dummy;} Hs15Snapshot;
static char visit[1<<16];static size_t visit_n;static uint32_t fail_row=0xffffffffu;
static int hs15_secured_entry(Hs15Snapshot *s,Rec r,unsigned stage,Rec team,unsigned tid,U eval,int portal){
 (void)s;(void)team;(void)eval;(void)portal;
 visit_n+=(size_t)snprintf(visit+visit_n,sizeof(visit)-visit_n,"%u:%u:%u ",tid,row(r),stage);
 if(visit_n>sizeof(visit)-64)visit_n=0;
 return row(r)!=fail_row;
}
static int t36_relation,t36_relation_team; /* referenced by the framework's memo users only */
#include "cache_stack.h"
'''

DRIVER = r'''
static uint64_t rng=0x9E3779B97F4A7C15ull;
static unsigned rnd(unsigned n){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return (unsigned)(rng%n);}
#define NT 60
#define NP 1600
#define NR 1800
static Obj team_obj[NT];static Arr pub[NT];static Obj players[NP];static Obj recruits[2][NR];
static void arr_init(Arr *a,U table,uint32_t r){memset(a,0,sizeof(*a));a->table=table;a->row=r;a->bad_at=-1;}
static void league(void){
 arr_init(&teams,0x7000,1);teams.n=30+rnd(NT-30);bad_teams_field=!rnd(200);
 for(int i=0;i<NP;i++){memset(&players[i],0,sizeof(players[i]));players[i].table=0x1000+(U)(i%3)*0x100;players[i].row=(uint32_t)(i/3);}
 for(int t=0;t<teams.n;t++){
  Obj *o=&team_obj[t];memset(o,0,sizeof(*o));o->table=0x2000;o->row=(uint32_t)t;o->id=rnd(300)?t:200;o->a33=&pub[t];
  arr_init(&pub[t],0x3000,(uint32_t)t);pub[t].n=rnd(13);if(!rnd(500))pub[t].n=193;
  for(int j=0;j<pub[t].n;j++)pub[t].items[j]=&players[rnd(NP)];
  if(pub[t].n && !rnd(150))pub[t].bad_at=rnd(pub[t].n);
  teams.items[t]=rnd(400)?o:0;
 }
 for(int k=0;k<2;k++){Arr *l=k?&list46:&list39;arr_init(l,0x4000+(U)k,(uint32_t)k);l->n=200+rnd(1600);
  for(int j=0;j<l->n;j++){Obj *r=&recruits[k][j];memset(r,0,sizeof(*r));r->table=0x5000+(U)k;r->row=(uint32_t)j;
   r->stage=rnd(4)?rnd(5):5+rnd(2);r->player=&players[rnd(NP)];l->items[j]=r;}
  if(!rnd(30))l->bad_at=rnd(l->n);
 }
}
static Obj *any_recruit(void){return &recruits[rnd(2)][rnd(list39.n<list46.n?list39.n:list46.n)];}
static long compares,diffs;
/* one owner query and one secured walk, compared with the original code (CACHE_OFF) */
static void query(int mode_now){
 Obj *r=any_recruit();int tid_c=-9,tid_l=-9;
 cache_stack_mode=mode_now;int c=portal_published_owner(r,123,&tid_c);
 int l=portal_published_owner_live(r,123,&tid_l);
 if(mode_now==CACHE_FAST){compares++;if(c!=l || (l==1 && tid_c!=tid_l))diffs++;}
 int k=rnd(2),t=rnd(teams.n);if(!teams.items[t])return;
 fail_row=rnd(20)?0xffffffffu:(uint32_t)rnd(k?list46.n:list39.n);
 Hs15Snapshot s;char a[sizeof(visit)];
 visit_n=0;cache_stack_mode=mode_now;int v1=hs15_secured(&s,&franchise,teams.items[t],123,k?46:39,k);memcpy(a,visit,visit_n);size_t na=visit_n;
 visit_n=0;cache_stack_mode=CACHE_OFF;int v2=hs15_secured(&s,&franchise,teams.items[t],123,k?46:39,k);
 cache_stack_mode=mode_now;
 compares++;if(v1!=v2 || na!=visit_n || memcmp(a,visit,na))diffs++;
 fail_row=0xffffffffu;
}
static void stage_write(void){Obj *r=any_recruit();r->stage=rnd(7);cache_note_stage_write();}
static void publish_change(int noted){int t=rnd(teams.n);if(pub[t].n<12)pub[t].items[pub[t].n++]=&players[rnd(NP)];else pub[t].n=0;if(noted)cache_phase_boundary();}
static void stage_change_unnoted(void){Obj *r=any_recruit();r->stage=r->stage>=5?0:5;}
static void run(int mode_now,int windows,int control){
 for(int w=0;w<windows;w++){
  league();cache_stack_mode=mode_now;
  int opened=cache_plan_window("TEST",123);assert(opened && cache.active);
  int nq=5+rnd(40);
  for(int i=0;i<nq;i++){
   query(mode_now);
   unsigned x=rnd(10);
   if(x==0)stage_write();
   else if(x==1)publish_change(1);
   else if(x==2 && control==1)publish_change(0);
   else if(x==2 && control==2)stage_change_unnoted();
  }
  cache_plan_window_end(opened);assert(!cache.active);
 }
}
int main(int argc,char **argv){
 int windows=argc>1?atoi(argv[1]):400;
 /* OFF: no allocation, no counting, the original code only */
 cache_stack_mode=CACHE_OFF;assert(!cache_plan_window("TEST",123));
 for(int i=0;i<200;i++){league();query(CACHE_OFF);}
 for(int k=0;k<CACHE_KINDS;k++)assert(!cache_counts[k].queries);
 assert(!allocations && !owner_index.slot && !secured_lists[0] && !secured_lists[1]);
 printf("OFF: 200 queries, no allocation, no counts\n");
 /* HOOK: generation and offer-run rules */
 cache_stack_mode=CACHE_FAST;
 {unsigned g=cache.gen,e=cache.offer_epoch;
  cache_hook_entry(CACHE_HOOK_GRADE);assert(cache.gen==g && cache.offer_epoch==e && !cache.team_pending);
  cache_hook_entry(CACHE_HOOK_OFFER);assert(cache.gen==g+1 && cache.offer_epoch==e);
  cache_hook_entry(CACHE_HOOK_TEAM_GRADE);assert(cache.gen==g+1 && cache.offer_epoch==e && cache.team_pending);
  cache_hook_entry(CACHE_HOOK_SCORE);assert(cache.gen==g+1 && cache.offer_epoch==e+2 && !cache.team_pending); /* ends the run, and the unresolved team hook too */
  cache_hook_entry(CACHE_HOOK_TEAM);assert(cache.gen==g+2 && cache.team_pending);
  cache_hook_entry(CACHE_HOOK_OTHER);assert(cache.gen==g+3 && cache.offer_epoch==e+4 && !cache.team_pending);
  cache_stack_mode=CACHE_OFF;cache_hook_entry(CACHE_HOOK_OTHER);assert(cache.gen==g+3);
  printf("HOOK: generation and offer-run rules hold for all kinds; off counts nothing\n");}
 /* P1 */
 compares=diffs=0;memset(cache_counts,0,sizeof(cache_counts));mismatch_rows=0;
 run(CACHE_FAST,windows,0);
 long fc=compares,fd=diffs;
 printf("P1 fast: %d windows, %ld compares with the original code, %ld differences\n",windows,fc,fd);
 compares=diffs=0;memset(cache_counts,0,sizeof(cache_counts));mismatch_rows=0;
 run(CACHE_SHADOW,windows,0);
 long sm=cache_counts[CACHE_OWNER_INDEX].mismatch+cache_counts[CACHE_SECURED_LIST].mismatch;
 long sc=cache_counts[CACHE_OWNER_INDEX].compares+cache_counts[CACHE_SECURED_LIST].compares;
 printf("P1 shadow: %ld cache compares, %ld mismatches, %ld differences from the original code\n",sc,sm,diffs);
 /* controls */
 memset(cache_counts,0,sizeof(cache_counts));run(CACHE_SHADOW,windows,1);
 long c1=cache_counts[CACHE_OWNER_INDEX].mismatch;
 memset(cache_counts,0,sizeof(cache_counts));run(CACHE_SHADOW,windows,2);
 long c2=cache_counts[CACHE_SECURED_LIST].mismatch;
 printf("C1 unnoted published change: %ld owner mismatches; C2 unnoted stage change: %ld secured-list mismatches\n",c1,c2);
 int ok=fd==0 && fc>1000 && sm==0 && diffs==0 && sc>1000 && c1>0 && c2>0;
 printf("%s: cache stack framework and league-scan caches\n",ok?"PASS":"FAIL");
 return ok?0:1;
}
'''


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--cc', default='gcc')
    ap.add_argument('--out', default=str(ROOT / 'build' / 'cache_stack_test'))
    ap.add_argument('--windows', type=int, default=400)
    a = ap.parse_args()
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    integrity = (R3 / 'portal_integrity.inc').read_text()
    hs15 = (R3 / 'hs15.inc').read_text()
    code = (HARNESS + (R3 / 'cache_stack.inc').read_text() + (R3 / 'cache_scans.inc').read_text() +
            cut(integrity, 'static int portal_published_owner_live(', '/* Keep the recruit key occupied') +
            cut(hs15, 'static int hs15_secured(', 'static int hs15_collect_live(') + DRIVER)
    (out / 'test_cache_stack.c').write_text(code)
    exe = out / 'test_cache_stack'
    subprocess.run([a.cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Wno-unused-function', '-Wno-unused-parameter',
                    '-Wno-unused-variable', '-Wno-misleading-indentation', '-I' + str(R3), str(out / 'test_cache_stack.c'), '-o', str(exe)], check=True)
    r = subprocess.run([str(exe), str(a.windows)], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    print(r.stdout.rstrip())
    raise SystemExit(r.returncode)


if __name__ == '__main__':
    main()
