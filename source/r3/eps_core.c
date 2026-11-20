/* EPS-1.0 numerical core. No game pointers, file writes or Windows dependency. */
#include "eps_core.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

typedef struct {double production[4],conceded[4];} EpsFactor;
typedef struct {double mse,se;double *b,*inv;} EpsFit;
static double eps_signed(double v,double f){return v>=0?v*f:v/f;}
static double eps_clip(double x){return x<.75?.75:x>1.25?1.25:x;}
static int eps_finite(double x){return isfinite(x)!=0;}
static int eps_band(double r){for(int i=0;i<13;i++)if(r<=eps_grade_cuts[i])return i;return 12;}
static void eps_ranks(const double *v,double *r,int high){
 for(int i=0;i<EPS_TEAMS;i++){int better=0,same=0;for(int j=0;j<EPS_TEAMS;j++){
  double delta=v[j]-v[i];if(fabs(delta)<=1e-10)same++;else if(high?delta>0:delta<0)better++;
 }r[i]=1+better+(same-1)*.5;}
}
int eps_profile(int a,int pos){if(a<0||a>=63)return -1;if(pos==20&&a>=61)return a==61?EPS_P_P_PLACEMENT:EPS_P_P_POWER;return eps_archetype_profiles[a];}
int eps_grade_pass(const EpsGrade *g,int req,int allowance){if(!g||req<0||req>12||allowance<0)return 0;if(g->status==0)return 1;int r=req+allowance;return g->grade<=(r>12?12:r);}
/* Cholesky factor and inverse. Ridge leaves the intercept unpenalized. */
static int eps_inverse(double *a,double *inv,int n){
 double *x=calloc((size_t)n,sizeof(double));if(!x)return 0;
 for(int i=0;i<n;i++)for(int j=0;j<=i;j++){
  double s=a[i*n+j];for(int k=0;k<j;k++)s-=a[i*n+k]*a[j*n+k];
  if(i==j){if(!(s>1e-14)||!eps_finite(s)){free(x);return 0;}a[i*n+j]=sqrt(s);}else a[i*n+j]=s/a[j*n+j];
 }
 for(int col=0;col<n;col++){
  for(int i=0;i<n;i++){double v=i==col?1.:0.;for(int k=0;k<i;k++)v-=a[i*n+k]*x[k];x[i]=v/a[i*n+i];}
  for(int i=n-1;i>=0;i--){double v=x[i];for(int k=i+1;k<n;k++)v-=a[k*n+i]*x[k];x[i]=v/a[i*n+i];}
  for(int i=0;i<n;i++)inv[i*n+col]=x[i];
 }free(x);return 1;
}
static void eps_x(const EpsRow *r,int teams,int x[3]){x[0]=0;x[1]=1+r->team;x[2]=1+teams+r->opponent;}
static double eps_dot3(const double *b,const int *x){return b[x[0]]+b[x[1]]+b[x[2]];}
static int eps_fit(const EpsRow *rows,int count,int teams,const double *y,const double *w,double penalty,EpsFit *fit,EpsFactor *factors,int channel){
 int n=1+2*teams;size_t nn=(size_t)n*n;double *a=calloc(nn,sizeof(double)),*iv=calloc(nn,sizeof(double)),*rhs=calloc((size_t)n,sizeof(double)),*b=calloc((size_t)n,sizeof(double));
 double *k0=calloc((size_t)n,sizeof(double)),*k1=calloc((size_t)n,sizeof(double)),*bm=calloc((size_t)n,sizeof(double));
 if(!a||!iv||!rhs||!b||!k0||!k1||!bm)goto fail;
 for(int j=1;j<n;j++)a[j*n+j]=penalty;
 for(int i=0;i<count;i++)if(w[i]>0){int x[3];eps_x(rows+i,teams,x);for(int j=0;j<3;j++){rhs[x[j]]+=w[i]*y[i];for(int k=0;k<3;k++)a[x[j]*n+x[k]]+=w[i];}}
 if(!eps_inverse(a,iv,n))goto fail;
 for(int i=0;i<n;i++)for(int j=0;j<n;j++)b[i]+=iv[i*n+j]*rhs[j];
 double sumerr=0,sumsq=0;int games=0;
 for(int i=0;i<count;i+=2){
  int x0[3],x1[3];eps_x(rows+i,teams,x0);eps_x(rows+i+1,teams,x1);
  for(int j=0;j<n;j++){k0[j]=iv[j*n+x0[0]]+iv[j*n+x0[1]]+iv[j*n+x0[2]];k1[j]=iv[j*n+x1[0]]+iv[j*n+x1[1]]+iv[j*n+x1[2]];}
  double c0=0,c1=0,v0=y[i]-eps_dot3(b,x0),v1=y[i+1]-eps_dot3(b,x1);
  if(w[i]>0&&w[i+1]>0){double aa=1/w[i]-eps_dot3(k0,x0),bb=-eps_dot3(k1,x0),cc=-eps_dot3(k0,x1),dd=1/w[i+1]-eps_dot3(k1,x1),det=aa*dd-bb*cc;if(!(det>0))goto fail;c0=(v0*dd-bb*v1)/det;c1=(aa*v1-cc*v0)/det;}
  else if(w[i]>0){double den=1/w[i]-eps_dot3(k0,x0);if(!(den>0))goto fail;c0=v0/den;}
  else if(w[i+1]>0){double den=1/w[i+1]-eps_dot3(k1,x1);if(!(den>0))goto fail;c1=v1/den;}
  for(int j=0;j<n;j++)bm[j]=b[j]-k0[j]*c0-k1[j]*c1;
  double err0=y[i]-eps_dot3(bm,x0),err1=y[i+1]-eps_dot3(bm,x1),ww=w[i]+w[i+1];
  if(ww>0){double err=(w[i]*err0*err0+w[i+1]*err1*err1)/ww;sumerr+=err;sumsq+=err*err;games++;}
  if(factors){double om=0,dm=0;for(int t=0;t<EPS_TEAMS;t++){om+=bm[1+t];dm+=bm[1+teams+t];}om/=EPS_TEAMS;dm/=EPS_TEAMS;double mu=bm[0]+om+dm;if(!(mu>0))goto fail;
   for(int side=0;side<2;side++){const EpsRow *r=rows+i+side;double pe=mu+bm[1+teams+r->opponent]-dm,ce=mu+bm[1+r->team]-om;factors[i+side].production[channel]=eps_clip(mu/fmax(mu*.25,pe));factors[i+side].conceded[channel]=eps_clip(mu/fmax(mu*.25,ce));}
  }
 }
 fit->mse=games?sumerr/games:0;fit->se=games>1?sqrt(fmax(0,(sumsq-sumerr*sumerr/games)/(games-1))/games):0;fit->b=0;fit->inv=0;
 free(a);free(iv);free(rhs);free(b);free(k0);free(k1);free(bm);return 1;
fail:free(a);free(iv);free(rhs);free(b);free(k0);free(k1);free(bm);return 0;
}
static int eps_factors(const EpsRow *rows,int count,int teams,EpsFactor *f,EpsOutput *out){
 static const double grid[10]={.25,.5,1,2,4,8,16,32,64,128};
 double *y=calloc((size_t)count+1,sizeof(double)),*w=calloc((size_t)count+1,sizeof(double));if(!y||!w){free(y);free(w);return 0;}
 for(int i=0;i<count;i++)for(int c=0;c<4;c++)f[i].production[c]=f[i].conceded[c]=1;
 for(int c=0;c<4;c++){
  double total=0,ws=0;int wn=0,active=0;
  for(int i=0;i<count;i++){const EpsRow *r=rows+i,*opp=rows+(i^1);double num=c==0?r->pass_yards:c==1?r->rush_yards:c==2?opp->sacks:opp->group[EPS_G_ALL][EPS_S_DEFTACKLESFORLOSS];w[i]=c==0?r->pass_attempts:c==1?r->rush_attempts:c==2?r->pass_attempts+opp->sacks:r->pass_attempts+r->rush_attempts;y[i]=w[i]>0?num/w[i]:0;total+=num;if(r->team<EPS_TEAMS&&w[i]>0){ws+=w[i];wn++;}}
  for(int i=0;i<count;i+=2)active+=(w[i]>0||w[i+1]>0);
  if(!wn||total<=0||active<4){out->selected_prior_games[c]=0;continue;}
  EpsFit runs[10];int best=0;
  for(int k=0;k<10;k++){if(!eps_fit(rows,count,teams,y,w,grid[k]*ws/wn,runs+k,0,c))goto fail;if(runs[k].mse<runs[best].mse)best=k;}
  int selected=best;for(int k=0;k<10;k++)if(runs[k].mse<=runs[best].mse+runs[best].se)selected=k;
  out->selected_prior_games[c]=grid[selected];EpsFit final;if(!eps_fit(rows,count,teams,y,w,grid[selected]*ws/wn,&final,f,c))goto fail;
 }
 free(y);free(w);return 1;
fail:free(y);free(w);return 0;
}

static void eps_add(EpsOutput *out,int t,int metric,double value){out->metric[t][metric].numerator+=value;}
static void eps_denominator(int m,int *den,double *prior,int *volume){
 *den=EPS_E_GAMES;*prior=4;*volume=1;
 switch(m){
 case EPS_M_PASS_ALLOWED_RATE:*den=EPS_E_OPPONENTPASSATTEMPTS;*prior=100;break;
 case EPS_M_RUSH_ALLOWED_RATE:*den=EPS_E_OPPONENTRUSHATTEMPTS;*prior=100;break;
 case EPS_M_SACK_ALLOWED_RATE:*den=EPS_E_DROPBACKS;*prior=100;break;
 case EPS_M_DE_SACK_RATE:case EPS_M_DT_SACK_RATE:case EPS_M_OLB_SACK_RATE:*den=EPS_E_OPPONENTDROPBACKS;*prior=100;break;
 case EPS_M_DE_TFL_RATE:case EPS_M_DT_TFL_RATE:case EPS_M_OLB_TFL_RATE:case EPS_M_MLB_TFL_RATE:case EPS_M_OLB_TACKLE_RATE:case EPS_M_MLB_TACKLE_RATE:*den=EPS_E_OPPONENTPLAYS;*prior=200;break;
 case EPS_M_KICKOFF_TOUCHBACK_RATE:*den=EPS_E_KICKOFFS;*prior=20;break;
 case EPS_M_PUNT_AVERAGE:case EPS_M_PUNT_INSIDE20_RATE:case EPS_M_PUNT_TOUCHBACK_RATE:*den=EPS_E_PUNTS;*prior=20;break;
 case EPS_M_FG_DISTANCE_RESIDUAL:*den=EPS_E_FGATTEMPTS;*prior=10;break;
 default:return;
 }*volume=0;
}
int eps_compute(const EpsRow *rows,int count,int teams,int year,const EpsOutput *previous,EpsOutput *out){
 if(!out||count<0||count>EPS_MAX_ROWS||(count&1)||teams<EPS_TEAMS||teams>EPS_MAX_TEAMS||(!rows&&count))return 0;
 memset(out,0,sizeof(*out));out->year=year;out->rows=count;out->teams=teams;
 if(previous&&(previous->year!=year-1||previous->teams!=teams))previous=0;
 for(int i=0;i<count;i++){const EpsRow *r=rows+i,*opp=rows+(i^1);if(r->team<0||r->team>=teams||r->opponent<0||r->opponent>=teams||r->game!=opp->game||r->team!=opp->opponent||r->opponent!=opp->team||r->team==r->opponent||r->pass_attempts<0||r->rush_attempts<0||r->sacks<0)return 0;}
 EpsFactor *f=calloc((size_t)count+1,sizeof(*f));if(!f)return 0;if(!eps_factors(rows,count,teams,f,out)){free(f);return 0;}
 double fg[EPS_TEAMS][8]={{0}},fgmade[EPS_TEAMS]={0},fgtotal[8]={0};
 for(int i=0;i<count;i++){
  const EpsRow *r=rows+i,*opp=rows+(i^1);int t=r->team;if(t>=EPS_TEAMS)continue;
  const double *all=r->group[EPS_G_ALL];double *ex=out->exposure[t];double fp=f[i].production[0],fr=f[i].production[1],cp=f[i^1].conceded[0],cr=f[i^1].conceded[1];
  ex[EPS_E_GAMES]++;ex[EPS_E_PASSATTEMPTS]+=r->pass_attempts;ex[EPS_E_RUSHATTEMPTS]+=r->rush_attempts;ex[EPS_E_DROPBACKS]+=r->pass_attempts+opp->sacks;ex[EPS_E_OPPONENTPASSATTEMPTS]+=opp->pass_attempts;ex[EPS_E_OPPONENTRUSHATTEMPTS]+=opp->rush_attempts;ex[EPS_E_OPPONENTDROPBACKS]+=opp->pass_attempts+r->sacks;ex[EPS_E_OPPONENTPLAYS]+=opp->pass_attempts+opp->rush_attempts;
  ex[EPS_E_FGATTEMPTS]+=all[EPS_S_KICKFGATTEMPTS];ex[EPS_E_KICKOFFS]+=all[EPS_S_KICKNUMKICKOFFS];ex[EPS_E_PUNTS]+=all[EPS_S_PUNTATTEMPTS];
  double pass=eps_signed(r->pass_yards,fp),rush=eps_signed(r->rush_yards,fr);
  eps_add(out,t,EPS_M_PASS_YPG,pass);eps_add(out,t,EPS_M_RUSH_YPG,rush);eps_add(out,t,EPS_M_TOTAL_YPG,pass+rush);eps_add(out,t,EPS_M_PASS_ATTEMPTS_PG,r->pass_attempts);eps_add(out,t,EPS_M_RUSH_ATTEMPTS_PG,r->rush_attempts);
  eps_add(out,t,EPS_M_SACK_ALLOWED_RATE,opp->sacks*f[i].production[2]);eps_add(out,t,EPS_M_PASS_ALLOWED_RATE,eps_signed(opp->pass_yards,cp));eps_add(out,t,EPS_M_RUSH_ALLOWED_RATE,eps_signed(opp->rush_yards,cr));
  eps_add(out,t,EPS_M_QB_RUSH_YPG,eps_signed(r->group[EPS_G_QB][EPS_S_RUSHYARDS],fr));eps_add(out,t,EPS_M_RB_RUSH_YPG,eps_signed(r->group[EPS_G_RB][EPS_S_RUSHYARDS],fr));eps_add(out,t,EPS_M_RB_CARRIES_PG,r->group[EPS_G_RB][EPS_S_RUSHATTEMPTS]);
  eps_add(out,t,EPS_M_HB_CATCHES_PG,r->group[EPS_G_HB][EPS_S_RECEIVECATCHES]);eps_add(out,t,EPS_M_HB_RECEIVING_YPG,eps_signed(r->group[EPS_G_HB][EPS_S_RECEIVEYARDS],fp));
  for(int z=0;z<2;z++){const double *g=r->group[z?EPS_G_TE:EPS_G_WR];double recv=eps_signed(g[EPS_S_RECEIVEYARDS],fp),air=eps_signed(g[EPS_S_RECEIVEYARDS]-g[EPS_S_RECEIVEYARDSAFTER],fp);eps_add(out,t,z?EPS_M_TE_CATCHES_PG:EPS_M_WR_CATCHES_PG,g[EPS_S_RECEIVECATCHES]);eps_add(out,t,z?EPS_M_TE_RECEIVING_YPG:EPS_M_WR_RECEIVING_YPG,recv);eps_add(out,t,z?EPS_M_TE_AIR_YPG:EPS_M_WR_AIR_YPG,air);
   if(!z){double wrush=eps_signed(g[EPS_S_RUSHYARDS],fr);eps_add(out,t,EPS_M_WR_SCRIMMAGE_YPG,recv+wrush);eps_add(out,t,EPS_M_WR_OPENFIELD_YPG,eps_signed(g[EPS_S_RECEIVEYARDSAFTER],fp)+wrush);}}
  const int dg[4]={EPS_G_DE,EPS_G_DT,EPS_G_OLB,EPS_G_MLB},sm[3]={EPS_M_DE_SACK_RATE,EPS_M_DT_SACK_RATE,EPS_M_OLB_SACK_RATE},tm[4]={EPS_M_DE_TFL_RATE,EPS_M_DT_TFL_RATE,EPS_M_OLB_TFL_RATE,EPS_M_MLB_TFL_RATE};
  for(int z=0;z<4;z++){const double *g=r->group[dg[z]];if(z<3)eps_add(out,t,sm[z],(g[EPS_S_DLINESACKS]+.5*g[EPS_S_DLINEHALFSACK])*f[i^1].conceded[2]);eps_add(out,t,tm[z],g[EPS_S_DEFTACKLESFORLOSS]*f[i^1].conceded[3]);if(z>=2)eps_add(out,t,z==2?EPS_M_OLB_TACKLE_RATE:EPS_M_MLB_TACKLE_RATE,g[EPS_S_DEFTACKLES]+g[EPS_S_ASSDEFTACKLES]);}
  eps_add(out,t,EPS_M_FG_ATTEMPTS_PG,all[EPS_S_KICKFGATTEMPTS]);fgmade[t]+=all[EPS_S_KICKFGMADE];
  eps_add(out,t,EPS_M_KICKOFF_TOUCHBACK_RATE,all[EPS_S_KICKTOUCHBACKS]);eps_add(out,t,EPS_M_PUNT_AVERAGE,all[EPS_S_PUNTYARDS]);eps_add(out,t,EPS_M_PUNT_INSIDE20_RATE,all[EPS_S_PUNTIN20]);eps_add(out,t,EPS_M_PUNT_TOUCHBACK_RATE,all[EPS_S_PUNTTOUCHBACKS]);
  for(int b=0;b<8;b++){double v=all[EPS_S_KICKFGATTEMPTS29ORLESS+b];fg[t][b]+=v;fgtotal[b]+=v;}
 }
 free(f);
 for(int m=0;m<EPS_METRICS;m++){
  int den,volume;double prior;eps_denominator(m,&den,&prior,&volume);double sum=0,n=0;for(int t=0;t<EPS_TEAMS;t++){sum+=out->metric[t][m].numerator;n+=out->exposure[t][den];}double national=n?sum/n:0;
  for(int t=0;t<EPS_TEAMS;t++){EpsMetric *x=&out->metric[t][m];x->denominator=out->exposure[t][den];x->prior_exposure=prior;x->prior_value=national;
   if(previous){int any=0;for(int p=0;p<EPS_PROFILES;p++)any|=previous->grade[t][p].status==1;if(any)x->prior_value=previous->metric[t][m].value;}
   if(m==EPS_M_FG_DISTANCE_RESIDUAL){double expected=0;for(int b=0;b<8;b+=2){double tries=fgtotal[b]-fg[t][b],made=fgtotal[b+1]-fg[t][b+1];expected+=fg[t][b]*(tries?made/tries:.5);}x->numerator=fgmade[t]-expected;x->prior_value=0;}
   x->observed=x->denominator?x->numerator/x->denominator:NAN;x->value=(x->numerator+prior*x->prior_value)/(x->denominator+prior);
   if(volume&&x->numerator==0&&out->exposure[t][EPS_E_GAMES]>=4)x->value=0;
   if(!eps_finite(x->value))return 0;
  }
 }
 double values[EPS_TEAMS],ranks[EPS_TEAMS],scores[EPS_TEAMS];
 for(int p=0;p<EPS_PROFILES;p++){
  const EpsProfile *d=eps_profiles+p;memset(scores,0,sizeof(scores));
  for(int k=0;k<d->count;k++){EpsPart part=d->part[k];for(int t=0;t<EPS_TEAMS;t++)values[t]=out->metric[t][part.metric].value;eps_ranks(values,ranks,part.high);for(int t=0;t<EPS_TEAMS;t++)scores[t]+=part.weight*(EPS_TEAMS-ranks[t])/(EPS_TEAMS-1)*100;}
  eps_ranks(scores,ranks,1);
  for(int t=0;t<EPS_TEAMS;t++){EpsGrade *g=&out->grade[t][p];const double *ex=out->exposure[t];g->score=scores[t];g->rank=ranks[t];g->grade=eps_band(ranks[t]);g->status=ex[EPS_E_GAMES]>=4&&ex[d->exposure]>=d->minimum&&ex[d->extra_exposure]>=d->extra_minimum;
   if(!g->status&&previous&&previous->grade[t][p].status==1){*g=previous->grade[t][p];g->status=2;}
  }
 }return 1;
}
