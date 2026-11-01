/* rp.c: roleplay points, immortal action logs, hints, poses
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
#include "dbat/dg_comm.h"
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

// definitions
void log_imm_action(const char *messg, ...)
{

  FILE *fl;
  const char *filename;
  struct stat fbuf;

  filename = REQUEST_FILE;

  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    /*  SYSERR_DESC:
       *  This is from do_gen_write() and indicates that it cannot call the
       *  stat() system call on the file required.  The error string at the
       *  end of the line should explain what the problem is.
       */
    return;
  }
  if (fbuf.st_size >= CONFIG_MAX_FILESIZE * 4) {
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("SYSERR: log_imm_action");
    /*  SYSERR_DESC:
        *  This is from do_gen_write(), and will be output if the file in
        *  question cannot be opened for appending to.  The error string
        *  at the end of the line should explain what the problem is.
        */

    return;
  }
  time_t ct    = time(0);
  char *time_s = asctime(localtime(&ct));

  va_list args;

  va_start(args, messg);
  time_s[strlen(time_s) - 1] = '\0';

  fprintf(fl, "%-15.15s :: ", time_s + 4);
  vfprintf(fl, messg, args);
  fprintf(fl, "\n");
  va_end(args);

  fclose(fl);
}

void log_custom(struct descriptor_data *d, struct obj_data *obj)
{
  FILE *fl;
  const char *filename;
  struct stat fbuf;

  filename = CUSTOM_FILE;

  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    /*  SYSERR_DESC:
       *  This is from do_gen_write() and indicates that it cannot call the
       *  stat() system call on the file required.  The error string at the
       *  end of the line should explain what the problem is.
       */
    return;
  }
  if (fbuf.st_size >= CONFIG_MAX_FILESIZE * 4) {
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("SYSERR: log_custom");
    /*  SYSERR_DESC:
        *  This is from do_gen_write(), and will be output if the file in
        *  question cannot be opened for appending to.  The error string
        *  at the end of the line should explain what the problem is.
        */

    return;
  }

  fprintf(fl, "@D[@cUser@W: @R%-20s @cName@W: @C%-20s @cCustom@W: @Y%s@D]\n", char_get_user(d->character), char_get_name(d->character), obj->short_description);
  fclose(fl);
}

/* Let's Reward Those Roleplayers! - Iovan*/
ACMD(do_rpp)
{

  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int tnlcost = 1, revcost = 1, selection = 0, pay = 0;
  int max_choice = 15; /* Controls the maximum number of menu choices */
  struct obj_data *obj;

  half_chop(argument, arg, arg2);

  if (char_is_npc(ch)) {
    return;
  }

  revcost = revcost * (char_get_level(ch) / 15);
  tnlcost = 1 + (tnlcost * (char_get_level(ch) / 40));

  if (revcost < 1) {
    revcost = 1;
  }
  if (tnlcost < 1) {
    tnlcost = 1;
  }

  if (char_plr_flagged(ch, PLR_PDEATH)) {
    revcost *= 6;
  }

  if (!*arg) { /* Display menu */
    send_to_char(ch, "@C                             Rewards Menu\n");
    send_to_char(ch, "@b  ------------------------------------------------------------------\n");
    send_to_char(ch, "  @C1@D)@R Disabled            @D[@G -- RPP @D]  @C2@D)@R Disabled              @D[@G -- RPP @D]@n\n");
    send_to_char(ch, "  @C3@D)@c Custom Equipment    @D[@G 20 RPP @D]  @C4@D)@c Alignment Change      @D[@G 20 RPP @D]\n");
    send_to_char(ch, "  @C5@D)@c 7,500 zenni         @D[@G  1 RPP @D]  @C6@D)@c +2 To A Stat          @D[@G  2 RPP @D]\n");
    send_to_char(ch, "  @C7@D)@c +750 PS             @D[@G  4 RPP @D]  @C8@D)@R Disabled               @D[@G -- RPP @D]\n");
    send_to_char(ch, "  @C9@D)@c 50%s TNL Exp         @D[@G%3d RPP @D] @C10@D)@c Aura Change           @D[@G  2 RPP @D]\n", "%", tnlcost);
    send_to_char(ch, " @C11@D)@c Reach Softcap       @D[@G  5 RPP @D] @C12@D)@c RPP Store             @D[@G ?? RPP @D]\n");
    send_to_char(ch, " @C13@D)@c Extra Feature       @D[@G  1 RPP @D] @C14@D)@c Restring Equipment    @D[@G  1 RPP @D]\n");
    send_to_char(ch, " @C15@D)@c Extra Skillslot     @D[@G  3 RPP @D] @C16@D)@R Disabled              @D[@G -- RPP @D]@n\n");
    send_to_char(ch, "@b  ------------------------------------------------------------------@n\n");
    send_to_char(ch, "@D                           [@YYour RPP@D:@G %3d@D]@n\n", char_get_rp(ch));
    // send_to_char(ch, "@D                           [@YRPP Bank@D:@G %3d@D]@n\n", GET_RBANK(ch));
    send_to_char(ch, "\nSyntax: rpp (num)\n");
    return;
  }

  /* What choice did they make? */

  selection = atoi(arg);

  if (selection <= 0 || selection > max_choice) {
    send_to_char(ch, "You must choose a number from the menu. Enter the command again with no arguments for the menu.\r\n");
    return;
  }

  /* Instant Reward Section*/
  if (selection > 2) {

    if (selection == 3) { /* Custom Equipment Construction */
      if (char_get_rp(ch) < 20) {
        send_to_char(ch, "You need at least 20 RPP to initiate a custom equipment build.\r\n");
        return;
      } else {
        desc_set_connected(ch->desc, CON_POBJ);
        ch->desc->obj_name   = strdup("Generic Armor Vest");
        ch->desc->obj_short  = strdup("@cGeneric @DArmor @WVest@n");
        ch->desc->obj_long   = strdup("@wA @cgeneric @Darmor @Wvest@w is lying here@n");
        ch->desc->obj_type   = 1;
        ch->desc->obj_weapon = 0;
        disp_custom_menu(ch->desc);
        ch->desc->obj_editflag = EDIT_CUSTOM;
        ch->desc->obj_editval  = EDIT_CUSTOM_MAIN;
        return;
      }
    }
    if (selection == 4) { /* Simple Align Change*/
      pay = 20;
      if (char_get_rp(ch) < pay) {
        send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
        return;
      } else {
        if (!*arg2) {
          send_to_char(ch, "What do you want to change your alignment to? (evil, sorta-evil, neutral, sorta-good, good)");
          return;
        }
        if (!strcasecmp(arg2, "evil")) {
          send_to_char(ch, "You change your alignment to Evil.\r\n");
          char_set_alignment(ch, -750);
        } else if (!strcasecmp(arg2, "sorta-evil")) {
          send_to_char(ch, "You change your alignment to Sorta Evil.\r\n");
          char_set_alignment(ch, -50);
        } else if (!strcasecmp(arg2, "neutral")) {
          send_to_char(ch, "You change your alignment to Neutral.\r\n");
          char_set_alignment(ch, 0);
        } else if (!strcasecmp(arg2, "sorta-good")) {
          send_to_char(ch, "You change your alignment to Sorta Good.\r\n");
          char_set_alignment(ch, 51);
        } else if (!strcasecmp(arg2, "good")) {
          send_to_char(ch, "You change your alignment to Good.\r\n");
          char_set_alignment(ch, 300);
        } else {
          send_to_char(ch, "That is not an acceptable option for changing alignment.\r\n");
          return;
        }

      } /* Can pay for it */
    } /* End Simple Align Change */

    if (selection == 5) { /*Simple Zenni Reward*/
      pay = 1;
      if (char_get_rp(ch) < pay) {
        send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
        return;
      } else {
        char_set_bank_gold(ch, char_get_bank_gold(ch) + (7500));
        send_to_char(ch, "Your bank zenni has been increased by 7,500\r\n");
      } /* Can pay for it */
    } /* End Simple Zenni Reward */

    if (selection == 6) { /* Simple Stat Change*/
      pay = 2;
      if (char_get_rp(ch) < pay) {
        send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
        return;
      } else {
        if (!*arg2) {
          send_to_char(ch, "What stat? (str, con, int, wis, spd, agl)");
          return;
        }
        if (!strcasecmp(arg2, "str")) {
          if (char_get_bonuses(ch, BONUS_WIMP) > 0 && ch->real_abils.str >= 70) {
            send_to_char(ch, "You can't because that stat maxes at 70 due to a trait negative.\r\n");
            return;
          } else if (char_get_aff_abils_str(ch) >= 100) {
            send_to_char(ch, "100 is the maximum for any stat.\r\n");
            return;
          }
          send_to_char(ch, "You increase your strength by 2.\r\n");
          ch->real_abils.str += 2;
        } else if (!strcasecmp(arg2, "con")) {
          if (char_get_bonuses(ch, BONUS_FRAIL) > 0 && ch->real_abils.con >= 70) {
            send_to_char(ch, "You can't because that stat maxes at 70 due to a trait negative.\r\n");
            return;
          } else if (char_get_aff_abils_con(ch) >= 100) {
            send_to_char(ch, "100 is the maximum for any stat.\r\n");
            return;
          }
          send_to_char(ch, "You increase your constitution by 2.\r\n");
          ch->real_abils.con += 2;
        } else if (!strcasecmp(arg2, "int")) {
          if (char_get_bonuses(ch, BONUS_DULL) > 0 && ch->real_abils.intel >= 70) {
            send_to_char(ch, "You can't because that stat maxes at 70 due to a trait negative.\r\n");
            return;
          } else if (char_get_aff_abils_intel(ch) >= 100) {
            send_to_char(ch, "100 is the maximum for any stat.\r\n");
            return;
          }
          send_to_char(ch, "You increase your intelligence by 2.\r\n");
          ch->real_abils.intel += 2;
        } else if (!strcasecmp(arg2, "wis")) {
          if (char_get_bonuses(ch, BONUS_FOOLISH) > 0 && ch->real_abils.wis >= 70) {
            send_to_char(ch, "You can't because that stat maxes at 70 due to a trait negative.\r\n");
            return;
          } else if (char_get_aff_abils_wis(ch) >= 100) {
            send_to_char(ch, "100 is the maximum for any stat.\r\n");
            return;
          }
          send_to_char(ch, "You increase your wisdom by 2.\r\n");
          ch->real_abils.wis += 2;
        } else if (!strcasecmp(arg2, "spd")) {
          if (char_get_bonuses(ch, BONUS_SLOW) > 0 && ch->real_abils.cha >= 70) {
            send_to_char(ch, "You can't because that stat maxes at 70 due to a trait negative.\r\n");
            return;
          } else if (char_get_aff_abils_cha(ch) >= 100) {
            send_to_char(ch, "100 is the maximum for any stat.\r\n");
            return;
          }
          send_to_char(ch, "You increase your speed by 2.\r\n");
          ch->real_abils.cha += 2;
        } else if (!strcasecmp(arg2, "agl")) {
          if (char_get_bonuses(ch, BONUS_CLUMSY) > 0 && ch->real_abils.dex >= 70) {
            send_to_char(ch, "You can't because that stat maxes at 70 due to a trait negative.\r\n");
            return;
          } else if (char_get_aff_abils_dex(ch) >= 100) {
            send_to_char(ch, "100 is the maximum for any stat.\r\n");
            return;
          }
          send_to_char(ch, "You increase your speed by 2.\r\n");
          ch->real_abils.dex += 2;
        } else {
          send_to_char(ch, "That is not an acceptable option for changing alignment.\r\n");
          return;
        }

      } /* Can pay for it */
    } /* End Simple Align Change */

    if (selection == 7) { /*Simple PS Reward*/
      pay = 4;
      if (char_get_rp(ch) < pay) {
        send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
        return;
      } else {
        char_set_practice_points(ch, char_get_practice_points(ch) + (750));
        send_to_char(ch, "Your practices have been increased by 750\r\n");
      } /* Can pay for it */
    } /* End Simple Zenni Reward */

    //  if (selection == 8) { /* Simple Revival Reward */
    //   pay = revcost;
    //   if (GET_RP(ch) < pay) {
    //  send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
    //  return;
    //   } else if (!AFF_FLAGGED(ch, AFF_SPIRIT)) {
    //    send_to_char(ch, "You aren't even dead!");
    //    return;
    // } else {
    //       resurrect(ch, 2);
    //    send_to_char(ch, "You have been revived.\r\n");
    //   } /* Can pay for it */
    //  } /* End Simple Revival Reward */

    if (selection == 9) { /*Simple Exp Reward*/
      pay = tnlcost;
      if (char_get_rp(ch) < pay) {
        send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
        return;
      } else if (char_get_level(ch) >= 100) {
        send_to_char(ch, "You can not buy experience anymore at your level. I think you know why.\r\n");
        return;
      } else if (level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) < 0) {
        send_to_char(ch, "You can not buy experience anymore UNTIL you level.\r\n");
        return;
      } else {
        char_set_exp(ch, char_get_exp(ch) + (level_exp(ch, char_get_level(ch) + 1) * .52));
        send_to_char(ch, "You gained 50%s of the entire experience needed for your next level.\r\n", "%");
      } /* Can pay for it */
    } /* End Simple Exp Reward */

    if (selection == 10) { /* Simple Aura Change*/
      pay = 2;
      if (char_get_rp(ch) < pay) {
        send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
        return;
      } else {
        if (!*arg2) {
          send_to_char(ch, "Change your aura to what? (white, blue, red, green, pink, purple, yellow, black, orange)");
          return;
        }
        if (!strcasecmp(arg2, "white")) {
          char_set_aura(ch, 0);
          send_to_char(ch, "You change your aura to white.\r\n");
        } else if (!strcasecmp(arg2, "blue")) {
          char_set_aura(ch, 1);
          send_to_char(ch, "You change your aura to blue.\r\n");
        } else if (!strcasecmp(arg2, "red")) {
          char_set_aura(ch, 2);
          send_to_char(ch, "You change your aura to red.\r\n");
        } else if (!strcasecmp(arg2, "green")) {
          char_set_aura(ch, 3);
          send_to_char(ch, "You change your aura to green.\r\n");
        } else if (!strcasecmp(arg2, "pink")) {
          char_set_aura(ch, 4);
          send_to_char(ch, "You change your aura to pink.\r\n");
        } else if (!strcasecmp(arg2, "purple")) {
          char_set_aura(ch, 5);
          send_to_char(ch, "You change your aura to purple.\r\n");
        } else if (!strcasecmp(arg2, "yellow")) {
          char_set_aura(ch, 6);
          send_to_char(ch, "You change your aura to yellow.\r\n");
        } else if (!strcasecmp(arg2, "black")) {
          char_set_aura(ch, 7);
          send_to_char(ch, "You change your aura to black.\r\n");
        } else if (!strcasecmp(arg2, "orange")) {
          char_set_aura(ch, 8);
          send_to_char(ch, "You change your aura to orange.\r\n");
        } else {
          send_to_char(ch, "That is not an acceptable option for changing alignment.\r\n");
          return;
        }

      } /* Can pay for it */
    } /* End Simple Aura Change */

    if (selection == 11) { /* Simple Soft-cap Reward*/
      pay = 5;
      if (char_get_rp(ch) < pay) {
        send_to_char(ch, "You do not have enough RPP for that selection.\r\n");
        return;
      } else if (char_get_level(ch) >= 100) {
        send_to_char(ch, "You can't use this at level 100.\r\n");
        return;
      } else if (char_is_arlian(ch)) {
        send_to_char(ch, "This is not available to bugs.\r\n");
        return;
      } else {
        if (is_soft_cap(ch, 0, 1.0) && is_soft_cap(ch, 1, 1.0) && is_soft_cap(ch, 2, 1.0)) {
          send_to_char(ch, "You are already above your softcap for this level.\r\n");
          return;
        }
        bring_to_cap(ch);
      } /* Can pay for it */
    } /* End Simple Soft-cap Reward */

    if (selection == 12) {
      if (!*arg2) {
        disp_rpp_store(ch);
        return;
      } else if (atoi(arg2) <= 0) {
        send_to_char(ch, "That is not a choice in the RPP store!\r\n");
        return;
      } else {
        int choice = atoi(arg2);
        handle_rpp_store(ch, choice);
        return;
      }
    }

    if (selection == 13) { /* Extra Feature Reward*/
      rpp_feature(ch, arg2);
      return;
    } /* End Extra Feature Reward */

    if (selection == 14) { /* Restring equipment reward */
      pay = 1;
      if (char_get_rp(ch) < pay) {
        send_to_char(ch, "You need at least 1 RPP to initiate an equipment restring.\r\n");
        return;
      } else if (!(obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
        send_to_char(ch, "You don't have a that equipment to restring in your inventory.\r\n");
        send_to_char(ch, "Syntax: rpp 14 (obj name)\r\n");
        return;
      } else if (obj_extra_flagged(obj, ITEM_CUSTOM)) {
        send_to_char(ch, "You can not restring a custom piece. Why? Cause I say so. :P\r\n");
        return;
      } else {
        desc_set_connected(ch->desc, CON_POBJ);
        char thename[MAX_INPUT_LENGTH], theshort[MAX_INPUT_LENGTH], thelong[MAX_INPUT_LENGTH];

        *thename  = '\0';
        *theshort = '\0';
        *thelong  = '\0';

        sprintf(thename, "%s", obj->name);
        sprintf(theshort, "%s", obj->short_description);
        sprintf(thelong, "%s", obj->description);

        ch->desc->obj_name   = strdup(thename);
        ch->desc->obj_was    = strdup(theshort);
        ch->desc->obj_short  = strdup(theshort);
        ch->desc->obj_long   = strdup(thelong);
        ch->desc->obj_point  = obj;
        ch->desc->obj_type   = 1;
        ch->desc->obj_weapon = 0;
        disp_restring_menu(ch->desc);
        ch->desc->obj_editflag = EDIT_RESTRING;
        ch->desc->obj_editval  = EDIT_RESTRING_MAIN;
        return;
      }
    } /* End equipment restring reward */

    if (selection == 15) { /* Skillslot Reward */
      pay = 3;
      if (char_get_rp(ch) < pay) {
        send_to_char(ch, "You do not have enough RPP in your bank for that selection.\r\n");
        return;
      } else if (char_get_bonuses(ch, BONUS_GMEMORY) && char_get_skill_slots(ch) >= 65) {
        send_to_char(ch, "You are already at your skillslot cap.\r\n");
        return;
      } else if (!char_get_bonuses(ch, BONUS_GMEMORY) && (char_get_skill_slots(ch) >= 60)) {
        send_to_char(ch, "You are already at your skillslot cap.\r\n");
        return;
      } else {
        char_set_skill_slots(ch, char_get_skill_slots(ch) + (1));
      } /* Can pay for it */
    } /*End Skillslot Reward */

    if (selection == 16) { /* DB Spawn Reward */
      pay = 5000;
      if (char_get_rp(ch) < pay) {
        send_to_char(ch, "You do not have enough RPP in your bank for that selection.\r\n");
        return;
      } else {
        int found          = FALSE;
        struct obj_data *k = NULL;
        for (k = object_list; k; k = k->next) {
          if (obj_extra_flagged(k, ITEM_FORGED)) {
            continue;
          }
          if (obj_get_vnum(k) == 20) {
            found = TRUE;
          } else if (obj_get_vnum(k) == 21) {
            found = TRUE;
          } else if (obj_get_vnum(k) == 22) {
            found = TRUE;
          } else if (obj_get_vnum(k) == 23) {
            found = TRUE;
          } else if (obj_get_vnum(k) == 24) {
            found = TRUE;
          } else if (obj_get_vnum(k) == 25) {
            found = TRUE;
          } else if (obj_get_vnum(k) == 26) {
            found = TRUE;
          }
        }
        if (found == FALSE) {
          send_to_char(ch, "You have reduced the Dragon Ball wait by a whole real life day!\r\n");
          send_to_all("%s has just reduced the Dragon Ball wait by a whole real life day!\r\n", char_get_name(ch));
          dballtime -= 86400;
          if (dballtime <= 0) {
            dballtime = 1;
          }
        } else if (SELFISHMETER >= 10) {
          send_to_char(ch, "Sorry, it seems there there are several powers interfering with the Dragon Balls.\r\n");
          return;
        } else {
          send_to_char(ch, "Sorry, but there is already a set of Dragon Balls in existence.\r\n");
          return;
        }
      } /* Can pay for it */
    }

  } /* End Instant Reward Section */

  /* Resuest Only Section */
  if (selection <= 2) {
    FILE *fl;
    const char *filename;
    struct stat fbuf;

    filename = REQUEST_FILE;

    if (selection == 1) {
      pay = 6500;
      if (char_get_rp(ch) < pay) {
        send_to_char(ch, "Nice try but you don't have enough RPP for that.\r\n");
        return;
      } else {
        send_to_char(ch, "You now have an Excel House Capsule!\r\n");
        struct obj_data *hobj = read_object(6, VIRTUAL);
        obj_to_char(hobj, ch);
        char_set_rp(ch, char_get_rp(ch) - (pay));
        ch->desc->rpp = char_get_rp(ch);
        userWrite(ch->desc, 0, 0, 0, "index");
        save_char(ch);
        send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", pay);
        send_to_imm("RPP Purchase: %s %d", char_get_name(ch), pay);
        return;
      }
    } else if (selection == 2) {
      return;
      pay = 200;
    }

    if (char_get_rp(ch) < pay) {
      send_to_char(ch, "Nice try but you don't have enough RPP for that.\r\n");
      return;
    }

    if (stat(filename, &fbuf) < 0) {
      perror("SYSERR: Can't stat() file");
      /*  SYSERR_DESC:
       *  This is from do_gen_write() and indicates that it cannot call the
       *  stat() system call on the file required.  The error string at the
       *  end of the line should explain what the problem is.
       */
      return;
    }
    if (fbuf.st_size >= CONFIG_MAX_FILESIZE) {
      send_to_char(ch, "Sorry, the file is full right now.. try again later.\r\n");
      return;
    }
    if (!(fl = fopen(filename, "a"))) {
      perror("SYSERR: do_reward_request");
      /*  SYSERR_DESC:
        *  This is from do_gen_write(), and will be output if the file in
        *  question cannot be opened for appending to.  The error string
        *  at the end of the line should explain what the problem is.
        */

      send_to_char(ch, "Could not open the file.  Sorry.\r\n");
      return;
    }
    if (selection == 1) {
      fprintf(fl, "@D[@cName@W: @C%-20s @cRequest@W: @Y%-20s@D]\n", char_get_name(ch), "House");
      send_to_imm("RPP Request: %s paid for house", char_get_name(ch));
      BOARDNEWCOD = time(0);
      save_mud_time(&time_info);
    } else if (selection == 2) {
      fprintf(fl, "@D[@cName@W: @C%-20s @cRequest@W: @Y%-20s@D]\n", char_get_name(ch), "Custom Skill");
      send_to_imm("RPP Request: %s paid for Custom Skill, uhoh spaggettios", char_get_name(ch));
      BOARDNEWCOD = time(0);
      save_mud_time(&time_info);
    }
    char_set_rp(ch, char_get_rp(ch) - (pay));
    ch->desc->rpp = char_get_rp(ch);
    userWrite(ch->desc, 0, 0, 0, "index");
    save_char(ch);
    send_to_char(ch, "@R%d@W RPP paid for your selection. An immortal will address the request soon enough. Be patient.@n\r\n", pay);

    fclose(fl);
  } /* End Request Only Section */

  /* Pay for purchases here */
  if (selection >= 4 && selection < 12 && pay > 0) {
    char_set_rp(ch, char_get_rp(ch) - (pay));
    ch->desc->rpp = char_get_rp(ch);
    userWrite(ch->desc, 0, 0, 0, "index");
    save_char(ch);
    send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", pay);
    send_to_imm("RPP Purchase: %s %d", char_get_name(ch), pay);
  }

  if (selection > 12 && pay > 0) {
    char_set_rp(ch, char_get_rp(ch) - (pay));
    ch->desc->rpp = char_get_rp(ch);
    userWrite(ch->desc, 0, 0, 0, "index");
    save_char(ch);
    send_to_char(ch, "@R%d@W RPP paid for your selection. Enjoy!@n\r\n", pay);
    send_to_imm("RPP Purchase: %s %d", char_get_name(ch), pay);
  }
}

ACMD(do_commune)
{

  if (char_is_npc(ch))
    return;

  if (!know_skill(ch, SKILL_COMMUNE)) {
    return;
  }

  if ((getCurST(ch)) >= getMaxST(ch)) {
    send_to_char(ch, "Your stamina is already at full.\r\n");
    return;
  }

  int prob = char_get_skill(ch, SKILL_COMMUNE), perc = axion_dice(0);
  int64_t cost = getMaxST(ch) * .05;

  if ((getCurKI(ch)) < cost) {
    send_to_char(ch, "You do not have enough ki to commune with the Eldritch Star.\r\n");
    return;
  }
  if (prob < perc) {
    decCurKI(ch, cost, 0);
    reveal_hiding(ch, 0);
    act("@cYou close your eyes and try to commune with the Eldritch Star. You are unable to concentrate though.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n closes $s eyes for a moment. Then $e reopens them and frowns.@n", TRUE, ch, 0, 0, TO_ROOM);
    char_set_wait(ch, PULSE_2SEC);
    return;
  } else {
    decCurKI(ch, cost, 0);
    decCurST(ch, cost, 0);
    reveal_hiding(ch, 0);
    act("@cYou close your eyes and commune with the Eldritch Star spiritually. You feel your stamina replenish some.@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@W$n closes $s eyes for a moment. Then $e reopens them and smiles.@n", TRUE, ch, 0, 0, TO_ROOM);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
}

ACMD(do_pose)
{

  if (!know_skill(ch, SKILL_POSE)) {
    return;
  }

  if ((getCurST(ch)) < getMaxST(ch) / 40) {
    send_to_char(ch, "You do not have enough stamina to pull off such an exciting pose!\r\n");
    return;
  }

  if (is_affected(ch, AFF_SPECIAL_POSE)) {
    send_to_char(ch, "You are already feeling good and confident from a previous pose.\r\n");
    return;
  }

  if (char_get_fighting(ch)) {
    send_to_char(ch, "You are too busy to pose right now!\r\n");
    return;
  }

  int prob = char_get_skill(ch, SKILL_POSE);
  int perc = rand_number(1, 70);

  if (ch->real_abils.str + 8 > 70 && char_get_bonuses(ch, BONUS_WIMP) > 0) {
    send_to_char(ch, "You can't handle having your strength increased beyond 70.\r\n");
    return;
  }
  if (ch->real_abils.dex + 8 > 70 && char_get_bonuses(ch, BONUS_CLUMSY) > 0) {
    send_to_char(ch, "You can't handle having your agility increased beyond 70.\r\n");
    return;
  }

  if (prob < perc) {
    reveal_hiding(ch, 0);
    act("@WYou attempt to strike an awe inspiring pose, but end up falling on your face!@n", TRUE, ch, 0, 0, TO_CHAR);
    act("@C$n@W attempts to strike an awe inspiring pose, but ends up falling on $s face!@n", TRUE, ch, 0, 0, TO_ROOM);
    decCurST(ch, getMaxST(ch) / 40, 0);
    improve_skill(ch, SKILL_POSE, 0);
    return;
  } else {
    reveal_hiding(ch, 0);
    switch (rand_number(1, 4)) {
      case 1:
        act("@WYou turn around with your back to everyone. You bend forward dramatically and put your head between your legs!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W turns around with $s back to you. $e bends forward dramatically and puts $s head between $s legs. Strange...@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("@WYou turn to the side while flexing your muscles and extend your arms up at an angle dramatically!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W turns to the side while flexing $s muscles and extending $s arms up at an angle dramatically!@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("@WYou extend one leg outward while you bend forward, balancing on a single leg!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W extends one leg outward while $e bends forward, balancing on a single leg!@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("@WYou drop down to one knee while angling your arms up to either side and slanting your hands down like wings!@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W drops down to one knee while angling $s arms up to either side and slanting $s hands down like wings!@n", TRUE, ch, 0, 0, TO_ROOM);
        break;
    }
    send_to_char(ch, "@WYou feel your confidence increase! @G+8 Str @Wand@G +8 Agl!@n\r\n");
    assign_affect(ch, AFF_SPECIAL_POSE, SKILL_POSE, -1, 8, 0, 0, 8, 0, 0);
    save_char(ch);
    int64_t before = (getMaxLF(ch));
    incCurLF(ch, (getMaxLF(ch)) - before, true);
    decCurST(ch, getMaxST(ch) / 40, 0);
    improve_skill(ch, SKILL_POSE, 0);
    return;
  }
}

/* End of do_fury for halfbreeds to release their raaage, rawrg! */

void hint_system(struct char_data *ch, int num)
{
  const char *hints[22] = {"Remember to save often.",                                                                                                                 /* 0 */
                           "Remember to eat or drink if you want to stay alive.",                                                                                     /* 1 */
                           "It is a good idea to save up PS for learning skills instead of just practicing them.",                                                    /* 2 */
                           "A good way to save up money is with the bank.",                                                                                           /* 3 */
                           "If you want to stay alive in this rough world you will need to be mindful of your surroundings.",                                         /* 4 */
                           "Knowing when to rest and recover can be the difference between life and death.",                                                          /* 5 */
                           "Not every battle can be won. Great warriors know how to pick their fights.",                                                              /* 6 */
                           "It is a good idea to experiment with skills fully before deciding their worth.",                                                          /* 7 */
                           "Having a well balanced repertoire of skills can help you out of any situation.",                                                          /* 8 */
                           "You can become hidden from your enemies on who and ooc with the whohide command.",                                                        /* 9 */
                           "You can value an item at a shopkeeper with the value command.",                                                                           /* 10 */
                           "There are ways to earn money through jobs, try looking for a job. Bum.",                                                                  /* 11 */
                           "You never know what may be hidden nearby. You should always check out anything you can.",                                                 /* 12 */
                           "You should check for a help file on any subject you can, you never know how the info may 'help' you.",                                    /* 13 */
                           "Until you are capable of taking care of yourself for long periods of time you should stick near your sensei.",                            /* 14 */
                           "You shouldn't travel to other planets until you have a stable supply of money.",                                                          /* 15 */
                           "There is a vast galaxy out there that you may not be able to reach by public ship.",                                                      /* 16 */
                           "Score is used to view the various statistics about your character.",                                                                      /* 17 */
                           "Status is used to view what is influencing your character and its characteristics.",                                                      /* 18 */
                           "You will need a scouter in order to use the Scouter Network (SNET).",                                                                     /* 19 */
                           "The DBAT forum is a great resource for finding out information and for conversing\r\nwith fellow players. http://advent-truth.com/forum", /* 20 */
                           "Found a bug or have a suggestion? Log into our forums and post in the relevant section."};
  if (num == 0) {
    num = rand_number(0, 21);
  }

  if (!char_is_android(ch) && !char_is_namek(ch)) {
    send_to_char(ch, "@D[@GHint@D] @G%s@n\r\n", hints[num]);
  } else {
    if (num == 1) {
      num = 0;
    }
    send_to_char(ch, "@D[@GHint@D] @G%s@n\r\n", hints[num]);
  }
  send_to_char(ch, "@D(@gYou can turn off hints with the command 'hints'@D)@n\r\n");
}

ACMD(do_think)
{
  skip_spaces(&argument);

  if (char_is_npc(ch)) {
    return;
  }
  if (char_in_arena(ch)) {
    send_to_char(ch, "Lol, no.\r\n");
    return;
  }
  if (char_get_skill(ch, SKILL_TELEPATHY)) {
    send_to_char(ch, "You can just use telepathy.\r\n");
    return;
  }
  if (!char_get_mindlink(ch)) {
    send_to_char(ch, "No one has linked with your mind.\r\n");
    return;
  }
  if (!*argument) {
    send_to_char(ch, "Syntax: think (message)\r\n");
    return;
  } else {
    struct char_data *tch;
    tch = char_get_mindlink(ch);
    send_to_char(ch, "@c%s@w reads your thoughts, '@C%s@w'@n\r\n", char_get_name(tch), argument);
    send_to_char(tch, "@c%s@w thinks, '@C%s@w'@n\r\n", char_get_name(ch), argument);
    send_to_imm("@GTELEPATHY: @C%s@G telepaths @c%s, @W'@w%s@W'@n", char_get_admlevel(ch) > 0 ? char_get_name(ch) : char_get_user(ch), char_get_admlevel(tch) > 0 ? char_get_name(tch) : char_get_user(tch), argument);
    return;
  }
}
