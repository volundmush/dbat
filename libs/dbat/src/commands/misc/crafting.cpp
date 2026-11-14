/* ************************************************************************
*  File: misc/crafting.c                                    Part of DBAT       *
*  Usage: Crafting, cooking, fishing and item commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
*                                   ---                                   *
*  This is an original file created by me for Dragonball Advent Truth     *
*  original credits maintained where relevant for act.other.c as this is  *
*  practically an act.other.c part two - Iovan 3/20/2011                  *
************************************************************************ */
#include <cstdlib>

#include "dbat/args.hpp"
#include "dbat/cmdargs.hpp"
#include "dbat/iterate.hpp"
#include "dbat/act.misc.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/songs.hpp"
#include "dbat/consts/fish.hpp"
#include "dbat/consts/recipes.hpp"
#include "dbat/consts/attacks.hpp"
#include "dbat/utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/random.hpp"
#include "dbat/affect.hpp"
#include "dbat/search.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/extract.hpp"
#include "dbat/relocate.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/db.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/spells.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/combat.hpp"
#include "dbat/obj_edit.hpp"
#include "dbat/fight.hpp"
#include "dbat/class.hpp"
#include "dbat/act.item.hpp"
#include "dbat/game_world.hpp"

#include <algorithm>
#include <array>

#include "misc.hpp"

namespace {
  /* ---- fishing ---- */

  constexpr const char *NO_POLE   = "You are not holding a fishing pole.\r\n";
  constexpr const char *NOT_POLE  = "You do not have a fishing pole in your hand!\r\n";
  constexpr const char *NOT_EVEN  = "You are not even fishing!\r\n";

  bool has_pole(struct char_data *ch)
  {
    struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);
    return pole && obj_get_type_flag(pole) == ITEM_FISHPOLE;
  }

  void stop_fishing(struct char_data *ch)
  {
    char_plr_set(ch, PLR_FISHING, false);
    char_set_accuracy_mod(ch, 0);
    char_set_fishstate(ch, FISH_NOFISH);
  }

  /* Where fish of which kinds are caught: four kinds from first, from the most common up */
  struct fishing_water {
    bool fresh;
    int planet;
    obj_vnum first;
  };
  constexpr fishing_water FISHING_WATERS[] = {
    {true, ROOM_EARTH, 1000},
    {true, ROOM_AETHER, 1012},
    {false, ROOM_EARTH, 1004},
    {false, ROOM_NAMEK, 1008},
  };

  void catch_fish(GameWorld *g, struct char_data *ch, int quality)
  {
    room_vnum here = char_get_in_room(ch);
    bool fresh     = room_flagged(g, here, ROOM_FISHFRESH);
    obj_vnum num   = 1000;
    auto water     = std::ranges::find_if(FISHING_WATERS, [&](const fishing_water &w) { return w.fresh == fresh && room_flagged(g, here, w.planet); });
    if (water != std::end(FISHING_WATERS)) {
      int roll = rand_number(g, 1, 10);
      num      = water->first + (roll <= 4 ? 0 : roll <= 7 ? 1 : roll <= 9 ? 2 : 3);
    }

    struct obj_data *fish = read_object(g, num);
    if (!fish) {
      send_to_imm(g, "Error: Fish success with no fish! Report to Iovan!\r\n");
      return;
    }
    act(g, "@CYou manage to pull a $p@C from the water and onto the ground in front of you!@n", true, ch, fish, nullptr, TO_CHAR);
    act(g, "@c$n@C manages to pull a $p@C from the water and onto the ground in front of $m!@n", true, ch, fish, nullptr, TO_ROOM);

    if (quality <= 0 && rand_number(g, 1, 20) >= 17)
      quality += rand_number(g, 2, 7);
    struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);
    if (obj_get_value(pole, 0) * 2 >= axion_dice(g, 0))
      quality += 2;
    else if (obj_get_value(pole, 0) >= axion_dice(g, 0))
      quality += 1;

    int weight;
    if (quality <= 1) {
      weight = rand_number(g, 0, 2);
    } else if (quality <= 3) {
      weight = rand_number(g, 3, 4);
      obj_set_cost(fish, obj_get_cost(fish) + (obj_get_cost(fish) * 0.20));
      obj_set_value(fish, 0, obj_get_value(fish, 0) + 1);
    } else if (quality <= 6) {
      weight = rand_number(g, 5, 9);
      obj_set_cost(fish, obj_get_cost(fish) + (obj_get_cost(fish) * 0.5));
      obj_set_value(fish, 0, obj_get_value(fish, 0) + 3);
    } else {
      weight = rand_number(g, 10, 15);
      obj_set_cost(fish, obj_get_cost(fish) + (obj_get_cost(fish) * 2));
      obj_set_value(fish, 0, obj_get_value(fish, 0) + 5);
    }
    obj_set_weight(fish, obj_get_weight(fish) + weight);

    obj_set_value(pole, 0, 0);
    obj_to_room(g, fish, char_get_in_room(ch));
    int64_t fish_weight = obj_get_weight(fish); /* before the get, whose triggers may take the fish */
    do_get(g, ch, "fish", 0, 0);
    send_to_char(g, ch, "@D[@cFish Weight@D: @G{:d}@D]@n\r\n", fish_weight);
    stop_fishing(ch);
  }

  /* A pulse of ch's fishing */
  void fish_tick(GameWorld *g, struct char_data *ch)
  {
    if (char_get_accuracy_mod(ch) <= 0 && char_get_fishstate(ch) == FISH_REELING) { /* We've caught it */
      int quality;
      if (char_get_accuracy(ch) >= rand_number(g, 60, 100))
        quality = rand_number(g, 0, 3) + rand_number(g, 0, 3) + rand_number(g, 0, 3);
      else if (char_get_accuracy(ch) >= rand_number(g, 45, 60))
        quality = rand_number(g, 0, 3) + rand_number(g, 0, 3);
      else
        quality = rand_number(g, 0, 3);
      catch_fish(g, ch, quality);
      return;
    }
    if (rand_number(g, 1, 5) < 3)
      return;

    int state = char_get_fishstate(ch);
    if (state == FISH_REELING && rand_number(g, 1, 100) <= 80) {
      if (char_get_accuracy(ch) >= 80)
        char_set_accuracy_mod(ch, char_get_accuracy_mod(ch) - rand_number(g, 6, 10));
      else if (char_get_accuracy(ch) >= 40)
        char_set_accuracy_mod(ch, char_get_accuracy_mod(ch) - rand_number(g, 5, 8));
      else
        char_set_accuracy_mod(ch, char_get_accuracy_mod(ch) - rand_number(g, 1, 4));
      act(g, "@CYou reel the line on your pole some.@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@c$n@C reels the line on $s pole slowly.@n", true, ch, nullptr, nullptr, TO_ROOM);
      send_to_char(g, ch, "@D[@wDistance@D: @Y{:d}@D]@n\r\n", char_get_accuracy_mod(ch) > 0 ? char_get_accuracy_mod(ch) : 0);
    } else if (state == FISH_REELING && rand_number(g, 1, 58) <= 55) {
      act(g, "@CYou struggle as the fish fights against your attempts to reel it in!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@c$n@C struggles with the fish on the end of $s pole!@n", true, ch, nullptr, nullptr, TO_ROOM);
    } else if (state == FISH_REELING || (state == FISH_HOOKED && rand_number(g, 1, 20) >= 12)) { /* Lose the fish */
      act(g, "@CYou feel the line go slack and realize you've lost the fish! You reel your line back in...@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@c$n@C frowns and then begins to reel in $s line.@n", true, ch, nullptr, nullptr, TO_ROOM);
      char_set_accuracy_mod(ch, 0);
      char_set_fishstate(ch, FISH_NOFISH);
      char_plr_set(ch, PLR_FISHING, false);
      if (state == FISH_REELING && has_pole(ch)) /* the bait goes with a fish that was being reeled */
        obj_set_value(char_get_equipment(ch, WEAR_WIELD2), 0, 0);
    } else if (state == FISH_BITE && rand_number(g, 1, 20) >= 12) {
      act(g, "@CYou feel as if the fish has stopped biting...@n", true, ch, nullptr, nullptr, TO_CHAR);
      char_set_fishstate(ch, FISH_NOFISH);
    } else if (state != FISH_HOOKED && state != FISH_BITE &&
               ((room_flagged(g, char_get_in_room(ch), ROOM_FISHFRESH) && rand_number(g, 1, 10) >= 8) ||
                (!room_flagged(g, char_get_in_room(ch), ROOM_FISHFRESH) && rand_number(g, 1, 20) >= 18))) {
      act(g, "@CYou feel a fish biting on your line! Better @Ghook@C it!@n", true, ch, nullptr, nullptr, TO_CHAR);
      char_set_fishstate(ch, FISH_BITE);
    }
  }

  void fish_cast(GameWorld *g, struct char_data *ch)
  {
    if (char_plr_flagged(ch, PLR_FISHING)) {
      send_to_char(g, ch, "You are already fishing! Syntax: fish stop\r\n");
      return;
    }
    if (!room_flagged(g, char_get_in_room(ch), ROOM_FISHING)) {
      send_to_char(g, ch, "This is not an area you can fish at.\r\n");
      return;
    }
    if (char_aff_flagged(ch, AFF_FLYING)) {
      send_to_char(g, ch, "You can't fish while flying.\r\n");
      return;
    }
    if (char_get_fighting(g, ch)) {
      send_to_char(g, ch, "You can't fish while fighting!\r\n");
      return;
    }
    struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);
    if (!pole) {
      send_to_char(g, ch, NO_POLE);
      return;
    }
    if (obj_get_type_flag(pole) != ITEM_FISHPOLE) {
      send_to_char(g, ch, NOT_POLE);
      return;
    }
    if (obj_get_value(pole, 0) == 0) {
      send_to_char(g, ch, "There is no bait on your line!\r\n");
      return;
    }
    reveal_hiding(g, ch, 0);
    act(g, "@CYou pull your arm back and then spring it forward, casting the baited line. A moment later there is a splash as the hook enters the water.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@C pulls $s arm back and then springs it foward, casting the line of $s fishing pole into the water.@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_accuracy_mod(ch, rand_number(g, 30, 80));
    char_plr_set(ch, PLR_FISHING, true);
    send_to_char(g, ch, "@D[@wDistance@D: @Y{:d}@D]@n\r\n", char_get_accuracy_mod(ch));
  }

  void fish_hook(GameWorld *g, struct char_data *ch)
  {
    if (!char_plr_flagged(ch, PLR_FISHING)) {
      send_to_char(g, ch, NOT_EVEN);
      return;
    }
    switch (char_get_fishstate(ch)) {
      case FISH_NOFISH:
        send_to_char(g, ch, "You do not have a fish biting on your line.\r\n");
        return;
      case FISH_REELING:
        send_to_char(g, ch, "You are already trying to reel the fish in!\r\n");
        return;
      case FISH_HOOKED:
        send_to_char(g, ch, "You already have the fish hooked! Reel it in!\r\n");
        return;
    }
    if (axion_dice(g, -18) > char_get_accuracy(ch)) {
      reveal_hiding(g, ch, 0);
      act(g, "@CYou pull hard but the fish spits the hook out a second before you pull! You return to waiting for a bite...@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@c$n@C pulls hard on $s fishing line, but a moment later $e frowns and returns to fishing.@n", true, ch, nullptr, nullptr, TO_ROOM);
      char_set_fishstate(ch, FISH_NOFISH);
      return;
    }
    reveal_hiding(g, ch, 0);
    act(g, "@CYou pull hard on your line and feel that you have managed to hook the fish and start reeling it in!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@C pulls hard on $s fishing line and starts to struggle with the fish on the other end!@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_fishstate(ch, FISH_REELING);
  }

  void fish_apply(GameWorld *g, struct char_data *ch, std::string &what)
  {
    struct obj_data *pole = char_get_equipment(ch, WEAR_WIELD2);
    if (!pole) {
      send_to_char(g, ch, NO_POLE);
      return;
    }
    if (obj_get_type_flag(pole) != ITEM_FISHPOLE) {
      send_to_char(g, ch, NOT_POLE);
      return;
    }
    if (obj_get_value(pole, 0) != 0) {
      send_to_char(g, ch, "Your fishing pole already has bait on its hook.\r\n");
      return;
    }
    if (what.empty()) {
      send_to_char(g, ch, "Syntax: fish apply (bait)\r\n");
      return;
    }
    auto found = find_carried(g, ch, what, "You don't have that bait.\r\n");
    if (reject(g, ch, found))
      return;
    struct obj_data *bait = *found;
    if (obj_get_type_flag(bait) != ITEM_FISHBAIT) {
      send_to_char(g, ch, "That isn't fishing bait!\r\n");
      return;
    }
    reveal_hiding(g, ch, 0);
    act(g, "@CYou carefully apply the $p@C to your hook.@n", true, ch, bait, nullptr, TO_CHAR);
    act(g, "@c$n@C carefully applies $p@C to $s fishing pole's hook.@n", true, ch, bait, nullptr, TO_ROOM);
    obj_set_value(pole, 0, obj_get_cost(bait));
    extract_obj(g, bait);
  }

  void fish_stop(GameWorld *g, struct char_data *ch)
  {
    if (!char_plr_flagged(ch, PLR_FISHING)) {
      send_to_char(g, ch, NOT_EVEN);
      return;
    }
    reveal_hiding(g, ch, 0);
    act(g, "@CYou reel in your line and stop fishing.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@C reels in $s fishing line and stops fishing.@n", true, ch, nullptr, nullptr, TO_ROOM);
    stop_fishing(ch);
  }

  /* ---- ink ---- */

  void combine_ink(GameWorld *g, struct char_data *ch, std::string &first, std::string &second)
  {
    auto found = find_carried(g, ch, first, "You do not have the first bottle that you were wanting to combine.\r\n");
    if (reject(g, ch, found))
      return;
    auto found2 = find_carried(g, ch, second, "You do not have the second bottle that you were wanting to combine.\r\n");
    if (reject(g, ch, found2))
      return;
    struct obj_data *obj = *found, *obj2 = *found2;
    if (obj_get_vnum(g, obj) != 3424 || obj_get_vnum(g, obj2) != 3424) {
      send_to_char(g, ch, "That is not an ink bottle!\r\n");
      return;
    }
    if (obj_get_value(obj, 6) <= 0) {
      send_to_char(g, ch, "There isn't any ink in the first bottle!\r\n");
      return;
    }
    if (obj_get_value(obj2, 6) <= 0) {
      send_to_char(g, ch, "There isn't any ink in the second bottle!\r\n");
      return;
    }
    /* The fuller bottle takes the other's ink, up to 24 */
    struct obj_data *into = obj, *from = obj2;
    if (obj_get_value(obj2, 6) > obj_get_value(obj, 6))
      std::swap(into, from);
    obj_set_value(into, 6, std::min(obj_get_value(into, 6) + obj_get_value(from, 6), 24));
    send_to_char(g, ch, "You combine the ink of the two bottles into one bottle, and discard the leftovers.\r\n");
    extract_obj(g, from);
  }

  void extract_ink(GameWorld *g, struct char_data *ch, struct obj_data *obj, int skill, int chance)
  {
    if (obj_get_value(obj, VAL_MAXMATURE) != 0 && obj_get_value(obj, VAL_MATURITY) < obj_get_value(obj, VAL_MAXMATURE)) {
      send_to_char(g, ch, "It's not mature enough to extract from!\r\n");
      return;
    }
    struct obj_data *bottle = nullptr; /* the last empty one carried */
    for (obj_data *item : char_inventory(g, ch))
      if (obj_get_vnum(g, item) == 3423)
        bottle = item;
    int64_t cost = (getMaxKI(g, ch) * 0.35) + 500;
    if (!bottle) {
      send_to_char(g, ch, "You do not have an empty bottle to put the extracted ink in.\r\n");
      return;
    }
    if (obj_get_value(bottle, 6) + 4 >= 24)
      cost += static_cast<int64_t>(getMaxKI(g, ch) * 0.5);
    if (getCurKI(g, ch) < cost) {
      send_to_char(g, ch, "You do not have enough ki! @D[@rNeeded@D: @R{}@D]@n\r\n", add_commas(cost));
      return;
    }

    decCurKI(g, ch, cost, 0);
    if (skill < chance) {
      act(g, "@WWith your ki flowing carefully into your hands you take a hold of the @G$p@W and begin to strip it of its leaves. Once it has been stripped you go to squeeze the ink carefully from the leaves into the bottle, but unfortunately the ink explodes into a mess instead!@n", true, ch, obj, nullptr, TO_CHAR);
      act(g, "@C$n@W takes a hold of the @G$p@W and begins to strip it of its leaves. Once it has been stripped $e bundles up the leaves in $s hands and begins to squeeze. A nasty explosion of a mess is all that follows!@n", true, ch, obj, nullptr, TO_ROOM);
      improve_skill(g, ch, SKILL_EXTRACT, 0);
      extract_obj(g, obj);
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    act(g, "@WWith your ki flowing carefully into your hands you take a hold of the @G$p@W and begin to strip it of its leaves. Once it has been stripped you go to squeeze the ink carefully from the leaves into the bottle, and manage to get every last drop of ink into it.@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W takes a hold of the @G$p@W and begins to strip it of its leaves. Once it has been stripped $e bundles up the leaves in $s hands and begins to squeeze ink carefully from the leaves into a bottle.@n", true, ch, obj, nullptr, TO_ROOM);
    extract_obj(g, obj);
    obj_set_value(bottle, 6, obj_get_value(bottle, 6) + rand_number(g, 4, 6));
    if (obj_get_value(bottle, 6) >= 24) { /* A full bottle is sealed with ki */
      struct obj_data *filled = read_object(g, 3424);
      extract_obj(g, bottle);
      obj_set_value(filled, 6, 24);
      obj_to_char(g, filled, ch);
      decCurKI(g, ch, 0, 0);
      act(g, "@GAs the last of the ink fills the bottle you infuse a final burst of ki into the bottle.@n", true, ch, filled, nullptr, TO_CHAR);
      act(g, "@GAs the last of the ink fills the bottle @g$n@G infuses a final burst of ki into the bottle.@n ", true, ch, filled, nullptr, TO_ROOM);
    } else {
      send_to_char(g, ch, "You will need to fill the bottle before giving it a final infusion of ki to complete the process.\r\n");
    }
    improve_skill(g, ch, SKILL_EXTRACT, 0);
    char_set_wait(ch, PULSE_3SEC);
  }

  /* ---- runes ---- */

  /* What a rune does to whoever wears it */
  struct rune_effect {
    const char *name;
    double per_skill; /* duration, per point of runic */
    int affect;       /* 0: Gebo, which teaches instead */
    int spell;
    const char *gained;
  };
  constexpr rune_effect KENAZ   = {"Kenaz", 0.16, AFF_INFRAVISION, SKILL_RUNIC, "@GYou can now see in the dark!"};
  constexpr rune_effect ALGIZ   = {"Algiz", 0.05, AFF_EARMOR, SKILL_PUNCH, "@GYou now have Ethereal Armor!"};
  constexpr rune_effect OAGAZ   = {"Oagaz", 0.04, AFF_ECHAINS, SKILL_KNEE, "@GYou now are protected by Ethereal Chains!"};
  constexpr rune_effect LAGUZ   = {"Laguz", 0.04, AFF_WATERBREATH, SKILL_SBC, "@GYou now have water breathing!"};
  constexpr rune_effect WUNJO   = {"Wunjo", 0.08, AFF_WUNJO, SKILL_SLAM, "@GYou are now blessed with a deeper understanding of things you experience!"};
  constexpr rune_effect PURISAZ = {"Purisaz", 0.06, AFF_POTENT, SKILL_HEELDROP, "@GYou feel as if your inner energy is more potent!"};
  constexpr rune_effect GEBO    = {"Gebo", 0, 0, 0, nullptr};

  struct rune {
    const char *word;
    int ink;
    bool ink_checked;              /* quirk: Kenaz is written without checking the ink */
    const rune_effect *on_self;
    const rune_effect *on_other;   /* quirk: Laguz writes Oagaz on others; null: Oagaz does nothing to others */
    bool practised;                /* quirk: Oagaz neither improves the skill nor takes time */
    const char *room_name;         /* quirk: what the room sees written on someone else, if not the name */
    bool shows_unclamped;          /* quirk: Purisaz on others shows the duration before it's raised to 1 */
  };
  constexpr rune RUNES[] = {
    {"kenaz", 1, false, &KENAZ, &KENAZ, true, nullptr, false},
    {"algiz", 2, true, &ALGIZ, &ALGIZ, true, nullptr, false},
    {"oagaz", 3, true, &OAGAZ, nullptr, false, nullptr, false},
    {"laguz", 4, true, &LAGUZ, &OAGAZ, true, nullptr, false},
    {"wunjo", 4, true, &WUNJO, &WUNJO, true, nullptr, false},
    {"purisaz", 4, true, &PURISAZ, &PURISAZ, true, "PUrisaz", true},
    {"gebo", 10, true, &GEBO, &GEBO, true, nullptr, false},
  };

  void write_rune(GameWorld *g, struct char_data *ch, struct char_data *vict, const rune &r, const rune_effect &effect, struct obj_data *bottle,
                  int inkcost, int skill, int64_t cost)
  {
    const char *flash = effect.affect ? "" : " The rune flashes out of existence immediately!";
    decCurKI(g, ch, cost, 0);
    if (vict == ch) {
      act(g, str_format("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@C{}@D'@B rune on your skin!{}@n", effect.name, flash).c_str(), true, ch, nullptr, nullptr, TO_CHAR);
      act(g, str_format("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@C{}@D'@B rune on $s skin.{}@n", effect.name, flash).c_str(), true, ch, nullptr, nullptr, TO_ROOM);
    } else {
      std::string to_char = str_format("@BYou dip your brush into the ink and infuse your ki skillfully into it. You pull the brush out and paint the @D'@C{}@D'@B rune on @b$N's@B skin!{}@n", effect.name, flash);
      std::string to_vict = str_format("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@C{}@D'@B rune on @RYOUR@B skin.{}@n", effect.name, flash);
      std::string to_room = str_format("@b$n@B dips $s brush into a bottle of ink and at the same time the ink starts to glow. Skillfully $e then writes the @D'@C{}@D'@B rune on @b$N's@B skin.{}@n", r.room_name ? r.room_name : effect.name, flash);
      act_triple(g, {to_char.c_str(), to_vict.c_str(), to_room.c_str(), true}, ch, nullptr, vict);
    }
    send_to_char(g, ch, "@D[@B{:d}@b ink used.@D]@n\r\n", inkcost);

    if (!effect.affect) {
      char_set_practice_points(g, vict, char_get_practice_points(g, vict) + 125);
      send_to_char(g, vict, "@GYou feel like you've just gained a lot of knowledge. Now if only you could apply it. @D[@m+125 PS@D]@n\r\n");
    } else {
      int duration = skill * effect.per_skill;
      if (vict != ch && r.shows_unclamped)
        send_to_char(g, vict, "{} @D(@WLasts@D: @w{:d}@D)@n\r\n", effect.gained, duration);
      if (duration < 1)
        duration = 1;
      if (vict == ch || !r.shows_unclamped)
        send_to_char(g, vict, "{} @D(@WLasts@D: @w{:d}@D)@n\r\n", effect.gained, duration);
      assign_affect(g, vict, effect.affect, effect.spell, duration, 0, 0, 0, 0, 0, 0);
    }

    obj_set_value(bottle, 6, obj_get_value(bottle, 6) - inkcost);
    if (obj_get_value(bottle, 6) <= 0) { /* The bottle is left empty */
      extract_obj(g, bottle);
      obj_to_char(g, read_object(g, 3423), ch);
    }
  }

  /* ---- silk ---- */

  /* What each bundle of silk weaves into; any other gives the plain piece */
  struct silk_grade {
    obj_vnum bundle;
    int armor_mult;
    int str;
    int intel;
    double price;
    int level;
  };
  constexpr silk_grade SILK_GRADES[] = {
    {16708, 20, 4, 4, 4, 80}, {16700, 15, 3, 3, 4, 75}, {16701, 10, 3, 3, 3, 50}, {16702, 6, 2, 2, 2, 25}, {16703, 4, 1, 1, 1.5, 5},
  };
  constexpr silk_grade PLAIN_SILK = {0, 1, 0, 0, 1, 0};

  /* What can be woven, and the word that picks it up */
  struct silk_piece {
    const char *name;
    obj_vnum vnum;
    const char *keyword;
  };
  constexpr silk_piece SILK_PIECES[] = {
    {"head", 16705, "headsash"},
    {"wrist", 16706, "wristband"},
    {"belt", 16707, "belt"},
  };

  /* The bundles silk is spun into, best first, by the skill needed */
  struct silk_bundle {
    int min_skill;
    obj_vnum vnum;
  };
  constexpr silk_bundle SILK_BUNDLES[] = {{100, 16700}, {90, 16701}, {80, 16702}, {50, 16703}, {0, 16704}};

  void weave_silk(GameWorld *g, struct char_data *ch, const std::string &what, int prob, int perc)
  {
    if (what.empty()) {
      send_to_char(g, ch, "Syntax: silk weave (head | wrist | belt)\r\n");
      return;
    }
    auto carried = char_inventory(g, ch);
    auto silk = std::ranges::find_if(carried, [&](obj_data *o) { return valid_silk(g, o) && !obj_extra_flagged(o, ITEM_FORGED); });
    if (silk == carried.end()) {
      send_to_char(g, ch, "You do not have an acceptable bundle of silk in your inventory!\r\n");
      return;
    }
    struct obj_data *obj = *silk;
    auto piece = std::ranges::find_if(SILK_PIECES, [&](const silk_piece &p) { return str_eq_nocase(what, p.name); });
    if (piece == std::end(SILK_PIECES)) {
      send_to_char(g, ch, "Syntax: silk weave (head | wrist | belt)");
      return;
    }
    if (prob <= perc) {
      act(g, "@WYou attempt to weave $p@W into the desired piece but end up ruining the entire bundle instead.@n", true, ch, obj, nullptr, TO_CHAR);
      act(g, "@C$n@W attempts to weave $p@W into some type of clothing but ends up ruining the entire bundle instead.@n", true, ch, obj, nullptr, TO_ROOM);
      extract_obj(g, obj);
      char_set_wait(ch, PULSE_4SEC);
      return;
    }

    struct obj_data *weaved = read_object(g, piece->vnum);
    obj_to_room(g, weaved, char_get_in_room(ch));
    auto graded = std::ranges::find(SILK_GRADES, obj_get_vnum(g, obj), &silk_grade::bundle);
    const silk_grade &grade = graded == std::end(SILK_GRADES) ? PLAIN_SILK : *graded;
    obj_get_affected(weaved)[0].location = 17;
    obj_get_affected(weaved)[0].modifier = 500 * grade.armor_mult;
    obj_set_cost(weaved, obj_get_cost(weaved) * grade.price);
    obj_set_value(weaved, 0, grade.level);
    obj_set_level(weaved, grade.level);
    if (grade.str > 0) {
      obj_get_affected(weaved)[1].location = 1;
      obj_get_affected(weaved)[1].modifier = grade.str;
    }
    if (grade.intel > 0) {
      obj_get_affected(weaved)[2].location = 3;
      obj_get_affected(weaved)[2].modifier = grade.intel;
    }
    act(g, "@WYou attempt to weave the bundle and manage to create $p@W!@n", true, ch, weaved, nullptr, TO_CHAR);
    act(g, "@C$n@W attempts to weave a bundle into something and manages to create $p@W!@n", true, ch, weaved, nullptr, TO_ROOM);
    do_get(g, ch, piece->keyword, 0, 0);
    extract_obj(g, obj);
    char_set_wait(ch, PULSE_4SEC);
  }

  void spin_silk(GameWorld *g, struct char_data *ch, int prob, int perc)
  {
    int64_t cost = ((getMaxKI(g, ch) * 0.01) * (prob * 0.20)) + (char_get_aff_abils_intel(ch) * char_get_level(ch));
    if (getCurKI(g, ch) < cost) {
      send_to_char(g, ch, "You do not have enough ki to weave any bundles of silk.\r\n");
      return;
    }
    char_set_wait(ch, PULSE_3SEC);

    /* Kurzaks are sometimes extraordinarily good at it */
    int superoll = rand_number(g, 1, 100), skill = char_get_skill(ch, SKILL_SILK);
    int super_chance = !char_is_kurzak(ch) ? 0 : skill >= 100 ? 8 : skill >= 60 ? 6 : skill >= 40 ? 3 : 0;
    if (super_chance > superoll) {
      struct obj_data *obj = read_object(g, 16708);
      obj_to_room(g, obj, char_get_in_room(ch));
      act(g, "@YYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@Y piled at your feet!@n", true, ch, obj, nullptr, TO_CHAR);
      send_to_char(g, ch, "@YIt's SUPER grand!@n\r\n");
      act(g, "@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", true, ch, obj, nullptr, TO_ROOM);
      decCurKI(g, ch, cost, 0);
      return;
    }
    if (prob <= perc) {
      act(g, "@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You end up making a poorly formed puddle of goo...@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@W seems to concentrate for a moment before spitting out a poorly formed puddle of goo...@n", true, ch, nullptr, nullptr, TO_ROOM);
      decCurKI(g, ch, cost, 0);
      improve_skill(g, ch, SKILL_SILK, 1);
      return;
    }
    const silk_bundle &bundle = *std::ranges::find_if(SILK_BUNDLES, [&](const silk_bundle &b) { return prob >= b.min_skill; });
    struct obj_data *obj      = read_object(g, bundle.vnum);
    obj_to_room(g, obj, char_get_in_room(ch));
    act(g, "@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@W piled at your feet!@n", true, ch, obj, nullptr, TO_CHAR);
    act(g, "@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
  }

  /* ---- cooking ---- */

  /* The ingredients, in the order a cook is told what's missing */
  enum ingredient { TOMATO, POTATO, ONION, APPLEPLUM, FBERRY, CARAMBOLA, LETTUCE, FLOUR, RICE, GARLIC, CARROT, CUCUMBER, GREENBEAN, NORMMEAT, REDPEP, NORMFISH, GOODFISH, GREATFISH, BESTFISH, INGREDIENTS };

  struct ingredient_kind {
    std::array<obj_vnum, 4> vnums; /* 0: none */
    const char *name;              /* "You need N <name><plural><after> for this recipe." */
    const char *plural;
    const char *after;
  };
  constexpr ingredient_kind INGREDIENT_KINDS[INGREDIENTS] = {
    {{RCP_TOMATO}, "tomato", "es", ""},
    {{RCP_POTATO}, "potato", "es", ""},
    {{RCP_ONION}, "onion", "s", ""},
    {{RCP_APPLEPLUM}, "appleplum", "s", ""},
    {{RCP_FROZENBERRY}, "frozen berry", "s", ""},
    {{RCP_CARAMBOLA}, "carambola", "s", ""},
    {{RCP_LETTUCE}, "head", "s", " of lettuce"},
    {{RCP_FLOUR}, "cup", "s", " of white flour"},
    {{RCP_RICE}, "cup", "s", " of white rice"},
    {{RCP_GARLIC}, "garlic clove", "s", ""},
    {{RCP_CARROT}, "carrot", "s", ""},
    {{RCP_CUCUMBER}, "cucumber", "s", ""},
    {{RCP_GREENBEAN}, "green bean", "s", ""},
    {{RCP_NORMAL_MEAT}, "normal raw steak", "s", ""},
    {{RCP_CHILIPEPPER}, "chili pepper", "s", ""},
    {{RCP_BLACKBASS, RCP_FLOUNDER, RCP_NARRI, RCP_GRAVELREBOI}, "black bass, flounder, narri, or gravel reboi", "", ""},
    {{RCP_SILVERTROUT, RCP_SILVEREEL, RCP_VALBISH, RCP_VOOSPIKE}, "silver trout, silver eel, valbish, or voos pike", "", ""},
    {{RCP_STRIPEDBASS, RCP_COBIA, RCP_GUSBLAT, RCP_SHADOWFISH}, "striped bass, cobia, gusblat, or shadowfish", "", ""},
    {{RCP_BLUECATFISH, RCP_TAMBOR, RCP_REPEEIL, RCP_SHADEEEL}, "blue catfish, tambor, repeeil, or shadeeel", "", ""},
  };

  struct portion {
    ingredient kind;
    int count;
  };

  /* The dishes, by the number a cook asks for */
  struct dish {
    std::array<portion, 5> portions; /* count 0: none */
    int difficulty;                  /* added to the roll */
    bool campfire;                   /* can be cooked over a campfire */
    obj_vnum meal;
    int psbonus;
    int expbonus;
  };
  constexpr dish DISHES[] = {
    {{{{NORMMEAT, 1}}}, 8, true, MEAL_STEAK, 1, 5},
    {{{{TOMATO, 2}}}, 0, false, MEAL_TOMATO_SOUP, 2, 15},
    {{{{POTATO, 2}}}, 0, false, MEAL_POTATO_SOUP, 1, 20},
    {{{{POTATO, 1}, {TOMATO, 1}, {CARROT, 1}, {GREENBEAN, 1}, {ONION, 1}}}, 0, false, MEAL_VEGETABLE_SOUP, 3, 45},
    {{{{NORMMEAT, 1}, {POTATO, 1}, {TOMATO, 1}, {GARLIC, 1}}}, 0, false, MEAL_MEAT_STEW, 2, 50},
    {{{{NORMMEAT, 1}, {REDPEP, 4}, {TOMATO, 2}}}, 0, false, MEAL_CHILI_SOUP, 0, 100},
    {{{{NORMFISH, 1}}}, 6, true, MEAL_NORM_FISH, 2, 12},
    {{{{GOODFISH, 1}}}, 10, true, MEAL_GOOD_FISH, 3, 40},
    {{{{GREATFISH, 1}}}, 12, true, MEAL_GREAT_FISH, 5, 80},
    {{{{BESTFISH, 1}}}, 16, true, MEAL_BEST_FISH, 7, 125},
    {{{{RICE, 1}}}, 0, false, MEAL_COOKED_RICE, 1, 8},
    {{{{RICE, 1}, {NORMFISH, 1}}}, 0, false, MEAL_SUSHI, 2, 20},
    {{{{FLOUR, 1}}}, 0, false, MEAL_BREAD, 1, 8},
    {{{{TOMATO, 1}, {CUCUMBER, 1}, {CARROT, 1}, {LETTUCE, 1}}}, 0, false, MEAL_SALAD, 5, 8},
    {{{{FLOUR, 1}, {APPLEPLUM, 1}}}, 0, false, MEAL_APPLEPLUM, 1, 9},
    {{{{FLOUR, 1}, {FBERRY, 1}}}, 0, false, MEAL_FBERRY_MUFFIN, 3, 12},
    {{{{FLOUR, 1}, {CARAMBOLA, 1}}}, 0, false, MEAL_CARAMBOLA_BREAD, 1, 9},
  };

  int ingredient_of(obj_vnum vnum)
  {
    for (int k = 0; k < INGREDIENTS; k++)
      if (std::ranges::find(INGREDIENT_KINDS[k].vnums, vnum) != INGREDIENT_KINDS[k].vnums.end())
        return k;
    return -1;
  }

  /* Goes through what ch carries for the dish's ingredients, using them up if use_up. Returns whether ch has them all;
   * if not, ch is told what's missing. */
  bool gather_ingredients(GameWorld *g, struct char_data *ch, const dish &d, bool use_up)
  {
    std::array<int, INGREDIENTS> needed;
    needed.fill(0);
    for (const portion &p : d.portions)
      needed[p.kind] += p.count;

    for (obj_data *obj : char_inventory(g, ch)) {
      int k = obj_get_vnum(g, obj) == 0 ? -1 : ingredient_of(obj_get_vnum(g, obj));
      if (k < 0 || needed[k] <= 0)
        continue;
      needed[k] -= 1;
      if (use_up)
        extract_obj(g, obj);
    }
    if (use_up || std::ranges::none_of(needed, [](int n) { return n > 0; }))
      return true;

    for (int k = 0; k < INGREDIENTS; k++)
      if (needed[k] > 0)
        send_to_char(g, ch, "@WYou need @m{:d}@W {}{}{} for this recipe.@n\r\n", needed[k], INGREDIENT_KINDS[k].name, needed[k] > 1 ? INGREDIENT_KINDS[k].plural : "",
                     INGREDIENT_KINDS[k].after);
    return false;
  }
} // namespace

ACMD(do_restring)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  descriptor_data *d = char_get_desc(ch);
  if (!d || char_get_in_room(ch) < 178 || char_get_in_room(ch) > 184) /* the menu needs someone typing */
    return;
  if (char_get_gold(ch) < 5000) {
    send_to_char(g, ch, "You need at least 5,000 zenni to initiate an equipment restring.\r\n");
    return;
  }
  auto found = find_carried(g, ch, arg, "You don't have a that equipment to restring in your inventory.\r\nSyntax: restring (obj name)\r\n");
  if (reject(g, ch, found))
    return;
  struct obj_data *obj = *found;
  if (obj_extra_flagged(obj, ITEM_CUSTOM)) {
    send_to_char(g, ch, "You can not restring a custom piece. Why? Because you already restrung it you dummy.\r\n");
    return;
  }
  desc_set_connected(d, CON_POBJ);
  d->obj_name      = str_format("{}", obj_get_name(obj));
  d->obj_was       = str_format("{}", obj_get_short_description(obj));
  d->obj_short     = d->obj_was;
  d->obj_long      = str_format("{}", obj_get_description(obj));
  d->obj_point_uid = obj_get_id(obj);
  d->obj_type      = 1;
  d->obj_weapon    = 0;
  disp_restring_menu(g, d);
  d->obj_editflag = EDIT_RESTRING;
  d->obj_editval  = EDIT_RESTRING_MAIN;
}

ACMD(do_fish)
{
  if (char_is_npc(ch))
    return;

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);
  if (arg.empty())
    send_to_char(g, ch, "Syntax: fish ( cast | hook | reel | apply | stop)\r\n");
  else if (str_eq_nocase(arg, "cast"))
    fish_cast(g, ch);
  else if (str_eq_nocase(arg, "hook"))
    fish_hook(g, ch);
  else if (str_eq_nocase(arg, "apply"))
    fish_apply(g, ch, arg2);
  else if (str_eq_nocase(arg, "stop"))
    fish_stop(g, ch);
  else
    send_to_char(g, ch, "Syntax: fish ( cast | hook | reel | apply | stop )\r\n");
}

void fish_update(GameWorld *g)
{
  for (char_data *ch : each_char(g)) {
    if (!char_plr_flagged(ch, PLR_FISHING))
      continue;
    if (!room_flagged(g, char_get_in_room(ch), ROOM_FISHING) || !has_pole(ch))
      stop_fishing(ch);
    else
      fish_tick(g, ch);
  }
}

ACMD(do_extract)
{
  if (!char_get_skill(ch, SKILL_EXTRACT)) {
    send_to_char(g, ch, "You do not know how to extract!\r\n");
    return;
  }

  int skill = char_get_skill(ch, SKILL_EXTRACT), chance = axion_dice(g, 0);
  std::string_view rest = argument;
  std::string arg       = take_word(rest);
  std::string arg2      = take_arg(rest);
  std::string arg3      = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: extract (object)\r\n");
    send_to_char(g, ch, "Syntax: extract combine (bottle1) (bottle2)\r\n");
    return;
  }
  if (str_eq_nocase(arg, "combine")) {
    combine_ink(g, ch, arg2, arg3);
    return;
  }

  auto found = find_carried(g, ch, arg, "You do not have that item.\r\n");
  if (reject(g, ch, found))
    return;
  if (obj_get_vnum(g, *found) != 3425) {
    send_to_char(g, ch, "That is not something you can extract from.\r\n");
    return;
  }
  extract_ink(g, ch, *found, skill, chance);
}

ACMD(do_runic)
{
  if (!char_get_skill(ch, SKILL_RUNIC)) {
    send_to_char(g, ch, "You do not know how to write down runes.\r\n");
    return;
  }

  int skill             = char_get_skill(ch, SKILL_RUNIC);
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);
  if (char_get_fighting(g, ch)) {
    send_to_char(g, ch, "You are too busy fighting to write runes!\r\n");
    return;
  }
  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, "Syntax: runic (target) (skill)\r\n");
    send_to_char(g, ch, "@D----@GRunic Skills@D----@n\r\n");
    send_to_char(g, ch, "@Rkenaz\n{}\n{}\n{}\n{}\n{}\n{}@n\n", skill >= 40 ? "@Galgiz" : "", skill >= 40 ? "@moagaz" : "", skill >= 50 ? "@CLaguz" : "", skill >= 60 ? "@Ywunjo" : "",
                 skill >= 80 ? "@rpurisaz" : "", skill >= 100 ? "@mgebo" : "");
    return;
  }

  /* The fullest bottle of ink, the first of them */
  struct obj_data *bottle = nullptr;
  int amount              = 0;
  for (obj_data *obj : char_inventory(g, ch)) {
    if (obj_get_vnum(g, obj) == 3424 && obj_get_value(obj, 6) > amount) {
      bottle = obj;
      amount = obj_get_value(obj, 6);
    }
  }
  bool brush = std::ranges::any_of(char_inventory(g, ch), [&](obj_data *obj) { return obj_get_vnum(g, obj) == 3427; });
  if (!bottle) {
    send_to_char(g, ch, "You do not have a bottle with enough ink in it.\r\n");
    return;
  }
  if (!brush) {
    send_to_char(g, ch, "You do not have a brush!\r\n");
    return;
  }

  int64_t cost = getMaxKI(g, ch) * 0.05;
  int bonus = 0, inkcost = 0;
  if (char_is_hoshijin(ch))
    bonus = 10;
  else
    inkcost += 2;

  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "You can't seem to find that person.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki to write runes.\r\n");
    return;
  }
  if (skill + bonus < axion_dice(g, 0) && rand_number(g, 1, 5) == 5) {
    act(g, "@BYou dip your brush into the ink, but as you infuse your ki you balance the flow wrong and end up destroying the ink bottle!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@b$n@B dips $s runic brush into a bottle filled with shimmering ink. @b$n@B appears to concentrate for a moment before a look of panic dons $s face. Just at that moment the bottle of ink explodes! Strange...@n", true, ch, nullptr, nullptr, TO_ROOM);
    extract_obj(g, bottle);
    improve_skill(g, ch, SKILL_RUNIC, 1);
    decCurKI(g, ch, cost, 0);
    char_set_wait(ch, PULSE_3SEC);
    return;
  }
  if (skill + bonus < axion_dice(g, 0)) {
    decCurKI(g, ch, cost, 0);
    act(g, "@BYou dip your brush into the ink, but as you infuse your ki you balance the flow wrong and end up evaporating some ink!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@b$n@B dips $s runic brush into a bottle filled with shimmering ink. @b$n@B appears to concentrate for a moment before some ink evaporates. Strange...@n", true, ch, nullptr, nullptr, TO_ROOM);
    improve_skill(g, ch, SKILL_RUNIC, 1);
    obj_set_value(bottle, 6, std::max(obj_get_value(bottle, 6) - rand_number(g, 1, 3), 0));
    char_set_wait(ch, PULSE_3SEC);
    return;
  }

  auto picked = std::ranges::find_if(RUNES, [&](const rune &r) { return str_eq_nocase(arg2, r.word); });
  if (picked == std::end(RUNES)) { /* quirk: this list leaves out Laguz */
    send_to_char(g, ch, "Syntax: runic (target) (skill)\r\n");
    send_to_char(g, ch, "@D----@GRunic Skills@D----@n\r\n");
    send_to_char(g, ch, "@Rkenaz\n{}\n{}\n{}\n{}\n{}@n\n", skill >= 40 ? "@Galgiz" : "", skill >= 40 ? "@moagaz" : "", skill >= 60 ? "@Ywunjo" : "", skill >= 80 ? "@rpurisaz" : "",
                 skill >= 100 ? "@mgebo" : "");
    return;
  }
  const rune &r = *picked;
  inkcost += r.ink;
  if (r.ink_checked && amount < inkcost) {
    send_to_char(g, ch, "You do not have a bottle with enough ink. @D[@bInkcost@D: @R{:d}@D]@n\r\n", inkcost);
    return;
  }
  const rune_effect *effect = vict == ch ? r.on_self : r.on_other;
  if (effect)
    write_rune(g, ch, vict, r, *effect, bottle, inkcost, skill, cost);
  if (!r.practised)
    return;
  improve_skill(g, ch, SKILL_RUNIC, 1);
  char_set_wait(ch, PULSE_3SEC);
}

ACMD(do_resize)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);

  if (!char_get_skill(ch, SKILL_BUILD)) {
    send_to_char(g, ch, "You do not have the skill to resize equipment!\r\n");
    return;
  }
  if (char_get_skill(ch, SKILL_BUILD) < 80) {
    send_to_char(g, ch, "Your build skill must be at least level 80 before you can resize equipment.\r\n");
    return;
  }
  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, "Syntax: resize (obj) (small | medium)\r\n");
    return;
  }
  auto found = find_carried(g, ch, arg, "You don't have that object!\r\n");
  if (reject(g, ch, found))
    return;
  struct obj_data *obj = *found;
  if (!wearable_obj(obj)) {
    send_to_char(g, ch, "That is not equipment! You can only resize equipment.\r\n");
    return;
  }
  if (getCurST(g, ch) < obj_get_weight(obj) + (getMaxST(g, ch) / 40)) {
    send_to_char(g, ch, "You do not have enough stamina to resize this object at this time.\r\n");
    return;
  }

  int size;
  if (str_eq_nocase(arg2, "small"))
    size = SIZE_SMALL;
  else if (str_eq_nocase(arg2, "medium"))
    size = SIZE_MEDIUM;
  else {
    send_to_char(g, ch, "Syntax: resize (obj) (small | medium)\r\n");
    return;
  }
  if (obj_get_size(obj) == size) {
    send_to_char(g, ch, size == SIZE_SMALL ? "The equipment is already small sized.\r\n" : "The equipment is already medium sized.\r\n");
    return;
  }
  act(g, "@WYou carefully adjust the size of @c$p@W.@n", true, ch, obj, nullptr, TO_CHAR);
  act(g, "@C$n@W carefully adjusts the size of @c$p@W.@n", true, ch, obj, nullptr, TO_ROOM);
  obj_set_size(obj, size);
  decCurST(g, ch, obj_get_weight(obj) + (getMaxST(g, ch) / 40), 0);
}

/* This is the mortal dig command. */
ACMD(do_bury)
{
  if (!char_has_arms(g, ch)) {
    send_to_char(g, ch, "You have no arms!\r\n");
    return;
  }
  if (char_get_grappling(g, ch) || char_get_grappled(g, ch)) {
    send_to_char(g, ch, "You are busy grappling with someone!\r\n");
    return;
  }
  if (char_get_absorbing(g, ch) || char_get_absorbby(g, ch)) {
    send_to_char(g, ch, "You are busy struggling with someone!\r\n");
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: dig [bury (item) | uncover]\r\n");
    return;
  }

  int sect = room_sect(g, char_get_in_room(ch));
  if (sect != SECT_FIELD && sect != SECT_HILLS && sect != SECT_FOREST && sect != SECT_DESERT && sect != SECT_MOUNTAIN) {
    send_to_char(g, ch, "You are not in a room with enough available dirt or sand to dig.\r\n");
    return;
  }
  bool sand = sect == SECT_DESERT;

  struct obj_data *fobj = nullptr; /* the last buried here */
  for (obj_data *buried : room_contents(g, char_get_in_room(ch)))
    if (obj_extra_flagged(buried, ITEM_BURIED))
      fobj = buried;

  if (str_eq_nocase(arg, "bury")) {
    if (arg2.empty()) {
      send_to_char(g, ch, "Bury what?\r\n");
      return;
    }
    auto found = find_carried(g, ch, arg2, "You don't have that object to bury.\r\n");
    if (reject(g, ch, found))
      return;
    struct obj_data *obj = *found;
    if (fobj != nullptr) {
      send_to_char(g, ch, "There is already something buried near here.\r\n");
      return;
    }
    if (!sand) {
      act(g, "@yYou start digging in a spot of soft dirt. Once you have an appropriately sized hole you drop @G$p@y in and then cover it.@n", true, ch, obj, nullptr, TO_CHAR);
      act(g, "@C$n@y starts digging in a spot of soft dirt. Once $e has an appropriately sized hole $e drops @G$p@y in and then covers it.@n", true, ch, obj, nullptr, TO_ROOM);
    } else {
      act(g, "@YYou start digging in a spot of soft sand. Once you have an appropriately sized hole you drop @G$p@Y in and then cover it.@n", true, ch, obj, nullptr, TO_CHAR);
      act(g, "@C$n@Y starts digging in a spot of soft sand. Once $e has an appropriately sized hole $e drops @G$p@Y in and then covers it.@n", true, ch, obj, nullptr, TO_ROOM);
    }
    obj_from_char(g, obj);
    obj_to_room(g, obj, char_get_in_room(ch));
    obj_extra_set(obj, ITEM_BURIED, true);
    return;
  }
  if (!str_eq_nocase(arg, "uncover")) {
    send_to_char(g, ch, "Syntax: dig [bury (item) | uncover]\r\n");
    return;
  }
  if (fobj == nullptr) {
    send_to_char(g, ch, "There is nothing buried here.\r\n");
    return;
  }
  if (!sand) {
    act(g, "@yYou slowly dig and reveal @G$p@y buried in the dirt! You pull it out and set it on the ground before covering the hole back up.@n", true, ch, fobj, nullptr, TO_CHAR);
    act(g, "@C$n@y starts digging and shortly reveals @G$p@y buried in the dirt! Quickly $e pulls it out and sets it on the ground before covering the hole back up.@n", true, ch, fobj, nullptr, TO_ROOM);
  } else {
    act(g, "@YYou slowly dig and reveal @G$p@Y buried in the sand! You pull it out and set it on the ground before covering the hole back up.@n", true, ch, fobj, nullptr, TO_CHAR);
    act(g, "@C$n@Y starts digging and shortly reveals @G$p@Y buried in the sand! Quickly $e pulls it out and sets it on the ground before covering the hole back up.@n", true, ch, fobj, nullptr, TO_ROOM);
  }
  obj_extra_set(fobj, ITEM_BURIED, false);
}

/* This determines of an object is a suitable bundle of silk or not */
bool valid_silk(GameWorld *g, struct obj_data *obj)
{
  constexpr obj_vnum BUNDLES[] = {16700, 16701, 16702, 16703, 16704, 16708};
  return std::ranges::find(BUNDLES, obj_get_vnum(g, obj)) != std::end(BUNDLES);
}

ACMD(do_silk)
{
  if (!know_skill(g, ch, SKILL_SILK))
    return;

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);
  int prob = char_get_skill(ch, SKILL_SILK), perc = rand_number(g, 1, 120);

  if (str_eq_nocase(arg, "weave"))
    weave_silk(g, ch, arg2, prob, perc);
  else if (str_eq_nocase(arg, "bundle"))
    spin_silk(g, ch, prob, perc);
  else
    send_to_char(g, ch, "Syntax: silk (weave | bundle)\r\n");
}

ACMD(do_cook)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (char_is_npc(ch))
    return;
  int cook_elem = cook_element(g, char_get_in_room(ch));
  if (!cook_elem) {
    send_to_char(g, ch, "You need a campfire or Flambus Stove nearby to cook.\r\n");
    return;
  }
  if (!char_get_skill(ch, SKILL_COOKING)) {
    send_to_char(g, ch, "You don't even know the basics!\r\n");
    return;
  }

  int skill = char_get_skill(ch, SKILL_COOKING), prob = axion_dice(g, 0);
  if (arg.empty()) {
    send_to_char(g, ch, "@D---------------------@RCooking@D---------------------@n\r\n");
    send_to_char(g, ch, "@Y 1@B) @CCooked Steak		@Y17@B) @CCarambola Bread@n\r\n");
    send_to_char(g, ch, "@Y 2@B) @CTomato Soup		@n\r\n");
    send_to_char(g, ch, "@Y 3@B) @CPotato Soup		@n\r\n");
    send_to_char(g, ch, "@Y 4@B) @CVegetable Soup		@n\r\n");
    send_to_char(g, ch, "@Y 5@B) @CMeat Stew			@n\r\n");
    send_to_char(g, ch, "@Y 6@B) @CChili Soup		@n\r\n");
    send_to_char(g, ch, "@Y 7@B) @CGrilled Fish		@n\r\n");
    send_to_char(g, ch, "@Y 8@B) @CGood Grilled Fish		@n\r\n");
    send_to_char(g, ch, "@Y 9@B) @CGreat Grilled Fish	@n\r\n");
    send_to_char(g, ch, "@Y10@B) @CMagnificent Grilled Fish	@n\r\n");
    send_to_char(g, ch, "@Y11@B) @CCooked White Rice		@n\r\n");
    send_to_char(g, ch, "@Y12@B) @CSushi			@n\r\n");
    send_to_char(g, ch, "@Y13@B) @CWhite Bread		@n\r\n");
    send_to_char(g, ch, "@Y14@B) @CBasic Salad		@n\r\n");
    send_to_char(g, ch, "@Y15@B) @CAppleplum Chasan		@n\r\n");
    send_to_char(g, ch, "@Y16@B) @CFrozen Berry Muffin	@n\r\n");
    send_to_char(g, ch, "@wSyntax: cook (recipe number)@n\r\n");
    return;
  }

  int num = parse_int(arg);
  if (num < 1 || num > static_cast<int>(std::size(DISHES))) {
    send_to_char(g, ch, "That is not a valid dish!\r\n");
    return;
  }
  const dish &d = DISHES[num - 1];
  prob += d.difficulty;

  if (!gather_ingredients(g, ch, d, false))
    return;
  if (cook_elem == 1 && !d.campfire) {
    send_to_char(g, ch, "You can not cook that dish over a campfire.\r\n");
    return;
  }
  gather_ingredients(g, ch, d, true);
  if (skill < prob) {
    act(g, "@wYou screw up the preparation of the recipe and end up wasting the ingredients!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@C$n@w starts to prepare some food, but ends up ruining the ingredients instead!@n", true, ch, nullptr, nullptr, TO_ROOM);
    improve_skill(g, ch, SKILL_COOKING, 0);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  struct obj_data *meal = read_object(g, d.meal);
  obj_to_char(g, meal, ch);
  int psbonus = d.psbonus, expbonus = d.expbonus;
  if (char_get_bonuses(ch, BONUS_RECIPE)) {
    psbonus += 1;
    expbonus += 3;
  }
  act(g, "@wYou carefully prepare the ingredients and then start cooking them. After a while of patience  and skillful care you successfully make @D'@C$p@D'@w!@n", true, ch, meal, nullptr, TO_CHAR);
  act(g, "@C$n@w carefully prepares some ingredients and starts cooking them. After a while of patience and skillful care $e succeeds in making @D'@C$p@D'@w!@n", true, ch, meal, nullptr, TO_ROOM);
  improve_skill(g, ch, SKILL_COOKING, 0);

  if (psbonus > 0 && skill * 0.10 > 0)
    psbonus = (skill * 0.10) * psbonus;
  if (expbonus > 0)
    expbonus = skill * expbonus;
  obj_set_value(meal, VAL_FOOD_PSBONUS, psbonus);
  obj_set_value(meal, VAL_FOOD_EXPBONUS, expbonus);
  char_set_wait(ch, PULSE_2SEC);
}

/* Feed senzu to someone you are grouped with. Why? TEAMWORK! */
ACMD(do_feed)
{
  if (char_is_npc(ch))
    return;

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Feed a senzu to whom?\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "That target isn't here.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (char_is_android(vict)) {
    send_to_char(g, ch, "They are unaffected by senzu beans.\r\n");
    return;
  }
  auto bean = find_carried(g, ch, arg2, "You need to give them a senzu.\r\n");
  if (reject(g, ch, bean))
    return;
  struct obj_data *obj = *bean;
  if (obj_get_type_flag(obj) != ITEM_POTION) {
    send_to_char(g, ch, "You can only feed senzu beans.\r\n");
    return;
  }
  if (obj_extra_flagged(obj, ITEM_FORGED)) {
    send_to_char(g, ch, "They can't swallow that, it is fake!\r\n");
    return;
  }
  if (obj_extra_flagged(obj, ITEM_BROKEN)) {
    send_to_char(g, ch, "They can't swallow that, it is broken!\r\n");
    return;
  }
  if (char_get_fighting(g, vict)) {
    send_to_char(g, ch, "They are a bit busy at the moment!\r\n");
    return;
  }
  if (!grouped_with(g, ch, vict)) {
    send_to_char(g, ch, "You need to be grouped with them first.\r\n");
    return;
  }
  act_triple(g, {"@WYou take $p@W and pop it into @C$N@W's mouth!@n", "@C$n@W takes $p@W and pops it into YOUR mouth!@n", "@C$n@W takes $p@W and pops it into @c$N@W's mouth!@n", true}, ch, obj, vict);
  mag_objectmagic(g, vict, obj, "");
}

/* This allows players to decapitate a corpse for a sick trophy. */
ACMD(do_spoil)
{
  if (char_is_npc(ch))
    return;

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "What corpse do you want to decapitate?\r\n");
    return;
  }
  auto found = find_in_room(g, ch, arg, "No corpse around here by that name.\r\n");
  if (reject(g, ch, found))
    return;
  struct obj_data *obj = *found;
  if (obj_get_value(obj, VAL_CORPSE_HEAD) == 0) {
    send_to_char(g, ch, "That corpse is already missing its head.\r\n");
    return;
  }

  /* A blade cuts it off; the weapon in the first hand, if there is one, else the second */
  struct obj_data *weapon = char_get_equipment(ch, WEAR_WIELD1) ? char_get_equipment(ch, WEAR_WIELD1) : char_get_equipment(ch, WEAR_WIELD2);
  int damtype             = weapon ? obj_get_value(weapon, VAL_WEAPON_DAMTYPE) : -1;
  if (damtype == TYPE_SLASH - TYPE_HIT || damtype == TYPE_PIERCE - TYPE_HIT || damtype == TYPE_STAB - TYPE_HIT) {
    act(g, "@C$n@W reaches down and @rcuts@W the head off of @R$p@W!@n", true, ch, obj, nullptr, TO_ROOM);
    act(g, "@WYou reach down and @rcut@W the head off of @R$p@W!@n", true, ch, obj, nullptr, TO_CHAR);
  } else {
    act(g, "@C$n@W reaches down and @rtears@W the head off of @R$p@W!@n", true, ch, obj, nullptr, TO_ROOM);
    act(g, "@WYou reach down and @rtear@W the head off of @R$p@W!@n", true, ch, obj, nullptr, TO_CHAR);
  }
  obj_set_value(obj, VAL_CORPSE_HEAD, 0);

  struct obj_data *head = create_obj(g);
  obj_set_item_number(head, NOTHING);
  obj_set_in_room(head, NOWHERE);
  std::string part = obj_get_name(obj);
  for (const char *word : {"headless", "corpse", "half", "burnt", "chunks", "beaten", "bloody"})
    search_replace(part, word, "");
  trim(part);
  obj_set_name(head, str_format("bloody head {}", part).c_str());
  obj_set_description(head, str_format("@wThe bloody head of {}@w is lying here@n", part).c_str());
  obj_set_short_description(head, str_format("@wThe bloody head of {}@w@n", part).c_str());

  obj_set_type_flag(head, ITEM_OTHER);
  obj_wear_set(head, ITEM_WEAR_TAKE, true);
  obj_extra_set(head, ITEM_UNIQUE_SAVE, true);
  obj_set_value(head, 0, 0);
  obj_set_value(head, 1, 0);
  obj_set_value(head, 2, 0);
  obj_set_value(head, 3, 0);
  obj_set_value(head, 4, 1);
  obj_set_value(head, 5, 1);
  obj_set_weight(head, rand_number(g, 4, 10));
  obj_set_cost_per_day(head, 0);
  add_unique_id(g, head);
  obj_to_room(g, head, char_get_in_room(ch));
  obj_from_room(g, head);
  obj_to_char(g, head, ch);
}
