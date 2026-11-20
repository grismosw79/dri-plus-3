/* Runs the exact production identity gate; no Windows or game process required. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../source/game_identity.h"

int main(void) {
    const char *current="3bc0317b57735ae9615051ea00c8840b0bcf895fdcfc7e8738c6565423f764c0";
    const char *steam="ed6b60934e818e291fd6802988f6dd64353f533d2410a2e03d95b2beeb5bc471";
    const char *rejected[]={
        NULL,"","3bc0317b","ed6b6093",
        "8247e6dcc68b78672ac1c79e2deb41900925e6303a71c1e0ace02ef8602b3be2",
        "99ac5da70511c0c8ec5f8677d0e847e6443e1bfa56b22842228edae4723387f8",
        "0000000000000000000000000000000000000000000000000000000000000000",
        "ed6b60934e818e291fd6802988f6dd64353f533d2410a2e03d95b2beeb5bc471x"
    };
    int checks=0;
    assert(supported_game_hash(current));checks++;
    assert(supported_game_hash(steam));checks++;
    for(unsigned i=0;i<sizeof(rejected)/sizeof(rejected[0]);i++){
        assert(!supported_game_hash(rejected[i]));checks++;
    }
    const char *accepted[]={current,steam};
    for(unsigned k=0;k<2;k++)for(unsigned j=0;j<64;j++){
        char changed[65];memcpy(changed,accepted[k],65);
        changed[j]=changed[j]=='0'?'1':'0';
        assert(!supported_game_hash(changed));checks++;
    }
    printf("PASS %d identity cases: both exact hashes accepted; prior, unknown and mutated hashes rejected.\n",checks);
    return 0;
}
