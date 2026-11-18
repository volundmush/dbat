/*
 * A player's script variables, plrvars/<bucket>/<name>.mem: a line for each global variable a script set on them,
 * "<name> <context> <value>", the name lowercased as it is read. Variables whose names start with '-' aren't saved.
 */
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "dbat/args.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/fileop.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"

/* load in a character's saved variables */
void read_saved_vars(struct char_data *ch)
{
  /* If getting to the menu from inside the game, the vars aren't removed. So let's not allocate them again. */
  if (char_get_script(ch))
    return;

  /* the script, which holds the vars, comes first: later calls to 'remote' need one */
  *char_script_slot(ch) = new script_data();

  std::optional<std::string> fn = player_path(SCRIPT_VARS_FILE, char_get_name(ch));
  io::loaded<io::text_reader> file = fn ? io::text_reader::open(*fn) : std::unexpected(io::load_error{});
  if (!file) {
    mud_log("{} had no variable file", char_get_name(ch));
    return;
  }
  while (std::optional<std::string_view> line = file->line()) {
    std::string_view rest = *line;
    std::string name      = take_word(rest);
    std::string context   = take_word(rest);
    add_var(char_get_script(ch)->global_vars, name, skip_spaces(rest), io::c_atol(context));
  }
}

/* save a characters variables out to disk */
void save_char_vars(GameWorld *g, struct char_data *ch)
{
  /* no script, and so no variables, while the player is logging in */
  if (char_get_script(ch) == nullptr || char_is_npc(ch))
    return;

  std::optional<std::string> fn = player_path(SCRIPT_VARS_FILE, char_get_name(ch));
  if (!fn)
    return;

  const dg::var_store &vars = char_get_script(ch)->global_vars;
  if (vars.empty()) {
    std::error_code ec;
    std::filesystem::remove(*fn, ec);
    return;
  }

  io::text_writer out(*fn, true);
  /* context is always zero for now */
  for (const dg::variable &v : vars)
    if (v.name[0] != '-') /* don't save if it begins with - */
      out.print("{} {:d} {}\n", v.name, v.context, v.value);
  if (!out.commit())
    mudlog(g, LOG_NRM, ADMLVL_GOD, true, "SYSERR: Could not write player variable file {}.", *fn);
}
