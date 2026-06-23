//
// Created by volund on 11/4/21.
//

#include "techniques.h"

#include "character_api.h"
#include "room_api.h"
#include "character_impl.h"
#include "character_macros.h"
#include "character_utils.h"
#include "combat.h"
#include "comm.h"
#include "consts/affflags.h"
#include "consts/applies.h"
#include "consts/maximums.h"
#include "consts/mobflags.h"
#include "consts/positions.h"
#include "consts/races.h"
#include "consts/sex.h"
#include "flags.h"
#include "random.h"
#include "search.h"
#include "spells.h"

#include <cstdlib>

bool tech_handle_zanzoken(char_data *ch, char_data *vict, const char *name) {
  if (((!IS_NPC(vict) && IS_ICER(vict) && rand_number(1, 30) >= 28) ||
       char_condition_has(vict, "zanzoken")) &&
      (getCurST(vict)) >= 1 && GET_POS(vict) != POS_SLEEPING) {
    if (!char_condition_has(ch, "zanzoken") ||
        (char_condition_has(ch, "zanzoken") &&
         GET_SPEEDI(ch) + rand_number(1, 5) <
             GET_SPEEDI(vict) + rand_number(1, 5))) {
      char msg[MAX_INPUT_LENGTH];
      snprintf(msg, sizeof(msg),
               "@C$N@c disappears, avoiding your %s before reappearing!@n",
               name);
      act(msg, TRUE, ch, nullptr, vict, TO_CHAR);
      snprintf(msg, sizeof(msg),
               "@cYou disappear, avoiding @C$n's@c %s before reappearing!@n",
               name);
      act(msg, TRUE, ch, nullptr, vict, TO_VICT);
      snprintf(msg, sizeof(msg),
               "@C$N@c disappears, avoiding @C$n's@c %s before reappearing!@n",
               name);
      act(msg, TRUE, ch, nullptr, vict, TO_NOTVICT);
      if (char_condition_has(ch, "zanzoken")) {
        char_condition_remove(ch, "zanzoken", "zanzoken_over");
      }
      char_condition_remove(vict, "zanzoken", "zanzoken_over");
      return false;
    } else {
      act("@C$N@c disappears, trying to avoid your attack but your zanzoken is "
          "faster!@n",
          FALSE, ch, 0, vict, TO_CHAR);
      act("@cYou zanzoken to avoid the attack but @C$n's@c zanzoken is "
          "faster!@n",
          FALSE, ch, 0, vict, TO_VICT);
      act("@C$N@c disappears, trying to avoid @C$n's@c attack but @C$n's@c "
          "zanzoken is faster!@n",
          FALSE, ch, 0, vict, TO_NOTVICT);
      char_condition_remove(vict, "zanzoken", "zanzoken_over");
      char_condition_remove(ch, "zanzoken", "zanzoken_over");
    }
  }
  return true;
}

void tech_handle_posmodifier(char_data *vict, int &pry, int &blk, int &dge,
                             int &prob) {
  switch (GET_POS(vict)) {
  case POS_SLEEPING:
    pry = 0;
    blk = 0;
    dge = 0;
    prob += 50;
  case POS_RESTING:
    pry /= 4;
    blk /= 4;
    dge /= 4;
    prob += 25;
  case POS_SITTING:
    pry /= 2;
    blk /= 2;
    dge /= 2;
    prob += 10;
  }
}

bool tech_handle_charge(char_data *ch, char *arg, double minimum,
                        double *attperc) {
  if (*arg) {
    double adjust = (double)(atoi(arg)) * 0.01;
    if (adjust < 0.01 || adjust > 1.00) {
      send_to_char(ch, "If you are going to supply a percentage of your charge "
                       "to use then use an acceptable number (1-100)\r\n");
      return false;
    } else if (adjust < *attperc && adjust >= minimum) {
      *attperc = adjust;
    } else if (adjust < minimum) {
      *attperc = minimum;
    }
  }
  return true;
}

bool tech_handle_targeting(char_data *ch, char *arg, char_data **vict,
                           obj_data **obj) {
  *vict = nullptr;
  *obj = nullptr;
  if (!*arg || !(*vict = get_char_vis(ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && char_room_get(FIGHTING(ch)) == char_room_get(ch)) {
      *vict = FIGHTING(ch);
      return true;
    } else if (!(*obj = get_obj_in_list_vis(ch, arg, nullptr,
                                            inv_for_room(char_room_get(ch))))) {
      send_to_char(ch, "Nothing around here by that name.\r\n");
      return false;
    }
    return true;
  }
  return true;
}



bool tech_handle_android_absorb(char_data *ch, char_data *vict) {
  if (IS_ANDROID(vict) && HAS_ARMS(vict) &&
      GET_SKILL(vict, SKILL_ABSORB) > rand_number(1, 140)) {
    act("@C$N@W absorbs your ki attack and all your charged ki with $S hand!@n",
        TRUE, ch, nullptr, vict, TO_CHAR);
    act("@WYou absorb @C$n's@W ki attack and all $s charged ki with your "
        "hand!@n",
        TRUE, ch, nullptr, vict, TO_VICT);
    act("@C$N@W absorbs @c$n's@W ki attack and all $s charged ki with $S "
        "hand!@n",
        TRUE, ch, nullptr, vict, TO_NOTVICT);
    int amot = GET_CHARGE(ch);
    if (IS_NPC(ch)) {
      amot = GET_MAX_MANA(ch) / 20;
    }
    if (GET_CHARGE(vict) + amot > GET_MAX_MANA(vict)) {
      incCurKI(vict, getMaxKI(vict) - GET_CHARGE(vict));
      char_charge_set(vict, GET_MAX_MANA(vict));
    } else {
      char_charge_set(vict, GET_CHARGE(vict) + amot);
    }
    return true;
  }
  return false;
}

void tech_handle_crashdown(char_data *ch, char_data *vict) {
  if (char_condition_has(vict, "flying")) {
    act("@w$N@w is knocked out of the air!@n", TRUE, ch, 0, vict, TO_CHAR);
    act("@wYou are knocked out of the air!@n", TRUE, ch, 0, vict, TO_VICT);
    act("@w$N@w is knocked out of the air!@n", TRUE, ch, 0, vict, TO_NOTVICT);
    char_condition_remove(vict, "flying", "stop_flying");
    char_position_set(vict, POS_SITTING);
  } else {
    handle_knockdown(vict);
  }
}
