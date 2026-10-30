#pragma once
/*
 * OPT_USEC determines how many commands will be processed by the MUD per
 * second and how frequently it does socket I/O.  A low setting will cause
 * actions to be executed more frequently but will increase overhead due to
 * more cycling to check.  A high setting (e.g. 1 Hz) may upset your players
 * as actions (such as large speedwalking chains) take longer to be executed.
 * You shouldn't need to adjust this.
 */
constexpr int OPT_USEC	= 100000; /* 10 passes per second */
constexpr int PASSES_PER_SEC	= (1000000 / OPT_USEC);
#define RL_SEC		* PASSES_PER_SEC
#define CD_TICK         * PASSES_PER_SEC

#define PULSE_ZONE	(CONFIG_PULSE_ZONE RL_SEC)
#define PULSE_MOBILE    (CONFIG_PULSE_MOBILE RL_SEC)
#define PULSE_VIOLENCE  (CONFIG_PULSE_VIOLENCE RL_SEC)
constexpr int PULSE_AUCTION	= (15 * PASSES_PER_SEC);
#define PULSE_AUTOSAVE	(CONFIG_PULSE_AUTOSAVE RL_SEC)
#define PULSE_IDLEPWD	(CONFIG_PULSE_IDLEPWD RL_SEC)
#define PULSE_SANITY	(CONFIG_PULSE_SANITY RL_SEC)
#define PULSE_USAGE	(CONFIG_PULSE_SANITY * 300 RL_SEC)   /* 5 mins */
#define PULSE_TIMESAVE	(CONFIG_PULSE_TIMESAVE * 900 RL_SEC) /* should be >= SECS_PER_MUD_HOUR */
#define PULSE_CURRENT	(CONFIG_PULSE_CURRENT RL_SEC)
constexpr int PULSE_1SEC      = (1 * PASSES_PER_SEC);
constexpr int PULSE_2SEC      = (2 * PASSES_PER_SEC);
constexpr int PULSE_3SEC      = (3 * PASSES_PER_SEC);
constexpr int PULSE_4SEC      = (4 * PASSES_PER_SEC);
constexpr int PULSE_5SEC      = (5 * PASSES_PER_SEC);
constexpr int PULSE_6SEC      = (6 * PASSES_PER_SEC);
constexpr int PULSE_7SEC      = (7 * PASSES_PER_SEC);

/* Cool Down Ticks */
constexpr int PULSE_CD1       = (1 * PASSES_PER_SEC);
constexpr int PULSE_CD2       = (2 * PASSES_PER_SEC);
constexpr int PULSE_CD3       = (3 * PASSES_PER_SEC);
constexpr int PULSE_CD4       = (4 * PASSES_PER_SEC); /* This and the 3 above are for safety */
constexpr int PULSE_CD5       = (5 * PASSES_PER_SEC); /* Punch */
constexpr int PULSE_CD6       = (6 * PASSES_PER_SEC);
constexpr int PULSE_CD7       = (7 * PASSES_PER_SEC);
constexpr int PULSE_CD8       = (8 * PASSES_PER_SEC);
constexpr int PULSE_CD9       = (9 * PASSES_PER_SEC);
constexpr int PULSE_CD10      = (10 * PASSES_PER_SEC);
constexpr int PULSE_CD11      = (11 * PASSES_PER_SEC);
constexpr int PULSE_CD12      = (12 * PASSES_PER_SEC);
/* End CD Ticks    */

/* mud-life time */
constexpr int SECS_PER_MUD_HOUR	= 300;
constexpr int SECS_PER_MUD_DAY	= (24*SECS_PER_MUD_HOUR);
constexpr int SECS_PER_MUD_MONTH	= (30*SECS_PER_MUD_DAY);
constexpr int SECS_PER_MUD_YEAR	= (12*SECS_PER_MUD_MONTH);

/* real-life time (remember Real Life?) */
constexpr int SECS_PER_REAL_MIN	= 60;
constexpr int SECS_PER_REAL_HOUR	= (60*SECS_PER_REAL_MIN);
constexpr int SECS_PER_REAL_DAY	= (24*SECS_PER_REAL_HOUR);
constexpr int SECS_PER_REAL_YEAR	= (365*SECS_PER_REAL_DAY);