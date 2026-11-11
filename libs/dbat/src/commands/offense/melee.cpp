/* melee.c: basic melee attacks
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
#include "strike.hpp"

namespace {

/* Where an elbow, a kick or a knee lands: its fourth place is a leg and its fifth an arm */
constexpr strike_spot LEG_THEN_ARM[5] = {strike_spot::body, strike_spot::head, strike_spot::body, strike_spot::leg,
                                         strike_spot::arm};

/* Frieza's students with style uppercut sooner */
int uppercut_cooldown(strike_shot &s)
{
  return char_is_frieza(s.ch) && char_get_skills_base(s.ch, SKILL_STYLE) >= 75 ? 5 : 7;
}

/* An uppercut to the chin can knock the victim out */
void uppercut_knockout(strike_shot &s, int where)
{
  struct char_data *vict = s.vict;

  if (where != 2)
    return;
  if (!char_aff_flagged(vict, AFF_KNOCKED) &&
      (rand_number(s.g, 1, 8) >= 7 && (getCurPL(s.g, vict) > getCurPL(s.g, s.ch) / 5) &&
       !char_aff_flagged(vict, AFF_SANCTUARY))) {
    act_triple(s.g, {"@C$N@W is knocked out!@n", "@WYou are knocked out!@n", "@C$N@W is knocked out!@n", true}, s.ch,
               nullptr, vict);
    setStatusKnockedOut(vict);
  }
}

/* Nail's students with style throw roundhouses sooner */
int roundhouse_cooldown(strike_shot &s)
{
  return char_is_nail(s.ch) && char_get_skills_base(s.ch, SKILL_STYLE) >= 75 ? 5 : 7;
}

/* A roundhouse to the head can knock a standing victim down */
void roundhouse_knockdown(strike_shot &s, int where)
{
  if (where == 2 && !char_aff_flagged(s.vict, AFF_FLYING) && char_get_position(s.vict) == POS_STANDING &&
      rand_number(s.g, 1, 8) >= 7)
    handle_knockdown(s.g, s.vict);
}

const strike_attack UPPERCUT = {
    .skill         = SKILL_UPPERCUT,
    .name          = "uppercut",
    .damtype       = 5,
    .stamina_share = 200,
    .improve_mode  = 1,
    .limb_hurt     = 195,
    .leg_flag      = 1,
    .fireshield    = "hand",
    .parry         = {"@C$N@W parries your uppercut with a punch of $S own!@n", "@WYou parry @C$n's@W uppercut with a punch of your own!@n", "@C$N@W parries @c$n's@W uppercut with a punch of $S own!@n", true},
    .block         = {"@C$N@W moves quickly and blocks your uppercut!@n", "@WYou move quickly and block @C$n's@W uppercut!@n", "@C$N@W moves quickly and blocks @c$n's@W uppercut!@n", true},
    .dodge         = {"@C$N@W manages to dodge your uppercut!@n", "@WYou dodge @C$n's@W uppercut!@n", "@C$N@W manages to dodge @c$n's@W uppercut!@n", true},
    .miss          = {"@WYou can't believe it but your uppercut misses!@n", "@C$n@W throws an uppercut at you but somehow misses!@n", "@c$n@W throws an uppercut at @C$N@W but somehow misses!@n", true},
    .miss_spent    = {"@WYou can't believe it but your uppercut misses!@n", "@C$n@W throws an uppercut at you but somehow misses!@n", "@c$n@W throws an uppercut at @C$N@W but somehow misses!@n", true},
    .hits          = {{"@WYou leap up and launch an uppercut into @C$N's@W body!@n", "@C$n@W leaps up and launches an uppercut into your body!@n", "@c$n@W leaps up and launches an uppercut into @C$N's@W body!@n", true}, {"@WYou smash an uppercut into @C$N's@W chin!@n", "@C$n@W smashes an uppercut into your chin!@n", "@c$n@W smashes an uppercut into @C$N's@W chin!@n", true}, {"@WYou uppercut @C$N@W, hitting $M directly in chest!@n", "@C$n@W uppercuts you, hitting you directly in the chest!@n", "@c$n@W uppercuts @C$N@W, hitting $M directly in the chest!@n", true}, {"@WYour poorly aimed uppercut hits @C$N@W in the arm!@n", "@C$n@W poorly aims an uppercut and hits you in the arm!@n", "@c$n@W poorly aims an uppercut and hits @C$N@W in the arm!@n", true}, {"@WYou slam an uppercut into @C$N's@W leg!@n", "@C$n@W slams an uppercut into your leg!@n", "@c$n@W slams an uppercut into @C$N's@W leg!@n", true}},
    .obj_to_char   = "@WYou uppercut $p@W as hard as you can!@n",
    .obj_to_room   = "@C$n@W uppercuts $p@W extremely hard!@n",
    .obj_hide      = true,
    .cooldown_for  = uppercut_cooldown,
    .on_spot       = uppercut_knockout,
};

const strike_attack ROUNDHOUSE = {
    .skill         = SKILL_ROUNDHOUSE,
    .name          = "roundhouse",
    .damtype       = 4,
    .stamina_share = 180,
    .improve_mode  = 1,
    .legs          = true,
    .limb_hurt     = 195,
    .leg_flag      = 1,
    .fireshield    = "leg",
    .parry         = {"@C$N@W parries your roundhouse with a punch of $S own!@n", "@WYou parry @C$n's@W roundhouse with a punch of your own!@n", "@C$N@W parries @c$n's@W roundhouse with a punch of $S own!@n", true},
    .block         = {"@C$N@W moves quickly and blocks your roundhouse!@n", "@WYou move quickly and block @C$n's@W roundhouse!@n", "@C$N@W moves quickly and blocks @c$n's@W roundhouse!@n", true},
    .dodge         = {"@C$N@W manages to dodge your roundhouse!@n", "@WYou dodge @C$n's@W roundhouse!@n", "@C$N@W manages to dodge @c$n's@W roundhouse!@n", true},
    .miss          = {"@WYou can't believe it but your roundhouse misses!@n", "@C$n@W throws a roundhouse at you but somehow misses!@n", "@c$n@W throws a roundhouse at @C$N@W but somehow misses!@n", true},
    .miss_spent    = {"@WYou can't believe it but your roundhouse misses!@n", "@C$n@W throws a roundhouse at you but somehow misses!@n", "@c$n@W throws a roundhouse at @C$N@W but somehow misses!@n", true},
    .hits          = {{"@WYou spin in mid air and land a kick into @C$N's@W body!@n", "@C$n@W spins in mid air and lands a kick into your body!@n", "@c$n@W spins in mid air and lands a kick into @C$N's@W body!@n", true}, {"@WYou spin a fierce roundhouse into @C$N's@W gut!@n", "@C$n@W spins a fierce roundhouse into your gut!@n", "@c$n@W spins a fierce roundhouse into @C$N's@W gut!@n", true}, {"@WYou throw a roundhouse at @C$N@W, hitting $M directly in the neck!@n", "@C$n@W throws a roundhouse at you, hitting YOU directly in the neck!@n", "@c$n@W throws a roundhouse at @C$N@W, hitting $M directly in the face!@n", true}, {"@WYour poorly aimed roundhouse hits @C$N@W in the arm!@n", "@C$n@W poorly aims a roundhouse and hits you in the arm!@n", "@c$n@W poorly aims a roundhouse and hits @C$N@W in the arm!@n", true}, {"@WYou slam a roundhouse into @C$N's@W leg!@n", "@C$n@W slams a roundhouse into your leg!@n", "@c$n@W slams a roundhouse into @C$N's@W leg!@n", true}},
    .obj_to_char   = "@WYou roundhouse $p@W as hard as you can!@n",
    .obj_to_room   = "@C$n@W roundhouses $p@W extremely hard!@n",
    .obj_hide      = true,
    .cooldown_for  = roundhouse_cooldown,
    .on_spot       = roundhouse_knockdown,
};

const strike_attack ELBOW = {
    .skill             = SKILL_ELBOW,
    .name              = "elbow",
    .damtype           = 2,
    .cooldown          = 6,
    .stamina_share     = 300,
    .fireshield        = "elbow",
    .spent_hurts_first = true,
    .spent_free        = true,
    .parry             = {"@C$N@W parries your elbow with an elbow of $S own!@n", "@WYou parry @C$n's@W elbow with one of your own!@n", "@C$N@W parries @c$n's@W elbow with an elbow of $S own!@n", true},
    .block             = {"@C$N@W blocks your elbow strike!@n", "@WYou block @C$n's@W elbow strike!@n", "@C$N@W blocks @c$n's@W elbow strike!@n", true},
    .dodge             = {"@C$N@W dodges your elbow strike!@n", "@WYou dodge @C$n's@W elbow strike!@n", "@C$N@W dodges @c$n's@W elbow strike!@n", true},
    .miss              = {"@WYou can't believe it, your elbow strike misses!@n", "@C$n@W throws an elbow strike at you, but thankfully misses!@n", "@c$n@W throws an elbow strike at @C$N@W, but misses!@n", true},
    .miss_spent        = {"@WYou can't believe it, your elbow strike misses!@n", "@C$n@W throws an elbow stike at you, but thankfully misses!@n", "@c$n@W throws an elbow strike at @C$N@W, but misses!@n", true},
    .hits              = {{"@WYou slam your elbow into @C$N's@W body!@n", "@C$n@W slams $s elbow into your body!@n", "@c$n@W slams $s elbow into @C$N's@W body!@n", true}, {"@WYou slam your elbow into @C$N's@W face!@n", "@C$n@W slams $s elbow into your face!@n", "@c$n@W slams $s elbow into @C$N's@W face!@n", true}, {"@WYou land your elbow against @C$N's@W gut!@n", "@C$n@W lands $s elbow against your gut!@n", "@C$n@W lands $s elbow against @C$N's@W gut!@n", true}, {"@WYou land your elbow against @C$N's@W leg!@n", "@C$n@W lands $s elbow against your leg!@n", "@C$n@W lands $s elbow against @C$N's@W leg!@n", true}, {"@WYou land your elbow against @C$N's@W arm!@n", "@C$n@W lands $s elbow against your arm!@n", "@C$n@W lands $s elbow against @C$N's@W arm!@n", true}},
    .spots             = LEG_THEN_ARM,
    .obj_to_char       = "@WYou elbow stike $p@W as hard as you can!@n",
    .obj_to_room       = "@C$n@W elbow strikes $p@W extremely hard!@n",
    .obj_hide          = true,
};

const strike_attack KICK = {
    .skill             = SKILL_KICK,
    .name              = "kick",
    .damtype           = 1,
    .cooldown          = 5,
    .stamina_share     = 400,
    .legs              = true,
    .fireshield        = "leg",
    .spent_hurts_first = true,
    .parry             = {"@C$N@W parries your kick with a kick of $S own!@n", "@WYou parry @C$n's@W kick with one of your own!@n", "@C$N@W parries @c$n's@W kick with a kick of $S own!@n", true},
    .block             = {"@C$N@W blocks your kick!@n", "@WYou block @C$n's@W kick!@n", "@C$N@W blocks @c$n's@W kick!@n", true},
    .dodge             = {"@C$N@W dodges your kick!@n", "@WYou dodge @C$n's@W kick!@n", "@C$N@W dodges @c$n's@W kick!@n", true},
    .miss              = {"@WYou can't believe it, your kick misses!@n", "@C$n@W throws a kick at you, but thankfully misses!@n", "@c$n@W throws a kick at @C$N@W, but misses!@n", true},
    .miss_spent        = {"@WYou can't believe it, your kick misses!@n", "@C$n@W throws a kick at you, but thankfully misses!@n", "@c$n@W throws a kick at @C$N@W, but misses!@n", true},
    .hits              = {{"@WYou slam your foot into @C$N's@W body!@n", "@C$n@W slams $s foot into your body!@n", "@c$n@W slams $s foot into @C$N's@W body!@n", true}, {"@WYou slam your foot into @C$N's@W face!@n", "@C$n@W slams $s foot into your face!@n", "@c$n@W slams $s foot into @C$N's@W face!@n", true}, {"@WYou land your foot against @C$N's@W gut!@n", "@C$n@W lands $s foot against your gut!@n", "@C$n@W lands $s foot against @C$N's@W gut!@n", true}, {"@WYou land your foot against @C$N's@W leg!@n", "@C$n@W lands $s foot against your leg!@n", "@C$n@W lands $s foot against @C$N's@W leg!@n", true}, {"@WYou land your foot against @C$N's@W arm!@n", "@C$n@W lands $s foot against your arm!@n", "@C$n@W lands $s foot against @C$N's@W arm!@n", true}},
    .spots             = LEG_THEN_ARM,
    .obj_to_char       = "@WYou kick $p@W as hard as you can!@n",
    .obj_to_room       = "@C$n@W kicks $p@W extremely hard!@n",
    .obj_hide          = true,
};

const strike_attack KNEE = {
    .skill             = SKILL_KNEE,
    .name              = "knee strike",
    .damtype           = 3,
    .cooldown          = 6,
    .stamina_share     = 250,
    .legs              = true,
    .fireshield        = "knee",
    .spent_hurts_first = true,
    .spent_free        = true,
    .parry             = {"@C$N@W parries your knee with a knee of $S own!@n", "@WYou parry @C$n's@W knee with one of your own!@n", "@C$N@W parries @c$n's@W knee with a knee of $S own!@n", true},
    .block             = {"@C$N@W blocks your knee strike!@n", "@WYou block @C$n's@W knee strike!@n", "@C$N@W blocks @c$n's@W knee strike!@n", true},
    .dodge             = {"@C$N@W dodges your knee strike!@n", "@WYou dodge @C$n's@W knee strike!@n", "@C$N@W dodges @c$n's@W knee strike!@n", true},
    .miss              = {"@WYou can't believe it, your knee strike misses!@n", "@C$n@W throws a knee strike at you, but thankfully misses!@n", "@c$n@W throws a knee strike at @C$N@W, but misses!@n", true},
    .miss_spent        = {"@WYou can't believe it, your  knee strike misses!@n", "@C$n@W throws a knee stike at you, but thankfully misses!@n", "@c$n@W throws a knee strike at @C$N@W, but misses!@n", true},
    .hits              = {{"@WYou slam your knee into @C$N's@W body!@n", "@C$n@W slams $s knee into your body!@n", "@c$n@W slams $s knee into @C$N's@W body!@n", true}, {"@WYou slam your knee into @C$N's@W face!@n", "@C$n@W slams $s knee into your face!@n", "@c$n@W slams $s knee into @C$N's@W face!@n", true}, {"@WYou land your knee against @C$N's@W gut!@n", "@C$n@W lands $s knee against your gut!@n", "@C$n@W lands $s knee against @C$N's@W gut!@n", true}, {"@WYou land your knee against @C$N's@W leg!@n", "@C$n@W lands $s knee against your leg!@n", "@C$n@W lands $s knee against @C$N's@W leg!@n", true}, {"@WYou land your knee against @C$N's@W arm!@n", "@C$n@W lands $s knee against your arm!@n", "@C$n@W lands $s knee against @C$N's@W arm!@n", true}},
    .spots             = LEG_THEN_ARM,
    .obj_to_char       = "@WYou knee stike $p@W as hard as you can!@n",
    .obj_to_room       = "@C$n@W knee strikes $p@W extremely hard!@n",
    .obj_hide          = true,
};

const strike_attack PUNCH = {
    .skill         = SKILL_PUNCH,
    .name          = "punch",
    .damtype       = 0,
    .cooldown      = 4,
    .stamina_share = 500,
    .fireshield    = "hand",
    .parry         = {"@C$N@W parries your punch with a punch of $S own!@n", "@WYou parry @C$n's@W punch with a punch of your own!@n", "@C$N@W parries @c$n's@W punch with a punch of $S own!@n", true},
    .block         = {"@C$N@W moves quickly and blocks your punch!@n", "@WYou move quickly and block @C$n's@W punch!@n", "@C$N@W moves quickly and blocks @c$n's@W punch!@n", true},
    .dodge         = {"@C$N@W manages to dodge your punch!@n", "@WYou dodge @C$n's@W punch!@n", "@C$N@W manages to dodge @c$n's@W punch!@n", true},
    .miss          = {"@WYou can't believe it but your punch misses!@n", "@C$n@W throws a punch at you but somehow misses!@n", "@c$n@W throws a punch at @C$N@W but somehow misses!@n", true},
    .miss_spent    = {"@WYou can't believe it but your punch misses!@n", "@C$n@W throws a punch at you but somehow misses!@n", "@c$n@W throws a punch at @C$N@W but somehow misses!@n", true},
    .hits          = {{"@WYou slam your fist into @C$N's@W body!@n", "@C$n@W slams $s fist into your body!@n", "@c$n@W slams $s fist into @C$N's@W body!@n", true}, {"@WYou slam your fist into @C$N's@W face!@n", "@C$n@W slams $s fist into your face!@n", "@c$n@W slams $s fist into @C$N's@W face!@n", true}, {"@WYou punch @C$N@W directly in the gut!@n", "@C$n@W punches you directly in the gut!@n", "@c$n@W punches @C$N@W directly in the gut!@n", true}, {"@WYou punch @C$N@W in the arm!@n", "@C$n@W punches you in the arm!@n", "@c$n@W punches @C$N@W in the arm!@n", true}, {"@WYou punch @C$N@W in the leg!@n", "@C$n@W punches you in the leg!@n", "@c$n@W punches @C$N@W in the arm!@n", true}},
    .obj_to_char   = "@WYou punch $p@W as hard as you can!@n",
    .obj_to_room   = "@C$n@W punches $p@W extremely hard!@n",
    .obj_hide      = true,
};

} // namespace

ACMD(do_uppercut) { perform_strike(g, ch, argument, UPPERCUT); }

ACMD(do_roundhouse) { perform_strike(g, ch, argument, ROUNDHOUSE); }

ACMD(do_elbow) { perform_strike(g, ch, argument, ELBOW); }

ACMD(do_kick) { perform_strike(g, ch, argument, KICK); }

ACMD(do_knee) { perform_strike(g, ch, argument, KNEE); }

ACMD(do_punch) { perform_strike(g, ch, argument, PUNCH); }
