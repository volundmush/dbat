/* barrages.c: multi-hit ki barrages
 * Split from act.offensive.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/args.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.offensive.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/consts/deathtype.hpp"
#include "dbat/config.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/random.hpp"
#include "dbat/search.hpp"
#include "dbat/relocate.hpp"
#include "dbat/db.hpp"
#include "dbat/extract.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/combat.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/fight.hpp"
#include "dbat/guild.hpp"
#include "dbat/class.hpp"
#include "dbat/techniques.hpp"
#include "dbat/game_world.hpp"
#include "ki_attack.hpp"

namespace {

/* A psychic barrage can break the victim's mind's eye, or else shock them */
void psychic_barrage_mind(ki_shot &s)
{
  GameWorld *g           = s.g;
  struct char_data *vict = s.vict;

  if (!char_aff_flagged(vict, AFF_MBREAK) && rand_number(g, 1, 4) == 4 && !char_aff_flagged(vict, AFF_SANCTUARY)) {
    act(g, "@mYour mind's eye has been shattered, you can't charge ki until you recover!@n", true, vict, nullptr, nullptr, TO_CHAR);
    act(g, "@M$n@m's mind has been damaged by the attack!@n", true, vict, nullptr, nullptr, TO_ROOM);
    char_aff_set(vict, AFF_MBREAK, true);
  } else if (!char_aff_flagged(vict, AFF_SHOCKED) && rand_number(g, 1, 4) == 4 && !char_aff_flagged(vict, AFF_SANCTUARY)) {
    act(g, "@MYour mind has been shocked!@n", true, vict, nullptr, nullptr, TO_CHAR);
    act(g, "@M$n@m's mind has been shocked!@n", true, vict, nullptr, nullptr, TO_ROOM);
    char_aff_set(vict, AFF_SHOCKED, true);
  }
}

/* Piccolo's students fire a scatter shot sooner the better their style */
int scatter_shot_cooldown(ki_shot &s)
{
  if (!char_is_piccolo(s.ch))
    return 8;
  int style = char_get_skills_base(s.ch, SKILL_STYLE);
  return style >= 100 ? 5 : style >= 75 ? 6
                        : style >= 40   ? 7
                                        : 8;
}

/* Hundreds of kiballs are harder to get away from */
void scatter_shot_spread(ki_shot &s, int &prob)
{
  prob += rand_number(s.g, 10, 20);
}

/* A master of the renzokou energy dan may spend less on it: mastery 1 a quarter, 2 a half */
void renzokou_mastery(ki_shot &s, int &)
{
  int roll          = rand_number(s.g, 1, 100);
  int master_chance = s.skill >= 100 ? 10 : s.skill >= 50 ? 5
                                                          : 0;
  int half_chance   = s.skill >= 100 ? 20 : s.skill >= 75 ? 10
                                        : s.skill >= 50   ? 5
                                                          : 0;

  if (master_chance >= roll)
    s.mastery = 1;
  else if (half_chance >= roll)
    s.mastery = 2;
  /* quirk: the quarter is the one that says "more efficient", the half "as efficient as possible" */
  if (s.mastery == 1)
    send_to_char(s.g, s.ch, "@GYour mastery of the technique has made your use of energy more efficient!@n\r\n");
  else if (s.mastery == 2)
    send_to_char(s.g, s.ch, "@GYour mastery of the technique has made your use of energy as efficient as possible!@n\r\n");
}

double renzokou_cost(const ki_shot &s, ki_outcome)
{
  return s.mastery == 1 ? s.attperc * 0.25 : s.mastery == 2 ? s.attperc * 0.5
                                                            : s.attperc;
}

/* How many of the shots (out of 100, or more for Nail's students) got through: none is a miss */
bool renzokou_count(ki_shot &s, int prob, int perc)
{
  int count = 0;

  /* quirk: a shot just short of perc (by up to 5) gets nothing through; and 75 was meant for "prob < perc - 20",
   * which the first case already takes */
  if (prob + 20 < perc)
    count = 0;
  else if (prob + 15 < perc)
    count = 10;
  else if (prob + 10 < perc)
    count = 25;
  else if (prob + 5 < perc)
    count = 50;
  else if (prob > perc)
    count = 100;
  if (count > 0) {
    if (char_is_nail(s.ch)) {
      int skill = char_get_skill(s.ch, SKILL_RENZO);
      count += skill >= 100 ? 200 : skill >= 60 ? 100
                                : skill >= 40   ? 40
                                                : 0;
    }
    if (rand_number(s.g, 1, 5) >= 5) /* a random boost or loss for everyone */
      count += rand_number(s.g, -15, 25);
  }
  s.tally = count;
  return count != 0;
}

/* Its messages are by how many got through, all to fewer; none of them for fewer than 10 */
int renzokou_locate(ki_shot &s)
{
  return s.tally >= 100 ? 1 : s.tally >= 75 ? 2
                          : s.tally >= 50   ? 3
                          : s.tally >= 25   ? 4
                          : s.tally >= 10   ? 5
                                            : 0;
}

/* Each shot that got through does a hundredth of the damage */
bool renzokou_strike(ki_shot &s, int where)
{
  if (where == 5) { /* quirk: rounded down to a hundredth first */
    s.dmg /= 100;
    s.dmg *= s.tally;
  } else {
    s.dmg = s.dmg * 0.01;
    s.dmg *= s.tally;
  }
  hurt(s.g, 0, 0, s.ch, s.vict, nullptr, s.dmg, 1);
  return true;
}

/* A master's tsuihidan can hit a vital spot */
void tsuihidan_mastery(ki_shot &s, int &)
{
  int roll   = rand_number(s.g, 1, 100);
  int chance = s.skill >= 100 ? 20 : s.skill >= 75 ? 10
                                 : s.skill >= 50   ? 5
                                                   : 0;
  s.mastery  = chance >= roll;
}

/* ... and sap as much stamina as it did damage */
void tsuihidan_sap(ki_shot &s)
{
  if (!s.mastery)
    return;
  decCurST(s.g, s.vict, s.dmg, 0);
  ki_act(s, {"@CYour tsuihidan hits a vital spot and seems to sap some of @c$N's@C stamina!@n",
             "@C$n's@C tsuihidan hits a vital spot and saps some of your stamina!@n",
             "@C$n's@C tsuihidan hits a vital spot and saps some of @c$N's@C stamina!", true});
}

const ki_attack PSYCHIC_BARRAGE = {
    .skill       = SKILL_PBARRAGE,
    .name        = "Psychic Barrage",
    .damtype     = 31,
    .attperc     = .2,
    .minimum     = .15,
    .cooldown    = 7,
    .block       = {"@C$N@W moves quickly and blocks your psychic barrage!@n", "@WYou move quickly and block @C$n's@W psychic barrage!@n", "@C$N@W moves quickly and blocks @c$n's@W psychic barrage!@n"},
    .dodge       = {"@C$N@W manages to dodge your psychic barrage, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W psychic barrage, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W psychic barrage, letting it slam into the surroundings!@n"},
    .miss        = {"@WYou can't believe it but your psychic barrage misses, flying through the air harmlessly!@n", "@C$n@W fires a psychic barrage at you, but misses!@n ", "@c$n@W fires a psychic barrage at @C$N@W, but somehow misses!@n "},
    .miss_spent  = {"@WYou can't believe it but your psychic barrage misses, flying through the air harmlessly!@n", "@C$n@W fires a psychic barrage at you, but misses!@n", "@c$n@W fires a psychic barrage at @C$N@W, but somehow misses!@n"},
    .hits        = {{"@WYou close your eyes for a moment as your charged ki is pooled into your brain. Your eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and look at @c$N@W intensly. Invisible waves of psychic energy slam into $S body!@n", "@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at YOU intensly. Invisible waves of psychic energy slam into YOUR body!@n", "@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at @c$N@W intensly. Invisible waves of psychic energy slam into @c$N@W's body!@n", true}, {"@WYou close your eyes for a moment as your charged ki is pooled into your brain. Your eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and look at @c$N@W intensly. Invisible waves of psychic energy slam into $S head!@n", "@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at YOU intensly. Invisible waves of psychic energy slam into YOUR head!@n", "@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at @c$N@W intensly. Invisible waves of psychic energy slam into @c$N@W's head!@n", true}, {"@WYou close your eyes for a moment as your charged ki is pooled into your brain. Your eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and look at @c$N@W intensly. Invisible waves of psychic energy slam into $S gut!@n", "@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at YOU intensly. Invisible waves of psychic energy slam into YOUR gut!@n", "@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at @c$N@W intensly. Invisible waves of psychic energy slam into @c$N@W's gut!@n", true}, {"@WYou close your eyes for a moment as your charged ki is pooled into your brain. Your eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and look at @c$N@W intensly. Invisible waves of psychic energy slam into $S arm!@n", "@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at YOU intensly. Invisible waves of psychic energy slam into YOUR arm!@n", "@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at @c$N@W intensly. Invisible waves of psychic energy slam into @c$N@W's arm!@n", true}, {"@WYou close your eyes for a moment as your charged ki is pooled into your brain. Your eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and look at @c$N@W intensly. Invisible waves of psychic energy slam into $S leg!@n", "@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at YOU intensly. Invisible waves of psychic energy slam into YOUR leg!@n", "@C$n@W closes $s eyes for a moment as $s charged ki is pooled into $s brain. @C$n@W's eyes snap open, flashing with @bb@Bl@Wu@we @yl@Yi@Wg@wht@W, and looks at @c$N@W intensly. Invisible waves of psychic energy slam into @c$N@W's leg!@n", true}},
    .obj_to_char = "@WYou fire a psychic barrage at $p@W!@n",
    .obj_to_room = "@C$n@W fires a psychic barrage at $p@W!@n",
    .after_hit   = psychic_barrage_mind,
};
/* left out of the table: */
/*   "@mYour mind's eye has been shattered, you can't charge ki until you recover!@n" */
/*   "@M$n@m's mind has been damaged by the attack!@n" */
/*   "@MYour mind has been shocked!@n" */
/*   "@M$n@m's mind has been shocked!@n" */

const ki_attack SCATTER_SHOT = {
    .skill           = SKILL_SCATTER,
    .name            = "Scatter Shot",
    .damtype         = 35,
    .attperc         = .3,
    .minimum         = .15,
    .kill_mode       = 3,
    .dodge_dmg_cap   = 80,
    .dodge_dmg       = 20,
    .limb_hurt       = 170,
    .dodge_explosion = "@wBright explosions erupts from the impacts!\r\n",
    .block           = {"@C$N@W moves quickly and blocks every kiball of your scatter shot!@n", "@WYou move quickly and block every kiball of @C$n's@W scatter shot!@n", "@C$N@W moves quickly and blocks every kiball of @c$n's@W scatter shot!@n"},
    .dodge           = {"@C$N@W manages to dodge your scatter shot kiballs, letting them slam into the surroundings!@n", "@WYou dodge @C$n's@W scatter shot kiballs, letting them slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W scatter shot kiballs, letting them slam into the surroundings!@n"},
    .miss            = {"@WYou can't believe it but all the kiballs of your scatter shot miss, flying through the air harmlessly!@n", "@C$n@W fires scatter shot kiballs at you, but they miss!@n ", "@c$n@W fires scatter shot kiballs at @C$N@W, but somehow they miss!@n "},
    .miss_spent      = {"@WYou can't believe it but all the kiballs of your scatter shot miss, flying through the air harmlessly!@n", "@C$n@W fires scatter shot kiballs at you, but they miss!@n ", "@c$n@W fires scatter shot kiballs at @C$N@W, but somehow they miss!@n "},
    .hits            = {{"@WYou bring your charged ki to your palms and furiously begin to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! You hold out your hand and clench it dramatically as your @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's body!@n", "@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at YOU! The kiballs surround you from every direction in a sphere, preventing your escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against YOUR body!@n", "@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's body!@n", true}, {"@WYou bring your charged ki to your palms and furiously begin to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! You hold out your hand and clench it dramatically as your @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's head!@n", "@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at YOU! The kiballs surround you from every direction in a sphere, preventing your escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against YOUR head!@n", "@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's head!@n", true}, {"@WYou bring your charged ki to your palms and furiously begin to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! You hold out your hand and clench it dramatically as your @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's gut!@n", "@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at YOU! The kiballs surround you from every direction in a sphere, preventing your escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against YOUR gut!@n", "@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's gut!@n", true}, {"@WYou bring your charged ki to your palms and furiously begin to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! You hold out your hand and clench it dramatically as your @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's arm!@n", "@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at YOU! The kiballs surround you from every direction in a sphere, preventing your escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against YOUR arm!@n", "@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's arm!@n", true}, {"@WYou bring your charged ki to your palms and furiously begin to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! You hold out your hand and clench it dramatically as your @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's leg!@n", "@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at YOU! The kiballs surround you from every direction in a sphere, preventing your escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against YOUR leg!@n", "@C$n@W brings $s charged ki to $s palms and furiously begins to throw hundreds of kiballs at @C$N@W! The kiballs surround $M from every direction in a sphere, preventing $S escape! @C$n@W holds out $s hand and clenches it dramatically as $s @yS@Yc@ra@Rt@yt@Yer @yS@Yh@ro@Rt@W kiballs close in and explode against @C$N@W's leg!@n", true}},
    .obj_to_char     = "@WYou fire a scatter shot at $p@W!@n",
    .obj_to_room     = "@C$n@W fires a scatter shot at $p@W!@n",
    .cooldown_for    = scatter_shot_cooldown,
    .steady          = scatter_shot_spread,
};

const ki_attack RENZOKOU = {
    .skill          = SKILL_RENZO,
    .name           = "Renzokou Energy Dan",
    .damtype        = 12,
    .attperc        = .125,
    .minimum        = .05,
    .cooldown       = 5,
    .absorb         = ki_absorb::costs_one,
    .parry_returns  = false,
    .parry_scatters = false,
    .targets_first  = true,
    .dodge_scatters = false,
    .parry          = {"@C$N@W deflects every shot of your renzokou energy dan, sending them flying away!@n", "@WYou deflect every shot @C$n's@W renzokou energy dan sending them flying away!@n", "@C$N@W deflects every shot of @c$n's@W renzokou energy dan sending them flying away!@n", true},
    .block          = {"@C$N@W moves quickly and blocks every renzokou energy dan shot with $S arms!@n", "@WYou move quickly and block @C$n's@W renzokou energy dan!@n", "@C$N@W moves quickly and blocks @c$n's@W renzokou energy dan with $S arms!@n", true},
    .dodge          = {"@C$N@W manages to dodge all your renzokou energy dan shots, letting them slam into the surroundings!@n", "@WYou dodge all of @C$n's @Wrenzokou energy dan shots, letting them slam into the surroundings!@n", "@C$N@W manages to dodge all @c$n's@W renzokou energy dan shots, letting them slam into the surroundings!@n", true},
    .miss           = {"@WYou can't believe it but all your renzokou energy dan shots miss, flying through the air harmlessly!@n", "@C$n@W fires hundreds of renzokou energy dan shots at you, but misses!@n ", "@c$n@W fires hundreds of renzokou energy dan shots at @C$N@W, but somehow misses!@n ", true},
    .miss_spent     = {"@WYou can't believe it but all your renzokou energy dan shots miss, flying through the air harmlessly!@n", "@C$n@W fires hundreds of renzokou energy dan shots at you, but misses!@n", "@c$n@W fires hundreds of renzokou energy dan shots at @C$N@W, but somehow misses!@n", true},
    .hits           = {{"@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! All of the shots hit!", "@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! All of the shots hit!", "@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! All of the shots hit!", true}, {"@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Most of the shots hit, but some of them are avoided by $M!", "@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! Most of the shots hit, but some of them you manage to avoid!", "@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Most of the shots hit, but some of them are avoided by $M!", true}, {"@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! About half of the shots hit, the rest are avoided by $M!", "@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! About half of the shots hit, the rest you manage to avoid!", "@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! About half of the shots hit, the rest are avoided by $M!", true}, {"@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Few of the shots hit, the rest are avoided by $M!", "@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! Few of the shots hit, the rest you manage to avoid!", "@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Few of the shots hit, the rest are avoided by $M!", true}, {"@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Very few of the shots hit, the rest are avoided by $M!", "@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! Very few of the shots hit, the rest you manage to avoid!", "@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Very few of the shots hit, the rest are avoided by $M!", true}},
    .obj_to_char    = "@WYou fire hundreds of renzokou energy dan shots at $p@W!@n",
    .obj_to_room    = "@C$n@W fires hundreds of renzokou energy dan shots at $p@W!@n",
    .obj_hide       = true,
    .cost           = renzokou_cost,
    .steady         = renzokou_mastery,
    .through        = renzokou_count,
    .locate         = renzokou_locate,
    .strike         = renzokou_strike,
};

const ki_attack TSUIHIDAN = {
    .skill          = SKILL_TSUIHIDAN,
    .name           = "Tsuihidan",
    .parry_name     = "tsuihidan",
    .damtype        = 11,
    .attperc        = .1,
    .minimum        = .05,
    .cooldown       = 5,
    .absorb         = ki_absorb::costs_one,
    .limb_hurt      = 195,
    .steers         = 1,
    .homing         = true,
    .parry_scatters = false,
    .soft_crit      = false,
    .dodge_scatters = false,
    .parry          = {"@C$N@W deflects your tsuihidan, sending it flying away!@n", "@WYou deflect @C$n's@W tsuihidan sending it flying away!@n", "@C$N@W deflects @c$n's@W tsuihidan sending it flying away!@n", true},
    .block          = {"@C$N@W moves quickly and blocks tsuihidan!@n", "@WYou move quickly and block @C$n's@W tsuihidan!@n", "@C$N@W moves quickly and blocks @c$n's@W tsuihidan!@n", true},
    .dodge          = {"@C$N@W manages to dodge your tsuihidan, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W tsuihidan, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W tsuihidan, letting it slam into the surroundings!@n", true},
    .miss           = {"@WYou can't believe it but your tsuihidan misses, flying through the air harmlessly!@n", "@C$n@W fires a tsuihidan at you, but misses!@n ", "@c$n@W fires a tsuihidan at @C$N@W, but somehow misses!@n ", true},
    .miss_spent     = {"@WYou can't believe it but your tsuihidan misses, flying through the air harmlessly!@n", "@C$n@W fires a tsuihidan at you, but misses!@n", "@c$n@W fires a tsuihidan at @C$N@W, but somehow misses!@n", true},
    .hits           = {{"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $s chest!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your chest!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's chest!@n", true}, {"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $s face!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your face!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's face!@n", true}, {"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $s gut!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your gut!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's gut!@n", true}, {"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $s arm!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your arm!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's arm!@n", true}, {"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $s leg!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your leg!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's leg!@n", true}},
    .obj_to_char    = "@WYou fire a tsuihidan at $p@W!@n",
    .obj_to_room    = "@C$n@W fires a tsuihidan at $p@W!@n",
    .obj_hide       = true,
    .steady         = tsuihidan_mastery,
    .after_hit      = tsuihidan_sap,
};
/* left out of the table: */
/*   "@CYour tsuihidan hits a vital spot and seems to sap some of @c$N's@C stamina!@n" */
/*   "@C$n's@C tsuihidan hits a vital spot and saps some of your stamina!@n" */
/*   "@C$n's@C tsuihidan hits a vital spot and saps some of @c$N's@C stamina!" */

} // namespace

ACMD(do_pbarrage) { perform_ki_attack(g, ch, argument, PSYCHIC_BARRAGE); }

ACMD(do_scatter) { perform_ki_attack(g, ch, argument, SCATTER_SHOT); }

ACMD(do_kakusanha)
{
  int perc, dge = 2, count = 0, skill;
  int64_t dmg;
  double attperc = .3, minimum = .10;
  std::string arg2;

  std::string_view rest = argument;
  arg2                  = take_arg(rest);

  /* Can they do the technique? */

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_KAKUSANHA)) {
    return;
  }

  if (!limb_ok(g, ch, 0)) {
    return;
  }

  if (!tech_handle_charge(g, ch, arg2, minimum, &attperc)) return;

  if (getMaxKI(g, ch) * attperc > char_get_charge(ch)) {
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(g, ch));
  }

  if (!check_points(g, ch, getMaxKI(g, ch) * minimum, 0)) {
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(g, ch, SKILL_KAKUSANHA); /* Set skill value */

  for (char_data *vict : room_people(g, char_get_in_room(ch))) {
    if (vict == ch) {
      continue;
    }
    if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
      continue;
    }
    if (getCurPL(g, vict) <= 0) {
      continue;
    }
    if (char_aff_flagged(vict, AFF_GROUP) && !char_is_npc(vict)) {
      if (char_get_master(g, vict) == ch) {
        continue;
      }
      if (char_get_master(g, ch) == vict) {
        continue;
      }
      if (char_get_master(g, vict) == char_get_master(g, ch)) {
        continue;
      }
    }
    if (char_mob_flagged(vict, MOB_NOKILL)) {
      continue;
    }
    if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
      continue;
    }
    count += 1;
  }
  if (count <= 0) {
    send_to_char(g, ch, "There is no one worth targeting around.\r\n");
    return;
  } /* No one worth targeting */
  else {
    handle_cooldown(g, ch, 5);
    int hits = 0;
    perc     = chance_to_hit(g, ch);

    if (skill < perc) {
      act(g, "@WYou pour your charged ki into your hands and bring them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of your palms. You fire one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before you swing your arms upward and the beam follows suit. The beam flies off harmlessly through the air as you lose control over it!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@W pours $s charged ki into $s hands and brings them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of $s palms. @C$n@W fires one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before $e swings $s arms upward and the beam follows suit. The beam flies off harmlessly through the air as $e loses control over it!@n", true, ch, nullptr, nullptr, TO_ROOM);
      pcost(g, ch, attperc, 0);

      improve_skill(g, ch, SKILL_KAKUSANHA, 0);
      return;
    }

    act(g, "@WYou pour your charged ki into your hands and bring them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of your palms. You fire one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before you swing your arms upward and the beam follows suit. Above your targets the beam breaks apart into five seperate pieces that follow their victims!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@W pours $s charged ki into $s hands and brings them both forward quickly. @yG@Yo@Wl@wden@W orbs of energy form at the extent of $s palms. @C$n@W fires one massive beam of @yg@Yo@Wl@wden @Wenergy from combining both orbs. The beam flies forward a short distance before $e swings $s arms upward and the beam follows suit. Above you the beam breaks apart into five seperate pieces that follow their victims!@n", true, ch, nullptr, nullptr, TO_ROOM);

    dmg = damtype(g, ch, 34, skill, attperc);
    if (count >= 3) {
      dmg = (dmg / 100) * 40;
    } else if (count > 1) {
      dmg = (dmg / 100) * 60;
    }

    for (char_data *vict : room_people(g, char_get_in_room(ch))) {
      if (vict == ch) {
        continue;
      }
      if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
        continue;
      }
      if (char_aff_flagged(vict, AFF_GROUP) && (char_get_master(g, vict) == ch || char_get_master(g, ch) == vict)) {
        continue;
      }
      if (char_mob_flagged(vict, MOB_NOKILL)) {
        continue;
      }
      if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
        continue;
      }
      if (getCurPL(g, vict) <= 0) {
        continue;
      }
      if (hits >= 5) {
        continue;
      }
      dge = handle_dodge(g, vict);
      if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
          (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
        hits++;
        act(g, "@C$N@c disappears, avoiding the beam chasing $M!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@cYou disappear, avoiding the beam chasing you!@n", false, ch, nullptr, vict, TO_VICT);
        act(g, "@C$N@c disappears, avoiding the beam chasing $M!@n", false, ch, nullptr, vict, TO_NOTVICT);
        char_aff_set(vict, AFF_ZANZOKEN, false);
        pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
        continue;
      }
      if (dge + rand_number(g, -10, 5) > skill) {
        hits++;
        act(g, "@c$N@W manages to escape the attack!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@WYou manage to escape the attack!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$N@W manages to escape the attack!@n", true, ch, nullptr, vict, TO_NOTVICT);
        improve_skill(g, vict, SKILL_DODGE, 0);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
        continue;
      }
      hits++;
      act(g, "@R$N@r is slammed by one of the beams!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@RYou are slammed by one of the beams!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@R$N@r is slammed by one of the beams!@n", true, ch, nullptr, vict, TO_NOTVICT);
      hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
      continue;
    } /* Hitting targets! */

    if (count < 5 && !room_flagged(g, char_get_in_room(ch), ROOM_SPACE)) {
      send_to_room(g, char_get_in_room(ch), "The rest of the beams slam into the ground!@n\r\n");
      send_to_room(g, char_get_in_room(ch), "@wBright explosions erupt from the impacts!\r\n");

      if (room_sect(g, char_get_in_room(ch)) != SECT_INSIDE) {
        impact_sound(g, ch, "@wA loud roar is heard nearby!@n\r\n");
        switch (rand_number(g, 1, 8)) {
          case 1:
            act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 2:
            if (rand_number(g, 1, 4) == 4 && room_get_geffect(g, char_get_in_room(ch)) == 0) {
              room_set_geffect(g, char_get_in_room(ch), 5);
              act(g, "Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!", true, ch, nullptr, nullptr, TO_CHAR);
              act(g, "Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!", true, ch, nullptr, nullptr, TO_ROOM);
            }
            break;
          case 3:
            act(g, "A cloud of dust envelopes the entire area!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "A cloud of dust envelopes the entire area!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 4:
            act(g, "The surrounding area roars and shudders from the impact!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "The surrounding area roars and shudders from the impact!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 5:
            act(g, "The ground shatters apart from the stress of the impact!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "The ground shatters apart from the stress of the impact!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 6:
            act(g, "The explosion continues to burn spreading out and devouring some more of the ground before dying out.", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "The explosion continues to burn spreading out and devouring some more of the ground before dying out.", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          default:
            /* we want no message for the default */
            break;
        }
      }
      if (room_sect(g, char_get_in_room(ch)) == SECT_UNDERWATER) {
        switch (rand_number(g, 1, 3)) {
          case 1:
            act(g, "The water churns violently!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "The water churns violently!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 2:
            act(g, "Large bubbles rise from the movement!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "Large bubbles rise from the movement!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 3:
            act(g, "The water collapses in on the hole created!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "The water collapses in on the hole create!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
        }
      }
      if (room_sect(g, char_get_in_room(ch)) == SECT_WATER_SWIM || room_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM) {
        switch (rand_number(g, 1, 3)) {
          case 1:
            act(g, "A huge column of water erupts from the impact!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "A huge column of water erupts from the impact!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 2:
            act(g, "The impact briefly causes a swirling vortex of water!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "The impact briefly causes a swirling vortex of water!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 3:
            act(g, "A huge depression forms in the water and erupts into a wave from the impact!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "A huge depression forms in the water and erupts into a wave from the impact!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
        }
      }
      if (room_sect(g, char_get_in_room(ch)) == SECT_INSIDE) {
        impact_sound(g, ch, "@wA loud roar is heard nearby!@n\r\n");
        switch (rand_number(g, 1, 8)) {
          case 1:
            act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "Debris is thrown into the air and showers down thunderously!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 2:
            act(g, "The structure of the surrounding room cracks and quakes from the blast!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "The structure of the surrounding room cracks and quakes from the blast!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 3:
            act(g, "Parts of the ceiling collapse, crushing into the floor!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "Parts of the ceiling collapse, crushing into the floor!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 4:
            act(g, "The surrounding area roars and shudders from the impact!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "The surrounding area roars and shudders from the impact!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 5:
            act(g, "The ground shatters apart from the stress of the impact!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "The ground shatters apart from the stress of the impact!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          case 6:
            act(g, "The walls of the surrounding room crack in the same instant!", true, ch, nullptr, nullptr, TO_CHAR);
            act(g, "The walls of the surrounding room crack in the same instant!", true, ch, nullptr, nullptr, TO_ROOM);
            break;
          default:
            /* we want no message for the default */
            break;
        }
      }

      if (room_get_dmg(g, char_get_in_room(ch)) <= (100 - ((5 - count) * 5))) {
        room_set_dmg(g, char_get_in_room(ch), room_get_dmg(g, char_get_in_room(ch)) + ((5 - count) * 5));
      }
    }

    pcost(g, ch, attperc, 0);

    improve_skill(g, ch, SKILL_KAKUSANHA, 0);
    handle_cooldown(g, ch, 5);
    return;
  } /* We have targets! Attempt to kills them! */
}

ACMD(do_renzo) { perform_ki_attack(g, ch, argument, RENZOKOU); }

ACMD(do_tsuihidan) { perform_ki_attack(g, ch, argument, TSUIHIDAN); }
