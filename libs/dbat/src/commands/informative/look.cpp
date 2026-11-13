/* look.c: look, examine, rooms, exits and targets
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iterator>
#include <ranges>
#include <span>
#include <utility>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/utils.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/songs.hpp"
#include "dbat/consts/time.hpp"
#include "dbat/help.hpp"
#include "dbat/command.hpp"
#include "dbat/flags.hpp"
#include "dbat/affect.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/random.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/search.hpp"
#include "dbat/relocate.hpp"
#include "dbat/fileop.hpp"
#include "dbat/races.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/extract.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/vehicles.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.social.hpp"
#include "dbat/maputils.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/boards.hpp"
#include "dbat/screen.hpp"
#include "dbat/mail.hpp"
#include "dbat/guild.hpp"
#include "dbat/disabled.hpp"
#include "dbat/game_world.hpp"

#include "informative_internal.hpp"

static void look_in_direction(GameWorld *g, struct char_data *ch, int dir);
static void look_in_obj(GameWorld *g, struct char_data *ch, const std::string &arg);
static void look_at_target(GameWorld *g, struct char_data *ch, std::string &arg, int read);
static void do_auto_exits(GameWorld *g, room_vnum target_room, struct char_data *ch, int exit_mode);
static void do_auto_exits2(GameWorld *g, room_vnum target_room, struct char_data *ch);

/* Portal appearance types */
static const char *const portal_appearance[] = {
    "All you can see is the glow of the portal.",
    "You see an image of yourself in the room - my, you are looking attractive today.",
    "All you can see is a swirling grey mist.",
    "The scene is of the surrounding countryside, but somehow blurry and lacking focus.",
    "The blackness appears to stretch on forever.",
    "Suddenly, out of the blackness a flaming red eye appears and fixes its gaze upon you.",
    "\n"};

namespace {
  /* A feature's word for each of its values, padded as it's shown. */
  struct feature_word {
    int value;
    const char *word;
  };

  constexpr feature_word HAIR_LENGTHS[] = {{HAIRL_LONG, "Long.         "}, {HAIRL_BALD, "Bald.         "}, {HAIRL_SHORT, "Short.        "},
                                           {HAIRL_MEDIUM, "Medium.       "}, {HAIRL_RLONG, "Really Long.  "}};
  constexpr feature_word HAIR_STYLES[]  = {{HAIRS_PLAIN, "Plain.        "}, {HAIRS_MOHAWK, "Mohawk.       "}, {HAIRS_SPIKY, "Spiky.        "},
                                           {HAIRS_CURLY, "Curly.        "}, {HAIRS_UNEVEN, "Uneven.       "}, {HAIRS_PONYTAIL, "Pony Tail.    "},
                                           {HAIRS_AFRO, "Afro.         "},  {HAIRS_FADE, "Fade.         "},   {HAIRS_CREW, "Crew Cut.     "},
                                           {HAIRS_FEATHERED, "Feathered.    "}, {HAIRS_DRED, "Dread Locks.  "}};
  constexpr feature_word HAIR_COLORS[]  = {{HAIRC_BLACK, "Black.        "}, {HAIRC_BROWN, "Brown.        "}, {HAIRC_BLONDE, "Blonde.       "},
                                           {HAIRC_GREY, "Grey.         "},  {HAIRC_RED, "Red.          "},   {HAIRC_ORANGE, "Orange.       "},
                                           {HAIRC_GREEN, "Green.        "}, {HAIRC_BLUE, "Blue.         "},  {HAIRC_PINK, "Pink.         "},
                                           {HAIRC_PURPLE, "Purple.       "}, {HAIRC_SILVER, "Silver.       "}, {HAIRC_CRIMSON, "Crimson.      "},
                                           {HAIRC_WHITE, "White.        "}};
  constexpr feature_word EYE_COLORS[]   = {{EYE_BLUE, "Blue.         "},  {EYE_BLACK, "Black.        "},  {EYE_GREEN, "Green.        "},
                                           {EYE_BROWN, "Brown.        "}, {EYE_RED, "Red.          "},    {EYE_AQUA, "Aqua.         "},
                                           {EYE_PINK, "Pink.         "},  {EYE_PURPLE, "Purple.       "}, {EYE_CRIMSON, "Crimson.      "},
                                           {EYE_GOLD, "Gold.         "},  {EYE_AMBER, "Amber.        "},  {EYE_EMERALD, "Emerald.      "}};
  constexpr feature_word SKIN_COLORS[]  = {{SKIN_WHITE, "White.        "}, {SKIN_TAN, "Tan.          "},   {SKIN_BLACK, "Black.        "},
                                           {SKIN_GREEN, "Green.        "}, {SKIN_ORANGE, "Orange.       "}, {SKIN_YELLOW, "Yellow.       "},
                                           {SKIN_RED, "Red.          "},   {SKIN_GREY, "Grey.         "},   {SKIN_BLUE, "Blue.         "},
                                           {SKIN_AQUA, "Aqua.         "},  {SKIN_PINK, "Pink.         "},   {SKIN_PURPLE, "Purple.       "}};
  /* horns, antennae and a majin's forelock go by hair length */
  constexpr feature_word HORN_LENGTHS[]     = {{HAIRL_BALD, "None.         "}, {HAIRL_SHORT, "Short.        "}, {HAIRL_MEDIUM, "Medium.       "},
                                               {HAIRL_LONG, "Long.         "}, {HAIRL_RLONG, "Really Long.  "}};
  constexpr feature_word ANTENNA_LENGTHS[]  = {{HAIRL_BALD, "Tiny.        "}, {HAIRL_SHORT, "Short.       "}, {HAIRL_MEDIUM, "Medium.      "},
                                               {HAIRL_LONG, "Long.        "}, {HAIRL_RLONG, "R. Long.     "}};
  constexpr feature_word FORELOCK_LENGTHS[] = {{HAIRL_BALD, "Tiny.         "}, {HAIRL_SHORT, "Short.        "}, {HAIRL_MEDIUM, "Medium.       "},
                                               {HAIRL_LONG, "Long.         "}, {HAIRL_RLONG, "R. Long.      "}};

  void feature_line(GameWorld *g, struct char_data *ch, const char *label, const char *word)
  {
    send_to_char(g, ch, "            @D[@c{:<11} @D: @W{}@D]@n\r\n", label, word);
  }

  /* The line for value, if it has one. */
  void feature_line(GameWorld *g, struct char_data *ch, const char *label, std::span<const feature_word> words, int value)
  {
    auto it = std::ranges::find(words, value, &feature_word::value);
    if (it != words.end())
      feature_line(g, ch, label, it->word);
  }

  /* A super saiyan's hair and eyes, by form */
  void super_saiyan_looks(GameWorld *g, struct char_data *ch, struct char_data *tch)
  {
    if (char_plr_flagged(tch, PLR_TRANS1) || char_plr_flagged(tch, PLR_TRANS2)) {
      feature_line(g, ch, "Hair Length", HAIR_LENGTHS, char_get_hairl(tch));
      feature_line(g, ch, "Hair Style", char_plr_flagged(tch, PLR_TRANS1) ? "Spiky.        " : "Sharp Spikes. ");
      feature_line(g, ch, "Hair Color", "Golden.       ");
      feature_line(g, ch, "Eye Color", "Emerald.      ");
    } else if (char_plr_flagged(tch, PLR_TRANS3)) {
      feature_line(g, ch, "Hair Length", "Really Long.  ");
      feature_line(g, ch, "Hair Style", "Spiky.        ");
      feature_line(g, ch, "Hair Color", "Golden.       ");
      feature_line(g, ch, "Eye Color", "Aqua Green.   ");
    } else if (char_plr_flagged(tch, PLR_TRANS4)) {
      feature_line(g, ch, "Hair Length", "Long.        ");
      feature_line(g, ch, "Hair Style", "Soft Spikes. ");
      feature_line(g, ch, "Hair Color", "Black.       ");
      feature_line(g, ch, "Eye Color", "Amber.       ");
    }
  }
} // namespace

/* Used when checking status or looking at a character */
void bringdesc(GameWorld *g, struct char_data *ch, struct char_data *tch)
{
  if (!(ch != nullptr && tch != nullptr && char_is_humanoid(tch))) {
    if (char_is_humanoid(tch))
      send_to_char(g, ch, "Error in bring-desc, please report.\r\n");
    return;
  }

  if (ch != tch && char_plr_flagged(tch, PLR_DISGUISED)) {
    feature_line(g, ch, "Hair Length", "Hidden.         ");
    feature_line(g, ch, "Hair Color", "Hidden.         ");
    feature_line(g, ch, "Hair Style", "Hidden.         ");
    feature_line(g, ch, "Eye Color", "Hidden.         ");
    feature_line(g, ch, "Skin Color", SKIN_COLORS, char_get_skin(tch));
    return;
  }

  bool saiyan_like = char_is_saiyan(tch) || char_is_halfbreed(tch);
  bool super       = saiyan_like && char_is_transformed(tch);
  bool hairy = char_is_human(tch) || char_is_saiyan(tch) || char_is_konatsu(tch) || char_is_mutant(tch) || char_is_android(tch) || char_is_kai(tch) ||
               char_is_halfbreed(tch) || char_is_truffle(tch) || char_is_hoshijin(tch);
  if (hairy && super) {
    super_saiyan_looks(g, ch, tch);
  } else if (hairy) {
    feature_line(g, ch, "Hair Length", HAIR_LENGTHS, char_get_hairl(tch));
    feature_line(g, ch, "Hair Style", HAIR_STYLES, char_get_hairs(tch));
    feature_line(g, ch, "Hair Color", HAIR_COLORS, char_get_hairc(tch));
  }
  if (char_is_demon(tch) || char_is_icer(tch))
    feature_line(g, ch, "Horn Length", HORN_LENGTHS, char_get_hairl(tch));
  if (char_is_namek(tch) || char_is_arlian(tch))
    feature_line(g, ch, "Ant. Length", ANTENNA_LENGTHS, char_get_hairl(tch));
  if (char_is_arlian(tch) && char_is_female(tch))
    feature_line(g, ch, "Wing Color", HAIR_COLORS, char_get_hairc(tch));
  else if (char_is_arlian(tch))
    feature_line(g, ch, "Wing Color", "White.        ");
  if (char_is_majin(tch))
    feature_line(g, ch, "For. Length", FORELOCK_LENGTHS, char_get_hairl(tch));
  if (!super)
    feature_line(g, ch, "Eye Color", EYE_COLORS, char_get_eye(tch));
  feature_line(g, ch, "Skin Color", SKIN_COLORS, char_get_skin(tch));
  if (char_get_majinize(tch) != 0 && char_get_majinize(tch) != 3)
    send_to_char(g, ch, "            @D[@cForehead    @D: @mMajin Symbol  @D]@n\r\n");
}

namespace {
  /* The order exits are listed in, and each direction's name in capitals, for the error about one. */
  constexpr int EXIT_ORDER[] = {NORTHWEST, NORTH, NORTHEAST, EAST, SOUTHEAST, SOUTH, SOUTHWEST, WEST, UP, DOWN, INDIR, OUTDIR};
  constexpr const char *DIR_SHOUTED[NUM_OF_DIRS] = {"NORTH", "EAST", "SOUTH", "WEST", "UP", "DOWN", "NORTHWEST", "NORTHEAST", "SOUTHEAST", "SOUTHWEST", "INSIDE", "OUTSIDE"};

  /* What an immortal is told of an exit: where it goes, and of its door if it has one. False if its door has no
     keyword to name it by. */
  bool exit_seen_by_immortal(GameWorld *g, struct char_data *ch, struct room_direction_data *exit, const std::string &dir, int door, std::string &line)
  {
    line = str_format("@c{:<9} @D- [@Y{:>5d}@D]@w {}.\r\n", dir, exit->to_room, room_get_name(g, exit->to_room));
    if (!IS_SET(exit->exit_info, EX_ISDOOR) && !IS_SET(exit->exit_info, EX_SECRET))
      return true;
    /* This exit has a door - tell all about it, if it has a keyword to name it by */
    if (exit_keyword(exit) == nullptr) {
      send_to_char(g, ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION {}@n\r\n", DIR_SHOUTED[door]);
      mud_log("ERROR: {} found error direction {} at room {:d}", char_get_name(ch), DIR_SHOUTED[door], char_get_in_room(ch));
      return false;
    }
    std::string argh = str_format("{} ", strcasecmp(fname(exit_keyword(exit)), "undefined") ? fname(exit_keyword(exit)) : "opening");
    line += str_format("                    The {}{} {} {} {}{}.\r\n", IS_SET(exit->exit_info, EX_SECRET) ? "@rsecret@w " : "",
                       (exit_keyword(exit) && strcasecmp(fname(exit_keyword(exit)), "undefined")) ? fname(exit_keyword(exit)) : "opening",
                       strstr(argh.c_str(), "s ") != nullptr ? "are" : "is", IS_SET(exit->exit_info, EX_CLOSED) ? "closed" : "open",
                       IS_SET(exit->exit_info, EX_LOCKED) ? "and locked" : "and unlocked", IS_SET(exit->exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
    return true;
  }

  /* What a mortal is told of an exit: where an open one goes, or that a door is closed, if they can see it. */
  std::string exit_seen_by_mortal(GameWorld *g, struct char_data *ch, struct room_direction_data *exit, const std::string &dir, bool has_light)
  {
    if (!IS_SET(exit->exit_info, EX_CLOSED))
      return str_format("@c{:<9} @D-@w {}\r\n", dir,
                        room_is_dark(g, exit->to_room) && !char_can_see_in_dark(g, ch) && !has_light ? "@bToo dark to tell.@w" : room_get_name(g, exit->to_room));
    if (CONFIG_DISP_CLOSED_DOORS && !IS_SET(exit->exit_info, EX_SECRET))
      return str_format("@c{:<9} @D-@w The {} appears @rclosed.@n\r\n", dir, exit_keyword(exit) ? fname(exit_keyword(exit)) : "opening");
    return {};
  }

  /* Which of ch's buoys float here */
  void buoys_here(GameWorld *g, struct char_data *ch, room_vnum room)
  {
    int here = (char_get_radar1(ch) == room) + (char_get_radar2(ch) == room) + (char_get_radar3(ch) == room);
    if (here == 2)
      send_to_char(g, ch, "@CTwo of your buoys are floating here.@n\r\n");
    else if (here == 3 && room != 0)
      send_to_char(g, ch, "@CAll three of your buoys are floating here. Why?@n\r\n");
    else if (char_get_radar1(ch) == room)
      send_to_char(g, ch, "@CYour @cBuoy #1@C is floating here.@n\r\n");
    else if (char_get_radar2(ch) == room)
      send_to_char(g, ch, "@CYour @cBuoy #2@C is floating here.@n\r\n");
    else if (char_get_radar3(ch) == room)
      send_to_char(g, ch, "@CYour @cBuoy #3@C is floating here.@n\r\n");
  }
} // namespace

static void do_auto_exits(GameWorld *g, room_vnum target_room, struct char_data *ch, int exit_mode)
{
  if (exit_mode == EXIT_OFF)
    send_to_char(g, ch, "@D------------------------------------------------------------------------@n\r\n");
  bool space = room_sect(g, target_room) == SECT_SPACE && target_room >= 20000;
  if (exit_mode == EXIT_NORMAL && !space && char_get_in_room(ch) == target_room) {
    /* Compass and Auto-map - Iovan 9-11-10 */
    send_to_char(g, ch, "@D------------------------------------------------------------------------@n\r\n");
    send_to_char(g, ch, "@w      Compass           Auto-Map            Map Key\r\n");
    send_to_char(g, ch, "@R     ---------         ----------   -----------------------------\r\n");
    gen_map(g, ch, 0);
    send_to_char(g, ch, "@D------------------------------------------------------------------------@n\r\n");
  }
  if (exit_mode == EXIT_NORMAL && space) {
    send_to_char(g, ch, "@D------------------------------[@CRadar@D]---------------------------------@n\r\n");
    printmap(g, target_room, ch, 1, -1);
    send_to_char(g, ch, "     @D[@wTurn autoexit complete on for directions instead of radar@D]@n\r\n");
    send_to_char(g, ch, "@D------------------------------------------------------------------------@n\r\n");
  }
  if (exit_mode != EXIT_COMPLETE && !(exit_mode == EXIT_NORMAL && !space && char_get_in_room(ch) != target_room))
    return;

  send_to_char(g, ch, "@D----------------------------[@gObvious Exits@D]-----------------------------@n\r\n");
  if (char_is_affected(ch, AFF_BLIND)) {
    send_to_char(g, ch, "You can't see a damned thing, you're blind!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_EYEC)) {
    send_to_char(g, ch, "You can't see a damned thing, your eyes are closed!\r\n");
    return;
  }
  if (room_is_dark(g, char_get_in_room(ch)) && !char_can_see_in_dark(g, ch) && !char_plr_flagged(ch, PLR_AURALIGHT)) {
    send_to_char(g, ch, "It is pitch black...\r\n");
    return;
  }

  /* Is the character using a working light source? */
  bool has_light = std::ranges::any_of(std::views::iota(0, NUM_WEARS), [&](int i) {
    struct obj_data *eq = char_get_equipment(ch, i);
    return eq && obj_get_type_flag(eq) == ITEM_LIGHT && obj_get_value(eq, VAL_LIGHT_HOURS);
  }) || char_plr_flagged(ch, PLR_AURALIGHT);
  bool sees_all = char_adm_flagged(ch, ADM_SEESECRET) || char_get_admlevel(ch) > 4; /* Immortals see everything */

  std::array<std::string, NUM_OF_DIRS> lines;
  int door_found = 0;
  for (int door = 0; door < NUM_OF_DIRS; door++) {
    struct room_direction_data *exit = room_get_dir_option(g, target_room, door);
    if (!exit || exit->to_room == NOWHERE)
      continue;
    std::string dir = dirs[door];
    CAP(dir);
    if (sees_all) {
      door_found++;
      if (!exit_seen_by_immortal(g, ch, exit, dir, door, lines[door]))
        return;
    } else if (!(lines[door] = exit_seen_by_mortal(g, ch, exit, dir, has_light)).empty()) {
      door_found++;
    }
  }
  if (!door_found)
    send_to_char(g, ch, " None.\r\n");
  for (int door : EXIT_ORDER)
    if (!lines[door].empty())
      send_to_char(g, ch, lines[door]);
  send_to_char(g, ch, "@D------------------------------------------------------------------------@n\r\n");

  if (room_flagged(g, target_room, ROOM_HOUSE)) {
    bool garden1 = room_flagged(g, target_room, ROOM_GARDEN1), garden2 = room_flagged(g, target_room, ROOM_GARDEN2);
    if (!garden1 && !garden2)
      send_to_char(g, ch, "@D[@GItems Stored@D: @g{:d}@D]@n\r\n", check_saveroom_count(g, ch, nullptr));
    else if (garden1 && !garden2)
      send_to_char(g, ch, "@D[@GPlants Planted@D: @g{:d}@W, @GMAX@D: @R8@D]@n\r\n", check_saveroom_count(g, ch, nullptr));
    else if (!garden1 && garden2)
      send_to_char(g, ch, "@D[@GPlants Planted@D: @g{:d}@W, @GMAX@D: @R20@D]@n\r\n", check_saveroom_count(g, ch, nullptr));
  }
  buoys_here(g, ch, target_room);
}

static void do_auto_exits2(GameWorld *g, room_vnum target_room, struct char_data *ch)
{
  send_to_char(g, ch, "\nExits: ");
  int shown = 0;
  for (int door = 0; door < NUM_OF_DIRS; door++) {
    struct room_direction_data *exit = room_get_dir_option(g, target_room, door);
    if (!exit || exit->to_room == NOWHERE || room_exit_flagged(exit, EX_CLOSED))
      continue;
    send_to_char(g, ch, "{} ", abbr_dirs[door]);
    shown++;
  }
  send_to_char(g, ch, "{}\r\n", shown ? "" : "None!");
}

ACMD(do_exits)
{
  if (!char_prf_flagged(g, ch, PRF_NODEC))
    do_auto_exits(g, char_get_in_room(ch), ch, EXIT_COMPLETE);
  else
    do_auto_exits2(g, char_get_in_room(ch), ch);
}

static const char *const exitlevels[] = {"off", "normal", "n/a", "complete", "\n"};

ACMD(do_autoexit)
{
  if (char_is_npc(ch))
    return;

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Your current autoexit level is {}.\r\n", exitlevels[char_exit_level(g, ch)]);
    return;
  }
  int tp = search_block(arg, exitlevels, false);
  if (tp == -1) {
    send_to_char(g, ch, "Usage: Autoexit { Off | Normal | Complete }\r\n");
    return;
  }
  if (tp == EXIT_OFF || tp == EXIT_NORMAL || tp == EXIT_COMPLETE) { /* "n/a" changes nothing */
    char_prf_set(g, ch, PRF_AUTOEXIT, tp != EXIT_OFF);
    char_prf_set(g, ch, PRF_FULL_EXIT, tp == EXIT_COMPLETE);
  }
  send_to_char(g, ch, "Your @rautoexit level@n is now {}.\r\n", exitlevels[char_exit_level(g, ch)]);
}

namespace {
  template <int Flag> bool room_is(GameWorld *g, room_vnum room) { return room_flagged(g, room, Flag); }

  struct place_line {
    bool (*is)(GameWorld *g, room_vnum room);
    const char *says;
  };

  /* Where a room is, the first that it's in */
  constexpr place_line PLACES[] = {
      {room_is<ROOM_EARTH>,   "Planet: @GEarth"},
      {room_is<ROOM_CERRIA>,  "Planet: @RCerria"},
      {room_planet_zenith,    "Planet: @BZenith"},
      {room_is<ROOM_AETHER>,  "Planet: @MAether"},
      {room_is<ROOM_FRIGID>,  "Planet: @CFrigid"},
      {room_is<ROOM_SPACE>,   "Planet: @DNone"},
      {room_is<ROOM_VEGETA>,  "Planet: @YVegeta"},
      {room_is<ROOM_NAMEK>,   "Planet: @gNamek"},
      {room_is<ROOM_KONACK>,  "Planet: @MKonack"},
      {room_is<ROOM_NEO>,     "Planet: @WNeo Nirvana"},
      {room_is<ROOM_AL>,      "Dimension: @yA@Yf@yt@Ye@yr@Yl@yi@Yf@ye"},
      {room_is<ROOM_HELL>,    "Dimension: @RPunishment Hell"},
      {room_is<ROOM_RHELL>,   "Dimension: @RH@re@Dl@Rl"},
      {room_is<ROOM_YARDRAT>, "Planet: @mYardrat"},
      {room_is<ROOM_KANASSA>, "Planet: @BKanassa"},
      {room_is<ROOM_ARLIA>,   "Planet: @GArlia"},
  };

  /* The gravities a room is told to have; any other says nothing */
  constexpr std::pair<int, const char *> GRAVITIES[] = {{10, "10x"},   {20, "20x"},   {30, "30x"},       {40, "40x"},       {50, "50x"},
                                                        {100, "100x"}, {200, "200x"}, {300, "300x"},     {400, "400x"},     {500, "500x"},
                                                        {1000, "1,000x"}, {5000, "5,000x"}, {10000, "10,000x"}};

  /* How a room's damage shows, by its terrain: at most 2, 4, 6, 10, 20, 30, 50, 75 and 99, then anything more */
  constexpr int DAMAGE_STEPS[] = {2, 4, 6, 10, 20, 30, 50, 75, 99};
  constexpr const char *FLOOR_DAMAGE[] = {
      "@wA small hole with chunks of debris that can be seen scarring the floor.@n",
      "@wA couple small holes with chunks of debris that can be seen scarring the floor.@n",
      "@wA few small holes with chunks of debris that can be seen scarring the floor.@n",
      "@wThere are several small holes with chunks of debris that can be seen scarring the floor.@n",
      "@wMany holes fill the floor of this area, many of which have burn marks.@n",
      "@wThe floor is severely damaged with many large holes.@n",
      "@wBattle damage covers the entire area. Displayed as a tribute to the battles that have\r\nbeen waged here.@n",
      "@wThis entire area is falling apart, it has been damaged so badly.@n",
      "@wThis area can not withstand much more damage. Everything has been damaged so badly it\r\nis hard to recognise any particular details about their former quality.@n",
      "@wThis area is completely destroyed. Nothing is recognisable. Chunks of debris\r\nlitter the ground, filling up holes, and overflowing onto what is left of the\r\nfloor. A haze of smoke is wafting through the air, creating a chilling atmosphere..@n",
  };
  constexpr const char *GROUND_DAMAGE[] = {
      "@wA small hole with chunks of debris that can be seen scarring the ground.@n",
      "@wA couple small craters with chunks of debris that can be seen scarring the ground.@n",
      "@wA few small craters with chunks of debris that can be seen scarring the ground.@n",
      "@wThere are several small craters with chunks of debris that can be seen scarring the ground.@n",
      "@wMany craters fill the ground of this area, many of which have burn marks.@n",
      "@wThe ground is severely damaged with many large craters.@n",
      "@wBattle damage covers the entire area. Displayed as a tribute to the battles that have\r\nbeen waged here.@n",
      "@wThis entire area is falling apart, it has been damaged so badly.@n",
      "@wThis area can not withstand much more damage. Everything has been damaged so badly it\r\nis hard to recognise any particular details about their former quality.@n",
      "@wThis area is completely destroyed. Nothing is recognisable. Chunks of debris\r\nlitter the ground, filling up craters, and overflowing onto what is left of the\r\nground. A haze of smoke is wafting through the air, creating a chilling atmosphere..@n",
  };
  constexpr const char *FOREST_DAMAGE[] = {
      "@wA small tree sits in a little crater here.@n",
      "@wTrees have been uprooted by craters in the ground.@n",
      "@wSeveral trees have been reduced to chunks of debris and are\r\nlaying in a few craters here. @n",
      "@wA large patch of trees have been destroyed and are laying in craters here.@n",
      "@wSeveral craters have merged into one large crater in one part of this forest.@n",
      "@wThe open sky can easily be seen through a hole of trees destroyed\r\nand resting at the bottom of several craters here.@n",
      "@wA good deal of burning tree pieces can be found strewn across the cratered ground here.@n",
      "@wVery few trees are left standing in this area, replaced instead by large craters.@n",
      "@wSingle solitary trees can be found still standing here or there in the area.\r\nThe rest have been almost completely obliterated in recent conflicts.@n",
      "@w  One massive crater fills this area. This desolate crater leaves no\r\nevidence of what used to be found in the area. Smoke slowly wafts into\r\nthe sky from the central point of the crater, creating an oppressive\r\natmosphere.@n",
  };
  constexpr const char *MOUNTAIN_DAMAGE[] = {
      "@wA small crater has been burned into the side of this mountain.@n",
      "@wA couple craters have been burned into the side of this mountain.@n",
      "@wBurned bits of boulders can be seen lying at the bottom of a few nearby craters.@n",
      "@wSeveral bad craters can be seen in the side of the mountain here.@n",
      "@wLarge boulders have rolled down the mountain side and collected in many nearby craters.@n",
      "@wMany craters are covering the mountainside here.@n",
      "@wThe mountain side has partially collapsed, shedding rubble down towards its base.@n",
      "@wA peak of the mountain has been blown off, leaving behind a smoldering tip.@n",
      "@wThe mountain side here has completely collapsed, shedding dangerous rubble down to its base.@n",
      "@w  Half the mountain has been blown away, leaving a scarred and jagged\r\nrock in its place. Billowing smoke wafts up from several parts of the\r\nmountain, filling the nearby skies and blotting out the sun.@n",
  };

  const char *const *damage_shown(int sect)
  {
    switch (sect) {
      case SECT_INSIDE:
        return FLOOR_DAMAGE;
      case SECT_CITY:
      case SECT_FIELD:
      case SECT_HILLS:
      case SECT_IMPORTANT:
        return GROUND_DAMAGE;
      case SECT_FOREST:
        return FOREST_DAMAGE;
      case SECT_MOUNTAIN:
        return MOUNTAIN_DAMAGE;
      default:
        return nullptr;
    }
  }

  void room_header(GameWorld *g, struct char_data *ch, room_vnum target_room)
  {
    bool decorated = !char_is_npc(ch) && !char_prf_flagged(g, ch, PRF_NODEC);
    if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_ROOMFLAGS)) {
      struct room_data *rm = room_of(g, char_get_in_room(ch)); /* quirk: the sector and triggers are those of the room ch is in */
      std::string flags    = sprintbitarray(room_get_room_flags(g, target_room), room_bits, RF_ARRAY_MAX);
      std::string sector   = sprinttype(room_get_sector_type(rm), sector_types);
      if (decorated)
        send_to_char(g, ch, "\r\n@wO----------------------------------------------------------------------O@n\r\n");
      send_to_char(g, ch, "@wLocation: @G{:<70}@w\r\n", room_get_name(g, target_room));
      if (room_get_script(rm)) {
        send_to_char(g, ch, "@D[@GTriggers");
        for (trig_data *t = room_get_script(rm)->trig_list; t; t = t->next)
          send_to_char(g, ch, " {:d}", trig_get_vnum(g, t));
        send_to_char(g, ch, "@D] ");
      }
      send_to_char(g, ch, "@wFlags: {:<70}@w\r\n", str_format("@D[ @G{}@D] @wSector: @D[ @G{} @D] @wVnum: @D[@G{:>5d}@D]@n", flags, sector, target_room));
      if (decorated)
        send_to_char(g, ch, "@wO----------------------------------------------------------------------O@n\r\n");
      return;
    }

    if (decorated)
      send_to_char(g, ch, "@wO----------------------------------------------------------------------O@n\r\n");
    send_to_char(g, ch, "@wLocation: {:<70}@n\r\n", room_get_name(g, target_room));
    auto place = std::ranges::find_if(PLACES, [&](const place_line &p) { return p.is(g, target_room); });
    send_to_char(g, ch, "@w{}@n\r\n", place == std::end(PLACES) ? "Planet: @WUNKNOWN" : place->says);
    int gravity = room_get_gravity(g, target_room);
    if (gravity <= 0)
      send_to_char(g, ch, "@wGravity: @WNormal@n\r\n");
    else if (auto it = std::ranges::find(GRAVITIES, gravity, &std::pair<int, const char *>::first); it != std::end(GRAVITIES))
      send_to_char(g, ch, "@wGravity: @W{}@n\r\n", it->second);
    if (room_flagged(g, target_room, ROOM_REGEN))
      send_to_char(g, ch, "@CA feeling of calm and relaxation fills this room.@n\r\n");
    if (room_flagged(g, target_room, ROOM_AURA))
      send_to_char(g, ch, "@GAn aura of @gregeneration@G surrounds this area.@n\r\n");
    if (room_flagged(g, target_room, ROOM_HBTC))
      send_to_char(g, ch, "@rThis room feels like it opperates in a different time frame.@n\r\n");
    if (decorated)
      send_to_char(g, ch, "@wO----------------------------------------------------------------------O@n\r\n");
  }

  void room_description(GameWorld *g, struct char_data *ch, room_vnum target_room)
  {
    int dmg  = room_get_dmg(g, target_room);
    int sect = room_sect(g, target_room);
    if (dmg <= 99)
      send_to_char(g, ch, "@w{}@n", room_get_description(g, target_room));
    if (dmg == 100 && (sect == SECT_WATER_SWIM || room_sunken(g, target_room) || sect == SECT_FLYING || sect == SECT_SHOP || sect == SECT_IMPORTANT))
      send_to_char(g, ch, "@w{}@n", room_get_description(g, target_room));
    if (const char *const *shown = damage_shown(sect); shown && dmg > 0) {
      auto step = std::ranges::find_if(DAMAGE_STEPS, [&](int most) { return dmg <= most; });
      send_to_char(g, ch, "\r\n");
      send_to_char(g, ch, shown[step - std::begin(DAMAGE_STEPS)]);
      send_to_char(g, ch, "\r\n");
    }
    int geffect = room_get_geffect(g, target_room);
    if (geffect >= 1 && geffect <= 5)
      send_to_char(g, ch, "@rLava@w is pooling in someplaces here...@n\r\n");
    if (geffect >= 6)
      send_to_char(g, ch, "@RLava@r covers pretty much the entire area!@n\r\n");
    if (geffect < 0)
      send_to_char(g, ch, "@cThe entire area is flooded with a @Cmystical@c cube of @Bwater!@n\r\n");
  }
} // namespace

void look_at_room(GameWorld *g, room_vnum target_room, struct char_data *ch, int ignore_brief)
{
  (void)ignore_brief;
  if (!char_get_desc(ch))
    return;
  if (room_is_dark(g, target_room) && !char_can_see_in_dark(g, ch) && !char_plr_flagged(ch, PLR_AURALIGHT)) {
    send_to_char(g, ch, "It is pitch black...\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_BLIND)) {
    send_to_char(g, ch, "You see nothing but infinite darkness...\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_EYEC)) {
    send_to_char(g, ch, "You can't see a damned thing, your eyes are closed!\r\n");
    return;
  }

  room_header(g, ch, target_room);
  if ((!char_is_npc(ch) && !char_prf_flagged(g, ch, PRF_BRIEF)) || room_flagged(g, target_room, ROOM_DEATH))
    room_description(g, ch, target_room);

  /* autoexits */
  if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_NODEC))
    do_auto_exits2(g, target_room, ch);
  if (!char_is_npc(ch) && !char_prf_flagged(g, ch, PRF_NODEC))
    do_auto_exits(g, target_room, ch, char_exit_level(g, ch));

  /* now list characters & objects */
  if (room_flagged(g, target_room, ROOM_GARDEN1))
    send_to_char(g, ch, "@D[@GPlants Planted@D: @g{:d}@W, @GMAX@D: @R8@D]@n\r\n", check_saveroom_count(g, ch, nullptr));
  else if (room_flagged(g, target_room, ROOM_GARDEN2))
    send_to_char(g, ch, "@D[@GPlants Planted@D: @g{:d}@W, @GMAX@D: @R20@D]@n\r\n", check_saveroom_count(g, ch, nullptr));
  else if (room_flagged(g, target_room, ROOM_HOUSE))
    send_to_char(g, ch, "@D[@GItems Stored@D: @g{:d}@D]@n\r\n", check_saveroom_count(g, ch, nullptr));
  list_obj_to_char(g, room_get_contents(g, target_room), ch, SHOW_OBJ_LONG, false);
  list_char_to_char(g, room_get_people(g, target_room), ch);
}

static void look_in_direction(GameWorld *g, struct char_data *ch, int dir)
{
  struct room_direction_data *exit = char_get_exit(g, ch, dir);
  if (!exit) {
    send_to_char(g, ch, "Nothing special there...\r\n");
    return;
  }
  if (exit_description(exit)) {
    send_to_char(g, ch, "{}", exit_description(exit));
  } else if (!find_first(char_inventory(g, ch), [&](obj_data *o) { return obj_get_vnum(g, o) == 17; })) {
    send_to_char(g, ch, "You were unable to discern anything about that direction. Try looking again...\r\n");
    obj_to_char(g, read_object(g, 17), ch);
  }

  if (room_exit_flagged(exit, EX_ISDOOR) && exit_keyword(exit)) {
    if (!room_exit_flagged(exit, EX_SECRET) && room_exit_flagged(exit, EX_CLOSED))
      send_to_char(g, ch, "The {} is closed.\r\n", fname(exit_keyword(exit)));
    else if (!room_exit_flagged(exit, EX_CLOSED))
      send_to_char(g, ch, "The {} is open.\r\n", fname(exit_keyword(exit)));
  }
}

namespace {
  /* Looking through a portal that lets one: what's at the other end */
  void look_through_portal(GameWorld *g, struct char_data *ch, struct obj_data *obj)
  {
    int appear = obj_get_value(obj, VAL_PORTAL_APPEAR);
    if (appear >= 0) {
      /* the appearance it has, or a default we shouldn't really get */
      send_to_char(g, ch, "{}\r\n", appear < MAX_PORTAL_TYPES ? portal_appearance[appear] : "All you can see is the glow of the portal.");
      return;
    }
    room_vnum portal_dest = obj_get_value(obj, VAL_PORTAL_DEST);
    if (!room_of(g, portal_dest) || (room_is_dark(g, portal_dest) && !char_can_see_in_dark(g, ch) && !char_plr_flagged(ch, PLR_AURALIGHT)))
      send_to_char(g, ch, "You see nothing but infinite darkness...\r\n");
    else
      send_to_char(g, ch, "After seconds of concentration you see the image of {}.\r\n", room_get_name(g, portal_dest));
  }

  void look_in_vehicle(GameWorld *g, struct char_data *ch, struct obj_data *obj)
  {
    if (obj_val_flagged(obj, CONT_CLOSED)) {
      send_to_char(g, ch, "It is closed.\r\n");
      return;
    }
    room_vnum inside = obj_get_value(obj, VAL_VEHICLE_ROOM);
    if (obj_get_value(obj, VAL_VEHICLE_APPEAR) >= 0 || !room_of(g, inside))
      send_to_char(g, ch, "You cannot see inside that.\r\n");
    else if (room_is_dark(g, inside) && !char_can_see_in_dark(g, ch) && !char_plr_flagged(ch, PLR_AURALIGHT))
      send_to_char(g, ch, "It is pitch black...\r\n");
    else {
      send_to_char(g, ch, "You look inside and see:\r\n");
      look_at_room(g, inside, ch, 0);
    }
  }

  /* The containers whose looking in is seen */
  constexpr obj_vnum WATCHED_CONTAINERS[] = {697, 698, 682, 683, 684};

  void look_in_container(GameWorld *g, struct char_data *ch, struct obj_data *obj, int bits)
  {
    if (obj_val_flagged(obj, CONT_CLOSED)) {
      send_to_char(g, ch, "It is closed.\r\n");
      return;
    }
    send_to_char(g, ch, "{}", obj_get_short_description(obj));
    if (obj_get_type_flag(obj) == ITEM_CONTAINER && std::ranges::contains(WATCHED_CONTAINERS, obj_get_vnum(g, obj)))
      act(g, "$n looks in $p.", true, ch, obj, nullptr, TO_ROOM);
    if (bits == FIND_OBJ_INV)
      send_to_char(g, ch, " (carried): \r\n");
    else if (bits == FIND_OBJ_ROOM)
      send_to_char(g, ch, " (here): \r\n");
    else if (bits == FIND_OBJ_EQUIP)
      send_to_char(g, ch, " (used): \r\n");
    list_obj_to_char(g, obj_get_contains(obj), ch, SHOW_OBJ_SHORT, true);
  }

  /* How full a drink container or fountain is */
  void look_in_drink(GameWorld *g, struct char_data *ch, struct obj_data *obj)
  {
    int capacity = obj_get_value(obj, VAL_DRINKCON_CAPACITY);
    int leftin   = obj_get_value(obj, VAL_DRINKCON_HOWFULL);
    if (leftin <= 0 && capacity == 0) {
      send_to_char(g, ch, "It is empty.\r\n");
      return;
    }
    if (capacity >= 0 && leftin > capacity) {
      send_to_char(g, ch, "Its contents seem somewhat murky.\r\n"); /* BUG */
      return;
    }
    std::string liquid = sprinttype(obj_get_value(obj, VAL_DRINKCON_LIQUID), color_liquid);
    if (capacity < 0 || leftin == capacity)
      send_to_char(g, ch, "It's full of a {} liquid.\r\n", liquid);
    else if (leftin >= capacity * .8)
      send_to_char(g, ch, "It's almost full of a {} liquid.\r\n", liquid);
    else if (leftin >= capacity * .5)
      send_to_char(g, ch, "It's about half full of a {} liquid.\r\n", liquid);
    else if (leftin >= capacity * .2)
      send_to_char(g, ch, "It's less than half full of a {} liquid.\r\n", liquid);
    else if (leftin > 0)
      send_to_char(g, ch, "It's barely filled with a {} liquid.\r\n", liquid);
    else
      send_to_char(g, ch, "It's empty.\r\n");
  }
} // namespace

static void look_in_obj(GameWorld *g, struct char_data *ch, const std::string &arg)
{
  if (arg.empty()) {
    send_to_char(g, ch, "Look in what?\r\n");
    return;
  }
  struct obj_data *obj    = nullptr;
  struct char_data *dummy = nullptr;
  int bits                = generic_find(g, arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &obj);
  if (!bits) {
    send_to_char(g, ch, "There doesn't seem to be {} {} here.\r\n", AN(arg.c_str()), arg);
    return;
  }

  int type = obj_get_type_flag(obj);
  if (type == ITEM_PORTAL && !obj_val_flagged(obj, CONT_CLOSEABLE))
    look_through_portal(g, ch, obj);
  else if (type == ITEM_VEHICLE)
    look_in_vehicle(g, ch, obj);
  else if (type == ITEM_WINDOW)
    look_out_window(g, ch, arg);
  else if (type == ITEM_CONTAINER || type == ITEM_PORTAL)
    look_in_container(g, ch, obj, bits);
  else if (type == ITEM_DRINKCON || type == ITEM_FOUNTAIN)
    look_in_drink(g, ch, obj);
  else
    send_to_char(g, ch, "There's nothing inside that!\r\n");
}

const char *find_exdesc(std::string_view word, const ex_desc_list &list)
{
  for (const extra_descr_data &i : list)
    if (i.keyword[0] == '.' ? isname(word, i.keyword.c_str() + 1) : isname(word, i.keyword.c_str()))
      return i.description.c_str();

  return (nullptr);
}

namespace {
  constexpr act_msgs LOOKS_AT = {nullptr, "$n looks at you.", "$n looks at $N.", true};

  /* What's told of a thing carried or worn, looked at by name: its sort, condition and make */
  void tell_of_item(GameWorld *g, struct char_data *ch, struct obj_data *obj, bool with_weight)
  {
    if (obj_get_type_flag(obj) == ITEM_WEAPON)
      send_to_char(g, ch, "The weapon type of {} is a {}.\r\n", obj_get_short_description(obj), weapon_type[obj_get_value(obj, VAL_WEAPON_SKILL)]);
    if (obj_get_type_flag(obj) == ITEM_SPELLBOOK)
      display_spells(g, ch, obj);
    if (obj_get_type_flag(obj) == ITEM_SCROLL)
      display_scroll(g, ch, obj);
    diag_obj_to_char(g, obj, ch);
    if (with_weight)
      send_to_char(g, ch, "It appears to be made of {}, and weights {}", material_names[obj_get_value(obj, 7)], add_commas(obj_get_weight(obj)));
    else /* quirk: worn things don't say what they weigh */
      send_to_char(g, ch, "It appears to be made of {}", material_names[obj_get_value(obj, 7)]);
  }

  void hatch_syntax(GameWorld *g, struct char_data *ch, const char *last)
  {
    send_to_char(g, ch, "@YSyntax@D: @CUnlock hatch\r\n");
    send_to_char(g, ch, "@YSyntax@D: @COpen hatch\r\n");
    send_to_char(g, ch, "@YSyntax@D: @CClose hatch\r\n");
    send_to_char(g, ch, "@YSyntax@D: @CUnlock hatch\r\n");
    send_to_char(g, ch, last);
  }

  /* What's told of a thing in the room, looked at by name */
  void tell_of_thing(GameWorld *g, struct char_data *ch, struct obj_data *thing)
  {
    if (obj_get_type_flag(thing) == ITEM_VEHICLE)
      hatch_syntax(g, ch, "@YSyntax@D: @CEnter hatch\r\n");
    else if (obj_get_type_flag(thing) == ITEM_HATCH)
      hatch_syntax(g, ch, "@YSyntax@D: @CLeave@n\r\n");
    else if (obj_get_type_flag(thing) == ITEM_WINDOW)
      look_out_window(g, ch, writable_copy(obj_get_name(thing)));

    if (obj_get_type_flag(thing) == ITEM_CONTROL) {
      int fuel = obj_get_value(thing, 2);
      send_to_char(g, ch, "@RFUEL@D: {}{}@n\r\n", fuel >= 200 ? "@G" : fuel >= 100 ? "@Y" : "@r", add_commas(fuel));
    }
    if (obj_get_type_flag(thing) == ITEM_WEAPON)
      send_to_char(g, ch, "The weapon type of {} is a {}.\r\n", obj_get_short_description(thing), weapon_type[obj_get_value(thing, VAL_WEAPON_SKILL)]);
    diag_obj_to_char(g, thing, ch);
    send_to_char(g, ch, "It appears to be made of {}, and weights {}", material_names[obj_get_value(thing, 7)], add_commas(obj_get_weight(thing)));
  }

  /* Reading: the board ch carries or sees, its message number, or else what's named looked at */
  void read_board(GameWorld *g, struct char_data *ch, std::string &arg, void (*look_at)(GameWorld *, struct char_data *, std::string &, int))
  {
    auto is_board        = [&](obj_data *o) { return obj_get_type_flag(o) == ITEM_BOARD; };
    struct obj_data *obj = find_first(char_inventory(g, ch), is_board);
    if (!obj)
      obj = find_first(room_contents(g, char_get_in_room(ch)), is_board);
    if (!obj)
      return;

    std::string_view rest = arg;
    std::string number    = take_arg(rest);
    if (number.empty()) {
      send_to_char(g, ch, "Read what?\r\n");
      return;
    }
    /* "read 5" or "read 4.mail": a period means a name */
    int msg = 1;
    if (isname(number, obj_get_name(obj))) {
      show_board(g, obj_get_vnum(g, obj), ch);
    } else if ((!isdigit(number[0]) || (!(msg = parse_int(number)))) || number.find('.') != std::string::npos) {
      std::string target = number + " " + std::string(rest);
      look_at(g, ch, target, 0);
    } else {
      board_display_msg(g, obj_get_vnum(g, obj), ch, msg);
    }
  }
} // namespace

/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */
static void look_at_target(GameWorld *g, struct char_data *ch, std::string &arg, int cmread)
{
  if (!char_get_desc(ch))
    return;
  if (arg.empty()) {
    send_to_char(g, ch, "Look at what?\r\n");
    return;
  }
  if (cmread) {
    read_board(g, ch, arg, look_at_target);
    return;
  }

  struct char_data *found_char = nullptr;
  struct obj_data *found_obj   = nullptr;
  int bits                     = generic_find(g, arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != nullptr) {
    look_at_char(g, found_char, ch);
    if (ch != found_char && !char_aff_flagged(ch, AFF_HIDE))
      act_triple(g, LOOKS_AT, ch, nullptr, found_char);
    return;
  }

  /* Strip off "number." from 2.foo and friends. */
  int fnum = get_number(arg);
  if (!fnum) {
    send_to_char(g, ch, "Look at what?\r\n");
    return;
  }

  /* The fnum-th extra description by that name: the room's, then what ch wears, carries and sees around */
  int i            = 0;
  const char *desc = nullptr;
  if ((desc = find_exdesc(arg, room_get_ex_description(g, char_get_in_room(ch)))) != nullptr && ++i == fnum) {
    send_to_char(g, ch, "{}", desc);
    return;
  }

  bool found = false;
  for (int j = 0; j < NUM_WEARS && !found; j++) {
    struct obj_data *eq = char_get_equipment(ch, j);
    if (!eq || !char_can_see_obj(g, ch, eq) || (desc = find_exdesc(arg, obj_get_ex_description(eq))) == nullptr || ++i != fnum)
      continue;
    send_to_char(g, ch, "{}", desc);
    if (isname(arg, obj_get_name(eq)))
      tell_of_item(g, ch, eq, false);
    found = true;
  }
  for (obj_data *item : char_inventory(g, ch)) {
    if (found)
      break;
    if (!char_can_see_obj(g, ch, item) || (desc = find_exdesc(arg, obj_get_ex_description(item))) == nullptr || ++i != fnum)
      continue;
    if (obj_get_type_flag(item) == ITEM_BOARD) {
      show_board(g, obj_get_vnum(g, item), ch);
    } else {
      send_to_char(g, ch, "{}", desc);
      if (isname(arg, obj_get_name(item)))
        tell_of_item(g, ch, item, true);
    }
    found = true;
  }
  for (obj_data *thing : room_contents(g, char_get_in_room(ch))) {
    if (found)
      break;
    if (!char_can_see_obj(g, ch, thing) || (desc = find_exdesc(arg, obj_get_ex_description(thing))) == nullptr || ++i != fnum)
      continue;
    if (obj_get_type_flag(thing) == ITEM_BOARD) {
      show_board(g, obj_get_vnum(g, thing), ch);
    } else {
      send_to_char(g, ch, "{}", desc);
      tell_of_thing(g, ch, thing);
    }
    found = true;
  }

  /* If an object was found back in generic_find */
  if (bits) {
    if (!found)
      show_obj_to_char(g, found_obj, ch, SHOW_OBJ_ACTION);
    else if (show_obj_modifiers(g, found_obj, ch))
      send_to_char(g, ch, "\r\n");
  } else if (!found)
    send_to_char(g, ch, "You do not see that here.\r\n");
}

void look_out_window(GameWorld *g, struct char_data *ch, std::string_view arg)
{
  struct obj_data *viewport = nullptr;

  /* First, lets find something to look out of or through. */
  if (!arg.empty()) {
    struct char_data *dummy = nullptr;
    if (!generic_find(g, arg, FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &dummy, &viewport)) {
      send_to_char(g, ch, "You don't see that here.\r\n");
      return;
    }
    if (obj_get_type_flag(viewport) != ITEM_WINDOW) {
      send_to_char(g, ch, "You can't look out that!\r\n");
      return;
    }
  } else if (char_is_outside(g, ch)) {
    send_to_char(g, ch, "But you are already outside.\r\n");
    return;
  } else {
    /* the last window in the room */
    for (obj_data *i : room_contents(g, char_get_in_room(ch)))
      if (obj_get_type_flag(i) == ITEM_WINDOW && isname("window", obj_get_name(i)))
        viewport = i;
  }
  if (!viewport) {
    send_to_char(g, ch, "You don't seem to be able to see outside.\r\n");
    return;
  }
  if (obj_val_flagged(viewport, CONT_CLOSEABLE) && obj_val_flagged(viewport, CONT_CLOSED)) {
    send_to_char(g, ch, "It is closed.\r\n");
    return;
  }

  room_vnum target_room = NOWHERE;
  if (obj_get_value(viewport, VAL_WINDOW_UNUSED1) >= 0) {
    /* We are looking out of a vehicle */
    if (struct obj_data *vehicle = find_vehicle_by_vnum(g, obj_get_value(viewport, VAL_WINDOW_UNUSED1)))
      target_room = obj_get_in_room(vehicle);
  } else if (obj_get_value(viewport, VAL_WINDOW_UNUSED4) >= 0) {
    target_room = obj_get_value(viewport, VAL_WINDOW_UNUSED4);
  } else {
    /* the default "outside": the last way out to a room not indoors */
    for (int door = 0; door < NUM_OF_DIRS; door++) {
      struct room_direction_data *exit = char_get_exit(g, ch, door);
      if (exit && exit->to_room != NOWHERE && !room_flagged(g, exit->to_room, ROOM_INDOORS))
        target_room = exit->to_room;
    }
  }
  if (!room_of(g, target_room)) {
    send_to_char(g, ch, "You don't seem to be able to see outside.\r\n");
    return;
  }
  if (obj_get_action_description(viewport))
    act(g, obj_get_action_description(viewport), true, ch, viewport, nullptr, TO_CHAR);
  else
    act(g, "$n looks out the window.", true, ch, nullptr, nullptr, TO_ROOM);
  send_to_char(g, ch, "You look outside and see:\r\n");
  look_at_room(g, target_room, ch, 0);
}

namespace {
  /* A direction named, or -1 */
  int direction_named(const std::string &word)
  {
    int dir = search_block(word, dirs, false);
    return dir >= 0 ? dir : search_block(word, abbr_dirs, false);
  }
} // namespace

ACMD(do_look)
{
  if (!char_get_desc(ch))
    return;
  if (char_get_position(ch) < POS_SLEEPING) {
    send_to_char(g, ch, "You can't see anything but stars!\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_BLIND)) {
    send_to_char(g, ch, "You can't see a damned thing, you're blind!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_EYEC)) {
    send_to_char(g, ch, "You can't see a damned thing, your eyes are closed!\r\n");
    return;
  }
  if (room_is_dark(g, char_get_in_room(ch)) && !char_can_see_in_dark(g, ch) && !char_plr_flagged(ch, PLR_AURALIGHT)) {
    send_to_char(g, ch, "It is pitch black...\r\n");
    list_char_to_char(g, room_get_people(g, char_get_in_room(ch)), ch); /* glowing red eyes */
    return;
  }

  std::string_view rest = argument;
  if (subcmd == SCMD_READ) {
    std::string arg = take_arg(rest);
    if (arg.empty())
      send_to_char(g, ch, "Read what?\r\n");
    else
      look_at_target(g, ch, arg, 1);
    return;
  }
  std::string arg  = take_word(rest);
  std::string arg2 = take_arg(rest);
  bool search      = subcmd == SCMD_SEARCH;
  auto toward      = [&](int dir) {
    if (search)
      search_in_direction(g, ch, dir);
    else
      look_in_direction(g, ch, dir);
  };

  int dir;
  if (arg.empty()) {
    if (search)
      search_room(g, ch);
    else
      look_at_room(g, char_get_in_room(ch), ch, 1);
  } else if (is_abbrev(arg, "inside") && arg2.empty() && (char_get_exit(g, ch, INDIR) || search)) {
    toward(INDIR);
  } else if (is_abbrev(arg, "inside") || is_abbrev(arg, "into") || is_abbrev(arg, "onto")) {
    look_in_obj(g, ch, arg2);
  } else if ((is_abbrev(arg, "outside") || is_abbrev(arg, "through") || is_abbrev(arg, "thru")) && subcmd == SCMD_LOOK && !arg2.empty()) {
    look_out_window(g, ch, arg2);
  } else if (is_abbrev(arg, "outside") && subcmd == SCMD_LOOK && !char_get_exit(g, ch, OUTDIR)) {
    look_out_window(g, ch, arg2);
  } else if ((dir = direction_named(arg)) >= 0) {
    toward(dir);
  } else if (is_abbrev(arg, "towards") && (dir = direction_named(arg2)) >= 0) {
    toward(dir);
  } else if (is_abbrev(arg, "at")) {
    if (search)
      send_to_char(g, ch, "That is not a direction!\r\n");
    else
      look_at_target(g, ch, arg2, 0);
  } else if (is_abbrev(arg, "around")) {
    bool found = false;
    for (const extra_descr_data &i : room_get_ex_description(g, char_get_in_room(ch))) {
      if (i.keyword[0] == '.')
        continue;
      send_to_char(g, ch, "{}{}:\r\n{}", (found ? "\r\n" : ""), i.keyword, i.description);
      found = true;
    }
    if (!found)
      send_to_char(g, ch, "You couldn't find anything noticeable.\r\n");
  } else if (find_exdesc(arg, room_get_ex_description(g, char_get_in_room(ch))) != nullptr || !search) {
    look_at_target(g, ch, arg, 0);
  } else {
    send_to_char(g, ch, "That is not a direction!\r\n");
  }
}

ACMD(do_examine)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "Examine what?\r\n");
    return;
  }

  /* look_at_target() eats the number. */
  std::string tempsave = arg;
  look_at_target(g, ch, tempsave, 0);

  struct char_data *tmp_char;
  struct obj_data *tmp_object;
  generic_find(g, arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);
  if (tmp_object && (obj_get_type_flag(tmp_object) == ITEM_DRINKCON || obj_get_type_flag(tmp_object) == ITEM_FOUNTAIN || obj_get_type_flag(tmp_object) == ITEM_CONTAINER)) {
    send_to_char(g, ch, "When you look inside, you see:\r\n");
    look_in_obj(g, ch, arg);
  }
}

const char *const default_color_choices[NUM_COLOR + 1] = {
    /* COLOR_NORMAL */ AA_NORMAL,
    /* COLOR_ROOMNAME */ AA_NORMAL ANSISEPSTR AF_CYAN,
    /* COLOR_ROOMOBJS */ AA_NORMAL ANSISEPSTR AF_GREEN,
    /* COLOR_ROOMPEOPLE */ AA_NORMAL ANSISEPSTR AF_YELLOW,
    /* COLOR_HITYOU */ AA_NORMAL ANSISEPSTR AF_RED,
    /* COLOR_YOUHIT */ AA_NORMAL ANSISEPSTR AF_GREEN,
    /* COLOR_OTHERHIT */ AA_NORMAL ANSISEPSTR AF_YELLOW,
    /* COLOR_CRITICAL */ AA_BOLD ANSISEPSTR AF_YELLOW,
    /* COLOR_HOLLER */ AA_BOLD ANSISEPSTR AF_YELLOW,
    /* COLOR_SHOUT */ AA_BOLD ANSISEPSTR AF_YELLOW,
    /* COLOR_GOSSIP */ AA_NORMAL ANSISEPSTR AF_YELLOW,
    /* COLOR_AUCTION */ AA_NORMAL ANSISEPSTR AF_CYAN,
    /* COLOR_CONGRAT */ AA_NORMAL ANSISEPSTR AF_GREEN,
    /* COLOR_TELL */ AA_NORMAL ANSISEPSTR AF_RED,
    /* COLOR_YOUSAY */ AA_NORMAL ANSISEPSTR AF_CYAN,
    /* COLOR_ROOMSAY */ AA_NORMAL ANSISEPSTR AF_WHITE,
    nullptr};
