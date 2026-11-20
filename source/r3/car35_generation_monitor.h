#ifndef DRI_CAR35_GENERATION_MONITOR_H
#define DRI_CAR35_GENERATION_MONITOR_H
#include <stddef.h>
#include <string.h>
/* Launcher-only complete-line recognition; handles split reads and long rows. */
typedef struct { char line[96];size_t used;unsigned completed; } Car35GenerationMonitor;
static int car35_generation_feed(Car35GenerationMonitor *s,const char *bytes,size_t size) {
    int notice=0;const char prefix[]="CAR35_GENERATION boundary=end ";
    for(size_t i=0;i<size;i++) {
        if(bytes[i]=='\n') {
            s->line[s->used]=0;
            if(!strncmp(s->line,prefix,sizeof(prefix)-1)) {s->completed++;notice=1;}
            s->used=0;
        } else if(s->used+1<sizeof(s->line))s->line[s->used++]=bytes[i];
    }
    return notice;
}
#endif
