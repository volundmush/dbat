/* ************************************************************************
*  File: misc/techniques.c                                    Part of DBAT       *
*  Usage: Race and class technique commands                               *
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

#include "misc.hpp"

namespace {
  /* A multiform of ch's, here with them */
  bool is_clone_of(GameWorld *g, struct char_data *tch, struct char_data *ch)
  {
    return tch != ch && char_is_npc(tch) && char_get_mob_vnum(g, tch) == 25 && char_get_original(g, tch) == ch;
  }

  void generate_multiform(GameWorld *g, struct char_data *ch)
  {
    constexpr mob_vnum CLONE = 25;
    if (!mob_proto_of(g, CLONE)) {
      send_to_imm(g, "Multiform Clone doesn't exist!");
      return;
    }

    std::string clone_name  = str_format("{}'s Clone", char_get_alias(ch));
    std::string clone_sdesc = str_format("{}'s @CClone@n", char_get_alias(ch));
    std::string clone_ldesc = str_format("{}'s @CClone@w is standing here.@n\n", char_get_alias(ch));

    char_data *clone = read_mobile(g, CLONE);
    char_set_alias(clone, clone_name.c_str());
    char_set_sdesc(clone, clone_sdesc.c_str());
    char_set_ldesc(clone, clone_ldesc.c_str());
    if (char_get_ddesc(ch))
      char_set_ddesc(clone, char_get_ddesc(ch));
    char_set_race(clone, char_get_race(ch));
    char_set_class(clone, char_get_class(ch));
    char_set_alignment(clone, char_get_alignment(ch));

    // Make the physical appearance match!
    char_set_sex(clone, char_get_sex(ch));
    char_set_hairl(clone, char_get_hairl(ch));
    char_set_hairs(clone, char_get_hairs(ch));
    char_set_hairc(clone, char_get_hairc(ch));
    char_set_skin(clone, char_get_skin(ch));
    char_set_eye(clone, char_get_eye(ch));
    char_set_distfea(clone, char_get_distfea(ch));
    char_set_aura(clone, char_get_aura(ch));

    char_set_weight(clone, char_get_weight(ch));
    char_set_height(clone, char_get_height(ch));
    char_set_size(clone, char_get_size(ch));
    char_set_class_level(clone, char_get_class_level(ch));
    char_copy_time(clone, ch);

    char_set_tail_growth(clone, char_get_tail_growth(ch));
    char_set_transclass(ch, char_get_transclass(ch)); /* quirk: sets ch's own, not the clone's */

    // Copying these values, but it shouldn't matter because clones no longer work this way.
    char_set_basepl(clone, getBasePL(ch));
    char_set_baseki(clone, getBaseKI(ch));
    char_set_basest(clone, getBaseST(ch));

    // Bioandroid Genome copy...
    char_set_genome(clone, 0, char_get_genome(ch, 0));
    char_set_genome(clone, 1, char_get_genome(ch, 1));

    for (int l = 0; l < 3; l++) { /* quirk: the left leg isn't copied */
      char_get_limbs(clone)[l] = char_get_limbs(ch)[l];
      char_set_limb_condition(clone, l, char_get_limb_condition(ch, l));
    }

    char_set_clones(ch, char_get_clones(ch) + 1);
    char_set_original(clone, ch);
    char_to_room(g, clone, char_get_in_room(ch));
    add_follower(g, clone, ch);
  }

  /* A grappler, a grappled one or someone dragged can't hold on to liquid */
  void let_go(GameWorld *g, struct char_data *ch)
  {
    if (char_get_grappled(g, ch)) {
      char_set_grappling(char_get_grappled(g, ch), nullptr);
      char_set_grappled(ch, nullptr);
    }
    if (char_get_grappling(g, ch)) {
      char_set_grappled(char_get_grappling(g, ch), nullptr);
      char_set_grappling(ch, nullptr);
    }
    if (char_get_drag(g, ch)) {
      char_set_dragged(char_get_drag(g, ch), nullptr);
      char_set_drag(ch, nullptr);
    }
  }

  double liquefy_cost(GameWorld *g, struct char_data *ch)
  {
    return (getMaxKI(g, ch) * .002) + 150;
  }

  constexpr const char *LIQUEFY_SYNTAX = "Syntax: liquefy hide\nSyntax: liquefy explode (target)\r\n";

  void liquefy_fumble(GameWorld *g, struct char_data *ch)
  {
    act(g, "@MYour body starts to become loose and sag, but you lose focus and it reverts to its original shape!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@m$n@M's body starts to become loose and sag, but $e seems to return normal a moment later.@n", true, ch, nullptr, nullptr, TO_ROOM);
    decCurKI(g, ch, liquefy_cost(g, ch), 0);
  }

  void liquefy_explode(GameWorld *g, struct char_data *ch, std::string &target)
  {
    let_go(g, ch);
    if (target.empty()) {
      send_to_char(g, ch, LIQUEFY_SYNTAX);
      return;
    }
    if (getCurKI(g, ch) < (getMaxKI(g, ch) * 0.10) + 150) {
      send_to_char(g, ch, "You do not have enough ki for that action!@n\r\n");
      return;
    }
    auto found = find_char(g, ch, target, FIND_CHAR_ROOM, "That target isn't here.\r\n");
    if (reject(g, ch, found))
      return;
    struct char_data *vict = *found;
    if (!can_kill(g, ch, vict, nullptr, 1)) {
      send_to_char(g, ch, "You can't kill them!\r\n");
      return;
    }
    if (axion_dice(g, 0) > char_get_level(ch)) {
      liquefy_fumble(g, ch);
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    if (char_get_speedi(g, ch) < char_get_speedi(g, vict)) {
      act_triple(g,
                 {"@MYour body rapidly turns to liquid and flies for @R$N's@M open mouth! However $E easily dodges and avoids your attempt!@n",
                  "@m$n@M's body rapidly turns to liquid and flies for @RYOUR@M open mouth! However you are faster and managed to dodge the attempt.@n",
                  "@m$n@M's body rapidly turns into liquid and flies for @R$N's@M open mouth! However $E easily dodges and avoids @m$n's@M attempt!@n", true},
                 ch, nullptr, vict);
      decCurKI(g, ch, liquefy_cost(g, ch), 0);
      if (!char_get_fighting(g, ch))
        set_fighting(g, ch, vict);
      if (!char_get_fighting(g, vict))
        set_fighting(g, vict, ch);
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    if (getCurPL(g, ch) < getCurPL(g, vict) * 2) {
      act_triple(g,
                 {"@MYour body rapidly turns to liquid and flies for @R$N's@M open mouth! However as you force yourself in through $S mouth $E successfully resists and forces your back out!@n",
                  "@m$n@M's body rapidly turns to liquid and flies for @RYOUR@M open mouth! However you think quickly and force $m out before $e has a chance to get fully into your body!@n",
                  "@m$n@M's body rapidly turns into liquid and flies for @R$N's@M open mouth! However as $e forces $mself in through @R$N's@M mouth $E manages to resist and force @m$n@M back out!@n", true},
                 ch, nullptr, vict);
      decCurKI(g, ch, liquefy_cost(g, ch), 0);
      int64_t dmg = getEffMaxPL(g, ch) * 0.08;
      hurt(g, 0, 0, ch, vict, nullptr, dmg, 0);
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    act_triple(g,
               {"@MYour body rapidly turns to liquid and flies for @R$N's@M open mouth! As you fill $S body you expand outward until $s body explodes into a gory mess!@n",
                "@m$n@M's body rapidly turns to liquid and flies for @RYOUR@M open mouth! As $e fills your body it begins to expand until it is unable to take the strain any longer and explodes!@n",
                "@m$n@M's body rapidly turns into liquid and flies for @R$N's@M open mouth! As $e forces $mself in through @R$N's@M mouth $S body begins to expand until it can't take the strain any longer and explodes!@n", true},
               ch, nullptr, vict);
    decCurKI(g, ch, liquefy_cost(g, ch), 0);
    if (char_aff_flagged(ch, AFF_GROUP))
      group_gain(g, ch, vict);
    else
      solo_gain(g, ch, vict);
    die(g, vict, ch);
    char_aff_set(ch, AFF_LIQUEFIED, true);
    char_set_wait(ch, PULSE_3SEC);
    handle_cooldown(g, ch, 9);
  }

  /* How much ki each size of ash cloud takes, and how long it hangs */
  struct ash_size {
    int64_t intel_mult;
    double ki_share;
    int timer;
    int thickness;
    const char *aftershock;
  };
  constexpr ash_size ASH_SIZES[] = {
    {20, 0.25, 1, 1, nullptr},
    {10, 0.10, 2, 2, "@WThe ashes ripple with a strong aftershock of power.@n\r\n"},
    {5, 0.05, 4, 3, "@WThe ashes ripple with an intense aftershock of power.@n\r\n"},
  };

  /* Where shimmer goes by name */
  struct shimmer_place {
    const char *name;
    room_vnum room;
  };
  constexpr shimmer_place SHIMMER_PLACES[] = {
    {"planet-earth", 300},   {"planet-namek", 10222},  {"planet-frigid", 4017}, {"planet-vegeta", 2200},
    {"planet-konack", 8006}, {"planet-aether", 12024}, {"afterlife", 6000},
  };

  void shimmer_usage(GameWorld *g, struct char_data *ch)
  {
    send_to_char(g, ch, "Who or where do you want to shimmer to? [target | planet-(planet name) | afterlife]\r\n");
    send_to_char(g, ch, "Example: shimmer goku\nExample 2: shimmer planet-earth\r\n");
  }

  constexpr const char *SHIMMER_SELF = "Moving to yourself would be kinda impossible wouldn't it? If not that then it would at least be pointless.\r\n";

  void hydromancy_syntax(GameWorld *g, struct char_data *ch, const char *first)
  {
    send_to_char(g, ch, first);
    send_to_char(g, ch, "Example: hydromancy flood nw\r\n");
    send_to_char(g, ch, "\nSyntax 2: hydromancy spike\r\n");
  }

  void hydromancy_spike(GameWorld *g, struct char_data *ch, int skill, int chance)
  {
    int64_t cost = 100 + (char_get_skill(ch, SKILL_STYLE) / (1 + (getMaxKI(g, ch) * 0.5)));
    if (getCurKI(g, ch) < cost) {
      send_to_char(g, ch, "You do not have enough ki to form an ice spike.\r\n");
      return;
    }
    if (skill < chance) {
      decCurKI(g, ch, cost, 0);
      act(g, "@CYou press your palms together in front of your body but you fail to produce the proper control to form the spike!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@c$n@C presses $s palms together and then slowly pulls them apart. Nothing important appears to have happened.", true, ch, nullptr, nullptr, TO_ROOM);
      improve_skill(g, ch, SKILL_STYLE, 2);
      return;
    }

    struct obj_data *obj = nullptr;
    if (skill >= 100)
      obj = read_object(g, 19058);
    else if (skill >= 50)
      obj = read_object(g, 19057);
    else if (skill >= 1)
      obj = read_object(g, 19056);
    if (!obj) {
      send_to_char(g, ch, "You fail to form an ice spike.\r\n");
      return;
    }

    decCurKI(g, ch, cost, 0);
    act(g, "@CYou press your palms together in front of your body and focusing ki you force water up along your body. That water pools between your palms and as pull your palms apart a @c$p@C forms!@n", true, ch, obj, nullptr, TO_CHAR);
    /* quirk: TO_VICT with no victim, so the room never sees it */
    act(g, "@c$n@C presses $s palms together in front of $s body and water begins to flow up $s body and pools between $s palms. Slowly pulling them apart reveals a @c$p@C as it forms between them!@n", true, ch, obj, nullptr, TO_VICT);
    if (obj_get_weight(obj) + getCurCarriedWeight(ch) <= getMaxCarryWeight(g, ch)) {
      obj_to_char(g, obj, ch);
    } else {
      send_to_char(g, ch, "You are unable to hold it and so let it go at your feet.\r\n");
      act(g, "@C$n@w drops an ice spike.@n", true, ch, nullptr, nullptr, TO_ROOM);
      obj_to_room(g, obj, char_get_in_room(ch));
    }
    improve_skill(g, ch, SKILL_STYLE, 1);
    char_set_con_cooldown(ch, 10);
  }

  /* What the flood does to someone else in the room */
  void flood_over(GameWorld *g, struct char_data *ch, struct char_data *vict, int dir, int64_t cost)
  {
    if (!can_kill(g, ch, vict, nullptr, 1)) {
      /* quirk: TO_VICT with no victim, so vict isn't told */
      act(g, "@CYou are protected from the water!@n", true, vict, nullptr, nullptr, TO_VICT);
      act(g, "@C$n@C is protected from the water!@n", true, vict, nullptr, nullptr, TO_ROOM);
    } else if (char_is_kanassan(vict)) {
      act(g, "@CYou effortlessly swim against the current.@n", true, vict, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@C effortlessly swims against the current.@n", true, vict, nullptr, nullptr, TO_ROOM);
    } else if (char_get_skills_base(vict, SKILL_BALANCE) >= axion_dice(g, -10)) {
      act(g, "@CYou manage to keep your balance and are not swept away!@n", true, vict, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@C manages to keep $s balance and is not swept away!@n", true, vict, nullptr, nullptr, TO_ROOM);
    } else if (char_aff_flagged(ch, AFF_FLYING)) { /* quirk: whether ch flies, not vict */
      act(g, "@CYou fly above the rushing waters and are untouched.@n", true, vict, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@C flies above the rushing waters and is untouched.@n", true, vict, nullptr, nullptr, TO_ROOM);
    } else {
      act(g, "@cYou are caught by the rushing waters and sent tumbling away!@n", true, vict, nullptr, nullptr, TO_CHAR);
      act(g, "@c$n@c is caught by the rushing waters and sent tumbling away!@n", true, vict, nullptr, nullptr, TO_ROOM);
      do_simple_move(g, vict, dir, true);
      hurt(g, 0, 0, ch, vict, nullptr, cost * 4, 1);
    }
  }

  void hydromancy_flood(GameWorld *g, struct char_data *ch, const std::string &where, int skill, int chance, int64_t cost)
  {
    if (where.empty()) {
      hydromancy_syntax(g, ch, "Syntax 1: hydromancy flood (direction)\r\n");
      return;
    }
    int dir = search_block(where, dirs, false);
    if (!char_can_go(g, ch, dir)) {
      send_to_char(g, ch, "You can not flood the water that direction!\r\n");
      return;
    }

    int last = char_get_lastattack(ch);
    char_set_lastattack(ch, 500);
    if (skill < chance) {
      act(g, "@BUsing your ki you attempt to create a rush of water! @RYou fail!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@b$n@B seems to attempt to create water with $s ki! @RHowever, $e fails!@n", true, ch, nullptr, nullptr, TO_ROOM);
      decCurKI(g, ch, cost, 0);
      char_set_wait(ch, PULSE_2SEC);
      return; /* quirk: lastattack stays 500 */
    }
    decCurKI(g, ch, cost, 0);
    act(g, str_format("@BUsing your ki you create a rush of water flooding away toward the @C{}@B!@n", dirs[dir]).c_str(), true, ch, nullptr, nullptr, TO_CHAR);
    act(g, str_format("@B$n@B uses $s ki to create a rush of water flooding away toward the @C{}@B!@n", dirs[dir]).c_str(), true, ch, nullptr, nullptr, TO_ROOM);
    for (char_data *vict : room_people(g, char_get_in_room(ch)))
      if (vict != ch)
        flood_over(g, ch, vict, dir, cost);
    room_set_geffect(g, char_get_exit(g, ch, dir)->to_room, -3);
    char_set_lastattack(ch, last);
    char_set_wait(ch, PULSE_2SEC);
    char_set_con_cooldown(ch, 15);
  }

  /* How a swing of ensnaring silk turns out */
  struct snare_outcome {
    act_msgs msgs;
    bool ensnares;
    bool vict_zanzoken_spent;
    bool ch_zanzoken_spent;
  };
  constexpr snare_outcome SNARE_FUMBLED = {
    {"@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Unfortunately you miss and lose the bundle...@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Fortunately $e misses and loses the bundle...@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Fortunately $e misses and loses the bundle...@n", true},
    false, false, false};
  constexpr snare_outcome SNARE_ZANZOKENED = {
    {"@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Unfortunately @c$N@W zanzokens away avoiding it and you lose the bundle...@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Fortunately you zanzoken away avoiding it and @C$n@W loses the bundle...@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Fortunately @c$N@W zanzokens away avoiding it and @C$n@W loses the bundle...@n", true},
    false, true, false};
  constexpr snare_outcome SNARE_BOTH_ZANZOKEN_MISS = {
    {"@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! You both zanzoken! Unfortunately @c$N@W manages to avoid it and you lose the bundle...@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! You both zanzoken! Fortunately you manage to avoid it and @C$n@W loses the bundle...@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! They both zanzoken! Fortunately @c$N@W manages to avoid it and @C$n@W loses the bundle...@n", true},
    false, true, true};
  constexpr snare_outcome SNARE_BOTH_ZANZOKEN_HIT = {
    {"@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Fortunately you manage to hit $M! You both zanzoken! Quickly you spin around $M and ensnare $S arms with the silk!@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Unfortunately $e manages to hit YOU! You both zanzoken! Quickly $e spins around you and ensnares your arms with the silk!@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Unfortunately $e manages to hit $M! They both zanzoken! Quickly $e spins around @c$N@W and ensnares $S arms with the silk!@n", true},
    true, true, true};
  constexpr snare_outcome SNARE_ZANZOKEN_HIT = {
    {"@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Fortunately you manage to hit $M! Quickly you zanzoken and spin around $M and ensnare $S arms with the silk!@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Unfortunately $e manages to hit YOU! Quickly $e zanzokens and spins around you and ensnares your arms with the silk!@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Unfortunately $e manages to hit $M! Quickly $e zanzokens and spins around @c$N@W and ensnares $S arms with the silk!@n", true},
    true, false, true};
  constexpr snare_outcome SNARE_DODGED = {
    {"@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Unfortunately @c$N@W manages to avoid it and you lose the bundle...@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Fortunately you manage to avoid it and @C$n@W loses the bundle...@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Fortunately @c$N@W manages to avoid it and @C$n@W loses the bundle...@n", true},
    false, false, false};
  constexpr snare_outcome SNARE_HIT = {
    {"@WYou unwind your bundle of silk and grab a loose end of it. Splitting that end to reveal the sticky innards of the strand you swing the strand at @c$N@W! Fortunately you manage to hit $M! Quickly you spin around $M and ensnare $S arms with the silk!@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at YOU! Unfortunately $e manages to hit YOU! Quickly $e spins around you and ensnares your arms with the silk!@n",
     "@C$n@W unwinds a bundle of silk and grabs a loose end of it. Splitting that end to reveal the sticky innards of the strand $e swings the strand at @c$N@W! Unfortunately $e manages to hit $M! Quickly $e spins around @c$N@W and ensnares $S arms with the silk!@n", true},
    true, false, false};

  /* vict is quicker than ch, this time */
  bool outpaced(GameWorld *g, struct char_data *ch, struct char_data *vict)
  {
    return char_get_speedi(g, ch) + rand_number(g, 1, 100) < char_get_speedi(g, vict) + rand_number(g, 1, 100);
  }

  /* The seas warp pool reaches */
  struct warp_sea {
    const char *planet;
    int flag; /* the rooms of that planet */
    const char *already;
    room_vnum room;
  };
  constexpr warp_sea WARP_SEAS[] = {
    {"earth", ROOM_EARTH, "You are already on Earth!\r\n", 850},
    {"frigid", ROOM_FRIGID, "You are already on Frigid!\r\n", 4609},
    {"kanassa", ROOM_KANASSA, "You are already on Kanasssa!\r\n", 15100},
    {"namek", ROOM_NAMEK, "You are already on Namek!\r\n", 10904},
    {"aether", ROOM_AETHER, "You are already on Aether!\r\n", 12252},
  };

  bool at_sea(GameWorld *g, room_vnum r)
  {
    return (r >= 4600 && r < 4700) || (r >= 795 && r < 1099) || (r >= 15100 && r < 15299) || (r >= 13155 && r < 13199) ||
           (room_flagged(g, r, ROOM_NAMEK) && room_sect(g, r) == SECT_WATER_NOSWIM) || (r >= 12103 && r < 12289);
  }

  /* The directions a glacial wall can block, and the side of the wall seen from the other room */
  struct wall_side {
    const char *name;
    int dir;
    int opposite;
  };
  constexpr wall_side WALL_SIDES[] = {
    {"n", 0, 2}, {"e", 1, 3},   {"s", 2, 0},  {"w", 3, 1},  {"u", 4, 5},  {"d", 5, 4},
    {"i", 10, 11}, {"o", 11, 10}, {"nw", 6, 8}, {"ne", 7, 9}, {"se", 8, 6}, {"sw", 9, 7},
  };
} // namespace

ACMD(do_spiritcontrol)
{
  if (!char_get_skill(ch, SKILL_SPIRITCONTROL)) {
    send_to_char(g, ch, "You do not know how to perform that technique.\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_SPIRITCONTROL)) {
    send_to_char(g, ch, "You have already concentrated and have full control of your spirit.\r\n");
    return;
  }
  int64_t cost = getMaxKI(g, ch) * 0.2;
  if (getCurST(g, ch) < cost) {
    send_to_char(g, ch, "You need at least 20% of your max ki in stamina to prepare this skill.\r\n");
    return;
  }
  decCurST(g, ch, cost, 0);
  act(g, "@YYou concentrate and quantify every last bit of your spiritual and mental energies. You have full control of them and can bring them forth in an instant.@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@y$n@Y seems to concentrate hard for a moment.@n", true, ch, nullptr, nullptr, TO_ROOM);
  int duration = rand_number(g, 2, 4);
  assign_affect(g, ch, AFF_SPIRITCONTROL, SKILL_SPIRITCONTROL, duration, 0, 0, 0, 0, 0, 0);
}

ACMD(do_tailhide)
{
  if (char_is_npc(ch))
    return;
  if (!char_is_saiyan(ch) && !char_is_halfbreed(ch)) {
    send_to_char(g, ch, "You have no need to hide your tail!\r\n");
    return;
  }
  if (!char_plr_flagged(ch, PLR_TAILHIDE)) {
    char_plr_set(ch, PLR_TAILHIDE, true);
    act(g, "You tuck your tail away, hiding it from view.", false, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n tucks $s tail away, hiding it from view.", false, ch, nullptr, nullptr, TO_ROOM);
  } else {
    char_plr_set(ch, PLR_TAILHIDE, false);
    act(g, "You have decided to display your tail for all to see!", false, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n has decided to display $s tail for all to see!", false, ch, nullptr, nullptr, TO_ROOM);
  }
}

ACMD(do_nogrow)
{
  if (char_is_npc(ch))
    return;
  if (!char_is_saiyan(ch) && !char_is_halfbreed(ch)) {
    send_to_char(g, ch, "What do you mean?\r\n");
    return;
  }
  if (!char_plr_flagged(ch, PLR_NOGROW)) {
    char_plr_set(ch, PLR_NOGROW, true);
    send_to_char(g, ch, "You have decided to halt your tail growth!\r\n");
  } else {
    char_plr_set(ch, PLR_NOGROW, false);
    send_to_char(g, ch, "You have decided to regrow your tail!\r\n");
  }
}

ACMD(do_multiform)
{
  if (!char_is_npc(ch) && !char_get_skill(ch, SKILL_MULTIFORM)) {
    send_to_char(g, ch, "You do not know how to perform that technique.\r\n");
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  auto mine             = [&](char_data *tch) { return is_clone_of(g, tch, ch); };

  if (str_eq_nocase(arg, "merge")) {
    if (std::ranges::none_of(room_people(g, char_get_in_room(ch)), mine)) {
      send_to_char(g, ch, "You have no multiforms present to merge with!\r\n");
      return;
    }
    for (char_data *tch : room_people(g, char_get_in_room(ch)))
      if (mine(tch))
        extract_char(g, tch);
    return;
  }

  if (!str_eq_nocase(arg, "split")) {
    send_to_char(g, ch, "Huh? Try help multiform\r\n");
    return;
  }
  int64_t cost = (getMaxKI(g, ch) * 0.005) + (getMaxST(g, ch) * 0.005) + 2;
  int penalty  = char_get_fighting(g, ch) ? rand_number(g, 8, 15) : 0;
  int roll     = axion_dice(g, penalty);
  cost *= (char_get_skill(ch, SKILL_MULTIFORM) * 0.2);

  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki to split!\r\n");
    return;
  }
  if (getCurST(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough stamina to split!\r\n");
    return;
  }
  improve_skill(g, ch, SKILL_MULTIFORM, 1);

  if (char_get_skill(ch, SKILL_MULTIFORM) < roll) {
    act(g, "@YYou focus your ki into your body while concentrating on the image of your body splitting into two. @yYou lose your concentration and fail to split though...@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@y$n@Y seems to concentrate really hard for a moment, before relaxing.@n", true, ch, nullptr, nullptr, TO_ROOM);
    decCurST(g, ch, cost, 0);
    decCurKI(g, ch, cost, 0);
    return;
  }
  act(g, "@YYou focus your ki into your body while concentrating on the image of your body splitting into two. Another you splits out of your body!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@YSuddenly @y$n@Y seems to concentrates really and after a brief moment splits into two copies of $mself!@n", true, ch, nullptr, nullptr, TO_ROOM);
  generate_multiform(g, ch);
}

void handle_multi_merge(GameWorld *g, struct char_data *form)
{
  struct char_data *ch = char_get_original(g, form);
  if (ch == nullptr)
    return;

  send_to_char(g, ch, "@YYou merge with one of your forms!@n\r\n");
  act(g, "@y$n@Y merges with one of his multiforms!@n\r\n", true, ch, nullptr, nullptr, TO_ROOM);
  char_set_clones(ch, MAX(0, char_get_clones(ch) - 1));
  extract_char(g, form);
}

ACMD(do_moondust)
{
  int64_t cost = getMaxST(g, ch) * 0.02;

  if (!char_is_arlian(ch) || char_get_sex(ch) != SEX_FEMALE) {
    send_to_char(g, ch, "You are not an arlian female.\r\n");
    return;
  }
  if (!char_aff_flagged(ch, AFF_GROUP)) {
    send_to_char(g, ch, "You need to be in a group to use this skill!\r\n");
    return;
  }

  cost += getMaxLF(g, ch) * 0.02;
  int64_t heal = cost * 3;
  if (getCurPL(g, ch) >= getEffMaxPL(g, ch) * 0.8)
    cost = cost * 0.5;
  if (getCurST(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough stamina to perform this technique.\r\n");
    return;
  }

  int chance = axion_dice(g, 0);
  if (chance > char_get_aff_abils_wis(ch) + rand_number(g, 1, 10)) {
    act(g, "@GYou spread your wings and begin to concentrate. Your wings begin to glow a soft sea green color. As you prepare to release a cloud of your charged wing dust you lose focus and the power you had begun to charge into your wings dissipates.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@g$n@G spreads $s wings and seems to concentrate for a moment. Suddenly $s wings begin to glow a soft sea green color. This soft glow grows brighter for a second before fading completely.@n", true, ch, nullptr, nullptr, TO_ROOM);
    decCurST(g, ch, cost, 0);
    char_set_wait(ch, PULSE_1SEC);
    return;
  }
  incCurHealth(g, ch, heal, true);
  decCurST(g, ch, cost, 0);
  char_set_wait(ch, PULSE_1SEC);

  act(g, "@GYou spread your wings and begin to concentrate. Your wings begin to glow a soft sea green color. As your wings grow brighter you focus your charged bio energy in a shockwave the unleashes a cloud of glowing green dust. You breath in the dust and feel it rejuvinate your body's cells!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@g$n@G spreads $s wings and seems to concentrate for a moment. Suddenly $s wings begin to glow a soft sea green color. This soft glow grows brighter and as $e flexes $s wings to their full extent a shockwave of energy explodes outward. Carried on this shockwave is a cloud of glowing dust! You notice some of the dust being breathed in by $s!@n", true, ch, nullptr, nullptr, TO_ROOM);
  send_to_char(g, ch, "@RHeal@Y: @C{}@n\r\n", add_commas(heal));

  for (char_data *vict : room_people(g, char_get_in_room(ch))) {
    if (vict == ch || !grouped_with(g, ch, vict))
      continue;
    incCurHealth(g, vict, heal, true);
    act(g, "@CYou breathe in the dust and are healed by it somewhat!@n", true, vict, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@C breathes in the dust and is healed somewhat!@n", true, vict, nullptr, nullptr, TO_ROOM);
    send_to_char(g, vict, "@RHeal@Y: @C{}@n\r\n", add_commas(heal));
  }
}

ACMD(do_shell)
{
  if (!char_is_arlian(ch)) {
    send_to_char(g, ch, "You are not capable of doing that!\r\n");
    return;
  }
  if (char_get_sex(ch) == SEX_FEMALE) {
    send_to_char(g, ch, "Sorry, you can't do that.\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_SHELL)) {
    act(g, "@mYou quickly absorb the armor carapace covering your body back inside.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@M$n's@m armored carapce retreats back to its original size.@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_aff_set(ch, AFF_SHELL, false);
    return;
  }
  if (getCurST(g, ch) < getMaxST(g, ch) * 0.2) {
    send_to_char(g, ch, "You do not have enough stamina to grow your armored carapace.@n\r\n");
    return;
  }
  if (axion_dice(g, 0) > char_get_aff_abils_con(ch) + rand_number(g, 1, 10)) {
    act(g, "@mYou crouch down and begin to focus on your body's carapace cells encouraging them to multiply! However your control is lacking and you ultimately fail to grow your armor very much.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@M$n@m crouches down and seems to strain for a moment before giving up and resuming $s normal stance.@n", true, ch, nullptr, nullptr, TO_ROOM);
    return;
  }
  act(g, "@mYou crouch down and begin to focus on your body's carapace cells, encouraging them to multiply! Very quickly millions of new carapace cells have been born and your armored carapace extends over all parts of your body!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@M$n@m crouches down and after a few moments of straining $s body's carapace armor starts to grow thicker and extends to cover all parts of $s body!@n", true, ch, nullptr, nullptr, TO_ROOM);
  decCurSTPercent(g, ch, .2, 0);
  char_aff_set(ch, AFF_SHELL, true);
}

ACMD(do_liquefy)
{
  if (!char_is_majin(ch)) {
    send_to_char(g, ch, "You are not capable of liquefying yourself right now. Try finding a giant blender maybe?\r\n");
    return;
  }

  if (char_aff_flagged(ch, AFF_LIQUEFIED)) {
    act(g, "@MSuddenly large chunks of goo start to hover up slowly. These very same chunks quickly begin to fly into each other, piling on as the ball of goo grows. Suddenly @m$n@M emerges as the ball of goo takes $s shape!@n", true, ch, nullptr, nullptr, TO_ROOM);
    act(g, "@MYou begin to pull the liquid chunks of your body together. Those chunks hover upward and merge into each other until a large ball of goo is formed. Slowly your body emerges as the pieces of your body take on their old form!@n", true, ch, nullptr, nullptr, TO_CHAR);
    char_aff_set(ch, AFF_LIQUEFIED, false);
    char_set_wait(ch, PULSE_3SEC);
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, LIQUEFY_SYNTAX);
    return;
  }
  if (getCurKI(g, ch) < liquefy_cost(g, ch)) {
    send_to_char(g, ch, "You do not have enough ki to manage this level of body control!\r\n");
    return;
  }

  if (str_eq_nocase(arg, "explode")) {
    liquefy_explode(g, ch, arg2);
    return;
  }
  if (!str_eq_nocase(arg, "hide")) {
    send_to_char(g, ch, LIQUEFY_SYNTAX);
    return;
  }
  let_go(g, ch);
  if (axion_dice(g, 0) > char_get_level(ch)) {
    liquefy_fumble(g, ch);
    return;
  }
  act(g, "@MYour body starts to become loose and sag. It continues to droop down until it begins to run down like a river of goo flowing from where your body was.@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@m$n@M's body starts to become loose and sag. Much of $s body begins to pour down and scatter around as pools of goo.@n", true, ch, nullptr, nullptr, TO_ROOM);
  decCurKI(g, ch, liquefy_cost(g, ch), 0);
  char_aff_set(ch, AFF_LIQUEFIED, true);
}

ACMD(do_lifeforce)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: life (0 - 99)\n0 is off.\r\n");
    return;
  }

  int setting = parse_int(arg);
  if (setting > 99) {
    send_to_char(g, ch, "Syntax: life (1 - 99)\n{} isn't an acceptable percent.\r\n", add_commas(setting));
    return;
  }
  if (setting <= 0) {
    send_to_char(g, ch, "Your will just isn't in the fight, huh?\nYou will not use up life force to maintain your PL period.\r\n");
    char_set_lifeperc(ch, 0);
    return;
  }
  send_to_char(g, ch, "Your life force will automatically kick in at {:d}% of your optimal PL.\r\n", setting);
  char_set_lifeperc(ch, setting);
}

ACMD(do_defend)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (arg.empty()) {
    struct char_data *defending = char_get_defending(g, ch);
    if (!defending) {
      send_to_char(g, ch, "Defend who?\r\n");
      return;
    }
    act_triple(g, {"@YYou stop defending @y$N@Y.@n", "@y$n@Y stops defending you.@n", "@y$n@Y stops defending @y$N@Y.@n", true}, ch, nullptr, defending);
    char_set_defender(defending, nullptr);
    char_set_defending(ch, nullptr);
    return;
  }

  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "You can't seem to find that person.\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (vict == ch) {
    send_to_char(g, ch, "Well hopefully you are smart enough to defend yourself.\r\n");
    return;
  }
  act_triple(g, {"@YYou start defending @y$N@Y.@n", "@y$n@Y starts defending you.@n", "@y$n@Y starts defending @y$N@Y.@n", true}, ch, nullptr, vict);
  /* Each has only one, so whoever was paired with these two is let go, and told. */
  if (char_get_defending(g, ch)) {
    if (char_get_defending(g, ch) != vict)
      act(g, "@y$n@Y stops defending you.@n", false, ch, nullptr, char_get_defending(g, ch), TO_VICT);
    char_set_defender(char_get_defending(g, ch), nullptr);
  }
  if (char_get_defender(g, vict)) {
    if (char_get_defender(g, vict) != ch)
      act(g, "@YYou are no longer defending @y$n@Y. Someone else has taken over.@n", false, vict, nullptr, char_get_defender(g, vict), TO_VICT);
    char_set_defending(char_get_defender(g, vict), nullptr);
  }
  char_set_defender(vict, ch);
  char_set_defending(ch, vict);
}

ACMD(do_scry)
{
  constexpr int cost = 2000;

  if (strcasecmp(char_get_name(ch), "Galeos")) {
    send_to_char(g, ch, "You do not know how to perform that technique.\r\n");
    return;
  }
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: scry (target)\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "Who are you using Oracle Scry on?\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (vict == ch) {
    send_to_char(g, ch, "You can't do that to yourself!\r\n");
    return;
  }
  if (char_is_npc(vict)) {
    send_to_char(g, ch, "No using this on mobs!\r\n");
    return;
  }
  if (char_get_practice_points(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough PS to Oracle Scry!\r\n");
    return;
  }

  reveal_hiding(g, ch, 0);
  act_triple(g,
             {"@GYou focus your mind and begin to allow the flood of images and energy to roar through your mind. You then allow those thoughts to make their way into the mind of @c$N@G. You can hardly comprehend the vastness of the information flooding in, yet still glimpse bits and pieces of your own destiny.@n",
              "@GYou see @C$n@G begin to focus, and then without warning, your mind is flooded painfully with images, energy and information. The data streams in a mad torrent through your psyche, and just when you think snapping is possible, the voice of @C$n@G comes to you and eases and guides you. You see images of potential futures, information not yet known, knowledge yet undiscovered. Though you could not fully  grasp what is to come, you feel more prepared at facing the unknown.@n",
              "@C$n@W appears to be performing some sort of ritual or something with @c$N@W.@n", true},
             ch, nullptr, vict);
  int64_t boost = char_get_aff_abils_intel(ch) * 0.5;
  gainBasePL(vict, (getBasePL(vict) * .01) * boost, false);
  gainBaseKI(vict, (getBaseKI(vict) * .01) * boost, false);
  gainBaseST(vict, (getBaseST(vict) * .01) * boost, false);

  send_to_char(g, vict, "Your Powerlevel, Ki, and Stamina have improved drastically! On top of that your Intelligence and Wisdom have improved permanantly!\r\n");
  char_set_real_abils_intel(vict, char_get_real_abils_intel(vict) + 2);
  char_set_real_abils_wis(vict, char_get_real_abils_wis(vict) + 2);
  char_set_practice_points(g, ch, char_get_practice_points(g, ch) - cost);
  if (char_get_level(ch) >= 100) {
    gainBaseAllPercent(ch, .025, true);
    send_to_char(g, ch, "Your Powerlevel, Ki, and Stamina have improved!\r\n");
    return;
  }
  send_to_char(g, ch, "@D[@mPractice Sessions@D:@R -2000@D]@n\r\n");
  int64_t needed = level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch);
  if (needed > 0) {
    char_set_exp(ch, char_get_exp(ch) + needed);
    send_to_char(g, ch, "The remaining experience needed for your next level up has been gained!@n\r\n");
  } else {
    send_to_char(g, ch, "Due to already having enough experience to level up you gain no expereince.\r\n");
  }
}

void ash_burn(GameWorld *g, struct char_data *ch)
{
  if (!ch || char_get_in_room(ch) == NOWHERE)
    return;
  for (obj_data *obj : room_contents(g, char_get_in_room(ch))) {
    if (obj_get_vnum(g, obj) != 1306 || axion_dice(g, 0) <= char_get_aff_abils_con(ch))
      continue;
    if (!char_is_android(ch) && !char_is_demon(ch) && !char_is_icer(ch)) {
      reveal_hiding(g, ch, 0);
      decCurST(g, ch, ((getMaxST(g, ch) * 0.005) + 20) * obj_get_cost(obj), 0);
      act(g, "@RYou choke on the the burning hot @Da@Ws@wh@Dc@Wl@wo@Du@Wd@R!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@R chokes on the burning hot @Da@Ws@wh@Dc@Wl@wo@Du@Wd@R!@n", true, ch, nullptr, nullptr, TO_ROOM);
    }
    if (!char_is_android(ch) && !char_is_demon(ch) && !char_is_npc(ch) && !char_plr_flagged(ch, PLR_EYEC) && !char_aff_flagged(ch, AFF_BLIND)) {
      reveal_hiding(g, ch, 0);
      act(g, "@DYour eyes sting from the hot ash! You can't see!@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@r$n@D eyes appear to have been hurt by the ash!@n", true, ch, nullptr, nullptr, TO_ROOM);
      assign_affect(g, ch, AFF_BLIND, SKILL_SOLARF, 1, 0, 0, 0, 0, 0, 0);
    }
  }
}

ACMD(do_ashcloud)
{
  if (!char_is_demon(ch)) {
    send_to_char(g, ch, "You are not trained in the use of ash and fire!\r\n");
    return;
  }
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: ashcloud (1 | 2 | 3)\r\n");
    return;
  }

  struct obj_data *ash = nullptr; /* the last carried */
  for (obj_data *obj : char_inventory(g, ch))
    if (obj_get_vnum(g, obj) == 1305)
      ash = obj;
  if (std::ranges::any_of(room_contents(g, char_get_in_room(ch)), [&](obj_data *obj) { return obj_get_vnum(g, obj) == 1306; })) {
    send_to_char(g, ch, "You can not pile more ash into the air without causing it to clump together and settle.\r\n");
    return;
  }
  if (!ash) {
    send_to_char(g, ch, "You do not have any ash!\r\n");
    return;
  }
  int level = parse_int(arg);
  if (level < 1 || level > 3) {
    send_to_char(g, ch, "Syntax: ashcloud (1 | 2 | 3)\r\n");
    return;
  }
  const ash_size &size = ASH_SIZES[level - 1];

  int64_t cost = (getMaxKI(g, ch) * size.ki_share) + (char_get_aff_abils_intel(ch) * size.intel_mult);
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki!\r\n");
    return;
  }
  if (room_sunken(g, char_get_in_room(ch))) {
    send_to_char(g, ch, "You can not create an ashcloud here, because it is too wet.\r\n");
    return;
  }
  if (room_sect(g, char_get_in_room(ch)) == SECT_SPACE) {
    send_to_char(g, ch, "You can not create an ashcloud in space.\r\n");
    return;
  }
  if (char_get_aff_abils_intel(ch) < axion_dice(g, -10)) {
    reveal_hiding(g, ch, 0);
    act(g, "@RYou take a handful of ashes, and when you go to blow flames at it you lose focus. The ashes are blown from your hands by your huge gust of breath.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@r$n@R takes a handful of ashes from $s belongings and blows it out of $s hands with a strong gust of air. @YStrange.@n", true, ch, nullptr, nullptr, TO_ROOM);
    extract_obj(g, ash);
    decCurKI(g, ch, cost, 0);
    return;
  }
  reveal_hiding(g, ch, 0);
  decCurKI(g, ch, cost, 0);
  act(g, "@RYou take a handful of ashes and you create a fierce heat within your lungs. With the heat ready you breathe ki infused flames at the pile of ashes! The flames and ashes mix and fill the surrounding area with a hot burning ash!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@r$n@R takes a handful of ashes and $e breathes ki infused flames at the pile of ashes! The flames and ashes mix and fill the surrounding area with a hot burning ash!@n", true, ch, nullptr, nullptr, TO_ROOM);
  if (size.aftershock)
    send_to_room(g, char_get_in_room(ch), size.aftershock);
  struct obj_data *cloud = read_object(g, 1306);
  obj_to_room(g, cloud, char_get_in_room(ch));
  extract_obj(g, ash);
  obj_set_timer(cloud, size.timer);
  obj_set_cost(cloud, size.thickness);
  ash_burn(g, ch);
}

ACMD(do_healglow)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!char_get_skill(ch, SKILL_HEALGLOW)) {
    send_to_char(g, ch, "You do not know how to perform that technique.\r\n");
    return;
  }

  struct char_data *vict = ch;
  if (!arg.empty()) {
    if (char_get_skill(ch, SKILL_HEALGLOW) < 100) {
      send_to_char(g, ch, "You can not target anyone except yourself unless you are a master of this technique.\nSyntax: healingglow\r\n");
      return;
    }
    auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "Nobody around by that name.\r\n");
    if (reject(g, ch, found))
      return;
    vict = *found;
  }

  if (char_aff_flagged(vict, AFF_HEALGLOW)) {
    send_to_char(g, ch, vict == ch ? "You already have a healing glow surrounding your body.\r\n" : "They already have a healing glow surrounding their body.\r\n");
    return;
  }
  if (char_get_fighting(g, vict)) {
    send_to_char(g, ch, vict == ch ? "You are too busy fighting!@n\r\n" : "They are too busy fighting!@n\r\n");
    return;
  }
  int64_t cost = getMaxKI(g, ch) * 0.5;
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki. It requires at least 50% of your ki in cost.\r\n");
    return;
  }

  int duration = char_get_skill(ch, SKILL_HEALGLOW) * 0.1;
  if (vict == ch) {
    act(g, "@CPlacing your hands on your body you begin to focus your energies. Slowly a strong blue glow glistens and shines across your skin!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@C places $s hands on $s body. Slowly a strong blue glow glistens and shines across $s skin!@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_aff_set(vict, AFF_HEALGLOW, true);
  } else {
    act_triple(g,
               {"@CPlacing your hands on @c$N's@C body you begin to focus your energies. Slowly a strong blue glow glistens and shines across $S skin!@n",
                "@c$n@C places $s hands on YOUR body. Slowly a strong blue glow glistens and shines across your skin!@n",
                "@c$n@C places $s hands on @c$N's@C body. Slowly a strong blue glow glistens and shines across $S skin!@n", true},
               ch, nullptr, vict);
    duration += rand_number(g, -2, 1);
  }
  if (duration <= 0)
    duration = 1;
  assign_affect(g, vict, AFF_HEALGLOW, SKILL_HEALGLOW, duration, 0, 0, 0, 0, 0, 0);
  decCurKI(g, ch, cost, 0);
}

/* Demon's lame skill */
ACMD(do_metamorph)
{
  if (char_is_npc(ch))
    return;
  if (!know_skill(g, ch, SKILL_METAMORPH))
    return;
  if (char_get_alignment(ch) >= 51) {
    send_to_char(g, ch, "Your heart is too pure to use that technique!\r\n");
    return;
  }

  int64_t cost = getMaxKI(g, ch) * 0.16;
  if (char_aff_flagged(ch, AFF_METAMORPH)) {
    send_to_char(g, ch, "You are already surrounded by a dark aura!\r\n");
    return;
  }
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki. You need {}.\r\n", add_commas(cost));
    return;
  }

  int chance = axion_dice(g, 0), perc = char_get_aff_abils_wis(ch) * 2;
  if (perc < 100 && perc > 60)
    perc = 100;
  else if (perc < 100)
    perc += 10;

  decCurKI(g, ch, cost / 2, 0);
  if (perc < chance) {
    act(g, "@WYou focus your energies and prepare your @RDark Metamorphisis@W but screw up your focus!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@WA dark @Rred@W glow starts to surround @C$n@W, but it fades quickly.@n", true, ch, nullptr, nullptr, TO_ROOM);
    return;
  }
  act(g, "'@RDark@W...' An explosion of sanguine aura erupts over the surface of your body, your eyes darkening to a bleeding crimson. The flaring glow emanating from your body pronounces the shadows cast, a darkening umbrage that threatens a malicious promise. Fists clench tightly, muscles bulking as you hiss; You complete the transition, relaxing visibly, '...@RMetamorphosis@W'@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "'@RDark@W...' An explosion of sanguine aura erupts over the surface of @C$n@W's body, $s eyes darkening to a bleeding crimson. The flaring glow emanating from $s body pronounces the shadows cast, a darkening umbrage that threatens a malicious promise. Fists clench tightly, muscles bulking as $e hisses; $e completes the transition, relaxing visibly, '...@RMetamorphosis@W'@n", true, ch, nullptr, nullptr, TO_ROOM);
  assign_affect(g, ch, AFF_METAMORPH, SKILL_METAMORPH, char_get_aff_abils_intel(ch) / 12, 0, 0, 0, 0, 0, 0);
  incCurHealthPercent(g, ch, .6, true);
}

ACMD(do_shimmer)
{
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);

  if (!char_is_npc(ch) && char_prf_flagged(g, ch, PRF_ARENAWATCH)) {
    char_prf_set(g, ch, PRF_ARENAWATCH, false);
    char_set_arenawatch(ch, -1);
    send_to_char(g, ch, "You stop watching the arena action.\r\n");
  }
  if (strcasecmp(char_get_name(ch), "Anubis")) {
    send_to_char(g, ch, "You do not even know how to perform that skill!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_PILOTING)) {
    send_to_char(g, ch, "You are busy piloting a ship!\r\n");
    return;
  }
  if (char_plr_flagged(ch, PLR_HEALT)) {
    send_to_char(g, ch, "You are inside a healing tank!\r\n");
    return;
  }
  if (char_get_in_room(ch) >= 19800 && char_get_in_room(ch) <= 19899) {
    send_to_char(g, ch, "@rYou are in a pocket dimension!@n\r\n");
    return;
  }
  if (arg.empty()) {
    shimmer_usage(g, ch);
    return;
  }

  int64_t cost = getMaxKI(g, ch) / 40;
  if (getCurKI(g, ch) - cost < 0) {
    send_to_char(g, ch, "You do not have enough ki to instantaneously move.\r\n");
    return;
  }

  int perc = axion_dice(g, 0), skill = 100;
  room_vnum location    = 0;
  struct char_data *tar = nullptr;
  auto place = std::ranges::find_if(SHIMMER_PLACES, [&](const shimmer_place &p) { return str_eq_nocase(arg, p.name); });
  if (place != std::end(SHIMMER_PLACES)) {
    location = place->room;
  } else if (!(tar = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_WORLD))) {
    send_to_char(g, ch, "@RThat target doesn't exist.@n\r\n");
    shimmer_usage(g, ch);
    return;
  }

  if (skill < perc || (char_get_fighting(g, ch) && rand_number(g, 1, 2) <= 1)) {
    if (tar == ch) {
      send_to_char(g, ch, SHIMMER_SELF);
      return;
    }
    send_to_char(g, ch, "You prepare to move instantly but mess up the process and waste some of your ki!\r\n");
    decCurKI(g, ch, cost, 0);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  reveal_hiding(g, ch, 0);
  char_set_wait(ch, PULSE_2SEC);
  if (!tar) {
    decCurKI(g, ch, cost, 0);
    act(g, "@wYour body begins to fade away almost appearing ghost like, before a ripple passes through your image and your are gone in an instant!@n", true, ch, nullptr, tar, TO_CHAR);
    act(g, "@w$n@w body begins to fade away almost appearing ghost like, before a ripple passes through $s image and $e is gone in an instant!@n", true, ch, nullptr, tar, TO_NOTVICT);
    handle_teleport(g, ch, nullptr, location);
    return;
  }
  if (tar == ch) {
    send_to_char(g, ch, SHIMMER_SELF);
    return;
  }
  if (char_get_grappling(g, ch) && char_get_grappling(g, ch) == tar) {
    send_to_char(g, ch, "You are already in the same room with them and are grappling with them!\r\n");
    return;
  }
  if (char_get_admlevel(tar) > 0 && char_get_admlevel(ch) < 1) {
    send_to_char(g, ch, "That immortal prevents you from reaching them.\r\n");
    return;
  }
  if (room_flagged(g, char_get_in_room(tar), ROOM_NOINSTANT)) {
    send_to_char(g, ch, "You can not go there as it is a protected area!\r\n");
    return;
  }
  if ((char_get_grappling(g, ch) && char_aff_flagged(char_get_grappling(g, ch), AFF_SPIRIT)) ||
      (char_get_drag(g, ch) && char_aff_flagged(char_get_drag(g, ch), AFF_SPIRIT)) ||
      (char_get_grappled(g, ch) && char_aff_flagged(char_get_grappled(g, ch), AFF_SPIRIT))) {
    send_to_char(g, ch, "You can not take the dead with you!\r\n");
    return;
  }

  decCurKI(g, ch, cost, 0);
  act_triple(g,
             {"@wYour body begins to fade away almost appearing ghost like, before a ripple passes through your image and your are gone in an instant!@n",
              "@w$n@w appears in an instant out of nowhere right next to you!@n",
              "@w$n@w body begins to fade away almost appearing ghost like, before a ripple passes through $s image and $e is gone in an instant!@n", true},
             ch, nullptr, tar);
  char_plr_set(ch, PLR_TRANSMISSION, true);
  handle_teleport(g, ch, tar, 0);
}

ACMD(do_channel)
{
  if (!char_is_demon(ch) || char_get_skills_base(ch, SKILL_STYLE) < 40) {
    send_to_char(g, ch, "You are not a Demon!\r\n");
    return;
  }

  int64_t cost = getMaxKI(g, ch) * 0.15;
  int chance = axion_dice(g, 0), skill = char_get_skill(ch, SKILL_STYLE);
  if (cost > getCurKI(g, ch)) {
    send_to_char(g, ch, "You do not have enough ki to channel with!\r\n");
    return;
  }

  auto rubies = char_inventory(g, ch);
  auto ruby = std::ranges::find_if(rubies, [&](obj_data *obj) { return obj_get_vnum(g, obj) == 6600 && !obj_extra_flagged(obj, ITEM_HOT); });
  if (ruby == rubies.end()) {
    send_to_char(g, ch, "You do not have any uncharged blood rubies.\r\n");
    return;
  }
  if (room_get_geffect(g, char_get_in_room(ch)) <= 0) {
    send_to_char(g, ch, "There is no lava here!\r\n");
    return;
  }

  if (skill < chance) {
    act(g, "@RAs you move your ki through the lava you begin to draw heat away from it into the ruby. You screw up the rate of heating though and cause the ruby to crumble to dust!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@RAs $n@R moves $s ki through the lava $e begins to draw heat away from it into a blood ruby. However $e screws up the rate of heating and causes the ruby to crumble to dust!@n", true, ch, nullptr, nullptr, TO_ROOM);
    extract_obj(g, *ruby);
  } else {
    act(g, "@RAs you move your ki through the lava you begin to draw heat away from it into the ruby. You do so at an even rate and end up with a glowing red hot blood ruby!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@RAs $n@R moves $s ki through the lava $e begins to draw heat away from it into a blood ruby. The ruby glows red hot as $e finishes the process of channeling the heat!@n", true, ch, nullptr, nullptr, TO_ROOM);
    room_set_geffect(g, char_get_in_room(ch), 0);
    obj_extra_set(*ruby, ITEM_HOT, true);
  }
  decCurKI(g, ch, cost, 0);
  char_set_wait(ch, PULSE_1SEC);
}

ACMD(do_hydromancy)
{
  if (!char_is_tsuna(ch) || char_get_skills_base(ch, SKILL_STYLE) <= 0) {
    send_to_char(g, ch, "You know nothing about hydromancy!\r\n");
    return;
  }

  int skill = char_get_skills_base(ch, SKILL_STYLE), chance = axion_dice(g, 0);
  int64_t cost = (getMaxKI(g, ch) / 12) - (char_get_aff_abils_intel(ch) * char_get_level(ch));

  room_vnum here = char_get_in_room(ch);
  if (room_get_geffect(g, here) >= 0 && room_sect(g, here) != SECT_WATER_SWIM && room_sect(g, here) != SECT_WATER_NOSWIM) {
    if (room_sect(g, here) != SECT_UNDERWATER)
      send_to_char(g, ch, "There is not sufficient water here.\r\n");
    else
      send_to_char(g, ch, "There is too much water here to control!\r\n");
    return;
  }
  if (cost <= 0)
    cost = 100;
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki to manipulate any water around you.\r\n");
    return;
  }
  if (char_get_con_cooldown(ch) > 0) {
    send_to_char(g, ch, "You must wait a short period before concentrating again.\r\n");
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);
  if (arg.empty())
    hydromancy_syntax(g, ch, "Syntax 1: hydromancy flood (direction)\r\n");
  else if (str_eq_nocase(arg, "spike"))
    hydromancy_spike(g, ch, skill, chance);
  else if (str_eq_nocase(arg, "flood"))
    hydromancy_flood(g, ch, arg2, skill, chance, cost);
  else
    hydromancy_syntax(g, ch, "Syntax 1: hydromancy (flood) (direction)\r\n");
}

ACMD(do_kanso)
{
  if (char_is_npc(ch))
    return;
  if (strcasecmp(char_get_name(ch), "Levanthoth")) { /* NOT the right player */
    send_to_char(g, ch, "You do not know how to perform that technique. \r\n");
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  auto found            = find_char(g, ch, arg, FIND_CHAR_ROOM, "Perform Kanso Suru on who?\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (char_is_android(vict)) {
    send_to_char(g, ch, "Mechanical beings are not effected by this technique.\r\n");
    return;
  }
  if (!can_kill(g, ch, vict, nullptr, 0))
    return;

  int64_t cost = getMaxKI(g, ch) / char_get_aff_abils_intel(ch);
  int dice = axion_dice(g, -5), skill = char_get_aff_abils_intel(ch), pdice = axion_dice(g, 0);
  int dam = rand_number(g, 1, 4);
  if (char_get_aff_abils_wis(ch) > axion_dice(g, -5)) /* Bonus based on wisdom */
    dam += 1;

  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki.\r\n");
    return;
  }
  if (skill < dice) {
    act_triple(g,
               {"You close your eyes and focus, before bounding effortlessly toward $N. Closing the distance, you place your hands on $N's chest but nothing happens!\r\n",
                "$n closes $s eyes and bounds effortlessly towards you. Closing the distance, $e places $s hands on your chest but nothing happens!\r\n",
                "$n closes $s eyes and bounds toward $N. Smirking, $e puts $m hands on $N's chest but nothing seems to happen.\r\n", true},
               ch, nullptr, vict);
    decCurKI(g, ch, cost, 0);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  act_triple(g,
             {"You close your eyes and focus, before effortlessly bounding towards $N. Closing the distance, you smirk at $N and place your hands on their chest. Electricity flows into their body as you draw water out of their very cells!\r\n",
              "$n closes $s eyes and focuses, before effortlessly bounding toward you. Closing the distance, $e smirks and places both of $s hands on your chest. Electricity begins to pulse through your body and a great thirst takes hold, as if $n is drawing the water from your body!\r\n",
              "$n closes $s eyes, before effortlessly bounding toward $N. Closing the distance, $n smirks and places both $s hands on $N's chest. Electricity seems to pass from $n's body to $N's!\r\n", true},
             ch, nullptr, vict);
  decCurKI(g, ch, cost, 0);

  /* The thirst moves from vict to ch */
  char_set_conditions(g, vict, THIRST, std::max(char_get_conditions(g, vict, THIRST) - dam, 0));
  char_set_conditions(g, ch, THIRST, std::min(char_get_conditions(g, ch, THIRST) + dam, 48));
  incCurHealth(g, ch, (getEffMaxPL(g, ch) * .01) * dam, true);
  char_set_wait(ch, PULSE_2SEC);

  if (!is_affected(vict, AFF_HYDROZAP)) { /* Drop their AGL/CON if not already lowered */
    send_to_char(g, vict, "@RYou feel less agile and your muscles ache!@n\r\n");
    assign_affect(g, vict, AFF_HYDROZAP, 0, -1, 0, -4, -4, 0, 0, 0);
    save_char(g, vict);
  }
  if (skill > pdice && !char_aff_flagged(vict, AFF_PARA)) { /* Paralyze them too */
    act_triple(g, {"@R$N@W is paralyzed by the attack!@n", "@RYou are paralyzed by the attack!@n", "@R$N@Wis paralyzed by the attack!@n", true}, ch, nullptr, vict);
    struct affected_type af = {};
    af.type                 = SKILL_PARALYZE;
    af.duration             = rand_number(g, 1, 3);
    af.modifier             = 0;
    af.location             = APPLY_NONE;
    af.bitvector            = AFF_PARA;
    affect_join(g, vict, &af, false, false, false, false);
  }
}

ACMD(do_hayasa)
{
  if (!char_is_npc(ch) && !char_get_skill(ch, SKILL_HAYASA)) {
    send_to_char(g, ch, "You do not know how to perform this technique!\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_HAYASA)) {
    send_to_char(g, ch, "You are already focusing ki to continually speed up your movements.\r\n");
    return;
  }

  int skill = char_get_skill(ch, SKILL_HAYASA), prob = axion_dice(g, 0);
  int64_t cost = getMaxKI(g, ch) / MAX(skill / 2, 1);
  int duration = skill >= 100 ? 6 : skill >= 80 ? 5 : skill >= 50 ? 4 : skill >= 25 ? 3 : 2;

  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki.\r\n");
    return;
  }
  decCurKI(g, ch, cost, 0);
  if (skill < prob) {
    act(g, "@CYou close your eyes for a brief moment and focus your ki around your body as a soft blue glow. The glow disappears though as you fail to maintain the effect...@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@C closes $s eyes for a brief moment and a soft blue glow begins to form around $s body. The glow disappears a second later though and $e frowns.@n", true, ch, nullptr, nullptr, TO_ROOM);
  } else {
    struct affected_type af = {};
    af.type                 = SPELL_HAYASA;
    af.duration             = duration;
    af.modifier             = 0;
    af.location             = APPLY_NONE;
    af.bitvector            = AFF_HAYASA;
    affect_join(g, ch, &af, false, false, false, false);
    char_set_speedboost(ch, char_get_speedcalc(g, ch) * 0.5);
    reveal_hiding(g, ch, 0);
    act(g, "@CYou close your eyes for a brief moment and focus your ki around your body as a soft blue glow. All your movements are faster now!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@C closes $s eyes for a brief moment and a soft blue glow begins to form around $s body. The glow pulsates gently as $e opens his eyes and smiles.@n", true, ch, nullptr, nullptr, TO_ROOM);
  }
  improve_skill(g, ch, SKILL_HAYASA, 1);
  char_set_wait(ch, PULSE_2SEC);
}

ACMD(do_ensnare)
{
  if (!know_skill(g, ch, SKILL_ENSNARE))
    return;

  auto carried = char_inventory(g, ch);
  auto silk = std::ranges::find_if(carried, [&](obj_data *obj) { return valid_silk(g, obj) && !obj_extra_flagged(obj, ITEM_FORGED); });
  if (silk == carried.end()) {
    send_to_char(g, ch, "You do not have a bundle of silk to ensnare an opponent with!\r\n");
    return;
  }
  struct obj_data *obj = *silk;

  int prob = char_get_skill(ch, SKILL_ENSNARE), perc = axion_dice(g, 0);
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "Syntax: ensnare (target)\r\n");
    return;
  }
  auto found = find_char(g, ch, arg, FIND_CHAR_ROOM, "Who are you trying to target with ensnare?\r\n");
  if (reject(g, ch, found))
    return;
  struct char_data *vict = *found;
  if (char_aff_flagged(vict, AFF_ENSNARED)) {
    send_to_char(g, ch, "They are already ensnared!\r\n");
    return;
  }
  if (!char_has_arms(g, vict)) {
    send_to_char(g, ch, "They don't have arms to ensnare!\r\n");
    return;
  }

  bool vict_zanzoken = char_aff_flagged(vict, AFF_ZANZOKEN), ch_zanzoken = char_aff_flagged(ch, AFF_ZANZOKEN);
  const snare_outcome *outcome;
  if (prob <= perc)
    outcome = &SNARE_FUMBLED;
  else if (vict_zanzoken && !ch_zanzoken)
    outcome = &SNARE_ZANZOKENED;
  else if (vict_zanzoken)
    outcome = outpaced(g, ch, vict) ? &SNARE_BOTH_ZANZOKEN_MISS : &SNARE_BOTH_ZANZOKEN_HIT;
  else if (ch_zanzoken)
    outcome = &SNARE_ZANZOKEN_HIT;
  else
    outcome = outpaced(g, ch, vict) ? &SNARE_DODGED : &SNARE_HIT;

  act_triple(g, outcome->msgs, ch, nullptr, vict);
  extract_obj(g, obj);
  if (outcome->ensnares)
    char_aff_set(vict, AFF_ENSNARED, true);
  char_set_wait(ch, PULSE_3SEC);
  improve_skill(g, ch, SKILL_ENSNARE, 0);
  if (outcome->vict_zanzoken_spent)
    char_aff_set(vict, AFF_ZANZOKEN, false);
  if (outcome->ch_zanzoken_spent)
    char_aff_set(ch, AFF_ZANZOKEN, false);
}

/* Let's an Arlian trade Stamina for either PL or Ki*/
ACMD(do_adrenaline)
{
  bool bio_arlian = char_is_bio(ch) && (char_get_genome(ch, 0) == 6 || char_get_genome(ch, 1) == 6);
  if (!char_is_arlian(ch) && !bio_arlian) {
    send_to_char(g, ch, "You are not an arlian and do not possess this ability\r\n");
    return;
  }

  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);
  if (arg.empty() || arg2.empty()) {
    send_to_char(g, ch, "Syntax: adrenaline (pl or ki) (percent)\r\nExample: adrenaline pl 10\r\n");
    return;
  }
  if (parse_int(arg2) < 0 || parse_int(arg2) > 100) {
    send_to_char(g, ch, "The percent must be between 1 and 100%.\r\n");
    return;
  }

  double percent = parse_int(arg2) * 0.01;
  if (getCurST(g, ch) - (getBasePL(ch) * percent) < 0) { /* quirk: measured against base PL, though stamina is traded */
    send_to_char(g, ch, "You do not have enough stamina to trade for adrenaline!\r\n");
    return;
  }

  int64_t trade = getBaseST(ch) * percent;
  if (str_eq_nocase(arg, "pl")) {
    act(g, "@GYou focus your mind and begin to overwork your powerful adrenal glands and your wounds begin to heal!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@g$n@G seems to concentrate and $s wounds begin to heal!@n", true, ch, nullptr, nullptr, TO_ROOM);
    if (getCurPL(g, ch) + trade > getEffMaxPL(g, ch))
      send_to_char(g, ch, "Some of your stamina was wasted because your powerlevel maxed out.\r\n");
    incCurHealth(g, ch, trade, true);
    decCurST(g, ch, trade, 0);
  } else if (str_eq_nocase(arg, "ki")) {
    act(g, "@GYou focus your mind and begin to overwork your powerful adrenal glands and you feel your ki replenish!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@g$n@G seems to concentrate and $e appears energized!@n", true, ch, nullptr, nullptr, TO_ROOM);
    if (getCurKI(g, ch) + trade > getMaxKI(g, ch))
      send_to_char(g, ch, "Some of your stamina was wasted because your ki maxed out.\r\n");
    incCurKI(g, ch, trade, true);
    decCurST(g, ch, trade, 0);
  } /* quirk: anything else says nothing */
}

/* This allows a player to cover their body in a shield of fire. */
ACMD(do_fireshield)
{
  if (!know_skill(g, ch, SKILL_FIRESHIELD))
    return;
  if (char_aff_flagged(ch, AFF_FIRESHIELD)) {
    send_to_char(g, ch, "You are already covered in a fireshield!\r\n");
    return;
  }
  if (char_aff_flagged(ch, AFF_SANCTUARY)) {
    send_to_char(g, ch, "You are covered in a barrier!\r\n");
    return;
  }
  if (room_sunken(g, char_get_in_room(ch))) {
    send_to_char(g, ch, "There is way too much water here!\r\n");
    return;
  }
  int64_t cost = getMaxKI(g, ch) * 0.03;
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki!\r\n");
    return;
  }

  int skill = init_skill(g, ch, SKILL_FIRESHIELD), prob = axion_dice(g, 0);
  if (skill <= prob) {
    act(g, "@WYou hold your hands up in front of you on either side and try to summon defensive @rf@Rl@Ya@rm@Re@Ys@W to cover your body. Yet you screw up and the technique fails!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@W holds $s hands up in front of $m on either side and tries to summon defensive @rf@Rl@Ya@rm@Re@Ys@W to cover $s body. Yet $e seems to screw up and the technique fails!@n", true, ch, nullptr, nullptr, TO_ROOM);
    improve_skill(g, ch, SKILL_FIRESHIELD, 0);
    decCurKI(g, ch, cost, 0);
    return;
  }
  act(g, "@WYou hold your hands up in front of you on either side and try to summon defensive @rf@Rl@Ya@rm@Re@ys@W to cover your body. The ki you have gathered pours out of your body and creates intense black @rf@Rl@Ya@rm@Re@Ys@W that cover your entire body in a protective layer!", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@c$n@W holds $s hands up in front of $m on either side and tries to summon defensive @rf@Rl@Ya@rm@Re@ys@W to cover $s body. The ki $e has gathered pours out of $s body and creates intense black @rf@Rl@Ya@rm@Re@Ys@W that cover $s entire body in a protective layer!", true, ch, nullptr, nullptr, TO_ROOM);
  improve_skill(g, ch, SKILL_FIRESHIELD, 0);
  decCurKI(g, ch, cost, 0);
  char_aff_set(ch, AFF_FIRESHIELD, true);
}

/* This allows a player to warp from one ocean/sea to another. */
ACMD(do_warppool)
{
  if (char_is_npc(ch))
    return;
  if (!know_skill(g, ch, SKILL_WARP))
    return;
  if (char_get_grappling(g, ch) || char_get_grappled(g, ch)) {
    send_to_char(g, ch, "You are grappling with someone!\r\n");
    return;
  }
  if (char_get_absorbing(g, ch) || char_get_absorbby(g, ch)) {
    send_to_char(g, ch, "You are struggling with someone!\r\n");
    return;
  }
  if (char_get_sits(g, ch)) {
    send_to_char(g, ch, "You should get up first.\r\n");
    return;
  }

  int perc     = char_get_skill(ch, SKILL_WARP);
  int64_t cost = getMaxKI(g, ch) / 20;
  int prob     = axion_dice(g, 0);
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "What planet are you wanting to warp to?\n[ earth | frigid | kanassa | namek | aether ]\r\n");
    return;
  }
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki to perform the technique.\r\n");
    return;
  }
  if (!at_sea(g, char_get_in_room(ch))) {
    send_to_char(g, ch, "You must be on or in a sea or ocean for warp pool to work.\r\n");
    return;
  }

  auto sea = std::ranges::find_if(WARP_SEAS, [&](const warp_sea &s) { return str_eq_nocase(s.planet, arg); });
  if (sea == std::end(WARP_SEAS)) {
    send_to_char(g, ch, "That is not an acceptable choice. It must be a planet with a large body of water.\n[ earth | frigid | kanassa | namek | aether ]\r\n");
    return;
  }
  if (room_flagged(g, char_get_in_room(ch), sea->flag)) {
    send_to_char(g, ch, sea->already);
    return;
  }

  if (prob > perc) {
    act(g, "@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. You lose your concentration and the ritual fails!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly a puzzled look comes across @c$n's @Cface and the water returns to normal.@n", true, ch, nullptr, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    improve_skill(g, ch, SKILL_WARP, 1);
    return;
  }
  act(g, "@CYou reach your hand out and begin to swirl nearby water with it. At the same time you release ki into the water and focus your mind on sensing out the distant body of water you wish to travel to. As you complete the ritual you connect the water you disturbed with the water you envisioned and warp between the two points!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@c$n@C reaches $s hand out and begins to swirl nearby water with it. The water that is being swirled begins to glow @wbright@B blue@C and has a distinct separation from the rest of the waters. Suddenly @c$n@C vanishes into this water! A moment later the waters return to normal.@n", true, ch, nullptr, nullptr, TO_ROOM);
  improve_skill(g, ch, SKILL_WARP, 1);
  char_from_room(g, ch);
  char_to_room(g, ch, sea->room);
  act(g, "@CSuddenly a large whirlpool of flashing water begins to form nearby. After a few seconds @c$n@C pops out of the center of the pool! The water then return to normal a moment laterr...@n", true, ch, nullptr, nullptr, TO_ROOM);
  decCurKI(g, ch, cost, 0);
}

/* This allows a player to block off the exit of a room */
ACMD(do_obstruct)
{
  if (char_is_npc(ch))
    return;
  if (!know_skill(g, ch, SKILL_HYOGA_KABE))
    return;
  if (room_flagged(g, char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(g, ch, "You can not use this in such a peaceful area.\r\n");
    return;
  }
  if (room_sect(g, char_get_in_room(ch)) == SECT_SPACE || room_flagged(g, char_get_in_room(ch), ROOM_SPACE)) {
    send_to_char(g, ch, "You can not wall off the vastness of space.\r\n");
    return;
  }
  if (room_sect(g, char_get_in_room(ch)) == SECT_FLYING) {
    send_to_char(g, ch, "You can not create gravity defying glacial walls.\r\n");
    return;
  }

  int skill    = char_get_skill(ch, SKILL_HYOGA_KABE);
  int prob     = axion_dice(g, 0);
  int64_t cost = (getMaxKI(g, ch) / MAX(skill, 1)) * 2.5;
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "What direction are you wanting to block off?\n[ N | E | S | W | NE | NW | SE | SW | U | D | I | O ]\r\n");
    return;
  }
  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, "You do not have enough ki to perform the technique.\r\n");
    return;
  }
  auto side = std::ranges::find_if(WALL_SIDES, [&](const wall_side &s) { return str_eq_nocase(s.name, arg); });
  if (side == std::end(WALL_SIDES)) {
    send_to_char(g, ch, "That is not an acceptable direction.\n[ N | E | S | W | NE | NW | SE | SW | U | D | I | O ]\r\n");
    return;
  }
  int dir = side->dir, dir2 = side->opposite;

  if (!char_get_exit(g, ch, dir)) {
    send_to_char(g, ch, "That direction does not exist here.\r\n");
    return;
  }
  if (skill < prob) {
    act(g, "@CYou channel your ki and start to create a wall of water, but lose your concentration and the water promptly disappears.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@C channels $s ki and starts to create a wall of water, but loses $s concentration and the water promptly disappears.@n", true, ch, nullptr, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    improve_skill(g, ch, SKILL_HYOGA_KABE, 0);
    return;
  }
  room_vnum newroom = room_get_dir_option(g, char_get_in_room(ch), dir)->to_room;
  if (room_flagged(g, newroom, ROOM_PEACEFUL)) {
    send_to_char(g, ch, "You can not block off a peaceful area.\r\n");
    return;
  }

  /* A wall already there, facing this way, is melted instead */
  auto beyond = room_contents(g, newroom);
  auto wall   = std::ranges::find_if(beyond, [&](obj_data *obj) { return obj_get_vnum(g, obj) == 79 && obj_get_cost(obj) == dir2; });
  if (wall != beyond.end()) {
    act(g, "@CYou place your hands on the glacial wall and concentrate. You unfreeze the wall and evaporate the water effortlessly.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@C places $s hands on the glacial wall and concentrates. Suddenly the wall melts and then evaporates!@n", true, ch, nullptr, nullptr, TO_ROOM);
    decCurKI(g, ch, cost / 2, 0);
    extract_obj(g, *wall);
    return;
  }

  struct obj_data *far_side = read_object(g, 79);
  obj_to_room(g, far_side, newroom);
  struct obj_data *near_side = read_object(g, 79);
  obj_to_room(g, near_side, char_get_in_room(ch));

  int64_t strength = (((char_get_aff_abils_intel(ch) * skill) * char_get_aff_abils_wis(ch)) * 20) + (getMaxKI(g, ch) * 0.001);
  for (int times : {20, 15, 10, 5, 2}) {
    if (strength > getEffMaxPL(g, ch) * times) {
      strength = getEffMaxPL(g, ch) + (strength / times);
      break;
    }
  }

  obj_set_cost(far_side, dir2);
  obj_set_weight(far_side, strength);
  obj_set_cost(near_side, dir);
  obj_set_weight(near_side, strength);
  obj_set_fellow_wall(far_side, near_side);
  obj_set_fellow_wall(near_side, far_side);
  act(g, "@CYou concentrate and channel your ki. A wall of water starts to form in such a way to block off the direction of your choice. As the wall becomes complete it freezes solid by your will!@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@c$n@C concentrates and channels $s ki. A wall of water starts to form in such a way to block off one of the directions of this area. As the wall becomes complete it freezes solid by @c$n's@C will!@n", true, ch, nullptr, nullptr, TO_ROOM);
  send_to_room(g, newroom, "@cA wall of water forms slowly upward blocking off the {} direction. This wall of water then freezes instantly once it stops growing.@n\r\n", dirs[dir2]);
  improve_skill(g, ch, SKILL_HYOGA_KABE, 0);
  decCurKI(g, ch, cost, 0);
}

/* This allows a player to flood a room. */
ACMD(do_dimizu)
{
  if (char_is_npc(ch))
    return;
  if (!know_skill(g, ch, SKILL_DIMIZU))
    return;

  int skill = char_get_skill(ch, SKILL_DIMIZU);
  int prob  = axion_dice(g, 0);
  room_vnum here = char_get_in_room(ch);

  if (room_get_geffect(g, here) < 0) {
    act(g, "@CYou concentrate and distabilie the water, separating the hydrogen and oxygen. The gases dissipate quickly.", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@C concentrates and the water filling the area seems to shudder. Suddenly the water begins to evaporate as the hydrogen and oxygen are separated.", true, ch, nullptr, nullptr, TO_ROOM);
    room_set_geffect(g, here, 0);
    char_set_wait(ch, PULSE_1SEC);
    return;
  }
  if (room_sect(g, here) == SECT_UNDERWATER) {
    send_to_char(g, ch, "The area is already underwater!\r\n");
    return;
  }
  if (room_sect(g, here) == SECT_SPACE || room_flagged(g, here, ROOM_SPACE)) {
    send_to_char(g, ch, "You can't flood space!\r\n");
    return;
  }
  if (getCurKI(g, ch) < getMaxKI(g, ch) / 12) {
    send_to_char(g, ch, "You do not have enough ki to perform the technique.\r\n");
    return;
  }
  if (skill < prob) {
    act(g, "@CYou gather your ki and concentrate on creating water from it. Water begins to flow upward around the entire area, but you lose your concentration and it all goes flooding away!@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n@C gathers $s ki and concentrates on creating water from it. Water begins to flow upward around the entire area, but $e loses $s concentration and all the water goes flooding away!@n", true, ch, nullptr, nullptr, TO_ROOM);
    decCurKI(g, ch, getMaxKI(g, ch) / 12, 0);
    improve_skill(g, ch, SKILL_DIMIZU, 0);
    return;
  }
  act(g, "@CYou gather your ki and concentrate on creating water from it. Water begins to flow upward around the entire area. You form the water into a perfect cube with barely any ripples in its walls. It will maintain this form for a while.@n", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "@c$n@C gathers $s ki and concentrates on creating water from it. Water begins to flow upward around the entire area. @c$n@C forms the water into a perfect cube with barely any ripples in its walls. It appears the water will maintain this form for a while.@n", true, ch, nullptr, nullptr, TO_ROOM);
  decCurKI(g, ch, getMaxKI(g, ch) / 12, 0);
  room_set_geffect(g, here, -3);
  improve_skill(g, ch, SKILL_DIMIZU, 0);
}

/* Allows a player to place a "beacon" on a room they want to return to if
 * they revive from death. */
ACMD(do_beacon)
{
  if (char_is_npc(ch))
    return;
  if (char_aff_flagged(ch, AFF_SPIRIT)) {
    send_to_char(g, ch, "You are dead. You can not stake out a room to return to upon revival.\r\n");
    return;
  }
  if (char_get_in_room(ch) >= 0 && char_get_in_room(ch) <= 14) {
    send_to_char(g, ch, "You can not stake out an immortal room to be revived in.\r\n");
    return;
  }
  send_to_char(g, ch, "You stake out the room you are in and will return to it if you die and are revived.\r\n");
  char_set_droom(ch, char_get_in_room(ch));
}
