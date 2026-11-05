/* grapple.c: grappling and other physical actions
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/act.other.h"
#include "dbat/consts/maximums.h"
#include "dbat/consts/shadowdragons.h"
#include "dbat/consts/auction.h"
#include "dbat/consts/aligns.h"
#include "dbat/utils.h"
#include "dbat/flags.h"
#include <sys/stat.h>
#include "dbat/config.h"
#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/object_utils.h"
#include "dbat/zone_utils.h"
#include "dbat/room_utils.h"
#include "dbat/stringutils.h"
#include "dbat/search.h"
#include "dbat/random.h"
#include "dbat/affect.h"
#include "dbat/relocate.h"
#include "dbat/extract.h"
#include "dbat/races.h"
#include "dbat/fileop.h"
#include "dbat/comm.h"
#include "dbat/handler.h"
#include "dbat/broadcast.h"
#include "dbat/combat.h"
#include "dbat/act.misc.h"
#include "dbat/weather.h"
#include "dbat/act.item.h"
#include "dbat/act.wizard.h"
#include "dbat/act.informative.h"
#include "dbat/act.movement.h"
#include "dbat/obj_edit.h"
#include "dbat/graph.h"
#include "dbat/alias.h"
#include "dbat/spells.h"
#include "dbat/interpreter.h"
#include "dbat/fight.h"
#include "dbat/class.h"
#include "dbat/shop.h"
#include "dbat/feats.h"
#include "dbat/guild.h"
#include "dbat/dg_scripts.h"
#include "dbat/objsave.h"
#include "dbat/mail.h"
#include "other_internal.h"
#include "dbat/game_world.h"

ACMD(do_grapple)
{

  /*if (IS_NPC(ch))
  return;*/

  if (!know_skill(g, ch, SKILL_GRAPPLE)) {
    return;
  }

  if (char_plr_flagged(ch, PLR_THANDW)) {
    send_to_char(g, ch, "Your are too busy wielding your weapon with two hands!\r\n");
    return;
  }

  if (char_get_absorbing(ch)) {
    send_to_char(g, ch, "You are currently absorbing from someone!\r\n");
    return;
  }
  if (char_get_absorbby(ch)) {
    send_to_char(g, ch, "You are currently being absorbed by someone! Try 'escape'!\r\n");
    return;
  }

  if (char_get_grappling(ch) != nullptr) {
    act(g, "@RYou stop grappling with @r$N@R!@n", true, ch, 0, char_get_grappling(ch), TO_CHAR);
    act(g, "@r$n@R stops grappling with @rYOU!!@n", true, ch, 0, char_get_grappling(ch), TO_VICT);
    act(g, "@r$n@R stops grappling with @r$N@R!@n", true, ch, 0, char_get_grappling(ch), TO_NOTVICT);
    char_set_grap(char_get_grappling(ch), -1);
    char_set_grappled(char_get_grappling(ch), nullptr);
    char_set_grappling(ch, nullptr);
    char_set_grap(ch, -1);
    return;
  }

  if (char_get_grappled(ch) != nullptr) {
    send_to_char(g, ch, "You are currently a victim of grappling! Try 'escape' to break free!\r\n");
    return;
  }

  if (!char_has_arms(ch)) {
    send_to_char(g, ch, "You have no available arms!\r\n");
    return;
  }

  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (!*arg || !*arg2) {
    send_to_char(g, ch, "Syntax: grapple (target) (hold | choke | grab)\r\n");
    return;
  }

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (!char_get_fighting(ch)) {
      send_to_char(g, ch, "That target isn't here.\r\n");
      return;
    }
    vict = char_get_fighting(ch);
  }

  if (!can_kill(g, ch, vict, nullptr, 0)) {
    return;
  }

  if (char_aff_flagged(vict, AFF_KNOCKED)) {
    send_to_char(g, ch, "They are unconcious. What would be the point?\r\n");
    return;
  }

  if (char_get_grappled(vict)) {
    send_to_char(g, ch, "They are currently in someone else's grasp!\r\n");
    return;
  }

  if (char_get_absorbby(vict)) {
    send_to_char(g, ch, "They are currently in someone else's grasp!\r\n");
    return;
  }

  if (char_get_absorbing(vict)) {
    send_to_char(g, ch, "They are currently absorbing from someone!\r\n");
    return;
  }

  if (!strcasecmp("hold", arg2) || !strcasecmp("choke", arg2) || !strcasecmp("grab", arg2) || !strcasecmp("wrap", arg2)) {
    int perc = char_get_skill(ch, SKILL_GRAPPLE), prob = axion_dice(g, 0), cost = getMaxST(g, ch) / 100;

    if ((getCurST(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough stamina to grapple!\r\n");
      return;
    }

    if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
        (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
      if (!char_aff_flagged(ch, AFF_ZANZOKEN) || (char_aff_flagged(ch, AFF_ZANZOKEN) && char_get_speedi(g, ch) + rand_number(g, 1, 5) < char_get_speedi(g, vict) + rand_number(g, 1, 5))) {
        reveal_hiding(g, ch, 0);
        act(g, "@C$N@c disappears, avoiding your grapple attempt before reappearing!@n", false, ch, 0, vict, TO_CHAR);
        act(g, "@cYou disappear, avoiding @C$n's@c grapple attempt before reappearing!@n", false, ch, 0, vict, TO_VICT);
        act(g, "@C$N@c disappears, avoiding @C$n's@c grapple attempt before reappearing!@n", false, ch, 0, vict, TO_NOTVICT);
        if (char_aff_flagged(ch, AFF_ZANZOKEN)) {
          char_aff_set(ch, AFF_ZANZOKEN, false);
        }
        char_aff_set(vict, AFF_ZANZOKEN, false);
        decCurST(g, ch, cost, 0);
        char_set_wait(ch, PULSE_4SEC);
        return;
      }
      reveal_hiding(g, ch, 0);
      act(g, "@C$N@c disappears, trying to avoid your grapple but your zanzoken is faster!@n", false, ch, 0, vict, TO_CHAR);
      act(g, "@cYou zanzoken to avoid the grapple attempt but @C$n's@c zanzoken is faster!@n", false, ch, 0, vict, TO_VICT);
      act(g, "@C$N@c disappears, trying to avoid @C$n's@c grapple attempt but @C$n's@c zanzoken is faster!@n", false, ch, 0, vict, TO_NOTVICT);
      char_aff_set(vict, AFF_ZANZOKEN, false);
      char_aff_set(ch, AFF_ZANZOKEN, false);
    }

    if (char_get_speedi(g, ch) > char_get_speedi(g, vict) * 2) {
      perc += 5;
    } else if (char_get_speedi(g, ch) > char_get_speedi(g, vict)) {
      perc += 2;
    } else if (char_get_speedi(g, ch) * 2 < char_get_speedi(g, vict)) {
      perc -= 5;
    } else if (char_get_speedi(g, ch) < char_get_speedi(g, vict)) {
      perc -= 2;
    }

    if ((getCurPL(g, ch) * 0.02) * char_get_aff_abils_str(ch) < (getCurPL(g, vict) * 0.01) * char_get_aff_abils_str(vict)) {
      reveal_hiding(g, ch, 0);
      act(g, "@RYou try to grapple with @r$N@R, but $E manages to overpower you!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@r$n@R tries to grapple with YOU, but you manage to overpower $m!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@r$n@R tries to grapple with @r$N@R, but $E manages to overpower @r$n@R!@n", true, ch, 0, vict, TO_NOTVICT);
      decCurST(g, ch, cost, 0);
      improve_skill(g, ch, SKILL_GRAPPLE, 1);
      char_set_wait(ch, PULSE_4SEC);
      return;
    }
    if ((getCurPL(g, ch) * 0.01) * char_get_aff_abils_str(ch) < (getCurPL(g, vict) * 0.01) * char_get_aff_abils_str(vict) && rand_number(g, 1, 4) == 1) {
      reveal_hiding(g, ch, 0);
      act(g, "@RYou try to grapple with @r$N@R, but $E manages to overpower you!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@r$n@R tries to grapple with YOU, but you manage to overpower $m!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@r$n@R tries to grapple with @r$N@R, but $E manages to overpower @r$n@R!@n", true, ch, 0, vict, TO_NOTVICT);
      decCurST(g, ch, cost, 0);
      improve_skill(g, ch, SKILL_GRAPPLE, 1);
      char_set_wait(ch, PULSE_4SEC);
      return;
    }
    if (perc < prob) {
      reveal_hiding(g, ch, 0);
      act(g, "@RYou try to grapple with @r$N@R, but $E manages to avoid it!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@r$n@R tries to grapple with YOU, but you manage to avoid it!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@r$n@R tries to grapple with @r$N@R, but $E manages to avoid it!@n", true, ch, 0, vict, TO_NOTVICT);
      decCurST(g, ch, cost, 0);
      improve_skill(g, ch, SKILL_GRAPPLE, 1);
      char_set_wait(ch, PULSE_4SEC);
      return;
    }
    if (!char_has_arms(vict) && !strcasecmp("grab", arg2)) {
      send_to_char(g, ch, "They don't even have an arm to grab onto!\r\n");
      return;
    }
    if (!strcasecmp("hold", arg2)) {
      reveal_hiding(g, ch, 0);
      act(g, "@RYou rush at @r$N@R and manage to get $M in a hold from behind!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@r$n@R rushes at YOU and manages to get you in a hold from behind!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@r$n@R rushes at @r$N@R and manages to get $M in a hold from behind!@n", true, ch, 0, vict, TO_NOTVICT);

      /* Let's grapple! */
      char_set_grappling(ch, vict);
      char_set_grap(ch, 1);
      char_set_grappled(vict, ch);
      char_set_grap(vict, 1);
      /* Let's grapple! */

      decCurST(g, ch, cost, 0);
      improve_skill(g, ch, SKILL_GRAPPLE, 1);
      char_set_wait(ch, PULSE_4SEC);
      return;
    }
    if (!strcasecmp("choke", arg2)) {
      reveal_hiding(g, ch, 0);
      act(g, "@RYou rush at @r$N@R and manage to grab $S throat with both hands!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@r$n@R rushes at YOU and manages to grab your throat with both hands!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@r$n@R rushes at @r$N@R and manages to grab $S throat with both hands!@n", true, ch, 0, vict, TO_NOTVICT);

      /* Let's grapple! */
      char_set_grappling(ch, vict);
      char_set_grap(ch, 2);
      char_set_grappled(vict, ch);
      char_set_grap(vict, 2);
      /* Let's grapple! */

      decCurST(g, ch, cost, 0);
      improve_skill(g, ch, SKILL_GRAPPLE, 1);
      char_set_wait(ch, PULSE_4SEC);
      return;
    }
    if (!strcasecmp("wrap", arg2)) {
      if (!char_is_majin(ch)) {
        send_to_char(g, ch, "Your body is not flexible enough to wrap around a target!\r\n");
        return;
      }

      act(g, "@MMoving quickly you stretch your body out and wrap it around the length of @c$N's@M body! You tighten your body until you begin crushing @c$N@M!", true, ch, 0, vict, TO_CHAR);
      act(g, "@C$n@M quickly stretches out $s body and wraps it around @RYOU@M! You feel $s body begin to crush your own!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@C$n@M quickly stretches out $s body and wraps it around @c$N@M! It appears that @c$N's@M body is being crushed slowly!@n", true, ch, 0, vict, TO_NOTVICT);

      /* Let's grapple! */
      char_set_grappling(ch, vict);
      char_set_grap(ch, 4);
      char_set_grappled(vict, ch);
      char_set_grap(vict, 4);
      /* Let's grapple! */

      decCurST(g, ch, cost, 0);
      improve_skill(g, ch, SKILL_GRAPPLE, 1);
      char_set_wait(ch, PULSE_4SEC);
      return;
    }
    if (!strcasecmp("grab", arg2)) {
      reveal_hiding(g, ch, 0);
      act(g, "@RYou rush at @r$N@R and manage to lock your arm onto $S!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@r$n@R rushes at YOU and manages to lock $s arm onto your's!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@r$n@R rushes at @r$N@R and manages to lock $s arm onto @r$N's@R!@n", true, ch, 0, vict, TO_NOTVICT);

      /* Let's grapple! */
      char_set_grappling(ch, vict);
      char_set_grap(ch, 3);
      char_set_grappled(vict, ch);
      char_set_grap(vict, 3);
      /* Let's grapple! */

      if (!char_plr_flagged(vict, PLR_THANDW)) {
        char_plr_set(vict, PLR_THANDW, false);
      }

      decCurST(g, ch, cost, 0);
      improve_skill(g, ch, SKILL_GRAPPLE, 1);
      char_set_wait(ch, PULSE_4SEC);
      return;
    }
  } else { /* You need to learn proper syntax! */
    send_to_char(g, ch, "Syntax: grapple (target) (hold | choke | grab | wrap)\r\n");
    return;
  }
}

ACMD(do_trip)
{

  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict = nullptr;

  one_argument(argument, arg);

  if (!check_skill(g, ch, SKILL_TRIP) && !char_is_npc(ch)) {
    return;
  }

  int64_t cost = getEffMaxPL(g, ch) / 200;

  if (cost > (getCurST(g, ch))) {
    send_to_char(g, ch, "You don't have enough stamina.\r\n");
    return;
  }

  int perc = init_skill(g, ch, SKILL_TRIP), prob = rand_number(g, 1, 114);

  if (perc == 0) {
    perc = char_get_level(ch) + rand_number(g, 1, 10);
  }

  vict = nullptr;
  vict = nullptr;
  if (!*arg || !(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (char_get_fighting(ch) && char_get_in_room(char_get_fighting(ch)) == char_get_in_room(ch)) {
      vict = char_get_fighting(ch);
    } else {
      send_to_char(g, ch, "That target isn't here.\r\n");
      return;
    }
  }

  if (!can_kill(g, ch, vict, nullptr, 0)) {
    return;
  }

  if (vict != nullptr) {
    if (char_aff_flagged(vict, AFF_FLYING)) {
      send_to_char(g, ch, "They are flying and are not on their feet!\r\n");
      return;
    }
    if (char_get_position(vict) == POS_SITTING) {
      send_to_char(g, ch, "They are not on their feet!\r\n");
      return;
    }
    if (char_plr_flagged(vict, PLR_HEALT)) {
      send_to_char(g, ch, "They are inside a healing tank!\r\n");
      return;
    }

    if (char_get_speedi(g, ch) > char_get_speedi(g, vict) * 2) {
      perc += 5;
    } else if (char_get_speedi(g, ch) > char_get_speedi(g, vict)) {
      perc += 2;
    } else if (char_get_speedi(g, ch) * 2 < char_get_speedi(g, vict)) {
      perc -= 5;
    } else if (char_get_speedi(g, ch) < char_get_speedi(g, vict)) {
      perc -= 2;
    }

    if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
        (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
      if (!char_aff_flagged(ch, AFF_ZANZOKEN) || (char_aff_flagged(ch, AFF_ZANZOKEN) && char_get_speedi(g, ch) + rand_number(g, 1, 5) < char_get_speedi(g, vict) + rand_number(g, 1, 5))) {
        reveal_hiding(g, ch, 0);
        act(g, "@C$N@c disappears, avoiding your trip before reappearing!@n", false, ch, 0, vict, TO_CHAR);
        act(g, "@cYou disappear, avoiding @C$n's@c trip before reappearing!@n", false, ch, 0, vict, TO_VICT);
        act(g, "@C$N@c disappears, avoiding @C$n's@c trip before reappearing!@n", false, ch, 0, vict, TO_NOTVICT);
        if (char_aff_flagged(ch, AFF_ZANZOKEN)) {
          char_aff_set(ch, AFF_ZANZOKEN, false);
        }
        char_aff_set(vict, AFF_ZANZOKEN, false);
        decCurST(g, ch, cost, 0);
        char_set_wait(ch, PULSE_4SEC);
        return;
      }
      reveal_hiding(g, ch, 0);
      act(g, "@C$N@c disappears, trying to avoid your trip but your zanzoken is faster!@n", false, ch, 0, vict, TO_CHAR);
      act(g, "@cYou zanzoken to avoid the trip but @C$n's@c zanzoken is faster!@n", false, ch, 0, vict, TO_VICT);
      act(g, "@C$N@c disappears, trying to avoid @C$n's@c trip but @C$n's@c zanzoken is faster!@n", false, ch, 0, vict, TO_NOTVICT);
      char_aff_set(vict, AFF_ZANZOKEN, false);
      char_aff_set(ch, AFF_ZANZOKEN, false);
    }

    if (perc < prob) { /* Fail! */
      reveal_hiding(g, ch, 0);
      act(g, "@mYou move to trip $N@m, but you screw up and $E keeps $S footing!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@m$n@m moves to trip YOU, but $e screws up and you manage to keep your footing!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@m$n@m moves to trip $N@m, but $e screws up and $N@m manages to keep $S footing!@n", true, ch, 0, vict, TO_NOTVICT);
      improve_skill(g, ch, SKILL_TRIP, 0);
      decCurST(g, ch, cost, 0);
      char_set_wait(ch, PULSE_4SEC);
      if (char_get_fighting(ch) == nullptr) {
        set_fighting(g, ch, vict);
      } else if (char_get_fighting(ch) != vict) {
        set_fighting(g, ch, vict);
      }
      if (char_get_fighting(vict) == nullptr) {
        set_fighting(g, vict, ch);
      } else if (char_get_fighting(vict) != ch) {
        set_fighting(g, vict, ch);
      }
      return;
    } else { /* Success! */
      reveal_hiding(g, ch, 0);
      act(g, "@mYou move to trip $N@m, and manage to knock $M off $S feet!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@m$n@m moves to trip YOU, and manages to knock you off your feet!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@m$n@m moves to trip $N@m, and manages to knock $N@m off $S feet!@n", true, ch, 0, vict, TO_NOTVICT);
      improve_skill(g, ch, SKILL_TRIP, 0);
      decCurST(g, ch, cost, 0);
      char_set_position(vict, POS_SITTING);
      char_set_wait(ch, PULSE_4SEC);
      if (char_get_fighting(ch) == nullptr) {
        set_fighting(g, ch, vict);
      }
      if (char_get_fighting(vict) == nullptr) {
        set_fighting(g, vict, ch);
      }
      return;
    }
  } else {
    send_to_char(g, ch, "ERROR: Report to Iovan.\r\n");
    return;
  }
}

ACMD(do_rip)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_is_npc(ch))
    return;

  if (!*arg) {
    send_to_char(g, ch, "Rip the tail off who?\r\n");
    return;
  }

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "That target isn't here.\r\n");
    return;
  }

  if (!char_plr_flagged(vict, PLR_TAIL) && !char_plr_flagged(vict, PLR_STAIL)) {
    send_to_char(g, ch, "They do not have a tail to rip off!\r\n");
    return;
  }

  if (ch != vict && char_get_position(ch) > POS_SLEEPING) {
    if ((getCurST(g, ch)) < getMaxST(g, ch) / 20) {
      send_to_char(g, ch, "You are too tired to manage to grab their tail!\r\n");
      return;
    }
    if (char_get_speedi(g, ch) > char_get_speedi(g, vict)) {
      decCurST(g, ch, getMaxST(g, ch) / 20, 0);
      if (getCurPL(g, ch) > getCurPL(g, vict) * 2) {
        reveal_hiding(g, ch, 0);
        act(g, "@rYou rush at @R$N@r and grab $S tail! With a powerful tug you pull it off!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@R$n@r rushes at YOU and grabs your tail! With a powerful tug $e pulls it off!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@R$n@R rushes at @R$N@r and grab $S tail! With a powerful tug $e pulls it off!@n", true, ch, 0, vict, TO_NOTVICT);
        char_lose_tail(g, vict);
        return;
      }
      reveal_hiding(g, ch, 0);
      act(g, "@rYou rush at @R$N@r and grab $S tail! You are too weak to pull it off though!@n", true, ch, 0, vict, TO_CHAR);
      act(g, "@R$n@r rushes at YOU and grabs your tail! $e is too weak to pull it off though!@n", true, ch, 0, vict, TO_VICT);
      act(g, "@R$n@R rushes at @R$N@r and grab $S tail! $e is too weak to pull it off though!@n", true, ch, 0, vict, TO_NOTVICT);
      return;
    }
    decCurST(g, ch, getMaxST(g, ch) / 20, 0);
    reveal_hiding(g, ch, 0);
    act(g, "@rYou rush at @R$N@r and try to grab $S tail, but fail!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@R$n@r rushes at YOU and tries to grab your tail, but fails!@n", true, ch, 0, vict, TO_VICT);
    act(g, "@R$n@R rushes at @R$N@r and tries to grab $S tail, but fails!@n", true, ch, 0, vict, TO_NOTVICT);
    return;
  }
  if (ch == vict) {
    reveal_hiding(g, ch, 0);
    act(g, "@rYou grab your own tail and yank it off!@n", true, ch, 0, 0, TO_CHAR);
    act(g, "@R$n@r grabs $s own tail and yanks it off!@n", true, ch, 0, 0, TO_ROOM);
    char_lose_tail(g, vict);
  } else {
    if ((getCurST(g, ch)) < getMaxST(g, ch) / 20) {
      send_to_char(g, ch, "You are too tired to manage to grab their tail!\r\n");
      return;
    }
    decCurST(g, ch, getMaxKI(g, ch) / 20, 0);
    reveal_hiding(g, ch, 0);
    act(g, "@rYou reach and grab @R$N's@r tail! With a powerful tug you pull it off!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@RYou feel your tail pulled off!@n", true, ch, 0, vict, TO_VICT);
    act(g, "@R$n@R reaches and grabs @R$N's@r tail! With a powerful tug $e pulls it off!@n", true, ch, 0, vict, TO_NOTVICT);
    char_lose_tail(g, vict);
    return;
  }
}

ACMD(do_drag)
{
  struct char_data *vict = nullptr;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_is_npc(ch))
    return;

  if (char_get_drag(ch)) {
    vict = char_get_drag(ch);
    char_set_drag(ch, nullptr);
    char_set_dragged(vict, nullptr);
    act(g, "@wYou stop dragging @C$N@W.@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@C$n@W stops dragging @c$N@W.@n", true, ch, 0, vict, TO_ROOM);
    return;
  }

  if (char_plr_flagged(ch, PLR_PILOTING)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }

  if (char_get_carrying(ch)) {
    send_to_char(g, ch, "You are busy carrying someone at the moment.\r\n");
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "Who do you want to drag?\r\n");
    return;
  }

  if (char_get_fighting(ch)) {
    send_to_char(g, ch, "You are a bit busy fighting right now!\r\n");
    return;
  }

  if (rroom_sect(g, char_get_in_room(ch)) == SECT_WATER_NOSWIM || rroom_sect(g, char_get_in_room(ch)) == SECT_WATER_SWIM) {
    send_to_char(g, ch, "You decide to not be a tugboat instead.\r\n");
    return;
  }

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "Drag who?\r\n");
    return;
  }

  if (vict == ch) {
    send_to_char(g, ch, "You can't drag yourself.\r\n");
    return;
  }

  if (char_get_dragged(vict)) {
    send_to_char(g, ch, "They are already being dragged!\r\n");
    return;
  }

  if (char_is_npc(vict) && char_mob_flagged(vict, MOB_NOKILL)) {
    send_to_char(g, ch, "They are not to be touched!\r\n");
    return;
  }

  if (char_get_position(vict) != POS_SLEEPING) {
    reveal_hiding(g, ch, 0);
    act(g, "@wYou try to grab and pull @C$N@W with you, but $E resists!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@C$n@W tries to grab and pull you! However you resist!@n", true, ch, 0, vict, TO_VICT);
    act(g, "@C$n@W tries to grab and pull @c$N@W but $E resists!@n", true, ch, 0, vict, TO_NOTVICT);
    if (char_is_npc(vict) && !char_get_fighting(vict)) {
      set_fighting(g, vict, ch);
    }
    return;
  }
  if (char_get_pc_weight(g, vict) + char_get_carry_weight(vict) > getMaxCarryWeight(g, ch)) {
    reveal_hiding(g, ch, 0);
    act(g, "@wYou try to grab and pull @C$N@W with you, but $E is too heavy!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@C$n@W tries to grab and pull @c$N@W but $E is too heavy!@n", true, ch, 0, vict, TO_ROOM);
    return;
  }
  reveal_hiding(g, ch, 0);
  act(g, "@wYou grab and start dragging @C$N@W.@n", true, ch, 0, vict, TO_CHAR);
  act(g, "@C$n@W grabs and starts dragging @c$N@W.@n", true, ch, 0, vict, TO_NOTVICT);
  char_set_drag(ch, vict);
  char_set_dragged(vict, ch);
  if (!char_aff_flagged(vict, AFF_KNOCKED) && !char_aff_flagged(vict, AFF_SLEEP) && rand_number(g, 1, 3)) {
    send_to_char(g, vict, "You feel your sleeping body being moved.\r\n");
    if (char_is_npc(vict) && !char_get_fighting(vict)) {
      set_fighting(g, vict, ch);
    }
  }
}

ACMD(do_stop)
{

  if (char_is_npc(ch))
    return;

  if (!char_get_fighting(ch)) {
    send_to_char(g, ch, "You are not even fighting!\r\n");
    return;
  }
  act(g, "@CYou move out of your fighting posture.@n", true, ch, 0, 0, TO_CHAR);
  act(g, "@c$n@C moves out of $s fighting posture.@n", true, ch, 0, 0, TO_ROOM);
  stop_fighting(g, ch);
  char_set_wait(ch, PULSE_2SEC);
  return;
}

ACMD(do_escape)
{

  if (!char_get_absorbby(ch) && !char_get_grappled(ch)) {
    send_to_char(g, ch, "You are not in anyone's grasp!\r\n");
    return;
  }
  int num = char_get_aff_abils_str(ch);

  struct char_data *offender = char_get_absorbby(ch) ? char_get_absorbby(ch) : char_get_grappled(ch);
  int64_t opl                = getCurPL(g, offender);
  int64_t dpl                = getCurPL(g, ch);
  int skill                  = char_get_absorbby(ch) ? char_get_skill(char_get_absorbby(ch), SKILL_ABSORB) : char_get_skill(char_get_grappled(ch), SKILL_GRAPPLE);

  if (dpl > opl * 10) {
    num += rand_number(g, 10, 15);
  } else if (dpl > opl * 5) {
    num += rand_number(g, 6, 10);
  } else if (dpl > opl * 2) {
    num += rand_number(g, 4, 8);
  } else if (dpl > opl) {
    num += rand_number(g, 2, 5);
  } else if (dpl * 10 <= opl) {
    skill -= rand_number(g, 10, 15);
  } else if (dpl * 5 <= opl) {
    skill -= rand_number(g, 6, 10);
  } else if (dpl * 2 <= opl) {
    skill -= rand_number(g, 4, 8);
  } else if (dpl < opl) {
    skill -= rand_number(g, 2, 5);
  }

  bool escaped = (skill - num) < axion_dice(g, 0);

  if (char_get_absorbby(ch)) {

    if (escaped) {
      act(g, "@c$N@W manages to break loose of @C$n's@W hold!@n", true, char_get_absorbby(ch), 0, ch, TO_NOTVICT);
      act(g, "@WYou manage to break loose of @C$n's@W hold!@n", true, char_get_absorbby(ch), 0, ch, TO_VICT);
      act(g, "@c$N@W manages to break loose of your hold!@n", true, char_get_absorbby(ch), 0, ch, TO_CHAR);
      if (char_get_fighting(ch) == nullptr) {
        set_fighting(g, ch, char_get_absorbby(ch));
      }
      if (char_get_fighting(char_get_absorbby(ch)) == nullptr) {
        set_fighting(g, char_get_absorbby(ch), ch);
      }
      char_set_absorbing(char_get_absorbby(ch), nullptr);
      char_set_absorbby(ch, nullptr);
    } else {
      act(g, "@c$N@W struggles to break loose of @C$n's@W hold!@n", true, char_get_absorbby(ch), 0, ch, TO_NOTVICT);
      act(g, "@WYou struggle to break loose of @C$n's@W hold!@n", true, char_get_absorbby(ch), 0, ch, TO_VICT);
      act(g, "@c$N@W struggles to break loose of your hold!@n", true, char_get_absorbby(ch), 0, ch, TO_CHAR);
      if (rand_number(g, 1, 3) == 3) {
        int64_t dmg = getEffMaxPL(g, ch) * 0.025;
        hurt(g, 0, 0, ch, char_get_absorbby(ch), nullptr, dmg, 0);
        if (char_get_position(char_get_absorbby(ch)) == POS_SLEEPING) {
          act(g, "@c$N@W manages to break loose of @C$n's@W hold!@n", true, char_get_absorbby(ch), 0, ch, TO_NOTVICT);
          act(g, "@WYou manage to break loose of @C$n's@W hold!@n", true, char_get_absorbby(ch), 0, ch, TO_VICT);
          act(g, "@c$N@W manages to break loose of your hold!@n", true, char_get_absorbby(ch), 0, ch, TO_CHAR);
          char_set_absorbing(char_get_absorbby(ch), nullptr);
          char_set_absorbby(ch, nullptr);
        }
      }
      char_set_wait(ch, PULSE_2SEC);
    }
  }

  if (char_get_grappled(ch)) {

    if (escaped) {
      if (char_get_grap(char_get_grappled(ch)) == 4) {
        act(g, "@c$N@M flexes with all $S might and causes your body to explode outward into gooey chunks!@n", true, char_get_grappled(ch), 0, ch, TO_CHAR);
        act(g, "@MYou flex with all your might and cause @C$n's@M body to explode outward into gooey chunks!@n", true, char_get_grappled(ch), 0, ch, TO_VICT);
        act(g, "@c$N@M flexes with all $S might and causes @C$n's@M body to explode outward into gooey chunks!@n", true, char_get_grappled(ch), 0, ch, TO_NOTVICT);

        act(g, "@MYou reform your body mere moments later.@n", true, char_get_grappled(ch), 0, 0, TO_CHAR);
        act(g, "@C$n@M reforms $s body mere moments later.", true, char_get_grappled(ch), 0, 0, TO_ROOM);
      } else {
        act(g, "@c$N@W manages to break loose of @C$n's@W hold!@n", true, char_get_grappled(ch), 0, ch, TO_NOTVICT);
        act(g, "@WYou manage to break loose of @C$n's@W hold!@n", true, char_get_grappled(ch), 0, ch, TO_VICT);
        act(g, "@c$N@W manages to break loose of your hold!@n", true, char_get_grappled(ch), 0, ch, TO_CHAR);
      }
      if (char_get_fighting(ch) == nullptr) {
        set_fighting(g, ch, char_get_grappled(ch));
      }
      if (char_get_fighting(char_get_grappled(ch)) == nullptr) {
        set_fighting(g, char_get_grappled(ch), ch);
      }
      char_set_grap(char_get_grappled(ch), -1);
      char_set_grappling(char_get_grappled(ch), nullptr);
      char_set_grappled(ch, nullptr);
      char_set_grap(ch, -1);
    } else {
      act(g, "@c$N@W struggles to break loose of @C$n's@W hold!@n", true, char_get_grappled(ch), 0, ch, TO_NOTVICT);
      act(g, "@WYou struggle to break loose of @C$n's@W hold!@n", true, char_get_grappled(ch), 0, ch, TO_VICT);
      act(g, "@c$N@W struggles to break loose of your hold!@n", true, char_get_grappled(ch), 0, ch, TO_CHAR);
      if (rand_number(g, 1, 3) == 3) {
        int64_t dmg = getEffMaxPL(g, ch) * 0.025;
        hurt(g, 0, 0, ch, char_get_grappled(ch), nullptr, dmg, 0);
        if (char_get_position(char_get_grappled(ch)) == POS_SLEEPING) {
          act(g, "@c$N@W manages to break loose of @C$n's@W hold!@n", true, char_get_grappled(ch), 0, ch, TO_NOTVICT);
          act(g, "@WYou manage to break loose of @C$n's@W hold!@n", true, char_get_grappled(ch), 0, ch, TO_VICT);
          act(g, "@c$N@W manages to break loose of your hold!@n", true, char_get_grappled(ch), 0, ch, TO_CHAR);
          char_set_grap(char_get_grappled(ch), -1);
          char_set_grappling(char_get_grappled(ch), nullptr);
          char_set_grappled(ch, nullptr);
          char_set_grap(ch, -1);
        }
      }
      char_set_wait(ch, PULSE_2SEC);
    }
  }
}

ACMD(do_block)
{
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_is_npc(ch))
    return;

  if (!*arg) {
    if (!char_get_blocks(ch)) {
      send_to_char(g, ch, "You want to block who?\r\n");
      return;
    }
    if (char_get_blocks(ch)) {
      act(g, "@wYou stop blocking @c$N@w.@n", true, ch, 0, char_get_blocks(ch), TO_CHAR);
      act(g, "@C$n@w stops blocking you.@n", true, ch, 0, char_get_blocks(ch), TO_VICT);
      act(g, "@C$n@w stops blocking @c$N@w.@n", true, ch, 0, char_get_blocks(ch), TO_NOTVICT);
      vict = char_get_blocks(ch);
      char_set_blocked(vict, nullptr);
      char_set_blocks(ch, nullptr);
      return;
    }
  }

  if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "You do not see the target here.\r\n");
    return;
  }

  if (char_get_blocks(ch) == vict) {
    send_to_char(g, ch, "They are already blocked by you!\r\n");
    return;
  }

  if (ch == vict) {
    send_to_char(g, ch, "You can't block yourself, are you mental?\r\n");
    return;
  }

  if (char_get_blocked(vict)) {
    send_to_char(g, ch, "They are already blocked by someone else!\r\n");
    return;
  }

  if (char_get_blocks(ch)) {
    act(g, "@wYou stop blocking @c$N@w.@n", true, ch, 0, char_get_blocks(ch), TO_CHAR);
    act(g, "@C$n@w stops blocking you.@n", true, ch, 0, char_get_blocks(ch), TO_VICT);
    act(g, "@C$n@w stops blocking @c$N@w.@n", true, ch, 0, char_get_blocks(ch), TO_NOTVICT);
    struct char_data *oldv = char_get_blocks(ch);
    char_set_blocked(oldv, nullptr);
    char_set_blocks(ch, vict);
    char_set_blocked(vict, ch);
    reveal_hiding(g, ch, 0);
    act(g, "@wYou start blocking @c$N's@w escape.@n", true, ch, 0, char_get_blocks(ch), TO_CHAR);
    act(g, "@C$n@w starts blocking your escape.@n", true, ch, 0, char_get_blocks(ch), TO_VICT);
    act(g, "@C$n@w starts blocking @c$N's@w escape.@n", true, ch, 0, char_get_blocks(ch), TO_NOTVICT);
    return;
  }
  char_set_blocks(ch, vict);
  char_set_blocked(vict, ch);
  reveal_hiding(g, ch, 0);
  act(g, "@wYou start blocking @c$N's@w escape.@n", true, ch, 0, char_get_blocks(ch), TO_CHAR);
  act(g, "@C$n@w starts blocking your escape.@n", true, ch, 0, char_get_blocks(ch), TO_VICT);
  act(g, "@C$n@w starts blocking @c$N's@w escape.@n", true, ch, 0, char_get_blocks(ch), TO_NOTVICT);
  return;
}

ACMD(do_steal)
{

  if (!char_get_skill(ch, SKILL_SLEIGHT_OF_HAND) && slot_count(ch) + 1 <= char_get_skill_slots(ch)) {
    send_to_char(g, ch, "You learn the very veeeery basics of theft. Which is don't get caught.\r\n");
    char_set_skills_base(ch, SKILL_SLEIGHT_OF_HAND, 1);
  } else if (!char_get_skill(ch, SKILL_SLEIGHT_OF_HAND) && slot_count(ch) + 1 > char_get_skill_slots(ch)) {
    send_to_char(g, ch, "You can't learn any more skills and thus can not steal right now!\r\n");
    return;
  }

  struct char_data *vict;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int gold = 0, prob = char_get_skill(ch, SKILL_SLEIGHT_OF_HAND), perc = 0, eq_pos;

  two_arguments(argument, arg, arg2);

  if (!*arg) {
    send_to_char(g, ch, "An important basic of theft is actually having a victim!\r\n");
    return;
  }

  if (!(vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "Steal what from who?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(g, ch, "Come on now, that's rather stupid!\r\n");
    return;
  }
  if (!can_kill(g, ch, vict, nullptr, 0)) {
    return;
  }
  if (char_get_level(ch) <= 8) {
    send_to_char(g, ch, "You are trapped inside the newbie shield until level 9 and can't piss off those bigger and better than you. Awww...\r\n");
    return;
  }
  if (char_mob_flagged(vict, MOB_NOKILL) && char_get_admlevel(ch) == ADMLVL_NONE) {
    send_to_char(g, ch, "That isn't such a good idea...\r\n");
    return;
  }

  if ((getCurST(g, ch)) < (getMaxST(g, ch) / 40) + char_get_carry_weight(ch)) {
    send_to_char(g, ch, "You do not have enough stamina.\r\n");
    return;
  }

  if (!char_is_npc(vict) && char_get_skill(vict, SKILL_SPOT)) {
    perc = char_get_skill(vict, SKILL_SPOT);
    perc += char_get_aff_abils_intel(vict);
  } else {
    perc = rand_number(g, char_get_aff_abils_intel(vict), char_get_aff_abils_intel(vict) + 10);
    if (char_is_npc(vict))
      perc += char_get_level(vict) * 0.25;
  }

  if (char_get_position(vict) == POS_SITTING)
    perc -= 5;
  if (char_get_position(vict) == POS_RESTING)
    perc -= 10;
  if (char_get_position(vict) <= POS_SLEEPING)
    perc -= 25; /* High chance to succeed. */

  prob += char_get_aff_abils_dex(ch);

  perc += rand_number(g, -5, 5); /* Random factor */
  prob += rand_number(g, -5, 5); /* Random factor */

  if (axion_dice(g, 0) > 100 && char_get_position(vict) != POS_SLEEPING) { /* Unlucky! */
    reveal_hiding(g, ch, 0);
    act(g, "@r$N@R just happens to glance in your direction! What terrible luck!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@RYou just happen to glance behind you and spot @r$n@R trying to STEAL from you!@n", true, ch, 0, vict, TO_VICT);
    act(g, "@r$N@R just happens to glance in @r$n's@R direction and catches $m trying to STEAL!@n", true, ch, 0, vict, TO_NOTVICT);
    prob = -1000;
  }

  if (prob + 20 < perc && char_get_position(vict) != POS_SLEEPING) { /* Critical failure! */
    reveal_hiding(g, ch, 0);
    act(g, "@rYou are caught trying to stick your hand in @R$N's@r possessions!@n", true, ch, 0, vict, TO_CHAR);
    act(g, "@rYou catch @R$n@r trying to rummage through your possessions!@n", true, ch, 0, vict, TO_VICT);
    act(g, "@R$n@R is caught by @R$N@r as $e sticks $s hand in @R$N's@r possessions!@n", true, ch, 0, vict, TO_NOTVICT);
    char_set_wait(ch, PULSE_3SEC);
    if (char_is_npc(vict)) {
      set_fighting(g, vict, ch);
    }
    improve_skill(g, vict, SKILL_SPOT, 1);
  } else {                           /* Ok it wasn't a critical failure... */
    if (!strcasecmp(arg, "zenni")) { /* MOOLA! */
      if (prob > perc) {
        if (char_get_gold(vict) > 0) {
          if (char_get_gold(vict) > 100) {
            gold = (char_get_gold(vict) / 100) * rand_number(g, 1, 10);
          } else {
            gold = char_get_gold(vict);
          }
          if (gold + char_get_gold(ch) > char_gold_carry(ch)) {
            send_to_char(g, ch, "You can't hold that much more zenni on your person!\r\n");
            return;
          }
          char_set_gold(vict, char_get_gold(vict) - (gold));
          char_set_gold(ch, char_get_gold(ch) + (gold));
          if (!char_is_npc(vict)) {
            char_plr_set(vict, PLR_STOLEN, true);
            mudlog(g, LOG_NRM, MAX(ADMLVL_GRGOD, char_get_invis_level(g, ch)), true, "THEFT: %s has stolen %s zenni@n from %s", char_get_name(ch), add_commas(gold), char_get_name(vict));
          }
          if (gold > 1)
            send_to_char(g, ch, "Bingo!  You got %d zenni.\r\n", gold);
          else
            send_to_char(g, ch, "You manage to swipe a solitary zenni.\r\n");
          if (axion_dice(g, 0) > prob) {
            send_to_char(g, ch, "You think that your movements might have been a bit obvious.\r\n");
            reveal_hiding(g, ch, 0);
            act(g, "@R$n@r just stole zenni from @R$N@r!@n", true, ch, 0, vict, TO_ROOM);
            send_to_char(g, vict, "You feel like something may be missing...\r\n");
            if (char_is_npc(vict) && rand_number(g, 1, 3) == 3) {
              set_fighting(g, vict, ch);
            }
            improve_skill(g, vict, SKILL_SPOT, 1);
          }
          improve_skill(g, ch, SKILL_SLEIGHT_OF_HAND, 1);
          return;
        }
        send_to_char(g, ch, "It appears like they are broke...\r\n");
        return;
      } else { /* Failure! */
        reveal_hiding(g, ch, 0);
        act(g, "@rYou are caught trying to steal zenni from @R$N@r!@n", true, ch, 0, vict, TO_CHAR);
        act(g, "@rYou catch @R$n's@r hand trying to snatch your zenni!@n", true, ch, 0, vict, TO_VICT);
        act(g, "@R$N@r catches @R$n's@r hand trying to snatch $S zenni!@n", true, ch, 0, vict, TO_NOTVICT);
        char_set_wait(ch, PULSE_3SEC);
        if (char_is_npc(vict)) {
          set_fighting(g, vict, ch);
        }
        improve_skill(g, ch, SKILL_SLEIGHT_OF_HAND, 2);
        improve_skill(g, vict, SKILL_SPOT, 1);
        return;
      }
    } else { /* It's an object... */
      if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(vict)))) {
        for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
          if (char_get_equipment(vict, eq_pos) && (isname(arg, obj_get_name(char_get_equipment(vict, eq_pos)))) && char_can_see_obj(g, ch, char_get_equipment(vict, eq_pos))) {
            obj = char_get_equipment(vict, eq_pos);
            break;
          }
        if (!obj) {
          act(g, "$E isn't wearing that item.", false, ch, 0, vict, TO_CHAR);
          return;
        }
        if (char_get_position(vict) > POS_SLEEPING) {
          send_to_char(g, ch, "Steal worn equipment from them while they are awake? That's a stupid idea...\r\n");
          return;
        }
        if (!give_otrigger(g, obj, vict, ch) || !receive_mtrigger(g, ch, vict, obj)) {
          send_to_char(g, ch, "Impossible!\r\n");
          return;
        }
        if (obj_get_vnum(g, obj) >= 20000) {
          send_to_char(g, ch, "You can't steal that!\r\n");
          return;
        }
        if (obj_get_vnum(g, obj) >= 18800 && obj_get_vnum(g, obj) <= 18999) {
          send_to_char(g, ch, "You can't steal that!\r\n");
          return;
        }
        if (obj_get_vnum(g, obj) >= 19100 && obj_get_vnum(g, obj) <= 19199) {
          send_to_char(g, ch, "You can't steal that!\r\n");
          return;
        }
        if (obj_get_type_flag(obj) == ITEM_KEY) {
          send_to_char(g, ch, "No stealing keys!\r\n");
          return;
        }
        if (obj_extra_flagged(obj, ITEM_NOSTEAL)) {
          send_to_char(g, ch, "You can't steal that!\r\n");
          return;
        }
        if (obj_get_weight(obj) + (getCurCarriedWeight(ch)) > getMaxCarryWeight(g, ch)) {
          send_to_char(g, ch, "You can't carry that much weight.\r\n");
          return;
        }
        if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(g, ch, "You don't have the room for it right now!\r\n");
          return;
        } else if (prob > perc) { /* Right off their back :) */
          act(g, "You unequip $p and steal it.", false, ch, obj, vict, TO_CHAR);
          if (axion_dice(g, 0) > prob) {
            send_to_char(g, ch, "You think that your movements might have been a bit obvious.\r\n");
            reveal_hiding(g, ch, 0);
            act(g, "@R$n@r just stole $p@r from @R$N@r!@n", true, ch, obj, vict, TO_ROOM);
            send_to_char(g, vict, "You feel your body being disturbed.\r\n");
            improve_skill(g, vict, SKILL_SPOT, 1);
          }
          obj_to_char(g, unequip_char(g, vict, eq_pos), ch);
          improve_skill(g, ch, SKILL_SLEIGHT_OF_HAND, 1);
          return;
        } else { /* Failure! */
          reveal_hiding(g, ch, 0);
          char_set_position(vict, POS_SITTING);
          act(g, "@rYou are caught trying to steal $p@r from @R$N@r!@n", true, ch, obj, vict, TO_CHAR);
          act(g, "@rYou feel your body being shifted while you sleep and wake up to find @R$n@r trying to steal $p@r from you!@n", true, ch, obj, vict, TO_VICT);
          act(g, "@R$N@r catches @R$n's@r trying to $p@r from $M during $S sleep!@n", true, ch, obj, vict, TO_NOTVICT);
          char_set_wait(ch, PULSE_3SEC);
          if (char_is_npc(vict)) {
            char_set_position(vict, POS_STANDING);
            set_fighting(g, vict, ch);
          }
          improve_skill(g, ch, SKILL_SLEIGHT_OF_HAND, 2);
          improve_skill(g, vict, SKILL_SPOT, 1);
          return;
        }
      } else { /* It's in their inventory */
        if (!give_otrigger(g, obj, vict, ch) || !receive_mtrigger(g, ch, vict, obj)) {
          send_to_char(g, ch, "Impossible!\r\n");
          return;
        }
        if (obj_get_vnum(g, obj) >= 20000) {
          send_to_char(g, ch, "You can't steal that!\r\n");
          return;
        }
        if (obj_extra_flagged(obj, ITEM_NOSTEAL)) {
          send_to_char(g, ch, "You can't steal that!\r\n");
          return;
        }
        if (obj_get_type_flag(obj) == ITEM_KEY) {
          send_to_char(g, ch, "No stealing keys!\r\n");
          return;
        }
        if (obj_get_weight(obj) + (getCurCarriedWeight(ch)) > getMaxCarryWeight(g, ch)) {
          send_to_char(g, ch, "You can't carry that much weight.\r\n");
          return;
        }
        if (char_get_carry_items(ch) + 1 > char_can_carry_n(ch)) {
          send_to_char(g, ch, "You don't have the room for it right now!\r\n");
          return;
        } else if (prob > perc) { /* Right out of their pockets */
          act(g, "You steal $p from $N.", false, ch, obj, vict, TO_CHAR);
          obj_from_char(g, obj);
          obj_to_char(g, obj, ch);
          if (!char_is_npc(vict)) {
            char_plr_set(vict, PLR_STOLEN, true);
            mudlog(g, LOG_NRM, MAX(ADMLVL_GRGOD, char_get_invis_level(g, ch)), true, "THEFT: %s has stolen %s@n from %s", char_get_name(ch), obj_get_short_description(obj), char_get_name(vict));
          }
          if (axion_dice(g, 0) > prob) {
            reveal_hiding(g, ch, 0);
            send_to_char(g, ch, "You think that your movements might have been a bit obvious.\r\n");
            act(g, "@R$n@r just stole $p@r from @R$N@r!@n", true, ch, obj, vict, TO_ROOM);
            send_to_char(g, vict, "You feel like something may be missing...\r\n");
            if (char_is_npc(vict) && rand_number(g, 1, 3) == 3) {
              set_fighting(g, vict, ch);
            }
            improve_skill(g, vict, SKILL_SPOT, 1);
          }
          improve_skill(g, ch, SKILL_SLEIGHT_OF_HAND, 1);
          return;
        } else { /* Failure! */
          reveal_hiding(g, ch, 0);
          act(g, "@rYou are caught trying to steal $p@r from @R$N@r!@n", true, ch, obj, vict, TO_CHAR);
          act(g, "@rYou catch @R$n@r trying to steal $p@r from you!@n", true, ch, obj, vict, TO_VICT);
          act(g, "@R$N@r catches @R$n's@r trying to $p@r!@n", true, ch, obj, vict, TO_NOTVICT);
          char_set_wait(ch, PULSE_3SEC);
          if (char_is_npc(vict)) {
            char_set_position(vict, POS_STANDING);
            set_fighting(g, vict, ch);
          }
          improve_skill(g, ch, SKILL_SLEIGHT_OF_HAND, 2);
          improve_skill(g, vict, SKILL_SPOT, 1);
          return;
        }
      }
    } /* End of an object */
  }
}
