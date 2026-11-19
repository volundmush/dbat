/*
 * The functions below this point is handling the context sensitive help system
 * If you add more olc options, be sure to add a case for it here. - Welcor
 */
#include <cstdlib>
#include <cerrno>

#include "dbat/descriptor_utils.hpp"
#include "dbat/context_help.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/oasis.hpp"
#include "dbat/dg_olc.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/comm.hpp"
#include "dbat/fileop.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/db.hpp"
#include "dbat/log.hpp"
#include "dbat/game_world.hpp"

static int find_context_gedit(const struct descriptor_data *d);
static int find_context_medit(const struct descriptor_data *d);
static int find_context_script_edit(const struct descriptor_data *d);
static int find_context_sedit(const struct descriptor_data *d);
static int find_context_zedit(const struct descriptor_data *d);

/* local global */

int legacy_help_context(const struct descriptor_data *d)
{
  switch (desc_get_connected(d)) {
    case CON_MEDIT: return find_context_medit(d);
    case CON_ZEDIT: return find_context_zedit(d);
    case CON_SEDIT: return find_context_sedit(d);
    case CON_GEDIT: return find_context_gedit(d);
    default: return NOTHING;
  }
}

static int find_context_zedit(const struct descriptor_data *d)
{
  switch (OLC_MODE(d)) {
    case ZEDIT_MAIN_MENU: return CONTEXT_ZEDIT_MAIN_MENU;
    case ZEDIT_DELETE_ENTRY: return CONTEXT_ZEDIT_DELETE_ENTRY;
    case ZEDIT_NEW_ENTRY: return CONTEXT_ZEDIT_NEW_ENTRY;
    case ZEDIT_CHANGE_ENTRY: return CONTEXT_ZEDIT_CHANGE_ENTRY;
    case ZEDIT_COMMAND_TYPE: return CONTEXT_ZEDIT_COMMAND_TYPE;
    case ZEDIT_IF_FLAG: return CONTEXT_ZEDIT_IF_FLAG;
    case ZEDIT_ARG1: return CONTEXT_ZEDIT_ARG1;
    case ZEDIT_ARG2: return CONTEXT_ZEDIT_ARG2;
    case ZEDIT_ARG3: return CONTEXT_ZEDIT_ARG3;
    case ZEDIT_ZONE_NAME: return CONTEXT_ZEDIT_ZONE_NAME;
    case ZEDIT_ZONE_LIFE: return CONTEXT_ZEDIT_ZONE_LIFE;
    case ZEDIT_ZONE_BOT: return CONTEXT_ZEDIT_ZONE_BOT;
    case ZEDIT_ZONE_TOP: return CONTEXT_ZEDIT_ZONE_TOP;
    case ZEDIT_ZONE_RESET: return CONTEXT_ZEDIT_ZONE_RESET;
    case ZEDIT_CONFIRM_SAVESTRING: return CONTEXT_ZEDIT_CONFIRM_SAVESTRING;
    case ZEDIT_SARG1: return CONTEXT_ZEDIT_SARG1;
    case ZEDIT_SARG2: return CONTEXT_ZEDIT_SARG2;
    default: return NOTHING;
  }
}

static int find_context_medit(const struct descriptor_data *d)
{
  switch (OLC_MODE(d)) {
    case MEDIT_MAIN_MENU: return CONTEXT_MEDIT_MAIN_MENU;
    case MEDIT_ALIAS: return CONTEXT_MEDIT_ALIAS;
    case MEDIT_S_DESC: return CONTEXT_MEDIT_S_DESC;
    case MEDIT_L_DESC: return CONTEXT_MEDIT_L_DESC;
    case MEDIT_D_DESC: return CONTEXT_MEDIT_D_DESC;
    case MEDIT_NPC_FLAGS: return CONTEXT_MEDIT_NPC_FLAGS;
    case MEDIT_AFF_FLAGS: return CONTEXT_MEDIT_AFF_FLAGS;
    case MEDIT_CONFIRM_SAVESTRING: return CONTEXT_MEDIT_CONFIRM_SAVESTRING;
    case MEDIT_SEX: return CONTEXT_MEDIT_SEX;
    case MEDIT_ACCURACY: return CONTEXT_MEDIT_ACCURACY;
    case MEDIT_DAMAGE: return CONTEXT_MEDIT_DAMAGE;
    case MEDIT_NDD: return CONTEXT_MEDIT_NDD;
    case MEDIT_SDD: return CONTEXT_MEDIT_SDD;
    case MEDIT_NUM_HP_DICE: return CONTEXT_MEDIT_NUM_HP_DICE;
    case MEDIT_SIZE_HP_DICE: return CONTEXT_MEDIT_SIZE_HP_DICE;
    case MEDIT_ADD_HP: return CONTEXT_MEDIT_ADD_HP;
    case MEDIT_AC: return CONTEXT_MEDIT_AC;
    case MEDIT_EXP: return CONTEXT_MEDIT_EXP;
    case MEDIT_GOLD: return CONTEXT_MEDIT_GOLD;
    case MEDIT_POS: return CONTEXT_MEDIT_POS;
    case MEDIT_DEFAULT_POS: return CONTEXT_MEDIT_DEFAULT_POS;
    case MEDIT_ATTACK: return CONTEXT_MEDIT_ATTACK;
    case MEDIT_LEVEL: return CONTEXT_MEDIT_LEVEL;
    case MEDIT_ALIGNMENT: return CONTEXT_MEDIT_ALIGNMENT;
    case OLC_SCRIPT_EDIT: return find_context_script_edit(d);
    default: return NOTHING;
  }
}

static int find_context_sedit(const struct descriptor_data *d)
{
  switch (OLC_MODE(d)) {
    case SEDIT_MAIN_MENU: return CONTEXT_SEDIT_MAIN_MENU;
    case SEDIT_CONFIRM_SAVESTRING: return CONTEXT_SEDIT_CONFIRM_SAVESTRING;
    case SEDIT_NOITEM1: return CONTEXT_SEDIT_NOITEM1;
    case SEDIT_NOITEM2: return CONTEXT_SEDIT_NOITEM2;
    case SEDIT_NOCASH1: return CONTEXT_SEDIT_NOCASH1;
    case SEDIT_NOCASH2: return CONTEXT_SEDIT_NOCASH2;
    case SEDIT_NOBUY: return CONTEXT_SEDIT_NOBUY;
    case SEDIT_BUY: return CONTEXT_SEDIT_BUY;
    case SEDIT_SELL: return CONTEXT_SEDIT_SELL;
    case SEDIT_PRODUCTS_MENU: return CONTEXT_SEDIT_PRODUCTS_MENU;
    case SEDIT_ROOMS_MENU: return CONTEXT_SEDIT_ROOMS_MENU;
    case SEDIT_NAMELIST_MENU: return CONTEXT_SEDIT_NAMELIST_MENU;
    case SEDIT_NAMELIST: return CONTEXT_SEDIT_NAMELIST;
    case SEDIT_OPEN1: return CONTEXT_SEDIT_OPEN1;
    case SEDIT_OPEN2: return CONTEXT_SEDIT_OPEN2;
    case SEDIT_CLOSE1: return CONTEXT_SEDIT_CLOSE1;
    case SEDIT_CLOSE2: return CONTEXT_SEDIT_CLOSE2;
    case SEDIT_KEEPER: return CONTEXT_SEDIT_KEEPER;
    case SEDIT_BUY_PROFIT: return CONTEXT_SEDIT_BUY_PROFIT;
    case SEDIT_SELL_PROFIT: return CONTEXT_SEDIT_SELL_PROFIT;
    case SEDIT_TYPE_MENU: return CONTEXT_SEDIT_TYPE_MENU;
    case SEDIT_DELETE_TYPE: return CONTEXT_SEDIT_DELETE_TYPE;
    case SEDIT_DELETE_PRODUCT: return CONTEXT_SEDIT_DELETE_PRODUCT;
    case SEDIT_NEW_PRODUCT: return CONTEXT_SEDIT_NEW_PRODUCT;
    case SEDIT_DELETE_ROOM: return CONTEXT_SEDIT_DELETE_ROOM;
    case SEDIT_NEW_ROOM: return CONTEXT_SEDIT_NEW_ROOM;
    case SEDIT_SHOP_FLAGS: return CONTEXT_SEDIT_SHOP_FLAGS;
    case SEDIT_NOTRADE: return CONTEXT_SEDIT_NOTRADE;
    default: return NOTHING;
  }
}

static int find_context_script_edit(const struct descriptor_data *d)
{
  return OLC(d)->script.help_context();
}

static int find_context_gedit(const struct descriptor_data *d)
{
  switch (OLC_MODE(d)) {
    case GEDIT_MAIN_MENU: return CONTEXT_GEDIT_MAIN_MENU;
    case GEDIT_CONFIRM_SAVESTRING: return CONTEXT_GEDIT_CONFIRM_SAVESTRING;
    case GEDIT_NO_CASH: return CONTEXT_GEDIT_NO_CASH;
    case GEDIT_NO_SKILL: return CONTEXT_GEDIT_NO_SKILL;
    case GEDIT_NUMERICAL_RESPONSE: return CONTEXT_GEDIT_NUMERICAL_RESPONSE;
    case GEDIT_CHARGE: return CONTEXT_GEDIT_CHARGE;
    case GEDIT_OPEN: return CONTEXT_GEDIT_OPEN;
    case GEDIT_CLOSE: return CONTEXT_GEDIT_CLOSE;
    case GEDIT_TRAINER: return CONTEXT_GEDIT_TRAINER;
    case GEDIT_NO_TRAIN: return CONTEXT_GEDIT_NO_TRAIN;
    case GEDIT_MINLVL: return CONTEXT_GEDIT_MINLVL;
    case GEDIT_SELECT_SPELLS: return CONTEXT_GEDIT_SELECT_SPELLS;
    case GEDIT_SELECT_SKILLS: return CONTEXT_GEDIT_SELECT_SKILLS;
    case GEDIT_SELECT_WPS: return CONTEXT_GEDIT_SELECT_WPS;
    case GEDIT_SELECT_LANGS: return CONTEXT_GEDIT_SELECT_LANGS;
    default: return NOTHING;
  }
}

static const char NO_HELP[]        = "No help available (yet)!\r\n";
constexpr int FIND_HELP_CHAR = '*';

bool context_help(GameWorld *g, struct descriptor_data *d, char *arg)
{
  int context = NOTHING;
  char actbuf[MAX_INPUT_LENGTH], *tmp;
  /* skip if context help isn't wanted */
  if (strncmp(arg, CONTEXT_HELP_STRING, strlen(CONTEXT_HELP_STRING)))
    return false;

  tmp = one_argument(arg, actbuf); /* the totally useless 'help' string.. */
  skip_spaces(&tmp);

  if (!*tmp)
    context = d->olc ? d->olc->help_context(d) : NOTHING;

  if (context != NOTHING && context < NUM_CONTEXTS && *g->context_help_list[context]) {
    if (*g->context_help_list[context] == FIND_HELP_CHAR) {
      copy_cstr(actbuf, g->context_help_list[context] + 1, sizeof(actbuf));
      do_help(g, d->character, actbuf, 0, 0);
    } else {
      write_to_output(g, d, "\r\n{}\r\n>  ", g->context_help_list[context]);
    }
    return true;
  }
  copy_cstr(actbuf, tmp, sizeof(actbuf));
  do_help(g, d->character, actbuf, 0, 0);
  return true;
}

/* The context help file, text/contexthelp: "#<context>" and its text up to a '~', for each context that has one */
void boot_context_help(GameWorld *g)
{
  /* init to 'no help' string */
  for (int i = 0; i < NUM_CONTEXTS; i++)
    g->context_help_list[i] = const_cast<char *>(NO_HELP); /* never freed: see below */

  io::loaded<io::text_reader> file = io::text_reader::open(CONTEXT_HELP_FILE);
  if (!file) {
    mud_log("No context help found : {}", io::why_unreadable(CONTEXT_HELP_FILE));
    return;
  }
  while (std::optional<std::string_view> line = file->line()) {
    int num = 0;
    if (!line->starts_with('#') || !io::fields(line->substr(1)).number(num) || num < 0 || num >= NUM_CONTEXTS)
      continue;
    io::loaded<std::string> text = file->tilde_string("Context sensitive help");
    if (!text) {
      mud_log("SYSERR: {}", text.error().text());
      exit(1);
    }
    /* an empty one is no help, as it was (it was a null, which showing it would crash on) */
    if (text->empty())
      continue;
    if (g->context_help_list[num] != NO_HELP)
      free(g->context_help_list[num]);
    g->context_help_list[num] = strdup(text->c_str());
  }
}

void free_context_help(GameWorld *g)
{
  int i;

  for (i = 0; i < NUM_CONTEXTS; i++)
    if (g->context_help_list[i] && g->context_help_list[i] != NO_HELP)
      free(g->context_help_list[i]);
}
