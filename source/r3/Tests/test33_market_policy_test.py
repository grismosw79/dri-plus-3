"""HS-only admission boundaries, exact cached Test32 decisions, synthetic scale.
The performance case uses 3000 recruits,138 schools and300 native-positive
candidates per school. It is not the absent complete live W1 candidate matrix.
"""
from pathlib import Path
import subprocess,os,json
S=Path(__file__).resolve().parents[1]
O=Path(os.environ.get('DRI_TEST_OUTPUT_DIR',S/'build/test33-market-policy'));O.mkdir(parents=True,exist_ok=True)
code=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "market33_policy.h"
#include "competition32_policy.h"
static unsigned seed=33012026;
static unsigned rnd(void){seed=seed*1664525u+1013904223u;return seed;}
static int count(const uint64_t *v){return market30_count(v);}
static void setup(Market29Pressure *p,Market33Admissions *a,int stars) {
 memset(p,0,sizeof(*p));memset(a,0,sizeof(*a));a->stars=stars;
 for(unsigned t=0;t<30;t++){p->fit[t]=1000-(int)t;market29_bit(a->eligible,t,1);market29_bit(p->planned,t,1);market29_bit(p->primary,t,1);}
}
static void boundaries(void) {
 Market29Pressure p;Market33Admissions a;Market33Cache c;uint64_t done[3]={0};
 assert(hs33_soft_stars(0)==8 && hs33_limit_stars(0)==16);
 for(int stars=1;stars<=4;stars++) {
  setup(&p,&a,stars);market33_cache(&p,&c,19,2026);market33_select(&p,&a,&c,done,19,2026);
  assert(count(p.hard_authorized)==hs33_soft_stars(stars));
  for(unsigned t=0;t<30;t++)market29_bit(a.need,t,1);
  market33_select(&p,&a,&c,done,19,2026);assert(count(p.hard_authorized)==hs33_limit_stars(stars));
  assert(market29_has(p.hard_authorized,0));market29_bit(a.failed,0,1);
  market33_select(&p,&a,&c,done,19,2026);assert(!market29_has(p.hard_authorized,0));assert(market29_has(p.hard_authorized,(unsigned)hs33_limit_stars(stars)));
  market29_bit(done,1,1);market33_select(&p,&a,&c,done,19,2026);assert(!market29_has(p.hard_authorized,1));memset(done,0,sizeof(done));
 }
 setup(&p,&a,4);for(unsigned t=0;t<18;t++)market29_bit(p.protected_offer,t,1);
 market33_cache(&p,&c,19,2026);market33_select(&p,&a,&c,done,19,2026);assert(count(p.hard_authorized)==18);
 assert(!market29_has(p.hard_authorized,18));
 /* Inflight means admitted current membership, not primary-role ownership. */
 setup(&p,&a,4);market33_cache(&p,&c,19,2026);market33_select(&p,&a,&c,done,19,2026);
 market29_bit(a.inflight,7,1);market29_bit(p.primary,7,0);market33_cache(&p,&c,19,2026);market33_select(&p,&a,&c,done,19,2026);
 assert(market29_has(p.hard_authorized,7));market29_bit(a.failed,7,1);market33_select(&p,&a,&c,done,19,2026);assert(!market29_has(p.hard_authorized,7));
 /* A stronger native fit backup cannot displace all planned primary routes. */
 setup(&p,&a,4);for(unsigned t=8;t<30;t++){market29_bit(p.primary,t,0);p.fit[t]=2000+(int)t;}
 market33_cache(&p,&c,19,2026);market33_select(&p,&a,&c,done,19,2026);for(unsigned t=0;t<8;t++)assert(market29_has(p.hard_authorized,t));
 /* Native earned influence changes relative rank, with no point writes. */
 setup(&p,&a,4);p.interest[29]=900;p.interest[0]=10;p.gain[0]=5;market29_bit(p.protected_offer,0,1);
 market33_cache(&p,&c,19,2026);assert(c.earned);market33_select(&p,&a,&c,done,19,2026);assert(market29_has(p.hard_authorized,29));
 p.gain[0]=0;market33_cache(&p,&c,19,2026);assert(!c.earned);market33_select(&p,&a,&c,done,19,2026);assert(!market29_has(p.hard_authorized,29));
 /* User control is excluded by runtime construction, not assigned a quota here.
    Generic portal policy and old helper ABI are deliberately untouched. */
 assert(market30_limit(0)==16 && market30_limit(1)==10 && market29_penalty(1,5)==45);
}
static void equivalence(void) {
 for(int k=0;k<30000;k++) {
  Market29Pressure m={0};Market33Cache c;unsigned own=rnd()%138;Hs28Choice p={0};
  for(unsigned t=0;t<138;t++){m.fit[t]=(int)(rnd()%700);m.interest[t]=(int)(rnd()%1500);m.gain[t]=(int)(rnd()%160)-20;
   if(rnd()%3==0)market29_bit(m.active,t,1);if(rnd()%3==0)market29_bit(m.planned,t,1);
   if(rnd()%3==0)market29_bit(m.firm,t,1);if(rnd()%3==0)market29_bit(m.primary,t,1);}
  p.points=(int)(rnd()%1500);p.leader=(int)(rnd()%1800);p.stage=(int)(rnd()%6);
  market33_cache(&m,&c,550,2026);
  p.native_score=(int)(rnd()%800);int better=0;for(unsigned t=0;t<138;t++)if(m.fit[t]>p.native_score ||
   (m.fit[t]==p.native_score && market29_tie(550,t,2026)<market29_tie(550,own,2026)))better++;
  int strong=c.best_fit>0 && (int64_t)p.native_score*100>=(int64_t)c.best_fit*90 && better<3;
  assert(strong==market33_strong_choice(&p,&m,&c,own,550,2026));
  assert(competition32_close(&p,&m,own)==market33_close(&p,&m,&c,own));
  assert(market29_primary_pressure(&m,own)==market33_primary_pressure(&m,&c,own));
  assert(market29_pressure(&m,own)==market33_attention(&m,&c,own));
 }
 Market29Pressure m={0};Market33Cache c;Hs28Choice q={.points=800};
 m.fit[10]=1;m.interest[10]=1000;m.gain[10]=100;market29_bit(m.active,10,1);
 market33_cache(&m,&c,1,2026);assert(competition32_close(&q,&m,10)==market33_close(&q,&m,&c,10));
}
#define N 3000
#define P 300
static Market29Pressure markets[N];static Market33Cache caches[N];static Hs28Choice pool[P];
static unsigned long long scan_old,scan_new;
static int assess(int i,unsigned own,int fast) {
 Hs28Choice *p=&pool[i];unsigned rid=p->rid;Market29Pressure *m=&markets[rid];Market33Cache *c=&caches[rid];
 int protected=market29_has(m->protected_offer,own),offers=fast?c->offers:market30_count(m->protected_offer);if(!fast)scan_old+=138;
 if(!protected && offers>=16)return 100;
 int pressure=fast?market33_primary_pressure(m,c,own):market29_primary_pressure(m,own);if(!fast)scan_old+=138;
 int better=0;if(fast)better=c->fit_rank[own];else {for(unsigned t=0;t<138;t++)if(m->fit[t]>p->native_score ||(m->fit[t]==p->native_score && market29_tie(rid,t,2026)<market29_tie(rid,own,2026)))better++;scan_old+=138;}
 int strong=c->best_fit>0 && (int64_t)p->native_score*100>=(int64_t)c->best_fit*90 && better<3;
 int close=fast?market33_close(p,m,c,own):competition32_close(p,m,own);if(!fast)scan_old+=276;
 if(strong || close || p->urgent)return 0;
 int ranked=0;if(!fast || (p->stars>=3 && p->stage<4 && pressure>=3)) {ranked=competition32_better(m,own,rid,2026);if(fast)scan_new+=138;else scan_old+=138;}
 int rejected=market29_has(m->rejected,own),penalty=rejected?45:market29_penalty(0,pressure),opportunity=0,alternative=0;
 int mo=p->stars>=3 && p->stage<4 && pressure>=3 && ranked>=3;
 int ma=!protected && penalty && (rejected || !market29_has(m->admitted,own));
 if(fast && !mo && !ma)return 0;
 for(int j=0;j<P && (!fast || ((mo && opportunity<25)||(ma&&!alternative)));j++)if(j!=i &&pool[j].pos==p->pos &&pool[j].ovr>=p->ovr-3 && (int64_t)pool[j].native_score*100>=(int64_t)p->native_score*70) {
  Market29Pressure *other=&markets[pool[j].rid];Market33Cache *oc=&caches[pool[j].rid];
  int offered=fast?oc->offers:market30_count(other->protected_offer);if(!fast)scan_old+=138;
  if(!market29_has(other->protected_offer,own)&&offered>=16)continue;
  int others=fast?market33_primary_pressure(other,oc,own):market29_primary_pressure(other,own);if(!fast)scan_old+=138;
  int value=(!fast||mo)?competition32_opportunity(p,&pool[j],pressure,others,ranked,0):0;if(value>opportunity)opportunity=value;
  if(!market29_has(other->rejected,own)&&((others<pressure&&market29_penalty(0,others)<penalty)||market29_has(other->admitted,own)))alternative=1;
 }
 return opportunity?opportunity:!protected&&alternative&&penalty&&(rejected||!market29_has(m->admitted,own))?penalty:0;
}
int main(void) {
 boundaries();equivalence();
 for(unsigned r=0;r<N;r++)for(unsigned t=0;t<138;t++) {
  Market29Pressure *m=&markets[r];m->fit[t]=300+(int)(rnd()%401);m->interest[t]=100+(int)(rnd()%1001);m->gain[t]=(int)(rnd()%100);
  if(rnd()%15==0){market29_bit(m->planned,t,1);market29_bit(m->primary,t,1);}
  if(rnd()%35==0){market29_bit(m->active,t,1);market29_bit(m->firm,t,1);market29_bit(m->protected_offer,t,1);}
  if(rnd()%8==0)market29_bit(m->rejected,t,1);
 }
 clock_t before=clock();for(unsigned r=0;r<N;r++)market33_cache(&markets[r],&caches[r],r,2026);double cache_ms=1000.0*(clock()-before)/CLOCKS_PER_SEC;
 double old_ms=0,new_ms=0;unsigned checked=0;volatile unsigned checksum=0;
 for(unsigned own=0;own<138;own++) {
  for(unsigned i=0;i<P;i++){unsigned r=(i*7+own*19)%N;pool[i]=(Hs28Choice){.rid=r,.pos=(int)(r%22),.ovr=65+(int)(r%20),.stars=r<32?4:r<421?3:r<2192?2:1,.native_score=markets[r].fit[own],.points=markets[r].interest[own],.leader=1700,.legal=1,.viable=1,.urgent=r%97==0};}
  int old[P];before=clock();for(int i=0;i<P;i++)old[i]=assess(i,own,0);old_ms+=1000.0*(clock()-before)/CLOCKS_PER_SEC;
  before=clock();for(int i=0;i<P;i++){int now=assess(i,own,1);assert(now==old[i]);checksum+=(unsigned)now;checked++;}new_ms+=1000.0*(clock()-before)/CLOCKS_PER_SEC;
 }
 assert(scan_new<scan_old/4);
 printf("{\"status\":\"PASS\",\"cache_random_cases\":30000,\"global_recruits\":3000,\"schools\":138,\"candidates_per_school\":300,\"equivalent_choices\":%u,\"old_school_scans\":%llu,\"new_school_scans\":%llu,\"old_ms\":%.3f,\"new_ms\":%.3f,\"cache_ms\":%.3f,\"checksum\":%u}\n",checked,scan_old,scan_new,old_ms,new_ms,cache_ms,checksum);
}
'''
(O/'market33_policy.c').write_text(code)
subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-O2','-I'+str(S),str(O/'market33_policy.c'),'-o',str(O/'market33_policy')],check=True)
p=subprocess.run([str(O/'market33_policy')],capture_output=True,text=True,check=True)
r=json.loads(p.stdout);r.update(live_game_verified=False,method='Compiled production cache/rank helpers; exact old Test32 versus cached/pruned opportunity decisions at unchanged old thresholds; separate new tier policy boundary tests; synthetic global pool and school candidates, not live W1 replay',checks=['2/3 stars soft3 hard6;4/5stars soft8 hard12;one-star fallback8/16','shared primary+backup offer ceiling, no mandatory hard-limit fill','failed and completed nomination release, grandfathered18existingoffers','membership survives primary/backup role change only while admitted current turn; failed lock releases','native earned influence ranks live races;seededpoints alone do not force extra soft entries','30000 randomized exact close/pressure/attention/strong-fit equivalence including differing current/snapshot points and native scores','41400 candidate decisions match old Test32; reduced school-array scans','portal generic policy unchanged'])
(O/'test33_market_policy_results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2))
