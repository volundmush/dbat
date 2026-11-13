/* dragonballs.c: dragon balls, wishes and summoning
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include <algorithm>
#include <array>
#include <functional>
#include <string_view>

#include <sys/stat.h>

#include "dbat/iterate.hpp"
#include "dbat/act.other.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/shadowdragons.hpp"
#include "dbat/consts/auction.hpp"
#include "dbat/consts/aligns.hpp"
#include "dbat/utils.hpp"
#include "dbat/flags.hpp"
#include "dbat/config.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/search.hpp"
#include "dbat/random.hpp"
#include "dbat/affect.hpp"
#include "dbat/relocate.hpp"
#include "dbat/extract.hpp"
#include "dbat/races.hpp"
#include "dbat/fileop.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/combat.hpp"
#include "dbat/act.misc.hpp"
#include "dbat/weather.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/obj_edit.hpp"
#include "dbat/graph.hpp"
#include "dbat/alias.hpp"
#include "dbat/spells.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/fight.hpp"
#include "dbat/class.hpp"
#include "dbat/shop.hpp"
#include "dbat/feats.hpp"
#include "dbat/guild.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/objsave.hpp"
#include "dbat/mail.hpp"
#include "dbat/game_world.hpp"

#include "other_internal.hpp"

/* A dragon saved as being somewhere, when it or that room no longer exists, is forgotten. */
static void load_shadow_dragon(GameWorld *g, int *room, mob_vnum vnum)
{
  mob_vnum r_num;
  room_vnum r_room;

  if (*room <= 0)
    return;
  r_num  = vnum;
  r_room = *room;
  if (!mob_proto_of(g, r_num) || !room_of(g, r_room)) {
    mud_log("SYSERR: Can't put shadow dragon (mob {:d}) back in room {:d}: one of them doesn't exist.", vnum, *room);
    *room = -1;
    return;
  }
  char_to_room(g, read_mobile(g, r_num), r_room);
}

void load_shadow_dragons(GameWorld *g)
{
  load_shadow_dragon(g, &g->SHADOW_DRAGON1, SHADOW_DRAGON1_VNUM);
  load_shadow_dragon(g, &g->SHADOW_DRAGON2, SHADOW_DRAGON2_VNUM);
  load_shadow_dragon(g, &g->SHADOW_DRAGON3, SHADOW_DRAGON3_VNUM);
  load_shadow_dragon(g, &g->SHADOW_DRAGON4, SHADOW_DRAGON4_VNUM);
  load_shadow_dragon(g, &g->SHADOW_DRAGON5, SHADOW_DRAGON5_VNUM);
  load_shadow_dragon(g, &g->SHADOW_DRAGON6, SHADOW_DRAGON6_VNUM);
  load_shadow_dragon(g, &g->SHADOW_DRAGON7, SHADOW_DRAGON7_VNUM);

  save_mud_time(g, &g->time_info);
}

void wishSYS(GameWorld *g)
{
  if (g->SHENRON == true) {
    if (g->SELFISHMETER < 10) {
      switch (g->DRAGONC) {
        case 300:
          send_to_room(g, g->DRAGONR, "@WThe dragon balls on the ground begin to glow yellow in slow pulses.@n\r\n");
          send_to_planet(g, 0, ROOM_EARTH, "@DThe sky begins to grow dark and cloudy suddenly.@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 295:
          send_to_room(g, g->DRAGONR, "@WSuddenly lightning shoots into the sky, twisting about as a roar can be heard for miles!@n\r\n");
          send_to_planet(g, 0, ROOM_EARTH, "@DThe sky flashes with lightning.@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 290:
          send_to_room(g, g->DRAGONR, "@WThe lightning takes shape and slowly the Eternal Dragon, Shenron, can be made out from the glow!@n\r\n");
          if (char_data *dragon = char_by_id(g, g->EDRAGON_uid)) {
            char_from_room(g, dragon);
            char_to_room(g, dragon, g->DRAGONR);
          }
          g->DRAGONC -= 1;
          break;
        case 285:
          send_to_planet(g, 0, ROOM_EARTH, "@DThe lightning stops suddenly, but the sky remains mostly dark.@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 280:
          send_to_room(g, g->DRAGONR, "@WThe glow around Shenron becomes subdued as the Eternal Dragon coils so that his head is looking down on the dragon balls!@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 275:
          send_to_room(g, g->DRAGONR, "@wShenron says, '@CWho summoned me? I will grant you any two wishes that are within my power.@w'@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 180:
          send_to_room(g, g->DRAGONR, "@wShenron says, '@CMake your wish already, you only have 3 minutes remaining.@w'@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 120:
          send_to_room(g, g->DRAGONR, "@wShenron says, '@CMake your wish. I am losing patience, you only have 2 minutes left.@w'@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 60:
          send_to_room(g, g->DRAGONR, "@wShenron says, '@CMake your wish now! You only have 1 minute left.@w'@n\r\n");
          g->DRAGONC -= 1;
          break;
        case 0:
          send_to_room(g, g->DRAGONR, "Shenron growls and disappears with a blinding flash that is absorbed into the dragon balls. The glowing dragon balls then float high into the sky, splitting into several directions and streaking across the sky!@n\r\n");
          send_to_planet(g, 0, ROOM_EARTH, "@DThe sky grows brighter again as the clouds disappear magicly.@n\r\n");
          if (char_data *dragon = char_by_id(g, g->EDRAGON_uid))
            extract_char(g, dragon);
          g->SHENRON = false;
          g->DRAGONC -= 1;
          save_mud_time(g, &g->time_info);
          break;
        default:
          g->DRAGONC -= 1;
          break;
      }
      if (g->WISH[0] == 1 && g->WISH[1] == 1) {
        g->DRAGONC = 0;
        g->WISH[0] = 0;
        g->WISH[1] = 0;
      }
    } else {
      send_to_room(g, g->DRAGONR, "@RThe dragon balls suddenly begin to crack and darkness begins to pour out through the cracks! Shenron begins to turn pitch black slowly as the darkness escapes. Suddenly Shenron explodes out into the distance in seven parts. Each part taking a dragon ball with it!@n\r\n");
      room_vnum num = rand_number(g, 200, 20000);
      int done = false, place = 1;
      g->DRAGONC = 0;
      g->WISH[0] = 0;
      g->WISH[1] = 0;
      while (done == false) {
        switch (place) {
          case 1:
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
                g->SHADOW_DRAGON1 = num;
                place          = 2;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
          case 2:
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
                g->SHADOW_DRAGON2 = num;
                place          = 3;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
          case 3:
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
                g->SHADOW_DRAGON3 = num;
                place          = 4;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
          case 4:
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
                g->SHADOW_DRAGON4 = num;
                place          = 5;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
          case 5:
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
                g->SHADOW_DRAGON5 = num;
                place          = 6;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
          case 6:
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
                g->SHADOW_DRAGON6 = num;
                place          = 7;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
          case 7:
            if (room_of(g, num)) {
              if (room_flagged(g, num, ROOM_EARTH) || room_flagged(g, num, ROOM_VEGETA) || room_flagged(g, num, ROOM_FRIGID) || room_flagged(g, num, ROOM_AETHER) || room_flagged(g, num, ROOM_NAMEK) || room_flagged(g, num, ROOM_KONACK) || room_flagged(g, num, ROOM_YARDRAT) || room_flagged(g, num, ROOM_YARDRAT)) {
                g->SHADOW_DRAGON7 = num;
                done           = true;
                num            = rand_number(g, 200, 20000);
              } else {
                num = rand_number(g, 200, 20000);
              }
            } else {
              num = rand_number(g, 200, 20000);
            }
            break;
        } /* End switch */
        save_mud_time(g, &g->time_info);
      } /* End while */
      struct char_data *mob = nullptr;
      mob_vnum r_num;

      r_num = SHADOW_DRAGON1_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON1);
      mob = nullptr;

      r_num = SHADOW_DRAGON2_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON2);
      mob = nullptr;

      r_num = SHADOW_DRAGON3_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON3);
      mob = nullptr;

      r_num = SHADOW_DRAGON4_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON4);
      mob = nullptr;

      r_num = SHADOW_DRAGON5_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON5);
      mob = nullptr;

      r_num = SHADOW_DRAGON6_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON6);
      mob = nullptr;

      r_num = SHADOW_DRAGON7_VNUM;
      mob   = read_mobile(g, r_num);
      char_to_room(g, mob, g->SHADOW_DRAGON7);
      mob = nullptr;

      if (char_data *dragon = char_by_id(g, g->EDRAGON_uid))
        extract_char(g, dragon);
      g->SHENRON = false;
      g->DRAGONC = 0;
    } /* End else */
  }
}

namespace {
  /* The dragon balls are 20 to 26, one star to seven */
  constexpr obj_vnum FIRST_DRAGON_BALL = 20, LAST_DRAGON_BALL = 26;

  bool is_dragon_ball(GameWorld *g, struct obj_data *obj)
  {
    obj_vnum vnum = obj_get_vnum(g, obj);
    return vnum >= FIRST_DRAGON_BALL && vnum <= LAST_DRAGON_BALL;
  }
} // namespace

ACMD(do_summon)
{
  if (!room_flagged(g, char_get_in_room(ch), ROOM_EARTH)) {
    send_to_char(g, ch, "@wYou can not summon Shenron when you are not on earth.@n\r\n");
    return;
  }
  if (room_flagged(g, char_get_in_room(ch), ROOM_NOINSTANT) || room_flagged(g, char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(g, ch, "You can not summon shenron in this protected area!\r\n");
    return;
  }
  if (room_sect(g, char_get_in_room(ch)) == SECT_INSIDE) {
    send_to_char(g, ch, "Go outside to summon Shenron!\r\n");
    return;
  }
  /* all seven, and none of them fakes */
  std::array<bool, 7> held{};
  for (obj_data *obj : char_inventory(g, ch))
    if (!obj_extra_flagged(obj, ITEM_FORGED) && is_dragon_ball(g, obj))
      held[obj_get_vnum(g, obj) - FIRST_DRAGON_BALL] = true;
  if (!std::ranges::all_of(held, std::identity())) {
    send_to_char(g, ch, "@wYou do not have all the dragon balls and can not summon the dragon!@n\r\n");
    return;
  }

  reveal_hiding(g, ch, 0);
  act(g, "@WYou place the dragon balls on the ground and with both hands outstretched towards them you say '@CArise Eternal Dragon Shenron!@W'@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@W$n places the dragon balls on the ground and with both hands outstretched towards them $e says '@CArise Eternal Dragon Shenron!@W'@n", true, ch, nullptr, nullptr, TO_ROOM);
  g->SHENRON = true;
  g->DRAGONC = 300;
  g->DRAGONR = char_get_in_room(ch);
  send_to_imm(g, "Shenron summoned to room: {:d}\r\n", g->DRAGONR);
  if (struct zone_data *zone = find_zone_by_thing(g, g->DRAGONR))
    g->DRAGONZ = zone->number;

  /* the first of each ball carried is used up; quirk: a fake can be used up in place of the real one */
  std::array<bool, 7> used{};
  for (obj_data *obj : char_inventory(g, ch)) {
    if (!is_dragon_ball(g, obj) || used[obj_get_vnum(g, obj) - FIRST_DRAGON_BALL])
      continue;
    used[obj_get_vnum(g, obj) - FIRST_DRAGON_BALL] = true;
    obj_from_char(g, obj);
    extract_obj(g, obj);
  }
  if (!mob_proto_of(g, 21)) {
    send_to_imm(g, "Shenron doesn't exist!");
    return;
  }
  struct char_data *mob = read_mobile(g, 21);
  char_to_room(g, mob, 0);
  g->EDRAGON_uid = char_get_id(mob);
}

/* Whether ch carries any dragon ball, fake or not */
bool dball_count(GameWorld *g, struct char_data *ch)
{
  return std::ranges::any_of(char_inventory(g, ch), [&](obj_data *obj) { return is_dragon_ball(g, obj); });
}

/* Wishes: what Shenron does with what is said to him. */
namespace {
  /* Who a wish names: at most three players, in the order they connected. */
  struct wisher {
    GameWorld *g;
    struct char_data *ch;
    std::array<struct char_data *, 3> named{};

    int count() const { return static_cast<int>(std::ranges::count_if(named, [](auto *c) { return c != nullptr; })); }
  };

  const char *second_wish(GameWorld *g) { return g->WISH[0] ? "" : " Now make your second wish."; }

  void shenron_says_no(GameWorld *g)
  {
    send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish cannot be granted, You might want to try something else instead, mortal!@w'@n\r\n");
  }

  /* A boon: one named player gains something, and the wish is granted. */
  struct boon {
    const char *word;
    const char *granted; /* "{name} ..." */
    const char *logged;  /* "... has made {logged} on {name}." */
    void (*bestow)(GameWorld *g, struct char_data *wch);
    bool saves;
  };

  template <int8_t (*Get)(const struct char_data *), void (*Set)(struct char_data *, int8_t)>
  void raise_by_10(GameWorld *, struct char_data *wch)
  {
    Set(wch, Get(wch) + 10);
    if (Get(wch) > 100)
      Set(wch, 100);
  }

  void more_practice(GameWorld *g, struct char_data *wch) { char_set_practice_points(g, wch, char_get_practice_points(g, wch) + rand_number(g, 2000, 5000)); }

  void more_armor(GameWorld *, struct char_data *wch) { char_set_armor(wch, char_get_armor(wch) + 5000); }

  void more_skill_slots(GameWorld *g, struct char_data *wch)
  {
    int roll = rand_number(g, 1, 3);
    send_to_char(g, wch, "@GYou suddenly feel like you could learn {:d} more skills!@n\r\n", roll);
    char_set_skill_slots(wch, char_get_skill_slots(wch) + roll);
  }

  void more_money(GameWorld *, struct char_data *wch) { char_set_bank_gold(wch, char_get_bank_gold(wch) + 1000000); }

  void immunity(GameWorld *, struct char_data *wch) { char_aff_set(wch, AFF_IMMUNITY, true); }

  constexpr boon KNOWLEDGE    = {"knowledge", "now has more knowledge", "a knowledge wish", more_practice, false};
  constexpr boon SPEED        = {"speed", "is now faster", "a speed wish", raise_by_10<char_get_real_abils_cha, char_set_real_abils_cha>, true};
  constexpr boon TOUGH        = {"tough", "is now tougher", "a tough wish", more_armor, false};
  constexpr boon STRENGTH     = {"strength", "has more strength", "a strength wish", raise_by_10<char_get_real_abils_str, char_set_real_abils_str>, true};
  constexpr boon INTELLIGENCE = {"intelligence", "is now smarter", "a intelligence wish", raise_by_10<char_get_real_abils_intel, char_set_real_abils_intel>, true};
  constexpr boon WISDOM       = {"wisdom", "is now wiser", "a wisdom wish", raise_by_10<char_get_real_abils_wis, char_set_real_abils_wis>, false /* quirk */};
  constexpr boon AGILITY      = {"agility", "is now more agile", "a agility wish", raise_by_10<char_get_real_abils_dex, char_set_real_abils_dex>, true};
  constexpr boon CONSTITUTION = {"constitution", "has more guts", "a constitutionwish" /* quirk */, raise_by_10<char_get_real_abils_con, char_set_real_abils_con>, true};
  constexpr boon SKILL        = {"skill", "has more skill", "a skill wish", more_skill_slots, true};
  constexpr boon MONEY        = {"money", "now has become richer", "a money wish", more_money, false};
  constexpr boon IMMUNITY     = {"immunity", "now has immunity to Burn, Freezing, Mind Break, Poison, Blindness, Yoikominminken, and Paralysis", "a immunity wish", immunity, false};

  template <const boon &B>
  bool grant_boon(wisher &w)
  {
    GameWorld *g = w.g;
    struct char_data *wch = w.named[0];
    send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} {}!{}@w'@n\r\n", char_get_name(wch), B.granted, second_wish(g));
    B.bestow(g, wch);
    if (B.saves)
      save_char(g, wch);
    g->SELFISHMETER += 1;
    mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made {} on {}.", char_get_name(w.ch), B.logged, char_get_name(wch));
    char_set_wait(w.ch, PULSE_4SEC);
    return true;
  }

  /* Power and vitality were wishes once. */
  bool refuse(wisher &w)
  {
    shenron_says_no(w.g);
    char_set_wait(w.ch, PULSE_4SEC);
    return false;
  }

  void not_dead(GameWorld *g, struct char_data *wch)
  {
    send_to_room(g, g->DRAGONR, "@wShenron says, '@C{} is not dead, and can not be revived.@w'@n\r\n", char_get_name(wch));
  }

  /* quirk: reviving one sends a death room of 0 to room 300, reviving more sends them to room 0 */
  void return_to_life(GameWorld *g, struct char_data *wch, bool zero_is_300)
  {
    if (!room_of(g, char_get_droom(wch)))
      char_set_droom(wch, 300);
    if (!room_of(g, char_get_droom(wch)))
      return;
    char_from_room(g, wch);
    char_to_room(g, wch, zero_is_300 && char_get_droom(wch) <= 0 ? 300 : char_get_droom(wch));
    look_at_room(g, char_get_in_room(wch), wch, 0);
    send_to_char(g, wch, "@wYou smile as the golden halo above your head disappears! You have returned to life where you had last died!@n\r\n");
    char_aff_set(wch, AFF_SPIRIT, false);
    char_aff_set(wch, AFF_ETHEREAL, false);
  }

  bool revive(wisher &w)
  {
    GameWorld *g = w.g;
    auto &named  = w.named;
    int count    = w.count();
    auto dead    = [](struct char_data *c) { return char_aff_flagged(c, AFF_SPIRIT); };

    if (count == 1) {
      if (!dead(named[0])) {
        not_dead(g, named[0]);
        return false;
      }
      send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} has returned to life!{}@w'@n\r\n", char_get_name(named[0]), second_wish(g));
      return_to_life(g, named[0], true);
      g->SELFISHMETER -= 2;
      mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a revive wish on {}.", char_get_name(w.ch), char_get_name(named[0]));
      return true; /* quirk: no wait for this one */
    }

    /* The first ones are told of each, the last only if it is the one alive. */
    for (int i = 0; i < count - 1; i++)
      if (!dead(named[i]))
        not_dead(g, named[i]);
    if (!dead(named[count - 1])) {
      not_dead(g, named[count - 1]);
      return false;
    }
    if (!std::all_of(named.begin(), named.begin() + count, dead))
      return false;

    if (count == 2)
      send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} and {} have returned to life!{}@w'@n\r\n",
                   char_get_name(named[0]), char_get_name(named[1]), second_wish(g));
    else
      send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {}, {}, and {} have returned to life!!{}@w'@n\r\n",
                   char_get_name(named[0]), char_get_name(named[1]), char_get_name(named[2]), second_wish(g));
    for (int i = 0; i < count; i++)
      return_to_life(g, named[i], false);
    g->SELFISHMETER -= 3;
    /* quirk: the log leaves out the first one revived */
    if (count == 2)
      mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a revive wish on {}.", char_get_name(w.ch), char_get_name(named[1]));
    else
      mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a revive wish on {} and {}.", char_get_name(w.ch), char_get_name(named[1]), char_get_name(named[2]));
    char_set_wait(w.ch, PULSE_4SEC);
    return true;
  }

  /* Takes both wishes. */
  bool immortality(wisher &w)
  {
    GameWorld *g = w.g;
    if (g->WISH[0] != 0)
      return false;
    send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} is now immortal!@w'@n\r\n", char_get_name(w.named[0]));
    char_plr_set(w.named[0], PLR_IMMORTAL, true);
    g->WISH[0] = 1;
    g->WISH[1] = 1;
    g->SELFISHMETER += 4;
    mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a immortal wish on {}.", char_get_name(w.ch), char_get_name(w.named[0]));
    char_set_wait(w.ch, PULSE_4SEC);
    return true;
  }

  bool immortality_spent(wisher &w)
  {
    if (w.g->WISH[0] == 1)
      send_to_room(w.g, w.g->DRAGONR, "@wShenron says, '@CI can not grant that wish, there is not enough remaining power in this summoning!@w'@n\r\n");
    return false;
  }

  bool mortality(wisher &w)
  {
    GameWorld *g = w.g;
    send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} is now mortal!{}@w'@n\r\n", char_get_name(w.named[0]), second_wish(g));
    char_plr_set(w.named[0], PLR_IMMORTAL, false);
    g->SELFISHMETER += 4;
    mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a mortal wish on {}.", char_get_name(w.ch), char_get_name(w.named[0]));
    char_set_wait(w.ch, PULSE_4SEC);
    return true;
  }

  /* quirk: the one wishing gets the senzus, and has no wait */
  bool senzus(wisher &w)
  {
    GameWorld *g = w.g;
    for (int i = 0; i < 10; i++)
      obj_to_char(g, read_object(g, 1), w.ch);
    send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {} now possesses 10 senzus!{}@w'@n\r\n", char_get_name(w.named[0]), second_wish(g));
    g->SELFISHMETER += 1;
    mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a senzu wish.", char_get_name(w.ch));
    return true;
  }

  bool roleplay(wisher &w)
  {
    GameWorld *g = w.g;
    send_to_room(g, g->DRAGONR, "@wShenron says, '@CYour wish has been granted, {}!{}@w'@n\r\n", char_get_name(w.named[0]), second_wish(g));
    mudlog(g, LOG_NRM, ADMLVL_GOD, true, "Shenron: {} has made a roleplay wish.", char_get_name(w.ch));
    char_set_wait(w.ch, PULSE_4SEC);
    return true;
  }

  struct wish {
    const char *word;
    bool (*grant)(wisher &w); /* true if granted */
  };

  /* Tried in order until one is granted. */
  constexpr wish WISHES[] = {
      {"knowledge",    grant_boon<KNOWLEDGE>   },
      {"speed",        grant_boon<SPEED>       },
      {"tough",        grant_boon<TOUGH>       },
      {"strength",     grant_boon<STRENGTH>    },
      {"intelligence", grant_boon<INTELLIGENCE>},
      {"wisdom",       grant_boon<WISDOM>      },
      {"agility",      grant_boon<AGILITY>     },
      {"constitution", grant_boon<CONSTITUTION>},
      {"skill",        grant_boon<SKILL>       },
      {"power",        refuse                  },
      {"money",        grant_boon<MONEY>       },
      {"immunity",     grant_boon<IMMUNITY>    },
      {"vitality",     refuse                  },
      {"revive",       revive                  },
      {"immortal",     immortality             },
      {"immortal",     immortality_spent       },
      {" mortal",      mortality               },
      {"senzu",        senzus                  },
      {"roleplay",     roleplay                },
  };
} // namespace

bool shenron_hears(GameWorld *g, struct char_data *ch, std::string_view text)
{
  if (!g->SHENRON || char_get_in_room(ch) != g->DRAGONR)
    return true;
  struct char_data *dragon = char_by_id(g, g->EDRAGON_uid);
  if (!dragon || char_get_in_room(dragon) != g->DRAGONR || !text.contains("wish"))
    return true;

  wisher w{g, ch};
  for (descriptor_data *d : each_desc(g)) {
    if (desc_get_connected(d) != CON_PLAYING || !text.contains(char_get_name(d->character)))
      continue;
    if (auto slot = std::ranges::find(w.named, nullptr); slot != w.named.end())
      *slot = d->character;
  }
  if (!w.named[0] && text.contains("myself"))
    w.named[0] = ch;
  if (!w.named[0])
    return false; /* quirk: nothing else hears it, not even the speech triggers */

  bool granted = false;
  for (const wish &x : WISHES) {
    if (text.contains(x.word) && x.grant(w)) {
      granted = true;
      break;
    }
  }

  if (!granted) {
    send_to_room(g, g->DRAGONR, "@wShenron says, '@CDo not waste my time with wishes I can not grant...@w'@n\r\n");
    return true;
  }
  if (g->WISH[0] == 1)
    g->WISH[1] = 1;
  else
    g->WISH[0] = 1;
  /* quirk: saves the first one named once for each one named */
  for (int i = 0; i < w.count(); i++)
    save_char(g, w.named[0]);
  save_mud_time(g, &g->time_info);
  return true;
}
