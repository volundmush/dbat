/*
 * The pages the game reads whole: the motd, the news, the greetings and the rest, each held as a string on GameWorld
 * and read again by "reload".
 *
 * A page is read as fgets() into a READ_SIZE buffer read it: each line, in pieces if it is longer, loses its last
 * character, as though it were the line end, and gets "\r\n" (quirk: a line in pieces loses one in each piece). A
 * page longer than MAX_STRING_LENGTH isn't read.
 */
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

#include "dbat/text_files.hpp"
#include "dbat/args.hpp"
#include "dbat/comm.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/log.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/utils.hpp"
#include "dbat/world_files.hpp"

namespace {

struct text_page {
  const char *keyword; /* "reload <keyword>" */
  const char *path;
  std::string GameWorld::*text;
  bool prune;              /* the greetings, without their blank lines at the end */
  const char *cant_read;   /* what "reload <keyword>" says if it can't be read */
  const char *all_says;    /* what "reload all" says; "" for nothing, and null if it doesn't read it */
};

/* in the order they are read at boot */
constexpr text_page pages[] = {
    {"news",       NEWS_FILE,       &GameWorld::news,       false, "Cannot read news\r\n",              "Cannot read news\r\n"            },
    {"credits",    CREDITS_FILE,    &GameWorld::credits,    false, "Cannot read credits\r\n",           "Cannot read credits\r\n"         },
    {"motd",       MOTD_FILE,       &GameWorld::motd,       false, "Cannot read motd\r\n",              "Cannot read motd\r\n"            },
    {"imotd",      IMOTD_FILE,      &GameWorld::imotd,      false, "Cannot read imotd\r\n",             "Cannot read imotd\r\n"           },
    {"help",       HELP_PAGE_FILE,  &GameWorld::help,       false, "Cannot read help front page\r\n",   "Cannot read help front page\r\n" },
    {"info",       INFO_FILE,       &GameWorld::info,       false, "Cannot read info\r\n",              "Cannot read info file\r\n"       },
    {"wizlist",    WIZLIST_FILE,    &GameWorld::wizlist,    false, "Cannot read wizlist\r\n",           "Cannot read wizlist\r\n"         },
    {"immlist",    IMMLIST_FILE,    &GameWorld::immlist,    false, "Cannot read immlist\r\n",           "Cannot read immlist\r\n"         },
    {"policy",     POLICIES_FILE,   &GameWorld::policies,   false, "Cannot read policy\r\n",            "Cannot read policies\r\n"        },
    {"handbook",   HANDBOOK_FILE,   &GameWorld::handbook,   false, "Cannot read handbook\r\n",          "Cannot read handbook\r\n"        },
    {"background", BACKGROUND_FILE, &GameWorld::background, false, "Cannot read background\r\n",        "Cannot read background\r\n"      },
    {"ihelp",      IHELP_PAGE_FILE, &GameWorld::ihelp,      false, "Cannot read help front page\r\n",   nullptr                           },
    {"greetings",  GREETINGS_FILE,  &GameWorld::GREETINGS,  true,  "Cannot read greetings.\r\n",        ""                                },
    {"greetansi",  GREETANSI_FILE,  &GameWorld::GREETANSI,  true,  "Cannot read greetings.\r\n",        ""                                },
};

/* the order "reload all" reads them in, after the levels */
constexpr const char *reload_all_order[] = {"greetings", "greetansi", "wizlist", "immlist",  "news",
                                            "credits",   "motd",      "imotd",   "help",     "info",
                                            "policy",    "handbook",  "background"};

const text_page &page_named(std::string_view keyword)
{
  return *std::ranges::find_if(pages, [&](const text_page &p) { return str_eq_nocase(p.keyword, keyword); });
}

/* The page at path, or nothing if it can't be read or is too long, as logged */
std::optional<std::string> read_page(const char *path)
{
  io::loaded<io::text_reader> file = io::text_reader::open(path);
  if (!file) {
    mud_log("SYSERR: reading {}: {}", path, io::why_unreadable(path));
    return std::nullopt;
  }
  std::string_view raw = file->text();

  std::string text;
  for (std::string_view rest = raw; !rest.empty();) {
    std::string_view line = io::chopped_piece(io::fgets_piece(rest));
    if (text.size() + line.size() + 3 > MAX_STRING_LENGTH) {
      mud_log("SYSERR: {}: string too big ({:d} max)", path, MAX_STRING_LENGTH);
      return std::nullopt;
    }
    text += line;
    text += "\r\n";
  }
  return text;
}

/* Reads the page again, leaving what it held if it can't; false if it couldn't */
bool reload_page(GameWorld *g, const text_page &page)
{
  std::optional<std::string> text = read_page(page.path);
  if (!text)
    return false;
  g->*page.text = std::move(*text);
  if (page.prune)
    prune_crlf(g->*page.text);
  return true;
}

void reload_help(GameWorld *g)
{
  free_help_table(g);
  index_boot(g, DB_BOOT_HLP);
}

} // namespace

void load_text_pages(GameWorld *g)
{
  for (const text_page &page : pages)
    reload_page(g, page);
}

/* this is necessary for the autowiz system */
void reboot_wizlists(GameWorld *g)
{
  reload_page(g, page_named("wizlist"));
  reload_page(g, page_named("immlist"));
}

ACMD(do_reboot)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (str_eq_nocase(arg, "all") || arg[0] == '*') {
    if (load_levels(g) < 0)
      send_to_char(g, ch, "Cannot read level configurations\r\n");
    for (const char *keyword : reload_all_order) {
      const text_page &page = page_named(keyword);
      if (!reload_page(g, page) && *page.all_says)
        send_to_char(g, ch, "{}", page.all_says);
    }
    reload_help(g);
  } else if (str_eq_nocase(arg, "levels")) {
    if (load_levels(g) < 0)
      send_to_char(g, ch, "Cannot read level configurations\r\n");
  } else if (str_eq_nocase(arg, "xhelp"))
    reload_help(g);
  else if (const text_page *page =
               std::ranges::find_if(pages, [&](const text_page &p) { return str_eq_nocase(p.keyword, arg); });
           page != std::end(pages)) {
    if (!reload_page(g, *page))
      send_to_char(g, ch, "{}", page->cant_read);
  } else {
    send_to_char(g, ch, "Unknown reload option.\r\n");
    return;
  }

  send_to_char(g, ch, CONFIG_OK);
}
