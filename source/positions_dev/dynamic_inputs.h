/* Bounded live team state, rebuilt at each CPU's pre-rollover callback. */
#define RM_MAX_PLAYERS 180
#define RM_FIELD_COUNT 268
#define RM_TEAM_LIMIT 138
#define RM_CHART_COUNT 35
#define RM_OWNER_SLOTS 65536
typedef struct { U store;uint32_t row; } RMIdentity;
typedef struct { U store;unsigned row,pid;int source,position,type; } RMPilotInput;
typedef struct { U store;unsigned row;int team; } RMOwner;
static unsigned pilot_count,source_counts[2],chart_counts[RM_CHART_COUNT];
static RMPilotInput rm_inputs[RM_MAX_PLAYERS];
static int rm_initial[RM_MAX_PLAYERS][RM_FIELD_COUNT],rm_expected[RM_MAX_PLAYERS][RM_FIELD_COUNT];
static RMIdentity rm_members[2][RM_MAX_PLAYERS],rm_chart_expected[RM_CHART_COUNT][RM_MAX_PLAYERS];
static int unit_status[6],team_conversions;
static RMOwner owners[RM_OWNER_SLOTS];
static int league_valid,expected_cpu,census_membership[RM_TEAM_LIMIT],membership_conflict[RM_TEAM_LIMIT];
static int prep_seen[RM_TEAM_LIMIT],training_seen[RM_TEAM_LIMIT],completed_cpu,trained_cpu;
static RMIdentity census_franchise;
static int census_year=-1,census_season=-1;
static volatile LONG prep_busy;
