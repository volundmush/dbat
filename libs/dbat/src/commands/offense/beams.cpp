/* beams.c: ki beam attacks
 * Split from act.offensive.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/act.offensive.hpp"
#include "dbat/consts/directions.hpp"
#include "dbat/consts/skills.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/random.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/combat.hpp"
#include "dbat/comm.hpp"
#include "ki_attack.hpp"

namespace {

/* Tribeam takes some of the user's life force into the blast, more the better they know it */
void tribeam_life_force(ki_shot &s)
{
  if (char_get_skill(s.ch, SKILL_TRIBEAM) >= 100) {
    s.dmg += (getMaxLF(s.g, s.ch)) * 0.20;
  } else if (char_get_skill(s.ch, SKILL_TRIBEAM) >= 60) {
    s.dmg += (getMaxLF(s.g, s.ch)) * 0.10;
  } else if (char_get_skill(s.ch, SKILL_TRIBEAM) >= 40) {
    s.dmg += (getMaxLF(s.g, s.ch)) * 0.05;
  }
}

/* and staggers a player it hits */
void tribeam_stun(ki_shot &s)
{
  if (!char_is_npc(s.vict)) {
    char_set_wait(s.vict, PULSE_2SEC);
  }
}

/* A dodonpa sometimes drains the victim's ki */
void dodonpa_drain_ki(ki_shot &s)
{
  if (rand_number(s.g, 1, 3) == 2) {
    decCurKI(s.g, s.vict, s.dmg / 4, 0);
    send_to_char(s.g, s.vict, "@RYou feel some of your ki drained away by the attack!@n\r\n");
  }
}

/* Perfection 3 makes a dodonpa at someone leave its user waiting */
void dodonpa_wait(ki_shot &s, ki_outcome how)
{
  if (how != ki_outcome::object && char_get_skills_perf(s.ch, SKILL_DODONPA) == 3) {
    char_set_wait(s.ch, PULSE_3SEC);
  }
}

void masenko_bonus(ki_shot &s)
{
  /* quirk: the three tiers all test for 100, so only the first is ever used */
  if (char_get_skill(s.ch, SKILL_MASENKO) >= 100) {
    s.dmg += s.dmg * 0.08;
  }
  if (char_is_piccolo(s.ch)) {
    if (char_get_skills_base(s.ch, SKILL_STYLE) >= 100)
      s.dmg += (getMaxKI(s.g, s.ch) * 0.08);
    else if (char_get_skills_base(s.ch, SKILL_STYLE) >= 60)
      s.dmg += (getMaxKI(s.g, s.ch) * 0.05);
    else if (char_get_skills_base(s.ch, SKILL_STYLE) >= 40)
      s.dmg += (getMaxKI(s.g, s.ch) * 0.03);
  }
}

/* A masenko that hits sometimes tires the victim */
void masenko_drain_stamina(ki_shot &s)
{
  if (rand_number(s.g, 1, 2) == 2 && !char_aff_flagged(s.vict, AFF_SANCTUARY)) {
    send_to_char(s.g, s.vict, "@RThe attack seems to have taken a toll on your stamina!@n\r\n");
    decCurST(s.g, s.vict, s.dmg / 4, 0);
  }
}

/* Knowing the kamehameha well gives back some of the ki it took; perfection 3 leaves its user waiting */
void kamehameha_refund(ki_shot &s, ki_outcome how)
{
  if (how == ki_outcome::absorbed)
    return;
  if (char_get_skill(s.ch, SKILL_KAMEHAMEHA) >= 100) {
    incCurKI(s.g, s.ch, (getMaxKI(s.g, s.ch) * s.attperc) * 0.25, true);
  } else if (char_get_skill(s.ch, SKILL_KAMEHAMEHA) >= 60) {
    incCurKI(s.g, s.ch, (getMaxKI(s.g, s.ch) * s.attperc) * 0.1, true);
  } else if (char_get_skill(s.ch, SKILL_KAMEHAMEHA) >= 40) {
    incCurKI(s.g, s.ch, (getMaxKI(s.g, s.ch) * s.attperc) * 0.05, true);
  }
  if (char_get_skills_perf(s.ch, SKILL_KAMEHAMEHA) == 3) {
    char_set_wait(s.ch, PULSE_3SEC);
  }
}

const ki_attack GALIKGUN = {
    .skill          = SKILL_GALIKGUN,
    .name           = "Galik Gun",
    .damtype        = 16,
    .attperc        = .15,
    .minimum        = .1,
    .cooldown       = 6,
    .perfection     = true,
    .absorb         = ki_absorb::costs_one,
    .block_improves = true,
    .multihit       = true,
    .block          = {"@C$N@W moves quickly and blocks your Galik Gun!@n", "@WYou move quickly and block @C$n's@W Galik Gun!@n", "@C$N@W moves quickly and blocks @c$n's@W Galik Gun!@n"},
    .dodge          = {"@C$N@W manages to dodge your Galik Gun, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W Galik Gun, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W Galik Gun, letting it slam into the surroundings!@n"},
    .miss           = {"@WYou can't believe it but your Galik Gun misses, flying through the air harmlessly!@n", "@C$n@W fires a Galik Gun at you, but misses!@n ", "@c$n@W fires a Galik Gun at @C$N@W, but somehow misses!@n "},
    .miss_spent     = {"@WYou can't believe it but your Galik Gun misses, flying through the air harmlessly!@n", "@C$n@W fires a Galik Gun at you, but misses!@n", "@c$n@W fires a Galik Gun at @C$N@W, but somehow misses!@n"},
    .hits           = {{"@WYou take your charged ki and form a sparkling purple shroud of energy around your body! You swing your arms towards @c$N@W with your palms flatly facing $M and shout '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around your body forms into a beam and crashes into $S body!@n", "@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards you with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into your body!@n", "@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards @c$N@W with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into @c$N@W's body!@n"}, {"@WYou take your charged ki and form a sparkling purple shroud of energy around your body! You swing your arms towards @c$N@W with your palms flatly facing $M and shout '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around your body forms into a beam and crashes into $S face!@n", "@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards you with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into your face!@n", "@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards @c$N@W with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into @c$N@W's face!@n"}, {"@WYou take your charged ki and form a sparkling purple shroud of energy around your body! You swing your arms towards @c$N@W with your palms flatly facing $M and shout '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around your body forms into a beam and crashes into $S gut!@n", "@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards you with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into your gut!@n", "@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards @c$N@W with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into @c$N@W's gut!@n"}, {"@WYou take your charged ki and form a sparkling purple shroud of energy around your body! You swing your arms towards @c$N@W with your palms flatly facing $M and shout '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around your body forms into a beam and crashes into $S arm!@n", "@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards you with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into your arm!@n", "@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards @c$N@W with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into @c$N@W's arm!@n"}, {"@WYou take your charged ki and form a sparkling purple shroud of energy around your body! You swing your arms towards @c$N@W with your palms flatly facing $M and shout '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around your body forms into a beam and crashes into $S leg!@n", "@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards you with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into your leg!@n", "@C$n@W takes $s charged ki and forms a sparkling purple shroud of energy around $mself! $e swings $s arms towards @c$N@W with palms facing out flatly and shouts '@mG@Mal@wik @mG@Mu@wn@W!' as the energy around $s body forms into a beam and crashes into @c$N@W's leg!@n"}},
    .obj_to_char    = "@WYou fire a Galik Gun at $p@W!@n",
    .obj_to_room    = "@C$n@W fires a Galik Gun at $p@W!@n",
};

const ki_attack FINAL_FLASH = {
    .skill         = SKILL_FINALFLASH,
    .name          = "Final Flash",
    .damtype       = 28,
    .attperc       = .2,
    .minimum       = .10,
    .cooldown      = 7,
    .dodge_dmg_cap = 90,
    .dodge_dmg     = 10,
    .limb_hurt     = 180,
    .powerhit_crit = true,
    .block         = {"@C$N@W moves quickly and blocks your final flash!@n", "@WYou move quickly and block @C$n's@W final flash!@n", "@C$N@W moves quickly and blocks @c$n's@W final flash!@n"},
    .dodge         = {"@C$N@W manages to dodge your final flash, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W final flash, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W final flash, letting it slam into the surroundings!@n"},
    .miss          = {"@WYou can't believe it but your final flash misses, flying through the air harmlessly!@n", "@C$n@W fires a final flash at you, but misses!@n ", "@c$n@W fires a final flash at @C$N@W, but somehow misses!@n "},
    .miss_spent    = {"@WYou can't believe it but your final flash misses, flying through the air harmlessly!@n", "@C$n@W fires a final flash at you, but misses!@n", "@c$n@W fires a final flash at @C$N@W, but somehow misses!@n"},
    .hits          = {{"@WYou grin as you bring either hand to the sides of your body. Your charged ki begins to pool there as @bblue@W orbs of energy form in your palms. You quickly bring both hands forward slamming your wrists together with the palms flat out facing @c$N@W! You shout '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's body!@n", "@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing YOU! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over YOUR body!@n", "@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing @c$N@W! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's body!@n"}, {"@WYou grin as you bring either hand to the sides of your body. Your charged ki begins to pool there as @bblue@W orbs of energy form in your palms. You quickly bring both hands forward slamming your wrists together with the palms flat out facing @c$N@W! You shout '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's face!@n", "@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing YOU! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over YOUR face!@n", "@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing @c$N@W! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's face!@n"}, {"@WYou grin as you bring either hand to the sides of your body. Your charged ki begins to pool there as @bblue@W orbs of energy form in your palms. You quickly bring both hands forward slamming your wrists together with the palms flat out facing @c$N@W! You shout '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's gut!@n", "@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing YOU! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over YOUR gut!@n", "@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing @c$N@W! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's gut!@n"}, {"@WYou grin as you bring either hand to the sides of your body. Your charged ki begins to pool there as @bblue@W orbs of energy form in your palms. You quickly bring both hands forward slamming your wrists together with the palms flat out facing @c$N@W! You shout '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's arm!@n", "@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing YOU! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over YOUR arm!@n", "@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing @c$N@W! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's arm!@n"}, {"@WYou grin as you bring either hand to the sides of your body. Your charged ki begins to pool there as @bblue@W orbs of energy form in your palms. You quickly bring both hands forward slamming your wrists together with the palms flat out facing @c$N@W! You shout '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's leg!@n", "@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing YOU! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over YOUR leg!@n", "@C$n@W grins as $e brings either hand to the sides of $s body. @C$n@W's charged ki begins to pool there as @bblue@W orbs of energy form in $s palms. @C$n@W quickly brings both hands forward slamming $s wrists together with the palms flat out facing @c$N@W! @C$n@W shouts '@DF@ci@Cn@Da@cl @CF@Dl@ca@Cs@Dh@W!' as a massive wave of energy erupts all over @c$N@W's leg!@n"}},
    .obj_to_char   = "@WYou fire a final flash at $p@W!@n",
    .obj_to_room   = "@C$n@W fires a final flash at $p@W!@n",
};

const ki_attack TRIBEAM = {
    .skill         = SKILL_TRIBEAM,
    .name          = "Tribeam",
    .damtype       = 26,
    .attperc       = .2,
    .minimum       = .10,
    .cooldown      = 7,
    .dodge_dmg_cap = 90,
    .dodge_dmg     = 10,
    .limb_hurt     = 180,
    .block         = {"@C$N@W moves quickly and blocks your tribeam!@n", "@WYou move quickly and block @C$n's@W tribeam!@n", "@C$N@W moves quickly and blocks @c$n's@W tribeam!@n"},
    .dodge         = {"@C$N@W manages to dodge your tribeam, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W tribeam, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W tribeam, letting it slam into the surroundings!@n"},
    .miss          = {"@WYou can't believe it but your tribeam misses, flying through the air harmlessly!@n", "@C$n@W fires a tribeam at you, but misses!@n ", "@c$n@W fires a tribeam at @C$N@W, but somehow misses!@n "},
    .miss_spent    = {"@WYou can't believe it but your tribeam misses, flying through the air harmlessly!@n", "@C$n@W fires a tribeam at you, but misses!@n", "@c$n@W fires a tribeam at @C$N@W, but somehow misses!@n"},
    .hits          = {{"@WYou gather your charged ki as you form a triangle with your hands. You aim the gap between your hands at @c$N@W and shout '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from your hands and slams into $S body!@n", "@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at YOU and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into YOUR body!@n", "@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at @c$N@W and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into @c$N@W's body!@n"}, {"@WYou gather your charged ki as you form a triangle with your hands. You aim the gap between your hands at @c$N@W and shout '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from your hands and slams into $S head!@n", "@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at YOU and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into YOUR head!@n", "@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at @c$N@W and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into @c$N@W's head!@n"}, {"@WYou gather your charged ki as you form a triangle with your hands. You aim the gap between your hands at @c$N@W and shout '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from your hands and slams into $S gut!@n", "@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at YOU and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into YOUR gut!@n", "@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at @c$N@W and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into @c$N@W's gut!@n"}, {"@WYou gather your charged ki as you form a triangle with your hands. You aim the gap between your hands at @c$N@W and shout '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from your hands and slams into $S arm!@n", "@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at YOU and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into YOUR arm!@n", "@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at @c$N@W and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into @c$N@W's arm!@n"}, {"@WYou gather your charged ki as you form a triangle with your hands. You aim the gap between your hands at @c$N@W and shout '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from your hands and slams into $S leg!@n", "@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at YOU and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into YOUR leg!@n", "@C$n@W gathers $s charged ki as $e forms a triangle with $s hands. @C$n@W aims the gap between $s hands at @c$N@W and shouts '@rT@RR@YI@rB@RE@YA@rM@W!'. A bright blast of energy explodes from $s hands and slams into @c$N@W's leg!@n"}},
    .obj_to_char   = "@WYou fire a tribeam at $p@W!@n",
    .obj_to_room   = "@C$n@W fires a tribeam at $p@W!@n",
    .bonus_damage  = tribeam_life_force,
    .after_hit     = tribeam_stun,
};

const ki_attack DODONPA = {
    .skill        = SKILL_DODONPA,
    .name         = "Dodonpa",
    .damtype      = 15,
    .attperc      = .1,
    .minimum      = .05,
    .cooldown     = 6,
    .perfection   = true,
    .block        = {"@C$N@W moves quickly and blocks your dodonpa!@n", "@WYou move quickly and block @C$n's@W dodonpa!@n", "@C$N@W moves quickly and blocks @c$n's@W dodonpa!@n"},
    .dodge        = {"@C$N@W manages to dodge your dodonpa, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W dodonpa, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W dodonpa, letting it slam into the surroundings!@n"},
    .miss         = {"@WYou can't believe it but your dodonpa misses, flying through the air harmlessly!@n", "@C$n@W fires a dodonpa at you, but misses!@n ", "@c$n@W fires a dodonpa at @C$N@W, but somehow misses!@n "},
    .miss_spent   = {"@WYou can't believe it but your dodonpa misses, flying through the air harmlessly!@n", "@C$n@W fires a dodonpa at you, but misses!@n", "@c$n@W fires a dodonpa at @C$N@W, but somehow misses!@n"},
    .hits         = {{"@WYou gather your charged ki into your index finger and point at @c$N@W. A @Ygolden@W glow forms around your finger tip right before you shout '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fire a large golden beam! The beam slams into $S body and explodes!@n", "@C$n@W gathers $s charged ki into $s index finger and points at you. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into your body and explodes!@n", "@C$n@W gathers $s charged ki into $s index finger and points at @c$N@W. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into @c$N@W's body and explodes!@n"}, {"@WYou gather your charged ki into your index finger and point at @c$N@W. A @Ygolden@W glow forms around your finger tip right before you shout '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fire a large golden beam! The beam slams into $S face and explodes!@n", "@C$n@W gathers $s charged ki into $s index finger and points at you. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into your face and explodes!@n", "@C$n@W gathers $s charged ki into $s index finger and points at @c$N@W. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into @c$N@W's face and explodes!@n"}, {"@WYou gather your charged ki into your index finger and point at @c$N@W. A @Ygolden@W glow forms around your finger tip right before you shout '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fire a large golden beam! The beam slams into $S gut and explodes!@n", "@C$n@W gathers $s charged ki into $s index finger and points at you. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into your gut and explodes!@n", "@C$n@W gathers $s charged ki into $s index finger and points at @c$N@W. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into @c$N@W's gut and explodes!@n"}, {"@WYou gather your charged ki into your index finger and point at @c$N@W. A @Ygolden@W glow forms around your finger tip right before you shout '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fire a large golden beam! The beam slams into $S arm and explodes!@n", "@C$n@W gathers $s charged ki into $s index finger and points at you. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into your arm and explodes!@n", "@C$n@W gathers $s charged ki into $s index finger and points at @c$N@W. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into @c$N@W's arm and explodes!@n"}, {"@WYou gather your charged ki into your index finger and point at @c$N@W. A @Ygolden@W glow forms around your finger tip right before you shout '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fire a large golden beam! The beam slams into $S leg and explodes!@n", "@C$n@W gathers $s charged ki into $s index finger and points at you. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into your leg and explodes!@n", "@C$n@W gathers $s charged ki into $s index finger and points at @c$N@W. A @Ygolden@W glow forms around the finger tip right before $e shouts '@YD@yo@Yd@yo@Yn@yp@Ya@W!' and fires a large golden beam! The beam slams into @c$N@W's leg and explodes!@n"}},
    .obj_to_char  = "@WYou fire a dodonpa at $p@W!@n",
    .obj_to_room  = "@C$n@W fires a dodonpa at $p@W!@n",
    .after_hit    = dodonpa_drain_ki,
    .after_paying = dodonpa_wait,
};

const ki_attack MASENKO = {
    .skill        = SKILL_MASENKO,
    .name         = "Masenko",
    .damtype      = 14,
    .attperc      = .15,
    .minimum      = .05,
    .cooldown     = 6,
    .perfection   = true,
    .absorb       = ki_absorb::costs_all,
    .block        = {"@C$N@W moves quickly and blocks your masenko!@n", "@WYou move quickly and block @C$n's@W masenko!@n", "@C$N@W moves quickly and blocks @c$n's@W masenko!@n"},
    .dodge        = {"@C$N@W manages to dodge your masenko, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W masenko, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W masenko, letting it slam into the surroundings!@n"},
    .miss         = {"@WYou can't believe it but your masenko misses, flying through the air harmlessly!@n", "@C$n@W fires a masenko at you, but misses!@n ", "@c$n@W fires a masenko at @C$N@W, but somehow misses!@n "},
    .miss_spent   = {"@WYou can't believe it but your masenko misses, flying through the air harmlessly!@n", "@C$n@W fires a masenko at you, but misses!@n", "@c$n@W fires a masenko at @C$N@W, but somehow misses!@n"},
    .hits         = {{"@WYou place your hands on top of each other and raise them over your head. Energy that you have charged gathers into your palms just before you bring them down aimed at @c$N@W! You scream '@RMasenko @rHa!@W' as you release your attack. A bright red wave of energy flows from your hands and slams into $S body!@n", "@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at you! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into YOUR body!@n", "@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at @c$N@W! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into @c$N@W's body!@n"}, {"@WYou place your hands on top of each other and raise them over your head. Energy that you have charged gathers into your palms just before you bring them down aimed at @c$N@W! You scream '@RMasenko @rHa!@W' as you release your attack. A bright red wave of energy flows from your hands and slams into $S face!@n", "@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at you! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into YOUR face!@n", "@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at @c$N@W! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into @c$N@W's face!@n"}, {"@WYou place your hands on top of each other and raise them over your head. Energy that you have charged gathers into your palms just before you bring them down aimed at @c$N@W! You scream '@RMasenko @rHa!@W' as you release your attack. A bright red wave of energy flows from your hands and slams into $S gut!@n", "@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at you! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into YOUR gut!@n", "@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at @c$N@W! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into @c$N@W's gut!@n"}, {"@WYou place your hands on top of each other and raise them over your head. Energy that you have charged gathers into your palms just before you bring them down aimed at @c$N@W! You scream '@RMasenko @rHa!@W' as you release your attack. A bright red wave of energy flows from your hands and slams into $S arm!@n", "@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at you! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into YOUR arm!@n", "@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at @c$N@W! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into @c$N@W's arm!@n"}, {"@WYou place your hands on top of each other and raise them over your head. Energy that you have charged gathers into your palms just before you bring them down aimed at @c$N@W! You scream '@RMasenko @rHa!@W' as you release your attack. A bright red wave of energy flows from your hands and slams into $S leg!@n", "@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at you! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into YOUR leg!@n", "@C$n@W places $s hands on top of each other and raises them over $s head. Energy that $e had charged gathers into $s palms just before $e brings them down aimed at @c$N@W! $e screams '@RMasenko @rHa!@W' as $e releases $s attack. A bright red wave of energy flows from $s hands and slams into @c$N@W's leg!@n"}},
    .obj_to_char  = "@WYou fire a masenko at $p@W!@n",
    .obj_to_room  = "@C$n@W fires a masenko at $p@W!@n",
    .bonus_damage = masenko_bonus,
    .after_hit    = masenko_drain_stamina,
};

const ki_attack KAMEHAMEHA = {
    .skill          = SKILL_KAMEHAMEHA,
    .name           = "KameHameHa Wave",
    .damtype        = 13,
    .attperc        = .15,
    .minimum        = .05,
    .cooldown       = 6,
    .perfection     = true,
    .absorb         = ki_absorb::costs_all,
    .block_improves = true,
    .block          = {"@C$N@W moves quickly and blocks your kamehameha!@n", "@WYou move quickly and block @C$n's@W kamehameha!@n", "@C$N@W moves quickly and blocks @c$n's@W kamehameha!@n"},
    .dodge          = {"@C$N@W manages to dodge your kamehameha, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W kamehameha, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W kamehameha, letting it slam into the surroundings!@n"},
    .miss           = {"@WYou can't believe it but your kamehameha misses, flying through the air harmlessly!@n", "@C$n@W fires a kamehameha at you, but misses!@n ", "@c$n@W fires a kamehameha at @C$N@W, but somehow misses!@n "},
    .miss_spent     = {"@WYou can't believe it but your kamehameha misses, flying through the air harmlessly!@n", "@C$n@W fires a kamehameha at you, but misses!@n", "@c$n@W fires a kamehameha at @C$N@W, but somehow misses!@n"},
    .hits           = {{"@WYou cup your hands at your side and begin to pool your charged ki there. As the ki pools there you begin to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly you bring your hands forward facing @c$N@W and shout '@BHAAAA!!!@W' while releasing a bright blue kamehameha at $M! It slams into $S body and explodes!@n", "@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing you and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into your body and explodes!@n", "@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing @c$N@W and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into $S body and explodes!@n"}, {"@WYou cup your hands at your side and begin to pool your charged ki there. As the ki pools there you begin to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly you bring your hands forward facing @c$N@W and shout '@BHAAAA!!!@W' while releasing a bright blue kamehameha at $M! It slams into $S face and explodes!@n", "@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing you and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into your face and explodes!@n", "@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing @c$N@W and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into $S face and explodes!@n"}, {"@WYou cup your hands at your side and begin to pool your charged ki there. As the ki pools there you begin to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly you bring your hands forward facing @c$N@W and shout '@BHAAAA!!!@W' while releasing a bright blue kamehameha at $M! It slams into $S gut and explodes!@n", "@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing you and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into your gut and explodes!@n", "@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing @c$N@W and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into $S gut and explodes!@n"}, {"@WYou cup your hands at your side and begin to pool your charged ki there. As the ki pools there you begin to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly you bring your hands forward facing @c$N@W and shout '@BHAAAA!!!@W' while releasing a bright blue kamehameha at $M! It slams into $S arm and explodes!@n", "@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing you and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into your arm and explodes!@n", "@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing @c$N@W and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into $S arm and explodes!@n"}, {"@WYou cup your hands at your side and begin to pool your charged ki there. As the ki pools there you begin to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly you bring your hands forward facing @c$N@W and shout '@BHAAAA!!!@W' while releasing a bright blue kamehameha at $M! It slams into $S leg and explodes!@n", "@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing you and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into your leg and explodes!@n", "@C$n@W cups $s hands at $s side and begins to pool charged ki there. As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. Suddenly $e brings $s hands forward facing @c$N@W and shouts '@BHAAAA!!!@W' while releasing a bright blue kamehameha! It slams into $S leg and explodes!@n"}},
    .obj_to_char    = "@WYou fire a kamehameha at $p@W!@n",
    .obj_to_room    = "@C$n@W fires a kamehameha at $p@W!@n",
    .after_paying   = kamehameha_refund,
};

/* Knowing the dualbeam well makes a critical hit likelier */
int dualbeam_aim(ki_shot &s, int hitspot)
{
  if (char_get_skill(s.ch, SKILL_DUALBEAM) >= 100) {
    if (rand_number(s.g, 1, 100) >= 60) {
      hitspot = 2;
    }
  } else if (char_get_skill(s.ch, SKILL_DUALBEAM) >= 60) {
    if (rand_number(s.g, 1, 100) >= 80) {
      hitspot = 2;
    }
  } else if (char_get_skill(s.ch, SKILL_DUALBEAM) >= 40) {
    if (rand_number(s.g, 1, 100) >= 95) {
      hitspot = 2;
    }
  }
  return hitspot;
}

/* A master of the beam can knock someone it hurts badly out of the room, or into a wall */
void beam_knockback(ki_shot &s, ki_outcome how)
{
  if (how != ki_outcome::hit)
    return;
  GameWorld *g           = s.g;
  struct char_data *ch   = s.ch;
  struct char_data *vict = s.vict;
  int master_roll = rand_number(g, 1, 100), master_chance = 0, master_pass = false;

  if (s.skill >= 100)
    master_chance = 20;
  else if (s.skill >= 75)
    master_chance = 10;
  else if (s.skill >= 50)
    master_chance = 5;

  if (master_chance >= master_roll)
    master_pass = true;

  if (getCurPL(g, vict) > 0 && s.dmg > getEffMaxPL(g, vict) / 4 && master_pass == true) {
    int attempt = rand_number(g, 0, NUM_OF_DIRS); /* Select a random direction */
    int count   = 0;
    while (count < 12) {
      attempt = count;
      if (char_can_go(g, vict, attempt)) {
        count = 12;
      } else {
        count++;
      }
    }
    if (char_can_go(g, vict, attempt)) {
      act(g, "$N@W is pushed away by the blast!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@WYou are pushed away by the blast!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "$N@W is pushed away by the blast!@n", true, ch, nullptr, vict, TO_NOTVICT);
      do_simple_move(g, vict, attempt, true);
    } else {
      act(g, "$N@W is pushed away by the blast, but is slammed into an obstruction!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@WYou are pushed away by the blast, but are slammed into an obstruction!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "$N@W is pushed away by the blast, but is slammed into an obstruction!@n", true, ch, nullptr, vict, TO_NOTVICT);
      s.dmg *= 2;
      hurt(g, 1, 195, ch, vict, nullptr, s.dmg, 1);
    }
  }
}

const ki_attack DUALBEAM = {
    .skill       = SKILL_DUALBEAM,
    .name        = "Dualbeam",
    .damtype     = 22,
    .attperc     = .1,
    .minimum     = .05,
    .cooldown    = 6,
    .shots       = 2,
    .block       = {"@C$N@W moves quickly and blocks your dualbeam!@n", "@WYou move quickly and block @C$n's@W dualbeam!@n", "@C$N@W moves quickly and blocks @c$n's@W dualbeam!@n"},
    .dodge       = {"@C$N@W manages to dodge your dualbeam, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W dualbeam, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W dualbeam, letting it slam into the surroundings!@n"},
    .miss        = {"@WYou can't believe it but your dualbeam misses, flying through the air harmlessly!@n", "@C$n@W fires a dualbeam at you, but misses!@n ", "@c$n@W fires a dualbeam at @C$N@W, but somehow misses!@n "},
    .miss_spent  = {"@WYou can't believe it but your dualbeam misses, flying through the air harmlessly!@n", "@C$n@W fires a dualbeam at you, but misses!@n", "@c$n@W fires a dualbeam at @C$N@W, but somehow misses!@n"},
    .hits        = {{"@WYou gather your charged energy up through the circuits in your arms. A @gg@Gr@We@wen @Wglow appears around your hand right as you aim it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S body in that instant!@n", "@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at YOU. A @gg@Gr@We@wen @Wbeam blasts out and slams into YOUR body in that instant!@n", "@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S body in that instant!@n"}, {"@WYou gather your charged energy up through the circuits in your arms. A @gg@Gr@We@wen @Wglow appears around your hand right as you aim it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S face in that instant!@n", "@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at YOU. A @gg@Gr@We@wen @Wbeam blasts out and slams into YOUR face in that instant!@n", "@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S face in that instant!@n"}, {"@WYou gather your charged energy up through the circuits in your arms. A @gg@Gr@We@wen @Wglow appears around your hand right as you aim it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S gut in that instant!@n", "@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at YOU. A @gg@Gr@We@wen @Wbeam blasts out and slams into YOUR gut in that instant!@n", "@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S gut in that instant!@n"}, {"@WYou gather your charged energy up through the circuits in your arms. A @gg@Gr@We@wen @Wglow appears around your hand right as you aim it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S arm in that instant!@n", "@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at YOU. A @gg@Gr@We@wen @Wbeam blasts out and slams into YOUR arm in that instant!@n", "@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S arm in that instant!@n"}, {"@WYou gather your charged energy up through the circuits in your arms. A @gg@Gr@We@wen @Wglow appears around your hand right as you aim it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S leg in that instant!@n", "@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at YOU. A @gg@Gr@We@wen @Wbeam blasts out and slams into YOUR leg in that instant!@n", "@C$n@W gathers $s charged energy up through the circuits in $s arms. A @gg@Gr@We@wen @Wglow appears around $s hand right as $e aims it at @c$N@W. A @gg@Gr@We@wen @Wbeam blasts out and slams into $S leg in that instant!@n"}},
    .obj_to_char = "@WYou fire a dualbeam at $p@W!@n",
    .obj_to_room = "@C$n@W fires a dualbeam at $p@W!@n",
    .aim         = dualbeam_aim,
};

const ki_attack BEAM = {
    .skill           = SKILL_BEAM,
    .name            = "beam",
    .damtype         = 10,
    .attperc         = .1,
    .minimum         = .01,
    .cooldown        = 5,
    .absorb          = ki_absorb::costs_one,
    .hide            = true,
    .block_improves  = true,
    .limb_hurt       = 195,
    .needs_free_hand = true,
    .dodge_explosion = nullptr,
    .parry           = {"@C$N@W deflects your beam, sending it flying away!@n", "@WYou deflect @C$n's@W beam sending it flying away!@n", "@C$N@W deflects @c$n's@W beam sending it flying away!@n"},
    .block           = {"@C$N@W moves quickly and blocks beam!@n", "@WYou move quickly and block @C$n's@W beam!@n", "@C$N@W moves quickly and blocks @c$n's@W beam!@n"},
    .dodge           = {"@C$N@W manages to dodge your beam, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W beam, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W beam, letting it slam into the surroundings!@n"},
    .miss            = {"@WYou can't believe it but your beam misses, flying through the air harmlessly!@n", "@C$n@W fires a bright yellow beam at you, but misses!@n ", "@c$n@W fires a bright yellow beam at @C$N@W, but somehow misses!@n "},
    .miss_spent      = {"@WYou can't believe it but your beam misses, flying through the air harmlessly!@n", "@C$n@W fires a bright yellow beam at you, but misses!@n", "@c$n@W fires a bright yellow beam at @C$N@W, but somehow misses!@n"},
    .hits            = {{"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S chest!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your chest!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's chest!@n"}, {"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S face!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your face!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's face!@n"}, {"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S gut!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your gut!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's gut!@n"}, {"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S arm!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your arm!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's arm!@n"}, {"@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S leg!@n", "@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your leg!@n", "@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's leg!@n"}},
    .obj_to_char     = "@WYou fire a beam at $p@W!@n",
    .obj_to_room     = "@C$n@W fires a beam at $p@W!@n",
    .after_paying    = beam_knockback,
};

} // namespace

/* Combat commands below this line */

ACMD(do_galikgun) { perform_ki_attack(g, ch, argument, GALIKGUN); }

ACMD(do_final) { perform_ki_attack(g, ch, argument, FINAL_FLASH); }

ACMD(do_tribeam) { perform_ki_attack(g, ch, argument, TRIBEAM); }

ACMD(do_dualbeam) { perform_ki_attack(g, ch, argument, DUALBEAM); }

ACMD(do_dodonpa) { perform_ki_attack(g, ch, argument, DODONPA); }

ACMD(do_masenko) { perform_ki_attack(g, ch, argument, MASENKO); }

ACMD(do_kamehameha) { perform_ki_attack(g, ch, argument, KAMEHAMEHA); }

ACMD(do_beam) { perform_ki_attack(g, ch, argument, BEAM); }
