#include "dbat/db/consts/directions.h"

/* cardinal directions */
const char *dirs[NUM_OF_DIRS+1] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northwest",
  "northeast",
  "southeast",
  "southwest",
  "inside",
  "outside",
  "\n"
};
const char *abbr_dirs[NUM_OF_DIRS+1] = 
{
  "n",
  "e",
  "s",
  "w",
  "u",
  "d",
  "nw",
  "ne",
  "se",
  "sw",
  "in",
  "out",
  "\n"
};

int rev_dir[NUM_OF_DIRS] =
{
  /* North */ SOUTH,
  /* East  */ WEST,
  /* South */ NORTH,
  /* West  */ EAST,
  /* Up    */ DOWN,
  /* Down  */ UP,
  /* NW    */ SOUTHEAST,
  /* NE    */ SOUTHWEST,
  /* SE    */ NORTHWEST,
  /* SW    */ NORTHEAST,
  /* In    */ OUTDIR,
  /* Out   */ INDIR
};