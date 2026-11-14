/* ************************************************************************
*  File: misc/songs.c                                    Part of DBAT       *
*  Usage: Songs and their effects                                         *
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
#include "dbat/args.hpp"
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
#include <climits>

#include "misc.hpp"

bool grouped_with(GameWorld *g, struct char_data *ch, struct char_data *vict)
{
  if (!char_aff_flagged(ch, AFF_GROUP) || !char_aff_flagged(vict, AFF_GROUP))
    return false;
  return ch == char_get_master(g, vict) || char_get_master(g, ch) == vict || char_get_master(g, ch) == char_get_master(g, vict);
}

namespace {
  constexpr const char *OUT_OF_KI  = "You no longer have the ki necessary to play your song.\r\n";
  constexpr const char *STOPS      = "@c$n@C stops playing $s song.@n";
  constexpr const char *NO_KI      = "@wYou do not have enough ki to power the instrument for that song!@n\r\n";
  constexpr const char *UNSKILLED  = "You do not posess the skill to play such a song!\r\n";

  /* Where each Teleportation Melody takes its listeners */
  struct destination {
    int song;
    const char *planet;
    room_vnum room;
  };
  constexpr destination DESTINATIONS[] = {
    {SONG_TELEPORT_EARTH, "Earth", 300},        {SONG_TELEPORT_FRIGID, "Frigid", 4047},   {SONG_TELEPORT_VEGETA, "Vegeta", 2234},
    {SONG_TELEPORT_NAMEK, "Namek", 10182},      {SONG_TELEPORT_ARLIA, "Arlia", 16087},    {SONG_TELEPORT_KANASSA, "Kanassa", 14910},
    {SONG_TELEPORT_KONACK, "Konack", 8003},     {SONG_TELEPORT_AETHER, "Aether", 12025},
  };

  const destination *destination_of(int song)
  {
    auto found = std::ranges::find(DESTINATIONS, song, &destination::song);
    return found == std::end(DESTINATIONS) ? nullptr : &*found;
  }

  const char *song_name(int song)
  {
    switch (song) {
      case SONG_SAFETY:        return "Song of Safety";
      case SONG_SHIELDING:     return "Song of Shielding";
      case SONG_SHADOW_STITCH: return "Shadow Stitch Minuet";
      default:                 return "Teleportation Melody";
    }
  }

  /* The vnum of the last ocarina ch carries, or 0 */
  obj_vnum held_instrument(GameWorld *g, struct char_data *ch)
  {
    obj_vnum instrument = 0;
    for (obj_data *obj : char_inventory(g, ch))
      if (obj_vnum v = obj_get_vnum(g, obj); v == 8802 || v == 8807)
        instrument = v;
    return instrument;
  }

  /* The song stops when ch's ki runs out */
  bool out_of_ki(GameWorld *g, struct char_data *ch)
  {
    if (getCurKI(g, ch) > 0)
      return false;
    send_to_char(g, ch, OUT_OF_KI);
    act(g, STOPS, true, ch, nullptr, nullptr, TO_ROOM);
    char_set_powerattack(ch, 0);
    return true;
  }

  /* The Song of Safety mends each limb a little */
  struct limb_mend {
    int checked; /* the limb whose condition decides whether... */
    int mended;  /* ...this one mends */
    const char *healed;
  };
  constexpr limb_mend LIMBS[] = {
    {0, 0, "Your right arm is no longer broken!@n\r\n"},
    {1, 1, "Your left arm is no longer broken!@n\r\n"},
    {2, 2, "Your right leg is no longer broken!@n\r\n"},
    {0, 3, "Your left leg is no longer broken!@n\r\n"}, /* quirk: the left leg mends while the right arm is hurt */
  };

  void play_safety(GameWorld *g, struct char_data *ch, struct char_data *vict, int skill)
  {
    if (vict != ch)
      act(g, "@CYour skillfully playing of the Song of Safety has an effect on @c$N@C.@n", true, ch, nullptr, vict, TO_CHAR);
    else
      act(g, "@CYour skillfully playing of the Song of Safety has an effect on your own body@C.@n", true, ch, nullptr, vict, TO_CHAR);
    int64_t restore = (10 * skill) + ((getMaxKI(g, ch) * 0.0004) * skill);
    incCurHealth(g, vict, restore, true);
    incCurST(g, vict, restore * .5, true);
    for (const limb_mend &limb : LIMBS) {
      if (char_get_limb_condition(vict, limb.checked) >= 100)
        continue;
      char_set_limb_condition(vict, limb.mended, char_get_limb_condition(vict, limb.mended) + (1 + (skill * 0.1)));
      if (char_get_limb_condition(vict, limb.mended) > 100) {
        send_to_char(g, vict, limb.healed);
        char_set_limb_condition(vict, limb.mended, 100);
      }
    }
    if (vict != ch)
      act(g, "@c$n's@C soothing Song of Safety has recovered some of your powerlevel, stamina, and limb condition.", true, ch, nullptr, vict, TO_VICT);
    act(g, "@c$n@C continues playing $s ocarina!@n", true, ch, nullptr, vict, TO_NOTVICT);
    improve_skill(g, ch, SKILL_MYSTICMUSIC, 2);
    decCurKI(g, ch, (getMaxKI(g, ch) * .0003) + skill, 0);
  }

  void play_shadow_stitch(GameWorld *g, struct char_data *ch, struct char_data *vict, int skill)
  {
    act_triple(g,
               {"@CYour forboding music has caused @c$N's@C shadows to stitch into $S body, slowing $S actions!@n",
                "@c$n's@C forboding music has caused YOUR shadows to stitch into YOUR body, slow YOUR actions down!@n",
                "@c$n's@C forboding music has caused @c$N's@C shadows to stitch into $S body, slowing $S actions!@n", true},
               ch, nullptr, vict);
    decCurKI(g, ch, getPercentOfMaxKI(g, ch, .001) + skill, 0);
    if (!char_is_npc(vict))
      char_set_wait(vict, PULSE_2SEC);
    else
      assign_affect(g, vict, AFF_SHADOW_STITCH, SKILL_MYSTICMUSIC, -1, 0, 0, 0, 0, 0, -2);
  }

  void play_shielding(GameWorld *g, struct char_data *ch, struct char_data *vict, int skill)
  {
    if (vict != ch) {
      act(g, "@CYour triumphant and soaring music has powered a barrier around @c$N@C!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@c$n's@C triumphant and soaring music has powered a barrier around you!@n", true, ch, nullptr, vict, TO_VICT);
    } else {
      act(g, "@CYour triumphant and soaring music has powered a barrier around yourself@C!@n", true, ch, nullptr, vict, TO_CHAR);
    }
    act(g, "@c$n's@C triumphant and soaring music has powered a barrier around @c$N@C!@n", true, ch, nullptr, vict, TO_NOTVICT);
    char_set_barrier(vict, char_get_barrier(vict) + (((getMaxKI(g, ch) * 0.005) * (skill * 0.25)) + skill));
    if (char_get_barrier(vict) >= getMaxKI(g, vict) * 0.75)
      char_set_barrier(vict, getMaxKI(g, vict) * 0.75);
    if (!char_aff_flagged(vict, AFF_SANCTUARY))
      char_aff_set(vict, AFF_SANCTUARY, true);
    decCurKI(g, ch, getPercentOfMaxKI(g, ch, .02) + skill, 0);
  }

  void teleport_listener(GameWorld *g, struct char_data *ch, struct char_data *vict, const destination &to)
  {
    std::string to_char = str_format("@CYour Teleportation Melody has transported @c$N@C to {} in a flash!@n", to.planet);
    std::string to_vict = str_format("@c$n's@C Teleportation Melody has transported you to {} in a flash!@n", to.planet);
    act_triple(g, {to_char.c_str(), to_vict.c_str(), "@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", true}, ch,
               nullptr, vict);
    char_from_room(g, vict);
    char_to_room(g, vict, to.room);
  }

  /* One verse of ch's song, played to everyone in the room */
  void resolve_song(GameWorld *g, struct char_data *ch)
  {
    int diceroll = axion_dice(g, 0);
    int skill    = char_get_skill(ch, SKILL_MYSTICMUSIC);
    int song     = char_get_powerattack(ch);

    if (!held_instrument(g, ch)) {
      send_to_char(g, ch, "You do not have an instrument.\r\n");
      act(g, STOPS, true, ch, nullptr, nullptr, TO_ROOM);
      char_set_powerattack(ch, 0);
      return;
    }

    if (skill <= diceroll) {
      act(g, "@CYou mess up a portion of the song, but continue playing.@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@c$n@C messes up a portion of $s song, but continues to play.@n", true, ch, nullptr, nullptr, TO_ROOM);
      return;
    }
    act(g, "@CYou continue playing your song.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, str_format("@c$n@C continues playing @y'@Y{}@y'@C.@n", song_name(song)).c_str(), true, ch, nullptr, nullptr, TO_ROOM);

    const destination *teleport = destination_of(song);
    for (char_data *vict : room_people(g, char_get_in_room(ch))) {
      switch (song) {
        case SONG_SAFETY:
          if (vict == ch || grouped_with(g, ch, vict))
            play_safety(g, ch, vict, skill);
          if (out_of_ki(g, ch))
            return;
          break;
        case SONG_SHADOW_STITCH:
          /* quirk: only allies who have leaders are spared, so a leader's own music stitches them */
          if (char_get_master(g, ch) && char_get_master(g, vict) && grouped_with(g, ch, vict))
            continue;
          if (skill > diceroll + 10)
            play_shadow_stitch(g, ch, vict, skill);
          if (out_of_ki(g, ch))
            return;
          break;
        case SONG_SHIELDING:
          if (vict == ch || grouped_with(g, ch, vict))
            play_shielding(g, ch, vict, skill);
          if (out_of_ki(g, ch))
            return;
          break;
        default:
          if (teleport && vict != ch && grouped_with(g, ch, vict))
            teleport_listener(g, ch, vict, *teleport);
          break;
      }
    }

    /* The player goes last */
    if (!teleport)
      return;
    char_from_room(g, ch);
    char_to_room(g, ch, teleport->room);
    char_set_powerattack(ch, 0);
    act(g,
        str_format("@CFinally as the last of your comrades has been teleported you teleport yourself to {} and stop your song.@n",
                   teleport->planet)
            .c_str(),
        true, ch, nullptr, nullptr, TO_CHAR);
  }

  /* What song NAME plays */
  struct tune {
    const char *name;
    int cost;      /* times 1% of max ki */
    int min_skill; /* in mystic music */
    int song;      /* 0: the Teleportation Melody, which takes a destination */
    const char *to_char;
    const char *to_room;
  };
  constexpr tune TUNES[] = {
    {"shielding", 20, 99, SONG_SHIELDING,
     "@CYou begin to play the Song of Shielding! Your fingers lightly glide over the ocarina and as you blow into it a triumphant series of notes issues forth.@n",
     "@c$n@C begins to play a song on $s ocarina. A triumphant song full of soaring sounds from the ocarina as it is played.@n"},
    {"teleport", 50, 80, 0,
     "@CYou begin to play the Melody of Teleportation! Your fingers lightly glide over the ocarina and as you blow into it a repeating light hearted melody issues forth.@n",
     "@c$n@C begins to play a song on $s ocarina. A light hearted melody can be heard sounding from the ocarina as it is played.@n"},
    {"shadow", 8, 50, SONG_SHADOW_STITCH,
     "@CYou begin to play the Shadow Stitch Minuet! Your fingers lightly glide over the ocarina and as you blow into it forboding low toned music issues forth.@n",
     "@c$n@C begins to play a song on $s ocarina. Depressing low toned music issues forth from the ocarina.@n"},
    {"safety", 3, INT_MIN, SONG_SAFETY,
     "@CYou begin to play the Song of Safety! Your fingers lightly glide over the ocarina and as you blow into it sweet music similar to a lullaby issues forth from the intrument.@n",
     "@c$n@C begins to play a song on $s ocarina. The music seems to be some sort of lullaby.@n"},
  };
} // namespace

void handle_songs(GameWorld *g)
{
  for (descriptor_data *d : each_desc(g))
    if (desc_is_playing(d) && d->character && char_get_powerattack(d->character) > 0)
      resolve_song(g, d->character);
}

ACMD(do_song)
{
  if (!char_get_skill(ch, SKILL_MYSTICMUSIC)) {
    send_to_char(g, ch, "You do not know how to play mystical music.\r\n");
    return;
  }

  obj_vnum instrument = held_instrument(g, ch);
  if (!instrument) {
    send_to_char(g, ch, "You do not have an instrument.\r\n");
    return;
  }

  if (char_get_powerattack(ch)) {
    act(g, "@cYou stop playing your ocarina.@n", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "@c$n stops playing their ocarina.@n", true, ch, nullptr, nullptr, TO_ROOM);
    char_set_powerattack(ch, 0);
    return;
  }

  int skill             = char_get_skill(ch, SKILL_MYSTICMUSIC);
  std::string_view rest = argument;
  std::string arg       = take_arg(rest);
  std::string arg2      = take_arg(rest);
  if (arg.empty()) {
    send_to_char(g, ch, "@YSongs Known\n@c-------------------@n\r\n");
    send_to_char(g, ch, "@W{}{}{}Song of Safety\r\n", skill > 99 ? "Song of Shielding\n" : "", skill > 80 ? "Melody of Teleportation\n" : "",
                 skill > 50 ? "Shadow Stitch Minuet\n" : "");
    send_to_char(g, ch, "@wSyntax: song (shielding | safety | teleport | shadow )\r\n");
    return;
  }

  auto found = std::ranges::find_if(TUNES, [&](const tune &t) { return str_eq_nocase(arg, t.name); });
  if (found == std::end(TUNES))
    return; /* quirk: an unknown song says nothing */
  const tune &t = *found;

  int64_t cost = getMaxKI(g, ch) * 0.01;
  cost *= t.cost;
  if (instrument == 8802)
    cost -= cost * 0.5;

  if (getCurKI(g, ch) < cost) {
    send_to_char(g, ch, NO_KI);
    return;
  }
  if (skill < t.min_skill) {
    send_to_char(g, ch, UNSKILLED);
    return;
  }
  if (!t.song) {
    if (arg2.empty()) {
      send_to_char(g, ch, "Where would you like to teleport to?\nSyntax: song teleport (earth | vegeta | kanassa | arlia | aether | namek | konack| frigid)\r\n");
      return;
    }
    if (char_aff_flagged(ch, AFF_SPIRIT)) {
      send_to_char(g, ch, "Not while you're dead!\r\n");
      return;
    }
    auto to = std::ranges::find_if(DESTINATIONS, [&](const destination &d) { return str_eq_nocase(arg2, d.planet); });
    if (to == std::end(DESTINATIONS)) {
      send_to_char(g, ch, "Syntax: song teleport (earth | vegeta | namek | aether | konack | kanassa | arlia | frigid)\r\n");
      return;
    }
    char_set_powerattack(ch, to->song);
  }
  act(g, t.to_char, true, ch, nullptr, nullptr, TO_CHAR);
  act(g, t.to_room, true, ch, nullptr, nullptr, TO_ROOM);
  if (t.song)
    char_set_powerattack(ch, t.song);
  decCurKI(g, ch, cost, 0);
}
