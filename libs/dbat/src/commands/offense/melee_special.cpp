/* melee_special.c: special melee attacks
 * Split from act.offensive.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/args.hpp"
#include "dbat/act.offensive.hpp"
#include "dbat/consts/affflags.hpp"
#include "dbat/consts/bonus.hpp"
#include "dbat/consts/mobflags.hpp"
#include "dbat/consts/skills.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/random.hpp"
#include "dbat/search.hpp"
#include "dbat/combat.hpp"
#include "dbat/comm.hpp"
#include "dbat/fight.hpp"
#include "dbat/spells.hpp"
#include "dbat/techniques.hpp"
#include "dbat/game_world.hpp"
#include "ki_attack.hpp"
#include "strike.hpp"

namespace {

/* Piccolo's students with style heeldrop sooner */
int heeldrop_cooldown(strike_shot &s)
{
  return char_is_piccolo(s.ch) && char_get_skills_base(s.ch, SKILL_STYLE) >= 75 ? 7 : 9;
}

/* A heeldrop is avoided as its victim's skills say, or by a mob by the attacker's level, rather than as
 * handle_defense() says */
void heeldrop_defense(strike_shot &s, int &pry, int &blk, int &dge)
{
  /* the least and most a mob rolls, by the attacker's level. quirk: there was a second, unreachable, entry for 40 */
  static constexpr struct {
    int level, lo, hi;
  } mobs[] = {
      {10,  20, 40 },
      {20,  20, 60 },
      {30,  20, 70 },
      {40,  30, 70 },
      {60,  40, 80 },
      {70,  50, 80 },
      {80,  50, 90 },
      {90,  60, 90 },
      {100, 70, 100}
  };

  pry = blk = dge = 0;
  if (!char_is_npc(s.vict)) {
    pry = handle_parry(s.g, s.vict);
    blk = char_get_skill(s.vict, SKILL_BLOCK);
    dge = char_get_skill(s.vict, SKILL_DODGE);
    return;
  }
  for (const auto &m : mobs) {
    if (char_get_level(s.ch) > m.level)
      continue;
    if (char_is_humanoid(s.vict)) {
      pry = rand_number(s.g, m.lo, m.hi);
      blk = rand_number(s.g, m.lo, m.hi);
    }
    dge = rand_number(s.g, m.lo, m.hi);
    return;
  }
}

/* A heeldrop to the body or the head can send the victim crashing down */
void heeldrop_crashdown(strike_shot &s, int where)
{
  if (where == 1 || where == 2)
    tech_handle_crashdown(s.g, s.ch, s.vict);
}

/* Bardock's students slam sooner */
int slam_cooldown(strike_shot &s)
{
  return char_is_bardock(s.ch) && char_get_skills_base(s.ch, SKILL_STYLE) >= 75 ? 7 : 9;
}

/* What the room shows of an impact: a null is nothing */
struct scenery {
  const char *to_char{};
  const char *to_room{}; /* if not to_char */
};

void show(const strike_shot &s, const scenery &sc)
{
  act(s.g, sc.to_char, true, s.ch, nullptr, s.vict, TO_CHAR);
  act(s.g, sc.to_room ? sc.to_room : sc.to_char, true, s.ch, nullptr, s.vict, TO_ROOM);
}

/* A slam to the head throws the victim down, which can knock them out, and into the ground, which craters it */
void slam_crater(strike_shot &s, int where)
{
  static constexpr scenery outdoors[8]   = {{"Debris is thrown into the air and showers down thunderously!"},
                                            {"Lava leaks up through cracks in the crater!"},
                                            {"A cloud of dust envelopes the entire area!"},
                                            {"The surrounding area roars and shudders from the impact!"},
                                            {"The ground shatters apart from the stress of the impact!"}};
  static constexpr scenery underwater[3] = {
      {"The water churns violently!"},
      {"Large bubbles rise from the movement!"},
      {"The water collapses in on the hole created!", "The water collapses in on the hole create!"}
  };
  static constexpr scenery water[3] = {
      {"A huge column of water erupts from the impact!"},
      {"The impact briefly causes a swirling vortex of water!"},
      {"A huge depression forms in the water and erupts into a wave from the impact!"}};
  static constexpr scenery inside[8] = {{"Debris is thrown into the air and showers down thunderously!"},
                                        {"The structure of the surrounding room cracks and quakes from the impact!"},
                                        {"Parts of the ceiling collapse, crushing into the floor!"},
                                        {"The surrounding area roars and shudders from the impact!"},
                                        {"The ground shatters apart from the stress of the impact!"},
                                        {"The walls of the surrounding room crack in the same instant!"}};
  GameWorld *g                       = s.g;
  struct char_data *vict             = s.vict;
  auto here                          = [&] { return char_get_in_room(vict); };
  auto sect                          = [&] { return room_sect(g, here()); };

  if (where != 2)
    return;
  if (!char_aff_flagged(vict, AFF_KNOCKED) &&
      (rand_number(g, 1, 4) >= 3 && (getCurPL(g, vict) > getCurPL(g, s.ch) / 5) &&
       !char_aff_flagged(vict, AFF_SANCTUARY))) {
    act_triple(g, {"@C$N@W is knocked out!@n", "@WYou are knocked out!@n", "@C$N@W is knocked out!@n", true}, s.ch,
               nullptr, vict);
    setStatusKnockedOut(vict);
  } else if ((char_get_position(vict) == POS_STANDING || char_get_position(vict) == POS_FIGHTING) &&
             !char_aff_flagged(vict, AFF_KNOCKED)) {
    char_set_position(vict, POS_SITTING);
  }
  if (room_get_dmg(g, here()) > 95 || room_flagged(g, here(), ROOM_SPACE))
    return;
  act_triple(g,
             {"@W$N@W slams into the ground forming a large crater with $S body!@n",
              "@WYou slam into the ground forming a large crater with your body!@n",
              "@W$N@W slams into the ground forming a large crater with $S body!@n", true},
             s.ch, nullptr, vict);
  if (sect() != SECT_INSIDE && sect() != SECT_UNDERWATER && sect() != SECT_WATER_SWIM &&
      sect() != SECT_WATER_NOSWIM) {
    impact_sound(g, s.ch, "@wA loud roar is heard nearby!@n\r\n");
    int roll = rand_number(g, 1, 8);
    if (roll != 2)
      show(s, outdoors[roll - 1]);
    else if (rand_number(g, 1, 4) == 4 && room_get_geffect(g, here()) == 0) {
      room_set_geffect(g, here(), 1);
      show(s, outdoors[1]);
    }
  }
  if (sect() == SECT_UNDERWATER)
    show(s, underwater[rand_number(g, 1, 3) - 1]);
  if (sect() == SECT_WATER_SWIM || sect() == SECT_WATER_NOSWIM)
    show(s, water[rand_number(g, 1, 3) - 1]);
  if (sect() == SECT_INSIDE) {
    impact_sound(g, s.ch, "@wA loud roar is heard nearby!@n\r\n");
    show(s, inside[rand_number(g, 1, 8) - 1]);
  }
  room_set_dmg(g, here(), room_get_dmg(g, here()) + 5);
}

/* A tailwhip to the body or the head can knock a standing victim down, more likely to the head */
void tailwhip_knockdown(strike_shot &s, int where)
{
  if (where <= 3 && !char_aff_flagged(s.vict, AFF_FLYING) && char_get_position(s.vict) == POS_STANDING &&
      rand_number(s.g, 1, 8) >= (where == 2 ? 6 : 7))
    handle_knockdown(s.g, s.vict);
}

/* A parried rogafufuken is a counterblow, as a parried strike is */
void rogafufuken_counterblow(ki_shot &s)
{
  GameWorld *g = s.g;

  improve_skill(g, s.vict, SKILL_PARRY, 0);
  pcost(g, s.ch, 0, getEffMaxPL(g, s.vict) / 300); /* quirk: by the victim's power level */
  pcost(g, s.vict, 0, getEffMaxPL(g, s.vict) / 500);
  s.dmg = damtype(g, s.vict, -2, s.skill, s.attperc);
  s.dmg *= calc_critical(g, s.ch, 1);
  hurt(g, 0, 0, s.vict, s.ch, nullptr, s.dmg, -1);
}

void rogafufuken_fireshield(ki_shot &s)
{
  tech_handle_fireshield(s.g, s.ch, s.vict, "hands and feet");
}

const strike_attack HEELDROP = {
    .skill             = SKILL_HEELDROP,
    .name              = "Heeldrop",
    .damtype           = 8,
    .stamina_share     = 90,
    .improve_mode      = 1,
    .needs             = strike_needs::nothing,
    .not_while_singing = true,
    .skill_before_grav = true,
    .parry_crit        = 0,
    .limb_hurt         = 180,
    .leg_flag          = 1,
    .fireshield        = "leg",
    .parry             = {"@C$N@W parries your heeldrop with a punch of $S own!@n", "@WYou parry @C$n's@W heeldrop with a punch of your own!@n", "@C$N@W parries @c$n's@W heeldrop with a punch of $S own!@n", true},
    .block             = {"@C$N@W moves quickly and blocks your heeldrop!@n", "@WYou move quickly and block @C$n's@W heeldrop!@n", "@C$N@W moves quickly and blocks @c$n's@W heeldrop!@n", true},
    .dodge             = {"@C$N@W manages to dodge your heeldrop!@n", "@WYou dodge @C$n's@W heeldrop!@n", "@C$N@W manages to dodge @c$n's@W heeldrop!@n", true},
    .miss              = {"@WYou can't believe it but your heeldrop misses!@n", "@C$n@W moves to heeldrop you, but misses!@n", "@c$n@W moves to heeldrop @C$N@W, but somehow misses!@n", true},
    .miss_spent        = {"@WYou can't believe it but your heeldrop misses!@n", "@C$n@W moves to heeldrop you, but misses!@n", "@c$n@W moves to heeldrop @C$N@W, but somehow misses!@n", true},
    .hits              = {{"@WYou disappear, appearing above @C$N@W you spin and heeldrop $M in the face!@n", "@C$n@W disappears, only to appear above you, spinning quickly and heeldropping you in the face!@n", "@c$n@W disappears, only to appear above @C$N@W, spinning quickly and heeldropping $M in the face!@n", true}, {"@WYou disappear, reappearing in front of @C$N@W, you flip upside down and slam your heel into the top of $S head!@n", "@C$n@W disappears, reappearing in front of you, $e flips upside down and slams $s heel into the top of your head!@n", "@c$n@W disappears, reappearing in front of @C$N@W, $e flips upside down and slams $s heel into the top of @C$N@W's head!@n", true}, {"@WYou fly at @C$N@W, heeldropping $S gut as you fly!@n", "@C$n@W flies at you, heeldropping your gut as $e flies!@n", "@c$n@W flies at @C$N@W, heeldropping $S gut as $e flies!@n", true}, {"@WYou heeldrop @C$N@W, hitting $M in the arm!@n", "@C$n@W heeldrops you, hitting you in the arm!@n", "@c$n@W heeldrops @C$N@W, hitting $M in the arm!@n", true}, {"@WYou heeldrop @C$N's@W leg!@n", "@C$n@W heeldrops your leg!@n", "@c$n@W heeldrops @C$N's@W leg!@n", true}},
    .obj_to_char       = "@WYou heeldrop $p@W as hard as you can!@n",
    .obj_to_room       = "@C$n@W heeldrops $p@W extremely hard!@n",
    .obj_hide          = true,
    .cooldown_for      = heeldrop_cooldown,
    .defend            = heeldrop_defense,
    .on_spot           = heeldrop_crashdown,
};

const strike_attack SLAM = {
    .skill         = SKILL_SLAM,
    .name          = "slam",
    .damtype       = 6,
    .stamina_share = 100,
    .improve_mode  = 1,
    .limb_hurt     = 195,
    .leg_flag      = 1,
    .fireshield    = "hands",
    .parry         = {"@C$N@W parries your slam with a punch of $S own!@n", "@WYou parry @C$n's@W slam with a punch of your own!@n", "@C$N@W parries @c$n's@W slam with a punch of $S own!@n", true},
    .block         = {"@C$N@W moves quickly and blocks your slam!@n", "@WYou move quickly and block @C$n's@W slam!@n", "@C$N@W moves quickly and blocks @c$n's@W slam!@n", true},
    .dodge         = {"@C$N@W manages to dodge your slam!@n", "@WYou dodge @C$n's@W slam!@n", "@C$N@W manages to dodge @c$n's@W slam!@n", true},
    .miss          = {"@WYou can't believe it but your slam misses!@n", "@C$n@W moves to slam you with both $s fists, but misses!@n", "@c$n@W moves to slam @C$N@W with both $s fists, but somehow misses!@n", true},
    .miss_spent    = {"@WYou can't believe it but your slam misses!@n", "@C$n@W moves to slam you with both $s fists, but misses!@n", "@c$n@W moves to slam @C$N@W with both $s fists, but somehow misses!@n", true},
    .hits          = {{"@WYou disappear, appearing above @C$N@W and slam a double fisted blow into $M!@n", "@C$n@W disappears, only to appear above you, slamming a double fisted blow into you!@n", "@c$n@W disappears, only to appear above @C$N@W, slamming a double fisted blow into $M!@n", true}, {"@WYou disappear, reappearing in front of @C$N@W, you grab $M! Spinning you send $M flying into the ground!@n", "@C$n@W disappears, reappearing in front of you, and $e grabs you! Spinning quickly $e sends you flying into the ground!@n", "@c$n@W disappears, reappearing in front of @C$N@W, and grabs $M! Spinning quickly $e sends $M flying into the ground!@n", true}, {"@WYou fly at @C$N@W, slamming both your fists into $S gut as you fly!@n", "@C$n@W flies at you, slamming both $s fists into your gut as $e flies!@n", "@c$n@W flies at @C$N@W, slamming both $s fists into $S gut as $e flies!@n", true}, {"@WYou slam both your fists into @C$N@W, hitting $M in the arm!@n", "@C$n@W slams both $s fists into you, hitting you in the arm!@n", "@c$n@W slams both $s fists into @C$N@W, hitting $M in the arm!@n", true}, {"@WYou slam both your fists into @C$N's@W leg!@n", "@C$n@W slams both $s fists into your leg!@n", "@c$n@W slams both $s fists into @C$N's@W leg!@n", true}},
    .obj_to_char   = "@WYou slam $p@W as hard as you can!@n",
    .obj_to_room   = "@C$n@W slams $p@W extremely hard!@n",
    .obj_hide      = true,
    .cooldown_for  = slam_cooldown,
    .before_damage = slam_crater,
};

const strike_attack TAILWHIP = {
    .skill             = SKILL_TAILWHIP,
    .name              = "tailwhip",
    .damtype           = 56,
    .cooldown          = 7,
    .stamina_share     = 120,
    .improve_mode      = 1,
    .needs             = strike_needs::tail,
    .not_while_singing = true,
    .keeps_combo       = true,
    .block_damtype     = 3,
    .limb_hurt         = 195,
    .leg_flag          = 1,
    .fireshield        = "tail",
    .parry             = {"@C$N@W parries your tailwhip with a punch of $S own!@n", "@WYou parry @C$n's@W tailwhip with a punch of your own!@n", "@C$N@W parries @c$n's@W tailwhip with a punch of $S own!@n", true},
    .block             = {"@C$N@W moves quickly and blocks your tailwhip!@n", "@WYou move quickly and block @C$n's@W tailwhip!@n", "@C$N@W moves quickly and blocks @c$n's@W tailwhip!@n", true},
    .dodge             = {"@C$N@W manages to dodge your tailwhip!@n", "@WYou dodge @C$n's@W tailwhip!@n", "@C$N@W manages to dodge @c$n's@W tailwhip!@n", true},
    .miss              = {"@WYou can't believe it but your tailwhip misses!@n", "@C$n@W throws a tailwhip at you but somehow misses!@n", "@c$n@W throws a tailwhip at @C$N@W but somehow misses!@n", true},
    .miss_spent        = {"@WYou can't believe it but your tailwhip misses!@n", "@C$n@W throws a tailwhip at you but somehow misses!@n", "@c$n@W throws a tailwhip at @C$N@W but somehow misses!@n", true},
    .hits              = {{"@WYou spin to swing your tail and slam it into @c$N@W's body!@n", "@C$n@W spins to swing $s tail and slams it into YOUR body!@n", "@C$n@W spins to swing $s tail and slams it into @c$N@W's body!@n", true}, {"@WYou flip forward and slam your tail into the top of @c$N@W's head brutally!@n", "@C$n@W flips forward and slams $s tail into the top of YOUR head brutally!@n", "@C$n@W flips forward and slams $s tail into the top of @c$N@W's head brutally!@n", true}, {"@WYou swing your tail and manage to slam it into @c$N@W's gut!@n", "@C$n@W swings $s tail and manages to slam it into YOUR gut!@n", "@C$n@W swings $s tail and manages to slam it into @c$N@W's gut!@n", true}, {"@WYou swing your tail and manage to slam it into @c$N@W's arm!@n", "@C$n@W swings $s tail and manages to slam it into YOUR arm!@n", "@C$n@W swings $s tail and manages to slam it into @c$N@W's arm!@n", true}, {"@WYou swing your tail and manage to slam it into @c$N@W's leg!@n", "@C$n@W swings $s tail and manages to slam it into YOUR leg!@n", "@C$n@W swings $s tail and manages to slam it into @c$N@W's leg!@n", true}},
    .obj_to_char       = "@WYou tailwhip $p@W as hard as you can!@n",
    .obj_to_room       = "@C$n@W tailwhips $p@W extremely hard!@n",
    .obj_hide          = true,
    .on_spot           = tailwhip_knockdown,
};

const ki_attack ROGAFUFUKEN = {
    .skill          = SKILL_ROGAFUFUKEN,
    .name           = "Rogafufuken",
    .damtype        = 23,
    .attperc        = .05,
    .minimum        = .01,
    .cooldown       = 7,
    .stamina_share  = 50,
    .ki_accuracy    = false,
    .limb_hurt      = 170,
    .dodge_scatters = false,
    .parry          = {"@C$N@W parries your rogafufuken with a punch of $s own!@n", "@WYou parry @C$n's@W rogafufuken with a punch of your own!@n", "@C$N@W parries @c$n's@W rogafufuken with a punch of $s own!@n"},
    .block          = {"@C$N@W moves quickly and blocks your rogafufuken!@n", "@WYou move quickly and block @C$n's@W rogafufuken!@n", "@C$N@W moves quickly and blocks @c$n's@W rogafufuken!@n"},
    .dodge          = {"@C$N@W manages to dodge your rogafufuken!@n", "@WYou dodge @C$n's@W rogafufuken!@n", "@C$N@W manages to dodge @c$n's@W rogafufuken!@n"},
    .miss           = {"@WYou can't believe it but your rogafufuken misses!@n", "@C$n@W fires a rogafufuken at you, but misses!@n ", "@c$n@W fires a rogafufuken at @C$N@W, but somehow misses!@n "},
    .miss_spent     = {"@WYou can't believe it but your rogafufuken misses!@n", "@C$n@W fires a rogafufuken at you, but misses!@n", "@c$n@W fires a rogafufuken at @C$N@W, but somehow misses!@n"},
    .hits           = {{"@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's body!@n", "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR body!@n", "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's body!@n", true}, {"@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's face!@n", "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR face!@n", "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's face!@n", true}, {"@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's gut!@n", "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR gut!@n", "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's gut!@n", true}, {"@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's arm!@n", "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR arm!@n", "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's arm!@n", true}, {"@WYou pour your charged energy into your hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as you leap towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. You unleash a flurry of hand strikes on @c$N@W's leg!@n", "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards YOU while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on YOUR leg!@n", "@C$n@W pours $s charged energy into $s hands and feet. A @rr@Re@wd@W glow trails behind the movements of either as $e leaps towards @c$N@W while yelling '@cW@Co@Wl@wf @DFang @rFist@W!'. $e unleashes a flurry of hand strikes on @c$N@W's leg!@n", true}},
    .obj_to_char    = "@WYou fire a rogafufuken at $p@W!@n",
    .obj_to_room    = "@C$n@W fires a rogafufuken at $p@W!@n",
    .on_parry       = rogafufuken_counterblow,
    .after_hit      = rogafufuken_fireshield,
};

/* A bite's hits, by where roll_hitloc() says it landed */
constexpr struct {
  act_msgs msgs;
  int crit; /* calc_critical()'s argument; 2 only with BONUS_SOFT */
  int eq;   /* dam_eq_loc(): 1 arms, 2 legs, 3 head, 4 body */
} BITES[5] = {
    {{"@WYou bite @C$N's@W face!@n", "@C$n@W bites your face!@n", "@c$n@W bites into $N's face!@n", true},        2, 3},
    {{"@WYou bite @C$N@W!@n", "@C$n@W bites you!@n", "@C$n@W bites @c$N@W!@n", true},                             0, 4},
    {{"@WYou bite @C$N's@W body!@n", "@C$n@W bites you on the body, sending blood flying!@n",
      "@C$n@W bites @c$N@W on the body, sending blood flying!@n", true},
     2,
     4                                                                                                                },
    {{"@WYou bite @C$N's@W arm!@n", "@C$n@W bites you on the arm!@n", "@C$n@W bites @c$N@W on the arm!@n", true}, 1, 1},
    {{"@WYou bite @C$N's@W leg!@n", "@C$n@W bites into your leg!@n", "@c$n@W bites into @c$N's@W leg!@n", true},  1, 2},
};

/* A bite at someone. stcost: what it costs the biter if it lands; anything else, half */
void bite_victim(GameWorld *g, struct char_data *ch, struct char_data *vict, int skill, int64_t stcost)
{
  int pry = 2, dge = 2, blk = 2;
  int64_t dmg;

  if (!can_kill(g, ch, vict, nullptr, 0))
    return;
  if (handle_defender(g, vict, ch))
    vict = char_get_defender(g, vict);
  int index = check_def(g, vict); /* the victim's parry, block and dodge */
  int prob  = roll_accuracy(g, ch, skill, true);
  int perc  = chance_to_hit(g, ch);
  index -= handle_speed(g, ch, vict);
  int avo = index / 4;
  handle_defense(g, vict, &pry, &blk, &dge);
  prob -= avo;
  tech_handle_posmodifier(vict, &pry, &blk, &dge, &prob);

  if (!tech_handle_zanzoken(g, ch, vict, "bite")) {
    pcost(g, ch, 0, stcost / 2);
    pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
    return;
  }

  if (prob < perc - 20) {
    if (getCurST(g, vict) <= 0) { /* quirk: the biter isn't told */
      act_triple(g,
                 {nullptr, "@C$n@W moves to bite you, but misses!@n",
                  "@c$n@W moves to bite @C$N@W, but somehow misses!@n", true},
                 ch, nullptr, vict);
      pcost(g, ch, 0, stcost / 2);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    if (pry > rand_number(g, 1, 140) && (!char_is_npc(vict) || !char_mob_flagged(vict, MOB_DUMMY))) {
      act_triple(g,
                 {"@C$N@W parries your bite with a punch of their own!@n",
                  "@WYou parry @C$n's@W bite with a punch of your own!@n",
                  "@C$N@W parries @c$n's@W bite with a punch of $S own!@n", true},
                 ch, nullptr, vict);
      improve_skill(g, vict, SKILL_PARRY, 0);
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
      dmg = damtype(g, vict, -1, skill, 0);
      dmg *= calc_critical(g, ch, 1);
      hurt(g, 0, 0, vict, ch, nullptr, dmg, -1); /* the counterblow */
      return;
    }
    if (blk > axion_dice(g, 10)) {
      act_triple(g,
                 {"@C$N@W moves quickly and blocks your bite!@n", "@WYou move quickly and block @C$n's@W bite!@n",
                  "@C$N@W moves quickly and blocks @c$n's@W bite!@n", true},
                 ch, nullptr, vict);
      improve_skill(g, vict, SKILL_BLOCK, 0);
      pcost(g, ch, 0, stcost / 2);
      pcost(g, vict, 0, getEffMaxPL(g, vict) / 500);
      dmg = damtype(g, ch, 6, skill, 0) / 4;
      hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
      return;
    }
    if (dge > axion_dice(g, 10)) {
      act_triple(g,
                 {"@C$N@W manages to dodge your bite!@n", "@WYou dodge @C$n's@W bite!@n",
                  "@C$N@W manages to dodge @c$n's@W bite!@n", true},
                 ch, nullptr, vict);
      improve_skill(g, vict, SKILL_DODGE, 0);
      pcost(g, ch, 0, stcost / 2);
      hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
      return;
    }
    act_triple(g,
               {"@WYou move to bite @C$N@W, but miss!@n", "@C$n@W moves to bite you, but misses!@n",
                "@c$n@W moves to bite @C$N@W, but somehow misses!@n", true},
               ch, nullptr, vict);
    pcost(g, ch, 0, stcost / 2);
    hurt(g, 0, 0, ch, vict, nullptr, 0, 0);
    return;
  }

  dmg = damtype(g, ch, 8, skill, 0);
  if (!char_is_npc(ch)) /* quirk: after rolling it as a mob's */
    dmg = damtype(g, ch, 0, skill, 0);
  dmg += dmg * 0.25;
  int where = roll_hitloc(g, ch, vict, skill);
  if (where >= 1 && where <= 5) {
    const auto &bite = BITES[where - 1];
    act_triple(g, bite.msgs, ch, nullptr, vict);
    if (bite.crit != 2)
      dmg *= calc_critical(g, ch, bite.crit);
    else if (char_get_bonuses(ch, BONUS_SOFT))
      dmg *= calc_critical(g, ch, 2);
    hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
    dam_eq_loc(g, vict, bite.eq);
  }
  /* a mutant's bite can poison */
  if (!char_is_npc(ch) && axion_dice(g, 0) > char_get_aff_abils_con(vict) && rand_number(g, 1, 5) == 5) {
    act(g, "@R$N@r was poisoned by your bite!@n", true, ch, nullptr, vict, TO_CHAR);
    act(g, "@rYou were poisoned by the bite!@n", true, ch, nullptr, vict, TO_VICT);
    char_set_poisonby(vict, ch);
    int duration = (char_get_aff_abils_intel(ch) / 50) + 1;
    assign_affect(g, vict, AFF_POISON, SKILL_POISON, duration, 0, 0, 0, 0, 0, 0);
  }
  pcost(g, ch, 0, stcost);
}

} // namespace

ACMD(do_rogafufuken)
{
  perform_ki_attack(g, ch, argument, ROGAFUFUKEN);
}

ACMD(do_heeldrop)
{
  perform_strike(g, ch, argument, HEELDROP);
}

/* Mutants with the venom genome, and mobs, can bite */
ACMD(do_bite)
{
  int64_t stcost         = 20 + (getEffMaxPL(g, ch) / 500);
  std::string_view rest  = argument;
  std::string arg        = take_arg(rest);
  struct char_data *vict = nullptr;
  struct obj_data *obj   = nullptr;

  if (!char_is_npc(ch) && (!char_is_mutant(ch) || (char_get_genome(ch, 0) != 7 && char_get_genome(ch, 1) != 7))) {
    send_to_char(g, ch, "You don't want to put that in your mouth, you don't know where it has been!\r\n");
    return;
  }
  if (!can_grav(g, ch))
    return;
  if (arg.empty() && !char_get_fighting(g, ch))
    return;
  if (!check_points(g, ch, 0, getEffMaxPL(g, ch) / 200))
    return;
  int skill = init_skill(g, ch, SKILL_PUNCH);
  if (skill <= 0)
    skill = 60;
  /* whoever is named, or else whoever ch is fighting here, or else something here */
  if (arg.empty() || !(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (char_get_fighting(g, ch) && char_get_in_room(char_get_fighting(g, ch)) == char_get_in_room(ch))
      vict = char_get_fighting(g, ch);
    else if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, char_get_in_room(ch)))))
      return;
  }
  handle_cooldown(g, ch, 4);
  if (vict) {
    bite_victim(g, ch, vict, skill, stcost);
    return;
  }
  /* can_kill() refuses a broken object, so it is never broken here */
  if (!can_kill(g, ch, nullptr, obj, 0))
    return;
  int64_t dmg = getCurPL(g, ch) / 10000 + char_get_aff_abils_str(ch);
  act(g, "@C$n@W bites $p@W extremely hard!@n", true, ch, obj, nullptr, TO_ROOM); /* quirk: the biter isn't told */
  hurt(g, 0, 0, ch, nullptr, obj, dmg, 0);
  pcost(g, ch, 0, stcost);
}

ACMD(do_slam)
{
  perform_strike(g, ch, argument, SLAM);
}

ACMD(do_tailwhip)
{
  perform_strike(g, ch, argument, TAILWHIP);
}
