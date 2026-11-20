#ifndef DRI_EPS_COLLECT_H
#define DRI_EPS_COLLECT_H
#include <stdint.h>
#include "eps_core.h"
typedef uintptr_t EpsRec;
typedef struct {
 EpsRec (*ref)(EpsRec,int);
 EpsRec (*array)(EpsRec,int);
 EpsRec (*at)(EpsRec,unsigned);
 unsigned (*count)(EpsRec);
 int32_t (*value)(EpsRec,int,int);
 uint64_t (*key)(EpsRec);
 int (*same)(EpsRec,EpsRec);
 void (*keep)(EpsRec);
 void (*drop)(EpsRec);
 int (*stat_fields)(EpsRec);
} EpsAPI;
/* reconciliations is a retained ABI counter: five validated team measures
   per row. It no longer asserts equality with individual aggregates. */
typedef struct {int success,rows,teams,players,player_games,fcs_games,reconciliations;const char *error;uint64_t schedule_hash;} EpsReadResult;
/* Independent value validity; never requires agreement between sources. */
int eps_row_valid(const EpsRow *row);
/* Diagnostic only: 0 difference/outside historical bounds (or invalid),
   1 exact, 2 historical small residual. Never edits or rejects a row. */
int eps_reconcile_row(const EpsRow *row);
/* cutoff is an exclusive regular-season week; -1 includes the full season. */
EpsReadResult eps_collect_through(const EpsAPI *api,EpsRec franchise,int year_index,EpsRow *rows,int cutoff);
EpsReadResult eps_collect(const EpsAPI *api,EpsRec franchise,int year_index,EpsRow *rows);
#endif
