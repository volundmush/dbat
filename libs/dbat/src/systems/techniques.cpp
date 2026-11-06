//
// Created by volund on 11/4/21.
//

#include "dbat/techniques.h"
#include "dbat/utils.h"
#include "dbat/combat.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/character_utils.h"
#include "dbat/room_utils.h"
#include "dbat/game_world.h"

bool tech_handle_zanzoken(GameWorld *g, char_data *ch, char_data *vict, const char *name)
{
  if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
      (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
    if (!char_aff_flagged(ch, AFF_ZANZOKEN) || (char_aff_flagged(ch, AFF_ZANZOKEN) && char_get_speedi(g, ch) + rand_number(g, 1, 5) < char_get_speedi(g, vict) + rand_number(g, 1, 5))) {
      char msg[MAX_INPUT_LENGTH];
      snprintf(msg, sizeof(msg), "@C$N@c disappears, avoiding your %s before reappearing!@n", name);
      act(g, msg, true, ch, nullptr, vict, TO_CHAR);
      snprintf(msg, sizeof(msg), "@cYou disappear, avoiding @C$n's@c %s before reappearing!@n", name);
      act(g, msg, true, ch, nullptr, vict, TO_VICT);
      snprintf(msg, sizeof(msg), "@C$N@c disappears, avoiding @C$n's@c %s before reappearing!@n", name);
      act(g, msg, true, ch, nullptr, vict, TO_NOTVICT);
      if (char_aff_flagged(ch, AFF_ZANZOKEN)) {
        char_aff_set(ch, AFF_ZANZOKEN, false);
      }
      char_aff_set(vict, AFF_ZANZOKEN, false);
      return false;
    }
    act(g, "@C$N@c disappears, trying to avoid your attack but your zanzoken is faster!@n", false, ch, nullptr, vict, TO_CHAR);
    act(g, "@cYou zanzoken to avoid the attack but @C$n's@c zanzoken is faster!@n", false, ch, nullptr, vict, TO_VICT);
    act(g, "@C$N@c disappears, trying to avoid @C$n's@c attack but @C$n's@c zanzoken is faster!@n", false, ch, nullptr, vict, TO_NOTVICT);
    char_aff_set(vict, AFF_ZANZOKEN, false);
    char_aff_set(ch, AFF_ZANZOKEN, false);
  }
  return true;
}

void tech_handle_posmodifier(char_data *vict, int *pry, int *blk, int *dge, int *prob)
{
  switch (char_get_position(vict)) {
    case POS_SLEEPING:
      *pry = 0;
      *blk = 0;
      *dge = 0;
      *prob += 50;
      [[fallthrough]];
    case POS_RESTING:
      *pry /= 4;
      *blk /= 4;
      *dge /= 4;
      *prob += 25;
      [[fallthrough]];
    case POS_SITTING:
      *pry /= 2;
      *blk /= 2;
      *dge /= 2;
      *prob += 10;
  }
}

bool tech_handle_charge(GameWorld *g, char_data *ch, char *arg, double minimum, double *attperc)
{
  if (*arg) {
    double adjust = (double)(atoi(arg)) * 0.01;
    if (adjust < 0.01 || adjust > 1.00) {
      send_to_char(g, ch, "If you are going to supply a percentage of your charge to use then use an acceptable number (1-100)\r\n");
      return false;
    }
    if (adjust < *attperc && adjust >= minimum) {
      *attperc = adjust;
    } else if (adjust < minimum) {
      *attperc = minimum;
    }
  }
  return true;
}

bool tech_handle_targeting(GameWorld *g, char_data *ch, char *arg, char_data **vict, obj_data **obj)
{
  *vict = nullptr;
  *obj  = nullptr;
  if (!*arg || !(*vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (char_get_fighting(ch) && char_get_in_room(char_get_fighting(ch)) == char_get_in_room(ch)) {
      *vict = char_get_fighting(ch);
      return true;
    }
    if (!(*obj = get_obj_in_list_vis(g, ch, arg, nullptr, rroom_get_contents(g, char_get_in_room(ch))))) {
      send_to_char(g, ch, "Nothing around here by that name.\r\n");
      return false;
    }
    return true;
  }
  return true;
}

void tech_handle_fireshield(GameWorld *g, char_data *ch, char_data *vict, const char *part)
{

  if (getCurPL(g, vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && !char_get_bonuses(ch, BONUS_FIREPROOF) && !char_is_demon(ch)) {
    const char *part_name = part ? part : "body";
    char msg[MAX_INPUT_LENGTH];
    snprintf(msg, sizeof(msg), "@c$N's@W fireshield burns your %s!@n", part_name);
    act(g, msg, true, ch, nullptr, vict, TO_CHAR);
    snprintf(msg, sizeof(msg), "@C$n's@W %s is burned by your fireshield!@n", part_name);
    act(g, msg, true, ch, nullptr, vict, TO_VICT);
    snprintf(msg, sizeof(msg), "@c$n's@W %s is burned by @C$N's@W fireshield!@n", part_name);
    act(g, msg, true, ch, nullptr, vict, TO_NOTVICT);
    int64_t dmg = getMaxKI(g, vict) * 0.02;
    char_set_lastattack(vict, char_get_lastattack(vict) + (1000));
    hurt(g, 0, 0, vict, ch, nullptr, dmg, 0);
    if (char_get_bonuses(ch, BONUS_FIREPRONE)) {
      send_to_char(g, ch, "@RYou are extremely flammable and are burned by the attack!@n\r\n");
      send_to_char(g, vict, "@RThey are easily burned!@n\r\n");
      char_aff_set(ch, AFF_BURNED, true);
    } else if (char_get_aff_abils_con(ch) < axion_dice(g, 0)) {
      send_to_char(g, ch, "@RYou are badly burned!@n\r\n");
      send_to_char(g, vict, "@RThey are burned!@n\r\n");
      char_aff_set(ch, AFF_BURNED, true);
    }
  } else if (getCurPL(g, vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && (char_get_bonuses(ch, BONUS_FIREPROOF) || char_is_demon(ch))) {
    send_to_char(g, vict, "@RThey appear to be fireproof!@n\r\n");
  }
}

bool tech_handle_android_absorb(GameWorld *g, char_data *ch, char_data *vict)
{
  if (char_is_android(vict) && char_has_arms(vict) && char_get_skill(vict, SKILL_ABSORB) > rand_number(g, 1, 140)) {
    act(g, "@C$N@W absorbs your ki attack and all your charged ki with $S hand!@n", true, ch, nullptr, vict, TO_CHAR);
    act(g, "@WYou absorb @C$n's@W ki attack and all $s charged ki with your hand!@n", true, ch, nullptr, vict, TO_VICT);
    act(g, "@C$N@W absorbs @c$n's@W ki attack and all $s charged ki with $S hand!@n", true, ch, nullptr, vict, TO_NOTVICT);
    int amot = char_get_charge(ch);
    if (char_is_npc(ch)) {
      amot = getMaxKI(g, ch) / 20;
    }
    if (char_get_charge(vict) + amot > getMaxKI(g, vict)) {
      incCurKI(g, vict, getMaxKI(g, vict) - char_get_charge(vict), true);
      char_set_charge(vict, getMaxKI(g, vict));
    } else {
      char_set_charge(vict, char_get_charge(vict) + (amot));
    }
    return true;
  }
  return false;
}

void tech_handle_crashdown(GameWorld *g, char_data *ch, char_data *vict)
{
  if (char_aff_flagged(vict, AFF_FLYING)) {
    act(g, "@w$N@w is knocked out of the air!@n", true, ch, nullptr, vict, TO_CHAR);
    act(g, "@wYou are knocked out of the air!@n", true, ch, nullptr, vict, TO_VICT);
    act(g, "@w$N@w is knocked out of the air!@n", true, ch, nullptr, vict, TO_NOTVICT);
    char_aff_set(vict, AFF_FLYING, false);
    char_set_altitude(vict, 0);
    char_set_position(vict, POS_SITTING);
  } else {
    handle_knockdown(g, vict);
  }
}