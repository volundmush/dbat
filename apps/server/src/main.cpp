//
// Created by basti on 10/22/2021.
//

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "dbat/config.hpp"
#include "dbat/comm.hpp"
#include "dbat/utils.hpp"
#include "dbat/mail.hpp"
#include "dbat/boards.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.social.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/ban.hpp"
#include "dbat/genolc.hpp"
#include "dbat/world_audit.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/net/platform.hpp"

namespace {

/* What the options decide that isn't kept on the game */
struct launch {
  GameWorld *g;
  std::string_view program; /* as it was run, for -h */
  std::string dir;     /* the data directory */
};

/* An option: its letter, whether it takes a value, and what it does */
struct option {
  char letter;
  enum class arity {
    nothing,
    value,    /* "-dlib" or "-d lib" */
    attached, /* only "-C5" */
  } takes;
  std::string_view missing; /* what to say, before exiting, if a value it takes isn't there; empty carries on */
  void (*apply)(launch &l, std::string_view value);
};

void print_usage(std::string_view program)
{
  /* From: Anil Mahajan <amahajan@proxicom.com> */
  file_print(stdout, "Usage: {} [-c] [-m] [-x] [-q] [-r] [-s] [-d pathname] [port #]\n"
                     "  -c             Enable syntax check mode.\n"
                     "  -d <directory> Specify library directory (defaults to 'lib').\n"
                     "  -f<file>       Use <file> for configuration.\n"
                     "  -h             Print this command line argument help.\n"
                     "  -m             Start in mini-MUD mode.\n"
                     "  -o <file>      Write log to <file> instead of stderr.\n"
                     "  -q             Quick boot (doesn't scan rent for object limits)\n"
                     "  -r             Restrict MUD -- no new players allowed.\n"
                     "  -s             Suppress special procedure assignments.\n"
                     " Note:         These arguments are 'CaSe SeNsItIvE!!!'\n"
                     "  -x             Load using secondary (ascii) files.\n",
             program);
}

using arity = option::arity;
const option OPTIONS[] = {
    {'f', arity::value, {}, [](launch &, std::string_view) {}}, /* read before the configuration is loaded */
    {'o', arity::value, "SYSERR: File name to log to expected after option -o.",
     [](launch &l, std::string_view file) {
       GameWorld *g   = l.g; /* for CONFIG_LOGNAME */
       CONFIG_LOGNAME = file;
     }},
    /* recover from copyover: this is the control socket */
    {'C', arity::attached, {},
     [](launch &l, std::string_view socket) {
       l.g->fCopyOver = true;
       l.g->listener  = net::socket(static_cast<int>(io::c_atol(socket)));
     }},
    {'d', arity::value, "SYSERR: Directory arg expected after option -d.", [](launch &l, std::string_view dir) { l.dir = dir; }},
    {'m', arity::nothing, {},
     [](launch &l, std::string_view) {
       l.g->mini_mud      = 1;
       l.g->no_rent_check = 1;
       std::puts("Running in minimized mode & with no rent check.");
     }},
    {'c', arity::nothing, {},
     [](launch &l, std::string_view) {
       l.g->scheck = 1;
       std::puts("Syntax check mode enabled.");
     }},
    {'q', arity::nothing, {},
     [](launch &l, std::string_view) {
       l.g->no_rent_check = 1;
       std::puts("Quick boot mode -- rent check supressed.");
     }},
    {'r', arity::nothing, {},
     [](launch &l, std::string_view) {
       l.g->circle_restrict = 1;
       std::puts("Restricting game -- no new players allowed.");
     }},
    {'s', arity::nothing, {},
     [](launch &l, std::string_view) {
       l.g->no_specials = 1;
       std::puts("Suppressing assignment of special routines.");
     }},
    /* before there's a log, so this says only that it can't be logged (quirk) */
    {'x', arity::nothing, {},
     [](launch &l, std::string_view) {
       l.g->xap_objs = 1;
       mud_log("Loading player objects from secondary (ascii) files.");
     }},
    {'h', arity::nothing, {},
     [](launch &l, std::string_view) {
       print_usage(l.program);
       std::exit(0);
     }},
};

/* The option an argument such as "-d" or "-dlib" is, or nullptr */
const option *option_for(std::string_view arg)
{
  char letter = arg.size() > 1 ? arg[1] : '\0';

  for (const option &o : OPTIONS)
    if (o.letter == letter)
      return &o;
  return nullptr;
}

/* The value of the option at args[pos] ("-dlib", or "-d lib", moving pos on to "lib"), or nothing if it ends the
   arguments, which exits if the option says to */
std::string_view value_of(std::span<const std::string_view> args, size_t &pos, const option &o)
{
  std::string_view arg = args[pos];

  if (arg.size() > 2 || o.takes == arity::attached)
    return arg.substr(2);
  if (++pos < args.size())
    return args[pos];
  if (!o.missing.empty()) {
    file_print(stdout, "{}\n", o.missing); /* as puts() wrote it */
    std::exit(1);
  }
  return {};
}

bool is_option(std::string_view arg) { return arg.starts_with('-'); }

/*
 * Load the game configuration. We must load BEFORE we use any of the constants stored in constants.c. Otherwise,
 * there will be no variables set to set the rest of the vars to, which will mean trouble --> Mythran
 *
 * So -f is read first, from the options before the port. This pass knows no other option, so it takes the value of
 * one such as "-d lib" for the port and stops there (quirk): "-d lib -f my.cfg" uses the default configuration.
 */
void read_config_option(GameWorld *g, std::span<const std::string_view> args)
{
  for (size_t pos = 1; pos < args.size() && is_option(args[pos]); pos++) {
    std::string_view arg = args[pos];

    if (arg.size() > 1 && arg[1] == 'f') {
      if (arg.size() > 2)
        CONFIG_CONFFILE = arg.substr(2);
      else if (++pos < args.size())
        CONFIG_CONFFILE = args[pos];
      else {
        std::puts("SYSERR: File name to read from expected after option -f.");
        std::exit(1);
      }
    }
  }
}

/* The options, then the port if there is one */
void read_options(std::span<const std::string_view> args, launch &l)
{
  size_t pos = 1;

  for (; pos < args.size() && is_option(args[pos]); pos++) {
    const option *o = option_for(args[pos]);

    if (!o) { /* said, and passed over */
      std::string_view arg = args[pos];
      file_print(stdout, "SYSERR: Unknown option -{:c} in argument string.\n", as_char(arg.size() > 1 ? arg[1] : '\0'));
      continue;
    }
    std::string_view value;
    if (o->takes != arity::nothing)
      value = value_of(args, pos, *o);
    o->apply(l, value);
  }

  if (pos < args.size()) {
    std::string_view port = args[pos];

    if (port.empty() || !std::isdigit(static_cast<unsigned char>(port[0]))) {
      file_print(stdout, "Usage: {} [-c] [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", args[0]);
      std::exit(1);
    } else if ((l.g->port = static_cast<int>(io::c_atol(port))) <= 1024) {
      file_print(stdout, "SYSERR: Illegal port number {:d}.\n", l.g->port);
      std::exit(1);
    }
  }
}

} // namespace

int main(int argc, char **argv)
{
  const std::vector<std::string_view> args(argv, argv + argc); /* the C strings the system gives, as views */
  std::error_code error;
  GameWorld *g = game_world_create();

  /*
   * Until signal_setup() runs. The deadlock timer outlives a copyover's exec, and an older server won't
   * have stopped it, so this process could otherwise die part way through booting.
   */
  net::process::stop_deadlock_timer();

  g->launch_args.assign(args.begin(), args.end());
  g->launch_dir = std::filesystem::current_path(error).string();

  read_config_option(g, args);
  if (CONFIG_CONFFILE.empty())
    CONFIG_CONFFILE = CONFIG_FILE;

  load_config(g);

  g->port = CONFIG_DFLT_PORT;
  launch l{g, args[0], CONFIG_DFLT_DIR};
  read_options(args, l);

  /* All arguments have been parsed, try to open log file. */
  setup_log(CONFIG_LOGNAME, g->fCopyOver);

  /*
     * Moved here to distinguish command line options and to show up
     * in the log if stderr is redirected to a file.
     */
  mud_log("Using {} for configuration.", CONFIG_CONFFILE);
  g->xap_objs = 1;
  std::filesystem::current_path(l.dir, error);
  if (error) {
    file_print(stderr, "SYSERR: Fatal error changing to data directory: {}\n", error.message());
    std::exit(1);
  }
  mud_log("Using {} as data directory.", l.dir);

  if (g->scheck) {
    boot_world(g);
    world_audit(g, [](std::string_view problem) { mud_log("SYSERR: World audit: {}", problem); });
  } else {
    mud_log("Running game on port {:d}.", g->port);
    init_game(g, g->port);
  }

  game_world_free_contents(g);
  game_world_destroy(g);

  mud_log("Done.");

  return (0);
}
