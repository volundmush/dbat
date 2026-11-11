/* kiballs.c: ki ball and sphere attacks
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

/* A crusher ball can slam its victim to the ground */
void crusher_crashdown(ki_shot &s)
{
  if (rand_number(s.g, 1, 3) == 3) {
    tech_handle_crashdown(s.g, s.ch, s.vict);
  }
}

void blessed_hammer_steady(ki_shot &, int &prob)
{
  prob += 15;
}

/* The blessed hammer bites harder on the protected */
void blessed_hammer_sanctuary(ki_shot &s)
{
  if (char_aff_flagged(s.vict, AFF_SANCTUARY)) {
    s.dmg *= calc_critical(s.g, s.ch, 1);
  }
}

/* and can burn anyone but a demon */
void blessed_hammer_burn(ki_shot &s)
{
  if (!char_aff_flagged(s.vict, AFF_BURNED) && rand_number(s.g, 1, 4) == 3 && !char_is_demon(s.vict)) {
    send_to_char(s.g, s.vict, "@RYou are burned by the attack!@n\r\n");
    send_to_char(s.g, s.ch, "@RThey are burned by the attack!@n\r\n");
    char_aff_set(s.vict, AFF_BURNED, true);
  }
}

void deathball_steady(ki_shot &s, int &prob)
{
  prob -= rand_number(s.g, 8, 10);
}

/* An expert can fire two or three kiballs in a row */
void kiball_volley(ki_shot &s, int &)
{
  int mult_roll = rand_number(s.g, 1, 100), mult_chance = 0;

  if (s.skill >= 100) {
    mult_chance = 30;
  } else if (s.skill >= 75) {
    mult_chance = 15;
  } else if (s.skill >= 50) {
    mult_chance = 10;
  }
  if (mult_roll <= mult_chance)
    s.landings = rand_number(s.g, 2, 3);
}

void kiball_volley_message(ki_shot &s)
{
  if (s.landings > 1) {
    act(s.g, "@CYour expertise has allowed you to fire multiple shots in a row!@n", true, s.ch, nullptr, s.vict, TO_CHAR);
    act(s.g, "@C$n's@C expertise has allowed $m to fire multiple shots in a row!@n", true, s.ch, nullptr, s.vict, TO_ROOM);
  }
}

const ki_attack CRUSHER = {
    .skill         = SKILL_CRUSHER,
    .name          = "Crusher Ball",
    .damtype       = 29,
    .attperc       = .2,
    .minimum       = .10,
    .cooldown      = 7,
    .perfection    = true,
    .dodge_dmg_cap = 90,
    .dodge_dmg     = 10,
    .limb_hurt     = 180,
    .block         = {"@C$N@W moves quickly and blocks your crusher ball!@n", "@WYou move quickly and block @C$n's@W crusher ball!@n", "@C$N@W moves quickly and blocks @c$n's@W crusher ball!@n"},
    .dodge         = {"@C$N@W manages to dodge your crusher ball, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W crusher ball, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W crusher ball, letting it slam into the surroundings!@n"},
    .miss          = {"@WYou can't believe it but your crusher ball misses, flying through the air harmlessly!@n", "@C$n@W fires a crusher ball at you, but misses!@n ", "@c$n@W fires a crusher ball at @C$N@W, but somehow misses!@n "},
    .miss_spent    = {"@WYou can't believe it but your crusher ball misses, flying through the air harmlessly!@n", "@C$n@W fires a crusher ball at you, but misses!@n", "@c$n@W fires a crusher ball at @C$N@W, but somehow misses!@n"},
    .hits          = {{"@WYou zoom up into the air higher than @c$N@W and raise your hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W you slam your hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into $S body!@n", "@C$n@W zooms up into the air higher than you and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on YOU $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into YOUR body!@n", "@C$n@W zooms up into the air higher than @c$N@W and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into @c$N@W's body!@n", true}, {"@WYou zoom up into the air higher than @c$N@W and raise your hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W you slam your hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into $S face!@n", "@C$n@W zooms up into the air higher than you and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on YOU $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into YOUR face!@n", "@C$n@W zooms up into the air higher than @c$N@W and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into @c$N@W's face!@n", true}, {"@WYou zoom up into the air higher than @c$N@W and raise your hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W you slam your hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into $S gut!@n", "@C$n@W zooms up into the air higher than you and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on YOU $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into YOUR gut!@n", "@C$n@W zooms up into the air higher than @c$N@W and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into @c$N@W's gut!@n", true}, {"@WYou zoom up into the air higher than @c$N@W and raise your hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W you slam your hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into $S arm!@n", "@C$n@W zooms up into the air higher than you and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on YOU $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into YOUR arm!@n", "@C$n@W zooms up into the air higher than @c$N@W and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into @c$N@W's arm!@n", true}, {"@WYou zoom up into the air higher than @c$N@W and raise your hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W you slam your hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into $S leg!@n", "@C$n@W zooms up into the air higher than you and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on YOU $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into YOUR leg!@n", "@C$n@W zooms up into the air higher than @c$N@W and raises $s hand. Charged @rred@W ki pools above that hand in a large blazing ball. Looking down on @c$N@W $e slams $s hand into the ball of energy while shouting '@rC@Rr@Wu@wsher @rB@Ra@Wl@wl!@W' Moments later the ball of energy slams into @c$N@W's leg!@n", true}},
    .obj_to_char   = "@WYou fire a crusher ball at $p@W!@n",
    .obj_to_room   = "@C$n@W fires a crusher ball at $p@W!@n",
    .after_hit     = crusher_crashdown,
};

const ki_attack BLESSED_HAMMER = {
    .skill         = SKILL_BLESSEDHAMMER,
    .name          = "@WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@C",
    .damtype       = 42,
    .attperc       = .05,
    .minimum       = .01,
    .cooldown      = 5,
    .needs_arms    = false,
    .powerhit_crit = true,
    .dodge_damtype = 17,
    .block         = {"@C$N@W moves quickly and blocks your @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@n!@n", "@WYou move quickly and block @C$n's@W @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@n!@n", "@C$N@W moves quickly and blocks @c$n's@W @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@n!@n"},
    .dodge         = {"@C$N@W manages to dodge your @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W, letting it slam into the surroundings!@n"},
    .miss          = {"@WYou can't believe it but your @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W misses, flying through the air harmlessly!@n", "@C$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W at you, but misses!@n ", "@c$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W at @C$N@W, but somehow misses!@n "},
    .miss_spent    = {"@WYou can't believe it but your @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W misses, flying through the air harmlessly!@n", "@C$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W at you, but misses!@n", "@c$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr@W at @C$N@W, but somehow misses!@n"},
    .hits          = {{"@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their chest!@n", "@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR chest!@n", "@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's chest!@n", true}, {"@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their face!@n", "@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR face!@n", "@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's face!@n", true}, {"@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their gut!@n", "@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR gut!@n", "@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's gut!@n", true}, {"@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their arm!@n", "@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR arm!@n", "@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's arm!@n", true}, {"@WFocusing your attention on @R$N@W, you reach back and form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer before hurling it with all your might into their leg!@n", "@C$n@W narrows $s eyes and focuses $s energy. They reach back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at you with all their @Rmight@W into YOUR leg!@n", "@C$n@W narrows their eyes at @c$N@W. $n reaches back to form an @ne@Dt@nh@De@nr@De@na@Dl @Whammer which they hurl at @c$N@W with all their might into @c$N@W's leg!@n", true}},
    .obj_to_char   = "@WYou fire a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr at $p@W!@n",
    .obj_to_room   = "@C$n@W fires a @WB@Dl@We@Ds@Ws@De@Wd @DH@Wa@Dm@Wm@De@Wr at $p@W!@n",
    .steady        = blessed_hammer_steady,
    .bonus_damage  = blessed_hammer_sanctuary,
    .after_hit     = blessed_hammer_burn,
};

const ki_attack DEATHBALL = {
    .skill         = SKILL_DEATHBALL,
    .name          = "Deathball",
    .damtype       = 38,
    .attperc       = .3,
    .minimum       = .15,
    .cooldown      = 8,
    .kill_mode     = 3,
    .dodge_dmg_cap = 80,
    .dodge_dmg     = 20,
    .limb_hurt     = 170,
    .head_mult     = 4,
    .arm_crit      = 0,
    .blockable     = false,
    .dodge         = {"@C$N@W manages to dodge your deathball, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W deathball, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W deathball, letting it slam into the surroundings!@n"},
    .miss          = {"@WYou can't believe it but your deathball misses, flying through the air harmlessly!@n", "@C$n@W fires a deathball at you, but misses!@n ", "@c$n@W fires a deathball at @C$N@W, but somehow misses!@n "},
    .miss_spent    = {"@WYou can't believe it but your deathball misses, flying through the air harmlessly!@n", "@C$n@W fires a deathball at you, but misses!@n", "@c$n@W fires a deathball at @C$N@W, but somehow misses!@n"},
    .hits          = {{"@WYou raise your hand with its index finger extended upwards. Your charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as you look down on @c$N@W. You move the hand that formed the attack and point at $M as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above you follows the movement! It descends on $M and crushes into $S body before exploding into a massive blast!@n", "@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on YOU. @C$n@W moves the hand that formed the attack and points at YOU as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on YOU and crushes into YOUR body before exploding into a massive blast!@n", "@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on @c$N@W. @C$n@W moves the hand that formed the attack and points at @c$N@W as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on @c$N@W and crushes into $S body before exploding into a massive blast!@n", true}, {"@WYou raise your hand with its index finger extended upwards. Your charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as you look down on @c$N@W. You move the hand that formed the attack and point at $M as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above you follows the movement! It descends on $M and crushes into $S head before exploding into a massive blast!@n", "@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on YOU. @C$n@W moves the hand that formed the attack and points at YOU as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on YOU and crushes into YOUR head before exploding into a massive blast!@n", "@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on @c$N@W. @C$n@W moves the hand that formed the attack and points at @c$N@W as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on @c$N@W and crushes into $S head before exploding into a massive blast!@n", true}, {"@WYou raise your hand with its index finger extended upwards. Your charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as you look down on @c$N@W. You move the hand that formed the attack and point at $M as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above you follows the movement! It descends on $M and crushes into $S gut before exploding into a massive blast!@n", "@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on YOU. @C$n@W moves the hand that formed the attack and points at YOU as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on YOU and crushes into YOUR gut before exploding into a massive blast!@n", "@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on @c$N@W. @C$n@W moves the hand that formed the attack and points at @c$N@W as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on @c$N@W and crushes into $S gut before exploding into a massive blast!@n", true}, {"@WYou raise your hand with its index finger extended upwards. Your charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as you look down on @c$N@W. You move the hand that formed the attack and point at $M as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above you follows the movement! It descends on $M and crushes into $S arm before exploding into a massive blast!@n", "@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on YOU. @C$n@W moves the hand that formed the attack and points at YOU as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on YOU and crushes into YOUR arm before exploding into a massive blast!@n", "@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on @c$N@W. @C$n@W moves the hand that formed the attack and points at @c$N@W as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on @c$N@W and crushes into $S arm before exploding into a massive blast!@n", true}, {"@WYou raise your hand with its index finger extended upwards. Your charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as you look down on @c$N@W. You move the hand that formed the attack and point at $M as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above you follows the movement! It descends on $M and crushes into $S leg before exploding into a massive blast!@n", "@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on YOU. @C$n@W moves the hand that formed the attack and points at YOU as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on YOU and crushes into YOUR leg before exploding into a massive blast!@n", "@C$n@W raises $s hand with its index finger extended upwards. @C$n@W's charged ki begins to pool above that finger, forming a small @rred@W orb of energy. The orb of energy quickly grows to an enormous size as $e looks down on @c$N@W. @C$n@W moves the hand that formed the attack and points at @c$N@W as the @rD@Re@Da@rt@Rh@Db@ra@Rl@Dl@W above $m follows the movement! It descends on @c$N@W and crushes into $S leg before exploding into a massive blast!@n", true}},
    .obj_to_char   = "@WYou fire a deathball at $p@W!@n",
    .obj_to_room   = "@C$n@W fires a deathball at $p@W!@n",
    .steady        = deathball_steady,
};

const ki_attack SPIRITBALL = {
    .skill          = SKILL_SPIRITBALL,
    .name           = "Spirit Ball",
    .parry_name     = "spiritball",
    .damtype        = 39,
    .attperc        = .25,
    .minimum        = .20,
    .cooldown       = 8,
    .kill_mode      = 3,
    .limb_hurt      = 170,
    .guided         = true,
    .dodge_scatters = false,
    .parry          = {"@C$N@W deflects your Spirit Ball, sending it flying away!@n", "@WYou deflect @C$n's@W Spirit Ball sending it flying away!@n", "@C$N@W deflects @c$n's@W Spirit Ball sending it flying away!@n", true},
    .block          = {"@C$N@W moves quickly and blocks the Spirit Ball!@n", "@WYou move quickly and block @C$n's@W Spirit Ball!@n", "@C$N@W moves quickly and blocks @c$n's@W Spirit Ball!@n", true},
    .dodge          = {"@C$N@W manages to dodge your Spirit Ball, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W Spirit Ball, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W Spirit Ball, letting it slam into the surroundings!@n", true},
    .miss           = {"@WYou can't believe it but your spirit ball misses, flying through the air harmlessly!@n", "@C$n@W fires a spirit ball at you, but misses!@n ", "@c$n@W fires a spirit ball at @C$N@W, but somehow misses!@n "},
    .miss_spent     = {"@WYou can't believe it but your spirit ball misses, flying through the air harmlessly!@n", "@C$n@W fires a spirit ball at you, but misses!@n", "@c$n@W fires a spirit ball at @C$N@W, but somehow misses!@n"},
    .hits           = {{"@WYou raise your palm upward with your arm bent in front of you. Your charged ki slowly begins to creep up your arm and form a large glowing orb of energy above your upraised palm. With your @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted you move your hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into $S body, and explodes with a load roar!@n", "@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at YOU! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at you, slamming into YOUR body, and explodes with a load roar!@n", "@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into @c$N@W's body, and explodes with a load roar!@n", true}, {"@WYou raise your palm upward with your arm bent in front of you. Your charged ki slowly begins to creep up your arm and form a large glowing orb of energy above your upraised palm. With your @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted you move your hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into $S head, and explodes with a load roar!@n", "@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at YOU! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at you, slamming into YOUR head, and explodes with a load roar!@n", "@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into @c$N@W's head, and explodes with a load roar!@n", true}, {"@WYou raise your palm upward with your arm bent in front of you. Your charged ki slowly begins to creep up your arm and form a large glowing orb of energy above your upraised palm. With your @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted you move your hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into $S gut, and explodes with a load roar!@n", "@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at YOU! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at you, slamming into YOUR gut, and explodes with a load roar!@n", "@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into @c$N@W's gut, and explodes with a load roar!@n", true}, {"@WYou raise your palm upward with your arm bent in front of you. Your charged ki slowly begins to creep up your arm and form a large glowing orb of energy above your upraised palm. With your @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted you move your hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into $S arm, and explodes with a load roar!@n", "@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at YOU! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at you, slamming into YOUR arm, and explodes with a load roar!@n", "@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into @c$N@W's arm, and explodes with a load roar!@n", true}, {"@WYou raise your palm upward with your arm bent in front of you. Your charged ki slowly begins to creep up your arm and form a large glowing orb of energy above your upraised palm. With your @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted you move your hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into $S leg, and explodes with a load roar!@n", "@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at YOU! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at you, slamming into YOUR leg, and explodes with a load roar!@n", "@C$n@W raises $s palm upward with $s arm bent in front of $m. @C$n@W's charged ki slowly begins to creep up $s arm and form a large glowing orb of energy above $s upraised palm. With $s @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wcompleted $e moves $s hand with index and middle fingers pointing at @c$N@W! The @yS@Yp@Wi@wri@yt @YB@Wa@wll @Wflies at $M, slamming into @c$N@W's leg, and explodes with a load roar!@n", true}},
    .obj_to_char    = "@WYou fire a spirit ball at $p@W!@n",
    .obj_to_room    = "@C$n@W fires a spirit ball at $p@W!@n",
};

const ki_attack KIBALL = {
    .skill           = SKILL_KIBALL,
    .name            = "kiball",
    .damtype         = 7,
    .attperc         = .05,
    .minimum         = .01,
    .cooldown        = 5,
    .absorb          = ki_absorb::costs_one,
    .block_improves  = true,
    .dodge_dmg_cap   = 98,
    .dodge_dmg       = 2,
    .limb_hurt       = 195,
    .needs_free_hand = true,
    .dodge_explosion = nullptr,
    .parry           = {"@C$N@W deflects your kiball, sending it flying away!@n", "@WYou deflect @C$n's@W kiball sending it flying away!@n", "@C$N@W deflects @c$n's@W kiball sending it flying away!@n", true},
    .block           = {"@C$N@W moves quickly and blocks kiball!@n", "@WYou move quickly and block @C$n's@W kiball!@n", "@C$N@W moves quickly and blocks @c$n's@W kiball!@n", true},
    .dodge           = {"@C$N@W manages to dodge your kiball, letting it slam into the surroundings!@n", "@WYou dodge @C$n's@W kiball, letting it slam into the surroundings!@n", "@C$N@W manages to dodge @c$n's@W kiball, letting it slam into the surroundings!@n", true},
    .miss            = {"@WYou can't believe it but your kiball misses, flying through the air harmlessly!@n", "@C$n@W fires a bright yellow kiball at you, but misses!@n", "@c$n@W fires a bright yellow kiball at @C$N@W, but somehow misses!@n", true},
    .miss_spent      = {"@WYou can't believe it but your kiball misses, flying through the air harmlessly!@n", "@C$n@W fires a bright yellow kiball at you, but misses!@n", "@c$n@W fires a bright yellow kiball at @C$N@W, but somehow misses!@n", true},
    .hits            = {{"@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball slams into $M quickly and explodes with roaring light!@n", "@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball slams into you quickly and explodes with roaring light!@n", "@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball slams into $M quickly and explodes with roaring light!@n", true}, {"@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball slams into $S face and explodes, shrouding $S head with smoke!@n", "@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball slams into your face and explodes, leaving you choking on smoke!@n", "@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball slams into $S face and explodes, shrouding $S head with smoke!@n", true}, {"@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball slams into $S body and explodes with a loud roar!@n", "@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball slams into your body and explodes with a loud roar!@n", "@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball slams into $S body and explodes with a loud roar!@n", true}, {"@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball grazes $S arm and explodes shortly after!@n", "@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball grazes your arm and explodes shortly after!@n", "@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball grazes $S arm and explodes shortly after!@n", true}, {"@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball grazes $S leg and explodes shortly after!@n", "@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball grazes your leg and explodes shortly after!@n", "@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball grazes $S leg and explodes shortly after!@n", true}},
    .obj_to_char     = "@WYou fire a kiball at $p@W!@n",
    .obj_to_room     = "@C$n@W fires a kiball at $p@W!@n",
    .obj_hide        = true,
    .steady          = kiball_volley,
    .before_landing  = kiball_volley_message,
};

/* A genocide or a genki dama: a great ball of ki held above the head, which travels to its target */
struct charged_ball {
  int skill{};
  int damtype{};
  obj_vnum vnum{};                                      /* the ball */
  int cooldown{};                                       /* handle_cooldown() when made, if not 0 */
  bool charge_bonus{};                                  /* the user gains a tenth of their power level as charge */
  void (*gather)(GameWorld *g, struct char_data *ch){}; /* before it forms */
  const char *fail_to_char{};
  const char *fail_to_room{};
  act_msgs formed;
};

/* The genki dama draws ki from the user's group */
void genki_gather(GameWorld *g, struct char_data *ch)
{
  for (char_data *friend_char : room_people(g, char_get_in_room(ch))) {
    if (friend_char == ch) {
      continue;
    }
    if (char_aff_flagged(friend_char, AFF_GROUP) && (char_get_master(g, friend_char) == ch || char_get_master(g, ch) == friend_char || char_get_master(g, friend_char) == char_get_master(g, ch))) {
      char_set_charge(ch, char_get_charge(ch) + ((getCurKI(g, ch)) / 10));
      decCurKI(g, ch, getCurKI(g, ch) / 20, 0);
    }
  }
}

const charged_ball GENOCIDE = {
    .skill        = SKILL_GENOCIDE,
    .damtype      = 41,
    .vnum         = 83,
    .charge_bonus = true,
    .fail_to_char = "@WYou raise one arm above your head and pour your charged ki there. A large swirling pink ball of energy begins to form above your raised hand. You lose concentration and the ball of energy dissipates!@n",
    .fail_to_room = "@C$n@W raises one arm above $s head and pours $s charged ki there. A large swirling pink ball of energy begins to form above $s raised hand. @C$n@W loses concentration and the ball of energy dissipates!@n",
    .formed       = {"@WYou raise one arm above your head and pour your charged ki there. A large swirling pink ball of energy begins to form above your raised hand. You grin viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is complete and you toss it at @c$N@W!@n", "@C$n@W raises one arm above $s head and pours $s charged ki there. A large swirling pink ball of energy begins to form above $s raised hand. @C$n@W grins viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is complete and $e tosses it at YOU!@n", "@C$n@W raises one arm above $s head and pours $s charged ki there. A large swirling pink ball of energy begins to form above $s raised hand. @C$n@W grins viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is complete and $e tosses it at @c$N@W!@n", true},
};

const charged_ball GENKIDAMA = {
    .skill        = SKILL_GENKIDAMA,
    .damtype      = 40,
    .vnum         = 82,
    .cooldown     = 10,
    .gather       = genki_gather,
    .fail_to_char = "@WYou raise both your arms upwards and begin to pool your charged ki there. You also start calling on the ki of all living beings in the vicinity who are willing to help. Your concentration wavers though and you waste the energy you have!@n",
    .fail_to_room = "@C$n@W raises both $s arms upwards and begin to pool $s charged ki there. @C$n@W also starts calling on the ki of all living beings in the vicinity who are willing to help. @C$n@W's concentration wavers though and $e wastes the energy $e has!@n",
    .formed       = {"@WYou raise both your arms upwards and begin to pool your charged ki there. You also start calling on the ki of all living beings in the vicinity who are willing to help. A large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W forms above your hands, when it is finished you lob it toward @c$N@W!@n", "@C$n@W raises both $s arms upwards and begin to pool $s charged ki there. @C$n@W also starts calling on the ki of all living beings in the vicinity who are willing to help. A large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W forms above $s hands, when it is finished $e lobs it toward YOU!@n", "@C$n@W raises both $s arms upwards and begin to pool $s charged ki there. @C$n@W also starts calling on the ki of all living beings in the vicinity who are willing to help. A large @cS@Cp@wi@cr@Ci@wt @cB@Co@wm@cb@W forms above $s hands, when it is finished $e lobs it toward @c$N@W!@n", true},
};

void form_charged_ball(GameWorld *g, struct char_data *ch, std::string_view argument, const charged_ball &b)
{
  int perc, prob;
  double attperc = .5, minimum = .4;
  struct char_data *vict = nullptr;
  std::string_view rest  = argument;
  std::string arg        = take_arg(rest);
  std::string arg2       = take_arg(rest);

  if (!can_grav(g, ch) || !check_skill(g, ch, b.skill) || !limb_ok(g, ch, 0))
    return;
  if (arg.empty() && !char_get_fighting(g, ch)) {
    send_to_char(g, ch, "Direct it at who?\r\n");
    return;
  }
  if (!tech_handle_charge(g, ch, arg2, minimum, &attperc))
    return;
  if (getMaxKI(g, ch) * attperc > char_get_charge(ch))
    attperc = (long double)(char_get_charge(ch)) / (long double)(getMaxKI(g, ch));
  if (!check_points(g, ch, getMaxKI(g, ch) * minimum, 0))
    return;
  if (arg.empty() || !(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (char_get_fighting(g, ch) && char_get_in_room(char_get_fighting(g, ch)) == char_get_in_room(ch)) {
      vict = char_get_fighting(g, ch);
    } else {
      send_to_char(g, ch, "No one around here by that name.\r\n");
      return;
    }
  }
  if (!can_kill(g, ch, vict, nullptr, 3))
    return;
  if (handle_defender(g, vict, ch))
    vict = char_get_defender(g, vict);

  prob = init_skill(g, ch, b.skill);
  perc = rand_number(g, 1, 115);
  if (prob < perc - 20) {
    act(g, b.fail_to_char, true, ch, nullptr, nullptr, TO_CHAR);
    act(g, b.fail_to_room, true, ch, nullptr, nullptr, TO_ROOM);
    hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
    pcost(g, ch, attperc, 0);
    if (b.cooldown)
      handle_cooldown(g, ch, b.cooldown);
    improve_skill(g, ch, b.skill, 2);
    return;
  }
  if (b.gather)
    b.gather(g, ch);

  int dista = 15 - (char_get_aff_abils_intel(ch) * 0.1);
  if (char_get_skill(ch, b.skill) >= 100) {
    dista -= 3;
  } else if (char_get_skill(ch, b.skill) >= 60) {
    dista -= 2;
  } else if (char_get_skill(ch, b.skill) >= 40) {
    dista -= 1;
  }

  struct obj_data *obj = read_object(g, b.vnum);
  obj_to_room(g, obj, char_get_in_room(vict));
  if (b.charge_bonus)
    char_set_charge(ch, char_get_charge(ch) + (getEffMaxPL(g, ch) / 10));
  obj_set_target(obj, vict);
  obj_set_kicharge(obj, damtype(g, ch, b.damtype, prob, attperc));
  obj_set_kitype(obj, b.skill);
  obj_set_user(obj, ch);
  obj_set_distance(obj, dista);
  pcost(g, ch, attperc, 0);
  act(g, b.formed.to_char, b.formed.hide, ch, nullptr, vict, TO_CHAR);
  act(g, b.formed.to_vict, b.formed.hide, ch, nullptr, vict, TO_VICT);
  act(g, b.formed.to_room, b.formed.hide, ch, nullptr, vict, TO_NOTVICT);
  if (b.cooldown)
    handle_cooldown(g, ch, b.cooldown);
  improve_skill(g, ch, b.skill, 2);
}

} // namespace

ACMD(do_geno) { form_charged_ball(g, ch, argument, GENOCIDE); }

ACMD(do_genki) { form_charged_ball(g, ch, argument, GENKIDAMA); }

ACMD(do_spiritball) { perform_ki_attack(g, ch, argument, SPIRITBALL); }

ACMD(do_deathball) { perform_ki_attack(g, ch, argument, DEATHBALL); }

ACMD(do_hellspear)
{
  int perc, dge = 2, count = 0, skill;
  int64_t dmg;
  double attperc = .3, minimum = .1;
  std::string arg2;

  std::string_view rest = argument;
  arg2                  = take_arg(rest);

  /* Can they do the technique? */

  if (!can_grav(g, ch)) {
    return;
  }

  if (!check_skill(g, ch, SKILL_HELLSPEAR)) {
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

  if (room_flagged(g, char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(g, ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  /* Passed sanity checks for doing the technique */

  skill = init_skill(g, ch, SKILL_HELLSPEAR); /* Set skill value */

  for (char_data *vict : room_people(g, char_get_in_room(ch))) {
    if (vict == ch) {
      continue;
    }
    if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
      continue;
    }
    if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
      continue;
    }
    if (char_mob_flagged(vict, MOB_NOKILL)) {
      continue;
    }
    count += 1;
  }
  if (count <= 0) {
    send_to_char(g, ch, "There is no one worth targeting around.\r\n");
    return;
  } /* No one worth targeting */
  else {
    perc = chance_to_hit(g, ch);

    if (skill < perc) {
      act(g, "@WYou fly up higher in the air while holding your hand up above your head. Your charged ki is condensed and materialized in the grasp of your raised hand forming a spear of energy. The spear disolves as you screw up the technique!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@W flies up higher in the air while holding $s hand up above $s head. @C$n@W's charged ki is condensed and materialized in the grasp of $s raised hand forming a spear of energy. The spear disolves as $e screws up the technique!@n", true, ch, nullptr, nullptr, TO_ROOM);
      pcost(g, ch, attperc, 0);

      improve_skill(g, ch, SKILL_HELLSPEAR, 0);
      return;
    }

    act(g, "@WYou fly up higher in the air while holding your hand above your head. Your charged ki is condensed and materialized in the grasp of your raised hand forming a spear of energy. Grinning evily you aim the spear at the ground below and throw it. As the red spear of energy slams into the ground your laughter rings throughout the area, and the @rH@Re@Dl@Wl @rS@Rp@De@Wa@wr B@rl@Ra@Ds@wt@W erupts with a roar!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@W flies up higher in the air while holding $s hand above $s head. @C$n@W's charged ki is condensed and materialized in the grasp of $s raised hand forming a spear of energy. Grinning evily $e aims the spear at the ground below and throws it. As the red spear of energy slams into the ground $s laughter rings throughout the area, and the @rH@Re@Dl@Wl @rS@Rp@De@Wa@wr B@rl@Ra@Ds@wt@W erupts with a roar!@n", true, ch, nullptr, nullptr, TO_ROOM);

    dmg = damtype(g, ch, 33, skill, attperc);

    for (char_data *vict : room_people(g, char_get_in_room(ch))) {
      if (vict == ch) {
        continue;
      }
      if (char_mob_flagged(vict, MOB_NOKILL)) {
        continue;
      }
      if (char_aff_flagged(vict, AFF_SPIRIT) && !char_is_npc(vict)) {
        continue;
      }
      if (char_get_level(vict) <= 8 && !char_is_npc(vict)) {
        continue;
      }
      dge = handle_dodge(g, vict);

      if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
          (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
        act(g, "@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr, vict, TO_CHAR);
        act(g, "@cYou disappear, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr, vict, TO_VICT);
        act(g, "@C$N@c disappears, avoiding the explosion before reappearing elsewhere!@n", false, ch, nullptr, vict, TO_NOTVICT);
        char_aff_set(vict, AFF_ZANZOKEN, false);
        pcost(g, vict, 0, getEffMaxPL(g, vict) / 200);
        continue;
      }
      if (dge + rand_number(g, -10, 5) > skill) {
        act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
        act(g, "@WYou manage to escape the explosion!@n", true, ch, nullptr, vict, TO_VICT);
        act(g, "@c$N@W manages to escape the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
        hurt(g, 0, 0, ch, vict, nullptr, 0, 1);
        improve_skill(g, vict, SKILL_DODGE, 0);
        continue;
      }
      act(g, "@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@RYou are caught by the explosion!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@R$N@r is caught by the explosion!@n", true, ch, nullptr, vict, TO_NOTVICT);
      if (!char_aff_flagged(vict, AFF_FLYING) && char_get_position(vict) == POS_STANDING && rand_number(g, 1, 4) == 4) {
        handle_knockdown(g, vict);
      }
      hurt(g, 0, 0, ch, vict, nullptr, dmg, 1);
      continue;
    } /* Hitting targets! */

    pcost(g, ch, attperc, 0);

    improve_skill(g, ch, SKILL_HELLSPEAR, 0);
    handle_cooldown(g, ch, 5);
    return;
  } /* We have targets! Attempt to kills them! */
}

ACMD(do_crusher) { perform_ki_attack(g, ch, argument, CRUSHER); }

/* Chimera */
ACMD(do_blessedhammer) { perform_ki_attack(g, ch, argument, BLESSED_HAMMER); }

ACMD(do_kiball) { perform_ki_attack(g, ch, argument, KIBALL); }
