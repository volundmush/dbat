/* special_beams.c: special beam attacks
 * Split from act.offensive.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/args.hpp"
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

/* A honoo's flames: a ruby helps them, and fireproof or fire-prone victims take less or more; the victim is ashed
 * until it is over */
int honoo_flames(ki_shot &s, int hitspot)
{
  if (check_ruby(s.g, s.ch))
    s.dmg += s.dmg * 0.2;
  if (char_get_bonuses(s.vict, BONUS_FIREPROOF))
    s.dmg -= s.dmg * 0.4;
  else if (char_get_bonuses(s.vict, BONUS_FIREPRONE))
    s.dmg += s.dmg * 0.4;
  char_aff_set(s.vict, AFF_ASHED, true);
  return hitspot;
}

/* ... and can burn them */
void honoo_burn(ki_shot &s)
{
  tech_handle_burn(s.g, s.ch, s.vict, 3);
}

/* A honoo dries up some of the water flooding the room */
void honoo_evaporate(ki_shot &s)
{
  room_vnum here = char_get_in_room(s.ch);

  if (room_get_geffect(s.g, here) < -1) {
    send_to_room(s.g, here, "The water surrounding the area evaporates some!\r\n");
    room_set_geffect(s.g, here, room_get_geffect(s.g, here) + 1);
  } else if (room_get_geffect(s.g, here) == -1) {
    send_to_room(s.g, here, "The water surrounding the area evaporates completely away!\r\n");
    room_set_geffect(s.g, here, 0);
  }
}

/* It does so as the attacker pays for a hit or a miss of someone spent... */
void honoo_paid(ki_shot &s, ki_outcome how)
{
  if (how == ki_outcome::spent)
    honoo_evaporate(s);
  else if (how == ki_outcome::hit) {
    honoo_evaporate(s);
    char_aff_set(s.vict, AFF_ASHED, false);
  }
}

/* ... and at the end of the rest */
void honoo_finish(ki_shot &s, ki_outcome how)
{
  if (how != ki_outcome::spent && how != ki_outcome::hit)
    honoo_evaporate(s);
}

/* It burns object 79 three times as hard */
bool honoo_at_object(ki_shot &s)
{
  GameWorld *g = s.g;

  s.dmg = damtype(g, s.ch, 21, s.skill, s.attperc) / 10;
  if (obj_get_vnum(g, s.obj) == 79)
    s.dmg *= 3;
  act(g, s.attack->obj_to_char, false, s.ch, s.obj, nullptr, TO_CHAR);
  act(g, s.attack->obj_to_room, false, s.ch, s.obj, nullptr, TO_ROOM);
  hurt(g, 0, 0, s.ch, nullptr, s.obj, s.dmg, 0);
  pcost(g, s.ch, s.attperc, 0);
  return true;
}

/* A psychic blast can drain the victim's charge, and shock their mind */
void psychic_blast_mind(ki_shot &s)
{
  GameWorld *g           = s.g;
  struct char_data *vict = s.vict;

  if (char_get_charge(vict) > 0 && rand_number(g, 1, 3) == 2) {
    char_set_charge(vict, char_get_charge(vict) - (s.dmg / 5));
    if (char_get_charge(vict) < 0)
      char_set_charge(vict, 0);
    send_to_char(g, vict, "@RYou lose some of your charged ki!@n\r\n");
  }
  if (!char_aff_flagged(vict, AFF_SHOCKED) && rand_number(g, 1, 4) == 4 && !char_aff_flagged(vict, AFF_SANCTUARY)) {
    act(g, "@MYour mind has been shocked!@n", true, vict, nullptr, nullptr, TO_CHAR);
    act(g, "@M$n@m's mind has been shocked!@n", true, vict, nullptr, nullptr, TO_ROOM);
    char_aff_set(vict, AFF_SHOCKED, true);
  }
}

/* Kousengan and the deathbeam are hard to get away from */
void sure_shot(ki_shot &, int &prob)
{
  prob += 15;
}

/* A kousengan hits a victim under sanctuary harder */
void kousengan_sanctuary(ki_shot &s)
{
  if (char_aff_flagged(s.vict, AFF_SANCTUARY))
    s.dmg *= calc_critical(s.g, s.ch, 1);
}

void kousengan_burn(ki_shot &s)
{
  if (!char_aff_flagged(s.vict, AFF_BURNED) && rand_number(s.g, 1, 4) == 3 && !char_is_demon(s.vict)) {
    send_to_char(s.g, s.vict, "@RYou are burned by the attack!@n\r\n");
    send_to_char(s.g, s.ch, "@RThey are burned by the attack!@n\r\n");
    char_aff_set(s.vict, AFF_BURNED, true);
  }
}

/* A deathbeam takes some of a living victim's life force, the more the better its user knows it */
void deathbeam_life_force(ki_shot &s)
{
  int skill = char_get_skill(s.ch, SKILL_DEATHBEAM);

  if (getCurPL(s.g, s.vict) < 2)
    return;
  if (skill >= 100)
    decCurLF(s.g, s.vict, s.dmg * .4, -1);
  else if (skill >= 60)
    decCurLF(s.g, s.vict, s.dmg * .2, -1);
  else if (skill >= 40)
    decCurLF(s.g, s.vict, s.dmg * .05, -1);
}

/* Perfection 3 makes a deathbeam leave its user waiting */
void deathbeam_wait(ki_shot &s, ki_outcome)
{
  if (char_get_skills_perf(s.ch, SKILL_DEATHBEAM) == 3)
    char_set_wait(s.ch, PULSE_3SEC);
}

const ki_attack HONOO = {
    .skill        = SKILL_HONOO,
    .name         = "Honoo",
    .damtype      = 21,
    .attperc      = .125,
    .minimum      = .1,
    .cooldown     = 6,
    .needs_arms   = false,
    .perfection   = true,
    .absorb       = ki_absorb::costs_one,
    .block        = {"@C$N@W moves quickly and blocks your honoo!@n", "@WYou move quickly and block @C$n's@W honoo!@n", "@C$N@W moves quickly and blocks @c$n's@W honoo!@n"},
    .dodge        = {"@C$N@W manages to dodge your honoo, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W honoo, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W honoo, letting it slam into the surroundings!@n"},
    .miss         = {"@WYou can't believe it but your honoo misses, flying through the air harmlessly!@n", "@C$n@W fires a honoo at you, but misses!@n ", "@c$n@W fires a honoo at @C$N@W, but somehow misses!@n "},
    .miss_spent   = {"@WYou can't believe it but your honoo misses, flying through the air harmlessly!@n", "@C$n@W fires a honoo at you, but misses!@n", "@c$n@W fires a honoo at @C$N@W, but somehow misses!@n"},
    .hits         = {{"@WYou gather your charged ki and bring it up into your throat while mixing it with the air in your lungs. You grin evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from your lips! @c$N@W's body is engulfed!@n", "@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at you before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! YOUR body is engulfed!@n", "@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! @c$N@W's body is engulfed!@n", true}, {"@WYou gather your charged ki and bring it up into your throat while mixing it with the air in your lungs. You grin evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from your lips! @c$N@W's face is engulfed!@n", "@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at you before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! YOUR face is engulfed!@n", "@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! @c$N@W's face is engulfed!@n", true}, {"@WYou gather your charged ki and bring it up into your throat while mixing it with the air in your lungs. You grin evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from your lips! @c$N@W's gut is engulfed!@n", "@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at you before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! YOUR gut is engulfed!@n", "@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! @c$N@W's gut is engulfed!@n", true}, {"@WYou gather your charged ki and bring it up into your throat while mixing it with the air in your lungs. You grin evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from your lips! @c$N@W's arm is engulfed!@n", "@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at you before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! YOUR arm is engulfed!@n", "@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! @c$N@W's arm is engulfed!@n", true}, {"@WYou gather your charged ki and bring it up into your throat while mixing it with the air in your lungs. You grin evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from your lips! @c$N@W's leg is engulfed!@n", "@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at you before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! YOUR leg is engulfed!@n", "@C$n@W gathers $s charged ki and brings it up into $s throat while mixing it with the air in $s lungs. $e grins evily at @c$N@W before unleashing a massive jet of @rf@Rl@Ya@rm@Re@Ys@W from $s lips! @c$N@W's leg is engulfed!@n", true}},
    .obj_to_char  = "@WYou fire a honoo at $p@W!@n",
    .obj_to_room  = "@C$n@W fires a honoo at $p@W!@n",
    .aim          = honoo_flames,
    .after_hit    = honoo_burn,
    .after_paying = honoo_paid,
    .on_object    = honoo_at_object,
    .finish       = honoo_finish,
};

const ki_attack PSYCHIC_BLAST = {
    .skill       = SKILL_PSYBLAST,
    .name        = "Psychic Blast",
    .damtype     = 20,
    .attperc     = .125,
    .minimum     = .1,
    .cooldown    = 6,
    .perfection  = true,
    .absorb      = ki_absorb::costs_one,
    .block       = {"@C$N@W moves quickly and blocks your psychic blast!@n", "@WYou move quickly and block @C$n's@W psychic blast!@n", "@C$N@W moves quickly and blocks @c$n's@W psychic blast!@n"},
    .dodge       = {"@C$N@W manages to dodge your psychic blast, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W psychic blast, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W psychic blast, letting it slam into the surroundings!@n"},
    .miss        = {"@WYou can't believe it but your psychic blast misses, flying through the air harmlessly!@n", "@C$n@W fires a psychic blast at you, but misses!@n ", "@c$n@W fires a psychic blast at @C$N@W, but somehow misses!@n "},
    .miss_spent  = {"@WYou can't believe it but your psychic blast misses, flying through the air harmlessly!@n", "@C$n@W fires a psychic blast at you, but misses!@n", "@c$n@W fires a psychic blast at @C$N@W, but somehow misses!@n"},
    .hits        = {{"@WYou gather your charged ki into your brain as a flash of @bb@Bl@wue@W light shoots from your forehead and slams into @c$N@W's body! $E screams for a moment as terrifying images sear through $S mind!@n", "@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into YOUR body! You scream for a moment as terrifying images sear through your mind!@n", "@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into @c$N@W's body! $E screams for a moment as terrifying images sear through $S mind!@n", true}, {"@WYou gather your charged ki into your brain as a flash of @bb@Bl@wue@W light shoots from your forehead and slams into @c$N@W's head! $E screams for a moment as terrifying images sear through $S mind!@n", "@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into YOUR head! You scream for a moment as terrifying images sear through your mind!@n", "@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into @c$N@W's head! $E screams for a moment as terrifying images sear through $S mind!@n", true}, {"@WYou gather your charged ki into your brain as a flash of @bb@Bl@wue@W light shoots from your forehead and slams into @c$N@W's gut! $E screams for a moment as terrifying images sear through $S mind!@n", "@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into YOUR gut! You scream for a moment as terrifying images sear through your mind!@n", "@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into @c$N@W's gut! $E screams for a moment as terrifying images sear through $S mind!@n", true}, {"@WYou gather your charged ki into your brain as a flash of @bb@Bl@wue@W light shoots from your forehead and slams into @c$N@W's arm! $E screams for a moment as terrifying images sear through $S mind!@n", "@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into YOUR arm! You scream for a moment as terrifying images sear through your mind!@n", "@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into @c$N@W's arm! $E screams for a moment as terrifying images sear through $S mind!@n", true}, {"@WYou gather your charged ki into your brain as a flash of @bb@Bl@wue@W light shoots from your forehead and slams into @c$N@W's leg! $E screams for a moment as terrifying images sear through $S mind!@n", "@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into YOUR leg! You scream for a moment as terrifying images sear through your mind!@n", "@C$n@W gathers $s charged ki into $s brain as a flash of @bb@Bl@wue@W light shoots from $s forehead and slams into @c$N@W's leg! $E screams for a moment as terrifying images sear through $S mind!@n", true}},
    .obj_to_char = "@WYou fire a psychic blast at $p@W!@n",
    .obj_to_room = "@C$n@W fires a psychic blast at $p@W!@n",
    .after_hit   = psychic_blast_mind,
};
/* left out of the table: */
/*   "@MYour mind has been shocked!@n" */
/*   "@M$n@m's mind has been shocked!@n" */

const ki_attack SPECIAL_BEAM_CANNON = {
    .skill         = SKILL_SBC,
    .name          = "Special Beam Cannon",
    .damtype       = 27,
    .attperc       = .15,
    .minimum       = .05,
    .cooldown      = 7,
    .dodge_dmg_cap = 90,
    .dodge_dmg     = 10,
    .limb_hurt     = 180,
    .blockable     = false,
    .dodge         = {"@C$N@W manages to dodge your special beam cannon, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W special beam cannon, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W special beam cannon, letting it slam into the surroundings!@n"},
    .miss          = {"@WYou can't believe it but your special beam cannon misses, flying through the air harmlessly!@n", "@C$n@W fires a special beam cannon at you, but misses!@n ", "@c$n@W fires a special beam cannon at @C$N@W, but somehow misses!@n "},
    .miss_spent    = {"@WYou can't believe it but your special beam cannon misses, flying through the air harmlessly!@n", "@C$n@W fires a special beam cannon at you, but misses!@n", "@c$n@W fires a special beam cannon at @C$N@W, but somehow misses!@n"},
    .hits          = {{"@WYou place your index and middle fingers against your forehead and pool your charged ki there. Sparks and light surround your fingertips as the technique becomes ready. You point your fingers at @c$N@W and yell '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from your fingers and slams into $S body!@n", "@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at YOU and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into YOUR body!@n", "@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at @c$N@W and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into @c$N@W's body!@n", true}, {"@WYou place your index and middle fingers against your forehead and pool your charged ki there. Sparks and light surround your fingertips as the technique becomes ready. You point your fingers at @c$N@W and yell '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from your fingers and slams into $S head!@n", "@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at YOU and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into YOUR head!@n", "@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at @c$N@W and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into @c$N@W's head!@n", true}, {"@WYou place your index and middle fingers against your forehead and pool your charged ki there. Sparks and light surround your fingertips as the technique becomes ready. You point your fingers at @c$N@W and yell '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from your fingers and slams into $S gut!@n", "@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at YOU and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into YOUR gut!@n", "@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at @c$N@W and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into @c$N@W's gut!@n", true}, {"@WYou place your index and middle fingers against your forehead and pool your charged ki there. Sparks and light surround your fingertips as the technique becomes ready. You point your fingers at @c$N@W and yell '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from your fingers and slams into $S arm!@n", "@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at YOU and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into YOUR arm!@n", "@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at @c$N@W and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into @c$N@W's arm!@n", true}, {"@WYou place your index and middle fingers against your forehead and pool your charged ki there. Sparks and light surround your fingertips as the technique becomes ready. You point your fingers at @c$N@W and yell '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from your fingers and slams into $S leg!@n", "@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at YOU and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into YOUR leg!@n", "@C$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @C$n@W points $s fingers at @c$N@W and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into @c$N@W's leg!@n", true}},
    .obj_to_char   = "@WYou fire a special beam cannon at $p@W!@n",
    .obj_to_room   = "@C$n@W fires a special beam cannon at $p@W!@n",
};

const ki_attack KOUSENGAN = {
    .skill             = SKILL_KOUSENGAN,
    .name              = "kousengan",
    .damtype           = 42,
    .attperc           = .05,
    .minimum           = .01,
    .cooldown          = 5,
    .not_while_singing = true,
    .needs_arms        = false,
    .powerhit_crit     = true,
    .dodge_damtype     = 17,
    .targets_first     = true,
    .block             = {"@C$N@W moves quickly and blocks your kousengan!@n", "@WYou move quickly and block @C$n's@W kousengan!@n", "@C$N@W moves quickly and blocks @c$n's@W kousengan!@n"},
    .dodge             = {"@C$N@W manages to dodge your kousengan, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W kousengan, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W kousengan, letting it slam into the surroundings!@n"},
    .miss              = {"@WYou can't believe it but your kousengan misses, flying through the air harmlessly!@n", "@C$n@W fires a kousengan at you, but misses!@n ", "@c$n@W fires a kousengan at @C$N@W, but somehow misses!@n "},
    .miss_spent        = {"@WYou can't believe it but your kousengan misses, flying through the air harmlessly!@n", "@C$n@W fires a kousengan at you, but misses!@n", "@c$n@W fires a kousengan at @C$N@W, but somehow misses!@n"},
    .hits              = {{"@WYou look at @c$N@W and grin. Then bright @Mpink@W lasers shoot from your eyes and slam into $S chest!@n", "@C$n@W looks at YOU and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into YOUR chest!@n", "@C$n@W looks at @c$N@W and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into @c$N@W's chest!@n", true}, {"@WYou look at @c$N@W and grin. Then bright @Mpink@W lasers shoot from your eyes and slam into $S face!@n", "@C$n@W looks at YOU and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into YOUR face!@n", "@C$n@W looks at @c$N@W and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into @c$N@W's face!@n", true}, {"@WYou look at @c$N@W and grin. Then bright @Mpink@W lasers shoot from your eyes and slam into $S gut!@n", "@C$n@W looks at YOU and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into YOUR gut!@n", "@C$n@W looks at @c$N@W and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into @c$N@W's gut!@n", true}, {"@WYou look at @c$N@W and grin. Then bright @Mpink@W lasers shoot from your eyes and slam into $S arm!@n", "@C$n@W looks at YOU and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into YOUR arm!@n", "@C$n@W looks at @c$N@W and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into @c$N@W's arm!@n", true}, {"@WYou look at @c$N@W and grin. Then bright @Mpink@W lasers shoot from your eyes and slam into $S leg!@n", "@C$n@W looks at YOU and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into YOUR leg!@n", "@C$n@W looks at @c$N@W and grins. Then bright @Mpink@W lasers shoot from $s eyes and slam into @c$N@W's leg!@n", true}},
    .obj_to_char       = "@WYou fire a kousengan at $p@W!@n",
    .obj_to_room       = "@C$n@W fires a kousengan at $p@W!@n",
    .steady            = sure_shot,
    .bonus_damage      = kousengan_sanctuary,
    .after_hit         = kousengan_burn,
};

const ki_attack DEATHBEAM = {
    .skill        = SKILL_DEATHBEAM,
    .name         = "Deathbeam",
    .damtype      = 17,
    .attperc      = .1,
    .minimum      = .05,
    .cooldown     = 5,
    .perfection   = true,
    .block        = {"@C$N@W moves quickly and blocks your deathbeam!@n", "@WYou move quickly and block @C$n's@W deathbeam!@n", "@C$N@W moves quickly and blocks @c$n's@W deathbeam!@n"},
    .dodge        = {"@C$N@W manages to dodge your deathbeam, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W deathbeam, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W deathbeam, letting it slam into the surroundings!@n"},
    .miss         = {"@WYou can't believe it but your deathbeam misses, flying through the air harmlessly!@n", "@C$n@W fires a deathbeam at you, but misses!@n ", "@c$n@W fires a deathbeam at @C$N@W, but somehow misses!@n "},
    .miss_spent   = {"@WYou can't believe it but your deathbeam misses, flying through the air harmlessly!@n", "@C$n@W fires a deathbeam at you, but misses!@n", "@c$n@W fires a deathbeam at @C$N@W, but somehow misses!@n"},
    .hits         = {{"@WYou move swiftly, drawing your charged ki to your index finger, and point at @c$N@W! You fire a @Rred@W Deathbeam from your finger which slams into $S body and explodes!@n", "@C$n@W moves swiftly, drawing charged ki to $s index finger, and point at you@W! $e fires a @Rred@W Deathbeam from $s finger which slams into your body and explodes!@n", "@C$n@W moves swiftly, drawing charged ki to $s index finger, and point at @c$N@W! $e fires a @Rred@W Deathbeam from $s finger which slams into @c$N@W's body and explodes!@n", true}, {"@WYou move swiftly, drawing your charged ki to your index finger, and point at @c$N@W! You fire a @Rred@W Deathbeam from your finger which slams into $S face and explodes!@n", "@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at you@W! $e fires a @Rred@W Deathbeam from $s finger which slams into your face and explodes!@n", "@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at @c$N@W! $e fires a @Rred@W Deathbeam from $s finger which slams into @c$N@W's face and explodes!@n", true}, {"@WYou move swiftly, drawing your charged ki to your index finger, and point at @c$N@W! You fire a @Rred@W Deathbeam from your finger which slams into $S gut and explodes!@n", "@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at you@W! $e fires a @Rred@W Deathbeam from $s finger which slams into your gut and explodes!@n", "@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at @c$N@W! $e fires a @Rred@W Deathbeam from $s finger which slams into @c$N@W's gut and explodes!@n", true}, {"@WYou move swiftly, drawing your charged ki to your index finger, and point at @c$N@W! You fire a @Rred@W Deathbeam from your finger which slams into $S arm and explodes!@n", "@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at you@W! $e fires a @Rred@W Deathbeam from $s finger which slams into your arm and explodes!@n", "@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at @c$N@W! $e fires a @Rred@W Deathbeam from $s finger which slams into @c$N@W's arm and explodes!@n", true}, {"@WYou move swiftly, drawing your charged ki to your index finger, and point at @c$N@W! You fire a @Rred@W Deathbeam from your finger which slams into $S leg and explodes!@n", "@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at you@W! $e fires a @Rred@W Deathbeam from $s finger which slams into your leg and explodes!@n", "@C$n@W moves swiftly, drawing charged ki to $s index finger, and points at @c$N@W! $e fires a @Rred@W Deathbeam from $s finger which slams into @c$N@W's leg and explodes!@n", true}},
    .obj_to_char  = "@WYou fire a deathbeam at $p@W!@n",
    .obj_to_room  = "@C$n@W fires a deathbeam at $p@W!@n",
    .steady       = sure_shot,
    .after_hit    = deathbeam_life_force,
    .after_paying = deathbeam_wait,
};

} // namespace

ACMD(do_honoo) { perform_ki_attack(g, ch, argument, HONOO); }

ACMD(do_psyblast) { perform_ki_attack(g, ch, argument, PSYCHIC_BLAST); }

ACMD(do_sbc) { perform_ki_attack(g, ch, argument, SPECIAL_BEAM_CANNON); }

ACMD(do_kousengan) { perform_ki_attack(g, ch, argument, KOUSENGAN); }

ACMD(do_deathbeam) { perform_ki_attack(g, ch, argument, DEATHBEAM); }
