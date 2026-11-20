/*
 * Originally written by: Michael Scott -- Manx.
 * Last known e-mail address: scottm@workcomm.net
 *
 * XXX: This needs Oasis-ifying.
 */

#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/comm.hpp"
#include "dbat/db.hpp"
#include "dbat/genolc.hpp"
#include "dbat/olc/session.hpp"
#include "dbat/improved-edit.hpp"
#include "dbat/tedit.hpp"
#include "dbat/config.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

void news_string_cleanup(GameWorld *g, struct descriptor_data *d, int terminator)
{
  const char *storage = NEWS_FILE;

  if (!storage)
    terminator = STRINGADD_ABORT;

  switch (terminator) {
    case STRINGADD_SAVE:
      if (d->draft.empty())
        mudlog(g, LOG_CMP, ADMLVL_IMPL, true, "SYSERR: Can't write file '{}'.", storage);
      else {
        char *tmstr;
        time_t mytime                = game_now(g);
        tmstr                        = asctime(localtime(&mytime));
        *(tmstr + strlen(tmstr) - 1) = '\0';

        std::string draft = d->draft;
        strip_cr(draft);
        std::string entry = str_format("#{:d} {}\n@cUpdated By@D: @C{:<13} @cDate@D: @Y{}@n\n{}\n", g->TOP_OF_NEWS, d->newsbuf,
                                       char_get_name(d->character), tmstr, draft);
        if (!io::append_file(storage, entry)) {
          mudlog(g, LOG_CMP, ADMLVL_IMPL, true, "SYSERR: Can't write file '{}'.", storage);
          write_to_output(g, d, "The news file couldn't be opened; your entry was not saved.\r\n");
          break;
        }
        d->newsbuf.clear();

        g->NEWSUPDATE = game_now(g);
        save_mud_time(g, &g->time_info);

        for (descriptor_data *i : each_desc(g)) {
          if (!desc_is_playing(i))
            continue;
          if (char_plr_flagged(i->character, PLR_WRITING))
            continue;
          if (g->NEWSUPDATE > char_get_lastpl(i->character))
            send_to_char(g, i->character, "\r\n@GA news entry has been made by {}, type 'news {:d}' to see it.@n\r\n", char_get_name(d->character), g->TOP_OF_NEWS);
        }
        do_reboot(g, d->character, "news", 0, 0);
      }
      break;
    case STRINGADD_ABORT:
      write_to_output(g, d, "Edit aborted.\r\n");
      act(g, "$n stops editing the news.", true, d->character, nullptr, nullptr, TO_ROOM);
      break;
    default:
      mud_log("SYSERR: news_string_cleanup: Unknown terminator status.");
      break;
  }

  /* Common cleanup code. */
  d->draft.clear();
  desc_set_connected(d, CON_PLAYING);
}

namespace {
  /* Editing one of the text files: the text editor does it all, and this writes the file out when it's saved */
  struct text_file_editor final : olc_session {
    std::string file; /* where the text is written */

    explicit text_file_editor(const char *path) : file(path) {}

    void parse(GameWorld *, struct descriptor_data *, std::string_view) override {} /* the text editor has every line */
    void text_done(GameWorld *g, struct descriptor_data *d, int action) override;
    void log_leaving(GameWorld *g, struct descriptor_data *d) const override
    {
      mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: {} stops editing text files.", char_get_name(d->character));
    }

    bool save(GameWorld *g, struct descriptor_data *d, std::string &text) const;
  };

  /* Writes the text to the file (the news with a heading); false, and the builder told, if it couldn't */
  bool text_file_editor::save(GameWorld *g, struct descriptor_data *d, std::string &text) const
  {
    const bool news = file == "text/news";
    io::text_writer fl(file);
    if (!text.empty()) {
      strip_cr(text);
      if (news) {
        time_t now        = game_now(g);
        std::string stamp = asctime(localtime(&now));
        stamp.pop_back(); /* its newline */
        fl.print("\n-----------------------------------------------\n@Y{} @cUpdated By@D: @C{}@n\r\n-----------------------------------------------\n{}\n", stamp,
                 char_get_name(d->character), text);
      } else
        fl.write(text.c_str());
    }
    if (!fl.commit()) {
      write_to_output(g, d, "Could not write the file; nothing was changed.\r\n");
      return false;
    }
    mudlog(g, LOG_CMP, ADMLVL_GOD, true, "OLC: {} saves '{}'.", char_get_name(d->character), file);
    write_to_output(g, d, "Saved.\r\n");

    if (news) {
      g->NEWSUPDATE = game_now(g);
      save_mud_time(g, &g->time_info);
      for (descriptor_data *i : each_desc(g))
        if (desc_is_playing(i) && !char_plr_flagged(i->character, PLR_WRITING) && g->NEWSUPDATE > char_get_lastpl(i->character))
          send_to_char(g, i->character, "\r\n@GThe NEWS file has been updated, type 'news' to see it.@n\r\n");
      do_reboot(g, d->character, "all", 0, 0);
    }
    return true;
  }

  void text_file_editor::text_done(GameWorld *g, struct descriptor_data *d, int action)
  {
    if (file.empty() || !d->str)
      action = STRINGADD_ABORT;

    if (action == STRINGADD_SAVE)
      save(g, d, *d->str);
    else if (action == STRINGADD_ABORT) {
      write_to_output(g, d, "Edit aborted.\r\n");
      act(g, "$n stops editing some scrolls.", true, d->character, nullptr, nullptr, TO_ROOM);
    } else
      mud_log("SYSERR: tedit_string_cleanup: Unknown terminator status.");

    leave_olc(g, d);
    desc_set_connected(d, CON_PLAYING);
  }
} // namespace

ACMD(do_tedit)
{
  int l, i = 0;
  std::string field;

  struct {
    const char *cmd{};
    char level{};
    std::string *buffer{};
    int size{};
    const char *filename{};
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

  std::string_view rest = argument;
  field                 = take_arg(rest);

  if (field.empty()) {
    send_to_char(g, ch, "Files available to be edited:\r\n");
    for (l = 0; *fields[l].cmd != '\n'; l++) {
      if (char_get_admlevel(ch) >= fields[l].level) {
        send_to_char(g, ch, "{:<11.11} ", fields[l].cmd);
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
    if (!strncmp(field.c_str(), fields[l].cmd, field.size()))
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

  open_session(g, char_get_desc(ch), std::make_unique<text_file_editor>(fields[l].filename), "do_tedit");

  if (!fields[l].buffer->empty()) {
    send_to_char(g, ch, "{}", fields[l].buffer->c_str());
  }

  string_write(char_get_desc(ch), fields[l].buffer, fields[l].size, 0);

  act(g, "$n begins editing a text file.", true, ch, nullptr, nullptr, TO_ROOM);
  char_plr_set(ch, PLR_WRITING, true);
  desc_set_connected(char_get_desc(ch), CON_TEDIT);
}
