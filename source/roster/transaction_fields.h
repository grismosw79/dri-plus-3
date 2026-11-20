#ifndef RM_TRANSACTION_FIELDS_H
#define RM_TRANSACTION_FIELDS_H

typedef enum {
    RM_PRESERVE_FIELD,
    RM_TEAM_PRESENTATION,
    RM_ROSTER_BOOKKEEPING
} RMTransactionField;

/* Player schema indices. Used only after the game has completed a signing
   or retirement and the destination/source roster checks have passed. */
static RMTransactionField rm_transaction_field(unsigned index) {
    switch (index) {
    case 131: /* CaptainsPatch */
    case 162: /* JerseyNum */
    case 190: /* NumPrideStickers */
    case 217: /* PLYR_ISCAPTAIN */
        return RM_TEAM_PRESENTATION;
    case 139: /* CurrentNILCompensation: independently verified at each step */
    case 159: /* IsInjuredReserve */
    case 210: /* PLYR_CONSECYEARSWITHTEAM */
    case 220: /* PLYR_PREVTEAMID */
    case 230: /* PrevTeamIndex */
    case 261: /* TeamIndex: independently verified with unique ownership */
        return RM_ROSTER_BOOKKEEPING;
    default:
        return RM_PRESERVE_FIELD;
    }
}

#endif
