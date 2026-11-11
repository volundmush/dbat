/* ************************************************************************
 *   File: Guild.c                                                         *
 *  Usage: GuildMaster's: loading files, assigning spec_procs, and handling*
 *                        practicing.                                      *
 *                                                                         *
 * Based on shop.c.  As such, the CircleMud License applies                *
 * Written by Jason Goodwin.   jgoodwin@expert.cc.purdue.edu               *
 ************************************************************************ */
#include <cstdlib>
#include <memory>

#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/guild.hpp"
#include "dbat/config.hpp"
#include "dbat/affect.hpp"
#include "dbat/fileop.hpp"
#include "dbat/utils.hpp"
#include "dbat/spells.hpp"
#include "dbat/comm.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/gengld.hpp"
#include "dbat/genolc.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/feats.hpp"
#include "dbat/act.comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/shop.hpp"
#include "dbat/class.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static bool guild_trade_flagged(const struct guild_data *rec, int flag);

const char *const guild_default_no_skill = "%s Sorry, but I don't know that one.";
const char *const guild_default_no_gold  = "%s Sorry, but I'm gonna need more zenni first.";

/* Local variables */

static std::string guild_customer_string(GameWorld *g, struct guild_data *guild_nr, int detailed);
static int calculate_skill_cost(GameWorld *g, struct char_data *ch, int skill);

static int calculate_skill_cost(GameWorld *g, struct char_data *ch, int skill)
{
  int cost = 0;

  if (IS_SET(g->spell_info[skill].flags, SKFLAG_TIER2)) {
    cost = 8;
  } else if (IS_SET(g->spell_info[skill].flags, SKFLAG_TIER3)) {
    cost = 15;
  } else if (IS_SET(g->spell_info[skill].flags, SKFLAG_TIER4)) {
    if (char_get_skills_base(ch, skill) == 0) {
      cost = 200;
    } else {
      cost = 25;
    }
  } else if (IS_SET(g->spell_info[skill].flags, SKFLAG_TIER5)) {
    if (char_get_skills_base(ch, skill) == 0) {
      cost = 300;
    } else {
      cost = 40;
    }
  } else {
    cost = 4;
  }

  if (char_get_skills_base(ch, skill) > 90)
    cost += 12;
  else if (char_get_skills_base(ch, skill) > 80)
    cost += 10;
  else if (char_get_skills_base(ch, skill) > 70)
    cost += 8;
  else if (char_get_skills_base(ch, skill) > 50)
    cost += 6;
  else if (char_get_skills_base(ch, skill) > 40)
    cost += 2;
  else if (char_get_skills_base(ch, skill) > 30)
    cost += 1;

  if (char_get_forgeting(ch) != 0)
    cost += 6;

  if (skill == SKILL_RUNIC)
    cost += 6;
  if (skill == SKILL_EXTRACT)
    cost += 3;

  if (char_is_hoshijin(ch) && (skill == SKILL_PUNCH || skill == SKILL_KICK || skill == SKILL_KNEE || skill == SKILL_ELBOW || skill == SKILL_UPPERCUT || skill == SKILL_ROUNDHOUSE || skill == SKILL_SLAM || skill == SKILL_HEELDROP || skill == SKILL_DAGGER || skill == SKILL_SWORD || skill == SKILL_CLUB || skill == SKILL_GUN || skill == SKILL_SPEAR || skill == SKILL_BRAWL)) {
    cost += 5;
  }

  if (skill == SKILL_INSTANTT) {
    if (char_get_skills_base(ch, skill) == 0) {
      cost = 2000;
    } else {
      cost = 50;
    }
  }
  if (skill == SKILL_MYSTICMUSIC) {
    cost = cost * 1.5;
  }

  return (cost);
}

void handle_ingest_learn(GameWorld *g, struct char_data *ch, struct char_data *vict)
{

  int i = 1;
  send_to_char(g, ch, "@YAll your current skills improve somewhat!@n\r\n");

  for (i = 0; i < SKILL_TABLE_SIZE; i++) {

    if (char_get_skills_base(ch, i) > 0 && char_get_skills_base(vict, i) > 0 && i != 141) {
      send_to_char(g, ch, "@YYou gained a lot of new knowledge about @y{}@Y!@n\r\n", g->spell_info[i].name);

      if (char_get_skills_base(ch, i) + 10 < 100) {
        char_set_skills_base(ch, i, char_get_skills_base(ch, i) + (10));
      } else if (char_get_skills_base(ch, i) > 0 && char_get_skills_base(ch, i) < 100) {
        char_set_skills_base(ch, i, char_get_skills_base(ch, i) + (1));
      } else {
        char_set_skills_base(ch, i, 100);
      }
    }
    if (((i >= 481 && i <= 489) || i == 517 || i == 535) && ((char_get_skills_base(ch, i) <= 0) && char_get_skills_base(vict, i) > 0)) {
      char_set_skills_base(ch, i, char_get_skills_base(ch, i) + rand_number(g, 10, 25));
      send_to_char(g, ch, "@YYou learned @y{}@Y from ingesting your target!@n\r\n", g->spell_info[i].name);
      char_set_skill_slots(ch, char_get_skill_slots(ch) + (1));
      char_set_ingestLearned(ch, 1);
    }
  }
}

ACMD(do_teach)
{

  if (char_is_npc(ch))
    return;

  std::string arg, arg2;
  int skill = 100;
  struct char_data *vict;

  std::string_view rest = argument;
  arg                   = take_arg(rest);
  arg2                  = take_arg(rest);

  if (arg.empty()) {
    send_to_char(g, ch, "What skill are you wanting to teach?\r\n");
    return;
  }

  skill = find_skill_num(g, arg.data(), SKTYPE_SKILL);

  if (skill < 0 || skill >= SKILL_TABLE_SIZE) {
    send_to_char(g, ch, "@MThat is not a valid skill!@n\r\n");
    return;
  }

  if (char_get_skills_base(ch, skill) < 101) {
    send_to_char(g, ch, "You are not a Grand Master in that skill!\r\n");
    send_to_char(g, ch, "@wSyntax: teach (skill) (target)@n\r\n");
    return;
  }

  if (arg2.empty()) {
    send_to_char(g, ch, "@wWho are you wanting to teach @C{}@w to?@n\r\n", g->spell_info[skill].name);
    send_to_char(g, ch, "@wSyntax: teach (skill) (target)@n\r\n");
    return;
  }

  if (!(vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "@wTeach who?@n\r\n");
    send_to_char(g, ch, "@wSyntax: teach (skill) (target)@n\r\n");
    return;
  }

  int cost = calculate_skill_cost(g, vict, skill), free = false;

  if (char_get_skills_base(ch, skill) >= 103) {
    cost = cost * 0.5;
    if (rand_number(g, 1, 4) == 4) {
      free = true;
    }
  } else if (char_get_skills_base(ch, skill) == 102) {
    cost = cost * 0.5;
  } else {
    cost = cost * 0.75;
  }

  if (cost == 0) /* Just to be sure */
    cost = 1;

  if (!char_get_master(g, vict)) {
    send_to_char(g, ch, "They must be following you in order for you to teach them.\r\n");
    return;
  }
  if (char_get_master(g, vict) != ch) {
    send_to_char(g, ch, "They must be following you in order for you to teach them.\r\n");
    return;
  }
  if (char_get_forgeting(vict) == skill) {
    send_to_char(g, ch, "They are trying to forget that skill!\r\n");
    return;
  }
  if (char_get_practice_points(g, vict) < cost) {
    send_to_char(g, ch, "They do not have enough practice sessions for you to teach them.\r\n");
    return;
  }
  if (char_get_skills_base(vict, skill) >= 80) {
    send_to_char(g, ch, "You can not teach them anymore.\r\n");
    return;
  }
  if (char_get_skills_base(vict, skill) > 0) {
    std::string tochar;
    std::string tovict;
    std::string toother;
    tochar  = str_format("@YYou instruct @y$N@Y in the finer points of @C{}@Y.@n\r\n", g->spell_info[skill].name);
    tovict  = str_format("@y$n@Y instructs you in the finer points of @C{}@Y.@n\r\n", g->spell_info[skill].name);
    toother = str_format("@y$n@Y instructs @y$N@Y in the finer points of @C{}@Y.@n\r\n", g->spell_info[skill].name);
    act(g, tochar.c_str(), true, ch, nullptr, vict, TO_CHAR);
    act(g, tovict.c_str(), true, ch, nullptr, vict, TO_VICT);
    act(g, toother.c_str(), true, ch, nullptr, vict, TO_NOTVICT);
    char_set_skills_base(vict, skill, char_get_skills_base(vict, skill) + 1);
    if (free == false) {
      char_set_practice_points(g, vict, char_get_practice_points(g, vict) - (cost));
    } else {
      send_to_char(g, ch, "@GYou teach your lesson so well that it cost them nothing to learn from you!@n\r\n");
      send_to_char(g, vict, "@GYour teacher taught you the lesson so well that it cost you nothing!@n\r\n");
    }
  } else {
    send_to_char(g, ch, "They do not even know the basics. It's a waste of your teaching skills.\r\n");
    return;
  }
}

static int compare_spells(const void *x, const void *y, void *world)
{
  GameWorld *g = static_cast<GameWorld *>(world);
  int a = *(const int *)x,
      b = *(const int *)y;

  return strcmp(g->spell_info[a].name, g->spell_info[b].name);
}

std::string print_skills_by_type(GameWorld *g, struct char_data *ch, int sktype, std::string_view argument)
{
  int i, t, known, count = 0, canknow = 0;
  std::string buf, buf2;

  std::string arg = take_arg(argument);

  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    t = g->spell_info[i].skilltype;

    if (t != sktype)
      continue;

    known = char_get_skill(ch, i);
    if (!arg.empty()) {
      if (parse_int(arg) <= 0 && !strstr(g->spell_info[i].name, arg.c_str())) {
        known = 0;
      } else if (parse_int(arg) > char_get_skill(ch, i)) {
        known = 0;
      }
    }

    if (known == 0) continue;

    if (t & SKTYPE_LANG) {
      buf += str_format("{:<20}  ({})\r\n",
                        g->spell_info[i].name, char_get_skills_base(ch, i) ? "known" : "unknown");
    } else if (t & SKTYPE_SKILL) {
      if (char_get_skills_mod(ch, i))
        buf2 = str_format(" (base {:d} + bonus {:d})", char_get_skills_base(ch, i), char_get_skills_mod(ch, i));
      else
        buf2.clear();
      count++;
      canknow = highest_skill_value(char_get_level(ch), char_get_skill(ch, i));
      buf += str_format("@y(@Y{:>2d}@y) @W{:<30}  @y(@Y{:>2d}@y) @C{:>3d}@D/@c{:>3d}   {}@n{}{}\r\n", count,
                        g->spell_info[i].name, count, char_get_skill(ch, i), canknow, char_get_skills_perf(ch, i) > 0 ? (char_get_skills_perf(ch, i) == 1 ? "@ROver Charge" : (char_get_skills_perf(ch, i) == 2 ? "@BAccurate" : "@GEfficient")) : "", char_get_skills_base(ch, i) > 100 ? " @D(@YGrand Master@D)@n" : "", buf2);
    }
  }

  return buf;
}

int slot_count(struct char_data *ch)
{
  int i, skills = -1, fail = false;
  int punch = false, kick = false, knee = false, elbow = false, kiball = false, kiblast = false, beam = false, renzo = false, shogekiha = false;

  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    if (char_get_skill(ch, i) > 0) {
      switch (i) {
        case SKILL_PUNCH:
          fail  = true;
          punch = true;
          break;
        case SKILL_KICK:
          fail = true;
          kick = true;
          break;
        case SKILL_ELBOW:
          fail  = true;
          elbow = true;
          break;
        case SKILL_KNEE:
          fail = true;
          knee = true;
          break;
        case SKILL_KIBALL:
          fail   = true;
          kiball = true;
          break;
        case SKILL_KIBLAST:
          fail    = true;
          kiblast = true;
          break;
        case SKILL_BEAM:
          fail = true;
          beam = true;
          break;
        case SKILL_SHOGEKIHA:
          fail      = true;
          shogekiha = true;
          break;
        case SKILL_RENZO:
          fail  = true;
          renzo = true;
          break;
        case SKILL_TELEPATHY:
          if (char_is_kanassan(ch) || char_is_kai(ch)) {
            fail = true;
          }
          break;
        case SKILL_ABSORB:
          if (char_is_bio(ch) || char_is_android(ch)) {
            fail = true;
          }
          break;
        case SKILL_TAILWHIP:
          if (char_is_icer(ch)) {
            fail = true;
          }
          break;
        case SKILL_SEISHOU:
          if (char_is_arlian(ch)) {
            fail = true;
          }
          break;
        case SKILL_REGENERATE:
          if (char_is_majin(ch) || char_is_namek(ch) || char_is_bio(ch)) {
            fail = true;
          }
          break;
      }
      if (fail == false) {
        skills += 1;
      }
      fail = false;
    }
  }

  if (punch == true && kick == true && elbow == true && knee == true) {
    skills += 1;
  }
  if (kiball == true && kiblast == true && beam == true && shogekiha == true && renzo == true) {
    skills += 1;
  }

  return (skills);
}

void list_skills(GameWorld *g, struct char_data *ch, std::string_view arg)
{
  int slots            = false;
  std::string buf2;

  buf2 = str_format("You have {:d} practice session{} remaining.\r\n", char_get_practice_points(g, ch), char_get_practice_points(g, ch) == 1 ? "" : "s");

  buf2 += "\r\nYou know the following skills:     @CKnown@D/@cPrac. Max@n\r\n@w-------------------------------------------------------@n\r\n";

  buf2 += str_format("{}", print_skills_by_type(g, ch, SKTYPE_SKILL, arg));

  if (slots == false) {
    buf2 += str_format("\r\n@DSkill Slots@W: @M{:d}@W/@m{:d}", slot_count(ch), char_get_skill_slots(ch));
  }

  send_to_char(g, ch, buf2);
}

static bool is_guild_open(GameWorld *g, struct char_data *keeper, struct guild_data *guild_nr, int msg)
{
  std::string buf;

  if (guild_get_open(guild_nr) > g->time_info.hours &&
      guild_get_close(guild_nr) < g->time_info.hours)
    buf = MSG_TRAINER_NOT_OPEN;

  if (buf.empty())
    return (true);
  if (msg)
    do_say(g, keeper, buf, g->cmd_tell, 0);

  return (false);
}

static bool is_guild_ok_char(GameWorld *g, struct char_data *keeper, struct char_data *ch, struct guild_data *guild_nr)
{
  std::string buf;

  if (!(char_can_see(g, keeper, ch))) {
    do_say(g, keeper, MSG_TRAINER_NO_SEE_CH, g->cmd_say, 0);
    return (false);
  }

  if (char_get_level(ch) < guild_get_minlvl(guild_nr)) {
    buf = str_format("{} {}",
                     char_get_name(ch), MSG_TRAINER_MINLVL);
    do_tell(g, keeper, buf.c_str(), g->cmd_tell, 0);
    return (false);
  }

  if ((char_is_good(ch) && guild_trade_flagged(guild_nr, TRADE_NOGOOD)) ||
      (char_is_evil(ch) && guild_trade_flagged(guild_nr, TRADE_NOEVIL)) ||
      (char_is_neutral(ch) && guild_trade_flagged(guild_nr, TRADE_NONEUTRAL))) {
    buf = str_format("{} {}",
                     char_get_name(ch), MSG_TRAINER_DISLIKE_ALIGN);
    do_tell(g, keeper, buf.c_str(), g->cmd_tell, 0);
    return (false);
  }

  if (char_is_npc(ch))
    return (false);

  if ((char_is_roshi(ch) && guild_trade_flagged(guild_nr, TRADE_NOWIZARD)) ||
      (char_is_piccolo(ch) && guild_trade_flagged(guild_nr, TRADE_NOCLERIC)) ||
      (char_is_krane(ch) && guild_trade_flagged(guild_nr, TRADE_NOROGUE)) ||
      (char_is_nail(ch) && guild_trade_flagged(guild_nr, TRADE_NOFIGHTER)) ||
      (char_is_ginyu(ch) && guild_trade_flagged(guild_nr, TRADE_NOPALADIN)) ||
      (char_is_frieza(ch) && guild_trade_flagged(guild_nr, TRADE_NOSORCERER)) ||
      (char_is_tapion(ch) && guild_trade_flagged(guild_nr, TRADE_NODRUID)) ||
      (char_is_andsix(ch) && guild_trade_flagged(guild_nr, TRADE_NOBARD)) ||
      (char_is_dabura(ch) && guild_trade_flagged(guild_nr, TRADE_NORANGER)) ||
      (char_is_bardock(ch) && guild_trade_flagged(guild_nr, TRADE_NOMONK)) ||
      (char_is_kabito(ch) && guild_trade_flagged(guild_nr, TRADE_NOBARBARIAN)) ||
      (char_is_jinto(ch) && guild_trade_flagged(guild_nr, TRADE_NOARCANE_ARCHER)) ||
      (char_is_tsuna(ch) && guild_trade_flagged(guild_nr, TRADE_NOARCANE_TRICKSTER)) ||
      (char_is_kurzak(ch) && guild_trade_flagged(guild_nr, TRADE_NOARCHMAGE)) ||
      (char_is_assassin(ch) && guild_trade_flagged(guild_nr, TRADE_NOASSASSIN)) ||
      (char_is_blackguard(ch) && guild_trade_flagged(guild_nr, TRADE_NOBLACKGUARD)) ||
      (char_is_dragon_disciple(ch) && guild_trade_flagged(guild_nr, TRADE_NODRAGON_DISCIPLE)) ||
      (char_is_duelist(ch) && guild_trade_flagged(guild_nr, TRADE_NODUELIST)) ||
      (char_is_dwarven_defender(ch) && guild_trade_flagged(guild_nr, TRADE_NODWARVEN_DEFENDER)) ||
      (char_is_eldritch_knight(ch) && guild_trade_flagged(guild_nr, TRADE_NOELDRITCH_KNIGHT)) ||
      (char_is_hierophant(ch) && guild_trade_flagged(guild_nr, TRADE_NOHIEROPHANT)) ||
      (char_is_horizon_walker(ch) && guild_trade_flagged(guild_nr, TRADE_NOHORIZON_WALKER)) ||
      (char_is_loremaster(ch) && guild_trade_flagged(guild_nr, TRADE_NOLOREMASTER)) ||
      (char_is_mystic_theurge(ch) && guild_trade_flagged(guild_nr, TRADE_NOMYSTIC_THEURGE)) ||
      (char_is_shadowdancer(ch) && guild_trade_flagged(guild_nr, TRADE_NOSHADOWDANCER)) ||
      (char_is_thaumaturgist(ch) && guild_trade_flagged(guild_nr, TRADE_NOTHAUMATURGIST))) {

    buf = str_format("{} {}",
                     char_get_name(ch), MSG_TRAINER_DISLIKE_CLASS);
    do_tell(g, keeper, buf.c_str(), g->cmd_tell, 0);
    return (false);
  }

  if ((!char_is_roshi(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYWIZARD)) ||
      (!char_is_piccolo(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYCLERIC)) ||
      (!char_is_krane(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYROGUE)) ||
      (!char_is_bardock(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYMONK)) ||
      (!char_is_ginyu(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYPALADIN)) ||
      (!char_is_nail(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYFIGHTER)) ||
      (!char_is_frieza(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYSORCERER)) ||
      (!char_is_tapion(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYDRUID)) ||
      (!char_is_andsix(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYBARD)) ||
      (!char_is_dabura(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYRANGER)) ||
      (!char_is_kabito(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYBARBARIAN)) ||
      (!char_is_jinto(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYARCANE_ARCHER)) ||
      (!char_is_tsuna(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYARCANE_TRICKSTER)) ||
      (!char_is_kurzak(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYARCHMAGE)) ||
      (!char_is_assassin(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYASSASSIN)) ||
      (!char_is_blackguard(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYBLACKGUARD)) ||
      (!char_is_dragon_disciple(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYDRAGON_DISCIPLE)) ||
      (!char_is_duelist(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYDUELIST)) ||
      (!char_is_dwarven_defender(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYDWARVEN_DEFENDER)) ||
      (!char_is_eldritch_knight(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYELDRITCH_KNIGHT)) ||
      (!char_is_hierophant(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYHIEROPHANT)) ||
      (!char_is_horizon_walker(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYHORIZON_WALKER)) ||
      (!char_is_loremaster(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYLOREMASTER)) ||
      (!char_is_mystic_theurge(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYMYSTIC_THEURGE)) ||
      (!char_is_shadowdancer(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYSHADOWDANCER)) ||
      (!char_is_thaumaturgist(ch) && guild_trade_flagged(guild_nr, TRADE_ONLYTHAUMATURGIST))) {
    buf = str_format("{} {}",
                     char_get_name(ch), MSG_TRAINER_DISLIKE_CLASS);
    do_tell(g, keeper, buf.c_str(), g->cmd_tell, 0);
    return (false);
  }

  if ((char_is_human(ch) && guild_trade_flagged(guild_nr, TRADE_NOHUMAN)) ||
      (char_is_saiyan(ch) && guild_trade_flagged(guild_nr, TRADE_NOSAIYAN)) ||
      (char_is_icer(ch) && guild_trade_flagged(guild_nr, TRADE_NOICER)) ||
      (char_is_konatsu(ch) && guild_trade_flagged(guild_nr, TRADE_NOKONATSU)) ||
      (char_is_namek(ch) && guild_trade_flagged(guild_nr, TRADE_NONAMEK)) ||
      (char_is_mutant(ch) && guild_trade_flagged(guild_nr, TRADE_NOMUTANT)) ||
      (char_is_kanassan(ch) && guild_trade_flagged(guild_nr, TRADE_NOKANASSAN)) ||
      (char_is_android(ch) && guild_trade_flagged(guild_nr, TRADE_NOANDROID)) ||
      (char_is_bio(ch) && guild_trade_flagged(guild_nr, TRADE_NOBIO)) ||
      (char_is_demon(ch) && guild_trade_flagged(guild_nr, TRADE_NODEMON)) ||
      (char_is_majin(ch) && guild_trade_flagged(guild_nr, TRADE_NOMAJIN)) ||
      (char_is_kai(ch) && guild_trade_flagged(guild_nr, TRADE_NOKAI)) ||
      (char_is_truffle(ch) && guild_trade_flagged(guild_nr, TRADE_NOTRUFFLE)) ||
      (char_is_hoshijin(ch) && guild_trade_flagged(guild_nr, TRADE_NOGOBLIN)) ||
      (char_is_animal(ch) && guild_trade_flagged(guild_nr, TRADE_NOANIMAL)) ||
      (char_is_saiba(ch) && guild_trade_flagged(guild_nr, TRADE_NOORC)) ||
      (char_is_serpent(ch) && guild_trade_flagged(guild_nr, TRADE_NOSNAKE)) ||
      (char_is_ogre(ch) && guild_trade_flagged(guild_nr, TRADE_NOTROLL)) ||
      (char_is_halfbreed(ch) && guild_trade_flagged(guild_nr, TRADE_NOHALFBREED)) ||
      (char_is_yardratian(ch) && guild_trade_flagged(guild_nr, TRADE_NOMINOTAUR)) ||
      (char_is_arlian(ch) && guild_trade_flagged(guild_nr, TRADE_NOKOBOLD))) {
    buf = str_format("{} {}",
                     char_get_name(ch), MSG_TRAINER_DISLIKE_RACE);
    do_tell(g, keeper, buf.c_str(), g->cmd_tell, 0);
    return (false);
  }
  return (true);
}

static bool is_guild_ok(GameWorld *g, struct char_data *keeper, struct char_data *ch, struct guild_data *guild_nr)
{
  if (is_guild_open(g, keeper, guild_nr, true))
    return (is_guild_ok_char(g, keeper, ch, guild_nr));

  return (false);
}

static int does_guild_know(GameWorld *g, struct guild_data *guild_nr, int i)
{
  if (i < 0 || i >= SKILL_TABLE_SIZE)
    return (false);
  return ((int)guild_nr->skills[i]);
}

static int does_guild_know_feat(GameWorld *g, struct guild_data *guild_nr, int i)
{
  if (i < 0 || i >= NUM_FEATS_DEFINED)
    return (false);
  return ((int)guild_nr->feats[i]);
}

void sort_spells(GameWorld *g)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 0; a < SKILL_TABLE_SIZE; a++)
    g->spell_sort_info[a] = a;

  qsort_r(&g->spell_sort_info[0], SKILL_TABLE_SIZE, sizeof(int), compare_spells, g);
}

/* this and list skills should probally be combined.  perhaps in the
 * next release?  */
static void what_does_guild_know(GameWorld *g, struct guild_data *guild_nr, struct char_data *ch)
{
  std::string buf2;
  int i, sortpos, canknow, j, k, count = 0, cost = 0;

  buf2 = str_format("You have {:d} practice session{} remaining.\r\n", char_get_practice_points(g, ch), char_get_practice_points(g, ch) == 1 ? "" : "s"); // ???

  buf2 += "You can practice these skills:     @CKnown@D/@cPrac. Max    @GPS Cost@n\r\n@w-------------------------------------------------------------@n\r\n";

  /* Need to check if trainer can train doesnt do it now ??? */
  for (sortpos = 0; sortpos < SKILL_TABLE_SIZE; sortpos++) {
    i = sortpos; /* spell_sort_info[sortpos]; */
    if (does_guild_know(g, guild_nr, i) && skill_type(g, i) == SKTYPE_SKILL) {
      for (canknow = 0, k = 0, j = 0; j < NUM_CLASSES; j++)
        if (char_get_class_ranks(ch, j) > 0 && (g->spell_info[i].can_learn_skill[j] > SKLEARN_CANT)) {
          k = g->spell_info[i].can_learn_skill[j];
        }
      canknow = highest_skill_value(char_get_level(ch), k);
      count++;
      cost = calculate_skill_cost(g, ch, i);
      if (k == SKLEARN_CLASS) {
        if (char_get_skills_base(ch, i) < canknow) {
          buf2 += str_format("@y(@Y{:>2d}@y) @W{:<30} @y(@Y{:>2d}@y) @C{:d}@D/@c{:>3d}        @g{:d}{}@n\r\n", count, g->spell_info[i].name, count, char_get_skills_base(ch, i), canknow, cost, char_get_skills_base(ch, i) > 100 ? "  @D(@YGrand Master@D)@n" : "");
        } else {
          buf2 += str_format("@y(@Y{:>2d}@y) @W{:<30} @y(@Y{:>2d}@y) @C{:d}@D/@c{:>3d}        @g{:d}{}@n\r\n", count, g->spell_info[i].name, count, char_get_skills_base(ch, i), canknow, cost, char_get_skills_base(ch, i) > 100 ? "  @D(@YGrand Master@D)@n" : "");
        }
      } else {
        buf2 += str_format("@y(@Y{:>2d}@y) @W{:<30} @y(@Y{:>2d}@y) @C{:d}@D/@c{:>3d}        @g{:d}{}@n\r\n", count, g->spell_info[i].name, count, char_get_skills_base(ch, i), canknow, cost, char_get_skills_base(ch, i) > 100 ? "  @D(@YGrand Master@D)@n" : "");
      }
    }
  }

  for (sortpos = 1; sortpos <= NUM_FEATS_DEFINED; sortpos++) {
    i = g->feat_sort_info[sortpos];
    if (does_guild_know_feat(g, guild_nr, i) && feat_is_available(ch, i, 0, nullptr) && g->feat_list[i].in_game && g->feat_list[i].can_learn) {
      buf2 += str_format("@b{:<20}@n\r\n", g->feat_list[i].name);
    }
  }

  if (CONFIG_ENABLE_LANGUAGES) {
    buf2 += "\r\nand the following languages:\r\n";

    for (sortpos = 0; sortpos < SKILL_TABLE_SIZE; sortpos++) {
      i = sortpos; /* spell_sort_info[sortpos]; */
      if (does_guild_know(g, guild_nr, i) && IS_SET(skill_type(g, i), SKTYPE_LANG)) {
        //if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)]) {
        for (canknow = 0, j = 0; j < NUM_CLASSES; j++)
          if (g->spell_info[i].can_learn_skill[j] > canknow)
            canknow = g->spell_info[i].can_learn_skill[j];
        canknow = highest_skill_value(char_get_level(ch), canknow);
        if (char_get_skills_base(ch, i) < canknow) {
          buf2 += str_format("{:<20} {}\r\n", g->spell_info[i].name, char_get_skills_base(ch, i) ? "known" : "unknown");
        }
        //}
      }
    }
  }

  send_to_char(g, ch, buf2);
}

static bool prereq_pass(GameWorld *g, struct char_data *ch, int snum)
{
  if (snum == SKILL_KOUSENGAN || snum == SKILL_TSUIHIDAN || snum == SKILL_RENZO || snum == SKILL_SHOGEKIHA) {
    if (char_get_skills_base(ch, SKILL_KIBALL) < 40 || char_get_skills_base(ch, SKILL_KIBLAST) < 40 || char_get_skills_base(ch, SKILL_BEAM) < 40) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained Kiball, Kiblast, and Beam to Skill LVL 40.");
      return false;
    }
  } else if (snum == SKILL_INSTANTT) {
    if (char_get_skills_base(ch, SKILL_FOCUS) < 90 || char_get_skills_base(ch, SKILL_CONCENTRATION) < 90 || char_get_skills_base(ch, SKILL_ZANZOKEN) < 90) {
      send_to_char(g, ch, "You can not train instant transmission until you have Focus, Concentration, and Zanzoken up to Skill LVL 90.");
      return false;
    }
  } else if (snum == SKILL_SLAM) {
    if (char_get_skills_base(ch, SKILL_UPPERCUT) < 50) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained uppercut to Skill LVL 50.");
      return false;
    }
  } else if (snum == SKILL_UPPERCUT) {
    if (char_get_skills_base(ch, SKILL_ELBOW) < 40) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained elbow to Skill LVL 40.");
      return false;
    }
  } else if (snum == SKILL_HEELDROP) {
    if (char_get_skills_base(ch, SKILL_ROUNDHOUSE) < 50) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained roundhouse to Skill LVL 50.");
      return false;
    }
  } else if (snum == SKILL_ROUNDHOUSE) {
    if (char_get_skills_base(ch, SKILL_KNEE) < 40) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained knee to Skill LVL 40.");
      return false;
    }
  } else if (snum == SKILL_KIBALL || snum == SKILL_KIBLAST || snum == SKILL_BEAM) {
    if (char_get_skills_base(ch, SKILL_FOCUS) < 30) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained focus to Skill LVL 30.");
      return false;
    }
  } else if (IS_SET(g->spell_info[snum].flags, SKFLAG_TIER2) || IS_SET(g->spell_info[snum].flags, SKFLAG_TIER3)) {
    if (snum != 530 && snum != 531) {
      if (char_get_skills_base(ch, SKILL_TSUIHIDAN) < 40 || char_get_skills_base(ch, SKILL_RENZO) < 40 || char_get_skills_base(ch, SKILL_SHOGEKIHA) < 40) {
        send_to_char(g, ch, "You can not train that skill until you at least have trained Tsuihidan, Renzokou Energy Dan, and Shogekiha to Skill LVL 40.");
        return false;
      }
    }
  } else if (IS_SET(g->spell_info[snum].flags, SKFLAG_TIER4)) {
    if (char_is_roshi(ch) && (char_get_skills_base(ch, SKILL_KAMEHAMEHA) < 40 || char_get_skills_base(ch, SKILL_KIENZAN) < 40)) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained Kamehameha and Kienzan to Skill LVL 40.");
      return false;
    }
    if (char_is_tsuna(ch) && (char_get_skills_base(ch, SKILL_WRAZOR) < 40 || char_get_skills_base(ch, SKILL_WSPIKE) < 40)) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained Water Razor and Water Spikes to Skill LVL 40.");
      return false;
    }
    if (char_is_piccolo(ch) && (char_get_skills_base(ch, SKILL_MASENKO) < 40 || char_get_skills_base(ch, SKILL_SBC) < 40)) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained Masenko and Special Beam Cannon to Skill LVL 40.");
      return false;
    }
    if (char_is_frieza(ch) && (char_get_skills_base(ch, SKILL_DEATHBEAM) < 40 || char_get_skills_base(ch, SKILL_KIENZAN) < 40)) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained Deathbeam and Kienzan to Skill LVL 40.");
      return false;
    }
    if (char_is_ginyu(ch) && (char_get_skills_base(ch, SKILL_CRUSHER) < 40 || char_get_skills_base(ch, SKILL_ERASER) < 40)) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained Crusher Ball and Eraser Cannon to Skill LVL 40.");
      return false;
    }
    if (char_is_bardock(ch) && (char_get_skills_base(ch, SKILL_GALIKGUN) < 40 || char_get_skills_base(ch, SKILL_FINALFLASH) < 40)) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained Galik Gun and Final Flash to Skill LVL 40.");
      return false;
    }
    if (char_is_tapion(ch) && (char_get_skills_base(ch, SKILL_TSLASH) < 40 || char_get_skills_base(ch, SKILL_DDSLASH) < 40)) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained Twin Slash and Darkness Dragon Slash to Skill LVL 40.");
      return false;
    }
    if (char_is_nail(ch) && (char_get_skills_base(ch, SKILL_MASENKO) < 40 || char_get_skills_base(ch, SKILL_KOUSENGAN) < 40)) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained Masenko and Kousengan to Skill LVL 40.");
      return false;
    }
    if (char_is_android(ch) && (char_get_skills_base(ch, SKILL_DUALBEAM) < 40 || char_get_skills_base(ch, SKILL_HELLFLASH) < 40)) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained Dual Beam and Hell Flash to Skill LVL 40.");
      return false;
    }
    if (char_is_jinto(ch) && char_get_skills_base(ch, SKILL_BREAKER) < 40) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained Star Breaker to Skill LVL 40.");
      return false;
    }
  } else if (IS_SET(g->spell_info[snum].flags, SKFLAG_TIER5)) {
    if (char_get_skills_base(ch, SKILL_FOCUS) < 60 || char_get_skills_base(ch, SKILL_CONCENTRATION) < 80) {
      send_to_char(g, ch, "You can not train that skill until you at least have trained focus to Skill LVL 60 and concentration to Skill LVL 80.");
      return false;
    }
  }
  return true;
}

static void handle_forget(GameWorld *g, struct char_data *keeper, struct guild_data *guild_nr, struct char_data *ch, std::string_view argument)
{

  int skill_num;

  std::string text(skip_spaces(argument));

  if (text.empty()) {
    send_to_char(g, ch, "What skill do you want to start to forget?\r\n");
    return;
  }

  skill_num = find_skill_num(g, text.data(), SKTYPE_SKILL);

  if (skill_num < 0 || skill_num >= SKILL_TABLE_SIZE) {
    send_to_char(g, ch, "@MThat is not a valid skill!@n\r\n");
    return;
  }

  if (char_get_skills_base(ch, skill_num) > 30) {
    send_to_char(g, ch, "@MYou can not forget that skill, you know too much about it.@n\r\n");
    return;
  }
  if (skill_num == SKILL_MIMIC && char_get_mimic(ch) > 0) {
    send_to_char(g, ch, "@MYou can not forget mimic while you are using it!\r\n");
  } else if (skill_num == SKILL_FOCUS) {
    send_to_char(g, ch, "@MYou can not forget such a fundamental skill!@n\r\n");
  } else if (char_get_skills_base(ch, skill_num) <= 0) {
    send_to_char(g, ch, "@MYou can not forget a skill you don't know!@n\r\n");
  } else if (char_get_forgeting(ch) == skill_num) {
    send_to_char(g, ch, "@MYou stop forgetting {}@n\r\n", g->spell_info[skill_num].name);
    char_set_forgetcount(ch, 0);
    char_set_forgeting(ch, 0);
  } else if (char_get_forgeting(ch) != 0) {
    send_to_char(g, ch, "@MYou stop forgetting {}, and start trying to forget {}.@n\r\n", g->spell_info[char_get_forgeting(ch)].name, g->spell_info[skill_num].name);
    char_set_forgetcount(ch, 0);
    char_set_forgeting(ch, skill_num);
  } else {
    send_to_char(g, ch, "@MYou start trying to forget {}.@n\r\n", g->spell_info[skill_num].name);
    char_set_forgetcount(ch, 0);
    char_set_forgeting(ch, skill_num);
  }
}

static void handle_grand(GameWorld *g, struct char_data *keeper, struct guild_data *guild_nr, struct char_data *ch, std::string_view argument)
{

  int skill_num;

  std::string text(skip_spaces(argument));

  if (!char_can_grand_master(ch)) {
    send_to_char(g, ch, "Your race can not become a Grand Master in a skill through this process.\r\n");
    return;
  }

  if (text.empty()) {
    send_to_char(g, ch, "What skill do you want to become a Grand Master in?");
    return;
  }

  skill_num = find_skill_num(g, text.data(), SKTYPE_SKILL);
  if (skill_num < 0 || skill_num >= SKILL_TABLE_SIZE) {
    send_to_char(g, ch, "@MThat is not a valid skill!@n\r\n");
    return;
  }
  std::string buf;

  if (!(does_guild_know(g, guild_nr, skill_num))) {
    buf = npc_message(guild_nr->no_such_skill, char_get_name(ch));
    do_tell(g, keeper, buf.c_str(), g->cmd_tell, 0);
    return;
  }

  if (char_get_skills_base(ch, skill_num) <= 0) {
    send_to_char(g, ch, "You do not know that skill!\r\n");
    return;
  }
  if (char_get_skills_base(ch, skill_num) < 100) {
    send_to_char(g, ch, "You haven't even mastered that skill. How can you become a Grand Master in it?\r\n");
    return;
  }
  if (char_get_skills_base(ch, skill_num) >= 103) {
    send_to_char(g, ch, "You have already become a Grand Master in that skill and have progessed as far as possible in it.\r\n");
    return;
  }
  if (char_get_practice_points(g, ch) < 1000) {
    send_to_char(g, ch, "You need at least 1,000 practice sessions to rank up beyond 100 in a skill.\r\n");
    return;
  }
  if (char_get_skills_base(ch, skill_num) == 100) {
    send_to_char(g, ch, "@YYou have ascended to Grand Master in the skill, @C{}@Y.\r\n", g->spell_info[skill_num].name);
  } else {
    send_to_char(g, ch, "@YYou have ranked up in your Grand Mastery of the skill, @C{}@Y.\r\n", g->spell_info[skill_num].name);
  }
  char_set_skills_base(ch, skill_num, char_get_skills_base(ch, skill_num) + 1);
  char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (1000));
}

static void handle_practice(GameWorld *g, struct char_data *keeper, struct guild_data *guild_nr, struct char_data *ch, std::string_view argument)
{
  //int percent = GET_SKILL(ch, skill);
  int skill_num, learntype, pointcost, highest, i;
  std::string buf;

  std::string text(skip_spaces(argument));

  if (text.empty()) {
    what_does_guild_know(g, guild_nr, ch);
    return;
  }

  if (char_get_practice_points(g, ch) <= 0) {
    send_to_char(g, ch, "You do not seem to be able to practice now.\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_SHOCKED)) {
    send_to_char(g, ch, "You can not practice while your mind is shocked!\r\n");
    return;
  }

  skill_num = find_skill_num(g, text.data(), SKTYPE_SKILL);
  if (skill_num < 0 || skill_num >= SKILL_TABLE_SIZE) {
    send_to_char(g, ch, "@MThat is not a valid skill!@n\r\n");
    return;
  }

  if (strstr(sensei_style[char_get_class(ch)], text.c_str())) {
    skill_num = 539;
  }

  if (skill_num == char_get_forgeting(ch)) {
    send_to_char(g, ch, "You can't practice that! You are trying to forget it!@n\r\n");
    return;
  }

  /****  Does the GM know the skill the player wants to learn?  ****/
  if (!(does_guild_know(g, guild_nr, skill_num))) {
    buf = npc_message(guild_nr->no_such_skill, char_get_name(ch));
    do_tell(g, keeper, buf.c_str(), g->cmd_tell, 0);
    return;
  }

  /**** Can the player learn the skill if the GM knows it?  ****/
  if (IS_SET(g->spell_info[skill_num].skilltype, SKTYPE_SKILL)) {
    for (learntype = 0, i = 0; i < NUM_CLASSES; i++)
      if (g->spell_info[skill_num].can_learn_skill[i] > learntype)
        learntype = g->spell_info[skill_num].can_learn_skill[i];
    switch (learntype) {
      case SKLEARN_CANT:
        buf = npc_message(guild_nr->no_such_skill, char_get_name(ch));
        do_tell(g, keeper, buf.c_str(), g->cmd_tell, 0);
        return;
      case SKLEARN_CROSSCLASS:
      case SKLEARN_CLASS:
        highest = highest_skill_value(char_get_level(ch), learntype);
        break;
      default:
        mud_log("Unknown SKLEARN type for skill {:d} in practice", skill_num);
        send_to_char(g, ch, "You can't learn that.\r\n");
        return;
    }
    pointcost = calculate_skill_cost(g, ch, skill_num);
    if (char_get_practice_points(g, ch) >= pointcost) {
      if (!prereq_pass(g, ch, skill_num)) {
        return;
      }
      if (char_get_skills_base(ch, skill_num) >= highest) {
        send_to_char(g, ch, "You cannot increase that skill again until you progress further.\r\n");
        return;
      }
      if (char_get_skills_base(ch, skill_num) >= 75 && char_get_bonuses(ch, BONUS_MASOCHISTIC) > 0) {
        if (skill_num == SKILL_PARRY || skill_num == SKILL_ZANZOKEN || skill_num == SKILL_DODGE || skill_num == SKILL_BARRIER || skill_num == SKILL_BLOCK || skill_num == SKILL_TSKIN) {
          send_to_char(g, ch, "You cannot increase that skill again because it would deny you the pain you enjoy.\r\n");
          return;
        }
      }
      if (char_get_skills_base(ch, skill_num) >= 75 && char_is_tapion(ch) && skill_num == SKILL_SENSE) {
        send_to_char(g, ch, "You cannot practice that anymore.\r\n");
        return;
      }
      if (char_get_skills_base(ch, skill_num) >= 75 && char_is_dabura(ch) && skill_num == SKILL_SENSE) {
        send_to_char(g, ch, "You cannot practice that anymore.\r\n");
        return;
      }
      if (char_get_skills_base(ch, skill_num) >= 75 && char_is_jinto(ch) && skill_num == SKILL_SENSE) {
        send_to_char(g, ch, "You cannot practice that anymore.\r\n");
        return;
      }
      if (char_get_skills_base(ch, skill_num) >= 75 && char_is_tsuna(ch) && skill_num == SKILL_SENSE) {
        send_to_char(g, ch, "You cannot practice that anymore.\r\n");
        return;
      }
      if (char_get_skills_base(ch, skill_num) >= 50 && char_is_frieza(ch) && skill_num == SKILL_SENSE) {
        send_to_char(g, ch, "You cannot practice that anymore.\r\n");
        return;
      }
      if (char_get_skills_base(ch, skill_num) >= 50 && char_is_andsix(ch) && skill_num == SKILL_SENSE) {
        send_to_char(g, ch, "You cannot practice that anymore.\r\n");
        return;
      }
      if (char_get_skills_base(ch, skill_num) >= 50 && char_is_kurzak(ch) && skill_num == SKILL_SENSE) {
        send_to_char(g, ch, "You cannot practice that anymore.\r\n");
        return;
      }
      if (char_get_skills_base(ch, skill_num) >= 50 && char_is_ginyu(ch) && skill_num == SKILL_SENSE) {
        send_to_char(g, ch, "You cannot practice that anymore.\r\n");
        return;
      }
      if (char_get_skills_base(ch, skill_num) >= 50 && char_is_bardock(ch) && skill_num == SKILL_SENSE) {
        send_to_char(g, ch, "You cannot practice that anymore.\r\n");
        return;
      }
      if (char_get_skills_base(ch, skill_num) >= 100) {
        send_to_char(g, ch, "You know everything about that skill.\r\n");
        return;
      }
      if (char_get_skills_base(ch, skill_num) == 0) {
        if (slot_count(ch) < char_get_skill_slots(ch)) {
          if (skill_num != 539)
            send_to_char(g, ch, "You practice and master the basics!\r\n");
          else
            send_to_char(g, ch, "You practice the basics of {}\r\n", sensei_style[char_get_class(ch)]);
          char_set_skills_base(ch, skill_num, char_get_skills_base(ch, skill_num) + rand_number(g, 10, 25));
          char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (pointcost));
          if (char_get_forgeting(ch) != 0 && char_get_skills_base(ch, char_get_forgeting(ch)) < 30) {
            char_set_forgetcount(ch, char_get_forgetcount(ch) + (1));
            if (char_get_forgetcount(ch) >= 5) {
              char_set_skills_base(ch, char_get_forgeting(ch), 0);
              send_to_char(g, ch, "@MYou have finally forgotten what little you knew of {}@n\r\n", g->spell_info[char_get_forgeting(ch)].name);
              char_set_forgeting(ch, 0);
              char_set_forgetcount(ch, 0);
              save_char(g, ch);
            }
          } else if (char_get_skills_base(ch, char_get_forgeting(ch)) < 30) {
            char_set_forgeting(ch, 0);
          }
        } else {
          send_to_char(g, ch, "You already know the maximum number of skills you can for the time being!\r\n");
          return;
        }
      } else {
        if (skill_num != 539)
          send_to_char(g, ch, "You practice for a while and manage to advance your technique. ({:d})\r\n", char_get_skills_base(ch, skill_num) + 1);
        else
          send_to_char(g, ch, "You practice the basics of {}. ({:d})\r\n", sensei_style[char_get_class(ch)], char_get_skills_base(ch, skill_num) + 1);
        char_set_skills_base(ch, skill_num, char_get_skills_base(ch, skill_num) + 1);
        char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (pointcost));
        if (char_get_skills_base(ch, skill_num) >= 100) {
          send_to_char(g, ch, "You learned a lot by mastering that skill.\r\n");
          if (char_is_konatsu(ch) && skill_num == SKILL_PARRY) {
            char_set_skills_base(ch, skill_num, char_get_skills_base(ch, skill_num) + 5);
          }
          gain_exp(g, ch, level_exp(ch, char_get_level(ch) + 1) / 20);
        }
        if (char_get_forgeting(ch) != 0) {
          char_set_forgetcount(ch, char_get_forgetcount(ch) + (1));
          if (char_get_forgetcount(ch) >= 5) {
            char_set_skills_base(ch, char_get_forgeting(ch), 0);
            send_to_char(g, ch, "@MYou have finally forgotten what little you knew of {}@n\r\n", g->spell_info[char_get_forgeting(ch)].name);
            char_set_forgeting(ch, 0);
            char_set_forgetcount(ch, 0);
            save_char(g, ch);
          }
        }
      }
    } else {
      send_to_char(g, ch, "You need {:d} practice session{} to increase that skill.\r\n",
                   pointcost, (pointcost == 1) ? "" : "s");
    }
  } else {
    buf = npc_message(guild_nr->no_such_skill, char_get_name(ch));
    do_tell(g, keeper, buf.c_str(), g->cmd_tell, 0);
  }
}

static void handle_gain(GameWorld *g, struct char_data *keeper, struct guild_data *guild_nr, struct char_data *ch, std::string_view argument)
{
  int whichclass = char_get_class(ch);

  int rpp_cost = rpp_to_level(ch);

  if (char_get_level(ch) < 100 && char_get_exp(ch) >= level_exp(ch, char_get_level(ch) + 1)) {
    if (char_get_rp(ch) < rpp_cost) {
      send_to_char(g, ch, "You need at least {:d} RPP to gain the next level.\r\n", rpp_cost);
    } else if (rpp_cost <= char_get_rp(ch)) {
      char_set_rp(ch, char_get_rp(ch) - (rpp_cost));
      char_get_desc(ch)->rpp = char_get_rp(ch);
      userWrite(g, char_get_desc(ch), 0, 0, 0, "index");
      send_to_char(g, ch, "@D(@cRPP@W: @w-{:d}@D)@n\n\n", rpp_cost);
      gain_level(g, ch, whichclass);
    } else {
      gain_level(g, ch, whichclass);
    }
  } else {
    send_to_char(g, ch, "You are not yet ready for further advancement.\r\n");
  }
}

int rpp_to_level(struct char_data *ch)
{

  switch (char_get_level(ch)) {
    case 2:
      // charge the RPP races to level for the first time.
      return race_rpp_cost(char_get_race(ch));
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
      return 3;
    case 96:
    case 97:
    case 98:
      return 4;
    case 99:
      return 5;
    default:
      return 0;
  }
}

static void handle_study(GameWorld *g, struct char_data *keeper, struct guild_data *guild_nr, struct char_data *ch, std::string_view argument)
{

  int expcost = 25000, goldcost = 750, fail = false, reward = 25, goldadjust = 0, expadjust = 0;

  if (char_get_level(ch) >= 100) {
    goldadjust = 500;
    //expadjust = 15000;
    expcost = 0;
  } else if (char_get_level(ch) >= 91) {
    goldadjust = 450;
    expadjust  = 12500;
  } else if (char_get_level(ch) >= 81) {
    goldadjust = 400;
    expadjust  = 10000;
  } else if (char_get_level(ch) >= 71) {
    goldadjust = 350;
    expadjust  = 7500;
  } else if (char_get_level(ch) >= 61) {
    goldadjust = 300;
    expadjust  = 5000;
  } else if (char_get_level(ch) >= 51) {
    goldadjust = 250;
    expadjust  = 2500;
  } else if (char_get_level(ch) >= 41) {
    goldadjust = 200;
  } else if (char_get_level(ch) >= 31) {
    goldadjust = 150;
  } else if (char_get_level(ch) >= 21) {
    goldadjust = 100;
  } else if (char_get_level(ch) >= 11) {
    goldadjust = 50;
  }

  goldcost += goldadjust;
  expcost += expadjust;

  if (char_get_exp(ch) < expcost) {
    send_to_char(g, ch, "You do not have enough experience to study. @D[@wCost@W: @G{}@D]@n\r\n", add_commas(expcost));
    fail = true;
  }

  if (char_get_gold(ch) < goldcost) {
    send_to_char(g, ch, "You do not have enough zenni to study. @D[@wCost@W: @Y{}@D]@n\r\n", add_commas(goldcost));
    fail = true;
  }

  if (fail == true)
    return;

  char_set_exp(ch, char_get_exp(ch) - (expcost));
  char_set_gold(ch, char_get_gold(ch) - (goldcost));
  char_set_practice_points(g, ch, char_get_practice_points(g, ch) + (25));

  act(g, "@c$N@W spends time lecturing you on various subjects.@n", true, ch, nullptr, keeper, TO_CHAR);
  act(g, "@c$N@W spends time lecturing @C$n@W on various subjects.@n", true, ch, nullptr, keeper, TO_ROOM);
  send_to_char(g, ch, "@wYou have gained {:d} practice sessions in exchange for {} EXP and {} zenni.\r\n", reward, add_commas(expcost), add_commas(goldcost));
}

SPECIAL(guild)
{
  char arg[MAX_INPUT_LENGTH];
  struct guild_data *guild_nr;
  int i;
  struct char_data *keeper = (struct char_data *)me;
  struct {
    const char *cmd{};
    void (*func)(GameWorld *, struct char_data *, struct guild_data *, struct char_data *, std::string_view){};
  } guild_cmd_tab[] = {
      {"practice", handle_practice},
      {"gain",     handle_gain    },
      {"forget",   handle_forget  },
      {"study",    handle_study   },
      {"grand",    handle_grand   },
      {nullptr,    nullptr        }
  };

  guild_nr = find_first(g->guilds.all(), [&](guild_data *gd) { return guild_get_gm(gd) == char_get_nr(keeper); });
  if (!guild_nr)
    return (false);

  if (guild_get_func(guild_nr))
    if ((guild_get_func(guild_nr))(g, ch, me, cmd, arg))
      return (true);

  /*** Is the GM able to train?    ****/
  if (!char_is_awake(keeper))
    return (false);

  for (i = 0; guild_cmd_tab[i].cmd; i++)
    if (CMD_IS(g, guild_cmd_tab[i].cmd))
      break;

  if (!guild_cmd_tab[i].cmd)
    return (false);

  if (!(is_guild_ok(g, keeper, ch, guild_nr)))
    return (true);

  (guild_cmd_tab[i].func)(g, keeper, guild_nr, ch, argument);

  return (true);
}

/**** This function is here just because I'm extremely paranoid.  Take
      it out if you aren't ;)  ****/
static void clear_skills(GameWorld *g, struct guild_data *gdindex)
{
  int i;

  for (i = 0; i < SKILL_TABLE_SIZE; i++)
    gdindex->skills[i] = 0;
}

/****  This is ripped off of read_line from shop.c.  They could be
 *  combined. But why? ****/

static void read_guild_line(GameWorld *g, guild_vnum vnum, FILE *gm_f, const char *string, void *data, const char *type)
{
  char buf[MAX_STRING_LENGTH];

  if (!get_line(gm_f, buf) || !sscanf(buf, string, data)) {
    file_print(stderr, "Error in guild #{:d}, Could not get {}\n", vnum, type);
    exit(1);
  }
}

/* An empty or malformed message would reach npc_message() as an unsafe format, so use the default. */
static std::string read_guild_message(GameWorld *g, guild_vnum vnum, FILE *gm_f, const char *why, const char *field,
                                      const char *fallback)
{
  const char *problem;
  std::string msg = take_string(fread_string(gm_f, why));

  if (msg.empty())
    return fallback;

  if ((problem = npc_msg_format_problem(msg.c_str(), false))) {
    mud_log("SYSERR: Guild #{:d} {}: {}; using the default.", vnum, field, problem);
    return fallback;
  }
  return msg;
}

/* The next line of a guild's list of skills, or the end of the game's boot if the file stops there. */
static void read_guild_skill_line(FILE *gm_f, char *buf, int *t1, int *t2, int *rv, int guild)
{
  if (!get_line(gm_f, buf)) {
    mud_log("SYSERR: Guild file ends in the middle of guild #{:d}", guild);
    exit(1);
  }
  *t1 = 0;
  *rv = sscanf(buf, "%d %d", t1, t2);
}

void boot_the_guilds(GameWorld *g, FILE *gm_f, const char *filename, int rec_count)
{
  char *buf;
  std::string buf2;
  char *p, buf3[READ_SIZE];
  room_vnum temp;
  int val, t1, t2, rv;
  int done = false;

  buf2 = str_format("beginning of GM file {}", filename);

  buf = fread_string(gm_f, buf2.c_str());
  while (!done && buf) {
    if (*buf == '#') { /* New Trainer */
      sscanf(buf, "#%d\n", &temp);
      buf2 = str_format("GM #{:d} in GM file {}", temp, filename);
      free(buf); /* Plug memory leak! */
      guild_vnum vnum                  = temp;
      std::unique_ptr<guild_data> made = std::make_unique<guild_data>();
      struct guild_data *guild         = made.get();
      guild_set_vnum(guild, temp);

      clear_skills(g, guild);
      read_guild_skill_line(gm_f, buf3, &t1, &t2, &rv, temp);
      while (t1 > -1) {
        if (rv == 1) { /* old style guilds, only skills */
          if (t1 < SKILL_TABLE_SIZE)
            guild->skills[(int)t1] = 1;
          else
            mud_log("SYSERR: Guild #{:d} teaches impossible skill {:d}", temp, t1);
        } else if (rv == 2) { /* new style guilds, skills and feats */
          if (t2 == 1) {
            if (t1 < SKILL_TABLE_SIZE)
              guild->skills[(int)t1] = 1;
            else
              mud_log("SYSERR: Guild #{:d} teaches impossible skill {:d}", temp, t1);
          } else if (t2 == 2) {
            if (t1 < NUM_FEATS_DEFINED)
              guild->feats[(int)t1] = 1;
            else
              mud_log("SYSERR: Guild #{:d} teaches impossible feat {:d}", temp, t1);
          } else {
            mud_log("SYSERR: Invalid 2nd arg in guild file!");
            exit(1);
          }
        } else {
          mud_log("SYSERR: Invalid format in guild file. Expecting 2 args but got {:d}!", rv);
          exit(1);
        }
        read_guild_skill_line(gm_f, buf3, &t1, &t2, &rv, temp);
      }
      read_guild_line(g, vnum, gm_f, "%f", &guild->charge, "GM_CHARGE");
      guild->no_such_skill   = read_guild_message(g, vnum, gm_f, buf2.c_str(), "no_such_skill", guild_default_no_skill);
      guild->not_enough_gold = read_guild_message(g, vnum, gm_f, buf2.c_str(), "not_enough_gold", guild_default_no_gold);

      read_guild_line(g, vnum, gm_f, "%d", &guild->minlvl, "GM_MINLVL");
      read_guild_line(g, vnum, gm_f, "%d", &guild->gm, "GM_TRAINER");

      if (!mob_proto_of(g, guild_get_gm(guild)))
        guild_set_gm(guild, NOBODY);
      read_guild_line(g, vnum, gm_f, "%d", &guild->with_who[0], "GM_WITH_WHO");

      read_guild_line(g, vnum, gm_f, "%d", &guild->open, "GM_OPEN");
      read_guild_line(g, vnum, gm_f, "%d", &guild->close, "GM_CLOSE");

      guild_set_func(guild, nullptr);
      buf = str_alloc(READ_SIZE);
      get_line(gm_f, buf);
      if (buf && *buf != '#' && *buf != '$') {
        p = buf;
        for (temp = 1; temp < GW_ARRAY_MAX; temp++) {
          if (!p || !*p)
            break;
          if (sscanf(p, "%d", &val) != 1) {
            mud_log("SYSERR: Can't parse GM_WITH_WHO line in {}: '{}'", buf2, buf);
            break;
          }
          guild_get_with_who(guild)
              [temp] = val;
          while (isdigit(*p) || *p == '-') {
            p++;
          }
          while (*p && !(isdigit(*p) || *p == '-')) {
            p++;
          }
        }
        while (temp < GW_ARRAY_MAX)
          guild_get_with_who(guild)
              [temp++] = 0;
        free(buf);
        buf = fread_string(gm_f, buf2.c_str());
      }
      if (g->guilds.find(vnum))
        mud_log("SYSERR: Guild #{:d} is defined twice; ignoring the second one.", vnum);
      else
        g->guilds.insert(vnum, std::move(made));
    } else {
      if (*buf == '$') /* EOF */
        done = true;
      free(buf); /* Plug memory leak! */
      buf = done ? nullptr : fread_string(gm_f, buf2.c_str());
    }
  }
}

void assign_the_guilds(GameWorld *g)
{

  g->cmd_say  = find_command(g, "say");
  g->cmd_tell = find_command(g, "tell");

  for (struct guild_data *gdindex : g->guilds.all()) {
    if (guild_get_gm(gdindex) == NOBODY)
      continue;

    if (mob_index_of(g, guild_get_gm(gdindex))->func && mob_index_of(g, guild_get_gm(gdindex))->func != guild)
      guild_set_func(gdindex, mob_index_of(g, guild_get_gm(gdindex))->func);

    mob_index_of(g, guild_get_gm(gdindex))->func = guild;
  }
}

static std::string guild_customer_string(GameWorld *g, struct guild_data *guild_nr, int detailed)
{
  std::string buf;

  for (int gindex = 0, flag = 0; *trade_letters[gindex] != '\n'; gindex++, flag++)
    if (detailed) {
      if (!guild_trade_flagged(guild_nr, flag))
        buf += str_format(", {}", trade_letters[gindex]);
    } else
      buf += guild_trade_flagged(guild_nr, flag) ? '_' : *trade_letters[gindex];
  return buf;
}

static void list_all_guilds(GameWorld *g, struct char_data *ch)
{
  const char *list_all_guilds_header =
      "Virtual   G.Master	Charge   Members\r\n"
      "----------------------------------------------------------------------\r\n";
  std::string buf = list_all_guilds_header;
  std::string buf1;

  for (struct guild_data *gm_nr : g->guilds.all()) {
    if (guild_get_gm(gm_nr) == NOBODY)
      buf1 = "<NONE>";
    else
      buf1 = str_format("{:>6d}", mob_index_of(g, guild_get_gm(gm_nr))->vnum);

    buf += str_format("{:>6d}\t{}\t\t{:>5.2f}\t{}\r\n",
                      guild_get_vnum(gm_nr), buf1, guild_get_charge(gm_nr), guild_customer_string(g, gm_nr, false));
  }

  send_to_char(g, ch, buf);
}

static void list_detailed_guild(GameWorld *g, struct char_data *ch, struct guild_data *gm_nr)
{
  int i;
  std::string buf;
  std::string buf1;
  std::string buf2;

  if (guild_get_gm(gm_nr) < NOBODY)
    buf1 = "<NONE>";
  else
    buf1 = str_format("{:>6d}   ", mob_index_of(g, guild_get_gm(gm_nr))->vnum);

  buf = str_format(" Guild Master: {}\r\n", buf1);
  buf += str_format(" Hours: {:>4d} to {:>4d},  Surcharge: {:>5.2f}\r\n",
                    guild_get_open(gm_nr), guild_get_close(gm_nr), guild_get_charge(gm_nr));
  buf += str_format(" Min Level will train: {:d}\r\n", guild_get_minlvl(gm_nr));
  buf += str_format(" Whom will train: {}\r\n", guild_customer_string(g, gm_nr, true));

  /* now for the REAL reason why someone would want to see a Guild :) */

  buf += " The GM can teach the following:\r\n";

  buf2.clear();
  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    if (does_guild_know(g, gm_nr, i))
      buf2 += str_format(" {} \r\n", g->spell_info[i].name);
  }

  buf += str_format("{}", buf2);

  send_to_char(g, ch, buf);
}

void show_guild(GameWorld *g, struct char_data *ch, char *arg)
{
  struct guild_data *gm_nr;
  room_vnum gm_num;

  if (!*arg)
    list_all_guilds(g, ch);
  else {
    if (is_number(arg))
      gm_num = atoi(arg);
    else
      gm_num = -1;

    if (gm_num > 0) {
      gm_nr = g->guilds.find(gm_num);
      if (!gm_nr) {
        send_to_char(g, ch, "Illegal guild master number.\n\r");
        return;
      }
      list_detailed_guild(g, ch, gm_nr);
    }
  }
}

/*
 * List all guilds in a zone.                              
 */
void list_guilds(GameWorld *g, struct char_data *ch, struct zone_data *rnum, guild_vnum vmin, guild_vnum vmax)
{
  IDXTYPE bottom, top;
  int counter = 0;

  if (rnum != nullptr) {
    bottom = rnum->bot;
    top    = rnum->top;
  } else {
    bottom = vmin;
    top    = vmax;
  }

  /****************************************************************************/
  /** Store the header for the guild listing.                                **/
  /****************************************************************************/
  send_to_char(g, ch,
               "Index VNum    Guild Master\r\n----- ------- ---------------------------------------------\r\n");

  if (g->guilds.empty())
    return;

  for (struct guild_data *i : g->guilds.all()) {
    if (guild_get_vnum(i) >= bottom && guild_get_vnum(i) <= top) {
      counter++;

      send_to_char(g, ch, "@g{:>4d}@n) [@c{:<5d}@n]", counter, guild_get_vnum(i));

      /************************************************************************/
      /** Retrieve the list of rooms for this guild.                         **/
      /************************************************************************/

      send_to_char(g, ch, " @c[@y{:d}@c]@y {}@n",
                   (guild_get_gm(i) == NOBODY) ? -1 : mob_index_of(g, guild_get_gm(i))->vnum,
                   (guild_get_gm(i) == NOBODY) ? "" : char_get_sdesc(mob_proto_of(g, guild_get_gm(i))));

      send_to_char(g, ch, "\r\n");
    }
  }

  if (counter == 0)
    send_to_char(g, ch, "None found.\r\n");
}

void destroy_guilds(GameWorld *g)
{
  g->guilds.clear();
}

int count_guilds(GameWorld *g, guild_vnum low, guild_vnum high)
{
  int j = 0;

  for (struct guild_data *guild : g->guilds.all())
    if (guild_get_vnum(guild) >= low && guild_get_vnum(guild) <= high)
      j++;
  return j;
}

void levelup_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
}

static bool guild_trade_flagged(const struct guild_data *rec, int flag)
{
  return IS_SET_AR(rec->with_who, flag) != 0;
}

bool guild_trade_toggle(struct guild_data *rec, int flag)
{
  return (TOGGLE_BIT_AR(rec->with_who, flag) & Q_BIT(flag)) != 0;
}
