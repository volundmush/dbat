/* ***********************************************************************
*  File: alias.c				A utility to CircleMUD	 *
* Usage: writing/reading player's aliases.				 *
*									 *
* Code done by Jeremy Hess and Chad Thompson				 *
* Modifed by George Greer for inclusion into CircleMUD bpl15.		 *
*									 *
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.		 *
*********************************************************************** */

#include "dbat/alias.h"
#include "dbat/consts/maximums.h"
#include "dbat/utils.h"

#include <linux/limits.h>
#include <errno.h>
#include "dbat/fileop.h"
#include "dbat/interpreter.h"
#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/stringutils.h"
#include "dbat/db.h"
#include "dbat/comm.h"
#include "dbat/atomicfile.h"

void write_aliases(struct char_data *ch)
{
  struct atomic_file af;
  FILE *file;
  char fn[MAX_STRING_LENGTH];
  struct alias_data *temp;

  get_filename(fn, sizeof(fn), ALIAS_FILE, char_get_name(ch));

  if (char_get_aliases(ch) == nullptr) {
    remove(fn);
    return;
  }

  if ((file = atomic_open(&af, fn, true)) == nullptr) {
    mud_log("SYSERR: Couldn't save aliases for %s in '%s': %s", char_get_name(ch), fn, strerror(errno));
    /*  SYSERR_DESC:
     *  This error occurs when the server fails to open the relevant alias
     *  file for writing.  The text at the end of the error should give a
     *  valid reason why.
     */
    return;
  }

  for (temp = char_get_aliases(ch); temp; temp = temp->next) {
    int aliaslen = strlen(temp->alias);
    int repllen  = strlen(temp->replacement) - 1;

    fprintf(file, "%d\n%s\n" /* Alias */
                  "%d\n%s\n" /* Replacement */
                  "%d\n",    /* Type */
            aliaslen, temp->alias,
            repllen, temp->replacement + 1,
            temp->type);
  }

  atomic_commit(&af);
}

/* Reads one alias record into a. Returns false if the file ends or is malformed partway through it. */
static bool read_alias_record(FILE *file, struct alias_data *a)
{
  char xbuf[MAX_STRING_LENGTH];
  int length;

  /* Read the aliased command. */
  if (fscanf(file, "%d\n", &length) != 1 || length < 0 || length >= (int)sizeof(xbuf) - 1)
    return false;

  fgets(xbuf, length + 1, file);
  a->alias = strdup(xbuf);

  /* Build the replacement. */
  if (fscanf(file, "%d\n", &length) != 1 || length < 0 || length >= (int)sizeof(xbuf) - 1)
    return false;

  *xbuf = ' '; /* Doesn't need terminated, fgets() will. */
  fgets(xbuf + 1, length + 1, file);
  a->replacement = strdup(xbuf);

  /* Figure out the alias type. */
  if (fscanf(file, "%d\n", &length) != 1)
    return false;

  a->type = length;
  return true;
}

void read_aliases(struct char_data *ch)
{
  FILE *file;
  char xbuf[MAX_STRING_LENGTH];
  struct alias_data *head = nullptr, **tail = &head;

  get_filename(xbuf, sizeof(xbuf), ALIAS_FILE, char_get_name(ch));

  if ((file = fopen(xbuf, "r")) == nullptr) {
    if (errno != ENOENT) {
      mud_log("SYSERR: Couldn't open alias file '%s' for %s: %s", xbuf, char_get_name(ch), strerror(errno));
      /*  SYSERR_DESC:
       *  This error occurs when the server fails to open the relevant alias
       *  file for reading.  The text at the end version should give a valid
       *  reason why.
       */
    }
    return;
  }

  /* A malformed record is dropped along with everything after it; the records before it are kept. */
  for (;;) {
    struct alias_data *a;

    CREATE(a, struct alias_data, 1);
    if (!read_alias_record(file, a)) {
      free(a->alias);
      free(a->replacement);
      free(a);
      break;
    }
    *tail = a;
    tail  = &a->next;

    if (feof(file))
      break;
  }

  char_set_aliases(ch, head);
  fclose(file);
}

void delete_aliases(const char *charname)
{
  char filename[PATH_MAX];

  if (!get_filename(filename, sizeof(filename), ALIAS_FILE, charname))
    return;

  if (remove(filename) < 0 && errno != ENOENT)
    mud_log("SYSERR: deleting alias file %s: %s", filename, strerror(errno));
  /*  SYSERR_DESC:
     *  When an alias file cannot be removed, this error will occur,
     *  and the reason why will be the tail end of the error.
     */
}

/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  char arg[MAX_INPUT_LENGTH];
  char *repl;
  struct alias_data *a, *temp;

  if (char_is_npc(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) { /* no argument specified -- list currently defined aliases */
    send_to_char(ch, "Currently defined aliases:\r\n");
    if ((a = char_get_aliases(ch)) == nullptr)
      send_to_char(ch, " None.\r\n");
    else {
      while (a != nullptr) {
        send_to_char(ch, "%-15s %s\r\n", a->alias, a->replacement);
        a = a->next;
      }
    }
  } else { /* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(char_get_aliases(ch), arg)) != nullptr) {
      struct alias_data *head = char_get_aliases(ch);
      REMOVE_FROM_LIST(a, head, next, temp);
      char_set_aliases(ch, head);
      free_alias(a);
    }
    /* if no replacement string is specified, assume we want to delete */
    if (!*repl) {
      if (a == nullptr)
        send_to_char(ch, "No such alias.\r\n");
      else
        send_to_char(ch, "Alias deleted.\r\n");
    } else { /* otherwise, either add or redefine an alias */
      if (!strcasecmp(arg, "alias")) {
        send_to_char(ch, "You can't alias 'alias'.\r\n");
        return;
      }
      CREATE(a, struct alias_data, 1);
      a->alias = strdup(arg);
      delete_doubledollar(repl);
      a->replacement = strdup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
        a->type = ALIAS_COMPLEX;
      else
        a->type = ALIAS_SIMPLE;
      a->next = char_get_aliases(ch);
      char_set_aliases(ch, a);
      send_to_char(ch, "Alias added.\r\n");
    }
  }
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
constexpr int NUM_TOKENS = 9;

/* Copies text to *write_point, stopping at end (the last byte of the buffer, kept for the NUL). */
static void alias_append(char **write_point, char *end, const char *text)
{
  while (*text && *write_point < end)
    *(*write_point)++ = *text++;
}

static void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  char buf2[MAX_RAW_INPUT_LENGTH], buf[MAX_RAW_INPUT_LENGTH]; /* raw? */
  char *end         = buf + sizeof(buf) - 1;                  /* $* and $1..$9 can expand past any buffer */
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  strlcpy(buf2, orig, sizeof(buf2));
  temp = strtok(buf2, " ");
  while (temp != nullptr && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp                    = strtok(nullptr, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue  = (struct txt_q){0};

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point              = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if (!*temp) { /* a '$' at the very end: keep it, and don't step past the string */
        alias_append(&write_point, end, "$");
        break;
      }
      if ((num = *temp - '1') < num_of_tokens && num >= 0)
        alias_append(&write_point, end, tokens[num]);
      else if (*temp == ALIAS_GLOB_CHAR)
        alias_append(&write_point, end, orig);
      else {
        char literal[2] = {*temp, '\0'};
        alias_append(&write_point, end, literal);
        if (*temp == '$') /* redouble $ for act safety */
          alias_append(&write_point, end, "$");
      }
    } else if (write_point < end)
      *(write_point++) = *temp;
  }

  *write_point              = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == nullptr)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head         = temp_queue.head;
  }
}

/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias_data *a, *tmp;

  /* Mobs don't have alaises. */
  if (char_is_npc(d->character))
    return (0);

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = char_get_aliases(d->character)) == nullptr)
    return (0);

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return (0);

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == nullptr)
    return (0);

  if (a->type == ALIAS_SIMPLE) {
    strlcpy(orig, a->replacement, maxlen);
    return (0);
  }
  perform_complex_alias(&d->input, ptr, a);
  return (1);
}