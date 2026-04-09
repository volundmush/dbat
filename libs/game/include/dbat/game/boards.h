#pragma once
#include "dbat/db/consts/types.h"

/* New Dynamic boards v2.4 -PjD (dughi@imaxx.net) */
#define BOARD_DIRECTORY                "etc/boards" SLASH
#define MAX_MESSAGE_LENGTH	4096	/* arbitrary -- change if needed */

#define BOARD_MAGIC	1048575	/* arbitrary number - see modify.c */

/* Provides individual message structure */
/* doubly linked so forward or back is relatively simple */


#define READ_LVL(i) (i->read_lvl)
#define WRITE_LVL(i) (i->write_lvl)
#define REMOVE_LVL(i) (i->remove_lvl)
#define BOARD_MNUM(i) (i->num_messages)
#define BOARD_VNUM(i) (i->vnum)
#define BOARD_NEXT(i) (i->next)
#define BOARD_MESSAGES(i) (i->messages)
#define BOARD_MEMORY(i,j) (i->memory[j])
#define BOARD_VERSION(i) 	 (i->version)
#define CURRENT_BOARD_VER	 2

#define MESG_POSTER(i) (i->poster)
#define MESG_TIMESTAMP(i) (i->timestamp)
#define MESG_SUBJECT(i) (i->subject)
#define MESG_DATA(i) (i->data)
#define MESG_NEXT(i) (i->next)
#define MESG_PREV(i) (i->prev)
#define MESG_POSTER_NAME(i)	 (i->name)

#define MEMORY_TIMESTAMP(i) (i->timestamp)
#define MEMORY_READER(i) (i->reader)
#define MEMORY_NEXT(i) (i->next)
#define MEMORY_READER_NAME(i)	 (i->name)

void init_boards(void);
struct board_info *create_new_board(obj_vnum board_vnum);
struct board_info *load_board(obj_vnum board_vnum);
int save_board(struct board_info *temp_board);
void clear_boards();
void clear_one_board(struct board_info *temp_board);
int parse_message( FILE *fl, struct board_info *temp_board);
void look_at_boards(void);
void show_board(obj_vnum board_vnum, struct char_data *ch);
void board_display_msg(obj_vnum board_vnum, struct char_data * ch, int arg);
int mesglookup(struct board_msg *message,struct char_data *ch,
	       struct board_info *board);

void write_board_message(obj_vnum board_vnum, struct char_data *ch, char *arg);
void board_respond(long board_vnum, struct char_data *ch, int mnum);

struct board_info *locate_board(obj_vnum board_vnum);

void remove_board_msg(obj_vnum board_vnum, struct char_data * ch, int arg);

extern struct board_info *bboards;
