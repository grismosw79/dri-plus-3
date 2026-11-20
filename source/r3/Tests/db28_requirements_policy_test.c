#include <assert.h>
#include <stdio.h>
#include "../db28_policy.h"
int main(void) {
    int checks=0;
    for(int kind=-1;kind<=16;kind++) {
        int expected=kind==2||kind==4||kind==7||kind==9||kind==10;
        assert(db28_shared_college_motivation(kind)==expected);checks++;
    }
    for(int c=-20;c<=20;c++) {
        assert(db28_transfer_class(c)==(c>=4&&c<=7));checks++;
    }
    for(int old=0;old<=12;old++)for(int now=0;now<=12;now++)
    for(int coach=0;coach<=6;coach++) {
        int answer=-1;assert(db28_replace_base(old+coach,old,now,&answer));
        assert(answer==now+coach);checks++;
    }
    int untouched=99;
    assert(!db28_replace_base(4,-1,4,&untouched)&&untouched==99);
    assert(!db28_replace_base(4,4,13,&untouched)&&untouched==99);
    assert(!db28_replace_base(4,4,4,0));
    for(int kind=0;kind<16;kind++)if(db28_shared_college_motivation(kind)) {
        const int *v=db28_expected_college_curve(kind);
        for(int ovr=0;ovr<100;ovr++) {
            int expected;
            if(kind==7)expected=ovr<80?8:ovr<85?4:2;
            else if(kind==9)expected=ovr<45?12:ovr>=95?1:12-(ovr-40)/5;
            else if(kind==10)expected=ovr<70?10:ovr<75?9:ovr<82?7:ovr<90?5:2;
            else expected=ovr<70?10:ovr<75?9:ovr<80?7:ovr<85?5:ovr<90?4:2;
            assert(db28_floor_grade(kind,v,ovr)==expected);checks++;
        }
        for(int changed=0;changed<13;changed++) {
            int bad[13];for(int i=0;i<13;i++)bad[i]=v[i];
            bad[changed]++;assert(db28_floor_grade(kind,bad,80)==-1);checks++;
        }
        assert(db28_floor_grade(kind,v,-1)==-1);
        assert(db28_floor_grade(kind,v,100)==-1);
    }
    printf("PASS: %d motivation, transfer-class, allowance, exact OVR band, and stale-curve rejection cases\n",checks);
    return 0;
}
