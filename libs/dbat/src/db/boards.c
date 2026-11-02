/* ************************************************************************
*   File: boards.c                                      Part of CircleMUD *
*  Usage: handling of multiple bulletin boards                            *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* Any ITEM_BOARD object is a board; its file etc/boards/<vnum> is created the
   first time it is looked at. The object's values give the read, write and
   remove levels. A poster can always remove their own messages. Messages
   cannot be removed while someone is still writing them. */

#include "dbat/boards.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/broadcast.h"
#include "dbat/config.h"
#include "dbat/xdir.h"
#include "dbat/fileop.h"
#include "dbat/stringutils.h"
#include "dbat/log.h"
#include "dbat/atomicfile.h"

#include <sys/stat.h>
#include <unistd.h>

struct board_info *bboards = NULL; /* our global board structure */

/* Boards whose new posts are announced, and whose last visit is kept in the player's lboard[]. */
static const struct special_board {
  obj_vnum vnum;
  int lboard;      /* index into char lboard[] */
  time_t *newest;  /* time of the newest post */
  bool admin_only; /* announce to admins only */
  const char *label;
} special_boards[] = {
    {3092, 0, &BOARDNEWMORT, false, "Mortal"    },
    {3098, 1, &BOARDNEWIMM,  true,  "Immortal"  },
    {3099, 3, &BOARDNEWDUO,  true,  "Punishment"},
    {3090, 4, &BOARDNEWBUI,  true,  "Builder"   },
};

static const struct special_board *find_special_board(obj_vnum vnum)
{
  for (size_t i = 0; i < sizeof(special_boards) / sizeof(special_boards[0]); i++)
    if (special_boards[i].vnum == vnum)
      return &special_boards[i];
  return NULL;
}

/* Records that ch has just looked at a special board. */
static void mark_board_visited(struct char_data *ch, obj_vnum vnum)
{
  const struct special_board *sb = find_special_board(vnum);

  if (sb)
    char_set_lboard(ch, sb->lboard, time(0));
}

/* Records a new post on a special board and tells everyone who hasn't seen it yet. */
static void announce_new_post(obj_vnum vnum)
{
  const struct special_board *sb = find_special_board(vnum);

  if (sb)
    *sb->newest = time(0);
  save_mud_time(&time_info);
  if (!sb)
    return;

  for (struct descriptor_data *d = descriptor_list; d; d = d->next) {
    if (!desc_is_playing(d) || char_plr_flagged(d->character, PLR_WRITING))
      continue;
    if (sb->admin_only && char_get_admlevel(d->character) < 1)
      continue;
    if (*sb->newest > char_get_lboard(d->character, sb->lboard))
      send_to_char(d->character, "\r\n@GThere is a new %s Board Post.@n\r\n", sb->label);
  }
}

void board_login_notices(struct char_data *ch)
{
  bool admin = char_get_admlevel(ch) >= 1;

  if (admin && BOARDNEWIMM > char_get_lboard(ch, 1))
    send_to_char(ch, "\r\n@GMake sure to check the immortal board, there is a new post there.@n\r\n");
  if (admin && BOARDNEWCOD > char_get_lboard(ch, 2))
    send_to_char(ch, "\r\n@GMake sure to check the request file, it has been updated.@n\r\n");
  if (admin && BOARDNEWBUI > char_get_lboard(ch, 4))
    send_to_char(ch, "\r\n@GMake sure to check the builder board, there is a new post there.@n\r\n");
  if (admin && BOARDNEWDUO > char_get_lboard(ch, 3))
    send_to_char(ch, "\r\n@GMake sure to check punishment board, there is a new post there.@n\r\n");
  if (BOARDNEWMORT > char_get_lboard(ch, 0))
    send_to_char(ch, "\r\n@GThere is a new bulletin board post.@n\r\n");
}

/* The prototype of a board's object, or NULL if it has none. */
static struct obj_data *board_proto(obj_vnum vnum)
{
  obj_rnum rnum = real_object(vnum);

  return rnum == NOTHING ? NULL : &obj_proto[rnum];
}

static void board_path(char *buf, size_t size, obj_vnum vnum)
{
  snprintf(buf, size, "%s%d", BOARD_DIRECTORY, vnum);
}

/* The memory[] bucket for a message. A deleted poster has id -1, which can make the sum negative. */
static int memory_slot(const struct board_msg *msg)
{
  int slot = (msg->timestamp % BOARD_MEMORY_SLOTS + get_id_by_name(msg->name) % BOARD_MEMORY_SLOTS) % BOARD_MEMORY_SLOTS;

  return slot < 0 ? slot + BOARD_MEMORY_SLOTS : slot;
}

/* True if a player's editor is currently writing into this message. */
static bool message_being_written(const struct board_msg *msg)
{
  for (struct descriptor_data *d = descriptor_list; d; d = d->next)
    if (!d->connected && d->str == &msg->data)
      return true;
  return false;
}

/* Saved messages: finished ones. Unfinished ones are written when their author saves them. */
static bool message_saveable(const struct board_msg *msg)
{
  return msg->data && !message_being_written(msg);
}

static void free_message(struct board_msg *msg)
{
  free(msg->name);
  free(msg->subject);
  free(msg->data);
  free(msg);
}

static void free_memory(struct board_memory *mem)
{
  free(mem->name);
  free(mem);
}

static void add_message_head(struct board_info *board, struct board_msg *msg)
{
  msg->prev = NULL;
  msg->next = board->messages;
  if (board->messages)
    board->messages->prev = msg;
  board->messages = msg;
}

static void add_message_tail(struct board_info *board, struct board_msg *msg)
{
  struct board_msg *last = board->messages;

  msg->next = NULL;
  if (!last) {
    msg->prev       = NULL;
    board->messages = msg;
    return;
  }
  while (last->next)
    last = last->next;
  last->next = msg;
  msg->prev  = last;
}

static void unlink_message(struct board_info *board, struct board_msg *msg)
{
  struct board_msg *temp;

  REMOVE_FROM_DOUBLE_LIST(msg, board->messages, next, prev);
}

/* The n'th message (1-based), counted from the newest, or from the oldest if from_oldest. */
static struct board_msg *nth_message(struct board_info *board, int n, bool from_oldest)
{
  struct board_msg *msg = board->messages;

  if (n < 1 || !msg)
    return NULL;
  if (from_oldest)
    while (msg->next)
      msg = msg->next;
  for (; msg && n != 1; n--)
    msg = from_oldest ? msg->prev : msg->next;
  return msg;
}

static struct board_msg *new_message(struct char_data *ch, const char *subject)
{
  struct board_msg *msg;

  CREATE(msg, struct board_msg, 1);
  msg->name      = strdup(char_get_name(ch));
  msg->timestamp = time(0);
  msg->subject   = strdup(subject);
  msg->data      = NULL;
  return msg;
}

/* asctime() without its trailing newline; points into asctime's static buffer. */
static char *board_time_str(const time_t *when)
{
  char *str = asctime(localtime(when));

  str[strlen(str) - 1] = '\0';
  return str;
}

static bool has_read(struct board_info *board, const struct board_msg *msg, struct char_data *ch)
{
  for (struct board_memory *mem = board->memory[memory_slot(msg)]; mem; mem = mem->next)
    if (!strcmp(mem->name, char_get_name(ch)) && mem->timestamp == msg->timestamp)
      return true;
  return false;
}

/* Records that ch has read msg. Returns true if that is new. */
static bool mark_read(struct board_info *board, const struct board_msg *msg, struct char_data *ch)
{
  struct board_memory *mem;
  int slot;

  if (has_read(board, msg, ch))
    return false;

  slot = memory_slot(msg);
  CREATE(mem, struct board_memory, 1);
  mem->name           = strdup(char_get_name(ch));
  mem->timestamp      = msg->timestamp;
  mem->next           = board->memory[slot];
  board->memory[slot] = mem;
  return true;
}

/* Reads "Board File" level line: read write remove count version. Missing fields get defaults. */
static void parse_header(const char *line, int t[5], obj_vnum vnum, const char *path)
{
  int found = sscanf(line, "%d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4);

  if (found == 5)
    return;
  if (found == 4) {
    mud_log("Parse error on board [vnum: %d], file '%s' - attempting to correct [4] args expecting 5.", vnum, path);
    t[4] = 1;
  } else {
    mud_log("Parse error on board [vnum: %d], file '%s' - attempting to correct [< 4] args expecting 5.", vnum, path);
    t[0] = t[1] = t[2] = CONFIG_LEVEL_CAP;
    t[3]               = -1;
    t[4]               = CURRENT_BOARD_VER;
  }
}

/* "S<slot> <reader> <timestamp>": a read record. Kept only if it still matches a loaded message,
   so messages must be loaded first. Records are dropped silently: deleted messages are normal. */
static void parse_memory_line(struct board_info *board, const char *line)
{
  char reader[128];
  int slot, timestamp;
  struct board_memory *mem;
  struct board_msg *msg;

  if (sscanf(line, "S %d %127s %d ", &slot, reader, &timestamp) != 3)
    return;
  if (slot < 0 || slot >= BOARD_MEMORY_SLOTS)
    return;

  for (msg = board->messages; msg; msg = msg->next)
    if (msg->timestamp == timestamp && memory_slot(msg) == slot)
      break;
  if (!msg)
    return;

  CREATE(mem, struct board_memory, 1);
  mem->name           = strdup(reader);
  mem->timestamp      = timestamp;
  mem->next           = board->memory[slot];
  board->memory[slot] = mem;
}

/* After a "#<n>" line: poster, timestamp, subject, then the text up to '~'. */
static bool parse_message(FILE *fl, struct board_info *board, const char *path)
{
  struct board_msg *msg;
  char poster[128], subject[READ_SIZE] = "";
  long timestamp;

  if (fscanf(fl, "%127s\n", poster) != 1 || fscanf(fl, "%ld\n", &timestamp) != 1) {
    mud_log("Parse error in message for board [vnum: %d].  Skipping.", board->vnum);
    return false;
  }
  get_line(fl, subject);

  CREATE(msg, struct board_msg, 1);
  msg->name      = strdup(poster);
  msg->timestamp = timestamp;
  msg->subject   = strdup(subject);
  msg->data      = fread_string(fl, path);
  add_message_tail(board, msg);
  return true;
}

/* see accompanying document for board file format */
static struct board_info *load_board(obj_vnum board_vnum)
{
  struct board_info *board;
  struct obj_data *obj;
  struct stat st;
  int t[5], msg_num = 0;
  char path[512], buf[READ_SIZE];
  FILE *fl;

  board_path(path, sizeof(path), board_vnum);
  if (!(fl = fopen(path, "r"))) {
    mud_log("Request to open board [vnum %d] failed - unable to open file '%s'.", board_vnum, path);
    return NULL;
  }
  if (!get_line(fl, buf) || strcmp("Board File", buf)) {
    mud_log("Invalid board file '%s' [vnum: %d] - failed to load.", path, board_vnum);
    fclose(fl);
    return NULL;
  }

  /* oddly enough, most errors in board files can be ignored, setting defaults */
  *buf = '\0';
  get_line(fl, buf);
  parse_header(buf, t, board_vnum, path);
  if (t[4] != CURRENT_BOARD_VER) {
    mud_log("SYSERR: Board file '%s' [vnum: %d] is format version %d; only version %d is supported. Not loaded.",
            path, board_vnum, t[4], CURRENT_BOARD_VER);
    fclose(fl);
    return NULL;
  }

  CREATE(board, struct board_info, 1);
  board->vnum = board_vnum;

  /* if the object exists, the object trumps the board file settings */
  if (!(obj = board_proto(board_vnum))) {
    mud_log("No associated object exists when attempting to create a board [vnum %d].", board_vnum);
    /* auto delete only if the file hasn't been modified in the last 7 days */
    if (!stat(path, &st) && time(NULL) - st.st_mtime > (60 * 60 * 24 * 7)) {
      mud_log("Deleting old board file '%s' [vnum %d].  7 days without modification & no associated object.", path, board_vnum);
      unlink(path);
      fclose(fl);
      free(board);
      return NULL;
    }
    board->read_lvl   = t[0];
    board->write_lvl  = t[1];
    board->remove_lvl = t[2];
    board->version    = t[4];
    mud_log("Board vnum %d, Version %d", board->vnum, board->version);
  } else {
    if (t[0] != obj_get_value(obj, VAL_BOARD_READ) ||
        t[1] != obj_get_value(obj, VAL_BOARD_WRITE) ||
        t[2] != obj_get_value(obj, VAL_BOARD_ERASE)) {
      mud_log("Mismatch in board <-> object read/write/remove settings for board [vnum: %d]. Correcting.", board_vnum);
    }
    board->read_lvl   = obj_get_value(obj, VAL_BOARD_READ);
    board->write_lvl  = obj_get_value(obj, VAL_BOARD_WRITE);
    board->remove_lvl = obj_get_value(obj, VAL_BOARD_ERASE);
    board->version    = t[4];
  }
  board->num_messages = t[3];

  while (get_line(fl, buf)) {
    if (*buf == 'S')
      parse_memory_line(board, buf);
    else if (*buf == '#' && parse_message(fl, board, path))
      msg_num++;
  }
  fclose(fl);

  if (msg_num != board->num_messages) {
    mud_log("Board [vnum: %d] message count (%d) not equal to actual message count (%d). Correcting.",
            board->vnum, board->num_messages, msg_num);
    board->num_messages = msg_num;
  }
  /* rewrite the file, which also repairs anything corrected above */
  save_board(board);
  return board;
}

static bool is_number_name(const char *name)
{
  if (!*name)
    return false;
  for (; *name; name++)
    if (!isdigit((unsigned char)*name))
      return false;
  return true;
}

void init_boards(void)
{
  struct xap_dir xd;
  struct board_info *board;
  int count;

  if (!insure_directory(BOARD_DIRECTORY)) {
    mud_log("Unable to open/create directory '%s' - Exiting", BOARD_DIRECTORY);
    exit(1);
  }

  if ((count = xdir_scan(BOARD_DIRECTORY, &xd)) <= 0) {
    mud_log("Funny, no board files found.\n");
    xdir_cleanup(&xd);
    return;
  }

  /* board files are named by vnum; skip ".", "..", ".cvsignore" and anything else */
  for (int i = 0; i < count; i++) {
    const char *name = xdir_get_name(&xd, i);

    if (!is_number_name(name))
      continue;
    if ((board = load_board(atoi(name))) != NULL) {
      board->next = bboards;
      bboards     = board;
    }
  }
  xdir_cleanup(&xd);

  int boards = 0, messages = 0;
  for (board = bboards; board; board = board->next, boards++)
    for (struct board_msg *msg = board->messages; msg; msg = msg->next)
      messages++;
  mud_log("There are %d boards located; %d messages", boards, messages);
}

/* Creates an empty board for an object that has no board file yet, and adds it to bboards. */
struct board_info *create_new_board(obj_vnum board_vnum)
{
  char path[512];
  FILE *fl;
  struct board_info *board;
  struct obj_data *obj;

  board_path(path, sizeof(path), board_vnum);
  if ((fl = fopen(path, "r"))) {
    fclose(fl);
    mud_log("Preexisting board file when attempting to create new board [vnum: %d]. Attempting to correct.", board_vnum);
    unlink(path);
  }

  CREATE(board, struct board_info, 1);
  if (!(obj = board_proto(board_vnum))) {
    mud_log("Creating board [vnum: %d] though no associated object with that vnum can be found. Using defaults.", board_vnum);
    board->read_lvl   = CONFIG_LEVEL_CAP;
    board->write_lvl  = CONFIG_LEVEL_CAP;
    board->remove_lvl = CONFIG_LEVEL_CAP;
  } else {
    board->read_lvl   = obj_get_value(obj, VAL_BOARD_READ);
    board->write_lvl  = obj_get_value(obj, VAL_BOARD_WRITE);
    board->remove_lvl = obj_get_value(obj, VAL_BOARD_ERASE);
  }
  board->vnum    = board_vnum;
  board->version = CURRENT_BOARD_VER;

  if (!save_board(board)) {
    mud_log("Hm. Error while creating new board file [vnum: %d]. Unable to create new file.", board_vnum);
    free(board);
    return NULL;
  }
  board->next = bboards;
  bboards     = board;
  return board;
}

/* Frees every board, at shutdown. */
void clear_boards(void)
{
  struct board_info *board, *next_board;
  struct board_msg *msg, *next_msg;
  struct board_memory *mem, *next_mem;

  for (board = bboards; board; board = next_board) {
    next_board = board->next;
    for (msg = board->messages; msg; msg = next_msg) {
      next_msg = msg->next;
      free_message(msg);
    }
    for (int slot = 0; slot < BOARD_MEMORY_SLOTS; slot++)
      for (mem = board->memory[slot]; mem; mem = next_mem) {
        next_mem = mem->next;
        free_memory(mem);
      }
    free(board);
  }
  bboards = NULL;
}

static struct board_info *locate_or_create_board(obj_vnum board_vnum)
{
  struct board_info *board = locate_board(board_vnum);

  if (!board) {
    mud_log("Creating new board - board #%d", board_vnum);
    board = create_new_board(board_vnum);
  }
  return board;
}

struct board_info *locate_board(obj_vnum board_vnum)
{
  for (struct board_info *board = bboards; board; board = board->next)
    if (board->vnum == board_vnum)
      return board;
  return NULL;
}

bool save_board(struct board_info *board)
{
  struct atomic_file af;
  struct board_msg *msg;
  FILE *fl;
  char path[512];
  int saved = 0, i = 1;

  if (!board)
    return false;

  board_path(path, sizeof(path), board->vnum);
  if (!(fl = atomic_open(&af, path, false))) {
    mud_log("Hm. Error while creating attempting to save board [vnum: %d].  Unable to create file '%s'", board->vnum, path);
    return false;
  }

  for (msg = board->messages; msg; msg = msg->next)
    if (message_saveable(msg))
      saved++;
  fprintf(fl, "Board File\n%d %d %d %d %d\n", board->read_lvl, board->write_lvl, board->remove_lvl, saved, CURRENT_BOARD_VER);

  for (msg = board->messages; msg; msg = msg->next)
    if (message_saveable(msg))
      fprintf(fl, "#%d\n%s\n%ld\n%s\n%s~\n", i++, msg->name, (long)msg->timestamp, msg->subject, msg->data);

  for (int slot = 0; slot < BOARD_MEMORY_SLOTS; slot++)
    for (struct board_memory *mem = board->memory[slot]; mem; mem = mem->next)
      fprintf(fl, "S%d %s %d\n", slot, mem->name, mem->timestamp);

  return atomic_commit(&af);
}

static void send_board_banner(struct char_data *ch, const struct obj_data *obj, obj_vnum board_vnum)
{
  if (!obj) {
    mud_log("SYSERR: DEFUNCT BOARD VNUM %d.", board_vnum);
    send_to_char(ch, "@W                  This is a bulletin board.\r\n");
    send_to_char(ch, "@rO@b============================================================================@rO@n\n");
    send_to_char(ch, "     @D[@GX@D] means you have read the message, @D[@RX@D] means you have not.\r\n"
                     "     @WUsage@D:@CREAD@D/@cREMOVE @D<@Wmessg #@D>@W, @CRESPOND @D<@Wmessg #@D>@W, @CWRITE @D<@Wheader@D>@W.@n\r\n"
                     "     @CVieworder@W, this changes the order in which posts are listed to you.@n\r\n");
    return;
  }
  send_to_char(ch, "@W                  This is the %20s\r\n", obj->short_description);
  send_to_char(ch, "@rO@b============================================================================@rO@n\n"
                   "     @D[@GX@D] means you have read the message, @D[@RX@D] means you have not.\r\n"
                   "     @WUsage@D:@CREAD@D/@cREMOVE @D<@Wmessg #@D>@W, @CRESPOND @D<@Wmessg #@D>@W, @CWRITE @D<@Wheader@D>@W.@n\r\n"
                   "     @CVieworder@W, this changes the order in which posts are listed to you.@n\r\n"
                   "     @D----------------------------------------------------------------\n");
}

void show_board(obj_vnum board_vnum, struct char_data *ch)
{
  struct board_info *board;
  struct obj_data *obj;
  bool oldest_first = char_prf_flagged(ch, PRF_VIEWORDER);
  char name[127];
  int count = 0;

  if (char_is_npc(ch)) {
    send_to_char(ch, "Gosh.. now .. if only mobs could read.. you'd be doing good.\r\n");
    return;
  }
  if (!(board = locate_or_create_board(board_vnum))) {
    send_to_char(ch, "Error: Your board could not be found. Please report.\n");
    return;
  }
  if (char_get_admlevel(ch) < board->read_lvl) {
    send_to_char(ch, "You try but fail to understand the holy words.\r\n");
    return;
  }

  obj = board_proto(board_vnum);
  send_board_banner(ch, obj, board_vnum);

  if (!board->num_messages || !board->messages) {
    send_to_char(ch, "                  @WThe board is empty.@n\r\n");
    return;
  }
  send_to_char(ch, "                  @WThere %s %d %s on the board.@n\r\n", (board->num_messages == 1) ? "is" : "are", board->num_messages, (board->num_messages == 1) ? "message" : "messages");

  for (struct board_msg *msg = nth_message(board, 1, oldest_first); msg; msg = oldest_first ? msg->prev : msg->next) {
    snprintf(name, sizeof(name), "%s", msg->name);
    send_to_char(ch, "@D[%s] (@C%2d@D) : @W%6.10s @D(@G%-10s@D) ::@w %-45s\r\n", has_read(board, msg, ch) ? "@GX@D" : "@RX@D", ++count, board_time_str(&msg->timestamp), CAP(name), msg->subject ? msg->subject : "No Subject");
  }
  send_to_char(ch, "@rO@b============================================================================@rO@n\n");

  if (obj)
    mark_board_visited(ch, board_vnum);
  save_char(ch);
}

void board_display_msg(obj_vnum board_vnum, struct char_data *ch, int arg)
{
  struct board_info *board;
  struct board_msg *msg;
  char name[127];
  bool added;

  if (char_is_npc(ch)) {
    send_to_char(ch, "Silly mob - reading is for pcs!\r\n");
    return;
  }
  if (!(board = locate_or_create_board(board_vnum))) {
    send_to_char(ch, "Error: Your board could not be found. Please report.\n");
    return;
  }
  if (char_get_admlevel(ch) < board->read_lvl) {
    send_to_char(ch, "You try but fail to understand the holy words.\r\n");
    return;
  }
  if (!board->messages) {
    send_to_char(ch, "The board is empty!\r\n");
    return;
  }
  if (!board_proto(board_vnum))
    send_to_imm("Error with %d board, object doesn't exist.", board_vnum);

  if (arg < 1) {
    send_to_char(ch, "You must specify the (positive) number of the message to be read!\r\n");
    return;
  }
  if (!(msg = nth_message(board, arg, char_prf_flagged(ch, PRF_VIEWORDER)))) {
    send_to_char(ch, "That message exists only in your imagination.\r\n");
    return;
  }

  added = mark_read(board, msg, ch);

  snprintf(name, sizeof(name), "%s", msg->name);
  send_to_char(ch, "@r_____________________________________________________________________________@n\r\n\r\n@gMessage @W[@Y%2d@W] @D: @c%6.10s @D(@C%s@D)\r\n@GTopic        @D: @w%s\r\n@r-------------------------------------------------------------------------@n\r\n\r\n%s\n@r_____________________________________________________________________________@n\r\n",
               arg,
               board_time_str(&msg->timestamp),
               CAP(name),
               msg->subject ? msg->subject : "No Subject",
               msg->data ? msg->data : "Looks like this message is empty.");

  if (board_proto(board_vnum))
    mark_board_visited(ch, board_vnum);
  /* really it's not so important to save after each view even if something WAS updated */
  if (added)
    save_board(board);
}

/* Shared by write and respond: puts msg on the board and opens the editor on its text. */
static void start_post(struct board_info *board, struct board_msg *msg, struct char_data *ch)
{
  board->num_messages = MAX(board->num_messages + 1, 1);
  add_message_head(board, msg);
  char_plr_set(ch, PLR_WRITING, true);
  string_write(ch->desc, &msg->data, MAX_MESSAGE_LENGTH, board->vnum + BOARD_MAGIC, NULL);
  announce_new_post(board->vnum);
}

void write_board_message(obj_vnum board_vnum, struct char_data *ch, char *arg)
{
  struct board_info *board;

  if (char_is_npc(ch)) {
    send_to_char(ch, "Orwellian police thwart your attempt at free speech.\r\n");
    return;
  }
  if (!ch->desc)
    return;
  if (!(board = locate_board(board_vnum))) {
    send_to_char(ch, "Error: Your board could not be found. Please report.\n");
    mud_log("Error in write_board_msg - board #%d", board_vnum);
    return;
  }
  if (char_get_admlevel(ch) < board->write_lvl) {
    send_to_char(ch, "You are not holy enough to write on this board.\r\n");
    return;
  }

  skip_spaces(&arg);
  if (strlen(arg) > 45) {
    send_to_char(ch, "Your subject can only be 45 characters long(including colorcode).\r\n");
    return;
  }
  act("@C$n@w starts writing on the board.@n", TRUE, ch, 0, 0, TO_ROOM);
  delete_doubledollar(arg);

  send_to_char(ch, "Write your message.  (/s saves /h for help)\r\n");
  start_post(board, new_message(ch, *arg ? arg : "No Subject"), ch);
}

void board_respond(long board_vnum, struct char_data *ch, int mnum)
{
  struct board_info *board;
  struct board_msg *msg, *other;
  char subject[READ_SIZE - 1], quote[MAX_MESSAGE_LENGTH + 128];

  if (!ch->desc)
    return;
  if (!(board = locate_board(board_vnum))) {
    send_to_char(ch, "Error: Your board could not be found. Please report.\n");
    mud_log("Error in board_respond - board #%ld", board_vnum);
    return;
  }
  if (char_get_admlevel(ch) < board->write_lvl) {
    send_to_char(ch, "You are not holy enough to write on this board.\r\n");
    return;
  }
  if (char_get_admlevel(ch) < board->read_lvl) {
    send_to_char(ch, "You are not holy enough to respond to this board.\r\n");
    return;
  }
  if (char_prf_flagged(ch, PRF_VIEWORDER))
    mnum = (board->num_messages - mnum) + 1;
  if (mnum < 1 || mnum > board->num_messages || !(other = nth_message(board, mnum, false))) {
    send_to_char(ch, "You can only respond to an actual message.\r\n");
    return;
  }

  /* a subject longer than a board file line can hold would not load back */
  snprintf(subject, sizeof(subject), "Re: %s", other->subject);
  msg = new_message(ch, subject);

  send_to_char(ch, "Write your message.  (/s saves /h for help)\r\n\r\n");
  act("@C$n@w starts writing on the board.@n", TRUE, ch, 0, 0, TO_ROOM);

  snprintf(quote, sizeof(quote), "\t@D------- @cQuoted message @D-------@w\r\n%s\t@D   ------- @cEnd Quote @D-------@w\r\n", other->data ? other->data : "");
  msg->data = strdup(quote);
  write_to_output(ch->desc, "%s", quote);
  start_post(board, msg, ch);
}

/* Called when the author aborts: removes the message whose text is being edited at data. */
bool board_discard_message(struct board_info *board, char **data)
{
  if (!board)
    return false;
  for (struct board_msg *msg = board->messages; msg; msg = msg->next) {
    if (&msg->data != data)
      continue;
    unlink_message(board, msg);
    free_message(msg);
    board->num_messages--;
    return true;
  }
  return false;
}

void remove_board_msg(obj_vnum board_vnum, struct char_data *ch, int arg)
{
  struct board_info *board;
  struct board_msg *msg;
  struct obj_data *obj;
  char buf[64];

  if (char_is_npc(ch)) {
    send_to_char(ch, "Nuts.. looks like you forgot your eraser back in mobland...\r\n");
    return;
  }
  if (!(board = locate_board(board_vnum))) {
    send_to_char(ch, "Error: Your board could not be found. Please report.\n");
    mud_log("Error in Board_remove_msg - board #%d", board_vnum);
    return;
  }
  if (arg < 1) {
    send_to_char(ch, "You must specify the (positive) number of the message to be read!\r\n");
    return;
  }
  if (char_prf_flagged(ch, PRF_VIEWORDER))
    arg = board->num_messages - arg + 1;
  if (!(msg = nth_message(board, arg, false))) {
    send_to_char(ch, "That message exists only in your imagination.\r\n");
    return;
  }
  if (!(obj = board_proto(board_vnum))) {
    mud_log("Board doesn't exists! Weird.");
    return;
  }
  if (!obj_extra_flagged(obj, ITEM_CBOARD) && char_get_admlevel(ch) < board->remove_lvl && strcmp(char_get_name(ch), msg->name)) {
    send_to_char(ch, "You can't remove other people's messages.\r\n");
    return;
  }
  if (message_being_written(msg)) {
    send_to_char(ch, "At least wait until the author is finished before removing it!\r\n");
    return;
  }

  unlink_message(board, msg);
  free_message(msg);
  board->num_messages--;
  send_to_char(ch, "Message removed.\r\n");
  snprintf(buf, sizeof(buf), "$n just removed message %d.", arg);
  act(buf, FALSE, ch, 0, 0, TO_ROOM);
  save_board(board);
}
