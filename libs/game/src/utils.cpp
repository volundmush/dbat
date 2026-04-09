/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "dbat/game/utils.h"
#include "dbat/game/comm.h"
#include "dbat/game/handler.h"
#include "dbat/game/random.h"
#include "dbat/game/spells.h"
#include "dbat/game/db.h"
#include "dbat/game/fight.h"
#include "dbat/game/class.h"
#include "dbat/game/feats.h"
#include "dbat/game/genzon.h"
#include "dbat/game/act.informative.h"
#include "dbat/game/screen.h"
#include <effolkronium/random.hpp>
#include "dbat/game/character_utils.h"


/* Add should be set to the amount you want to add to whatever is rolled. */
int roll_aff_duration(int num, int add)
{
 int start = num / 20;
 int finish = num / 10;
 int outcome = add;


 outcome += rand_number(start, finish);

 return (outcome);
}


void customWrite(struct char_data *ch, struct obj_data *obj)
{

 if (IS_NPC(ch))
  return;

 char fname[40], line[256], prev[256];
 char buf[MAX_STRING_LENGTH];
 FILE *fl, *file;

 if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, ch->desc->user)) {
  log("ERROR: Custom unable to be saved to user file!");
  return;
 }

 if (!(file = fopen(fname, "r"))) {
  log("ERROR: Custom unable to be saved to user file!");
  return;
 }

 while (!feof(file)) {
  get_line(file, line);
  if (strcasecmp(prev, line))
   sprintf(buf+strlen(buf), "%s\n", line);
  *prev = '\0';
  sprintf(prev, line);
 }

 fclose(file);

 if (!get_filename(fname, sizeof(fname), CUSTOME_FILE, ch->desc->user)) {
  log("ERROR: Custom unable to be saved to user file!");
  return;
 }

 if (!(fl = fopen(fname, "w"))) {
  log("ERROR: Custom unable to be saved to user file!");
  return;
 }

 sprintf(buf+strlen(buf), "%s\n", obj->short_description);
 fprintf(fl, "%s\n", buf);
 

 fclose(fl);
}



/* This updates the malfunctioning of certain objects that are damaged. */
void broken_update()
{
 struct obj_data *k, *money;

 int rand_gravity[14] = {0, 10, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 5000, 10000};
 int dice = rand_number(2, 12), grav_roll = 0, grav_change = FALSE, health = 0;

 for (k = object_list; k; k = k->next) {
  if (k->carried_by != NULL) {
   continue;
  }

  if (rand_number(1, 2) == 2) {
   continue;
  }

  health = GET_OBJ_VAL(k, VAL_ALL_HEALTH); // Indicated the health of the object in question

  if (GET_OBJ_VNUM(k) == 11) { /* Gravity Generator */
   grav_roll = rand_number(0, 13);
   if (health <= 10) {
    grav_change = TRUE;
   } else if (health <= 40 && dice <= 8) {
    grav_change = TRUE;
   } else if (health <= 80 && dice <= 5) {
    grav_change = TRUE;
   } else if (health <= 99 && dice <= 3) {
    grav_change = TRUE;
   }
   if (grav_change == TRUE) {
    ROOM_GRAVITY(IN_ROOM(k)) = rand_gravity[grav_roll];
    GET_OBJ_WEIGHT(k) = rand_gravity[grav_roll];
    send_to_room(IN_ROOM(k), "@RThe gravity generator malfunctions! The gravity level has changed!@n\r\n");
   }
  } /* End Gravity Section */

  if (GET_OBJ_VNUM(k) == 3034) { /* ATM */
   if (health <= 10) {
    send_to_room(IN_ROOM(k), "@RThe ATM machine shoots smoking bills from its money slot. The bills burn up as they float through the air!@n\r\n");
   } else if (health <= 40 && dice <= 8) {
    send_to_room(IN_ROOM(k), "@RGibberish flashes across the cracked ATM info screen.@n\r\n");
   } else if (health <= 80 && dice == 4) {
    send_to_room(IN_ROOM(k), "@GThe damaged ATM spits out some money while flashing ERROR on its screen!@n\r\n");
    money = create_money(rand_number(1, 30));
    obj_to_room(money, IN_ROOM(k));
   } else if (health <= 99 && dice < 4) {
    send_to_room(IN_ROOM(k), "@RThe ATM machine emits a loud grinding sound from inside.@n\r\n");
   }
  } /* End ATM */

  dice = rand_number(2, 12); // Reset the dice
 } /* End For */

}


void game_info(const char *format, ...) 
{ 
  struct descriptor_data *i; 
  va_list args; 
  char messg[MAX_STRING_LENGTH]; 

  if (format == NULL) 
    return; 

  sprintf(messg, "@r-@R=@D<@GCOPYOVER@D>@R=@r- @W"); 

  for (i = descriptor_list; i; i = i->next) { 
    if (STATE(i) != CON_PLAYING && (STATE(i) != CON_REDIT && STATE(i) != CON_OEDIT && STATE(i) != CON_MEDIT)) 
      continue; 
    if (!(i->character)) 
      continue; 

    write_to_output(i, messg); 
    va_start(args, format); 
    vwrite_to_output(i, format, args); 
    va_end(args); 
    write_to_output(i, "@n\r\n@R>>>@GMake sure to pick up your bed items and save.@n\r\n"); 
  } 
}

/* creates a random number in long long int */
int64_t large_rand(int64_t from, int64_t to)
{
  /* error checking in case people call this incorrectly */
  if (from > to) {
    int64_t tmp = from;
    from = to;
    to = tmp;
  }

  /* This should always be of the form:
   *
   *    ((float)(to - from + 1) * rand() / (float)(RAND_MAX + from) + from);
   *
   * if you are using rand() due to historical non-randomness of the
   * lower bits in older implementations.  We always use circle_random()
   * though, which shouldn't have that problem. Mean and standard
   * deviation of both are identical (within the realm of statistical
   * identity) if the rand() implementation is non-broken.  */
  return ((circle_random() % (to - from + 1)) + from);
}

/* creates a random number in interval [from;to] */
int rand_number(int from, int to)
{
  /* error checking in case people call this incorrectly */
  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
  }
  return effolkronium::random_static::get(from, to);
}

/* Axion engine dice function */
int axion_dice(int adjust)
{

 int die1 = 0, die2 = 0, roll = 0;

 die1 = rand_number(1, 60);
 die2 = rand_number(1, 60);

 roll = (die1 + die2) + adjust;

 if (roll < 2)
  roll = 2;

 return (roll);
}

/* simulates dice roll */
int dice(int num, int size)
{
  int sum = 0;

  if (size <= 0 || num <= 0)
    return (0);

  while (num-- > 0)
    sum += rand_number(1, size);

  return (sum);
}


char *CAP(char *txt)
{
  int i;
  for (i = 0; txt[i] != '\0' && (txt[i] == '@' && IS_COLOR_CHAR(txt[i + 1])); i += 2);

  txt[i] = UPPER(txt[i]);
  return (txt);
}


char *strlwr(char *s) 
{ 
   if (s != NULL) 
   { 
      char *p; 

      for (p = s; *p; ++p) 
         *p = LOWER(*p); 
   } 
   return s; 
} 


/* Strips \r\n from end of string.  */
void prune_crlf(char *txt)
{
  int i = strlen(txt) - 1;

  while (txt[i] == '\n' || txt[i] == '\r')
    txt[i--] = '\0';
}

/* log a death trap hit */
void log_death_trap(struct char_data *ch)
{
  mudlog(BRF, ADMLVL_IMMORT, TRUE, "%s hit death trap #%d (%s)", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), world[IN_ROOM(ch)].name);
}


/* New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.  */
void basic_mud_vlog(const char *format, va_list args)
{
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  if (logfile == NULL) {
    puts("SYSERR: Using log() before stream was initialized!");
    return;
  }

  if (format == NULL)
    format = "SYSERR: log() received a NULL format.";

  time_s[strlen(time_s) - 1] = '\0';

  fprintf(logfile, "%-15.15s :: ", time_s + 4);
  vfprintf(logfile, format, args);
  fputc('\n', logfile);
  fflush(logfile);
}


/* So mudlog() can use the same function. */
void basic_mud_log(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  basic_mud_vlog(format, args);
  va_end(args);
}


/* the "touch" command, essentially. */
int touch(const char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    log("SYSERR: %s: %s", path, strerror(errno));
    return (-1);
  } else {
    fclose(fl);
    return (0);
  }
}


/* mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992 */
void mudlog(int type, int level, int file, const char *str, ...)
{
  char buf[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  va_list args;

  if (str == NULL)
    return;	/* eh, oh well. */

  if (file) {
    va_start(args, str);
    basic_mud_vlog(str, args);
    va_end(args);
  }

  if (level < ADMLVL_IMMORT)
    level = ADMLVL_IMMORT;

  strcpy(buf, "[ ");	/* strcpy: OK */
  va_start(args, str);
  vsnprintf(buf + 2, sizeof(buf) - 6, str, args);
  va_end(args);
  strcat(buf, " ]\r\n");	/* strcat: OK */

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (GET_ADMLEVEL(i->character) < level)
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    if (type > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(i->character, "@g%s@n", buf);
  }
}









/* This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * You still want to call abort() or exit(1) for
 * non-recoverable errors, of course...
 *
 * XXX: Wonder if flushing streams includes sockets?  */
FILE *player_fl;
void core_dump_real(const char *who, int line)
{
  /* log("SYSERR: Assertion failed at %s:%d!", who, line); */

}

/* General use directory functions & structures. Required due to */
/* various differences between directory handling code on        */
/* different OS'es.  Needs solid testing though.                 */
/* Added by Dynamic Boards v2.4 - PjD (dughi@imaxx.net)          */
