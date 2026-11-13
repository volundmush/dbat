/* who.c: who, users, where, finger, whois, toplist
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iterator>
#include <ranges>

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

static void print_object_location(GameWorld *g, int num, struct obj_data *obj, struct char_data *ch, int recur);
static void perform_mortal_where(GameWorld *g, struct char_data *ch, const std::string &arg);
static void perform_immort_where(GameWorld *g, struct char_data *ch, const std::string &arg);

ACMD(do_finger)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "What user are you wanting to look at?\r\n");
    return;
  }
  if (!readUserIndex(arg)) {
    send_to_char(g, ch, "That user does not exist\r\n");
    return;
  }
  fingerUser(g, ch, arg);
}

namespace {
  struct who_rank {
    const char *heading;
    int min_level, max_level; /* admin levels */
  };

  constexpr who_rank WHO_RANKS[] = {
      {"\r\n               @c------------  @D[    @gI@Gm@Wm@Do@Gr@Dt@Wa@Gl@gs   @D]  @c------------@n\r\n", ADMLVL_IMMORT, ADMLVL_IMPL      },
      {"\r\n               @c------------  @D[     @DM@ro@Rr@wt@Ra@rl@Ds    ]  @c------------@n\r\n",       0,             ADMLVL_IMMORT - 1},
  };

  bool in_rank(struct char_data *tch, const who_rank &rank) { return char_get_admlevel(tch) >= rank.min_level && char_get_admlevel(tch) <= rank.max_level; }

  /* The one playing through d, as who and users list them: the body left behind by one who has switched. */
  struct char_data *player_of(descriptor_data *d)
  {
    if (!desc_is_playing(d))
      return nullptr;
    return d->original ? d->original : d->character;
  }

  struct edit_tag {
    int connected;
    const char *tag;
  };

  constexpr edit_tag EDIT_TAGS[] = {
      {CON_OEDIT,    " (O Edit)"},
      {CON_MEDIT,    " (M Edit)"},
      {CON_ZEDIT,    " (Z Edit)"},
      {CON_SEDIT,    " (S Edit)"},
      {CON_REDIT,    " (R Edit)"},
      {CON_TEDIT,    " (T Edit)"},
      {CON_TRIGEDIT, " (T Edit)"},
      {CON_AEDIT,    " (S Edit)"}, /* quirk: as shop editing */
      {CON_CEDIT,    " (C Edit)"},
      {CON_HEDIT,    " (H Edit)"},
  };

  struct who_flag {
    bool prf; /* a preference, or a player flag */
    int flag;
    const char *tag;
    bool immortals_see_only = false;
  };

  constexpr who_flag WHO_FLAGS[] = {
      {true,  PRF_DEAF,      " (DEAF)"           },
      {true,  PRF_NOTELL,    " (NO TELL)"        },
      {true,  PRF_NOGOSS,    " (NO OOC)"         },
      {false, PLR_NOSHOUT,   " (MUTED)"          },
      {true,  PRF_HIDE,      " (WH)"             },
      {true,  PRF_BUILDWALK, " (Buildwalking)"   },
      {true,  PRF_AFK,       " (AFK)"            },
      {false, PLR_FISHING,   " (@BFISHING@n)", true},
      {true,  PRF_NOWIZ,     " (NO WIZ)"         },
  };

  void who_line(GameWorld *g, struct char_data *ch, descriptor_data *d, struct char_data *tch)
  {
    const char *line_color = "@w";
    std::string usr        = str_format("@W(@R{}@W){}", char_get_user(tch), char_plr_flagged(tch, PLR_BIOGR) ? "" : (char_is_spoiled(tch) ? " @R*@n" : ""));
    send_to_char(g, ch, "{}               @D<@C{:<12}@D> {}@w{}", line_color,
                 char_get_admlevel(ch) > 0 ? char_get_name(tch) : (char_get_admlevel(tch) > 0 ? char_get_name(tch) : (char_get_user(tch) ? char_get_user(tch) : "NULL")),
                 char_get_admlevel(ch) > 0 ? usr.c_str() : "", line_color);

    if (char_get_admlevel(tch))
      send_to_char(g, ch, " ({})", admin_level_names[char_get_admlevel(tch)]);
    if (d->snooping && d->snooping->character != ch && char_get_admlevel(ch) >= 3)
      send_to_char(g, ch, " (Snoop: {})", char_get_name(d->snooping->character));
    if (char_get_invis_level(g, tch))
      send_to_char(g, ch, " (i{:d})", char_get_invis_level(g, tch));
    else if (char_aff_flagged(tch, AFF_INVISIBLE))
      send_to_char(g, ch, " (invis)");

    if (char_plr_flagged(tch, PLR_MAILING))
      send_to_char(g, ch, " (mailing)");
    else if (d->olc)
      send_to_char(g, ch, " (OLC)");
    else if (char_plr_flagged(tch, PLR_WRITING))
      send_to_char(g, ch, " (writing)");
    if (d->original)
      send_to_char(g, ch, " (out of body)");

    if (auto it = std::ranges::find(EDIT_TAGS, d->connected, &edit_tag::connected); it != std::end(EDIT_TAGS))
      send_to_char(g, ch, "{}", it->tag);
    for (const who_flag &f : WHO_FLAGS) {
      bool has = f.prf ? char_prf_flagged(g, tch, f.flag) : char_plr_flagged(tch, f.flag);
      if (has && (!f.immortals_see_only || char_get_admlevel(ch) >= ADMLVL_IMMORT))
        send_to_char(g, ch, "{}", f.tag);
    }
    send_to_char(g, ch, "@n\r\n");
  }
} // namespace

/* Written by Rhade */
ACMD(do_who)
{
  char *tmstr                  = asctime(localtime(&g->PCOUNTDATE));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  auto in_view = [&](struct char_data *tch) {
    return char_can_see(g, ch, tch) && char_get_level(tch) >= 0 && char_get_level(tch) <= CONFIG_LEVEL_CAP;
  };
  auto hidden = [&](struct char_data *tch) { return char_prf_flagged(g, tch, PRF_HIDE) && tch != ch && char_get_admlevel(ch) < ADMLVL_IMMORT; };

  send_to_char(g, ch, "\r\n      @r{@b===============  @D[  @DD@wr@ca@Cg@Y(@R*@Y)@Wn@cB@Da@cl@Cl @DA@wd@cv@Ce@Wnt @DT@wr@cu@Ct@Wh@n  @D]  @b===============@r}      @n\r\n");

  /* how many each rank has to show, and how many hide from mortals */
  int hide = 0;
  std::array<int, std::size(WHO_RANKS)> shown_in{};
  for (descriptor_data *d : each_desc(g)) {
    struct char_data *tch = player_of(d);
    if (!tch || !in_view(tch))
      continue;
    if (hidden(tch)) {
      hide += 1;
      continue;
    }
    for (std::size_t i = 0; i < std::size(WHO_RANKS); i++)
      if (in_rank(tch, WHO_RANKS[i]))
        shown_in[i]++;
  }

  int num_can_see = 0;
  for (std::size_t i = 0; i < std::size(WHO_RANKS); i++) {
    if (!shown_in[i])
      continue;
    send_to_char(g, ch, "{}", WHO_RANKS[i].heading);
    for (descriptor_data *d : each_desc(g)) {
      struct char_data *tch = player_of(d);
      if (!tch || !in_rank(tch, WHO_RANKS[i]) || !in_view(tch) || hidden(tch))
        continue;
      num_can_see++;
      who_line(g, ch, d, tch);
    }
    send_to_char(g, ch, "\r\n");
  }

  if (!num_can_see)
    send_to_char(g, ch, "                            Nobody at all!\r\n");
  else if (num_can_see == 1)
    send_to_char(g, ch, "                         One lonely character displayed.\r\n");
  else {
    send_to_char(g, ch, "                           @Y{:d}@w characters displayed.\r\n", num_can_see);
    if (hide > 0)
      send_to_char(g, ch, "                           and @Y{:d}@w character{} hidden.\r\n", hide, hide > 1 ? "s" : "");
  }
  if (g->circle_restrict > 0 && g->circle_restrict <= 100)
    send_to_char(g, ch, "                      @rThe mud has been wizlocked to lvl {:d}@n\r\n", g->circle_restrict);
  if (g->circle_restrict == 101)
    send_to_char(g, ch, "                      @rThe mud has been wizlocked to IMMs only.@n\r\n");
  send_to_char(g, ch, "      @r{@b=================================================================@r}@n\r\n");
  send_to_char(g, ch, "           @cHighest Logon Count Ever@D: @Y{:d}@w, on {}\r\n", g->HIGHPCOUNT, tmstr);
  send_to_char(g, ch, "                        @cHighest Logon Count Today@D: @Y{:d}@n\r\n", g->PCOUNT);
}

#define USERS_FORMAT \
  "format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-o] [-p]\r\n"

ACMD(do_users)
{
  std::string name_search, host_search;
  int low = 0, high = CONFIG_LEVEL_CAP;
  bool outlaws = false, playing = false, deadweight = false;

  std::string_view rest = argument;
  while (!rest.empty()) {
    std::string arg = take_word(rest);
    rest            = skip_spaces(rest);
    if (arg[0] != '-') {
      send_to_char(g, ch, "{}", USERS_FORMAT);
      return;
    }
    switch (arg[1]) { /* '\0' for a bare "-" */
      case 'o':
      case 'k':
        outlaws = true;
        playing = true;
        break;
      case 'p':
        playing = true;
        break;
      case 'd':
        deadweight = true;
        break;
      case 'l':
        playing = true;
        arg     = take_word(rest);
        rest    = skip_spaces(rest);
        sscanf(arg.c_str(), "%d-%d", &low, &high);
        break;
      case 'n':
        playing     = true;
        name_search = take_word(rest);
        rest        = skip_spaces(rest);
        break;
      case 'h':
        playing     = true;
        host_search = take_word(rest);
        rest        = skip_spaces(rest);
        break;
      default:
        send_to_char(g, ch, "{}", USERS_FORMAT);
        return;
    }
  }
  send_to_char(g, ch,
               "Num Name                 User-name            State          Idl Login    C\r\n--- -------------------- -------------------- -------------- --- -------- -\r\n");

  int num_can_see = 0;
  for (descriptor_data *d : each_desc(g)) {
    if (desc_get_connected(d) != CON_PLAYING && playing)
      continue;
    if (desc_get_connected(d) == CON_PLAYING && deadweight)
      continue;
    if (desc_is_playing(d)) {
      struct char_data *tch = player_of(d);
      if (!tch)
        continue;
      if (!host_search.empty() && !strstr(d->host, host_search.c_str()))
        continue;
      if (!name_search.empty() && !str_eq_nocase(char_get_name(tch), name_search))
        continue;
      if (!char_can_see(g, ch, tch) || char_get_level(tch) < low || char_get_level(tch) > high)
        continue;
      if (char_prf_flagged(g, tch, PRF_HIDE) && tch != ch && char_get_admlevel(ch) < ADMLVL_IMMORT)
        continue;
      if (outlaws && !char_plr_flagged(tch, PLR_KILLER) && !char_plr_flagged(tch, PLR_THIEF))
        continue;
      if (char_get_invis_level(g, tch) > char_get_admlevel(ch))
        continue;
    }

    char *timeptr = asctime(localtime(&d->login_time)) + 11;
    *(timeptr + 8) = '\0';

    std::string state = desc_get_connected(d) == CON_PLAYING && d->original ? "Switched" : connected_types[desc_get_connected(d)];
    std::string idletime;
    if (d->character && desc_get_connected(d) == CON_PLAYING && char_get_admlevel(d->character) <= char_get_admlevel(ch))
      idletime = str_format("{:>3d}", char_get_timer(d->character) * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);

    const char *alias = d->original && char_get_alias(d->original) ? char_get_alias(d->original)
                        : d->character && char_get_alias(d->character) ? char_get_alias(d->character)
                                                                        : "UNDEFINED";
    const char *user  = !d->user.empty() ? d->user.c_str() : "UNKNOWN";
    std::string line  = str_format("{:>3d} {:<20} {:<20} {:<14} {:<3} {:<8} {:>1} ", d->desc_num, alias, user, state, idletime, timeptr, "N");
    if (*d->host)
      line += str_format("\n{:>3d} [{} Site: {}]\r\n", d->desc_num, user, d->host);
    else
      line += str_format("\n{:>3d} [{} Site: Hostname unknown]\r\n", d->desc_num, user);

    if (desc_get_connected(d) != CON_PLAYING)
      line = str_format("@g{}@n", line);
    if (desc_get_connected(d) != CON_PLAYING || char_can_see(g, ch, d->character)) {
      send_to_char(g, ch, line);
      num_can_see++;
    }
  }

  send_to_char(g, ch, "\r\n{:d} visible sockets connected.\r\n", num_can_see);
}

static void perform_mortal_where(GameWorld *g, struct char_data *ch, const std::string &arg)
{
  if (arg.empty()) {
    send_to_char(g, ch, "Players in your Zone\r\n--------------------\r\n");
    for (descriptor_data *d : each_desc(g)) {
      if (desc_get_connected(d) != CON_PLAYING || d->character == ch)
        continue;
      struct char_data *i = d->original ? d->original : d->character;
      if (!i || char_get_in_room(i) == NOWHERE || !char_can_see(g, ch, i))
        continue;
      if (room_get_zone(g, char_get_in_room(ch)) != room_get_zone(g, char_get_in_room(i)))
        continue;
      send_to_char(g, ch, "{:<20} - {}\r\n", char_get_name(i), room_get_name(g, char_get_in_room(i)));
    }
    return;
  }

  /* only the first by that name */
  struct char_data *tch = find_first(each_char(g), [&](char_data *c) {
    return char_get_in_room(c) != NOWHERE && c != ch && char_can_see(g, ch, c) &&
           room_get_zone(g, char_get_in_room(c)) == room_get_zone(g, char_get_in_room(ch)) && isname(arg, char_get_alias(c));
  });
  if (!tch) {
    send_to_char(g, ch, "Nobody around by that name.\r\n");
    return;
  }
  send_to_char(g, ch, "{:<25} - {}\r\n", char_get_name(tch), room_get_name(g, char_get_in_room(tch)));
}

static void print_object_location(GameWorld *g, int num, struct obj_data *obj, struct char_data *ch, int recur)
{
  if (num > 0)
    send_to_char(g, ch, "O{:>3d}. {:<25} - ", num, obj_get_short_description(obj));
  else
    send_to_char(g, ch, "{:>33}", " - ");

  if (obj_get_script(obj))
    send_to_char(g, ch, "[T{:d}]", obj_get_proto_script(obj)->vnum);

  if (obj_get_in_room(obj) != NOWHERE)
    send_to_char(g, ch, "[{:>5d}] {}\r\n", obj_get_in_room(obj), room_get_name(g, obj_get_in_room(obj)));
  else if (obj_get_carried_by(obj))
    send_to_char(g, ch, "carried by {} in room [{:d}]\r\n", char_pers(g, obj_get_carried_by(obj), ch), char_get_in_room(obj_get_carried_by(obj)));
  else if (obj_get_worn_by(obj))
    send_to_char(g, ch, "worn by {} in room [{:d}]\r\n", char_pers(g, obj_get_worn_by(obj), ch), char_get_in_room(obj_get_worn_by(obj)));
  else if (obj_get_in_obj(obj)) {
    send_to_char(g, ch, "inside {}{}\r\n", obj_get_short_description(obj_get_in_obj(obj)), (recur ? ", which is" : " "));
    if (recur)
      print_object_location(g, 0, obj_get_in_obj(obj), ch, recur);
  } else
    send_to_char(g, ch, "in an unknown location\r\n");
}

namespace {
  template <int Flag> bool flagged(GameWorld *g, room_vnum room) { return room_flagged(g, room, Flag); }

  struct planet {
    const char *name;
    bool (*has)(GameWorld *g, room_vnum room);
  };

  /* The first that a room is on; failing them all, it's unknown. */
  constexpr planet PLANETS[] = {
      {"@GEarth@n",   flagged<ROOM_EARTH>  },
      {"@CFrigid@n",  flagged<ROOM_FRIGID> },
      {"@YVegeta@n",  flagged<ROOM_VEGETA> },
      {"@MKonack@n",  flagged<ROOM_KONACK> },
      {"@gNamek@n",   flagged<ROOM_NAMEK>  },
      {"@mAether@n",  flagged<ROOM_AETHER> },
      {"@mArlia@n",   flagged<ROOM_ARLIA>  },
      {"@CZenith@n",  room_planet_zenith   },
      {"@YYardrat@n", flagged<ROOM_YARDRAT>},
      {"@cKanassa@n", flagged<ROOM_KANASSA>},
  };

  const char *planet_name(GameWorld *g, room_vnum room)
  {
    auto it = std::ranges::find_if(PLANETS, [&](const planet &p) { return p.has(g, room); });
    return it == std::end(PLANETS) ? "@RUNKOWN@n" : it->name;
  }
} // namespace

static void perform_immort_where(GameWorld *g, struct char_data *ch, const std::string &arg)
{
  if (arg.empty()) {
    mudlog(g, LOG_NRM, MAX(ADMLVL_GRGOD, char_get_invis_level(g, ch)), true, "GODCMD: {} has checked where to check player locations", char_get_name(ch));
    send_to_char(g, ch, "Players                  Vnum    Planet        Location\r\n-------                 ------   ----------    ----------------\r\n");
    for (descriptor_data *d : each_desc(g)) {
      if (!desc_is_playing(d))
        continue;
      struct char_data *i = d->original ? d->original : d->character;
      if (!i || !char_can_see(g, ch, i) || char_get_in_room(i) == NOWHERE)
        continue;
      if (d->original)
        send_to_char(g, ch, "{:<20} - [{:>5d}]   {} (in {})\r\n", char_get_name(i), char_get_in_room(d->character),
                     room_get_name(g, char_get_in_room(d->character)), char_get_name(d->character));
      else
        send_to_char(g, ch, "{:<20} - [{:>5d}]   {:<14} {}\r\n", char_get_name(i), char_get_in_room(i), planet_name(g, char_get_in_room(i)), room_get_name(g, char_get_in_room(i)));
    }
    return;
  }

  mudlog(g, LOG_NRM, MAX(ADMLVL_GRGOD, char_get_invis_level(g, ch)), true, "GODCMD: {} has checked where for the location of {}", char_get_name(ch), arg);
  int num = 0;
  for (char_data *tch : each_char(g)) {
    if (!char_can_see(g, ch, tch) || char_get_in_room(tch) == NOWHERE || !isname(arg, char_get_alias(tch)))
      continue;
    send_to_char(g, ch, "M{:>3d}. {:<25} - [{:>5d}] {:<25}", ++num, char_get_name(tch), char_get_in_room(tch), room_get_name(g, char_get_in_room(tch)));
    if (char_is_npc(tch) && char_get_script(tch)) {
      if (!char_get_script(tch)->trig_list->next)
        send_to_char(g, ch, "[T{:>5d}] ", trig_get_vnum(g, char_get_script(tch)->trig_list));
      else
        send_to_char(g, ch, "[TRIGS] ");
    }
    send_to_char(g, ch, "\r\n");
  }
  for (obj_data *k : each_obj(g))
    if (char_can_see_obj(g, ch, k) && isname(arg, obj_get_name(k)))
      print_object_location(g, ++num, k, ch, true);
  if (!num)
    send_to_char(g, ch, "Couldn't find any such thing.\r\n");
  else
    send_to_char(g, ch, "\r\nFound {:d} matches.\r\n", num);
}

ACMD(do_where)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_adm_flagged(ch, ADM_FULLWHERE) || char_get_admlevel(ch) > 4)
    perform_immort_where(g, ch, arg);
  else
    perform_mortal_where(g, ch, arg);
}

namespace {
  /* The toplist file has five names for each, in this order; the last, roleplay points, isn't shown. */
  constexpr const char *TOPLIST_SECTIONS[] = {
      "       @D-@RPowerlevel@D-@n\r\n",
      "       @D-@BKi        @D-@n\r\n",
      "       @D-@GStamina   @D-@n\r\n",
      "       @D-@gZenni     @D-@n\r\n",
  };
} // namespace

ACMD(do_toplist)
{
  if (char_is_npc(ch))
    return;

  char fname[40], filler[50], line[256];
  FILE *file;
  if (!get_filename(fname, sizeof(fname), INTRO_FILE, "toplist") || !(file = fopen(fname, "r"))) {
    send_to_char(g, ch, "The toplist file does not exist.");
    return;
  }

  /* 25 places, a line each; any the file is short of are empty */
  std::array<std::string, 25> titles;
  for (std::size_t count = 0; count < titles.size() && get_line(file, line); count++) {
    int64_t stats;
    *filler = '\0';
    sscanf(line, "%49s %" SCNd64 "\n", filler, &stats);
    titles[count] = filler;
  }
  fclose(file);

  send_to_char(g, ch, "@D-=[@BDBAT Top Lists for @REra@C {:d}@D]=-@n\r\n", CURRENT_ERA);
  for (std::size_t x = 0; x < std::size(TOPLIST_SECTIONS) * 5; x++) {
    if (x % 5 == 0)
      send_to_char(g, ch, "{}", TOPLIST_SECTIONS[x / 5]);
    send_to_char(g, ch, "    @D|@c{:d}@W: @C{:>13}@D|@n\r\n", x % 5 + 1, titles[x]);
  }
}

ACMD(do_whois)
{
  static constexpr const char *immlevels[ADMLVL_IMPL + 1] = {
      "[Mortal]", "[Enforcer]", "[First Class Enforcer]", "[High Enforcer]", "[Vice Admin]", "[Administrator]", "[Implementor]",
  };

  argument = skip_spaces(argument);
  if (argument.empty()) {
    send_to_char(g, ch, "Who?\r\n");
    return;
  }

  struct char_data *victim = new char_data();
  clear_char(victim);
  char_make_player_specials(victim);
  if (load_char(g, std::string(argument).c_str(), victim) < 0) {
    send_to_char(g, ch, "There is no such player.\r\n");
    free_char(g, victim);
    return;
  }
  send_to_char(g, ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\r\n");
  if (char_get_admlevel(victim) >= ADMLVL_IMMORT) {
    send_to_char(g, ch, "@cName     @D: @G{}\r\n", char_get_name(victim));
    send_to_char(g, ch, "@cImm Level@D: @G{}\r\n", immlevels[char_get_admlevel(victim)]);
    send_to_char(g, ch, "@cTitle    @D: @G{}\r\n", char_get_title(victim));
  } else {
    send_to_char(g, ch, "@cName  @D: @w{}\r\n@cSensei@D: @w{}\r\n@cRace  @D: @w{}\r\n@cTitle @D: @w{}@n\r\n@cClan  @D: @w{}@n\r\n", char_get_name(victim),
                 char_sensei_name(victim), char_true_race_name(victim), char_get_title(victim), "None.");
  }
  send_to_char(g, ch, "@D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~@n\r\n");
  free_char(g, victim);
}
