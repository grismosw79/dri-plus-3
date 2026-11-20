#ifndef DRI_GAME_IDENTITY_H
#define DRI_GAME_IDENTITY_H
#include <string.h>

/* Existing qualified TU1117 executable. */
static const char GAME_HASH[] = "3bc0317b57735ae9615051ea00c8840b0bcf895fdcfc7e8738c6565423f764c0";
/* Same fingerprint reported by two Steam/MMC users. T3 is a test build:
   full native capture comparison and live qualification are still pending.
   Accepting this identity never skips DriInitialize or any native guard. */
static const char GAME_HASH_STEAM_MMC[] = "ed6b60934e818e291fd6802988f6dd64353f533d2410a2e03d95b2beeb5bc471";

static int supported_game_hash(const char *digest) {
    return digest && (!strcmp(digest,GAME_HASH) || !strcmp(digest,GAME_HASH_STEAM_MMC));
}
#endif
