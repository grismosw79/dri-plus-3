#include <assert.h>
#include "car35_generation_monitor.h"
int main(void) {
    Car35GenerationMonitor s={0};
    const char *line="CAR35_GENERATION boundary=end event=1 observation_only=1\r\n";
    for(size_t i=0;i<strlen(line)-1;i++)assert(!car35_generation_feed(&s,line+i,1));
    assert(car35_generation_feed(&s,line+strlen(line)-1,1));assert(s.completed==1);
    assert(!car35_generation_feed(&s,"CAR35_GENERATION boundary=begin event=2\n",38));
    char long_line[1000];memset(long_line,'x',sizeof(long_line));
    assert(!car35_generation_feed(&s,long_line,sizeof(long_line)));
    assert(!car35_generation_feed(&s,line,strlen(line)));assert(s.completed==1);
    assert(car35_generation_feed(&s,line,strlen(line)));assert(s.completed==2);
    assert(!car35_generation_feed(&s,"",0));
    return 0;
}
