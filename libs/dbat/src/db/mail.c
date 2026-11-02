/* ************************************************************************
*   File: mail.c                                        Part of CircleMUD *
*  Usage: Internal funcs and player spec-procs of mud-mail system         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/******* MUD MAIL SYSTEM MAIN FILE ***************************************

Written by Jeremy Elson (jelson@circlemud.org)

*************************************************************************/
#include "dbat/mail.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/interpreter.h"
#include "dbat/handler.h"
#include "dbat/improved-edit.h"
#include "dbat/players.h"
#include "dbat/fileop.h"
#include "dbat/atomicfile.h"

/* local globals */
static mail_index_type *mail_index   = NULL; /* list of recs in the mail file  */
static position_list_type *free_list = NULL; /* list of free positions in file */
static long file_end_pos             = 0;    /* length of file */

/* local functions */
static void postmaster_send_mail(struct char_data *ch, struct char_data *mailman, int cmd, char *arg);
static void postmaster_check_mail(struct char_data *ch, struct char_data *mailman, int cmd, char *arg);
static void postmaster_receive_mail(struct char_data *ch, struct char_data *mailman, int cmd, char *arg);
static void push_free_list(long pos);
static long pop_free_list(void);
static mail_index_type *find_char_in_index(long searchee);
static void write_to_file(void *buf, int size, long filepos);
static void read_from_file(void *buf, int size, long filepos);
static void index_mail(long id_to_index, long pos);
static int mail_recip_ok(const char *name);

/* -------------------------------------------------------------------------- */

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

void free_mail_index(void)
{
  mail_index_type *tmp;

  while (mail_index) {
    tmp = mail_index->next;
    if (mail_index->list_start) {
      position_list_type *i, *j;
      i = mail_index->list_start;
      while (i) {
        j = i->next;
        free(i);
        i = j;
      }
    }
    free(mail_index);
    mail_index = tmp;
  }
}

static int mail_recip_ok(const char *name)
{
  int player_i, ret = FALSE;

  if ((player_i = get_ptable_by_name(name)) >= 0) {
    if (!IS_SET(player_table[player_i].flags, PINDEX_DELETED))
      ret = TRUE;
  }
  return ret;
}

/*
 * void push_free_list(long #1)
 * #1 - What int8_toffset into the file the block resides.
 *
 * Net effect is to store a list of free blocks in the mail file in a linked
 * list.  This is called when people receive their messages and at startup
 * when the list is created.
 */
static void push_free_list(long pos)
{
  position_list_type *new_pos;

  CREATE(new_pos, position_list_type, 1);
  new_pos->position = pos;
  new_pos->next     = free_list;
  free_list         = new_pos;
}

/*
 * long pop_free_list(none)
 * Returns the offset of a free block in the mail file.
 *
 * Typically used whenever a person mails a message.  The blocks are not
 * guaranteed to be sequential or in any order at all.
 */
static long pop_free_list(void)
{
  position_list_type *old_pos;
  long return_value;

  /*
   * If we don't have any free blocks, we append to the file.
   */
  if ((old_pos = free_list) == NULL)
    return (file_end_pos);

  /* Save the offset of the free block. */
  return_value = free_list->position;
  /* Remove this block from the free list. */
  free_list = old_pos->next;
  /* Get rid of the memory the node took. */
  free(old_pos);
  /* Give back the free offset. */
  return (return_value);
}

void clear_free_list(void)
{
  while (free_list)
    pop_free_list();
}

/*
 * main_index_type *find_char_in_index(long #1)
 * #1 - The idnum of the person to look for.
 * Returns a pointer to the mail block found.
 *
 * Finds the first mail block for a specific person based on id number.
 */
static mail_index_type *find_char_in_index(long searchee)
{
  mail_index_type *tmp;

  if (searchee < 0) {
    mud_log("SYSERR: Mail system -- non fatal error #1 (searchee == %ld).", searchee);
    return (NULL);
  }
  for (tmp = mail_index; (tmp && tmp->recipient != searchee); tmp = tmp->next);

  return (tmp);
}

/*
 * Mail operations are copy-on-write: mail_begin() reads the whole mail file into memory, the
 * block reads and writes below work on that copy, and mail_commit() writes it back through
 * atomic_open(), so a crash leaves either the old mail file or the new one, never a block chain
 * that is half rewritten. The file is small (BLOCK_SIZE blocks, a few KB).
 */
static char *mail_buf      = NULL; /* the mail file while an operation is open */
static long mail_buf_len   = 0;
static long mail_buf_alloc = 0;

static bool mail_begin(void)
{
  FILE *fl;

  if (!(fl = fopen(MAIL_FILE, "rb"))) {
    mud_log("SYSERR: Unable to open mail file '%s'.", MAIL_FILE);
    no_mail = TRUE;
    return false;
  }
  fseek(fl, 0L, SEEK_END);
  mail_buf_len   = ftell(fl);
  mail_buf_alloc = mail_buf_len + 8 * BLOCK_SIZE;
  CREATE(mail_buf, char, mail_buf_alloc);
  rewind(fl);
  if (mail_buf_len && fread(mail_buf, mail_buf_len, 1, fl) != 1) {
    mud_log("SYSERR: Unable to read mail file '%s'.", MAIL_FILE);
    fclose(fl);
    free(mail_buf);
    mail_buf = NULL;
    no_mail  = TRUE;
    return false;
  }
  fclose(fl);
  return true;
}

/* Ends a mail operation without writing anything. */
static void mail_end(void)
{
  free(mail_buf);
  mail_buf     = NULL;
  mail_buf_len = mail_buf_alloc = 0;
}

static void mail_commit(void)
{
  struct atomic_file af;
  FILE *fl;
  bool written = false;

  if ((fl = atomic_open(&af, MAIL_FILE, false))) {
    if (mail_buf_len && fwrite(mail_buf, mail_buf_len, 1, fl) != 1)
      atomic_abort(&af);
    else
      written = atomic_commit(&af);
  }
  if (!written) { /* the in-memory index no longer matches the file */
    mud_log("SYSERR: Mail system disabled! Unable to write mail file '%s'.", MAIL_FILE);
    no_mail = TRUE;
  }
  file_end_pos = mail_buf_len;
  mail_end();
}

/*
 * void write_to_file(void * #1, int #2, long #3)
 * #1 - A pointer to the data to write, usually the 'block' record.
 * #2 - How much to write (because we'll write NUL terminated strings.)
 * #3 - What offset (block position) in the file to write to.
 *
 * Writes a mail block into the open mail operation (see mail_begin()).
 */
static void write_to_file(void *buf, int size, long filepos)
{
  if (filepos % BLOCK_SIZE) {
    mud_log("SYSERR: Mail system -- fatal error #2!!! (invalid file position %ld)", filepos);
    no_mail = TRUE;
    return;
  }
  if (!mail_buf) {
    mud_log("SYSERR: Mail system -- write outside a mail operation (position %ld)", filepos);
    no_mail = TRUE;
    return;
  }
  if (filepos + size > mail_buf_alloc) {
    long old_alloc = mail_buf_alloc;

    mail_buf_alloc = filepos + size + 8 * BLOCK_SIZE;
    RECREATE(mail_buf, char, mail_buf_alloc);
    memset(mail_buf + old_alloc, 0, mail_buf_alloc - old_alloc);
  }
  if (filepos > mail_buf_len) /* as writing past the end of a file would: the gap reads as zeros */
    memset(mail_buf + mail_buf_len, 0, filepos - mail_buf_len);
  memcpy(mail_buf + filepos, buf, size);
  mail_buf_len = MAX(mail_buf_len, filepos + size);

  /* new blocks are appended at the end (see pop_free_list()) */
  file_end_pos = mail_buf_len;
}

/*
 * void read_from_file(void * #1, int #2, long #3)
 * #1 - A pointer to where we should store the data read.
 * #2 - How large the block we're reading is.
 * #3 - What position in the file to read.
 *
 * This reads a block from the open mail operation (see mail_begin()).
 */
static void read_from_file(void *buf, int size, long filepos)
{
  if (filepos % BLOCK_SIZE) {
    mud_log("SYSERR: Mail system -- fatal error #3!!! (invalid filepos read %ld)", filepos);
    no_mail = TRUE;
    return;
  }
  if (!mail_buf) {
    mud_log("SYSERR: Mail system -- read outside a mail operation (position %ld)", filepos);
    no_mail = TRUE;
    return;
  }
  memset(buf, 0, size);
  if (filepos < mail_buf_len)
    memcpy(buf, mail_buf + filepos, MIN(size, mail_buf_len - filepos));
}

static void index_mail(long id_to_index, long pos)
{
  mail_index_type *new_index;
  position_list_type *new_position;

  if (id_to_index < 0) {
    mud_log("SYSERR: Mail system -- non-fatal error #4. (id_to_index == %ld)", id_to_index);
    return;
  }
  if (!(new_index = find_char_in_index(id_to_index))) {
    /* name not already in index.. add it */
    CREATE(new_index, mail_index_type, 1);
    new_index->recipient  = id_to_index;
    new_index->list_start = NULL;

    /* add to front of list */
    new_index->next = mail_index;
    mail_index      = new_index;
  }
  /* now, add this position to front of position list */
  CREATE(new_position, position_list_type, 1);
  new_position->position = pos;
  new_position->next     = new_index->list_start;
  new_index->list_start  = new_position;
}

/*
 * int scan_file(none)
 * Returns false if mail file is corrupted or true if everything correct.
 *
 * This is called once during boot-up.  It scans through the mail file
 * and indexes all entries currently in the mail file.
 */
int scan_file(void)
{
  FILE *mail_file;
  header_block_type next_block;
  int total_messages = 0, block_num = 0;

  if (!(mail_file = fopen(MAIL_FILE, "rb"))) {
    mud_log("   Mail file non-existant... creating new file.");
    touch(MAIL_FILE);
    return (1);
  }
  while (fread(&next_block, sizeof(header_block_type), 1, mail_file)) {
    if (next_block.block_type == HEADER_BLOCK) {
      index_mail(next_block.header_data.to, block_num * BLOCK_SIZE);
      total_messages++;
    } else if (next_block.block_type == DELETED_BLOCK) {
      push_free_list(block_num * BLOCK_SIZE);
    }
    /* Anything else is a body block of a stored message (its type is the next block's position or LAST_BLOCK).
     * It must not go on the free list, or new mail would overwrite messages that haven't been read yet. */
    block_num++;
  }

  file_end_pos = ftell(mail_file);
  fclose(mail_file);
  mud_log("   %ld bytes read.", file_end_pos);
  if (file_end_pos % BLOCK_SIZE) {
    mud_log("SYSERR: Error booting mail system -- Mail file corrupt!");
    mud_log("SYSERR: Mail disabled!");
    return (0);
  }
  mud_log("   Mail file read -- %d messages.", total_messages);
  return (1);
} /* end of scan_file */

/*
 * int has_mail(long #1)
 * #1 - id number of the person to check for mail.
 * Returns true or false.
 *
 * A simple little function which tells you if the guy has mail or not.
 */
int has_mail(long recipient)
{
  return (find_char_in_index(recipient) != NULL);
}

/*
 * void store_mail(long #1, long #2, char * #3)
 * #1 - id number of the person to mail to.
 * #2 - id number of the person the mail is from.
 * #3 - The actual message to send.
 *
 * call store_mail to store mail.  (hard, huh? :-) )  Pass 3 arguments:
 * who the mail is to (long), who it's from (long), and a pointer to the
 * actual message text (char *).
 */
void store_mail(long to, long from, char *message_pointer)
{
  header_block_type header;
  data_block_type data;
  long last_address, target_address;
  char *msg_txt = message_pointer;
  int bytes_written, total_length = strlen(message_pointer);

  if ((sizeof(header_block_type) != sizeof(data_block_type)) ||
      (sizeof(header_block_type) != BLOCK_SIZE)) {
    core_dump();
    return;
  }

  if ((from < 0 && from != -1337) || to < 0 || !*message_pointer) {
    mud_log("SYSERR: Mail system -- non-fatal error #5. (from == %ld, to == %ld)", from, to);
    return;
  }
  memset((char *)&header, 0, sizeof(header)); /* clear the record */
  header.block_type             = HEADER_BLOCK;
  header.header_data.next_block = LAST_BLOCK;
  header.header_data.from       = from;
  header.header_data.to         = to;
  header.header_data.mail_time  = time(0);
  strncpy(header.txt, msg_txt, HEADER_BLOCK_DATASIZE); /* strncpy: OK (h.txt:HEADER_BLOCK_DATASIZE+1) */
  header.txt[HEADER_BLOCK_DATASIZE] = '\0';

  if (!mail_begin())
    return;
  target_address = pop_free_list(); /* find next free block */
  index_mail(to, target_address);   /* add it to mail index in memory */
  write_to_file(&header, BLOCK_SIZE, target_address);

  if (strlen(msg_txt) <= HEADER_BLOCK_DATASIZE) {
    mail_commit();
    return; /* that was the whole message */
  }

  bytes_written = HEADER_BLOCK_DATASIZE;
  msg_txt += HEADER_BLOCK_DATASIZE; /* move pointer to next bit of text */

  /*
   * find the next block address, then rewrite the header to reflect where
   * the next block is.
   */
  last_address                  = target_address;
  target_address                = pop_free_list();
  header.header_data.next_block = target_address;
  write_to_file(&header, BLOCK_SIZE, last_address);

  /* now write the current data block */
  memset((char *)&data, 0, sizeof(data)); /* clear the record */
  data.block_type = LAST_BLOCK;
  strncpy(data.txt, msg_txt, DATA_BLOCK_DATASIZE); /* strncpy: OK (d.txt:DATA_BLOCK_DATASIZE+1) */
  data.txt[DATA_BLOCK_DATASIZE] = '\0';
  write_to_file(&data, BLOCK_SIZE, target_address);
  bytes_written += strlen(data.txt);
  msg_txt += strlen(data.txt);

  /*
   * if, after 1 header block and 1 data block there is STILL part of the
   * message left to write to the file, keep writing the new data blocks and
   * rewriting the old data blocks to reflect where the next block is.  Yes,
   * this is kind of a hack, but if the block size is big enough it won't
   * matter anyway.  Hopefully, MUD players won't pour their life stories out
   * into the Mud Mail System anyway.
   * 
   * Note that the block_type data field in data blocks is either a number >=0,
   * meaning a link to the next block, or LAST_BLOCK flag (-2) meaning the
   * last block in the current message.  This works much like DOS' FAT.
   */
  while (bytes_written < total_length) {
    last_address   = target_address;
    target_address = pop_free_list();

    /* rewrite the previous block to link it to the next */
    data.block_type = target_address;
    write_to_file(&data, BLOCK_SIZE, last_address);

    /* now write the next block, assuming it's the last.  */
    data.block_type = LAST_BLOCK;
    strncpy(data.txt, msg_txt, DATA_BLOCK_DATASIZE); /* strncpy: OK (d.txt:DATA_BLOCK_DATASIZE+1) */
    data.txt[DATA_BLOCK_DATASIZE] = '\0';
    write_to_file(&data, BLOCK_SIZE, target_address);

    bytes_written += strlen(data.txt);
    msg_txt += strlen(data.txt);
  }
  mail_commit();
} /* store mail */

/*
 * char *read_delete(long #1)
 * #1 - The id number of the person we're checking mail for.
 * Returns the message text of the mail received.
 *
 * Retrieves one messsage for a player. The mail is then discarded from
 * the file and the mail index.
 */
static char *read_delete(long recipient, char **from)
{
  header_block_type header;
  data_block_type data;
  mail_index_type *mail_pointer, *prev_mail;
  position_list_type *position_pointer;
  long mail_address, following_block;
  char *tmstr, buf[MAX_MAIL_SIZE + 256]; /* + header */
  char *to;

  if (recipient < 0) {
    mud_log("SYSERR: Mail system -- non-fatal error #6. (recipient: %ld)", recipient);
    return (NULL);
  }
  if (!(mail_pointer = find_char_in_index(recipient))) {
    mud_log("SYSERR: Mail system -- post office spec_proc error?  Error #7. (invalid character in index)");
    return (NULL);
  }
  if (!(position_pointer = mail_pointer->list_start)) {
    mud_log("SYSERR: Mail system -- non-fatal error #8. (invalid position pointer %p)", position_pointer);
    return (NULL);
  }
  if (!(position_pointer->next)) { /* just 1 entry in list. */
    mail_address = position_pointer->position;
    free(position_pointer);

    /* now free up the actual name entry */
    if (mail_index == mail_pointer) { /* name is 1st in list */
      mail_index = mail_pointer->next;
      free(mail_pointer);
    } else {
      /* find entry before the one we're going to del */
      for (prev_mail = mail_index;
           prev_mail->next != mail_pointer;
           prev_mail = prev_mail->next);
      prev_mail->next = mail_pointer->next;
      free(mail_pointer);
    }
  } else {
    /* move to next-to-last record */
    while (position_pointer->next->next)
      position_pointer = position_pointer->next;
    mail_address = position_pointer->next->position;
    free(position_pointer->next);
    position_pointer->next = NULL;
  }

  /* ok, now lets do some readin'! */
  if (!mail_begin())
    return (NULL);
  read_from_file(&header, BLOCK_SIZE, mail_address);

  if (header.block_type != HEADER_BLOCK) {
    mud_log("SYSERR: Oh dear. (Header block %ld != %d)", header.block_type, HEADER_BLOCK);
    mail_end();
    no_mail = TRUE;
    mud_log("SYSERR: Mail system disabled!  -- Error #9. (Invalid header block.)");
    return (NULL);
  }
  tmstr                        = asctime(localtime(&header.header_data.mail_time));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (header.header_data.from != -1337) {
    *from = get_name_by_id(header.header_data.from);
  } else {
    *from = strdup("Auctioneer");
  }
  to = get_name_by_id(recipient);

  if (*from == NULL) {
    *from = strdup("Unknown");
  }

  snprintf(buf, sizeof(buf),
           " @D* * * * @CGalactic Mail System @D* * * *\r\n"
           "@cDate@D:@w %s\r\n"
           "  @cTo@D:@G %s\r\n"
           "@cFrom@D:@R %s\r\n"
           "\r\n"
           "@w%s@n",

           tmstr,
           to ? CAP(to) : "Unknown",
           *from ? CAP(*from) : "Unknown",
           header.txt);
  following_block = header.header_data.next_block;

  /* mark the block as deleted */
  header.block_type = DELETED_BLOCK;
  write_to_file(&header, BLOCK_SIZE, mail_address);
  push_free_list(mail_address);

  while (following_block != LAST_BLOCK) {
    read_from_file(&data, BLOCK_SIZE, following_block);

    strcat(buf, data.txt); /* strcat: OK (data.txt:DATA_BLOCK_DATASIZE < buf:MAX_MAIL_SIZE) */
    mail_address    = following_block;
    following_block = data.block_type;
    data.block_type = DELETED_BLOCK;
    write_to_file(&data, BLOCK_SIZE, mail_address);
    push_free_list(mail_address);
  }
  mail_commit();

  return strdup(buf);
}

/****************************************************************
* Below is the spec_proc for a postmaster using the above       *
* routines.  Written by Jeremy Elson (jelson@circlemud.org) *
****************************************************************/

SPECIAL(postmaster)
{
  if (!ch->desc || char_is_npc(ch))
    return (0); /* so mobs don't get caught here */

  if (!(CMD_IS("mail") || CMD_IS("check") || CMD_IS("receive")))
    return (0);

  if (no_mail) {
    send_to_char(ch, "Sorry, the mail system is having technical difficulties.\r\n");
    return (0);
  }

  if (CMD_IS("mail")) {
    postmaster_send_mail(ch, (struct char_data *)me, cmd, argument);
    return (1);
  } else if (CMD_IS("check")) {
    postmaster_check_mail(ch, (struct char_data *)me, cmd, argument);
    return (1);
  } else if (CMD_IS("receive")) {
    postmaster_receive_mail(ch, (struct char_data *)me, cmd, argument);
    return (1);
  } else
    return (0);
}

static void postmaster_send_mail(struct char_data *ch, struct char_data *mailman,
                                 int cmd, char *arg)
{
  long recipient;
  char buf[MAX_INPUT_LENGTH], **mailwrite;

  if (char_get_level(ch) < MIN_MAIL_LEVEL && char_get_admlevel(ch) < ADMLVL_IMMORT) {
    snprintf(buf, sizeof(buf), "$n tells you, 'Sorry, you have to be level %d to send mail!'", MIN_MAIL_LEVEL);
    act(buf, FALSE, mailman, 0, ch, TO_VICT);
    return;
  }
  one_argument(arg, buf);

  if (!*buf) { /* you'll get no argument from me! */
    act("$n tells you, 'You need to specify an addressee!'",
        FALSE, mailman, 0, ch, TO_VICT);
    return;
  }
  if (char_get_gold(ch) < STAMP_PRICE && !char_adm_flagged(ch, ADM_MONEY)) {
    snprintf(buf, sizeof(buf), "$n tells you, 'A stamp costs %d zenni.'\r\n"
                               "$n tells you, '...which I see you can't afford.'",
             STAMP_PRICE);
    act(buf, FALSE, mailman, 0, ch, TO_VICT);
    return;
  }
  if ((recipient = get_id_by_name(buf)) < 0 || !mail_recip_ok(buf)) {
    act("$n tells you, 'No one by that name is registered here!'",
        FALSE, mailman, 0, ch, TO_VICT);
    return;
  }
  act("$n starts to write some mail.", TRUE, ch, 0, 0, TO_ROOM);
  snprintf(buf, sizeof(buf), "$n tells you, 'I'll take %d zenni for the stamp.'\r\n"
                             "$n tells you, 'Write your message. (/s saves /h for help).'",
           STAMP_PRICE);

  act(buf, FALSE, mailman, 0, ch, TO_VICT);
  act("@C$n@w starts writing a letter.@n", TRUE, ch, 0, 0, TO_ROOM);
  char_set_gold(ch, char_get_gold(ch) - (STAMP_PRICE));
  char_plr_set(ch, PLR_MAILING, true); /* string_write() sets writing. */

  /* Start writing! */
  CREATE(mailwrite, char *, 1);
  string_write(ch->desc, mailwrite, MAX_MAIL_SIZE, recipient, NULL);
}

static void postmaster_check_mail(struct char_data *ch, struct char_data *mailman,
                                  int cmd, char *arg)
{
  if (has_mail(char_get_idnum(ch)))
    act("$n tells you, 'You have mail waiting.'", FALSE, mailman, 0, ch, TO_VICT);
  else
    act("$n tells you, 'Sorry, you don't have any mail waiting.'", FALSE, mailman, 0, ch, TO_VICT);
}

static void postmaster_receive_mail(struct char_data *ch, struct char_data *mailman,
                                    int cmd, char *arg)
{
  char buf[256];
  struct obj_data *obj;
  int y;
  char *from;

  if (!has_mail(char_get_idnum(ch)) && mailman != NULL) {
    snprintf(buf, sizeof(buf), "$n tells you, 'Sorry, you don't have any mail waiting.'");
    act(buf, FALSE, mailman, 0, ch, TO_VICT);
    return;
  }
  while (has_mail(char_get_idnum(ch))) {
    obj              = create_obj();
    obj->item_number = NOTHING;

    obj_set_type_flag(obj, ITEM_NOTE);
    for (y = 0; y < TW_ARRAY_MAX; y++)
      obj->wear_flags[y] = 0;
    obj_wear_set(obj, ITEM_WEAR_TAKE, true);
    obj_wear_set(obj, ITEM_WEAR_HOLD, true);
    obj_set_weight(obj, 1);
    obj_set_cost(obj, 30);
    obj_set_cost_per_day(obj, 10);
    obj->action_description = read_delete(char_get_idnum(ch), &from);
    char bla[256], blm[256];
    sprintf(bla, "@WA piece of mail@n");
    sprintf(blm, "@WSomeone has left a piece of mail here@n");
    obj->short_description = strdup(bla);
    obj->description       = strdup(blm);
    sprintf(bla, "mail paper letter");
    obj->name = strdup(bla);
    *bla      = '\0';
    *blm      = '\0';
    obj_extra_set(obj, ITEM_UNIQUE_SAVE, true);
    add_unique_id(obj);
    obj_set_value(obj, 7, MATERIAL_PAPER);
    obj_set_value(obj, VAL_NOTE_HEALTH, 100);
    obj_set_value(obj, VAL_NOTE_MAXHEALTH, 100);

    if (obj->action_description == NULL)
      obj->action_description =
          strdup("Mail system error - please report.  Error #11.\r\n");

    /* so it saves */
    obj_extra_set(obj, ITEM_UNIQUE_SAVE, true);

    if (desc_is_playing(ch->desc) && mailman != NULL) {
      obj_to_char(obj, ch);
      act("$n gives you a piece of mail.", FALSE, mailman, 0, ch, TO_VICT);
      act("$N gives $n a piece of mail.", FALSE, ch, 0, mailman, TO_ROOM);
    } else {
      extract_obj(obj);
    }
  }
}

void notify_if_playing(struct char_data *from, int recipient_id)
{
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    if ((desc_is_playing(d)) && (char_get_idnum(d->character) == recipient_id) && (has_mail(char_get_idnum(d->character)))) {
      send_to_char(d->character, "\r\n\007\007\007@G@lYou have new mudmail from %s.@n\r\n", char_get_name(from));
    }
  }
}
