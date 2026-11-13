/* map.c: maps, scanning and searching
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>

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

static double terrain_bonus(GameWorld *g, struct char_data *ch);
static void map_draw_room(GameWorld *g, char map[9][10], int x, int y, room_vnum rnum,
                          struct char_data *ch);

/* This is used to determine the terrain bonus for search_room - Iovan 12/16/2012*/
static double terrain_bonus(GameWorld *g, struct char_data *ch)
{

  double bonus = 0.0;

  switch (room_sect(g, char_get_in_room(ch))) {
    case SECT_FOREST:
      bonus += 0.5;
      break;
    case SECT_SPACE:
      bonus += -0.5;
      break;
    case SECT_WATER_NOSWIM:
      bonus += 0.25;
      break;
    case SECT_MOUNTAIN:
      bonus += 0.1;
      break;
    default:
      bonus += 0.0;
      break;
  }

  if (room_flagged(g, char_get_in_room(ch), ROOM_SPACE)) {
    bonus += -0.5;
  }

  return (bonus);
}

constexpr act_msgs FOUND_HIDING = {"@YYou find @y$N@Y hiding nearby!@n", "@y$n@Y has found your hiding spot!@n", "@y$n@Y has found @y$N's@Y hiding spot!@n", true};

/* This is used to find hidden people in a room with search - Iovan 12/16/2012 */
void search_room(GameWorld *g, struct char_data *ch)
{

  int perc = (char_get_aff_abils_intel(ch) * 0.6) + char_get_skill(ch, SKILL_SPOT) + char_get_skill(ch, SKILL_SEARCH) + char_get_skill(ch, SKILL_LISTEN);
  int prob = 0, found = 0;
  double bonus = 1.0, terrain = 1.0;

  if ((getCurST(g, ch)) < getMaxST(g, ch) * 0.001) {
    send_to_char(g, ch, "You do not have enough stamina.\r\n");
    return;
  }

  if (char_get_skill(ch, SKILL_SENSE)) {
    bonus += (char_get_skill(ch, SKILL_SENSE) * 0.01);
  }

  reveal_hiding(g, ch, 0);
  act(g, "@y$n@Y begins searching the room carefully.@n", true, ch, nullptr, nullptr, TO_ROOM);
  char_set_wait(ch, PULSE_1SEC);

  for (char_data *vict : room_people(g, char_get_in_room(ch))) {
    if (char_aff_flagged(vict, AFF_HIDE) && vict != ch) {
      if (char_get_suppression(vict) >= 1) {
        perc *= (char_get_suppression(vict) * 0.01);
      }
      prob = char_get_aff_abils_dex(vict) + (char_get_aff_abils_intel(vict) * 0.6) + char_get_skill(vict, SKILL_HIDE) + char_get_skill(vict, SKILL_MOVE_SILENTLY);

      if (char_aff_flagged(vict, AFF_LIQUEFIED)) {
        prob *= 1.5;
      }
      if (char_is_mutant(ch) && (char_get_genome(ch, 0) == 4 || char_get_genome(ch, 1) == 4)) {
        perc += 5;
      }
      if (char_is_mutant(vict) && (char_get_genome(vict, 0) == 5 || char_get_genome(vict, 1) == 5)) {
        prob += 10;
      }
      terrain += terrain_bonus(g, vict);
      if (perc * bonus >= prob * terrain) { /* Found them. */
        act_triple(g, FOUND_HIDING, ch, nullptr, vict);
        reveal_hiding(g, vict, 4);
        found++;
      }
    }
  }

  for (obj_data *obj : room_contents(g, char_get_in_room(ch))) {
    if (obj_extra_flagged(obj, ITEM_BURIED) && perc * bonus > rand_number(g, 50, 200)) {
      act(g, "@YYou uncover @y$p@Y, which had been burried here.@n", true, ch, obj, nullptr, TO_CHAR);
      act(g, "@y$n@Y uncovers @y$p@Y, which had burried here.@n", true, ch, obj, nullptr, TO_ROOM);
      obj_extra_set(obj, ITEM_BURIED, false);
      found++;
    }
  }
  decCurSTPercent(g, ch, .001, 0);

  if (found == 0)
    send_to_char(g, ch, "You find nothing hidden.\r\n");
}

namespace {
  /* Where each of the eight flat directions is on the map, from the room drawn; up, down, in and out aren't drawn. */
  struct map_step {
    int dx, dy;
  };

  constexpr std::optional<map_step> MAP_STEPS[NUM_OF_DIRS] = {
      map_step{0, -1}, map_step{1, 0}, map_step{0, 1}, map_step{-1, 0}, std::nullopt, std::nullopt,
      map_step{-1, -1}, map_step{1, -1}, map_step{1, 1}, map_step{-1, 1}, std::nullopt, std::nullopt,
  };

  struct sector_glyph {
    int sect;
    char glyph;
    char with_lava; /* or the glyph again, for a sector lava doesn't change */
  };

  /* A room's glyph by its sector; lava (any ground effect) marks some as their lava glyph. Failing all, '-'. */
  constexpr sector_glyph SECTOR_GLYPHS[] = {
      {SECT_INSIDE,       'i', '2'}, /* quirk: lava inside is drawn as lava on a plain */
      {SECT_FIELD,        'p', '2'},
      {SECT_DESERT,       '!', '7'},
      {SECT_CITY,         '(', '1'},
      {SECT_FOREST,       'f', '6'},
      {SECT_MOUNTAIN,     '^', '5'},
      {SECT_HILLS,        'h', '3'},
      {SECT_FLYING,       's', 's'},
      {SECT_WATER_NOSWIM, '`', '`'},
      {SECT_WATER_SWIM,   '+', '+'},
      {SECT_SHOP,         '&', '&'},
      {SECT_IMPORTANT,    '*', '*'},
  };

  char room_glyph(GameWorld *g, room_vnum room)
  {
    if (room_sunken(g, room))
      return '=';
    auto it = std::ranges::find(SECTOR_GLYPHS, room_sect(g, room), &sector_glyph::sect);
    if (it == std::end(SECTOR_GLYPHS))
      return '-';
    return room_get_geffect(g, room) >= 1 ? it->with_lava : it->glyph;
  }
} // namespace

/* Draws the rooms around rnum, which is at x, y: a closed door ('8'), or the room it opens on. */
static void map_draw_room(GameWorld *g, char map[9][10], int x, int y, room_vnum rnum, struct char_data *)
{
  for (int door = 0; door < NUM_OF_DIRS; door++) {
    struct room_direction_data *exit = room_get_dir_option(g, rnum, door);
    if (!exit || exit->to_room == NOWHERE || !MAP_STEPS[door])
      continue;
    char *cell = &map[y + MAP_STEPS[door]->dy][x + MAP_STEPS[door]->dx];
    if (room_exit_flagged(exit, EX_CLOSED)) {
      if (!room_exit_flagged(exit, EX_SECRET))
        *cell = '8';
      continue;
    }
    char glyph = room_glyph(g, exit->to_room);
    if (door == WEST && glyph == '2' && room_sect(g, exit->to_room) == SECT_INSIDE)
      cell = &map[y + 1][x]; /* quirk: lava inside, to the west, is drawn to the south */
    *cell = glyph;
  }
}

ACMD(do_map)
{
  gen_map(g, ch, 1);
}

namespace {
  /* The map's glyphs as they are shown, in the order they are replaced. */
  constexpr std::pair<const char *, const char *> GLYPH_COLOURS[] = {
      {"x", "@RX"}, {"&", "@m$"}, {"*", "@m#"}, {"+", "@c~"}, {"s", "@CS"}, {"i", "@wI"}, {"(", "@WC"}, {"^", "@DM"},
      {"h", "@yH"}, {"`", "@BW"}, {"=", "@bU"}, {"p", "@GP"}, {"f", "@gF"}, {"!", "@YD"}, {"-", "@w:"},
      /* lava */
      {"1", "@4@YC@n"}, {"2", "@4@YP@n"}, {"3", "@4@YH@n"}, {"7", "@4@YD@n"}, {"5", "@4@YM@n"}, {"6", "@4@YF@n"},
      /* closed doors */
      {"8", "@1 @n"},
  };

  /* The key beside each line of the small map */
  constexpr const char *MAP_KEY[] = {
      "    @WC: City, @wI@W: Inside, @GP@W: Plain@n\r\n",
      "    @gF@W: Forest, @DM@W: Mountain, @yH@W: Hills@n\r\n",
      "    @CS@W: Sky, @BW@W: Water, @bU@W: Underwater@n\r\n",
      "    @m$@W: Shop, @m#@W: Important, @YD@W: Desert@n\r\n",
      "    @c~@W: Shallow Water, @4 @n@W: Lava, @RX@W: You@n\r\n",
  };

  bool exit_visible(GameWorld *g, struct char_data *ch, int dir)
  {
    struct room_direction_data *exit = char_get_exit(g, ch, dir);
    return exit && !room_exit_flagged(exit, EX_SECRET);
  }

  /* An exit's mark beside the small map: shut, open, or blank where there's none to see. */
  const char *exit_mark(GameWorld *g, struct char_data *ch, int dir, const char *shut, const char *open, int shut_flag = EX_CLOSED)
  {
    if (!exit_visible(g, ch, dir))
      return "   ";
    return room_exit_flagged(char_get_exit(g, ch, dir), shut_flag) ? shut : open;
  }

  /* The exits beside line i of the small map */
  std::string exits_beside(GameWorld *g, struct char_data *ch, int i, const char *line)
  {
    switch (i) {
      case 2:
        return str_format("@w       @w|{}@w|           {}", exit_mark(g, ch, NORTH, " @rN ", " @CN "), line);
      case 3: /* quirk: northeast is never shown shut */
        return str_format("@w @w|{}@w| |{}@w| |{}@w|     {}", exit_mark(g, ch, NORTHWEST, " @rNW", " @CNW"), exit_mark(g, ch, UP, " @yU ", " @YU "),
                          exit_mark(g, ch, NORTHEAST, "@rNE ", "@CNE ", EX_SECRET), line);
      case 4: {
        const char *inside = exit_visible(g, ch, INDIR)    ? exit_mark(g, ch, INDIR, " @rI ", " @mI ")
                             : exit_visible(g, ch, OUTDIR) ? exit_mark(g, ch, OUTDIR, "@rOUT", "@mOUT")
                                                           : "@r{ }";
        return str_format("@w @w|{}@w| |{}@w| |{}@w|     {}", exit_mark(g, ch, WEST, "  @rW", "  @CW"), inside, exit_mark(g, ch, EAST, "@rE  ", "@CE  "), line);
      }
      case 5: /* quirk: southeast is never shown shut */
        return str_format("@w @w|{}@w| |{}@w| |{}@w|     {}", exit_mark(g, ch, SOUTHWEST, " @rSW", " @CSW"), exit_mark(g, ch, DOWN, " @yD ", " @YD "),
                          exit_mark(g, ch, SOUTHEAST, "@rSE ", "@CSE ", EX_SECRET), line);
      default:
        return str_format("@w       @w|{}@w|           {}", exit_mark(g, ch, SOUTH, " @rS ", " @CS "), line);
    }
  }
} // namespace

void gen_map(GameWorld *g, struct char_data *ch, int num)
{
  char map[9][10] = {{'-'}, {'-'}};

  if (num == 1) {
    /* Map Key */
    send_to_char(g, ch, "@W               @D-[@CArea Map@D]-\r\n");
    send_to_char(g, ch, "@D-------------------------------------------@w\r\n");
    send_to_char(g, ch, "@WC = City, @wI@W = Inside, @GP@W = Plain, @gF@W = Forest\r\n");
    send_to_char(g, ch, "@DM@W = Mountain, @yH@W = Hills, @CS@W = Sky, @BW@W = Water\r\n");
    send_to_char(g, ch, "@bU@W = Underwater, @m$@W = Shop, @m#@W = Important,\r\n");
    send_to_char(g, ch, "@YD@W = Desert, @c~@W = Shallow Water, @4 @n@W = Lava,\r\n");
    send_to_char(g, ch, "@WLastly @RX@W = You.\r\n");
    send_to_char(g, ch, "@D-------------------------------------------\r\n");
    send_to_char(g, ch, "@D                  @CNorth@w\r\n");
    send_to_char(g, ch, "@D                    @c^@w\r\n");
    send_to_char(g, ch, "@D             @CWest @c< O > @CEast@w\r\n");
    send_to_char(g, ch, "@D                    @cv@w\r\n");
    send_to_char(g, ch, "@D                  @CSouth@w\r\n");
    send_to_char(g, ch, "@D                ---------@w\r\n");
  }

  /* blank the map */
  for (auto &row : map)
    strcpy(row, "         ");

  /* the room ch is in, and the rooms its open exits lead to */
  map_draw_room(g, map, 4, 4, char_get_in_room(ch), ch);
  for (int door = 0; door < NUM_OF_DIRS; door++) {
    struct room_direction_data *exit = char_get_exit(g, ch, door);
    if (exit && exit->to_room != NOWHERE && !room_exit_flagged(exit, EX_CLOSED) && MAP_STEPS[door])
      map_draw_room(g, map, 4 + MAP_STEPS[door]->dx, 4 + MAP_STEPS[door]->dy, exit->to_room, ch);
  }

  /* make it obvious what room they are in */
  map[4][4] = 'x';

  /* print out the map: the middle five lines */
  for (int i = 2; i <= 6; i++) {
    std::string line = num == 1 ? str_format("@w                {}\r\n", map[i]) : exits_beside(g, ch, i, map[i]);
    for (auto [glyph, shown] : GLYPH_COLOURS)
      search_replace(line, glyph, shown);
    if (num == 1)
      send_to_char(g, ch, line);
    else
      send_to_char(g, ch, "{}{}", line, MAP_KEY[i - 2]);
  }
  if (num == 1)
    send_to_char(g, ch, "@D                ---------@w\r\n");
}

namespace {
  /* What ch sees of a room scanned: its name, what's in it, and its lava. */
  void scan_room(GameWorld *g, struct char_data *ch, room_vnum room, const char *far, const char *direction)
  {
    send_to_char(g, ch, "@w-----------------------------------------@n\r\n");
    send_to_char(g, ch, "          {}{}{}: {} {}\n\r", CCCYN(g, ch, C_NRM), far, direction,
                 room_get_name(g, room) ? room_get_name(g, room) : "You don't think you saw what you just saw.", CCNRM(g, ch, C_NRM));
    send_to_char(g, ch, "@W          -----------------          @n\r\n");
    list_obj_to_char(g, room_get_contents(g, room), ch, SHOW_OBJ_LONG, false);
    list_char_to_char(g, room_get_people(g, room), ch);
    if (room_get_geffect(g, room) >= 1 && room_get_geffect(g, room) <= 5)
      send_to_char(g, ch, "@rLava@w is pooling in someplaces here...@n\r\n");
    if (room_get_geffect(g, room) >= 6)
      send_to_char(g, ch, "@RLava@r covers pretty much the entire area!@n\r\n");
  }
} // namespace

ACMD(do_scan)
{
  static constexpr const char *dirnames[] = {"North", "East", "South", "West", "Up", "Down", "Northwest", "Northeast", "Southeast", "Southwest", "Inside", "Outside"};

  if (char_get_position(ch) < POS_SLEEPING) {
    send_to_char(g, ch, "You can't see anything but stars!\n\r");
    return;
  }
  if (!char_is_awake(ch)) {
    send_to_char(g, ch, "You must be dreaming.\n\r");
    return;
  }
  if (char_aff_flagged(ch, AFF_BLIND)) {
    send_to_char(g, ch, "You can't see a damn thing, you're blind!\n\r");
    return;
  }
  if (char_plr_flagged(ch, PLR_EYEC)) {
    send_to_char(g, ch, "You can't see a damned thing, your eyes are closed!\r\n");
    return;
  }

  for (int i = 0; i < 10; i++) {
    if (!char_get_exit(g, ch, i))
      continue;
    if (room_is_dark(g, char_get_in_room(ch)) && char_get_admlevel(ch) < ADMLVL_IMMORT && !char_aff_flagged(ch, AFF_INFRAVISION)) {
      send_to_char(g, ch, "{}: DARK\n\r", dirnames[i]);
      continue;
    }
    if (!char_can_go(g, ch, i))
      continue;
    scan_room(g, ch, room_get_dir_option(g, char_get_in_room(ch), i)->to_room, "", dirnames[i]);

    /* and the room past it */
    struct room_direction_data *far = char_get_exit2(g, ch, i);
    if (!far || !far->to_room || far->to_room == NOWHERE || IS_SET(far->exit_info, EX_CLOSED))
      continue;
    if (room_is_dark(g, far->to_room))
      send_to_char(g, ch, "{}<-> {}Far {}: Too dark to tell! {}<->{}\r\n", QMAG, QCYN, dirnames[i], QMAG, QNRM);
    else
      scan_room(g, ch, far->to_room, "Far ", dirnames[i]);
  }
  send_to_char(g, ch, "@w-----------------------------------------@n\r\n");
}

void search_in_direction(GameWorld *g, struct char_data *ch, int dir)
{
  send_to_char(g, ch, "You search for secret doors.\r\n");
  act(g, "$n searches the area intently.", true, ch, nullptr, nullptr, TO_ROOM);

  /* SEARCHING is allowed untrained */
  int skill_lvl = char_get_skill(ch, SKILL_SEARCH);
  if (char_is_truffle(ch) || char_is_human(ch))
    skill_lvl = skill_lvl + 2;
  if (char_is_halfbreed(ch))
    skill_lvl = skill_lvl + 1;

  struct room_direction_data *exit = char_get_exit(g, ch, dir);
  bool check                       = skill_lvl > (exit ? exit->dchide : 20);

  if (!exit)
    send_to_char(g, ch, "There is no exit there.\r\n");
  else if (exit_description(exit) && !room_exit_flagged(exit, EX_SECRET))
    send_to_char(g, ch, "{}", exit_description(exit));
  else if (!room_exit_flagged(exit, EX_SECRET))
    send_to_char(g, ch, "There is a normal exit there.\r\n");
  else if (room_exit_flagged(exit, EX_ISDOOR) && exit_keyword(exit) && check)
    send_to_char(g, ch, "There is a hidden door keyword: '{}' {}there.\r\n", fname(exit_keyword(exit)), room_exit_flagged(exit, EX_CLOSED) ? "" : "open ");
  else
    send_to_char(g, ch, "There is no exit there.\r\n");
}
