/* Snapshot identity/content checks, independent of the native file API. */
#include "eps_snapshot.h"
#include <math.h>
uint64_t eps_hash(const void *data,size_t size){
 const unsigned char *p=data;uint64_t h=UINT64_C(1469598103934665603);
 for(size_t i=0;i<size;i++){h^=p[i];h*=UINT64_C(1099511628211);}return h;
}
int eps_disk_valid(const EpsDisk *d,unsigned league,int year,int index,uint64_t signature,int final){
 if(d->magic!=EPS_DISK_MAGIC||d->version!=EPS_DISK_VERSION||d->bytes!=sizeof(EpsOutput)||
    d->league!=league||d->year!=(unsigned)year||d->index!=(unsigned)index||
    d->output.year!=year||d->output.teams<138||d->output.teams>154||
    d->output.rows<0||d->output.rows>4096||(d->output.rows&1)||
    (final?!d->final:d->signature!=signature)||d->output_hash!=eps_hash(&d->output,sizeof(d->output)))return 0;
 for(int t=0;t<EPS_TEAMS;t++){for(int p=0;p<EPS_PROFILES;p++){
  const EpsGrade *g=&d->output.grade[t][p];
  if(g->grade<0||g->grade>12||g->status<0||g->status>2||!isfinite(g->rank)||!isfinite(g->score))return 0;
 }}return 1;
}
