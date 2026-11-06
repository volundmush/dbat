/*
 * Originally written by: Michael Scott -- Manx.
 * Last known e-mail address: scottm@workcomm.net
 *
 * XXX: This needs Oasis-ifying.
 */

#include "dbat/utils.h"
#include "dbat/interpreter.h"
#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/genolc.h"
#include "dbat/oasis.h"
#include "dbat/improved-edit.h"
#include "dbat/tedit.h"
#include "dbat/config.h"
#include "dbat/atomicfile.h"
#include "dbat/character_utils.h"
#include "dbat/game_world.h"

void news_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator)
{
  FILE *fl;
  const char *storage = NEWS_FILE;

  if (!storage)
    terminator = STRINGADD_ABORT;

  switch (terminator) {
    case STRINGADD_SAVE:
      if (!*d->str)
        mudlog(g, LOG_CMP, ADMLVL_IMPL, true, "SYSERR: Can't write file '%s'.", storage);
      else if (!(fl = fopen(storage, "a"))) {
        mudlog(g, LOG_CMP, ADMLVL_IMPL, true, "SYSERR: Can't write file '%s'.", storage);
        write_to_output(g, d, "The news file couldn't be opened; your entry was not saved.\r\n");
      } else {
        char *tmstr;
        time_t mytime                = game_now(g);
        tmstr                        = (char *)asctime(localtime(&mytime));
        *(tmstr + strlen(tmstr) - 1) = '\0';

        fprintf(fl, "#%d %s\n@cUpdated By@D: @C%-13s @cDate@D: @Y%s@n\n", g->TOP_OF_NEWS, d->newsbuf, char_get_name(d->character), tmstr);
        free(d->newsbuf);
        d->newsbuf = nullptr;
        strip_cr(*d->str);
        fprintf(fl, "%s\n", *d->str);
        free(*d->str);
        *d->str = nullptr;
        fclose(fl);

        g->NEWSUPDATE = game_now(g);
        save_mud_time(g, &g->time_info);
        struct descriptor_data *i;

        for (i = g->descriptors; i; i = i->next) {
          if (!desc_is_playing(i))
            continue;
          if (char_plr_flagged(i->character, PLR_WRITING))
            continue;
          if (g->NEWSUPDATE > char_get_lastpl(i->character))
            send_to_char(g, i->character, "\r\n@GA news entry has been made by %s, type 'news %d' to see it.@n\r\n", char_get_name(d->character), g->TOP_OF_NEWS);
        }
        do_reboot(g, d->character, (char[]){"news"}, 0, 0);
      }
      break;
    case STRINGADD_ABORT:
      write_to_output(g, d, "Edit aborted.\r\n");
      act(g, "$n stops editing the news.", true, d->character, 0, 0, TO_ROOM);
      break;
    default:
      mud_log("SYSERR: news_string_cleanup: Unknown terminator status.");
      break;
  }

  /* Common cleanup code. */
  desc_set_connected(d, CON_PLAYING);
}

void tedit_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator)
{
  struct atomic_file af;
  FILE *fl;
  char *storage = OLC_STORAGE(d);

  if (!storage)
    terminator = STRINGADD_ABORT;

  switch (terminator) {
    case STRINGADD_SAVE:
      if (!(fl = atomic_open(&af, storage, false)))
        mudlog(g, LOG_CMP, ADMLVL_IMPL, true, "SYSERR: Can't write file '%s'.", storage);
      else {
        if (*d->str && !(strcmp(storage, "text/news"))) {
          char *tmstr;
          time_t mytime                = game_now(g);
          tmstr                        = (char *)asctime(localtime(&mytime));
          *(tmstr + strlen(tmstr) - 1) = '\0';

          strip_cr(*d->str);
          fprintf(fl, "\n-----------------------------------------------\n@Y%s @cUpdated By@D: @C%s@n\r\n-----------------------------------------------\n%s\n", tmstr, char_get_name(d->character), *d->str);
        } else if (*d->str) {
          strip_cr(*d->str);
          fputs(*d->str, fl);
        }
        if (!atomic_commit(&af)) {
          write_to_output(g, d, "Could not write the file; nothing was changed.\r\n");
          break;
        }
        mudlog(g, LOG_CMP, ADMLVL_GOD, true, "OLC: %s saves '%s'.", char_get_name(d->character), storage);
        write_to_output(g, d, "Saved.\r\n");

        if (!(strcmp(storage, "text/news"))) {
          g->NEWSUPDATE = game_now(g);
          save_mud_time(g, &g->time_info);
          struct descriptor_data *i;

          for (i = g->descriptors; i; i = i->next) {
            if (!desc_is_playing(i))
              continue;
            if (char_plr_flagged(i->character, PLR_WRITING))
              continue;
            if (g->NEWSUPDATE > char_get_lastpl(i->character))
              send_to_char(g, i->character, "\r\n@GThe NEWS file has been updated, type 'news' to see it.@n\r\n");
          }
          do_reboot(g, d->character, (char[]){"all"}, 0, 0);
        }
      }
      break;
    case STRINGADD_ABORT:
      write_to_output(g, d, "Edit aborted.\r\n");
      act(g, "$n stops editing some scrolls.", true, d->character, 0, 0, TO_ROOM);
      break;
    default:
      mud_log("SYSERR: tedit_string_cleanup: Unknown terminator status.");
      break;
  }

  /* Common cleanup code. */
  cleanup_olc(g, d, CLEANUP_ALL);
  desc_set_connected(d, CON_PLAYING);
}

ACMD(do_tedit)
{
  int l, i = 0;
  char field[MAX_INPUT_LENGTH];
  char *backstr = nullptr;

  struct {
    const char *cmd;
    char level;
    char **buffer;
    int size;
    const char *filename;
  } fields[] = {
      /* edit the lvls to your own needs */
      {"credits",    ADMLVL_IMPL,  &g->credits,    24000, CREDITS_FILE   },
      {"donottouch", 6,            &g->news,       24000, NEWS_FILE      },
      {"motd",       ADMLVL_IMPL,  &g->motd,       24000, MOTD_FILE      },
      {"imotd",      ADMLVL_IMPL,  &g->imotd,      24000, IMOTD_FILE     },
      {"help",       ADMLVL_GRGOD, &g->help,       24000, HELP_PAGE_FILE },
      {"info",       ADMLVL_GRGOD, &g->info,       24000, INFO_FILE      },
      {"background", ADMLVL_IMPL,  &g->background, 24000, BACKGROUND_FILE},
      {"handbook",   ADMLVL_IMPL,  &g->handbook,   24000, HANDBOOK_FILE  },
      {"update",     ADMLVL_IMPL,  &g->policies,   24000, POLICIES_FILE  },
      {"ihelp",      ADMLVL_GRGOD, &g->ihelp,      24000, IHELP_PAGE_FILE},
      {"\n",         0,            nullptr,     0,     nullptr        }
  };

  if (char_get_desc(ch) == nullptr)
    return;

  one_argument(argument, field);

  if (!*field) {
    send_to_char(g, ch, "Files available to be edited:\r\n");
    for (l = 0; *fields[l].cmd != '\n'; l++) {
      if (char_get_admlevel(ch) >= fields[l].level) {
        send_to_char(g, ch, "%-11.11s ", fields[l].cmd);
        if (!(++i % 7))
          send_to_char(g, ch, "\r\n");
      }
    }
    if (i % 7)
      send_to_char(g, ch, "\r\n");
    if (i == 0)
      send_to_char(g, ch, "None.\r\n");
    return;
  }
  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (*fields[l].cmd == '\n') {
    send_to_char(g, ch, "Invalid text editor option.\r\n");
    return;
  }

  if (char_get_admlevel(ch) < fields[l].level) {
    send_to_char(g, ch, "You are not godly enough for that!\r\n");
    return;
  }

  /* set up editor stats */
  clear_screen(g, char_get_desc(ch));
  send_editor_help(g, char_get_desc(ch));
  send_to_char(g, ch, "Edit file below:\r\n\r\n");

  if (char_get_desc(ch)->olc) {
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: do_tedit: Player already had olc structure.");
    free(char_get_desc(ch)->olc);
  }
  CREATE(char_get_desc(ch)->olc, struct oasis_olc_data, 1);

  if (*fields[l].buffer) {
    send_to_char(g, ch, "%s", *fields[l].buffer);
    backstr = strdup(*fields[l].buffer);
  }

  OLC_STORAGE(char_get_desc(ch)) = strdup(fields[l].filename);
  string_write(char_get_desc(ch), fields[l].buffer, fields[l].size, 0, backstr);

  act(g, "$n begins editing a text file.", true, ch, 0, 0, TO_ROOM);
  char_plr_set(ch, PLR_WRITING, true);
  desc_set_connected(char_get_desc(ch), CON_TEDIT);
}
