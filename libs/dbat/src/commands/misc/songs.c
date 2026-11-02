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
#include "dbat/act.misc.h"
#include "dbat/consts/maximums.h"
#include "dbat/consts/songs.h"
#include "dbat/consts/fish.h"
#include "dbat/consts/recipes.h"
#include "dbat/consts/attacks.h"
#include "dbat/utils.h"

#include "dbat/character_utils.h"
#include "dbat/room_utils.h"
#include "dbat/object_utils.h"
#include "dbat/descriptor_utils.h"

#include "dbat/random.h"
#include "dbat/affect.h"
#include "dbat/search.h"
#include "dbat/stringutils.h"
#include "dbat/extract.h"
#include "dbat/relocate.h"
#include "dbat/interpreter.h"

#include "dbat/db.h"

#include "dbat/broadcast.h"
#include "dbat/act.wizard.h"
#include "dbat/act.movement.h"
#include "dbat/act.informative.h"

#include "dbat/spells.h"
#include "dbat/comm.h"

#include "dbat/handler.h"
#include "dbat/combat.h"
#include "dbat/obj_edit.h"
#include "dbat/fight.h"
#include "dbat/class.h"
#include "dbat/act.item.h"
#include "misc.h"

/* local functions */
static void resolve_song(struct char_data *ch);

void handle_songs()
{
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    if (!desc_is_playing(d))
      continue;
    if (!d->character)
      continue;
    if (char_get_powerattack(d->character) > 0) {
      resolve_song(d->character);
    }
  }
}

static void apply_shadow_sitch(struct char_data *ch, struct char_data *vict, int skill)
{
  act("@CYour forboding music has caused @c$N's@C shadows to stitch into $S body, slowing $S actions!@n", true, ch, 0, vict, TO_CHAR);
  act("@c$n's@C forboding music has caused YOUR shadows to stitch into YOUR body, slow YOUR actions down!@n", true, ch, 0, vict, TO_VICT);
  act("@c$n's@C forboding music has caused @c$N's@C shadows to stitch into $S body, slowing $S actions!@n", true, ch, 0, vict, TO_NOTVICT);
  decCurKI(ch, getPercentOfMaxKI(ch, .001) + skill, 0);
  if (!char_is_npc(vict)) {
    char_set_wait(vict, PULSE_2SEC);
  } else {
    assign_affect(vict, AFF_SHADOW_STITCH, SKILL_MYSTICMUSIC, -1, 0, 0, 0, 0, 0, -2);
  }
}

static void resolve_song(struct char_data *ch)
{

  struct char_data *vict = nullptr, *next_v = nullptr;
  struct obj_data *obj2 = nullptr, *next_obj;
  int diceroll          = axion_dice(0);
  int skill             = char_get_skill(ch, SKILL_MYSTICMUSIC);
  int instrument        = 0;

  char buf[MAX_INPUT_LENGTH];

  if (char_get_powerattack(ch) <= 0) {
    return;
  }

  for (obj2 = ch->carrying; obj2; obj2 = next_obj) {
    next_obj = obj2->next_content;
    if (obj_get_vnum(obj2) == 8802 || obj_get_vnum(obj2) == 8807) {
      instrument = obj_get_vnum(obj2);
    }
  }

  if (instrument == 0) {
    send_to_char(ch, "You do not have an instrument.\r\n");
    act("@c$n@C stops playing $s song.@n", true, ch, 0, 0, TO_ROOM);
    char_set_powerattack(ch, 0);
    return;
  }

  if (skill > diceroll) {
    snprintf(buf, sizeof(buf), "@c$n@C continues playing @y'@Y%s@y'@C.@n", char_get_powerattack(ch) == SONG_SAFETY ? "Song of Safety" : (char_get_powerattack(ch) == SONG_SHIELDING ? "Song of Shielding" : (char_get_powerattack(ch) == SONG_SHADOW_STITCH ? "Shadow Stitch Minuet" : "Teleportation Melody")));
    act("@CYou continue playing your song.@n", true, ch, 0, 0, TO_CHAR);
    act(buf, true, ch, 0, 0, TO_ROOM);
  } else {
    act("@CYou mess up a portion of the song, but continue playing.@n", true, ch, 0, 0, TO_CHAR);
    act("@c$n@C messes up a portion of $s song, but continues to play.@n", true, ch, 0, 0, TO_ROOM);
    return;
  }

  for (vict = world[char_get_in_room(ch)].people; vict; vict = next_v) {
    next_v = vict->next_in_room;
    switch (char_get_powerattack(ch)) {
      case SONG_SAFETY:
        if ((ch->master == vict->master || ch == vict->master || vict == ch->master) || vict == ch) {
          if ((char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) || vict == ch) {
            if (ch == vict->master || ch->master == vict || ch->master == vict->master || vict == ch) {
              if (skill > diceroll) {
                int64_t restore = (10 * skill) + ((getMaxKI(ch) * 0.0004) * skill);
                if (vict != ch) {
                  act("@CYour skillfully playing of the Song of Safety has an effect on @c$N@C.@n", true, ch, 0, vict, TO_CHAR);
                } else {
                  act("@CYour skillfully playing of the Song of Safety has an effect on your own body@C.@n", true, ch, 0, vict, TO_CHAR);
                }
                incCurHealth(vict, restore, true);
                incCurST(vict, restore * .5, true);

                if (char_get_limb_condition(vict, (1) - 1) < 100) {
                  char_set_limb_condition(vict, (1) - 1, char_get_limb_condition(vict, (1) - 1) + (1 + (skill * 0.1)));
                  if (char_get_limb_condition(vict, (1) - 1) > 100) {
                    send_to_char(vict, "Your right arm is no longer broken!@n\r\n");
                    char_set_limb_condition(vict, (1) - 1, 100);
                  }
                }
                if (char_get_limb_condition(vict, (2) - 1) < 100) {
                  char_set_limb_condition(vict, (2) - 1, char_get_limb_condition(vict, (2) - 1) + (1 + (skill * 0.1)));
                  if (char_get_limb_condition(vict, (2) - 1) > 100) {
                    send_to_char(vict, "Your left arm is no longer broken!@n\r\n");
                    char_set_limb_condition(vict, (2) - 1, 100);
                  }
                }
                if (char_get_limb_condition(vict, (3) - 1) < 100) {
                  char_set_limb_condition(vict, (3) - 1, char_get_limb_condition(vict, (3) - 1) + (1 + (skill * 0.1)));
                  if (char_get_limb_condition(vict, (3) - 1) > 100) {
                    send_to_char(vict, "Your right leg is no longer broken!@n\r\n");
                    char_set_limb_condition(vict, (3) - 1, 100);
                  }
                }
                if (char_get_limb_condition(vict, (1) - 1) < 100) {
                  char_set_limb_condition(vict, (4) - 1, char_get_limb_condition(vict, (4) - 1) + (1 + (skill * 0.1)));
                  if (char_get_limb_condition(vict, (4) - 1) > 100) {
                    send_to_char(vict, "Your left leg is no longer broken!@n\r\n");
                    char_set_limb_condition(vict, (4) - 1, 100);
                  }
                }
                if (vict != ch) {
                  act("@c$n's@C soothing Song of Safety has recovered some of your powerlevel, stamina, and limb condition.", true, ch, 0, vict, TO_VICT);
                }
                act("@c$n@C continues playing $s ocarina!@n", true, ch, 0, vict, TO_NOTVICT);
                improve_skill(ch, SKILL_MYSTICMUSIC, 2);
                decCurKI(ch, (getMaxKI(ch) * .0003) + skill, 0);
              }
            }
          }
        }
        if ((getCurKI(ch)) <= 0) {
          send_to_char(ch, "You no longer have the ki necessary to play your song.\r\n");
          act("@c$n@C stops playing $s song.@n", true, ch, 0, 0, TO_ROOM);
          char_set_powerattack(ch, 0);
          return;
        }
        break;
      case SONG_SHADOW_STITCH:
        if (ch->master && vict->master) {
          if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
            if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
              continue;
            }
            if (skill > diceroll + 10) {
              apply_shadow_sitch(ch, vict, skill);
            }
          } else if (skill > diceroll + 10) {
            apply_shadow_sitch(ch, vict, skill);
          }
        } else if (skill > diceroll + 10) {
          apply_shadow_sitch(ch, vict, skill);
        }
        if ((getCurKI(ch)) <= 0) {
          send_to_char(ch, "You no longer have the ki necessary to play your song.\r\n");
          act("@c$n@C stops playing $s song.@n", true, ch, 0, 0, TO_ROOM);
          char_set_powerattack(ch, 0);
          return;
        }
        break;
      case SONG_TELEPORT_EARTH:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Earth in a flash!@n", true, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Earth in a flash!@n", true, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", true, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(300));
            }
          }
        }
        break;
      case SONG_TELEPORT_VEGETA:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Vegeta in a flash!@n", true, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Vegeta in a flash!@n", true, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", true, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(2234));
            }
          }
        }
        break;
      case SONG_TELEPORT_FRIGID:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Frigid in a flash!@n", true, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Frigid in a flash!@n", true, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", true, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(4047));
            }
          }
        }
        break;
      case SONG_TELEPORT_KONACK:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Konack in a flash!@n", true, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Konack in a flash!@n", true, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", true, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(8003));
            }
          }
        }
        break;
      case SONG_TELEPORT_NAMEK:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Namek in a flash!@n", true, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Namek in a flash!@n", true, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", true, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(10182));
            }
          }
        }
        break;
      case SONG_TELEPORT_ARLIA:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Arlia in a flash!@n", true, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Arlia in a flash!@n", true, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", true, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(16087));
            }
          }
        }
        break;
      case SONG_TELEPORT_AETHER:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Aether in a flash!@n", true, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Aether in a flash!@n", true, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", true, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(12025));
            }
          }
        }
        break;
      case SONG_TELEPORT_KANASSA:
        if (vict == ch)
          continue;
        if (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP)) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master) {
            if (skill > diceroll) {
              act("@CYour Teleportation Melody has transported @c$N@C to Kanassa in a flash!@n", true, ch, 0, vict, TO_CHAR);
              act("@c$n's@C Teleportation Melody has transported you to Kanassa in a flash!@n", true, ch, 0, vict, TO_VICT);
              act("@c$n's@C Teleportation Melody has transported @c$N@C away in a flash!@n", true, ch, 0, vict, TO_NOTVICT);
              char_from_room(vict);
              char_to_room(vict, real_room(14910));
            }
          }
        }
        break;
      case SONG_SHIELDING:
        if (vict == ch || (char_aff_flagged(ch, AFF_GROUP) && char_aff_flagged(vict, AFF_GROUP))) {
          if (ch == vict->master || ch->master == vict || ch->master == vict->master || vict == ch) {
            if (skill > diceroll) {
              if (vict != ch) {
                act("@CYour triumphant and soaring music has powered a barrier around @c$N@C!@n", true, ch, 0, vict, TO_CHAR);
                act("@c$n's@C triumphant and soaring music has powered a barrier around you!@n", true, ch, 0, vict, TO_VICT);
              } else {
                act("@CYour triumphant and soaring music has powered a barrier around yourself@C!@n", true, ch, 0, vict, TO_CHAR);
              }
              act("@c$n's@C triumphant and soaring music has powered a barrier around @c$N@C!@n", true, ch, 0, vict, TO_NOTVICT);
              char_set_barrier(vict, char_get_barrier(vict) + (((getMaxKI(ch) * 0.005) * (skill * 0.25)) + skill));
              if (char_get_barrier(vict) >= getMaxKI(vict) * 0.75) {
                char_set_barrier(vict, getMaxKI(vict) * 0.75);
              }
              if (!char_aff_flagged(vict, AFF_SANCTUARY)) {
                char_aff_set(vict, AFF_SANCTUARY, true);
              }
              incCurKI(ch, getPercentOfMaxKI(ch, .02) + skill, true);
            }
          }
        }
        if ((getCurKI(ch)) <= 0) {
          send_to_char(ch, "You no longer have the ki necessary to play your song.\r\n");
          act("@c$n@C stops playing $s song.@n", true, ch, 0, 0, TO_ROOM);
          char_set_powerattack(ch, 0);
          return;
        }
        break;
    }
  }

  if (char_get_powerattack(ch) >= 4 && skill > diceroll) {
    switch (char_get_powerattack(ch)) {
      case SONG_TELEPORT_EARTH:
        char_from_room(ch);
        char_to_room(ch, real_room(300));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Earth and stop your song.@n", true, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_VEGETA:
        char_from_room(ch);
        char_to_room(ch, real_room(2234));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Vegeta and stop your song.@n", true, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_FRIGID:
        char_from_room(ch);
        char_to_room(ch, real_room(4047));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Frigid and stop your song.@n", true, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_NAMEK:
        char_from_room(ch);
        char_to_room(ch, real_room(10182));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Namek and stop your song.@n", true, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_KANASSA:
        char_from_room(ch);
        char_to_room(ch, real_room(14910));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Kanassa and stop your song.@n", true, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_AETHER:
        char_from_room(ch);
        char_to_room(ch, real_room(12025));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Aether and stop your song.@n", true, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_ARLIA:
        char_from_room(ch);
        char_to_room(ch, real_room(16087));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Arlia and stop your song.@n", true, ch, 0, 0, TO_CHAR);
        break;
      case SONG_TELEPORT_KONACK:
        char_from_room(ch);
        char_to_room(ch, real_room(8003));
        char_set_powerattack(ch, 0);
        act("@CFinally as the last of your comrades has been teleported you teleport yourself to Konack and stop your song.@n", true, ch, 0, 0, TO_CHAR);
        break;
    }
  }
}

ACMD(do_song)
{

  if (!char_get_skill(ch, SKILL_MYSTICMUSIC)) {
    send_to_char(ch, "You do not know how to play mystical music.\r\n");
    return;
  }

  struct obj_data *obj2 = nullptr, *next_obj;
  int instrument        = 0;

  for (obj2 = ch->carrying; obj2; obj2 = next_obj) {
    next_obj = obj2->next_content;
    if (obj_get_vnum(obj2) == 8802 || obj_get_vnum(obj2) == 8807) {
      instrument = obj_get_vnum(obj2);
    }
  }

  if (instrument == 0) {
    send_to_char(ch, "You do not have an instrument.\r\n");
    return;
  }

  if (char_get_powerattack(ch)) {
    act("@cYou stop playing your ocarina.@n", true, ch, 0, 0, TO_CHAR);
    act("@c$n stops playing their ocarina.@n", true, ch, 0, 0, TO_ROOM);
    char_set_powerattack(ch, 0);
    return;
  }

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int skill = char_get_skill(ch, SKILL_MYSTICMUSIC);

  two_arguments(argument, arg, arg2);
  if (!*arg) {
    send_to_char(ch, "@YSongs Known\n@c-------------------@n\r\n");
    send_to_char(ch, "@W%s%s%sSong of Safety\r\n", skill > 99 ? "Song of Shielding\n" : "", skill > 80 ? "Melody of Teleportation\n" : "", skill > 50 ? "Shadow Stitch Minuet\n" : "");
    send_to_char(ch, "@wSyntax: song (shielding | safety | teleport | shadow )\r\n");
    return;
  }
  int64_t cost = getMaxKI(ch) * 0.01;
  int modifier = 1;
  if (!strcasecmp(arg, "shielding")) {
    modifier = 20;
    cost *= modifier;
  } else if (!strcasecmp(arg, "teleport")) {
    modifier = 50;
    cost *= modifier;
  } else if (!strcasecmp(arg, "shadow")) {
    modifier = 8;
    cost *= modifier;
  } else if (!strcasecmp(arg, "safety")) {
    modifier = 3;
    cost *= modifier;
  }

  if (instrument == 8802) {
    cost -= cost * 0.5;
  }

  if (modifier == 0) {
    send_to_char(ch, "@wSyntax: song (shielding | safety | teleport | shadow )\r\n");
    return;
  }
  if (modifier == 3 && (getCurKI(ch)) < cost) {
    send_to_char(ch, "@wYou do not have enough ki to power the instrument for that song!@n\r\n");
    return;
  }
  if (modifier == 3) {
    act("@CYou begin to play the Song of Safety! Your fingers lightly glide over the ocarina and as you blow into it sweet music similar to a lullaby issues forth from the intrument.@n", true, ch, 0, 0, TO_CHAR);
    act("@c$n@C begins to play a song on $s ocarina. The music seems to be some sort of lullaby.@n", true, ch, 0, 0, TO_ROOM);
    char_set_powerattack(ch, SONG_SAFETY);
    decCurKI(ch, cost, 0);
    return;
  }
  if (modifier == 8 && (getCurKI(ch)) < cost) {
    send_to_char(ch, "@wYou do not have enough ki to power the instrument for that song!@n\r\n");
    return;
  }
  if (modifier == 8 && skill <= 49) {
    send_to_char(ch, "You do not posess the skill to play such a song!\r\n");
    return;
  }
  if (modifier == 8) {
    act("@CYou begin to play the Shadow Stitch Minuet! Your fingers lightly glide over the ocarina and as you blow into it forboding low toned music issues forth.@n", true, ch, 0, 0, TO_CHAR);
    act("@c$n@C begins to play a song on $s ocarina. Depressing low toned music issues forth from the ocarina.@n", true, ch, 0, 0, TO_ROOM);
    char_set_powerattack(ch, SONG_SHADOW_STITCH);
    decCurKI(ch, cost, 0);
    return;
  }
  if (modifier == 50 && (getCurKI(ch)) < cost) {
    send_to_char(ch, "@wYou do not have enough ki to power the instrument for that song!@n\r\n");
    return;
  }
  if (modifier == 50 && skill <= 79) {
    send_to_char(ch, "You do not posess the skill to play such a song!\r\n");
    return;
  }
  if (modifier == 50) {
    if (!*arg2) {
      send_to_char(ch, "Where would you like to teleport to?\nSyntax: song teleport (earth | vegeta | kanassa | arlia | aether | namek | konack| frigid)\r\n");
      return;
    }
    if (char_aff_flagged(ch, AFF_SPIRIT)) {
      send_to_char(ch, "Not while you're dead!\r\n");
      return;
    }
    if (!strcasecmp(arg2, "earth")) {
      char_set_powerattack(ch, SONG_TELEPORT_EARTH);
    } else if (!strcasecmp(arg2, "frigid")) {
      char_set_powerattack(ch, SONG_TELEPORT_FRIGID);
    } else if (!strcasecmp(arg2, "vegeta")) {
      char_set_powerattack(ch, SONG_TELEPORT_VEGETA);
    } else if (!strcasecmp(arg2, "namek")) {
      char_set_powerattack(ch, SONG_TELEPORT_NAMEK);
    } else if (!strcasecmp(arg2, "arlia")) {
      char_set_powerattack(ch, SONG_TELEPORT_ARLIA);
    } else if (!strcasecmp(arg2, "kanassa")) {
      char_set_powerattack(ch, SONG_TELEPORT_KANASSA);
    } else if (!strcasecmp(arg2, "konack")) {
      char_set_powerattack(ch, SONG_TELEPORT_KONACK);
    } else if (!strcasecmp(arg2, "aether")) {
      char_set_powerattack(ch, SONG_TELEPORT_AETHER);
    } else {
      send_to_char(ch, "Syntax: song teleport (earth | vegeta | namek | aether | konack | kanassa | arlia | frigid)\r\n");
      return;
    }
    act("@CYou begin to play the Melody of Teleportation! Your fingers lightly glide over the ocarina and as you blow into it a repeating light hearted melody issues forth.@n", true, ch, 0, 0, TO_CHAR);
    act("@c$n@C begins to play a song on $s ocarina. A light hearted melody can be heard sounding from the ocarina as it is played.@n", true, ch, 0, 0, TO_ROOM);
    decCurKI(ch, cost, 0);
    return;
  }
  if (modifier == 20 && (getCurKI(ch)) < cost) {
    send_to_char(ch, "@wYou do not have enough ki to power the instrument for that song!@n\r\n");
    return;
  }
  if (modifier == 20 && skill <= 98) {
    send_to_char(ch, "You do not posess the skill to play such a song!\r\n");
    return;
  }
  if (modifier == 20) {
    act("@CYou begin to play the Song of Shielding! Your fingers lightly glide over the ocarina and as you blow into it a triumphant series of notes issues forth.@n", true, ch, 0, 0, TO_CHAR);
    act("@c$n@C begins to play a song on $s ocarina. A triumphant song full of soaring sounds from the ocarina as it is played.@n", true, ch, 0, 0, TO_ROOM);
    char_set_powerattack(ch, SONG_SHIELDING);
    decCurKI(ch, cost, 0);
    return;
  }
}
