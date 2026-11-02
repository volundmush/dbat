/* ************************************************************************
*   File: accounts.c                                    Part of CircleMUD *
*  Usage: user account files                                              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#include "dbat/interpreter.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/dg_scripts.h"
#include "dbat/fileop.h"
#include "dbat/broadcast.h"
#include "dbat/players.h"
#include "dbat/atomicfile.h"

#include <unistd.h>
#include "nanny_internal.h"

/* The account's character slots, numbered from 1. */
char **account_slot(struct descriptor_data *d, int n)
{
  switch (n) {
    case 1:
      return &d->tmp1;
    case 2:
      return &d->tmp2;
    case 3:
      return &d->tmp3;
    case 4:
      return &d->tmp4;
    default:
      return &d->tmp5;
  }
}

void set_account_slot(struct descriptor_data *d, int n, const char *name)
{
  char **slot = account_slot(d, n);

  free(*slot);
  *slot = strdup(name);
}

/* Empties every slot holding name (used when a character is deleted or missing). */
void empty_account_slots_named(struct descriptor_data *d, const char *name)
{
  for (int n = 1; n <= MAX_ACCOUNT_SLOTS; n++)
    if (!strcasecmp(name, *account_slot(d, n)))
      set_account_slot(d, n, "Empty");
}

int readUserIndex(char *name)
{
  char fname[40];
  FILE *fl;

  /* Read User Index */
  if (!get_filename(fname, sizeof(fname), USER_FILE, name)) {
    return 0;
  } else if (!(fl = fopen(fname, "r"))) {
    return 0;
  }
  fclose(fl);
  return 1;
}

/* True if name is on the lockout list (lib/etc lockout intro file). */
bool lockRead(const char *name)
{
  char fname[40], filler[50], line[256];
  bool known = false;
  FILE *fl;

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout")) {
    return false;
  } else if (!(fl = fopen(fname, "r"))) {
    return false;
  }

  while (get_line(fl, line)) {
    if (sscanf(line, "%49s", filler) == 1 && !strcasecmp(name, filler)) {
      known = true;
    }
  }
  fclose(fl);
  return known;
}

void userLoad(struct descriptor_data *d, char *name)
{
  char fname[40], filler[100] = "", line[256] = "";
  FILE *fl;

  /* Read and Write User Index */
  if (!get_filename(fname, sizeof(fname), USER_FILE, name)) {
    return;
  } else if (!(fl = fopen(fname, "r"))) {
    mud_log("ERROR: could not load user, %s, from filename, %s.", name, fname);
    return;
  }
  int count = 0;
  while (!feof(fl)) {
    get_line(fl, line);
    count += 1;
    switch (count) {
      case 1:
        sscanf(line, "%99s", filler);
        if (d->user) {
          free(d->user);
          d->user = NULL;
        }
        d->user = strdup(filler);
        break;
      case 2:
        sscanf(line, "%99s", filler);
        if (d->email) {
          free(d->email);
          d->email = NULL;
        }
        d->email = strdup(filler);
        break;
      case 3:
        sscanf(line, "%99s", filler);
        if (d->pass) {
          free(d->pass);
          d->pass = NULL;
        }
        d->pass = strdup(filler);
        break;
      case 4:
        sscanf(line, "%d\n", &d->total);
        break;
      case 5:
        sscanf(line, "%d\n", &d->rpp);
        break;
      case 6:
      case 7:
      case 8:
      case 9:
      case 10:
        sscanf(line, "%99s", filler);
        set_account_slot(d, count - 5, filler);
        break;
      case 11:
        sscanf(line, "%d\n", &d->level);
        break;
      case 12:
        sscanf(line, "%d\n", &d->customfile);
        break;
      case 13:
        sscanf(line, "%d\n", &d->rbank);
        break;
    }
    *filler = '\0';
  }
  fclose(fl);
  return;
}

void userCreate(struct descriptor_data *d)
{
  struct atomic_file af;
  char fname[40];
  FILE *fl;

  /* Write Their User File */

  if (!get_filename(fname, sizeof(fname), USER_FILE, d->user))
    return;

  if (!(fl = atomic_open(&af, fname, true))) {
    mud_log("ERROR: could not save user, %s, to filename, %s.", d->user, fname);
    return;
  }

  /* User's Account Name */
  fprintf(fl, "%s\n", CAP(d->user));
  /* User's Email */
  fprintf(fl, "%s\n", d->email);
  /* User's Password */
  fprintf(fl, "%s\n", d->pass);
  /* User's Allowed Player Total */
  fprintf(fl, "3\n");
  d->total = 3;
  /* User's Total RPP */
  fprintf(fl, "0\n");
  d->rpp = 0;
  /* Player Slots */
  fprintf(fl, "Empty\n");
  fprintf(fl, "Empty\n");
  fprintf(fl, "Empty\n");
  fprintf(fl, "Empty\n");
  fprintf(fl, "Empty\n");
  fprintf(fl, "0\n");
  fprintf(fl, "1\n");
  /* User's Total RPP */
  fprintf(fl, "0\n");
  d->rbank = 0;

  atomic_commit(&af);

  for (int n = 1; n <= MAX_ACCOUNT_SLOTS; n++)
    set_account_slot(d, n, "Empty");
  customCreate(d);

  return;
}

/* Delete A User Account */
void userDelete(struct descriptor_data *d)
{
  int player_i;

  char fname[40];

  if (get_filename(fname, sizeof(fname), USER_FILE, d->user)) {
    for (int n = 1; n <= MAX_ACCOUNT_SLOTS; n++) {
      const char *name = *account_slot(d, n);
      if (strcasecmp(name, "Empty") && (player_i = get_ptable_by_name(name)) >= 0)
        remove_player(player_i);
    }
    unlink(fname);
    return;
  } else {
    write_to_output(d, "Error. Your user file doesn't even exist!\n");
    return;
  }
}

/* For transfering money or doing things with an offline player */
const char *rIntro(struct char_data *ch, char *arg)
{
  char fname[40], filler[50] = "", scrap[100] = "", line[256] = "";
  static char name[80];
  int known = FALSE;
  FILE *fl;

  /* Read Introduction File */
  if (char_is_npc(ch)) {
    return "NOTHING";
  }

  if (!get_filename(fname, sizeof(fname), INTRO_FILE, char_get_name(ch))) {
    return "NOTHING";
  } else if (!(fl = fopen(fname, "r"))) {
    return "NOTHING";
  }

  while (!feof(fl)) {
    get_line(fl, line);
    sscanf(line, "%49s %99s", filler, scrap);
    if (!strcasecmp(arg, scrap)) {
      known = TRUE;
      sprintf(name, "%s", filler);
    }
  }
  fclose(fl);

  if (known == TRUE)
    return (name);
  else
    return "NOTHING";
}

/* Update A User Account */
void userWrite(struct descriptor_data *d, int setTot, int setRpp, int setRBank, const char *name)
{
  struct atomic_file af;
  char fname[40];
  FILE *fl;

  if (!strcasecmp(name, "index")) {
    if (!d) {
      return;
    }
    if (d->user == NULL) {
      return;
    }
    if (!get_filename(fname, sizeof(fname), USER_FILE, d->user))
      return;

    if (!(fl = atomic_open(&af, fname, true))) {
      mud_log("ERROR: could not save user, %s, to filename, %s.", d->user, fname);
      return;
    }

    /* User's Account Name */
    fprintf(fl, "%s\n", CAP(d->user));

    /* User's Email */
    fprintf(fl, "%s\n", d->email);

    /* User's Pass */
    fprintf(fl, "%s\n", d->pass);

    /* User's Allowed Player Total */
    if (setTot <= 3 || setTot > 5) {
      fprintf(fl, "%d\n", d->total);
    } else if (setTot > 3) {
      fprintf(fl, "%d\n", setTot);
      d->total = setTot;
    }
    /* User's RPP Total */
    if (setRpp == 0) {
      fprintf(fl, "%d\n", d->rpp);
    } else {
      d->rpp += setRpp;
      fprintf(fl, "%d\n", d->rpp);
    }

    /* Write Player Slots */
    for (int n = 1; n <= MAX_ACCOUNT_SLOTS; n++)
      fprintf(fl, "%s\n", *account_slot(d, n) ? *account_slot(d, n) : "Empty");

    fprintf(fl, "%d\n", d->level);

    /* Write whether we have a custom file or not */
    fprintf(fl, "%d\n", d->customfile);

    /* User's RPP Bank Total */
    if (setRBank == 0) {
      fprintf(fl, "%d\n", d->rbank);
    } else {
      d->rbank += setRBank;
      fprintf(fl, "%d\n", d->rbank);
    }

    atomic_commit(&af);
    return;
  } else if (strcasecmp(name, "index")) {
    char filename[40], uname[100] = "", email[100] = "", pass[100] = "", tmp1[100] = "", tmp2[100] = "", tmp3[100] = "", tmp4[100] = "", tmp5[100] = "", line[256] = "";
    int total = 0, rpp = 0, level = 0, custom = 0, rbank = 0;
    FILE *file;

    /* Read and Write User Index */
    if (!get_filename(filename, sizeof(filename), USER_FILE, name)) {
      return;
    } else if (!(file = fopen(filename, "r"))) {
      mud_log("ERROR: could not load user, %s, from filename, %s.", name, filename);
      return;
    }

    int count = 0;
    while (!feof(file)) {
      get_line(file, line);
      count += 1;
      switch (count) {
        case 1:
          sscanf(line, "%99s", uname);
          break;
        case 2:
          sscanf(line, "%99s", email);
          break;
        case 3:
          sscanf(line, "%99s", pass);
          break;
        case 4:
          sscanf(line, "%d\n", &total);
          break;
        case 5:
          sscanf(line, "%d\n", &rpp);
          break;
        case 6:
          sscanf(line, "%99s", tmp1);
          break;
        case 7:
          sscanf(line, "%99s", tmp2);
          break;
        case 8:
          sscanf(line, "%99s", tmp3);
          break;
        case 9:
          sscanf(line, "%99s", tmp4);
          break;
        case 10:
          sscanf(line, "%99s", tmp5);
          break;
        case 11:
          sscanf(line, "%d\n", &level);
          break;
        case 12:
          sscanf(line, "%d\n", &custom);
          break;
        case 13:
          sscanf(line, "%d\n", &rbank);
          break;
      }
    }
    fclose(file);

    if (!get_filename(fname, sizeof(fname), USER_FILE, name))
      return;

    if (!(fl = atomic_open(&af, fname, true))) {
      mud_log("ERROR: could not save user, %s, to filename, %s.", name, fname);
      return;
    }
    /* User's Account Name */
    fprintf(fl, "%s\n", uname);
    /* User's Email */
    fprintf(fl, "%s\n", email);

    /* User's Pass */
    fprintf(fl, "%s\n", pass);

    /* User's Allowed Player Total */
    if (setTot <= 3 || setTot > 5) {
      fprintf(fl, "%d\n", total);
    } else if (setTot > 3) {
      fprintf(fl, "%d\n", setTot);
    }
    /* User's RPP Total */
    if (setRpp == 0) {
      fprintf(fl, "%d\n", rpp);
    } else if (rpp + setRpp < 0) {
      send_to_imm("RPP would be below 0, reward canceled.");
      fprintf(fl, "%d\n", rpp);
    } else {
      rpp += setRpp;
      fprintf(fl, "%d\n", rpp);
    }

    /* Write Player Slots */
    fprintf(fl, "%s\n", tmp1);
    fprintf(fl, "%s\n", tmp2);
    fprintf(fl, "%s\n", tmp3);
    fprintf(fl, "%s\n", tmp4);
    fprintf(fl, "%s\n", tmp5);
    fprintf(fl, "%d\n", level);
    fprintf(fl, "%d\n", custom);

    /* User's RPP Total */
    if (setRBank == 0) {
      fprintf(fl, "%d\n", rbank);
    } else if (rbank + setRBank < 0) {
      send_to_imm("RPP Bank would be below 0, reward canceled.");
      fprintf(fl, "%d\n", rbank);
    } else {
      rbank += setRBank;
      fprintf(fl, "%d\n", rbank);
    }

    atomic_commit(&af);
    return;
  } else {
    send_to_imm("Error with userWrite!");
    return;
  }
}

void fingerUser(struct char_data *ch, char *name)
{
  char filename[40], uname[100] = "", email[100] = "", pass[100] = "", tmp1[100] = "", tmp2[100] = "", tmp3[100] = "", tmp4[100] = "", tmp5[100] = "", line[256] = "";
  int total = 0, rpp = 0, rbank = 0;
  FILE *file;

  /* Read and Write User Index */
  if (!get_filename(filename, sizeof(filename), USER_FILE, name)) {
    send_to_char(ch, "That user doesn't exist.\r\n");
    return;
  } else if (!(file = fopen(filename, "r"))) {
    send_to_char(ch, "That user is bugged! Report to Iovan.\r\n");
    return;
  }

  int count = 0;
  while (!feof(file)) {
    get_line(file, line);
    count += 1;
    switch (count) {
      case 1:
        sscanf(line, "%99s", uname);
        break;
      case 2:
        sscanf(line, "%99s", email);
        break;
      case 3:
        sscanf(line, "%99s", pass);
        break;
      case 4:
        sscanf(line, "%d\n", &total);
        break;
      case 5:
        sscanf(line, "%d\n", &rpp);
        break;
      case 6:
        sscanf(line, "%99s", tmp1);
        break;
      case 7:
        sscanf(line, "%99s", tmp2);
        break;
      case 8:
        sscanf(line, "%99s", tmp3);
        break;
      case 9:
        sscanf(line, "%99s", tmp4);
        break;
      case 10:
        sscanf(line, "%99s", tmp5);
        break;
      case 13:
        sscanf(line, "%d\n", &rbank);
        break;
    }
  }
  fclose(file);

  send_to_char(ch, "@D[@gUsername   @D: @w%-30s@D]@n\r\n", uname);
  send_to_char(ch, "@D[@gEmail      @D: @w%-30s@D]@n\r\n", email);
  if (char_get_admlevel(ch) > 4) {
    send_to_char(ch, "@D[@gPass       @D: @w%-30s@D]@n\r\n", pass);
  } else if (char_get_admlevel(ch) > 0) {
    send_to_char(ch, "@D[@gPass       @D: @w%-30s@D]@n\r\n", "??????????");
  }
  send_to_char(ch, "@D[@gTotal Slots@D: @w%-30d@D]@n\r\n", total);
  send_to_char(ch, "@D[@gRP Points  @D: @w%-30d@D]@n\r\n", rpp);
  send_to_char(ch, "@D[@gRP Bank    @D: @w%-30d@D]@n\r\n", rbank);
  if (char_get_admlevel(ch) > 0) {
    send_to_char(ch, "@D[@gCh. Slot 1 @D: @w%-30s@D]@n\r\n", tmp1);
    send_to_char(ch, "@D[@gCh. Slot 2 @D: @w%-30s@D]@n\r\n", tmp2);
    send_to_char(ch, "@D[@gCh. Slot 3 @D: @w%-30s@D]@n\r\n", tmp3);
    send_to_char(ch, "@D[@gCh. Slot 4 @D: @w%-30s@D]@n\r\n", tmp4);
    send_to_char(ch, "@D[@gCh. Slot 5 @D: @w%-30s@D]@n\r\n", tmp5);
    send_to_char(ch, "\n");
    customRead(ch->desc, 1, name);
  }
  return;
}

void userRead(struct descriptor_data *d)
{
  write_to_output(d, "                 @RUser Menu@n\n");
  write_to_output(d, "@D=============================================@n\r\n");
  write_to_output(d, "@D|@gUser Account  @D: @w%-27s@D|@n\n", d->user);
  write_to_output(d, "@D|@gEmail Address @D: @w%-27s@D|@n\n", d->email);
  write_to_output(d, "@D|@gMax Characters@D: @w%-27d@D|@n\n", d->total);
  write_to_output(d, "@D|@gRP Points     @D: @w%-27d@D|@n\n", d->rpp);
  write_to_output(d, "@D|@gRP Bank       @D: @w%-27d@D|@n\n", d->rbank);
  write_to_output(d, "@D=============================================@n\r\n\r\n");
  write_to_output(d, "      @D[@y----@YSelect A Character Slot@y----@D]@n\n");
  for (int n = 1; n <= MAX_ACCOUNT_SLOTS; n++)
    if (n <= 3 || d->total >= n)
      write_to_output(d, "                @B(@W%d@B) @C%s@n\n", n, *account_slot(d, n));
  write_to_output(d, "\n");
  write_to_output(d, "      @D[@y---- @YSelect Another Option @y----@D]@n\n");
  write_to_output(d, "                @B(@WB@B) @CBuy New C. Slot @D(@R15 RPP@D)@n\n");
  write_to_output(d, "                @B(@WC@B) @CUser's Customs\n");
  write_to_output(d, "                @B(@WD@B) @RDelete User@n\n");
  write_to_output(d, "                @B(@WE@B) @CEmail@n\n");
  write_to_output(d, "                @B(@WP@B) @CNew Password@n\n");
  write_to_output(d, "                @B(@WQ@B) @CQuit@n\n");
  write_to_output(d, "\r\nMake your choice: \n");
}
