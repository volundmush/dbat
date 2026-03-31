#pragma once
#include "consts/types.h"

struct board_msg {
  long poster;
  time_t timestamp;
  char *subject;
  char *data;
  struct board_msg *next;
  struct board_msg *prev;
  char *name;
};

/* Defines what we require to generate a hash for lookup
   of a message given a reader */

struct board_memory {
  int timestamp;
  int reader;
  struct board_memory *next;
  char *name;
};

struct board_info {
   int	read_lvl;	/* min level to read messages on this board */
   int	write_lvl;	/* min level to write messages on this board */
   int	remove_lvl;	/* min level to remove messages from this board */
  int  num_messages;           /* num messages of this board */
  int  vnum;
  struct board_info *next;
  struct board_msg *messages;
  int  version;
  
  /* why 301? why not?  It might not be the greatest, but if you really
     know what a hash is, you'll realize that in this case, I didn't even
     work on the algorithm, so it shouldn't make a bit of difference */
  
  struct board_memory *memory[301];
};

extern struct board_info *bboards;