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

#include "dbat/game/alias.h"
#include "dbat/game/utils.h"
#include "dbat/game/interpreter.h"
#include "dbat/game/db.h"


void write_aliases(struct char_data *ch)
{
  FILE *file;
  char fn[MAX_STRING_LENGTH];
  struct alias_data *temp;

  get_filename(fn, sizeof(fn), ALIAS_FILE, GET_NAME(ch));
  remove(fn);

  if (GET_ALIASES(ch) == NULL)
    return;

  if ((file = fopen(fn, "w")) == NULL) {
    log("SYSERR: Couldn't save aliases for %s in '%s': %s", GET_NAME(ch), fn, strerror(errno));
    /*  SYSERR_DESC:
     *  This error occurs when the server fails to open the relevant alias
     *  file for writing.  The text at the end of the error should give a
     *  valid reason why.
     */
    return;
  }

  for (temp = GET_ALIASES(ch); temp; temp = temp->next) {
    int aliaslen = strlen(temp->alias);
    int repllen = strlen(temp->replacement) - 1;

    fprintf(file, "%d\n%s\n"	/* Alias */
		  "%d\n%s\n"	/* Replacement */
		  "%d\n",	/* Type */
		aliaslen, temp->alias,
		repllen, temp->replacement + 1,
		temp->type);
  }
  
  fclose(file);
}

void read_aliases(struct char_data *ch)
{   
  FILE *file;
  char xbuf[MAX_STRING_LENGTH];
  struct alias_data *t2, *prev = NULL;
  int length;

  get_filename(xbuf, sizeof(xbuf), ALIAS_FILE, GET_NAME(ch));

  if ((file = fopen(xbuf, "r")) == NULL) {
    if (errno != ENOENT) {
      log("SYSERR: Couldn't open alias file '%s' for %s: %s", xbuf, GET_NAME(ch), strerror(errno));
      /*  SYSERR_DESC:
       *  This error occurs when the server fails to open the relevant alias
       *  file for reading.  The text at the end version should give a valid
       *  reason why.
       */
    }
    return;
  }
 
  CREATE(GET_ALIASES(ch), struct alias_data, 1);
  t2 = GET_ALIASES(ch); 

  for (;;) {
    /* Read the aliased command. */
    if (fscanf(file, "%d\n", &length) != 1)
      goto read_alias_error;

    fgets(xbuf, length + 1, file);
    t2->alias = strdup(xbuf);

    /* Build the replacement. */
    if (fscanf(file, "%d\n", &length) != 1)
       goto read_alias_error;

    *xbuf = ' ';		/* Doesn't need terminated, fgets() will. */
    fgets(xbuf + 1, length + 1, file);
    t2->replacement = strdup(xbuf); 

    /* Figure out the alias type. */
    if (fscanf(file, "%d\n", &length) != 1)
      goto read_alias_error;

    t2->type = length; 

    if (feof(file))
      break;

    CREATE(t2->next, struct alias_data, 1);
    prev = t2;
    t2 = t2->next;
  }; 
  
  fclose(file);
  return;

read_alias_error:
  if (t2->alias)
    free(t2->alias);
  free(t2);
  if (prev)
    prev->next = NULL;
  fclose(file);
} 

void delete_aliases(const char *charname)
{
  char filename[PATH_MAX];

  if (!get_filename(filename, sizeof(filename), ALIAS_FILE, charname))
    return;

  if (remove(filename) < 0 && errno != ENOENT)
    log("SYSERR: deleting alias file %s: %s", filename, strerror(errno));
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

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) {			/* no argument specified -- list currently defined aliases */
    send_to_char(ch, "Currently defined aliases:\r\n");
    if ((a = GET_ALIASES(ch)) == NULL)
      send_to_char(ch, " None.\r\n");
    else {
      while (a != NULL) {
	send_to_char(ch, "%-15s %s\r\n", a->alias, a->replacement);
	a = a->next;
      }
    }
  } else {			/* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next, temp);
      free_alias(a);
    }
    /* if no replacement string is specified, assume we want to delete */
    if (!*repl) {
      if (a == NULL)
	send_to_char(ch, "No such alias.\r\n");
      else
	send_to_char(ch, "Alias deleted.\r\n");
    } else {			/* otherwise, either add or redefine an alias */
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
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
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
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  char buf2[MAX_RAW_INPUT_LENGTH], buf[MAX_RAW_INPUT_LENGTH];	/* raw? */
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  strcpy(buf2, orig);	/* strcpy: OK (orig:MAX_INPUT_LENGTH < buf2:MAX_RAW_INPUT_LENGTH) */
  temp = strtok(buf2, " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
	strcpy(write_point, tokens[num]);	/* strcpy: OK */
	write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
	strcpy(write_point, orig);		/* strcpy: OK */
	write_point += strlen(orig);
      } else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
	*(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
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
  if (IS_NPC(d->character))
    return (0);

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return (0);

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return (0);

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return (0);

  if (a->type == ALIAS_SIMPLE) {
    strlcpy(orig, a->replacement, maxlen);
    return (0);
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return (1);
  }
}