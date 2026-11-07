/*
 * Messages broadcast to many players at once by game systems: power-level sensing,
 * scouters, planet-wide quakes, flying characters seen across a zone, and the
 * immortal log channel. (These used to live in dg/dg_comm.c, but are not part of
 * DG Scripts.)
 */
#include "dbat/broadcast.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/utils.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/races.hpp"
#include "dbat/comm.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/graph.hpp"
#include "dbat/spells.hpp"
#include "dbat/handler.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/search.hpp"
#include "dbat/game_world.hpp"

void fly_zone(GameWorld *g, zone_rnum zone, const char *messg, struct char_data *ch)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = g->descriptors; i; i = i->next) {
    if (!i->connected && i->character && char_is_awake(i->character) && char_is_outside(g, i->character) && (char_get_in_room(i->character) != NOWHERE) && (rroom_get_zone(g, char_get_in_room(i->character)) == zone) && i->character != ch) {
      if (char_plr_flagged(i->character, PLR_DISGUISED)) {
        write_to_output(g, i, "A disguised figure %s", messg);
      } else {
        write_to_output(g, i, "%s%s %s", readIntro(i->character, ch) == 1 ? "" : "A ", get_i_name(i->character, ch), messg);
      }
    }
  }
}

void send_to_sense(GameWorld *g, int type, const char *messg, struct char_data *ch)
{
  struct descriptor_data *i;
  struct char_data *tch;
  struct obj_data *obj;

  if (!messg || !*messg)
    return;

  for (i = g->descriptors; i; i = i->next) {
    if (desc_get_connected(i) != CON_PLAYING) {
      continue;
    }
    tch = i->character;
    obj = char_get_equipment(tch, WEAR_EYE);
    if (tch == ch) {
      continue;
    }
    if (!char_get_skill(tch, SKILL_SENSE)) {
      continue;
    }
    if (((rroom_get_zone(g, char_get_in_room(ch)) != rroom_get_zone(g, char_get_in_room(tch)) && type == 0) || !char_is_awake(tch))) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_SHIP)) {
      continue;
    }
    if (obj && type == 0) {
      continue;
    }
    if (char_is_android(ch)) {
      continue;
    }
    if (char_get_in_room(ch) == char_get_in_room(tch)) {
      continue;
    }
    if (getCurPL(g, ch) < (getCurPL(g, tch) * 0.001) + 1) {
      continue;
    }
    if (type == 0) {
      if (getEffMaxPL(g, ch) > getEffMaxPL(g, tch)) {
        write_to_output(g, i, "%s who is stronger than you. They are nearby.\r\n", messg);
      } else if (getEffMaxPL(g, ch) >= getEffMaxPL(g, tch) * .9) {
        write_to_output(g, i, "%s who is near your strength. They are nearby.\r\n", messg);
      } else if (getEffMaxPL(g, ch) >= getEffMaxPL(g, tch) * .6) {
        write_to_output(g, i, "%s who is a good bit weaker than you. They are nearby.\r\n", messg);
      } else if (getEffMaxPL(g, ch) >= getEffMaxPL(g, tch) * .4) {
        write_to_output(g, i, "%s who is a lot weaker than you. They are nearby.\r\n", messg);
      } else {
        continue;
      }
      if (readIntro(tch, ch) == 1) {
        write_to_output(g, i, "@YYou recognise this signal as @y%s@Y!@n\r\n", get_i_name(tch, ch));
      } else if (read_sense_memory(g, ch, tch)) {
        write_to_output(g, i, "@YYou recognise this signal, but don't seem to know their name.@n\r\n");
      }
    } else if (planet_check(g, ch, tch)) {
      char *blah = sense_location(g, ch);
      char power[MAX_INPUT_LENGTH];
      char align[MAX_INPUT_LENGTH];
      if (getCurPL(g, ch) > getCurPL(g, tch) * 10) {
        sprintf(power, ", who is @Runbelievably stronger@Y than you");
      } else if (getCurPL(g, ch) > getCurPL(g, tch) * 5) {
        sprintf(power, ", who is much @Rstronger@Y than you");
      } else if (getCurPL(g, ch) > getCurPL(g, tch) * 2) {
        sprintf(power, ", who is more than twice as @Rstrong@Y as you");
      } else if (getCurPL(g, ch) > getCurPL(g, tch)) {
        sprintf(power, ", who is somewhat @mstronger@Y than you");
      } else if (getCurPL(g, ch) * 10 < getCurPL(g, tch)) {
        sprintf(power, ", who is @Munbelievably weaker@Y than you");
      } else if (getCurPL(g, ch) * 5 < getCurPL(g, tch)) {
        sprintf(power, ", who is much @Mweaker@Y than you");
      } else if (getCurPL(g, ch) * 2 < getCurPL(g, tch)) {
        sprintf(power, ", who is more than twice as @Mweak@Y as you");
      } else if (getCurPL(g, ch) < getCurPL(g, tch)) {
        sprintf(power, ", who is somewhat @Wweaker@Y than you");
      } else {
        sprintf(power, ", who is close to @Cequal@Y with you");
      }
      if (char_get_alignment(ch) >= 1000) {
        sprintf(align, ", with a @wsaintly@Y aura,");
      } else if (char_get_alignment(ch) >= 500) {
        sprintf(align, ", with a very @Cgood@Y aura,");
      } else if (char_get_alignment(ch) >= 200) {
        sprintf(align, ", with a @cgood@Y aura,");
      } else if (char_get_alignment(ch) > -100) {
        sprintf(align, ", with a near @Wneutral@Y aura,");
      } else if (char_get_alignment(ch) > -200) {
        sprintf(align, ", with a sorta @revil@Y aura,");
      } else if (char_get_alignment(ch) > -500) {
        sprintf(align, ", with an @revil@Y aura,");
      } else if (char_get_alignment(ch) > -900) {
        sprintf(align, ", with a @rvery evil@Y aura,");
      } else {
        sprintf(align, ", with a @rd@De@Wv@wil@Wi@Ds@rh@Y aura,");
      }
      if (strstr(messg, "land"))
        write_to_output(g, i, "@YYou sense %s%s%s %s! They appear to have landed at...@G%s@n\r\n", readIntro(tch, ch) == 1 ? get_i_name(tch, ch) : "someone", power, align, messg, blah);
      else
        write_to_output(g, i, "@YYou sense %s%s%s %s!@n\r\n", readIntro(tch, ch) == 1 ? get_i_name(tch, ch) : "someone", power, align, messg);
    }
  }
}

void send_to_scouter(GameWorld *g, const char *messg, struct char_data *ch, int num, int type)
{
  struct descriptor_data *i;
  struct char_data *tch;
  struct obj_data *obj;

  if (!messg || !*messg)
    return;

  for (i = g->descriptors; i; i = i->next) {
    if (desc_get_connected(i) != CON_PLAYING) {
      continue;
    }
    tch = i->character;
    obj = char_get_equipment(tch, WEAR_EYE);
    if (tch == ch) {
      continue;
    }
    if ((((rroom_get_zone(g, char_get_in_room(ch)) != rroom_get_zone(g, char_get_in_room(tch))) && type == 0) || !char_is_awake(tch))) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_SHIP)) {
      continue;
    }
    if (char_get_invis_level(g, ch) > char_get_admlevel(tch)) {
      continue;
    }
    if (char_is_android(ch)) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_EARTH) && !rroom_flagged(g, char_get_in_room(tch), ROOM_EARTH)) {
      continue;
    }
    if (rroom_planet_zenith(g, char_get_in_room(ch)) && !rroom_planet_zenith(g, char_get_in_room(tch))) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_FRIGID) && !rroom_flagged(g, char_get_in_room(tch), ROOM_FRIGID)) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_NAMEK) && !rroom_flagged(g, char_get_in_room(tch), ROOM_NAMEK)) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_AL) && !rroom_flagged(g, char_get_in_room(tch), ROOM_AL)) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_VEGETA) && !rroom_flagged(g, char_get_in_room(tch), ROOM_VEGETA)) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_KONACK) && !rroom_flagged(g, char_get_in_room(tch), ROOM_KONACK)) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_NEO) && !rroom_flagged(g, char_get_in_room(tch), ROOM_NEO)) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_YARDRAT) && !rroom_flagged(g, char_get_in_room(tch), ROOM_YARDRAT)) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_KANASSA) && !rroom_flagged(g, char_get_in_room(tch), ROOM_KANASSA)) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_ARLIA) && !rroom_flagged(g, char_get_in_room(tch), ROOM_ARLIA)) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(ch), ROOM_AETHER) && !rroom_flagged(g, char_get_in_room(tch), ROOM_AETHER)) {
      continue;
    }
    if (!obj) {
      continue;
    }
    if (char_get_in_room(ch) == char_get_in_room(tch)) {
      continue;
    }
    if (type == 0) {
      if (num == 1) {
        if (obj_extra_flagged(obj, ITEM_BSCOUTER) && getCurPL(g, ch) >= 150000) {
          write_to_output(g, i, "@D[@GBlip@D]@r Rising Powerlevel Detected@D:@Y ??????????\r\n");
        } else if (obj_extra_flagged(obj, ITEM_MSCOUTER) && getCurPL(g, ch) >= 5000000) {
          write_to_output(g, i, "@D[@GBlip@D]@r Rising Powerlevel Detected@D:@Y ??????????\r\n");
        } else if (obj_extra_flagged(obj, ITEM_ASCOUTER) && getCurPL(g, ch) >= 15000000) {
          write_to_output(g, i, "@D[@GBlip@D]@r Rising Powerlevel Detected@D:@Y ??????????\r\n");
        } else {
          write_to_output(g, i, "%s@n", messg);
        }
      } else {
        if (obj_extra_flagged(obj, ITEM_BSCOUTER) && getCurPL(g, ch) >= 150000) {
          write_to_output(g, i, "@D[@GBlip@D]@r Nearby Powerlevel Detected@D:@Y ??????????\r\n");
        } else if (obj_extra_flagged(obj, ITEM_MSCOUTER) && getCurPL(g, ch) >= 5000000) {
          write_to_output(g, i, "@D[@GBlip@D]@r Nearby Powerlevel Detected@D:@Y ??????????\r\n");
        } else if (obj_extra_flagged(obj, ITEM_ASCOUTER) && getCurPL(g, ch) >= 15000000) {
          write_to_output(g, i, "@D[@GBlip@D]@r Nearby Powerlevel Detected@D:@Y ??????????\r\n");
        } else {
          write_to_output(g, i, "%s\r\n", messg);
        }
      }
    } else if (type == 1 && char_get_skill(tch, SKILL_SENSE) < 20) {
      if (obj_extra_flagged(obj, ITEM_BSCOUTER) && getCurPL(g, ch) >= 150000) {
        write_to_output(g, i, "@D[@GBlip@D]@w %s. @RPL@D:@Y ??????????\r\n", messg);
      } else if (obj_extra_flagged(obj, ITEM_MSCOUTER) && getCurPL(g, ch) >= 5000000) {
        write_to_output(g, i, "@D[@GBlip@D]@w %s. @RPL@D:@Y ??????????\r\n", messg);
      } else if (obj_extra_flagged(obj, ITEM_ASCOUTER) && getCurPL(g, ch) >= 15000000) {
        write_to_output(g, i, "@D[@GBlip@D]@w %s. @RPL@D:@Y ??????????\r\n", messg);
      } else {
        write_to_output(g, i, "@D[Blip@D]@w %s. @RPL@D:@Y %s@n\r\n\r\n", messg, add_commas(getCurPL(g, ch)));
      }
    } else if (type == 2 && char_get_skill(tch, SKILL_SENSE) < 20) {
      char *blah = sense_location(g, ch);
      if (obj_extra_flagged(obj, ITEM_BSCOUTER) && getCurPL(g, ch) >= 150000) {
        write_to_output(g, i, "@D[@GBlip@D]@w %s at... @G%s. @RPL@D:@Y ??????????\r\n", messg, blah);
      } else if (obj_extra_flagged(obj, ITEM_MSCOUTER) && getCurPL(g, ch) >= 5000000) {
        write_to_output(g, i, "@D[@GBlip@D]@w %s at... @G%s. @RPL@D:@Y ??????????\r\n", messg, blah);
      } else if (obj_extra_flagged(obj, ITEM_ASCOUTER) && getCurPL(g, ch) >= 15000000) {
        write_to_output(g, i, "@D[@GBlip@D]@w %s at... @G%s. @RPL@D:@Y ??????????\r\n", messg, blah);
      } else {
        write_to_output(g, i, "@D[Blip@D]@w %s at... @G%s. @RPL@D:@Y %s@n\r\n\r\n", messg, blah, add_commas(getCurPL(g, ch)));
      }
    }
  }
}

void send_to_worlds(GameWorld *g, struct char_data *ch)
{
  struct descriptor_data *i;
  char message[MAX_INPUT_LENGTH];

  if (getEffMaxPL(g, ch) > 2000000000) {
    sprintf(message, "@RThe whole planet begins to quake violently as if the world is ending!@n\r\n");
  } else if (getEffMaxPL(g, ch) > 1000000000) {
    sprintf(message, "@RThe whole planet begins to quake violently with a thunderous roar!@n\r\n");
  } else if (getEffMaxPL(g, ch) > 500000000) {
    sprintf(message, "@RThe whole planet begins to quake violently!@n\r\n");
  } else if (getEffMaxPL(g, ch) > 100000000) {
    sprintf(message, "@RThe whole planet rumbles and shakes!@n\r\n");
  } else if (getEffMaxPL(g, ch) > 50000000) {
    sprintf(message, "@RThe whole planet rumbles faintly!@n\r\n");
  } else {
    return;
  }

  for (i = g->descriptors; i; i = i->next) {
    if (desc_get_connected(i) != CON_PLAYING) {
      continue;
    }
    if (rroom_flagged(g, char_get_in_room(i->character), ROOM_EARTH) && rroom_flagged(g, char_get_in_room(ch), ROOM_EARTH)) {
      send_to_char(g, i->character, "%s", message);
    } else if (rroom_flagged(g, char_get_in_room(i->character), ROOM_VEGETA) && rroom_flagged(g, char_get_in_room(ch), ROOM_VEGETA)) {
      send_to_char(g, i->character, "%s", message);
    } else if (rroom_planet_zenith(g, char_get_in_room(i->character)) && rroom_planet_zenith(g, char_get_in_room(ch))) {
      send_to_char(g, i->character, "%s", message);
    } else if (rroom_flagged(g, char_get_in_room(i->character), ROOM_NAMEK) && rroom_flagged(g, char_get_in_room(ch), ROOM_NAMEK)) {
      send_to_char(g, i->character, "%s", message);
    } else if (rroom_flagged(g, char_get_in_room(i->character), ROOM_KONACK) && rroom_flagged(g, char_get_in_room(ch), ROOM_KONACK)) {
      send_to_char(g, i->character, "%s", message);
    } else if (rroom_flagged(g, char_get_in_room(i->character), ROOM_YARDRAT) && rroom_flagged(g, char_get_in_room(ch), ROOM_YARDRAT)) {
      send_to_char(g, i->character, "%s", message);
    } else if (rroom_flagged(g, char_get_in_room(i->character), ROOM_FRIGID) && rroom_flagged(g, char_get_in_room(ch), ROOM_FRIGID)) {
      send_to_char(g, i->character, "%s", message);
    } else if (rroom_flagged(g, char_get_in_room(i->character), ROOM_KANASSA) && rroom_flagged(g, char_get_in_room(ch), ROOM_KANASSA)) {
      send_to_char(g, i->character, "%s", message);
    } else if (rroom_flagged(g, char_get_in_room(i->character), ROOM_ARLIA) && rroom_flagged(g, char_get_in_room(ch), ROOM_ARLIA)) {
      send_to_char(g, i->character, "%s", message);
    } else if (rroom_flagged(g, char_get_in_room(i->character), ROOM_AETHER) && rroom_flagged(g, char_get_in_room(ch), ROOM_AETHER)) {
      send_to_char(g, i->character, "%s", message);
    }
  }
}

void send_to_imm(GameWorld *g, const char *messg, ...)
{
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = g->descriptors; i; i = i->next) {
    if (desc_get_connected(i) != CON_PLAYING) {
      continue;
    }
    if (char_get_admlevel(i->character) == 0) {
      continue;
    }
    if (!char_prf_flagged(g, i->character, PRF_LOG2)) {
      continue;
    }
    if (char_plr_flagged(i->character, PLR_WRITING)) {
      continue;
    }
    write_to_output(g, i, "@g[ Log: ");
    va_list args;
    va_start(args, messg);

    vwrite_to_output(g, i, messg, args);
    write_to_output(g, i, " ]@n\n");
    va_end(args);
  }
  va_list args;
  va_start(args, messg);
  mud_vlog(messg, args);
  va_end(args);
}
