#include "eps_snapshot.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
 assert(argc==2);EpsDisk *d=calloc(1,sizeof(*d));assert(d);
 FILE *f=fopen(argv[1],"rb");assert(f);assert(fread(&d->output,1,sizeof(d->output),f)==sizeof(d->output));assert(fgetc(f)==EOF);fclose(f);
 d->magic=EPS_DISK_MAGIC;d->version=EPS_DISK_VERSION;d->bytes=sizeof(EpsOutput);d->league=1549427592;
 d->year=2026;d->index=0;d->signature=123456789;d->final=1;d->cache_hash=987654321;d->output_hash=eps_hash(&d->output,sizeof(d->output));
 assert(eps_disk_valid(d,d->league,2026,0,d->signature,0));
 assert(eps_disk_valid(d,d->league,2026,0,0,1));
 assert(!eps_disk_valid(d,d->league+1,2026,0,d->signature,0));
 assert(!eps_disk_valid(d,d->league,2027,0,d->signature,0));
 assert(!eps_disk_valid(d,d->league,2026,1,d->signature,0));
 assert(!eps_disk_valid(d,d->league,2026,0,d->signature+1,0));
 d->final=0;assert(!eps_disk_valid(d,d->league,2026,0,0,1));d->final=1;
 d->version++;assert(!eps_disk_valid(d,d->league,2026,0,d->signature,0));d->version--;
 d->output.grade[0][0].grade^=1;assert(!eps_disk_valid(d,d->league,2026,0,d->signature,0));d->output.grade[0][0].grade^=1;
 d->output.grade[0][0].status=3;d->output_hash=eps_hash(&d->output,sizeof(d->output));
 assert(!eps_disk_valid(d,d->league,2026,0,d->signature,0));
 free(d);puts("PASS: valid current/final snapshots; wrong dynasty/year/index/season/version, unfinished prior, corrupted payload and invalid status rejected");return 0;
}
