/* look.c: look, examine, rooms, exits and targets
 * Split from act.informative.c (CircleMUD/DBAT; see license.doc). */
#include "dbat/consts/maximums.h"
#include "dbat/consts/attacks.h"
#include "dbat/utils.h"
#include "dbat/config.h"
#include "dbat/consts/songs.h"
#include "dbat/consts/time.h"
#include "dbat/help.h"
#include "dbat/command.h"
#include "dbat/flags.h"
#include "dbat/affect.h"
#include "dbat/stringutils.h"
#include "dbat/random.h"
#include "dbat/object_utils.h"
#include "dbat/character_utils.h"
#include "dbat/descriptor_utils.h"
#include "dbat/room_utils.h"
#include "dbat/search.h"
#include "dbat/relocate.h"
#include "dbat/fileop.h"
#include "dbat/races.h"
#include "dbat/interpreter.h"
#include "dbat/dg_scripts.h"
#include "dbat/extract.h"
#include "dbat/act.informative.h"
#include "dbat/act.wizard.h"
#include "dbat/vehicles.h"
#include "dbat/act.item.h"
#include "dbat/act.social.h"
#include "dbat/maputils.h"
#include "dbat/comm.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/boards.h"
#include "dbat/screen.h"
#include "dbat/mail.h"
#include "dbat/guild.h"
#include "dbat/disabled.h"
#include "informative_internal.h"

static void look_in_direction(struct char_data *ch, int dir);
static void look_in_obj(struct char_data *ch, char *arg);
static void look_at_target(struct char_data *ch, char *arg, int read);
static void do_auto_exits(room_rnum target_room, struct char_data *ch, int exit_mode);
static void do_auto_exits2(room_rnum target_room, struct char_data *ch);

/* Portal appearance types */
static const char *portal_appearance[] = {
    "All you can see is the glow of the portal.",
    "You see an image of yourself in the room - my, you are looking attractive today.",
    "All you can see is a swirling grey mist.",
    "The scene is of the surrounding countryside, but somehow blurry and lacking focus.",
    "The blackness appears to stretch on forever.",
    "Suddenly, out of the blackness a flaming red eye appears and fixes its gaze upon you.",
    "\n"};

/* Used when checking status or looking at a character */
void bringdesc(struct char_data *ch, struct char_data *tch)
{

  if (ch != NULL && tch != NULL && char_is_humanoid(tch)) {

    if (ch != tch && char_plr_flagged(tch, PLR_DISGUISED)) {
      send_to_char(ch, "            @D[@cHair Length @D: @WHidden.         @D]@n\r\n");
      send_to_char(ch, "            @D[@cHair Color  @D: @WHidden.         @D]@n\r\n");
      send_to_char(ch, "            @D[@cHair Style  @D: @WHidden.         @D]@n\r\n");
      send_to_char(ch, "            @D[@cEye Color   @D: @WHidden.         @D]@n\r\n");
      if (char_get_skin(tch) == SKIN_WHITE) {
        send_to_char(ch, "            @D[@cSkin Color  @D: @WWhite.        @D]@n\r\n");
      } else if (char_get_skin(tch) == SKIN_TAN) {
        send_to_char(ch, "            @D[@cSkin Color  @D: @WTan.          @D]@n\r\n");
      } else if (char_get_skin(tch) == SKIN_BLACK) {
        send_to_char(ch, "            @D[@cSkin Color  @D: @WBlack.        @D]@n\r\n");
      } else if (char_get_skin(tch) == SKIN_GREEN) {
        send_to_char(ch, "            @D[@cSkin Color  @D: @WGreen.        @D]@n\r\n");
      } else if (char_get_skin(tch) == SKIN_ORANGE) {
        send_to_char(ch, "            @D[@cSkin Color  @D: @WOrange.       @D]@n\r\n");
      } else if (char_get_skin(tch) == SKIN_YELLOW) {
        send_to_char(ch, "            @D[@cSkin Color  @D: @WYellow.       @D]@n\r\n");
      } else if (char_get_skin(tch) == SKIN_RED) {
        send_to_char(ch, "            @D[@cSkin Color  @D: @WRed.          @D]@n\r\n");
      } else if (char_get_skin(tch) == SKIN_GREY) {
        send_to_char(ch, "            @D[@cSkin Color  @D: @WGrey.         @D]@n\r\n");
      } else if (char_get_skin(tch) == SKIN_BLUE) {
        send_to_char(ch, "            @D[@cSkin Color  @D: @WBlue.         @D]@n\r\n");
      } else if (char_get_skin(tch) == SKIN_AQUA) {
        send_to_char(ch, "            @D[@cSkin Color  @D: @WAqua.         @D]@n\r\n");
      } else if (char_get_skin(tch) == SKIN_PINK) {
        send_to_char(ch, "            @D[@cSkin Color  @D: @WPink.         @D]@n\r\n");
      } else if (char_get_skin(tch) == SKIN_PURPLE) {
        send_to_char(ch, "            @D[@cSkin Color  @D: @WPurple.       @D]@n\r\n");
      }
      return;
    }

    if (char_is_human(tch) || char_is_saiyan(tch) || char_is_konatsu(tch) || char_is_mutant(tch) || char_is_android(tch) || char_is_kai(tch) || char_is_halfbreed(tch) || char_is_truffle(tch) || char_is_hoshijin(tch)) {
      if ((!char_is_saiyan(tch) && !char_is_halfbreed(tch)) || ((char_is_saiyan(tch) || char_is_halfbreed(tch)) && !char_is_transformed(tch))) {
        if (char_get_hairl(tch) == HAIRL_LONG) {
          send_to_char(ch, "            @D[@cHair Length @D: @WLong.         @D]@n\r\n");
        } else if (char_get_hairl(tch) == HAIRL_BALD) {
          send_to_char(ch, "            @D[@cHair Length @D: @WBald.         @D]@n\r\n");
        } else if (char_get_hairl(tch) == HAIRL_SHORT) {
          send_to_char(ch, "            @D[@cHair Length @D: @WShort.        @D]@n\r\n");
        } else if (char_get_hairl(tch) == HAIRL_MEDIUM) {
          send_to_char(ch, "            @D[@cHair Length @D: @WMedium.       @D]@n\r\n");
        } else if (char_get_hairl(tch) == HAIRL_RLONG) {
          send_to_char(ch, "            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
        }
        if (char_get_hairs(tch) == HAIRS_PLAIN) {
          send_to_char(ch, "            @D[@cHair Style  @D: @WPlain.        @D]@n\r\n");
        } else if (char_get_hairs(tch) == HAIRS_MOHAWK) {
          send_to_char(ch, "            @D[@cHair Style  @D: @WMohawk.       @D]@n\r\n");
        } else if (char_get_hairs(tch) == HAIRS_SPIKY) {
          send_to_char(ch, "            @D[@cHair Style  @D: @WSpiky.        @D]@n\r\n");
        } else if (char_get_hairs(tch) == HAIRS_CURLY) {
          send_to_char(ch, "            @D[@cHair Style  @D: @WCurly.        @D]@n\r\n");
        } else if (char_get_hairs(tch) == HAIRS_UNEVEN) {
          send_to_char(ch, "            @D[@cHair Style  @D: @WUneven.       @D]@n\r\n");
        } else if (char_get_hairs(tch) == HAIRS_PONYTAIL) {
          send_to_char(ch, "            @D[@cHair Style  @D: @WPony Tail.    @D]@n\r\n");
        } else if (char_get_hairs(tch) == HAIRS_AFRO) {
          send_to_char(ch, "            @D[@cHair Style  @D: @WAfro.         @D]@n\r\n");
        } else if (char_get_hairs(tch) == HAIRS_FADE) {
          send_to_char(ch, "            @D[@cHair Style  @D: @WFade.         @D]@n\r\n");
        } else if (char_get_hairs(tch) == HAIRS_CREW) {
          send_to_char(ch, "            @D[@cHair Style  @D: @WCrew Cut.     @D]@n\r\n");
        } else if (char_get_hairs(tch) == HAIRS_FEATHERED) {
          send_to_char(ch, "            @D[@cHair Style  @D: @WFeathered.    @D]@n\r\n");
        } else if (char_get_hairs(tch) == HAIRS_DRED) {
          send_to_char(ch, "            @D[@cHair Style  @D: @WDread Locks.  @D]@n\r\n");
        }
        if (char_get_hairc(tch) == HAIRC_BLACK) {
          send_to_char(ch, "            @D[@cHair Color  @D: @WBlack.        @D]@n\r\n");
        } else if (char_get_hairc(tch) == HAIRC_BROWN) {
          send_to_char(ch, "            @D[@cHair Color  @D: @WBrown.        @D]@n\r\n");
        } else if (char_get_hairc(tch) == HAIRC_BLONDE) {
          send_to_char(ch, "            @D[@cHair Color  @D: @WBlonde.       @D]@n\r\n");
        } else if (char_get_hairc(tch) == HAIRC_GREY) {
          send_to_char(ch, "            @D[@cHair Color  @D: @WGrey.         @D]@n\r\n");
        } else if (char_get_hairc(tch) == HAIRC_RED) {
          send_to_char(ch, "            @D[@cHair Color  @D: @WRed.          @D]@n\r\n");
        } else if (char_get_hairc(tch) == HAIRC_ORANGE) {
          send_to_char(ch, "            @D[@cHair Color  @D: @WOrange.       @D]@n\r\n");
        } else if (char_get_hairc(tch) == HAIRC_GREEN) {
          send_to_char(ch, "            @D[@cHair Color  @D: @WGreen.        @D]@n\r\n");
        } else if (char_get_hairc(tch) == HAIRC_BLUE) {
          send_to_char(ch, "            @D[@cHair Color  @D: @WBlue.         @D]@n\r\n");
        } else if (char_get_hairc(tch) == HAIRC_PINK) {
          send_to_char(ch, "            @D[@cHair Color  @D: @WPink.         @D]@n\r\n");
        } else if (char_get_hairc(tch) == HAIRC_PURPLE) {
          send_to_char(ch, "            @D[@cHair Color  @D: @WPurple.       @D]@n\r\n");
        } else if (char_get_hairc(tch) == HAIRC_SILVER) {
          send_to_char(ch, "            @D[@cHair Color  @D: @WSilver.       @D]@n\r\n");
        } else if (char_get_hairc(tch) == HAIRC_CRIMSON) {
          send_to_char(ch, "            @D[@cHair Color  @D: @WCrimson.      @D]@n\r\n");
        } else if (char_get_hairc(tch) == HAIRC_WHITE) {
          send_to_char(ch, "            @D[@cHair Color  @D: @WWhite.        @D]@n\r\n");
        }
      } else if (char_is_saiyan(tch) || char_is_halfbreed(tch)) {
        if (char_plr_flagged(tch, PLR_TRANS1)) {
          if (char_get_hairl(tch) == HAIRL_LONG) {
            send_to_char(ch, "            @D[@cHair Length @D: @WLong.         @D]@n\r\n");
          } else if (char_get_hairl(tch) == HAIRL_BALD) {
            send_to_char(ch, "            @D[@cHair Length @D: @WBald.         @D]@n\r\n");
          } else if (char_get_hairl(tch) == HAIRL_SHORT) {
            send_to_char(ch, "            @D[@cHair Length @D: @WShort.        @D]@n\r\n");
          } else if (char_get_hairl(tch) == HAIRL_MEDIUM) {
            send_to_char(ch, "            @D[@cHair Length @D: @WMedium.       @D]@n\r\n");
          } else if (char_get_hairl(tch) == HAIRL_RLONG) {
            send_to_char(ch, "            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
          }
          send_to_char(ch, "            @D[@cHair Style  @D: @WSpiky.        @D]@n\r\n");
          send_to_char(ch, "            @D[@cHair Color  @D: @WGolden.       @D]@n\r\n");
          send_to_char(ch, "            @D[@cEye Color   @D: @WEmerald.      @D]@n\r\n");
        } else if (char_plr_flagged(tch, PLR_TRANS2)) {
          if (char_get_hairl(tch) == HAIRL_LONG) {
            send_to_char(ch, "            @D[@cHair Length @D: @WLong.         @D]@n\r\n");
          } else if (char_get_hairl(tch) == HAIRL_BALD) {
            send_to_char(ch, "            @D[@cHair Length @D: @WBald.         @D]@n\r\n");
          } else if (char_get_hairl(tch) == HAIRL_SHORT) {
            send_to_char(ch, "            @D[@cHair Length @D: @WShort.        @D]@n\r\n");
          } else if (char_get_hairl(tch) == HAIRL_MEDIUM) {
            send_to_char(ch, "            @D[@cHair Length @D: @WMedium.       @D]@n\r\n");
          } else if (char_get_hairl(tch) == HAIRL_RLONG) {
            send_to_char(ch, "            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
          }
          send_to_char(ch, "            @D[@cHair Style  @D: @WSharp Spikes. @D]@n\r\n");
          send_to_char(ch, "            @D[@cHair Color  @D: @WGolden.       @D]@n\r\n");
          send_to_char(ch, "            @D[@cEye Color   @D: @WEmerald.      @D]@n\r\n");
        } else if (char_plr_flagged(tch, PLR_TRANS3)) {
          send_to_char(ch, "            @D[@cHair Length @D: @WReally Long.  @D]@n\r\n");
          send_to_char(ch, "            @D[@cHair Style  @D: @WSpiky.        @D]@n\r\n");
          send_to_char(ch, "            @D[@cHair Color  @D: @WGolden.       @D]@n\r\n");
          send_to_char(ch, "            @D[@cEye Color   @D: @WAqua Green.   @D]@n\r\n");
        } else if (char_plr_flagged(tch, PLR_TRANS4)) {
          send_to_char(ch, "            @D[@cHair Length @D: @WLong.        @D]@n\r\n");
          send_to_char(ch, "            @D[@cHair Style  @D: @WSoft Spikes. @D]@n\r\n");
          send_to_char(ch, "            @D[@cHair Color  @D: @WBlack.       @D]@n\r\n");
          send_to_char(ch, "            @D[@cEye Color   @D: @WAmber.       @D]@n\r\n");
        }
      }
    }
    if (char_is_demon(tch) || char_is_icer(tch)) {
      if (char_get_hairl(tch) == HAIRL_BALD) {
        send_to_char(ch, "            @D[@cHorn Length @D: @WNone.         @D]@n\r\n");
      }
      if (char_get_hairl(tch) == HAIRL_SHORT) {
        send_to_char(ch, "            @D[@cHorn Length @D: @WShort.        @D]@n\r\n");
      }
      if (char_get_hairl(tch) == HAIRL_MEDIUM) {
        send_to_char(ch, "            @D[@cHorn Length @D: @WMedium.       @D]@n\r\n");
      }
      if (char_get_hairl(tch) == HAIRL_LONG) {
        send_to_char(ch, "            @D[@cHorn Length @D: @WLong.         @D]@n\r\n");
      }
      if (char_get_hairl(tch) == HAIRL_RLONG) {
        send_to_char(ch, "            @D[@cHorn Length @D: @WReally Long.  @D]@n\r\n");
      }
    }
    if (char_is_namek(tch) || char_is_arlian(tch)) {
      if (char_get_hairl(tch) == HAIRL_BALD) {
        send_to_char(ch, "            @D[@cAnt. Length @D: @WTiny.        @D]@n\r\n");
      }
      if (char_get_hairl(tch) == HAIRL_SHORT) {
        send_to_char(ch, "            @D[@cAnt. Length @D: @WShort.       @D]@n\r\n");
      }
      if (char_get_hairl(tch) == HAIRL_MEDIUM) {
        send_to_char(ch, "            @D[@cAnt. Length @D: @WMedium.      @D]@n\r\n");
      }
      if (char_get_hairl(tch) == HAIRL_LONG) {
        send_to_char(ch, "            @D[@cAnt. Length @D: @WLong.        @D]@n\r\n");
      }
      if (char_get_hairl(tch) == HAIRL_RLONG) {
        send_to_char(ch, "            @D[@cAnt. Length @D: @WR. Long.     @D]@n\r\n");
      }
    }
    if (char_is_arlian(tch) && char_is_female(tch)) {
      if (char_get_hairc(tch) == HAIRC_BLACK) {
        send_to_char(ch, "            @D[@cWing Color  @D: @WBlack.        @D]@n\r\n");
      } else if (char_get_hairc(tch) == HAIRC_BROWN) {
        send_to_char(ch, "            @D[@cWing Color  @D: @WBrown.        @D]@n\r\n");
      } else if (char_get_hairc(tch) == HAIRC_BLONDE) {
        send_to_char(ch, "            @D[@cWing Color  @D: @WBlonde.       @D]@n\r\n");
      } else if (char_get_hairc(tch) == HAIRC_GREY) {
        send_to_char(ch, "            @D[@cWing Color  @D: @WGrey.         @D]@n\r\n");
      } else if (char_get_hairc(tch) == HAIRC_RED) {
        send_to_char(ch, "            @D[@cWing Color  @D: @WRed.          @D]@n\r\n");
      } else if (char_get_hairc(tch) == HAIRC_ORANGE) {
        send_to_char(ch, "            @D[@cWing Color  @D: @WOrange.       @D]@n\r\n");
      } else if (char_get_hairc(tch) == HAIRC_GREEN) {
        send_to_char(ch, "            @D[@cWing Color  @D: @WGreen.        @D]@n\r\n");
      } else if (char_get_hairc(tch) == HAIRC_BLUE) {
        send_to_char(ch, "            @D[@cWing Color  @D: @WBlue.         @D]@n\r\n");
      } else if (char_get_hairc(tch) == HAIRC_PINK) {
        send_to_char(ch, "            @D[@cWing Color  @D: @WPink.         @D]@n\r\n");
      } else if (char_get_hairc(tch) == HAIRC_PURPLE) {
        send_to_char(ch, "            @D[@cWing Color  @D: @WPurple.       @D]@n\r\n");
      } else if (char_get_hairc(tch) == HAIRC_SILVER) {
        send_to_char(ch, "            @D[@cWing Color  @D: @WSilver.       @D]@n\r\n");
      } else if (char_get_hairc(tch) == HAIRC_CRIMSON) {
        send_to_char(ch, "            @D[@cWing Color  @D: @WCrimson.      @D]@n\r\n");
      } else if (char_get_hairc(tch) == HAIRC_WHITE) {
        send_to_char(ch, "            @D[@cWing Color  @D: @WWhite.        @D]@n\r\n");
      }
    } else if (char_is_arlian(tch) && !char_is_female(tch)) {
      send_to_char(ch, "            @D[@cWing Color  @D: @WWhite.        @D]@n\r\n");
    }
    if (char_is_majin(tch)) {
      if (char_get_hairl(tch) == HAIRL_BALD) {
        send_to_char(ch, "            @D[@cFor. Length @D: @WTiny.         @D]@n\r\n");
      }
      if (char_get_hairl(tch) == HAIRL_SHORT) {
        send_to_char(ch, "            @D[@cFor. Length @D: @WShort.        @D]@n\r\n");
      }
      if (char_get_hairl(tch) == HAIRL_MEDIUM) {
        send_to_char(ch, "            @D[@cFor. Length @D: @WMedium.       @D]@n\r\n");
      }
      if (char_get_hairl(tch) == HAIRL_LONG) {
        send_to_char(ch, "            @D[@cFor. Length @D: @WLong.         @D]@n\r\n");
      }
      if (char_get_hairl(tch) == HAIRL_RLONG) {
        send_to_char(ch, "            @D[@cFor. Length @D: @WR. Long.      @D]@n\r\n");
      }
    }
    if ((!char_is_saiyan(tch) && !char_is_halfbreed(tch)) || ((char_is_saiyan(tch) || char_is_halfbreed(tch)) && !char_is_transformed(tch))) {
      if (char_get_eye(tch) == EYE_BLUE) {
        send_to_char(ch, "            @D[@cEye Color   @D: @WBlue.         @D]@n\r\n");
      } else if (char_get_eye(tch) == EYE_BLACK) {
        send_to_char(ch, "            @D[@cEye Color   @D: @WBlack.        @D]@n\r\n");
      } else if (char_get_eye(tch) == EYE_GREEN) {
        send_to_char(ch, "            @D[@cEye Color   @D: @WGreen.        @D]@n\r\n");
      } else if (char_get_eye(tch) == EYE_BROWN) {
        send_to_char(ch, "            @D[@cEye Color   @D: @WBrown.        @D]@n\r\n");
      } else if (char_get_eye(tch) == EYE_RED) {
        send_to_char(ch, "            @D[@cEye Color   @D: @WRed.          @D]@n\r\n");
      } else if (char_get_eye(tch) == EYE_AQUA) {
        send_to_char(ch, "            @D[@cEye Color   @D: @WAqua.         @D]@n\r\n");
      } else if (char_get_eye(tch) == EYE_PINK) {
        send_to_char(ch, "            @D[@cEye Color   @D: @WPink.         @D]@n\r\n");
      } else if (char_get_eye(tch) == EYE_PURPLE) {
        send_to_char(ch, "            @D[@cEye Color   @D: @WPurple.       @D]@n\r\n");
      } else if (char_get_eye(tch) == EYE_CRIMSON) {
        send_to_char(ch, "            @D[@cEye Color   @D: @WCrimson.      @D]@n\r\n");
      } else if (char_get_eye(tch) == EYE_GOLD) {
        send_to_char(ch, "            @D[@cEye Color   @D: @WGold.         @D]@n\r\n");
      } else if (char_get_eye(tch) == EYE_AMBER) {
        send_to_char(ch, "            @D[@cEye Color   @D: @WAmber.        @D]@n\r\n");
      } else if (char_get_eye(tch) == EYE_EMERALD) {
        send_to_char(ch, "            @D[@cEye Color   @D: @WEmerald.      @D]@n\r\n");
      }
    }
    if (char_get_skin(tch) == SKIN_WHITE) {
      send_to_char(ch, "            @D[@cSkin Color  @D: @WWhite.        @D]@n\r\n");
    } else if (char_get_skin(tch) == SKIN_TAN) {
      send_to_char(ch, "            @D[@cSkin Color  @D: @WTan.          @D]@n\r\n");
    } else if (char_get_skin(tch) == SKIN_BLACK) {
      send_to_char(ch, "            @D[@cSkin Color  @D: @WBlack.        @D]@n\r\n");
    } else if (char_get_skin(tch) == SKIN_GREEN) {
      send_to_char(ch, "            @D[@cSkin Color  @D: @WGreen.        @D]@n\r\n");
    } else if (char_get_skin(tch) == SKIN_ORANGE) {
      send_to_char(ch, "            @D[@cSkin Color  @D: @WOrange.       @D]@n\r\n");
    } else if (char_get_skin(tch) == SKIN_YELLOW) {
      send_to_char(ch, "            @D[@cSkin Color  @D: @WYellow.       @D]@n\r\n");
    } else if (char_get_skin(tch) == SKIN_RED) {
      send_to_char(ch, "            @D[@cSkin Color  @D: @WRed.          @D]@n\r\n");
    } else if (char_get_skin(tch) == SKIN_GREY) {
      send_to_char(ch, "            @D[@cSkin Color  @D: @WGrey.         @D]@n\r\n");
    } else if (char_get_skin(tch) == SKIN_BLUE) {
      send_to_char(ch, "            @D[@cSkin Color  @D: @WBlue.         @D]@n\r\n");
    } else if (char_get_skin(tch) == SKIN_AQUA) {
      send_to_char(ch, "            @D[@cSkin Color  @D: @WAqua.         @D]@n\r\n");
    } else if (char_get_skin(tch) == SKIN_PINK) {
      send_to_char(ch, "            @D[@cSkin Color  @D: @WPink.         @D]@n\r\n");
    } else if (char_get_skin(tch) == SKIN_PURPLE) {
      send_to_char(ch, "            @D[@cSkin Color  @D: @WPurple.       @D]@n\r\n");
    }
    if (char_get_majinize(tch) != 0 && char_get_majinize(tch) != 3) {
      send_to_char(ch, "            @D[@cForehead    @D: @mMajin Symbol  @D]@n\r\n");
    }
  } else if (!char_is_humanoid(tch)) {
    /* Display nothing */
    return;
  } else {
    send_to_char(ch, "Error in bring-desc, please report.\r\n");
  }
}

static void do_auto_exits(room_rnum target_room, struct char_data *ch, int exit_mode)
{
  int door, door_found = 0, has_light = FALSE, i;
  char dlist1[500];
  char dlist2[500];
  char dlist3[500];
  char dlist4[500];
  char dlist5[500];
  char dlist6[500];
  char dlist7[500];
  char dlist8[500];
  char dlist9[500];
  char dlist10[500];
  char dlist11[500];
  char dlist12[500];

  *dlist1  = '\0';
  *dlist2  = '\0';
  *dlist3  = '\0';
  *dlist4  = '\0';
  *dlist5  = '\0';
  *dlist6  = '\0';
  *dlist7  = '\0';
  *dlist8  = '\0';
  *dlist9  = '\0';
  *dlist10 = '\0';
  *dlist11 = '\0';
  *dlist12 = '\0';

  if (exit_mode == EXIT_OFF) {
    send_to_char(ch, "@D------------------------------------------------------------------------@n\r\n");
  }
  int space = FALSE;
  if (rroom_sect(target_room) == SECT_SPACE && rroom_get_vnum(target_room) >= 20000) {
    space = TRUE;
  }
  if (exit_mode == EXIT_NORMAL && space == FALSE && char_get_in_room(ch) == target_room) {
    /* Compass and Auto-map - Iovan 9-11-10 */
    send_to_char(ch, "@D------------------------------------------------------------------------@n\r\n");
    send_to_char(ch, "@w      Compass           Auto-Map            Map Key\r\n");
    send_to_char(ch, "@R     ---------         ----------   -----------------------------\r\n");
    gen_map(ch, 0);
    send_to_char(ch, "@D------------------------------------------------------------------------@n\r\n");
  }
  if (exit_mode == EXIT_NORMAL && space == TRUE) {
    /* printmap */
    send_to_char(ch, "@D------------------------------[@CRadar@D]---------------------------------@n\r\n");
    printmap(target_room, ch, 1, -1);
    send_to_char(ch, "     @D[@wTurn autoexit complete on for directions instead of radar@D]@n\r\n");
    send_to_char(ch, "@D------------------------------------------------------------------------@n\r\n");
  }
  if (exit_mode == EXIT_COMPLETE || (exit_mode == EXIT_NORMAL && space == FALSE && char_get_in_room(ch) != target_room)) {
    send_to_char(ch, "@D----------------------------[@gObvious Exits@D]-----------------------------@n\r\n");
    if (char_is_affected(ch, AFF_BLIND)) {
      send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
      return;
    }
    if (char_plr_flagged(ch, PLR_EYEC)) {
      send_to_char(ch, "You can't see a damned thing, your eyes are closed!\r\n");
      return;
    }
    if (room_is_dark(char_get_in_room(ch)) && !char_can_see_in_dark(ch) && !char_plr_flagged(ch, PLR_AURALIGHT)) {
      send_to_char(ch, "It is pitch black...\r\n");
      return;
    }

    /* Is the character using a working light source? */
    for (i = 0; i < NUM_WEARS; i++)
      if (char_get_equipment(ch, i))
        if (obj_get_type_flag(char_get_equipment(ch, i)) == ITEM_LIGHT)
          if (obj_get_value(char_get_equipment(ch, i), VAL_LIGHT_HOURS))
            has_light = TRUE;

    if (char_plr_flagged(ch, PLR_AURALIGHT)) {
      has_light = TRUE;
    }

    for (door = 0; door < NUM_OF_DIRS; door++) {
      if (rroom_get_dir_option(target_room, door) &&
          rroom_get_dir_option(target_room, door)->to_room != NOWHERE) {
        /* We have a door that leads somewhere */
        if (char_adm_flagged(ch, ADM_SEESECRET) || char_get_admlevel(ch) > 4) {
          /* Immortals see everything */
          door_found++;
          char blam[9];
          sprintf(blam, "%s", dirs[door]);
          *blam = toupper(*blam);
          if (door == 6) {
            sprintf(dlist1, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
            if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
              /* This exit has a door - tell all about it */
              char argh[100];
              if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
                send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION NORTHWEST@n\r\n");
                mud_log("ERROR: %s found error direction NORTHWEST at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
                return;
              }
              sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
              sprintf(dlist1 + strlen(dlist1), "                    The %s%s %s %s %s%s.\r\n",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ? "@rsecret@w " : "",
                      (rroom_get_dir_option(target_room, door)->keyword &&
                       strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined"))
                          ? fname(rroom_get_dir_option(target_room, door)->keyword)
                          : "opening",
                      strstr(argh, "s ") != NULL ? "are" : "is",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ? "closed" : "open",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ? "and locked" : "and unlocked",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
            }
          }
          if (door == 0) {
            sprintf(dlist2, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
            if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
              /* This exit has a door - tell all about it */
              if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
                send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION NORTH@n\r\n");
                mud_log("ERROR: %s found error direction NORTH at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
                return;
              }
              char argh[200];
              sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
              sprintf(dlist2 + strlen(dlist2), "                    The %s%s %s %s %s%s.\r\n",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ? "@rsecret@w " : "",
                      (rroom_get_dir_option(target_room, door)->keyword &&
                       strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined"))
                          ? fname(rroom_get_dir_option(target_room, door)->keyword)
                          : "opening",
                      strstr(argh, "s ") != NULL ? "are" : "is",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ? "closed" : "open",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ? "and locked" : "and unlocked",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
            }
          }
          if (door == 7) {
            sprintf(dlist3, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
            if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
              /* This exit has a door - tell all about it */
              if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
                send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION NORTHEAST@n\r\n");
                mud_log("ERROR: %s found error direction NORTHEAST at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
                return;
              }
              char argh[100];
              sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
              sprintf(dlist3 + strlen(dlist3), "                    The %s%s %s %s %s%s.\r\n",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ? "@rsecret@w " : "",
                      (rroom_get_dir_option(target_room, door)->keyword &&
                       strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined"))
                          ? fname(rroom_get_dir_option(target_room, door)->keyword)
                          : "opening",
                      strstr(argh, "s ") != NULL ? "are" : "is",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ? "closed" : "open",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ? "and locked" : "and unlocked",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
            }
          }
          if (door == 1) {
            sprintf(dlist4, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
            if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
              /* This exit has a door - tell all about it */
              if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
                send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION EAST@n\r\n");
                mud_log("ERROR: %s found error direction EAST at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
                return;
              }
              char argh[100];
              sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
              sprintf(dlist4 + strlen(dlist4), "                    The %s%s %s %s %s%s.\r\n",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ? "@rsecret@w " : "",
                      (rroom_get_dir_option(target_room, door)->keyword &&
                       strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined"))
                          ? fname(rroom_get_dir_option(target_room, door)->keyword)
                          : "opening",
                      strstr(argh, "s ") != NULL ? "are" : "is",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ? "closed" : "open",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ? "and locked" : "and unlocked",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
            }
          }
          if (door == 8) {
            sprintf(dlist5, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
            if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
              /* This exit has a door - tell all about it */
              if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
                send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION SOUTHEAST@n\r\n");
                mud_log("ERROR: %s found error direction SOUTHEAST at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
                return;
              }
              char argh[100];
              sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
              sprintf(dlist5 + strlen(dlist5), "                    The %s%s %s %s %s%s.\r\n",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ? "@rsecret@w " : "",
                      (rroom_get_dir_option(target_room, door)->keyword &&
                       strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined"))
                          ? fname(rroom_get_dir_option(target_room, door)->keyword)
                          : "opening",
                      strstr(argh, "s ") != NULL ? "are" : "is",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ? "closed" : "open",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ? "and locked" : "and unlocked",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
            }
          }
          if (door == 2) {
            sprintf(dlist6, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
            if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
              /* This exit has a door - tell all about it */
              if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
                send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION SOUTH@n\r\n");
                mud_log("ERROR: %s found error direction SOUTH at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
                return;
              }
              char argh[100];
              sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
              sprintf(dlist6 + strlen(dlist6), "                    The %s%s %s %s %s%s.\r\n",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ? "@rsecret@w " : "",
                      (rroom_get_dir_option(target_room, door)->keyword &&
                       strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined"))
                          ? fname(rroom_get_dir_option(target_room, door)->keyword)
                          : "opening",
                      strstr(argh, "s ") != NULL ? "are" : "is",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ? "closed" : "open",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ? "and locked" : "and unlocked",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
            }
          }
          if (door == 9) {
            sprintf(dlist7, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
            if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
              /* This exit has a door - tell all about it */
              if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
                send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION SOUTHWEST@n\r\n");
                mud_log("ERROR: %s found error direction SOUTHWEST at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
                return;
              }
              char argh[100];
              sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
              sprintf(dlist7 + strlen(dlist7), "                    The %s%s %s %s %s%s.\r\n",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ? "@rsecret@w " : "",
                      (rroom_get_dir_option(target_room, door)->keyword &&
                       strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined"))
                          ? fname(rroom_get_dir_option(target_room, door)->keyword)
                          : "opening",
                      strstr(argh, "s ") != NULL ? "are" : "is",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ? "closed" : "open",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ? "and locked" : "and unlocked",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
            }
          }
          if (door == 3) {
            sprintf(dlist8, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
            if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
              /* This exit has a door - tell all about it */
              if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
                send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION WEST@n\r\n");
                mud_log("ERROR: %s found error direction WEST at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
                return;
              }
              char argh[100];
              sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
              sprintf(dlist8 + strlen(dlist8), "                    The %s%s %s %s %s%s.\r\n",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ? "@rsecret@w " : "",
                      (rroom_get_dir_option(target_room, door)->keyword &&
                       strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined"))
                          ? fname(rroom_get_dir_option(target_room, door)->keyword)
                          : "opening",
                      strstr(argh, "s ") != NULL ? "are" : "is",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ? "closed" : "open",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ? "and locked" : "and unlocked",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
            }
          }
          if (door == 4) {
            sprintf(dlist9, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
            if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
              /* This exit has a door - tell all about it */
              if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
                send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION UP@n\r\n");
                mud_log("ERROR: %s found error direction UP at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
                return;
              }
              char argh[100];
              sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
              sprintf(dlist9 + strlen(dlist9), "                    The %s%s %s %s %s%s.\r\n",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ? "@rsecret@w " : "",
                      (rroom_get_dir_option(target_room, door)->keyword &&
                       strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined"))
                          ? fname(rroom_get_dir_option(target_room, door)->keyword)
                          : "opening",
                      strstr(argh, "s ") != NULL ? "are" : "is",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ? "closed" : "open",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ? "and locked" : "and unlocked",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
            }
          }
          if (door == 5) {
            sprintf(dlist10, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
            if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
              /* This exit has a door - tell all about it */
              if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
                send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION DOWN@n\r\n");
                mud_log("ERROR: %s found error direction DOWN at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
                return;
              }
              char argh[100];
              sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
              sprintf(dlist10 + strlen(dlist10), "                    The %s%s %s %s %s%s.\r\n",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ? "@rsecret@w " : "",
                      (rroom_get_dir_option(target_room, door)->keyword &&
                       strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined"))
                          ? fname(rroom_get_dir_option(target_room, door)->keyword)
                          : "opening",
                      strstr(argh, "s ") != NULL ? "are" : "is",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ? "closed" : "open",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ? "and locked" : "and unlocked",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
            }
          }
          if (door == 10) {
            sprintf(dlist11, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
            if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
              /* This exit has a door - tell all about it */
              if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
                send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION INSIDE@n\r\n");
                mud_log("ERROR: %s found error direction INSIDE at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
                return;
              }
              char argh[100];
              sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
              sprintf(dlist11 + strlen(dlist11), "                    The %s%s %s %s %s%s.\r\n",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ? "@rsecret@w " : "",
                      (rroom_get_dir_option(target_room, door)->keyword &&
                       strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined"))
                          ? fname(rroom_get_dir_option(target_room, door)->keyword)
                          : "opening",
                      strstr(argh, "s ") != NULL ? "are" : "is",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ? "closed" : "open",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ? "and locked" : "and unlocked",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
            }
          }
          if (door == 11) {
            sprintf(dlist12, "@c%-9s @D- [@Y%5d@D]@w %s.\r\n", blam, world[rroom_get_dir_option(target_room, door)->to_room].number, world[rroom_get_dir_option(target_room, door)->to_room].name);
            if (IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_ISDOOR) ||
                IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
              /* This exit has a door - tell all about it */
              if (fname(rroom_get_dir_option(target_room, door)->keyword) == NULL) {
                send_to_char(ch, "@RREPORT THIS ERROR IMMEADIATLY FOR DIRECTION OUTSIDE@n\r\n");
                mud_log("ERROR: %s found error direction OUTSIDE at room %d", char_get_name(ch), rroom_get_vnum(char_get_in_room(ch)));
                return;
              }
              char argh[100];
              sprintf(argh, "%s ", strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined") ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
              sprintf(dlist12 + strlen(dlist12), "                    The %s%s %s %s %s%s.\r\n",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET) ? "@rsecret@w " : "",
                      (rroom_get_dir_option(target_room, door)->keyword &&
                       strcasecmp(fname(rroom_get_dir_option(target_room, door)->keyword), "undefined"))
                          ? fname(rroom_get_dir_option(target_room, door)->keyword)
                          : "opening",
                      strstr(argh, "s ") != NULL ? "are" : "is",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED) ? "closed" : "open",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_LOCKED) ? "and locked" : "and unlocked",
                      IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_PICKPROOF) ? " (pickproof)" : "");
            }
          }
        } else { /* This is what mortal characters see */
          if (!IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_CLOSED)) {
            /* And the door is open */
            door_found++;
            char blam[9];
            sprintf(blam, "%s", dirs[door]);
            *blam = toupper(*blam);
            if (door == 6) {
              sprintf(dlist1, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
            }
            if (door == 0) {
              sprintf(dlist2, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
            }
            if (door == 7) {
              sprintf(dlist3, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
            }
            if (door == 1) {
              sprintf(dlist4, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
            }
            if (door == 8) {
              sprintf(dlist5, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
            }
            if (door == 2) {
              sprintf(dlist6, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
            }
            if (door == 9) {
              sprintf(dlist7, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
            }
            if (door == 3) {
              sprintf(dlist8, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
            }
            if (door == 4) {
              sprintf(dlist9, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
            }
            if (door == 5) {
              sprintf(dlist10, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
            }
            if (door == 10) {
              sprintf(dlist11, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
            }
            if (door == 11) {
              sprintf(dlist12, "@c%-9s @D-@w %s\r\n", blam, room_is_dark(rroom_get_dir_option(target_room, door)->to_room) && !char_can_see_in_dark(ch) && !has_light ? "@bToo dark to tell.@w" : world[rroom_get_dir_option(target_room, door)->to_room].name);
            }

          } else if (CONFIG_DISP_CLOSED_DOORS &&
                     !IS_SET(rroom_get_dir_option(target_room, door)->exit_info, EX_SECRET)) {
            /* But we tell them the door is closed */
            door_found++;
            char blam[9];
            sprintf(blam, "%s", dirs[door]);
            *blam = toupper(*blam);
            if (door == 6) {
              sprintf(dlist1, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            }
            if (door == 0) {
              sprintf(dlist2, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            }
            if (door == 7) {
              sprintf(dlist3, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            }
            if (door == 1) {
              sprintf(dlist4, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            }
            if (door == 8) {
              sprintf(dlist5, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            }
            if (door == 2) {
              sprintf(dlist6, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            }
            if (door == 9) {
              sprintf(dlist7, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            }
            if (door == 3) {
              sprintf(dlist8, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            }
            if (door == 4) {
              sprintf(dlist9, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            }
            if (door == 5) {
              sprintf(dlist10, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            }
            if (door == 10) {
              sprintf(dlist11, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            }
            if (door == 11) {
              sprintf(dlist12, "@c%-9s @D-@w The %s appears @rclosed.@n\r\n", blam, (rroom_get_dir_option(target_room, door)->keyword) ? fname(rroom_get_dir_option(target_room, door)->keyword) : "opening");
            }
          }
        }
      }
    }
    if (!door_found)
      send_to_char(ch, " None.\r\n");
    if (strstr(dlist1, "Northwest")) {
      send_to_char(ch, "%s", dlist1);
      *dlist1 = '\0';
    }
    if (strstr(dlist2, "North")) {
      send_to_char(ch, "%s", dlist2);
      *dlist2 = '\0';
    }
    if (strstr(dlist3, "Northeast")) {
      send_to_char(ch, "%s", dlist3);
      *dlist3 = '\0';
    }
    if (strstr(dlist4, "East")) {
      send_to_char(ch, "%s", dlist4);
      *dlist4 = '\0';
    }
    if (strstr(dlist5, "Southeast")) {
      send_to_char(ch, "%s", dlist5);
      *dlist5 = '\0';
    }
    if (strstr(dlist6, "South")) {
      send_to_char(ch, "%s", dlist6);
      *dlist6 = '\0';
    }
    if (strstr(dlist7, "Southwest")) {
      send_to_char(ch, "%s", dlist7);
      *dlist7 = '\0';
    }
    if (strstr(dlist8, "West")) {
      send_to_char(ch, "%s", dlist8);
      *dlist8 = '\0';
    }
    if (strstr(dlist9, "Up")) {
      send_to_char(ch, "%s", dlist9);
      *dlist9 = '\0';
    }
    if (strstr(dlist10, "Down")) {
      send_to_char(ch, "%s", dlist10);
      *dlist10 = '\0';
    }
    if (strstr(dlist11, "Inside")) {
      send_to_char(ch, "%s", dlist11);
      *dlist11 = '\0';
    }
    if (strstr(dlist12, "Outside")) {
      send_to_char(ch, "%s", dlist12);
      *dlist12 = '\0';
    }
    send_to_char(ch, "@D------------------------------------------------------------------------@n\r\n");
    if (rroom_flagged(target_room, ROOM_HOUSE) && !rroom_flagged(target_room, ROOM_GARDEN1) && !rroom_flagged(target_room, ROOM_GARDEN2)) {
      send_to_char(ch, "@D[@GItems Stored@D: @g%d@D]@n\r\n", check_saveroom_count(ch, NULL));
    }
    if (rroom_flagged(target_room, ROOM_HOUSE) && rroom_flagged(target_room, ROOM_GARDEN1) && !rroom_flagged(target_room, ROOM_GARDEN2)) {
      send_to_char(ch, "@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R8@D]@n\r\n", check_saveroom_count(ch, NULL));
    }
    if (rroom_flagged(target_room, ROOM_HOUSE) && !rroom_flagged(target_room, ROOM_GARDEN1) && rroom_flagged(target_room, ROOM_GARDEN2)) {
      send_to_char(ch, "@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R20@D]@n\r\n", check_saveroom_count(ch, NULL));
    }
    if (char_get_radar1(ch) == rroom_get_vnum(target_room) && char_get_radar2(ch) == rroom_get_vnum(target_room) && char_get_radar3(ch) != rroom_get_vnum(target_room)) {
      send_to_char(ch, "@CTwo of your buoys are floating here.@n\r\n");
    } else if (char_get_radar1(ch) == rroom_get_vnum(target_room) && char_get_radar2(ch) != rroom_get_vnum(target_room) && char_get_radar3(ch) == rroom_get_vnum(target_room)) {
      send_to_char(ch, "@CTwo of your buoys are floating here.@n\r\n");
    } else if (char_get_radar1(ch) != rroom_get_vnum(target_room) && char_get_radar2(ch) == rroom_get_vnum(target_room) && char_get_radar3(ch) == rroom_get_vnum(target_room)) {
      send_to_char(ch, "@CTwo of your buoys are floating here.@n\r\n");
    } else if (char_get_radar1(ch) == rroom_get_vnum(target_room) && char_get_radar2(ch) == rroom_get_vnum(target_room) && char_get_radar3(ch) == rroom_get_vnum(target_room) && target_room != 0) {
      send_to_char(ch, "@CAll three of your buoys are floating here. Why?@n\r\n");
    } else if (char_get_radar1(ch) == rroom_get_vnum(target_room)) {
      send_to_char(ch, "@CYour @cBuoy #1@C is floating here.@n\r\n");
    } else if (char_get_radar2(ch) == rroom_get_vnum(target_room)) {
      send_to_char(ch, "@CYour @cBuoy #2@C is floating here.@n\r\n");
    } else if (char_get_radar3(ch) == rroom_get_vnum(target_room)) {
      send_to_char(ch, "@CYour @cBuoy #3@C is floating here.@n\r\n");
    }
  }
}

static void do_auto_exits2(room_rnum target_room, struct char_data *ch)
{
  int door, slen = 0;

  send_to_char(ch, "\nExits: ");

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (!rroom_get_dir_option(target_room, door) || rroom_get_dir_option(target_room, door)->to_room == NOWHERE)
      continue;
    if (room_exit_flagged(rroom_get_dir_option(target_room, door), EX_CLOSED))
      continue;

    send_to_char(ch, "%s ", abbr_dirs[door]);
    slen++;
  }

  send_to_char(ch, "%s\r\n", slen ? "" : "None!");
}

ACMD(do_exits)
{
  /* Why duplicate code? */
  if (!char_prf_flagged(ch, PRF_NODEC)) {
    do_auto_exits(char_get_in_room(ch), ch, EXIT_COMPLETE);
  } else {
    do_auto_exits2(char_get_in_room(ch), ch);
  }
}

static const char *exitlevels[] = {
    "off", "normal", "n/a", "complete", "\n"};

ACMD(do_autoexit)
{
  char arg[MAX_INPUT_LENGTH];
  int tp;

  if (char_is_npc(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Your current autoexit level is %s.\r\n", exitlevels[char_exit_level(ch)]);
    return;
  }
  if (((tp = search_block(arg, exitlevels, FALSE)) == -1)) {
    send_to_char(ch, "Usage: Autoexit { Off | Normal | Complete }\r\n");
    return;
  }
  switch (tp) {
    case EXIT_OFF:
      char_prf_set(ch, PRF_AUTOEXIT, false);
      char_prf_set(ch, PRF_FULL_EXIT, false);
      break;
    case EXIT_NORMAL:
      char_prf_set(ch, PRF_AUTOEXIT, true);
      char_prf_set(ch, PRF_FULL_EXIT, false);
      break;
    case EXIT_COMPLETE:
      char_prf_set(ch, PRF_AUTOEXIT, true);
      char_prf_set(ch, PRF_FULL_EXIT, true);
      break;
  }
  send_to_char(ch, "Your @rautoexit level@n is now %s.\r\n", exitlevels[char_exit_level(ch)]);
}

void look_at_room(room_rnum target_room, struct char_data *ch, int ignore_brief)
{
  struct room_data *rm = &world[char_get_in_room(ch)];
  trig_data *t;

  if (!ch->desc)
    return;

  if (room_is_dark(target_room) && !char_can_see_in_dark(ch) && !char_plr_flagged(ch, PLR_AURALIGHT)) {
    send_to_char(ch, "It is pitch black...\r\n");
    return;
  } else if (char_aff_flagged(ch, AFF_BLIND)) {
    send_to_char(ch, "You see nothing but infinite darkness...\r\n");
    return;
  } else if (char_plr_flagged(ch, PLR_EYEC)) {
    send_to_char(ch, "You can't see a damned thing, your eyes are closed!\r\n");
    return;
  }
  if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_ROOMFLAGS)) {
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char buf3[MAX_STRING_LENGTH];

    sprintbitarray(rroom_get_room_flags(target_room), room_bits, RF_ARRAY_MAX, buf, sizeof(buf));
    sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
    if (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_NODEC)) {
      send_to_char(ch, "\r\n@wO----------------------------------------------------------------------O@n\r\n");
    }

    send_to_char(ch, "@wLocation: @G%-70s@w\r\n", world[target_room].name);
    if (SCRIPT(rm)) {
      send_to_char(ch, "@D[@GTriggers");
      for (t = TRIGGERS(SCRIPT(rm)); t; t = t->next)
        send_to_char(ch, " %d", GET_TRIG_VNUM(t));
      send_to_char(ch, "@D] ");
    }
    snprintf(buf3, sizeof(buf3), "@D[ @G%s@D] @wSector: @D[ @G%s @D] @wVnum: @D[@G%5d@D]@n", buf, buf2, rroom_get_vnum(target_room));
    send_to_char(ch, "@wFlags: %-70s@w\r\n", buf3);
    if (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_NODEC)) {
      send_to_char(ch, "@wO----------------------------------------------------------------------O@n\r\n");
    }
  } else {
    if (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_NODEC)) {
      send_to_char(ch, "@wO----------------------------------------------------------------------O@n\r\n");
    }
    send_to_char(ch, "@wLocation: %-70s@n\r\n", world[target_room].name);
    if (rroom_flagged(target_room, ROOM_EARTH)) {
      send_to_char(ch, "@wPlanet: @GEarth@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_CERRIA)) {
      send_to_char(ch, "@wPlanet: @RCerria@n\r\n");
    } else if (rroom_planet_zenith(target_room)) {
      send_to_char(ch, "@wPlanet: @BZenith@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_AETHER)) {
      send_to_char(ch, "@wPlanet: @MAether@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_FRIGID)) {
      send_to_char(ch, "@wPlanet: @CFrigid@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_SPACE)) {
      send_to_char(ch, "@wPlanet: @DNone@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_VEGETA)) {
      send_to_char(ch, "@wPlanet: @YVegeta@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_NAMEK)) {
      send_to_char(ch, "@wPlanet: @gNamek@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_KONACK)) {
      send_to_char(ch, "@wPlanet: @MKonack@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_NEO)) {
      send_to_char(ch, "@wPlanet: @WNeo Nirvana@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_AL)) {
      send_to_char(ch, "@wDimension: @yA@Yf@yt@Ye@yr@Yl@yi@Yf@ye@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_HELL)) {
      send_to_char(ch, "@wDimension: @RPunishment Hell@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_RHELL)) {
      send_to_char(ch, "@wDimension: @RH@re@Dl@Rl@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_YARDRAT)) {
      send_to_char(ch, "@wPlanet: @mYardrat@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_KANASSA)) {
      send_to_char(ch, "@wPlanet: @BKanassa@n\r\n");
    } else if (rroom_flagged(target_room, ROOM_ARLIA)) {
      send_to_char(ch, "@wPlanet: @GArlia@n\r\n");
    }

    else {
      send_to_char(ch, "@wPlanet: @WUNKNOWN@n\r\n");
    }
    if (rroom_get_gravity(target_room) <= 0) {
      send_to_char(ch, "@wGravity: @WNormal@n\r\n");
    } else if (rroom_get_gravity(target_room) == 10) {
      send_to_char(ch, "@wGravity: @W10x@n\r\n");
    } else if (rroom_get_gravity(target_room) == 20) {
      send_to_char(ch, "@wGravity: @W20x@n\r\n");
    } else if (rroom_get_gravity(target_room) == 30) {
      send_to_char(ch, "@wGravity: @W30x@n\r\n");
    } else if (rroom_get_gravity(target_room) == 40) {
      send_to_char(ch, "@wGravity: @W40x@n\r\n");
    } else if (rroom_get_gravity(target_room) == 50) {
      send_to_char(ch, "@wGravity: @W50x@n\r\n");
    } else if (rroom_get_gravity(target_room) == 100) {
      send_to_char(ch, "@wGravity: @W100x@n\r\n");
    } else if (rroom_get_gravity(target_room) == 200) {
      send_to_char(ch, "@wGravity: @W200x@n\r\n");
    } else if (rroom_get_gravity(target_room) == 300) {
      send_to_char(ch, "@wGravity: @W300x@n\r\n");
    } else if (rroom_get_gravity(target_room) == 400) {
      send_to_char(ch, "@wGravity: @W400x@n\r\n");
    } else if (rroom_get_gravity(target_room) == 500) {
      send_to_char(ch, "@wGravity: @W500x@n\r\n");
    } else if (rroom_get_gravity(target_room) == 1000) {
      send_to_char(ch, "@wGravity: @W1,000x@n\r\n");
    } else if (rroom_get_gravity(target_room) == 5000) {
      send_to_char(ch, "@wGravity: @W5,000x@n\r\n");
    } else if (rroom_get_gravity(target_room) == 10000) {
      send_to_char(ch, "@wGravity: @W10,000x@n\r\n");
    }
    if (rroom_flagged(target_room, ROOM_REGEN)) {
      send_to_char(ch, "@CA feeling of calm and relaxation fills this room.@n\r\n");
    }
    if (rroom_flagged(target_room, ROOM_AURA)) {
      send_to_char(ch, "@GAn aura of @gregeneration@G surrounds this area.@n\r\n");
    }
    if (rroom_flagged(target_room, ROOM_HBTC)) {
      send_to_char(ch, "@rThis room feels like it opperates in a different time frame.@n\r\n");
    }
    if (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_NODEC)) {
      send_to_char(ch, "@wO----------------------------------------------------------------------O@n\r\n");
    }
  }

  if ((!char_is_npc(ch) && !char_prf_flagged(ch, PRF_BRIEF)) || rroom_flagged(target_room, ROOM_DEATH)) {
    if (rroom_get_dmg(target_room) <= 99) {
      send_to_char(ch, "@w%s@n", world[target_room].description);
    }
    if (rroom_get_dmg(target_room) == 100 && (rroom_sect(target_room) == SECT_WATER_SWIM || rroom_sunken(target_room) || rroom_sect(target_room) == SECT_FLYING || rroom_sect(target_room) == SECT_SHOP || rroom_sect(target_room) == SECT_IMPORTANT)) {
      send_to_char(ch, "@w%s@n", world[target_room].description);
    }
    if (rroom_sect(target_room) == SECT_INSIDE && rroom_get_dmg(target_room) > 0) {
      send_to_char(ch, "\r\n");
      if (rroom_get_dmg(target_room) <= 2) {
        send_to_char(ch, "@wA small hole with chunks of debris that can be seen scarring the floor.@n");
      } else if (rroom_get_dmg(target_room) <= 4) {
        send_to_char(ch, "@wA couple small holes with chunks of debris that can be seen scarring the floor.@n");
      } else if (rroom_get_dmg(target_room) <= 6) {
        send_to_char(ch, "@wA few small holes with chunks of debris that can be seen scarring the floor.@n");
      } else if (rroom_get_dmg(target_room) <= 10) {
        send_to_char(ch, "@wThere are several small holes with chunks of debris that can be seen scarring the floor.@n");
      } else if (rroom_get_dmg(target_room) <= 20) {
        send_to_char(ch, "@wMany holes fill the floor of this area, many of which have burn marks.@n");
      } else if (rroom_get_dmg(target_room) <= 30) {
        send_to_char(ch, "@wThe floor is severely damaged with many large holes.@n");
      } else if (rroom_get_dmg(target_room) <= 50) {
        send_to_char(ch, "@wBattle damage covers the entire area. Displayed as a tribute to the battles that have\r\nbeen waged here.@n");
      } else if (rroom_get_dmg(target_room) <= 75) {
        send_to_char(ch, "@wThis entire area is falling apart, it has been damaged so badly.@n");
      } else if (rroom_get_dmg(target_room) <= 99) {
        send_to_char(ch, "@wThis area can not withstand much more damage. Everything has been damaged so badly it\r\nis hard to recognise any particular details about their former quality.@n");
      } else if (rroom_get_dmg(target_room) >= 100) {
        send_to_char(ch, "@wThis area is completely destroyed. Nothing is recognisable. Chunks of debris\r\nlitter the ground, filling up holes, and overflowing onto what is left of the\r\nfloor. A haze of smoke is wafting through the air, creating a chilling atmosphere..@n");
      }
      send_to_char(ch, "\r\n");
    } else if ((rroom_sect(target_room) == SECT_CITY || rroom_sect(target_room) == SECT_FIELD || rroom_sect(target_room) == SECT_HILLS || rroom_sect(target_room) == SECT_IMPORTANT) && rroom_get_dmg(target_room) > 0) {
      send_to_char(ch, "\r\n");
      if (rroom_get_dmg(target_room) <= 2) {
        send_to_char(ch, "@wA small hole with chunks of debris that can be seen scarring the ground.@n");
      } else if (rroom_get_dmg(target_room) <= 4) {
        send_to_char(ch, "@wA couple small craters with chunks of debris that can be seen scarring the ground.@n");
      } else if (rroom_get_dmg(target_room) <= 6) {
        send_to_char(ch, "@wA few small craters with chunks of debris that can be seen scarring the ground.@n");
      } else if (rroom_get_dmg(target_room) <= 10) {
        send_to_char(ch, "@wThere are several small craters with chunks of debris that can be seen scarring the ground.@n");
      } else if (rroom_get_dmg(target_room) <= 20) {
        send_to_char(ch, "@wMany craters fill the ground of this area, many of which have burn marks.@n");
      } else if (rroom_get_dmg(target_room) <= 30) {
        send_to_char(ch, "@wThe ground is severely damaged with many large craters.@n");
      } else if (rroom_get_dmg(target_room) <= 50) {
        send_to_char(ch, "@wBattle damage covers the entire area. Displayed as a tribute to the battles that have\r\nbeen waged here.@n");
      } else if (rroom_get_dmg(target_room) <= 75) {
        send_to_char(ch, "@wThis entire area is falling apart, it has been damaged so badly.@n");
      } else if (rroom_get_dmg(target_room) <= 99) {
        send_to_char(ch, "@wThis area can not withstand much more damage. Everything has been damaged so badly it\r\nis hard to recognise any particular details about their former quality.@n");
      } else if (rroom_get_dmg(target_room) >= 100) {
        send_to_char(ch, "@wThis area is completely destroyed. Nothing is recognisable. Chunks of debris\r\nlitter the ground, filling up craters, and overflowing onto what is left of the\r\nground. A haze of smoke is wafting through the air, creating a chilling atmosphere..@n");
      }
      send_to_char(ch, "\r\n");
    } else if (rroom_sect(target_room) == SECT_FOREST && rroom_get_dmg(target_room) > 0) {
      send_to_char(ch, "\r\n");
      if (rroom_get_dmg(target_room) <= 2) {
        send_to_char(ch, "@wA small tree sits in a little crater here.@n");
      } else if (rroom_get_dmg(target_room) <= 4) {
        send_to_char(ch, "@wTrees have been uprooted by craters in the ground.@n");
      } else if (rroom_get_dmg(target_room) <= 6) {
        send_to_char(ch, "@wSeveral trees have been reduced to chunks of debris and are\r\nlaying in a few craters here. @n");
      } else if (rroom_get_dmg(target_room) <= 10) {
        send_to_char(ch, "@wA large patch of trees have been destroyed and are laying in craters here.@n");
      } else if (rroom_get_dmg(target_room) <= 20) {
        send_to_char(ch, "@wSeveral craters have merged into one large crater in one part of this forest.@n");
      } else if (rroom_get_dmg(target_room) <= 30) {
        send_to_char(ch, "@wThe open sky can easily be seen through a hole of trees destroyed\r\nand resting at the bottom of several craters here.@n");
      } else if (rroom_get_dmg(target_room) <= 50) {
        send_to_char(ch, "@wA good deal of burning tree pieces can be found strewn across the cratered ground here.@n");
      } else if (rroom_get_dmg(target_room) <= 75) {
        send_to_char(ch, "@wVery few trees are left standing in this area, replaced instead by large craters.@n");
      } else if (rroom_get_dmg(target_room) <= 99) {
        send_to_char(ch, "@wSingle solitary trees can be found still standing here or there in the area.\r\nThe rest have been almost completely obliterated in recent conflicts.@n");
      } else if (rroom_get_dmg(target_room) >= 100) {
        send_to_char(ch, "@w  One massive crater fills this area. This desolate crater leaves no\r\nevidence of what used to be found in the area. Smoke slowly wafts into\r\nthe sky from the central point of the crater, creating an oppressive\r\natmosphere.@n");
      }
      send_to_char(ch, "\r\n");
    } else if (rroom_sect(target_room) == SECT_MOUNTAIN && rroom_get_dmg(target_room) > 0) {
      send_to_char(ch, "\r\n");
      if (rroom_get_dmg(target_room) <= 2) {
        send_to_char(ch, "@wA small crater has been burned into the side of this mountain.@n");
      } else if (rroom_get_dmg(target_room) <= 4) {
        send_to_char(ch, "@wA couple craters have been burned into the side of this mountain.@n");
      } else if (rroom_get_dmg(target_room) <= 6) {
        send_to_char(ch, "@wBurned bits of boulders can be seen lying at the bottom of a few nearby craters.@n");
      } else if (rroom_get_dmg(target_room) <= 10) {
        send_to_char(ch, "@wSeveral bad craters can be seen in the side of the mountain here.@n");
      } else if (rroom_get_dmg(target_room) <= 20) {
        send_to_char(ch, "@wLarge boulders have rolled down the mountain side and collected in many nearby craters.@n");
      } else if (rroom_get_dmg(target_room) <= 30) {
        send_to_char(ch, "@wMany craters are covering the mountainside here.@n");
      } else if (rroom_get_dmg(target_room) <= 50) {
        send_to_char(ch, "@wThe mountain side has partially collapsed, shedding rubble down towards its base.@n");
      } else if (rroom_get_dmg(target_room) <= 75) {
        send_to_char(ch, "@wA peak of the mountain has been blown off, leaving behind a smoldering tip.@n");
      } else if (rroom_get_dmg(target_room) <= 99) {
        send_to_char(ch, "@wThe mountain side here has completely collapsed, shedding dangerous rubble down to its base.@n");
      } else if (rroom_get_dmg(target_room) >= 100) {
        send_to_char(ch, "@w  Half the mountain has been blown away, leaving a scarred and jagged\r\nrock in its place. Billowing smoke wafts up from several parts of the\r\nmountain, filling the nearby skies and blotting out the sun.@n");
      }
      send_to_char(ch, "\r\n");
    }
    if (rroom_get_geffect(target_room) >= 1 && rroom_get_geffect(target_room) <= 5) {
      send_to_char(ch, "@rLava@w is pooling in someplaces here...@n\r\n");
    }
    if (rroom_get_geffect(target_room) >= 6) {
      send_to_char(ch, "@RLava@r covers pretty much the entire area!@n\r\n");
    }
    if (rroom_get_geffect(target_room) < 0) {
      send_to_char(ch, "@cThe entire area is flooded with a @Cmystical@c cube of @Bwater!@n\r\n");
    }
  }
  /* autoexits */
  if (!char_is_npc(ch) && char_prf_flagged(ch, PRF_NODEC))
    do_auto_exits2(target_room, ch);
  if (!char_is_npc(ch) && !char_prf_flagged(ch, PRF_NODEC))
    do_auto_exits(target_room, ch, char_exit_level(ch));

  /* now list characters & objects */
  if (rroom_flagged(target_room, ROOM_GARDEN1)) {
    send_to_char(ch, "@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R8@D]@n\r\n", check_saveroom_count(ch, NULL));
  } else if (rroom_flagged(target_room, ROOM_GARDEN2)) {
    send_to_char(ch, "@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R20@D]@n\r\n", check_saveroom_count(ch, NULL));
  } else if (rroom_flagged(target_room, ROOM_HOUSE)) {
    send_to_char(ch, "@D[@GItems Stored@D: @g%d@D]@n\r\n", check_saveroom_count(ch, NULL));
  }
  list_obj_to_char(world[target_room].contents, ch, SHOW_OBJ_LONG, FALSE);
  list_char_to_char(world[target_room].people, ch);
}

static void look_in_direction(struct char_data *ch, int dir)
{
  if (char_get_exit(ch, dir)) {
    if (char_get_exit(ch, dir)->general_description)
      send_to_char(ch, "%s", char_get_exit(ch, dir)->general_description);
    else {
      struct obj_data *obj, *next_obj;
      int founded = FALSE;
      for (obj = ch->carrying; obj; obj = next_obj) {
        next_obj = obj->next_content;
        if (obj_get_vnum(obj) == 17) {
          founded = TRUE;
        }
      }
      if (founded == FALSE) {
        send_to_char(ch, "You were unable to discern anything about that direction. Try looking again...\r\n");
        struct obj_data *obj;
        obj = read_object(17, VIRTUAL);
        obj_to_char(obj, ch);
      }
    }

    if (room_exit_flagged(char_get_exit(ch, dir), EX_ISDOOR) && char_get_exit(ch, dir)->keyword) {
      if (!room_exit_flagged(char_get_exit(ch, dir), EX_SECRET) &&
          room_exit_flagged(char_get_exit(ch, dir), EX_CLOSED))
        send_to_char(ch, "The %s is closed.\r\n", fname(char_get_exit(ch, dir)->keyword));
      else if (!room_exit_flagged(char_get_exit(ch, dir), EX_CLOSED))
        send_to_char(ch, "The %s is open.\r\n", fname(char_get_exit(ch, dir)->keyword));
    }
  } else
    send_to_char(ch, "Nothing special there...\r\n");
}

static void look_in_obj(struct char_data *ch, char *arg)
{
  struct obj_data *obj    = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char(ch, "Look in what?\r\n");
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
  } else if (find_exdesc(arg, obj->ex_description) != NULL && !bits)
    send_to_char(ch, "There's nothing inside that!\r\n");
  else if ((obj_get_type_flag(obj) == ITEM_PORTAL) && !obj_val_flagged(obj, CONT_CLOSEABLE)) {
    if (obj_get_value(obj, VAL_PORTAL_APPEAR) < 0) {
      /* You can look through the portal to the destination */
      /* where does this lead to? */
      room_rnum portal_dest = real_room(obj_get_value(obj, VAL_PORTAL_DEST));
      if (portal_dest == NOWHERE) {
        send_to_char(ch, "You see nothing but infinite darkness...\r\n");
      } else if (room_is_dark(portal_dest) && !char_can_see_in_dark(ch) && !char_plr_flagged(ch, PLR_AURALIGHT)) {
        send_to_char(ch, "You see nothing but infinite darkness...\r\n");
      } else {
        send_to_char(ch, "After seconds of concentration you see the image of %s.\r\n", world[portal_dest].name);
      }
    } else if (obj_get_value(obj, VAL_PORTAL_APPEAR) < MAX_PORTAL_TYPES) {
      /* display the appropriate description from the list of descriptions
*/
      send_to_char(ch, "%s\r\n", portal_appearance[obj_get_value(obj, VAL_PORTAL_APPEAR)]);
    } else {
      /* We shouldn't really get here, so give a default message */
      send_to_char(ch, "All you can see is the glow of the portal.\r\n");
    }
  } else if (obj_get_type_flag(obj) == ITEM_VEHICLE) {
    if (obj_val_flagged(obj, CONT_CLOSED))
      send_to_char(ch, "It is closed.\r\n");
    else if (obj_get_value(obj, VAL_VEHICLE_APPEAR) < 0) {
      /* You can look inside the vehicle */
      /* where does this lead to? */
      room_rnum vehicle_inside = real_room(obj_get_value(obj, VAL_VEHICLE_ROOM));
      if (vehicle_inside == NOWHERE) {
        send_to_char(ch, "You cannot see inside that.\r\n");
      } else if (room_is_dark(vehicle_inside) && !char_can_see_in_dark(ch) && !char_plr_flagged(ch, PLR_AURALIGHT)) {
        send_to_char(ch, "It is pitch black...\r\n");
      } else {
        send_to_char(ch, "You look inside and see:\r\n");
        look_at_room(vehicle_inside, ch, 0);
      }
    } else {
      send_to_char(ch, "You cannot see inside that.\r\n");
    }
  } else if (obj_get_type_flag(obj) == ITEM_WINDOW) {
    look_out_window(ch, arg);
  } else if ((obj_get_type_flag(obj) != ITEM_DRINKCON) &&
             (obj_get_type_flag(obj) != ITEM_FOUNTAIN) &&
             (obj_get_type_flag(obj) != ITEM_CONTAINER) &&
             (obj_get_type_flag(obj) != ITEM_PORTAL)) {
    send_to_char(ch, "There's nothing inside that!\r\n");
  } else if ((obj_get_type_flag(obj) == ITEM_CONTAINER) ||
             (obj_get_type_flag(obj) == ITEM_PORTAL)) {
    if (obj_val_flagged(obj, CONT_CLOSED))
      send_to_char(ch, "It is closed.\r\n");
    else {
      send_to_char(ch, "%s", obj->short_description);
      if (obj_get_type_flag(obj) == ITEM_CONTAINER && (obj_get_vnum(obj) == 697 || obj_get_vnum(obj) == 698 || obj_get_vnum(obj) == 682 || obj_get_vnum(obj) == 683 || obj_get_vnum(obj) == 684)) {
        act("$n looks in $p.", TRUE, ch, obj, 0, TO_ROOM);
      }
      switch (bits) {
        case FIND_OBJ_INV:
          send_to_char(ch, " (carried): \r\n");
          break;
        case FIND_OBJ_ROOM:
          send_to_char(ch, " (here): \r\n");
          break;
        case FIND_OBJ_EQUIP:
          send_to_char(ch, " (used): \r\n");
          break;
      }

      list_obj_to_char(obj->contains, ch, SHOW_OBJ_SHORT, TRUE);
    }
  } else { /* item must be a fountain or drink container */
    if (obj_get_value(obj, VAL_DRINKCON_HOWFULL) <= 0 && (obj_get_value(obj, VAL_DRINKCON_CAPACITY) == 0))
      send_to_char(ch, "It is empty.\r\n");
    else {
      if (obj_get_value(obj, VAL_DRINKCON_CAPACITY) < 0) {
        char buf2[MAX_STRING_LENGTH];
        sprinttype(obj_get_value(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
        send_to_char(ch, "It's full of a %s liquid.\r\n", buf2);
      } else if (obj_get_value(obj, VAL_DRINKCON_HOWFULL) > obj_get_value(obj, VAL_DRINKCON_CAPACITY)) {
        send_to_char(ch, "Its contents seem somewhat murky.\r\n"); /* BUG */
      } else {
        char buf2[MAX_STRING_LENGTH];
        amt        = obj_get_value(obj, VAL_DRINKCON_CAPACITY);
        int leftin = obj_get_value(obj, VAL_DRINKCON_HOWFULL);
        sprinttype(obj_get_value(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
        if (leftin == amt) {
          send_to_char(ch, "It's full of a %s liquid.\r\n", buf2);
        } else if (leftin >= amt * .8) {
          send_to_char(ch, "It's almost full of a %s liquid.\r\n", buf2);
        } else if (leftin >= amt * .5) {
          send_to_char(ch, "It's about half full of a %s liquid.\r\n", buf2);
        } else if (leftin >= amt * .2) {
          send_to_char(ch, "It's less than half full of a %s liquid.\r\n", buf2);
        } else if (leftin > 0) {
          send_to_char(ch, "It's barely filled with a %s liquid.\r\n", buf2);
        } else {
          send_to_char(ch, "It's empty.\r\n");
        }
      }
    }
  }
}

char *find_exdesc(char *word, struct extra_descr_data *list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    /*if (isname(word, i->keyword))*/
    if (*i->keyword == '.' ? isname(word, i->keyword + 1) : isname(word, i->keyword))
      return (i->description);

  return (NULL);
}

/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */
static void look_at_target(struct char_data *ch, char *arg, int cmread)
{
  int bits, found = FALSE, j, fnum, i = 0, msg = 1;
  struct char_data *found_char = NULL;
  struct obj_data *obj, *found_obj = NULL;
  char *desc;
  char number[MAX_STRING_LENGTH];

  if (!ch->desc)
    return;

  if (!*arg) {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  if (cmread) {
    for (obj = ch->carrying; obj; obj = obj->next_content) {
      if (obj_get_type_flag(obj) == ITEM_BOARD) {
        found = TRUE;
        break;
      }
    }
    if (!obj) {
      for (obj = world[char_get_in_room(ch)].contents; obj; obj = obj->next_content) {
        if (obj_get_type_flag(obj) == ITEM_BOARD) {
          found = TRUE;
          break;
        }
      }
    }
    if (obj) {
      arg = one_argument(arg, number);
      if (!*number) {
        send_to_char(ch, "Read what?\r\n");
        return;
      }

      /* Okay, here i'm faced with the fact that the person could be
	 entering in something like 'read 5' or 'read 4.mail' .. so, whats the
	 difference between the two?  Well, there's a period in the second,
	 so, we'll just stick with that basic difference */

      if (isname(number, obj->name)) {
        show_board(obj_get_vnum(obj), ch);
      } else if ((!isdigit(*number) || (!(msg = atoi(number)))) ||
                 (strchr(number, '.'))) {
        char target[MAX_INPUT_LENGTH]; /* "<number> <arg>", built aside: arg may not overlap */
        snprintf(target, sizeof(target), "%s %s", number, arg);
        look_at_target(ch, target, 0);
      } else {
        board_display_msg(obj_get_vnum(obj), ch, msg);
      }
    }
  } else {
    bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &found_char, &found_obj);

    /* Is the target a character? */
    if (found_char != NULL) {
      look_at_char(found_char, ch);
      if (ch != found_char) {
        if (!char_aff_flagged(ch, AFF_HIDE)) {
          act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
          act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
        }
      }
      return;
    }

    /* Strip off "number." from 2.foo and friends. */
    if (!(fnum = get_number(&arg))) {
      send_to_char(ch, "Look at what?\r\n");
      return;
    }

    /* Does the argument match an extra desc in the room? */
    if ((desc = find_exdesc(arg, world[char_get_in_room(ch)].ex_description)) != NULL && ++i == fnum) {
      page_string(ch->desc, desc, FALSE);
      return;
    }

    /* Does the argument match an extra desc in the char's equipment? */
    for (j = 0; j < NUM_WEARS && !found; j++)
      if (char_get_equipment(ch, j) && char_can_see_obj(ch, char_get_equipment(ch, j)))
        if ((desc = find_exdesc(arg, char_get_equipment(ch, j)->ex_description)) != NULL && ++i == fnum) {
          send_to_char(ch, "%s", desc);
          if (isname(arg, char_get_equipment(ch, j)->name)) {
            if (obj_get_type_flag(char_get_equipment(ch, j)) == ITEM_WEAPON) {
              send_to_char(ch, "The weapon type of %s is a %s.\r\n",
                           obj_get_short_description(char_get_equipment(ch, j)),
                           weapon_type[(int)obj_get_value(char_get_equipment(ch, j), VAL_WEAPON_SKILL)]);
            }
            if (obj_get_type_flag(char_get_equipment(ch, j)) == ITEM_SPELLBOOK) {
              display_spells(ch, char_get_equipment(ch, j));
            }
            if (obj_get_type_flag(char_get_equipment(ch, j)) == ITEM_SCROLL) {
              display_scroll(ch, char_get_equipment(ch, j));
            }
            diag_obj_to_char(char_get_equipment(ch, j), ch);
            send_to_char(ch, "It appears to be made of %s", material_names[obj_get_value(char_get_equipment(ch, j), 7)]);
          }
          found = TRUE;
        }

    /* Does the argument match an extra desc in the char's inventory? */
    for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
      if (char_can_see_obj(ch, obj))
        if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
          if (obj_get_type_flag(obj) == ITEM_BOARD) {
            show_board(obj_get_vnum(obj), ch);
          } else {
            send_to_char(ch, "%s", desc);
            if (isname(arg, obj->name)) {
              if (obj_get_type_flag(obj) == ITEM_WEAPON) {
                send_to_char(ch, "The weapon type of %s is a %s.\r\n",
                             obj_get_short_description(obj), weapon_type[(int)obj_get_value(obj, VAL_WEAPON_SKILL)]);
              }
              if (obj_get_type_flag(obj) == ITEM_SPELLBOOK) {
                display_spells(ch, obj);
              }
              if (obj_get_type_flag(obj) == ITEM_SCROLL) {
                display_scroll(ch, obj);
              }
              diag_obj_to_char(obj, ch);
              send_to_char(ch, "It appears to be made of %s, and weights %s", material_names[obj_get_value(obj, 7)], add_commas(obj_get_weight(obj)));
            }
          }
          found = TRUE;
        }
    }

    /* Does the argument match an extra desc of an object in the room? */
    for (obj = world[char_get_in_room(ch)].contents; obj && !found; obj = obj->next_content)
      if (char_can_see_obj(ch, obj))
        if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
          if (obj_get_type_flag(obj) == ITEM_BOARD) {
            show_board(obj_get_vnum(obj), ch);
          } else {
            send_to_char(ch, "%s", desc);
            if (obj_get_type_flag(obj) == ITEM_VEHICLE) {
              send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @COpen hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @CClose hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @CEnter hatch\r\n");
            } else if (obj_get_type_flag(obj) == ITEM_HATCH) {
              send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @COpen hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @CClose hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @CUnlock hatch\r\n");
              send_to_char(ch, "@YSyntax@D: @CLeave@n\r\n");
            } else if (obj_get_type_flag(obj) == ITEM_WINDOW) {
              look_out_window(ch, obj->name);
            }

            if (obj_get_type_flag(obj) == ITEM_CONTROL) {
              send_to_char(ch, "@RFUEL@D: %s%s@n\r\n", obj_get_value(obj, 2) >= 200 ? "@G" : obj_get_value(obj, 2) >= 100 ? "@Y"
                                                                                                                          : "@r",
                           add_commas(obj_get_value(obj, 2)));
            }
            if (obj_get_type_flag(obj) == ITEM_WEAPON) {
              send_to_char(ch, "The weapon type of %s is a %s.\r\n", obj_get_short_description(obj), weapon_type[(int)obj_get_value(obj, VAL_WEAPON_SKILL)]);
            }
            diag_obj_to_char(obj, ch);
            send_to_char(ch, "It appears to be made of %s, and weights %s", material_names[obj_get_value(obj, 7)], add_commas(obj_get_weight(obj)));
          }
          found = TRUE;
        }

    /* If an object was found back in generic_find */
    if (bits) {
      if (!found)
        show_obj_to_char(found_obj, ch, SHOW_OBJ_ACTION);
      else {
        if (show_obj_modifiers(found_obj, ch))
          send_to_char(ch, "\r\n");
      }
    } else if (!found)
      send_to_char(ch, "You do not see that here.\r\n");
  }
}

void look_out_window(struct char_data *ch, char *arg)
{
  struct obj_data *i, *viewport = NULL, *vehicle = NULL;
  struct char_data *dummy = NULL;
  room_rnum target_room   = NOWHERE;
  int bits, door;

  /* First, lets find something to look out of or through. */
  if (*arg) {
    /* Find this object and see if it is a window */
    if (!(bits = generic_find(arg,
                              FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP,
                              ch, &dummy, &viewport))) {
      send_to_char(ch, "You don't see that here.\r\n");
      return;
    } else if (obj_get_type_flag(viewport) != ITEM_WINDOW) {
      send_to_char(ch, "You can't look out that!\r\n");
      return;
    }
  } else if (char_is_outside(ch)) {
    /* yeah, sure stupid */
    send_to_char(ch, "But you are already outside.\r\n");
    return;
  } else {
    /* Look for any old window in the room */
    for (i = world[char_get_in_room(ch)].contents; i; i = i->next_content)
      if ((obj_get_type_flag(i) == ITEM_WINDOW) &&
          isname("window", i->name)) {
        viewport = i;
        continue;
      }
  }
  if (!viewport) {
    /* Nothing suitable to look through */
    send_to_char(ch, "You don't seem to be able to see outside.\r\n");
  } else if (obj_val_flagged(viewport, CONT_CLOSEABLE) &&
             obj_val_flagged(viewport, CONT_CLOSED)) {
    /* The window is closed */
    send_to_char(ch, "It is closed.\r\n");
  } else {
    if (obj_get_value(viewport, VAL_WINDOW_UNUSED1) < 0) {
      /* We are looking out of the room */
      if (obj_get_value(viewport, VAL_WINDOW_UNUSED4) < 0) {
        /* Look for the default "outside" room */
        for (door = 0; door < NUM_OF_DIRS; door++)
          if (char_get_exit(ch, door))
            if (char_get_exit(ch, door)->to_room != NOWHERE)
              if (!rroom_flagged(char_get_exit(ch, door)->to_room, ROOM_INDOORS)) {
                target_room = char_get_exit(ch, door)->to_room;
                continue;
              }
      } else {
        target_room = real_room(obj_get_value(viewport, VAL_WINDOW_UNUSED4));
      }
    } else {
      /* We are looking out of a vehicle */
      if ((vehicle = find_vehicle_by_vnum(obj_get_value(viewport, VAL_WINDOW_UNUSED1))))
        target_room = obj_get_in_room(vehicle);
    }
    if (target_room == NOWHERE) {
      send_to_char(ch, "You don't seem to be able to see outside.\r\n");
    } else {
      if (viewport->action_description)
        act(viewport->action_description, TRUE, ch, viewport, 0, TO_CHAR);
      else
        act("$n looks out the window.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "You look outside and see:\r\n");
      look_at_room(target_room, ch, 0);
    }
  }
}

ACMD(do_look)
{
  int look_type;

  if (!ch->desc)
    return;
  if (char_get_position(ch) < POS_SLEEPING)
    send_to_char(ch, "You can't see anything but stars!\r\n");
  else if (char_aff_flagged(ch, AFF_BLIND))
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
  else if (char_plr_flagged(ch, PLR_EYEC))
    send_to_char(ch, "You can't see a damned thing, your eyes are closed!\r\n");
  else if (room_is_dark(char_get_in_room(ch)) && !char_can_see_in_dark(ch) && !char_plr_flagged(ch, PLR_AURALIGHT)) {
    send_to_char(ch, "It is pitch black...\r\n");
    list_char_to_char(world[char_get_in_room(ch)].people, ch); /* glowing red eyes */
  } else {
    char arg[MAX_INPUT_LENGTH], arg2[200];

    if (subcmd == SCMD_READ) {
      one_argument(argument, arg);
      if (!*arg)
        send_to_char(ch, "Read what?\r\n");
      else
        look_at_target(ch, arg, 1);
      return;
    }
    argument = any_one_arg(argument, arg);
    one_argument(argument, arg2);
    if (!*arg) {
      if (subcmd == SCMD_SEARCH) {
        search_room(ch);
      } else {
        look_at_room(char_get_in_room(ch), ch, 1);
        if (char_get_admlevel(ch) < 1 && !char_aff_flagged(ch, AFF_HIDE)) {
          //act("@w$n@w looks around the room.@n", TRUE, ch, 0, 0, TO_ROOM);
        }
      }
    } else if (is_abbrev(arg, "inside") && char_get_exit(ch, INDIR) && !*arg2) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, INDIR);
      else
        look_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside") && (subcmd == SCMD_SEARCH) && !*arg2) {
      search_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside") ||
               is_abbrev(arg, "into") || is_abbrev(arg, "onto")) {
      look_in_obj(ch, arg2);
    } else if ((is_abbrev(arg, "outside") ||
                is_abbrev(arg, "through") ||
                is_abbrev(arg, "thru")) &&
               (subcmd == SCMD_LOOK) && *arg2) {
      look_out_window(ch, arg2);
    } else if (is_abbrev(arg, "outside") &&
               (subcmd == SCMD_LOOK) && !char_get_exit(ch, OUTDIR)) {
      look_out_window(ch, arg2);
    } else if ((look_type = search_block(arg, dirs, FALSE)) >= 0 ||
               (look_type = search_block(arg, abbr_dirs, FALSE)) >= 0) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, look_type);
      else
        look_in_direction(ch, look_type);
    } else if ((is_abbrev(arg, "towards")) &&
               ((look_type = search_block(arg2, dirs, FALSE)) >= 0 ||
                (look_type = search_block(arg2, abbr_dirs, FALSE)) >= 0)) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, look_type);
      else
        look_in_direction(ch, look_type);
    } else if (is_abbrev(arg, "at")) {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "That is not a direction!\r\n");
      else
        look_at_target(ch, arg2, 0);
    } else if (is_abbrev(arg, "around")) {
      struct extra_descr_data *i;
      int found = 0;

      for (i = world[char_get_in_room(ch)].ex_description; i; i = i->next) {
        if (*i->keyword != '.') {
          send_to_char(ch, "%s%s:\r\n%s",
                       (found ? "\r\n" : ""), i->keyword, i->description);
          found = 1;
        }
      }
      if (!found)
        send_to_char(ch, "You couldn't find anything noticeable.\r\n");
    } else if (find_exdesc(arg, world[char_get_in_room(ch)].ex_description) != NULL) {
      look_at_target(ch, arg, 0);
    } else {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "That is not a direction!\r\n");
      else
        look_at_target(ch, arg, 0);
    }
  }
}

ACMD(do_examine)
{
  struct char_data *tmp_char;
  struct obj_data *tmp_object;
  char tempsave[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Examine what?\r\n");
    return;
  }

  /* look_at_target() eats the number. */
  look_at_target(ch, strcpy(tempsave, arg), 0); /* strcpy: OK */

  generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((obj_get_type_flag(tmp_object) == ITEM_DRINKCON) ||
        (obj_get_type_flag(tmp_object) == ITEM_FOUNTAIN) ||
        (obj_get_type_flag(tmp_object) == ITEM_CONTAINER)) {
      send_to_char(ch, "When you look inside, you see:\r\n");
      look_in_obj(ch, arg);
    }
  }
}

#define WHO_FORMAT \
  "Usage: who [minlev[-maxlev]] [-k] [-n name] [-q] [-r] [-s] [-z]\r\n"

char *default_color_choices[NUM_COLOR + 1] = {
    /* COLOR_NORMAL */ AA_NORMAL,
    /* COLOR_ROOMNAME */ AA_NORMAL ANSISEPSTR AF_CYAN,
    /* COLOR_ROOMOBJS */ AA_NORMAL ANSISEPSTR AF_GREEN,
    /* COLOR_ROOMPEOPLE */ AA_NORMAL ANSISEPSTR AF_YELLOW,
    /* COLOR_HITYOU */ AA_NORMAL ANSISEPSTR AF_RED,
    /* COLOR_YOUHIT */ AA_NORMAL ANSISEPSTR AF_GREEN,
    /* COLOR_OTHERHIT */ AA_NORMAL ANSISEPSTR AF_YELLOW,
    /* COLOR_CRITICAL */ AA_BOLD ANSISEPSTR AF_YELLOW,
    /* COLOR_HOLLER */ AA_BOLD ANSISEPSTR AF_YELLOW,
    /* COLOR_SHOUT */ AA_BOLD ANSISEPSTR AF_YELLOW,
    /* COLOR_GOSSIP */ AA_NORMAL ANSISEPSTR AF_YELLOW,
    /* COLOR_AUCTION */ AA_NORMAL ANSISEPSTR AF_CYAN,
    /* COLOR_CONGRAT */ AA_NORMAL ANSISEPSTR AF_GREEN,
    /* COLOR_TELL */ AA_NORMAL ANSISEPSTR AF_RED,
    /* COLOR_YOUSAY */ AA_NORMAL ANSISEPSTR AF_CYAN,
    /* COLOR_ROOMSAY */ AA_NORMAL ANSISEPSTR AF_WHITE,
    NULL};
