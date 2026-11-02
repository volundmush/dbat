#pragma once
#include "consts/types.h"

/* New Dynamic boards v2.4 -PjD (dughi@imaxx.net) */
#define BOARD_DIRECTORY "etc/boards" SLASH
constexpr int MAX_MESSAGE_LENGTH = 4096;    /* arbitrary -- change if needed */
constexpr int BOARD_MAGIC        = 1048575; /* added to a board vnum in descriptor mail_to; see modify.c */
constexpr int CURRENT_BOARD_VER  = 2;       /* the only board file format still supported */
constexpr int BOARD_MEMORY_SLOTS = 301;     /* buckets of "who has read what" records */

/* One message. Doubly linked, newest first, so forward or back is simple. */
struct board_msg {
  time_t timestamp;
  char *subject;
  char *data;
  struct board_msg *next;
  struct board_msg *prev;
  char *name; /* poster */
};

/* One "reader has read the message with this timestamp" record, hashed by message. */
struct board_memory {
  int timestamp;
  struct board_memory *next;
  char *name; /* reader */
};

struct board_info {
  int read_lvl;     /* min level to read messages on this board */
  int write_lvl;    /* min level to write messages on this board */
  int remove_lvl;   /* min level to remove messages from this board */
  int num_messages; /* num messages of this board */
  int vnum;
  struct board_info *next;
  struct board_msg *messages;
  int version;
  struct board_memory *memory[BOARD_MEMORY_SLOTS];
};

extern struct board_info *bboards;

void init_boards(void);
struct board_info *create_new_board(obj_vnum board_vnum);
struct board_info *locate_board(obj_vnum board_vnum);
bool save_board(struct board_info *board);
void clear_boards(void);

void show_board(obj_vnum board_vnum, struct char_data *ch);
void board_display_msg(obj_vnum board_vnum, struct char_data *ch, int arg);
void write_board_message(obj_vnum board_vnum, struct char_data *ch, char *arg);
void board_respond(long board_vnum, struct char_data *ch, int mnum);
void remove_board_msg(obj_vnum board_vnum, struct char_data *ch, int arg);

bool board_discard_message(struct board_info *board, char **data);
void board_login_notices(struct char_data *ch);
