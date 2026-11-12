//
// Created by volund on 11/4/21.
//

#include <cstdlib>

#include "dbat/techniques.hpp"
#include "dbat/utils.hpp"
#include "dbat/combat.hpp"
#include "dbat/comm.hpp"
#include "dbat/spells.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/consts/sectortypes.hpp"
#include "dbat/fight.hpp"
#include "dbat/random.hpp"

bool tech_handle_zanzoken(GameWorld *g, char_data *ch, char_data *vict, const char *name)
{
  if (((!char_is_npc(vict) && char_is_icer(vict) && rand_number(g, 1, 30) >= 28) || char_aff_flagged(vict, AFF_ZANZOKEN)) &&
      (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
    if (!char_aff_flagged(ch, AFF_ZANZOKEN) || (char_aff_flagged(ch, AFF_ZANZOKEN) && char_get_speedi(g, ch) + rand_number(g, 1, 5) < char_get_speedi(g, vict) + rand_number(g, 1, 5))) {
      std::string msg = str_format("@C$N@c disappears, avoiding your {} before reappearing!@n", name);
      act(g, msg.c_str(), true, ch, nullptr, vict, TO_CHAR);
      msg = str_format("@cYou disappear, avoiding @C$n's@c {} before reappearing!@n", name);
      act(g, msg.c_str(), true, ch, nullptr, vict, TO_VICT);
      msg = str_format("@C$N@c disappears, avoiding @C$n's@c {} before reappearing!@n", name);
      act(g, msg.c_str(), true, ch, nullptr, vict, TO_NOTVICT);
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

bool tech_handle_charge(GameWorld *g, char_data *ch, std::string_view arg, double minimum, double *attperc)
{
  if (!arg.empty()) {
    double adjust = (double)(parse_int(arg)) * 0.01;
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

bool tech_handle_targeting(GameWorld *g, char_data *ch, std::string &arg, char_data **vict, obj_data **obj)
{
  *vict = nullptr;
  *obj  = nullptr;
  if (arg.empty() || !(*vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
    if (char_get_fighting(g, ch) && char_get_in_room(char_get_fighting(g, ch)) == char_get_in_room(ch)) {
      *vict = char_get_fighting(g, ch);
      return true;
    }
    if (!(*obj = get_obj_in_list_vis(g, ch, arg, nullptr, room_get_contents(g, char_get_in_room(ch))))) {
      send_to_char(g, ch, "Nothing around here by that name.\r\n");
      return false;
    }
    return true;
  }
  return true;
}

void tech_handle_burn(GameWorld *g, char_data *ch, char_data *vict, int face)
{
  if (!char_aff_flagged(vict, AFF_BURNED) && rand_number(g, 1, 4) == face && !char_is_demon(vict) && !char_get_bonuses(vict, BONUS_FIREPROOF)) {
    send_to_char(g, vict, "@RYou are burned by the attack!@n\r\n");
    send_to_char(g, ch, "@RThey are burned by the attack!@n\r\n");
    char_aff_set(vict, AFF_BURNED, true);
  } else if (char_get_bonuses(vict, BONUS_FIREPROOF) || char_is_demon(vict)) {
    send_to_char(g, ch, "@RThey appear to be fireproof!@n\r\n");
  } else if (char_get_bonuses(vict, BONUS_FIREPRONE)) {
    send_to_char(g, vict, "@RYou are extremely flammable and are burned by the attack!@n\r\n");
    send_to_char(g, ch, "@RThey are easily burned!@n\r\n");
    char_aff_set(vict, AFF_BURNED, true);
  }
}

void tech_impact_scenery(GameWorld *g, char_data *ch, char_data *vict, room_vnum where, int lava, bool water_too)
{
  struct scenery {
    const char *to_char{};
    const char *to_room{}; /* if not to_char */
  };
  /* a null is nothing; outdoors' second is lava, if the ground is quiet */
  static constexpr scenery outdoors[8]   = {{"Debris is thrown into the air and showers down thunderously!"},
                                            {"Lava spews up through cracks in the ground, roaring into the sky as a large column of molten rock!"},
                                            {"A cloud of dust envelopes the entire area!"},
                                            {"The surrounding area roars and shudders from the impact!"},
                                            {"The ground shatters apart from the stress of the impact!"},
                                            {"The explosion continues to burn spreading out and devouring some more of the ground before dying out."}};
  static constexpr scenery underwater[3] = {
      {"The water churns violently!"},
      {"Large bubbles rise from the movement!"},
      {"The water collapses in on the hole created!", "The water collapses in on the hole create!"}
  };
  static constexpr scenery water[3]  = {{"A huge column of water erupts from the impact!"},
                                        {"The impact briefly causes a swirling vortex of water!"},
                                        {"A huge depression forms in the water and erupts into a wave from the impact!"}};
  static constexpr scenery inside[8] = {{"Debris is thrown into the air and showers down thunderously!"},
                                        {"The structure of the surrounding room cracks and quakes from the blast!"},
                                        {"Parts of the ceiling collapse, crushing into the floor!"},
                                        {"The surrounding area roars and shudders from the impact!"},
                                        {"The ground shatters apart from the stress of the impact!"},
                                        {"The walls of the surrounding room crack in the same instant!"}};
  auto show = [&](const scenery &sc) {
    if (!sc.to_char)
      return;
    act(g, sc.to_char, true, ch, nullptr, vict, TO_CHAR);
    act(g, sc.to_room ? sc.to_room : sc.to_char, true, ch, nullptr, vict, TO_ROOM);
  };
  int sect = room_sect(g, where);

  if (sect != SECT_INSIDE &&
      (water_too || (sect != SECT_UNDERWATER && sect != SECT_WATER_SWIM && sect != SECT_WATER_NOSWIM))) {
    impact_sound(g, ch, "@wA loud roar is heard nearby!@n\r\n");
    int roll = rand_number(g, 1, 8);
    if (roll != 2)
      show(outdoors[roll - 1]);
    else if (rand_number(g, 1, 4) == 4 && room_get_geffect(g, where) == 0) {
      room_set_geffect(g, where, lava);
      show(outdoors[1]);
    }
  }
  if (sect == SECT_UNDERWATER)
    show(underwater[rand_number(g, 1, 3) - 1]);
  if (sect == SECT_WATER_SWIM || sect == SECT_WATER_NOSWIM)
    show(water[rand_number(g, 1, 3) - 1]);
  if (sect == SECT_INSIDE) {
    impact_sound(g, ch, "@wA loud roar is heard nearby!@n\r\n");
    show(inside[rand_number(g, 1, 8) - 1]);
  }
}

void tech_handle_fireshield(GameWorld *g, char_data *ch, char_data *vict, const char *part)
{

  if (getCurPL(g, vict) > 0 && !char_aff_flagged(vict, AFF_SPIRIT) && char_aff_flagged(vict, AFF_FIRESHIELD) && !char_get_bonuses(ch, BONUS_FIREPROOF) && !char_is_demon(ch)) {
    const char *part_name = part ? part : "body";
    std::string msg       = str_format("@c$N's@W fireshield burns your {}!@n", part_name);
    act(g, msg.c_str(), true, ch, nullptr, vict, TO_CHAR);
    msg = str_format("@C$n's@W {} is burned by your fireshield!@n", part_name);
    act(g, msg.c_str(), true, ch, nullptr, vict, TO_VICT);
    msg = str_format("@c$n's@W {} is burned by @C$N's@W fireshield!@n", part_name);
    act(g, msg.c_str(), true, ch, nullptr, vict, TO_NOTVICT);
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
  if (char_is_android(vict) && char_has_arms(g, vict) && char_get_skill(vict, SKILL_ABSORB) > rand_number(g, 1, 140)) {
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

bool tech_handle_arms(GameWorld *g, char_data *ch)
{
  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no available arms!\r\n");
    return false;
  }
  if (char_get_limb_condition(ch, (1) - 1) > 0 && char_get_limb_condition(ch, (1) - 1) < 50 && char_get_limb_condition(ch, (2) - 1) < 0) {
    send_to_char(g, ch, "Using your broken right arm has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (1) - 1, char_get_limb_condition(ch, (1) - 1) - (rand_number(g, 3, 5)));
    if (char_get_limb_condition(ch, (1) - 1) < 0) {
      act(g, "@RYour right arm has fallen apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@R's right arm has fallen apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
  } else if (char_get_limb_condition(ch, (2) - 1) > 0 && char_get_limb_condition(ch, (2) - 1) < 50 && char_get_limb_condition(ch, (1) - 1) < 0) {
    send_to_char(g, ch, "Using your broken left arm has damaged it more!@n\r\n");
    char_set_limb_condition(ch, (2) - 1, char_get_limb_condition(ch, (2) - 1) - (rand_number(g, 3, 5)));
    if (char_get_limb_condition(ch, (2) - 1) < 0) {
      act(g, "@RYour left arm has fallen apart!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@R's left arm has fallen apart!@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
  }
  return true;
}
