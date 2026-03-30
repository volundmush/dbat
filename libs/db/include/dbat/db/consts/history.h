#pragma once
#define HIST_ALL       0
#define HIST_SAY       1
#define HIST_GOSSIP    2
#define HIST_WIZNET    3
#define HIST_TELL      4
#define HIST_SHOUT     5
#define HIST_GRATS     6
#define HIST_HOLLER    7
#define HIST_AUCTION   8
#define HIST_SNET      9

#define NUM_HIST      10

extern const char *history_types[NUM_HIST+1];