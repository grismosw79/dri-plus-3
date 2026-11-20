#ifndef DRI_VISIT29_CORE_H
#define DRI_VISIT29_CORE_H
#include <stdint.h>
#include <stddef.h>
/* Captured native legal-date vector: opponent reference, week, week type,
   opponent rank, offseason/bye marker. Only priority is temporary policy data. */
typedef struct {uintptr_t opponent; int32_t week,type,priority; unsigned char bye,pad[3];} Visit29Date;
_Static_assert(sizeof(Visit29Date)==24 && offsetof(Visit29Date,priority)==16,"native visit date ABI");
static int visit29_capacity(int native_capacity) {
    return native_capacity>10?10:native_capacity;
}
/* Native admission, cost, capacity, recruit conflicts and future-calendar
   checks have already built this candidate list. Reweight that list only.
   Within three weeks, retain native opponent preference. Beyond that window,
   prefer the earliest legal date, then the native opponent rank. */
static int visit29_dates(Visit29Date *dates,size_t count,int week,int type) {
    if(!dates || count==0 || count>32 || week<0 || week>17 || (type!=0 && type!=1)) return 0;
    for(size_t n=0;n<count;n++) {
        Visit29Date *d=&dates[n];
        if(d->type!=1 || d->week<=week || d->week>30 || d->priority<0 || d->priority>999) return 0;
    }
    for(size_t n=0;n<count;n++) {
        Visit29Date *d=&dates[n];int delay=d->week-week;
        if(delay>3)d->priority+=1000+delay*1000;
    }
    return 1;
}
#endif
