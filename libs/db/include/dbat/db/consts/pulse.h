#pragma once
/*
 * OPT_USEC determines how many commands will be processed by the MUD per
 * second and how frequently it does socket I/O.  A low setting will cause
 * actions to be executed more frequently but will increase overhead due to
 * more cycling to check.  A high setting (e.g. 1 Hz) may upset your players
 * as actions (such as large speedwalking chains) take longer to be executed.
 * You shouldn't need to adjust this.
 */
#define OPT_USEC	100000		/* 10 passes per second */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
#define RL_SEC		* PASSES_PER_SEC
#define CD_TICK         * PASSES_PER_SEC

#define PULSE_ZONE	(CONFIG_PULSE_ZONE RL_SEC)
#define PULSE_MOBILE    (CONFIG_PULSE_MOBILE RL_SEC)
#define PULSE_VIOLENCE  (CONFIG_PULSE_VIOLENCE RL_SEC)
#define PULSE_AUCTION	(15 RL_SEC)
#define PULSE_AUTOSAVE	(CONFIG_PULSE_AUTOSAVE RL_SEC)
#define PULSE_IDLEPWD	(CONFIG_PULSE_IDLEPWD RL_SEC)
#define PULSE_SANITY	(CONFIG_PULSE_SANITY RL_SEC)
#define PULSE_USAGE	(CONFIG_PULSE_SANITY * 300 RL_SEC)   /* 5 mins */
#define PULSE_TIMESAVE	(CONFIG_PULSE_TIMESAVE * 900 RL_SEC) /* should be >= SECS_PER_MUD_HOUR */
#define PULSE_CURRENT	(CONFIG_PULSE_CURRENT RL_SEC)
#define PULSE_1SEC      (1 RL_SEC)
#define PULSE_2SEC      (2 RL_SEC)
#define PULSE_3SEC      (3 RL_SEC)
#define PULSE_4SEC      (4 RL_SEC)
#define PULSE_5SEC      (5 RL_SEC)
#define PULSE_6SEC      (6 RL_SEC)
#define PULSE_7SEC      (7 RL_SEC)

/* Cool Down Ticks */
#define PULSE_CD1       (1 CD_TICK)
#define PULSE_CD2       (2 CD_TICK)
#define PULSE_CD3       (3 CD_TICK)
#define PULSE_CD4       (4 CD_TICK) /* This and the 3 above are for safety */
#define PULSE_CD5       (5 CD_TICK) /* Punch */
#define PULSE_CD6       (6 CD_TICK)
#define PULSE_CD7       (7 CD_TICK)
#define PULSE_CD8       (8 CD_TICK)
#define PULSE_CD9       (9 CD_TICK)
#define PULSE_CD10      (10 CD_TICK)
#define PULSE_CD11      (11 CD_TICK)
#define PULSE_CD12      (12 CD_TICK)
/* End CD Ticks    */