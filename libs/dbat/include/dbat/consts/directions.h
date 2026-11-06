#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* The cardinal directions: used as index to room_data.dir_option[] */
constexpr int NORTH     = 0;
constexpr int EAST      = 1;
constexpr int SOUTH     = 2;
constexpr int WEST      = 3;
constexpr int UP        = 4;
constexpr int DOWN      = 5;
constexpr int NORTHWEST = 6;
constexpr int NORTHEAST = 7;
constexpr int SOUTHEAST = 8;
constexpr int SOUTHWEST = 9;
constexpr int INDIR     = 10;
constexpr int OUTDIR    = 11;

constexpr int NUM_OF_DIRS = 12; /* number of directions in a room (nsewud) */

extern const char *const dirs[NUM_OF_DIRS + 1];
extern const char *const abbr_dirs[NUM_OF_DIRS + 1];
extern const int rev_dir[NUM_OF_DIRS];

#ifdef __cplusplus
}
#endif
