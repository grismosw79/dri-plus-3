/* RM18R22: annual RM12 policy. No quotas or changed rates. Portable for parity tests. */
#ifndef RM18R22_ANNUAL_POLICY_H
#define RM18R22_ANNUAL_POLICY_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "annual_stats.h"
typedef struct {
 unsigned row,pid; int team,source,position,type,before,score,native_score;
 int up_outcomes,down_bp,has_row,reason,school,redshirt,injury,max_injury;
 int supported,metric_valid,group; double metric,confidence,percentile;
 int stats[RM18R22_STAT_COUNT];
} RMDevRecord;
static const char *dev_reasons[]={"bottom_decile_with_sample","bottom_quartile_with_sample","established_low_usage","injury_context_protected","missing_or_ambiguous_season","no_regression_trigger","protected_first_year_or_redshirt"};
static void annual_metric(RMDevRecord *d) {
 const int *v=d->stats; int p=d->position,downs=v[DS_DOWNSPLAYED];
 d->native_score=d->has_row?annual_native_score(p,v):0;
 d->group=(p==9?5:p==8?6:p);d->metric_valid=0;d->metric=0;d->percentile=-1;
 double exposure=p==19?v[DS_KICKFGATTEMPTS]/20.0:p==20?v[DS_PUNTATTEMPTS]/40.0:downs/600.0;
 d->confidence=d->has_row?fmax(0,fmin(1,exposure)):0;
 if(!d->has_row)return;
 if(p>=5&&p<=9&&downs>=300){d->metric=-1000.0*v[DS_OLINESACKSALLOWED]/downs;d->metric_valid=1;}
 else if(p==19&&v[DS_KICKFGATTEMPTS]>=10){
  double terms[]={v[DS_KICKFGATTEMPTS29ORLESS]*.95,v[DS_KICKFGATTEMPTS30TO39]*.85,v[DS_KICKFGATTEMPTS40TO49]*.70,v[DS_KICKFGATTEMPTS50ORMORE]*.50};
  double expected=0,correction=0;for(unsigned k=0;k<4;k++){double t=expected+terms[k];correction+=fabs(expected)>=fabs(terms[k])?(expected-t)+terms[k]:(terms[k]-t)+expected;expected=t;}expected+=correction;
  d->metric=(v[DS_KICKFGMADE]-expected)/v[DS_KICKFGATTEMPTS];d->metric_valid=1;
 }else if(p==20&&v[DS_PUNTATTEMPTS]>=20){double a=v[DS_PUNTATTEMPTS];d->metric=v[DS_PUNTNETYARDS]/a+4*v[DS_PUNTIN20]/a-4*v[DS_PUNTTOUCHBACKS]/a;d->metric_valid=1;}
 else if((p==0||p==1||p==3||p==4||(p>=10&&p<=18))&&downs>=300){d->metric=(double)d->native_score/downs*600;d->metric_valid=1;}
}
static int annual_double_compare(const void *a,const void *b){double x=*(const double*)a,y=*(const double*)b;return (x>y)-(x<y);}
/* Sort one group at a time; memory is O(players), ranks are midpoint ties. */
static void annual_evaluate(RMDevRecord *d,unsigned count,double *scratch) {
 static const int top[]={40,25,10,0},next[]={15,10,4,0},part[]={5,2,1,0};
 for(unsigned i=0;i<count;i++)annual_metric(d+i);
 for(int group=0;group<22;group++){
  unsigned n=0;for(unsigned i=0;i<count;i++)if(d[i].group==group&&d[i].metric_valid)scratch[n++]=d[i].metric;
  if(n<20)continue;qsort(scratch,n,sizeof(double),annual_double_compare);
  for(unsigned i=0;i<count;i++)if(d[i].group==group&&d[i].metric_valid){
   unsigned lo=0,hi=n;while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(scratch[mid]<d[i].metric)lo=mid+1;else hi=mid;}unsigned less=lo;
   hi=n;while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(scratch[mid]<=d[i].metric)lo=mid+1;else hi=mid;}
   d[i].percentile=(less+.5*(lo-less))/n;
  }
 }
 for(unsigned i=0;i<count;i++){
  RMDevRecord *r=d+i;double rate=0;int t=r->before,p=r->position;
  r->up_outcomes=r->down_bp=0;r->reason=5;
  if(t<0||t>3||p<0||p>21){r->supported=0;r->score=0;continue;}
  if(r->percentile>=.9)rate=top[t]*r->confidence;
  else if(r->percentile>=.75)rate=next[t]*r->confidence;
  else if(r->percentile<0&&(p==2||p==21)&&r->has_row&&r->confidence>=.5)rate=part[t]*r->confidence;
  r->up_outcomes=(int)floor(rate*101/100+1e-9);r->score=t<3?25*(t+1)+r->up_outcomes-1:0;
  if(!r->has_row)r->reason=4;
  else if((r->school==0&&r->redshirt!=3)||r->redshirt==2)r->reason=6;
  else if((r->injury!=255&&r->injury!=-1)||r->max_injury>0)r->reason=3;
  else if(r->stats[DS_DOWNSPLAYED]<100&&r->stats[DS_GAMESPLAYED]>=8&&r->stats[DS_GAMESSTARTED]==0){r->reason=2;r->down_bp=2500;}
  else if(r->percentile>=0&&r->percentile<.1){r->reason=0;r->down_bp=3500;}
  else if(r->percentile>=0&&r->percentile<.25){r->reason=1;r->down_bp=2000;}
 }
}
static uint64_t annual_draw(const RMDevRecord *d,int year,uint64_t domain){
 /* Year zero reproduces RM12 exactly. Later seasons get their own stable draw. */
 uint64_t x=((uint64_t)d->pid<<32)^d->row^UINT64_C(0x524d30395f323032)^domain^((uint64_t)(unsigned)year*UINT64_C(0x9e3779b97f4a7c15));
 x=(x^(x>>30))*UINT64_C(0xbf58476d1ce4e5b9);x=(x^(x>>27))*UINT64_C(0x94d049bb133111eb);return x^(x>>31);
}
#endif
