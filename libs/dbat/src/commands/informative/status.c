/* status.c: score, status, inventory, equipment, consider
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
#include "dbat/game_world.h"

static void bonus_status(GameWorld *g, struct char_data *ch);
// definitions
ACMD(do_evolve)
{

  if (!char_is_arlian(ch) || char_is_npc(ch)) {
    send_to_char(g, ch, "You are not an arlian!\r\n");
    return;
  }

  char arg[MAX_INPUT_LENGTH];
  one_argument(argument, arg);

  int64_t plcost = char_get_level(ch), stcost = char_get_level(ch), kicost = char_get_level(ch);

  plcost += molt_threshold(ch) * 0.65 + (getBasePL(ch) * 0.15);
  kicost += (molt_threshold(ch) * 0.50) + (getBaseKI(ch) * 0.22);
  stcost += (molt_threshold(ch) * 0.50) + (getBaseST(ch) * 0.15);

  if (!*arg) {
    send_to_char(g, ch, "@D-=@YConvert Evolution Points To What?@D=-@n\r\n");
    send_to_char(g, ch, "@D-------------------------------------@n\r\n");
    send_to_char(g, ch, "@CPowerlevel  @D: @Y%s @Wpts\r\n", add_commas(plcost));
    send_to_char(g, ch, "@CKi          @D: @Y%s @Wpts\r\n", add_commas(kicost));
    send_to_char(g, ch, "@CStamina     @D: @Y%s @Wpts\r\n", add_commas(stcost));
    send_to_char(g, ch, "@D[@Y%s @Wpts currently@D]@n\r\n", add_commas(char_get_moltexp(ch)));
    return;
  }
  if (!strcasecmp(arg, "powerlevel") || !strcasecmp(arg, "pl")) {
    if (plcost > molt_threshold(ch)) {
      send_to_char(g, ch, "You need a few more evolution levels before you can start upgrading powerlevel.\r\n");
      return;
    }
    if (char_get_moltexp(ch) < plcost) {
      send_to_char(g, ch, "You do not have enough evolution experience.\r\n");
      return;
    }
    int64_t plgain = (getBasePL(ch)) * 0.01;

    if (plgain <= 0) {
      plgain = rand_number(g, 1, 5);
    } else {
      plgain = rand_number(g, plgain, plgain * .5);
    }
    gainBasePL(ch, plgain, false);
    char_set_moltexp(ch, char_get_moltexp(ch) - (plcost));
    send_to_char(g, ch, "Your body evolves to make better use of the way it is now, and you feel that your body has strengthened. @D[@RPL@D: @Y+%s@D]@n\r\n", add_commas(plgain));
  } else if (!strcasecmp(arg, "ki")) {
    if (kicost > molt_threshold(ch)) {
      send_to_char(g, ch, "You need a few more evolution levels before you can start upgrading ki.\r\n");
      return;
    }
    if (char_get_moltexp(ch) < kicost) {
      send_to_char(g, ch, "You do not have enough evolution experience.\r\n");
      return;
    }
    int64_t kigain = (getBaseKI(ch)) * 0.01;

    if (kigain <= 0) {
      kigain = rand_number(g, 1, 5);
    } else {
      kigain = rand_number(g, kigain, kigain * .5);
    }
    gainBaseKI(ch, kigain, false);
    char_set_moltexp(ch, char_get_moltexp(ch) - (kicost));
    send_to_char(g, ch, "Your body evolves to make better use of the way it is now, and you feel that your spirit has strengthened. @D[@CKi@D: @Y+%s@D]@n\r\n", add_commas(kigain));
  } else if (!strcasecmp(arg, "stamina") || !strcasecmp(arg, "st")) {
    if (stcost > molt_threshold(ch)) {
      send_to_char(g, ch, "You need a few more evolution levels before you can start upgrading stamina.\r\n");
      return;
    }
    if (char_get_moltexp(ch) < stcost) {
      send_to_char(g, ch, "You do not have enough evolution experience.\r\n");
      return;
    }
    int64_t stgain = (getBaseST(ch)) * 0.01;

    if (stgain <= 0) {
      stgain = rand_number(g, 1, 5);
    } else {
      stgain = rand_number(g, stgain, stgain * .5);
    }
    gainBaseST(ch, stgain, false);
    char_set_moltexp(ch, char_get_moltexp(ch) - (stcost));
    send_to_char(g, ch, "Your body evolves to make better use of the way it is now, and you feel that your body has more stamina. @D[@GST@D: @Y+%s@D]@n\r\n", add_commas(stgain));
  }
}

bool perf_skill(int skill)
{

  if (!skill) {
    return false;
  }

  switch (skill) {
    case 464:
      return true;
      break;
    case 441:
      return true;
      break;
    case 444:
      return true;
      break;
    case 475:
      return true;
      break;
    case 474:
      return true;
      break;
    case 476:
      return true;
      break;
    case 488:
      return true;
      break;
    case 472:
      return true;
      break;
    case 485:
      return true;
      break;
    case 442:
      return true;
      break;
    case 510:
      return true;
      break;
    case 533:
      return true;
      break;
    default:
      return false;
      break;
  }
}

ACMD(do_perf)
{
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  int i, skill = 1, found = false, type = 0;

  two_arguments(argument, arg, arg2);

  if (char_is_npc(ch) || char_get_admlevel(ch) > 0) {
    send_to_char(g, ch, "I don't think so.\r\n");
    return;
  }

  if (!*arg || !*arg2) {
    send_to_char(g, ch, "@WType @G1@D: @wOver Charged@n\r\n");
    send_to_char(g, ch, "@WType @G2@D: @wAccurate@n\r\n");
    send_to_char(g, ch, "@WType @G3@D: @wEfficient@n\r\n");
    send_to_char(g, ch, "Syntax: perfect (skillname) (type 1/2/or 3)\r\n");
    return;
  }
  if (strlen(arg) < 4) {
    send_to_char(g, ch, "The skill name should be longer than 3 characters...\r\n");
    return;
  }
  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    if (g->spell_info[i].skilltype != SKTYPE_SKILL)
      continue;

    if (found == true)
      continue;

    if (strstr(g->spell_info[i].name, arg)) {
      skill = i;
      found = true;
    }
  }
  if (found == false) {
    send_to_char(g, ch, "The skill %s doesn't exist.\r\n", arg);
    return;
  }
  if (!char_get_skill(ch, skill)) {
    send_to_char(g, ch, "You don't know %s.\r\n", arg);
    return;
  }
  if (char_get_skill(ch, skill) < 100) {
    send_to_char(g, ch, "You have not mastered the skill %s and thus can't perfect it.\r\n", arg);
    return;
  }
  if (char_get_skills_perf(ch, skill) > 0) {
    send_to_char(g, ch, "You have already mastered the skill %s and chosen how to perfect it.\r\n", arg);
    return;
  }
  if (!perf_skill(skill)) {
    send_to_char(g, ch, "You can't perfect that type of skill.\r\n");
    return;
  }
  if (atoi(arg2) < 1 || atoi(arg2) > 3) {
    send_to_char(g, ch, "@WType @G1@D: @wOver Charged@n\r\n");
    send_to_char(g, ch, "@WType @G2@D: @wAccurate@n\r\n");
    send_to_char(g, ch, "@WType @G3@D: @wEfficient@n\r\n");
    send_to_char(g, ch, "@RType must be a number between 1 and 3.@n\r\n");
    return;
  }
  type = atoi(arg2);
  switch (type) {
    case 1:
      send_to_char(g, ch, "You perfect the skill %s so that you can over charge it!\r\n", g->spell_info[skill].name);
      char_set_skills_perf(ch, skill, 1);
      break;
    case 2:
      send_to_char(g, ch, "You perfect the skill %s so that you have supreme accuracy with it!\r\n", g->spell_info[skill].name);
      char_set_skills_perf(ch, skill, 2);
      break;
    case 3:
      send_to_char(g, ch, "You perfect the skill %s so that you require a lower minimum charge for it!\r\n", g->spell_info[skill].name);
      char_set_skills_perf(ch, skill, 3);
      break;
  }
}

ACMD(do_gold)
{
  if (char_get_gold(ch) == 0)
    send_to_char(g, ch, "You're broke!\r\n");
  else if (char_get_gold(ch) == 1)
    send_to_char(g, ch, "You have one little zenni.\r\n");
  else
    send_to_char(g, ch, "You have %d zenni.\r\n", char_get_gold(ch));
}

ACMD(do_score)
{

  if (char_is_npc(ch))
    return;

  int view = 0, full = 5, personal = 1, health = 2, stats = 3, other = 4;

  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    view = full;
  } else if (strstr("personal", arg) || strstr("Personal", arg)) {
    view = personal;
  } else if (strstr("health", arg) || strstr("Health", arg)) {
    view = health;
  } else if (strstr("statistics", arg) || strstr("Statistics", arg)) {
    view = stats;
  } else if (strstr("other", arg) || strstr("Other", arg)) {
    view = other;
  } else {
    send_to_char(g, ch, "Syntax: score, or... score (personal, health, statistics, other)\r\n");
    return;
  }

  if (view == full || view == personal) {
    send_to_char(g, ch, "  @cO@D-----------------------------[  @cPersonal  @D]-----------------------------@cO@n\n");
    send_to_char(g, ch, "  @D|  @CName@D: @W%15s@D,   @CTitle@D: @W%-38s@D|@n\n", char_get_name(ch), char_get_title(ch));
    if (char_is_android(ch)) {
      char model[100], version[100];
      int absorb = 0;
      if (char_plr_flagged(ch, PLR_ABSORB)) {
        snprintf(model, sizeof(model), "@CAbsorption");
      } else if (char_plr_flagged(ch, PLR_REPAIR)) {
        snprintf(model, sizeof(model), "@GSelf Repairing");
      } else if (char_plr_flagged(ch, PLR_SENSEM)) {
        snprintf(model, sizeof(model), "@RSensor Equiped");
      }
      if (char_plr_flagged(ch, PLR_TRANS1)) {
        snprintf(version, sizeof(version), "Beta 1.0");
      } else if (char_plr_flagged(ch, PLR_TRANS2)) {
        snprintf(version, sizeof(version), "ANS 2.0");
      } else if (char_plr_flagged(ch, PLR_TRANS3)) {
        snprintf(version, sizeof(version), "ANS 3.0");
      } else if (char_plr_flagged(ch, PLR_TRANS4)) {
        snprintf(version, sizeof(version), "ANS 4.0");
      } else if (char_plr_flagged(ch, PLR_TRANS5)) {
        snprintf(version, sizeof(version), "ANS 5.0");
      } else if (char_plr_flagged(ch, PLR_TRANS6)) {
        snprintf(version, sizeof(version), "ANS 6.0");
      } else {
        snprintf(version, sizeof(version), "Alpha 0.5");
      }
      send_to_char(g, ch, "  @D| @CModel@D: %15s@D,    @CUGP@D: @G%15s@D,  @CVersion@D: @r%-12s@D|@n\n", model, absorb > 0 ? "@RN/A" : add_commas(char_get_upgrade(ch)), version);
    }
    send_to_char(g, ch, "  @D|  @CRace@D: @W%10s@D,  @CSensei@D: @W%15s@D,     @CArt@D: @W%-17s@D|@n\n", char_true_race_name(ch), char_sensei_name(ch), char_sensei_style(ch));
    char hei[300], wei[300];
    snprintf(hei, sizeof(hei), "%dcm", get_measure(ch, char_get_pc_height(g, ch), 0));
    snprintf(wei, sizeof(wei), "%dkg", get_measure(ch, 0, char_get_pc_weight(g, ch)));
    send_to_char(g, ch, "  @D|   @CAge@D: @W%10s@D,  @CHeight@D: @W%15s@D,  @CWeight@D: @W%-17s@D|@n\n", add_commas(char_get_age_years(g, ch)), hei, wei);
    send_to_char(g, ch, "  @D|@CGender@D: @W%10s@D,  @C  Size@D: @W%15s@D,  @C Align@D: @W%-17s@D|@n\n", genders[(int)char_get_sex(ch)], size_names[get_size(ch)], disp_align(ch));
  }
  if (view == full || view == health) {
    send_to_char(g, ch, "  @cO@D-----------------------------@D[   @cHealth   @D]-----------------------------@cO@n\n");
    send_to_char(g, ch, "                 @D<@rPowerlevel@D>          <@BKi@D>             <@GStamina@D>@n\n");
    send_to_char(g, ch, "    @wCurrent   @D-[@R%-16s@D]-[@R%-16s@D]-[@R%-16s@D]@n\n", add_commas(getCurPL(g, ch)), add_commas((getCurKI(g, ch))), add_commas((getCurST(g, ch))));
    send_to_char(g, ch, "    @wMaximum   @D-[@r%-16s@D]-[@r%-16s@D]-[@r%-16s@D]@n\n", add_commas(getEffMaxPL(g, ch)), add_commas(getMaxKI(g, ch)), add_commas(getMaxST(g, ch)));
    send_to_char(g, ch, "    @wBase      @D-[@m%-16s@D]-[@m%-16s@D]-[@m%-16s@D]@n\n", add_commas((getEffBasePL(ch))), add_commas((getEffBaseKI(ch))), add_commas((getEffBaseST(ch))));
    if (!char_is_android(ch) && (getCurLF(g, ch)) > 0) {
      send_to_char(g, ch, "    @wLife Force@D-[@C%16s@D%s@c%16s@D]- @wLife Percent@D-[@Y%3d%s@D]@n\n", add_commas((getCurLF(g, ch))), "/", add_commas((getMaxLF(g, ch))), char_get_lifeperc(ch), "%");
    } else if (!char_is_android(ch)) {
      send_to_char(g, ch, "    @wLife Force@D-[@C%16s@D%s@c%16s@D]- @wLife Percent@D-[@Y%3d%s@D]@n\n", add_commas(0), "/", add_commas((getMaxLF(g, ch))), char_get_lifeperc(ch), "%");
    }
  }
  if (view == full || view == stats) {
    send_to_char(g, ch, "  @cO@D-----------------------------@D[ @cStatistics @D]-----------------------------@cO@n\n");
    send_to_char(g, ch, "      @D<@wCharacter Level@D: @w%-3d@D> <@wRPP@D: @w%-3d@D> <@wRPP Bank@D: @w%-3d@D>@n\n", char_get_level(ch), char_get_rp(ch), char_get_rbank(ch));
    send_to_char(g, ch, "      @D<@wSpeed Index@D: @w%-15s@D> <@wArmor Index@D: @w%-15s@D>@n\n", add_commas(char_get_speedi(g, ch)), add_commas(char_get_armor(ch)));
    send_to_char(g, ch, "    @D[    @RStrength@D|@G%2d (%3d)@D] [     @YAgility@D|@G%2d (%3d)@D] [      @BSpeed@D|@G%2d (%3d)@D]@n\n", char_get_real_abils_str(ch), char_get_aff_abils_str(ch), char_get_real_abils_dex(ch), char_get_aff_abils_dex(ch), char_get_real_abils_cha(ch), char_get_aff_abils_cha(ch));
    send_to_char(g, ch, "    @D[@gConstitution@D|@G%2d (%3d)@D] [@CIntelligence@D|@G%2d (%3d)@D] [     @MWisdom@D|@G%2d (%3d)@D]@n\n", char_get_real_abils_con(ch), char_get_aff_abils_con(ch), char_get_real_abils_intel(ch), char_get_aff_abils_intel(ch), char_get_real_abils_wis(ch), char_get_aff_abils_wis(ch));
  }
  if (view == full || view == other) {
    send_to_char(g, ch, "  @cO@D-----------------------------@D[   @cOther    @D]-----------------------------@cO@n\n");
    send_to_char(g, ch, "                @D<@YZenni@D>                 <@rInventory Weight@D>@n\n");
    send_to_char(g, ch, "      @D[   @CCarried@D| @W%-15s@D] [   @CCarried@D| @W%-15s@D]@n\n", add_commas(char_get_gold(ch)), add_commas((getCurCarriedWeight(ch))));
    send_to_char(g, ch, "      @D[      @CBank@D| @W%-15s@D] [ @CMax Carry@D| @W%-15s@D]@n\n", add_commas(char_get_bank_gold(ch)), add_commas(getMaxCarryWeight(g, ch)));
    send_to_char(g, ch, "      @D[ @CMax Carry@D| @W%-15s@D]@n\n", add_commas(char_gold_carry(ch)));
    int numb = char_get_bank_interest(ch);
    send_to_char(g, ch, "      @D[  @CInterest@D| @W%-15s@D]\n", add_commas(numb));
    if (char_is_arlian(ch)) {
      send_to_char(g, ch, "                             @D<@GEvolution @D>@n\n");
      send_to_char(g, ch, "      @D[ @CEvo Level@D| @W%-15d@D] [   @CEvo Exp@D| @W%-15s@D]\n", char_get_moltlevel(ch), add_commas(char_get_moltexp(ch)));
      send_to_char(g, ch, "      @D[ @CThreshold@D| @W%-15s@D]@n\n", add_commas(molt_threshold(ch)));
    }
    if (char_get_level(ch) < 100) {
      send_to_char(g, ch, "                             @D<@gAdvancement@D>@n\n");
    }
    if (char_get_level(ch) < 100) {
      send_to_char(g, ch, "      @D[@CExperience@D| @W%-15s@D] [@CNext Level@D| @W%-15s@D]@n\n", add_commas(char_get_exp(ch)), add_commas(level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch)));
      send_to_char(g, ch, "      @D[  @CRpp Cost@D| @W%-15d@D]@n\n", rpp_to_level(ch));
    }

    send_to_char(g, ch, "\n     @D<@wPlayed@D: @yYears @D(@W%2d@D) @yWeeks @D(@W%2d@D) @yDays @D(@W%2d@D) @yHours @D(@W%2d@D) @yMinutes @D(@W%2d@D)>@n\n", (int)char_get_time_played(ch) / 31536000, (int)((char_get_time_played(ch) % 31536000) / 604800), (int)((char_get_time_played(ch) % 604800) / 86400), (int)((char_get_time_played(ch) % 86400) / 3600), (int)((char_get_time_played(ch) % 3600) / 60));
  }
  send_to_char(g, ch, "  @cO@D------------------------------------------------------------------------@cO@n\n");
}

void trans_check(GameWorld *g, struct char_data *ch, struct char_data *vict)
{
  /* Rillao: transloc, add new transes here */
  if (char_is_human(vict)) {
    if (char_plr_flagged(vict, PLR_TRANS1)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Human First@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS2)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Human Second@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS3)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Human Third@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS4)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Human Fourth@n\r\n");
    } else {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (char_is_hoshijin(vict)) {
    if (char_get_mimic(vict) == 0 || vict == ch) {
      if (char_get_starphase(vict) == 1) {
        send_to_char(g, ch, "         @cCurrent Transformation@D: @CBirth Phase@n\r\n");
      } else if (char_get_starphase(vict) == 2) {
        send_to_char(g, ch, "         @cCurrent Transformation@D: @CLife Phase@n\r\n");
      } else {
        send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
      }
    } else {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if ((char_is_saiyan(vict) || char_is_halfbreed(vict)) && !char_plr_flagged(vict, PLR_LSSJ)) {
    if (char_plr_flagged(vict, PLR_TRANS1) && !char_plr_flagged(vict, PLR_FPSSJ)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Saiyan First@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS1) && char_plr_flagged(vict, PLR_FPSSJ)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @YFull Powered @CSuper Saiyan@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS2)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Saiyan Second@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS3)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Saiyan Third@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS4)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Saiyan Fourth@n\r\n");
    } else if (char_plr_flagged(vict, PLR_OOZARU)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @COozaru@n\r\n");
    } else {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (char_is_saiyan(vict) && char_plr_flagged(vict, PLR_LSSJ)) {
    if (char_plr_flagged(vict, PLR_TRANS1) && !char_plr_flagged(vict, PLR_FPSSJ)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Saiyan First@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS1) && char_plr_flagged(vict, PLR_FPSSJ)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @YFull Powered @CSuper Saiyan@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS2)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @YLegendary @CSuper Saiyan@n\r\n");
    } else if (char_plr_flagged(vict, PLR_OOZARU)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @COozaru@n\r\n");
    } else {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (char_is_namek(vict)) {
    if (char_plr_flagged(vict, PLR_TRANS1)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Namek First@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS2)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Namek Second@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS3)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Namek Third@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS4)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Namek Fourth@n\r\n");
    } else {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (char_is_icer(vict)) {
    if (char_plr_flagged(vict, PLR_TRANS1)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CTransform First@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS2)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CTransform Second@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS3)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CTransform Third@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS4)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CTransform Fourth@n\r\n");
    } else {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (char_is_konatsu(vict)) {
    if (char_plr_flagged(vict, PLR_TRANS1)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CShadow First@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS2)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CShadow Second@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS3)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CShadow Third@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS4)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CShadow Fourth@n\r\n");
    } else {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (char_is_mutant(vict)) {
    if (char_plr_flagged(vict, PLR_TRANS1)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CMutate First@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS2)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CMutate Second@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS3)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CMutate Third@n\r\n");
    } else {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (char_is_bio(vict)) {
    if (char_plr_flagged(vict, PLR_TRANS1)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CMature@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS2)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSemi-perfect@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS3)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CPerfect@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS4)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper Perfect@n\r\n");
    } else {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (char_is_android(vict)) {
    if (char_plr_flagged(vict, PLR_TRANS1)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSeries 1.0@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS2)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSeries 2.0@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS3)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSeries 3.0@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS4)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSeries 4.0@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS5)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSeries 5.0@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS6)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSeries 6.0@n\r\n");
    } else {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (char_is_majin(vict)) {
    if (char_plr_flagged(vict, PLR_TRANS1)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CAffinity@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS2)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CSuper@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS3)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CTrue@n\r\n");
    } else {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (char_is_truffle(vict)) {
    if (char_plr_flagged(vict, PLR_TRANS1)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CAscend First@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS2)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CAscend Second@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS3)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CAscend Third@n\r\n");
    } else {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else if (char_is_kai(vict)) {
    if (char_plr_flagged(vict, PLR_TRANS1)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CMystic First@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS2)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CMystic Second@n\r\n");
    } else if (char_plr_flagged(vict, PLR_TRANS3)) {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @CMystic Third@n\r\n");
    } else {
      send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
    }
  } else {
    send_to_char(g, ch, "         @cCurrent Transformation@D: @wNone@n\r\n");
  }

} // End trans check

ACMD(do_status)
{
  char arg[MAX_INPUT_LENGTH];
  struct affected_type *aff;

  const char *forget_level[7] = {
      "@GRemembered Well@n",
      "@GRemembered Well Enough@n",
      "@RGetting Foggy@n",
      "@RHalf Forgotten@n",
      "@rAlmost Forgotten@n",
      "@rForgotten@n",
      "\n"};

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(g, ch, "@D<@b------------------------@D[@YYour Status@D]@b-------------------------@D>@n\r\n\r\n");
    send_to_char(g, ch, "            @D---------------@CAppearance@D---------------\n");
    bringdesc(g, ch, ch);
    send_to_char(g, ch, "            @D---------------@RAppendages@D---------------\n");

    if (char_plr_flagged(ch, PLR_HEAD)) {
      send_to_char(g, ch, "            @D[@cHead        @D: @GHave.          @D]@n\r\n");
    }
    if (!char_plr_flagged(ch, PLR_HEAD)) {
      send_to_char(g, ch, "            @D[@cHead        @D: @rMissing.         @D]@n\r\n");
    }
    if (char_get_limb_condition(ch, (1) - 1) >= 50 && !char_plr_flagged(ch, PLR_CRARM)) {
      send_to_char(g, ch, "            @D[@cRight Arm   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(ch, (1) - 1), "%", "%");
    } else if (char_get_limb_condition(ch, (1) - 1) > 0 && !char_plr_flagged(ch, PLR_CRARM)) {
      send_to_char(g, ch, "            @D[@cRight Arm   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(ch, (1) - 1), "%", "%");
    } else if (char_get_limb_condition(ch, (1) - 1) > 0 && char_plr_flagged(ch, PLR_CRARM)) {
      send_to_char(g, ch, "            @D[@cRight Arm   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(ch, (1) - 1), "%", "%");
    } else if (char_get_limb_condition(ch, (1) - 1) <= 0) {
      send_to_char(g, ch, "            @D[@cRight Arm   @D: @rMissing.         @D]@n\r\n");
    }
    if (char_get_limb_condition(ch, (2) - 1) >= 50 && !char_plr_flagged(ch, PLR_CLARM)) {
      send_to_char(g, ch, "            @D[@cLeft Arm    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(ch, (2) - 1), "%", "%");
    } else if (char_get_limb_condition(ch, (2) - 1) > 0 && !char_plr_flagged(ch, PLR_CLARM)) {
      send_to_char(g, ch, "            @D[@cLeft Arm    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(ch, (2) - 1), "%", "%");
    } else if (char_get_limb_condition(ch, (2) - 1) > 0 && char_plr_flagged(ch, PLR_CLARM)) {
      send_to_char(g, ch, "            @D[@cLeft Arm    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(ch, (2) - 1), "%", "%");
    } else if (char_get_limb_condition(ch, (2) - 1) <= 0) {
      send_to_char(g, ch, "            @D[@cLeft Arm    @D: @rMissing.         @D]@n\r\n");
    }
    if (char_get_limb_condition(ch, (3) - 1) >= 50 && !char_plr_flagged(ch, PLR_CLARM)) {
      send_to_char(g, ch, "            @D[@cRight Leg   @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(ch, (3) - 1), "%", "%");
    } else if (char_get_limb_condition(ch, (3) - 1) > 0 && !char_plr_flagged(ch, PLR_CRLEG)) {
      send_to_char(g, ch, "            @D[@cRight Leg   @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(ch, (3) - 1), "%", "%");
    } else if (char_get_limb_condition(ch, (3) - 1) > 0 && char_plr_flagged(ch, PLR_CRLEG)) {
      send_to_char(g, ch, "            @D[@cRight Leg   @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(ch, (3) - 1), "%", "%");
    } else if (char_get_limb_condition(ch, (3) - 1) <= 0) {
      send_to_char(g, ch, "            @D[@cRight Leg   @D: @rMissing.         @D]@n\r\n");
    }
    if (char_get_limb_condition(ch, (4) - 1) >= 50 && !char_plr_flagged(ch, PLR_CLLEG)) {
      send_to_char(g, ch, "            @D[@cLeft Leg    @D: @G%2d%s@D/@g100%s        @D]@n\r\n", char_get_limb_condition(ch, (4) - 1), "%", "%");
    } else if (char_get_limb_condition(ch, (4) - 1) > 0 && !char_plr_flagged(ch, PLR_CLLEG)) {
      send_to_char(g, ch, "            @D[@cLeft Leg    @D: @rBroken @y%2d%s@D/@g100%s @D]@n\r\n", char_get_limb_condition(ch, (4) - 1), "%", "%");
    } else if (char_get_limb_condition(ch, (4) - 1) > 0 && char_plr_flagged(ch, PLR_CLLEG)) {
      send_to_char(g, ch, "            @D[@cLeft Leg    @D: @cCybernetic @G%2d%s@D/@G100%s@D]@n\r\n", char_get_limb_condition(ch, (4) - 1), "%", "%");
    } else if (char_get_limb_condition(ch, (4) - 1) <= 0) {
      send_to_char(g, ch, "            @D[@cLeft Leg    @D: @rMissing.         @D]@n\r\n");
    }

    if ((char_is_saiyan(ch) || char_is_halfbreed(ch)) && char_plr_flagged(ch, PLR_STAIL)) {
      send_to_char(g, ch, "            @D[@cTail        @D: @GHave.            @D]@n\r\n");
    }
    if ((char_is_saiyan(ch) || char_is_halfbreed(ch)) && !char_plr_flagged(ch, PLR_STAIL)) {
      send_to_char(g, ch, "            @D[@cTail        @D: @rMissing.         @D]@n\r\n");
    }
    if ((char_is_icer(ch) || char_is_bio(ch)) && char_plr_flagged(ch, PLR_TAIL)) {
      send_to_char(g, ch, "            @D[@cTail        @D: @GHave.            @D]@n\r\n");
    }
    if ((char_is_icer(ch) || char_is_bio(ch)) && !char_plr_flagged(ch, PLR_TAIL)) {
      send_to_char(g, ch, "            @D[@cTail        @D: @rMissing.         @D]@n\r\n");
    }
    send_to_char(g, ch, "\r\n");

    send_to_char(g, ch, "         @D-----------------@YHunger@D/@yThirst@D-----------------@n\r\n");
    if (char_get_conditions(g, ch, HUNGER) >= 48) {
      send_to_char(g, ch, "         You are full.\r\n");
    } else if (char_get_conditions(g, ch, HUNGER) >= 40) {
      send_to_char(g, ch, "         You are nearly full.\r\n");
    } else if (char_get_conditions(g, ch, HUNGER) >= 30) {
      send_to_char(g, ch, "         You are not hungry.\r\n");
    } else if (char_get_conditions(g, ch, HUNGER) >= 21) {
      send_to_char(g, ch, "         You wouldn't mind a snack.\r\n");
    } else if (char_get_conditions(g, ch, HUNGER) >= 15) {
      send_to_char(g, ch, "         You are slightly hungry.\r\n");
    } else if (char_get_conditions(g, ch, HUNGER) >= 10) {
      send_to_char(g, ch, "         You are partially hungry.\r\n");
    } else if (char_get_conditions(g, ch, HUNGER) >= 5) {
      send_to_char(g, ch, "         You are really hungry.\r\n");
    } else if (char_get_conditions(g, ch, HUNGER) >= 2) {
      send_to_char(g, ch, "         You are extremely hungry.\r\n");
    } else if (char_get_conditions(g, ch, HUNGER) >= 0) {
      send_to_char(g, ch, "         You are starving!\r\n");
    } else if (char_get_conditions(g, ch, HUNGER) < 0) {
      send_to_char(g, ch, "         You need not eat.\r\n");
    }
    if (char_get_conditions(g, ch, THIRST) >= 48) {
      send_to_char(g, ch, "         You are not thirsty.\r\n");
    } else if (char_get_conditions(g, ch, THIRST) >= 40) {
      send_to_char(g, ch, "         You are nearly quenched.\r\n");
    } else if (char_get_conditions(g, ch, THIRST) >= 30) {
      send_to_char(g, ch, "         You are not thirsty.\r\n");
    } else if (char_get_conditions(g, ch, THIRST) >= 21) {
      send_to_char(g, ch, "         You wouldn't mind a drink.\r\n");
    } else if (char_get_conditions(g, ch, THIRST) >= 15) {
      send_to_char(g, ch, "         You are slightly thirsty.\r\n");
    } else if (char_get_conditions(g, ch, THIRST) >= 10) {
      send_to_char(g, ch, "         You are partially thirsty.\r\n");
    } else if (char_get_conditions(g, ch, THIRST) >= 5) {
      send_to_char(g, ch, "         You are really thirsty.\r\n");
    } else if (char_get_conditions(g, ch, THIRST) >= 2) {
      send_to_char(g, ch, "         You are extremely thirsty.\r\n");
    } else if (char_get_conditions(g, ch, THIRST) >= 0) {
      send_to_char(g, ch, "         You are dehydrated!\r\n");
    } else if (char_get_conditions(g, ch, THIRST) < 0) {
      send_to_char(g, ch, "         You need not drink.\r\n");
    }
    send_to_char(g, ch, "         @D--------------------@D[@GInfo@D]---------------------@n\r\n");
    trans_check(g, ch, ch);
    send_to_char(g, ch, "         You have died %d times.\r\n", char_get_dcount(ch));
    if (char_plr_flagged(ch, PLR_NOSHOUT)) {
      send_to_char(g, ch, "         You have been @rmuted@n on public channels.\r\n");
    }
    if (char_get_in_room(ch) == real_room(g, 9)) {
      send_to_char(g, ch, "         You are in punishment hell, so sad....\r\n");
    }
    if (!char_prf_flagged(g, ch, PRF_HINTS)) {
      send_to_char(g, ch, "         You have hints turned off.\r\n");
    }
    if (g->NEWSUPDATE > char_get_lastpl(ch)) {
      send_to_char(g, ch, "         Check the 'news', it has been updated recently.\r\n");
    }
    if (has_mail(g, char_get_idnum(ch))) {
      send_to_char(g, ch, "         Check your mail at the nearest postmaster.\r\n");
    }
    if (char_prf_flagged(g, ch, PRF_HIDE)) {
      send_to_char(g, ch, "         You are hidden from who and ooc.\r\n");
    }
    if (char_get_voice(ch) != nullptr) {
      send_to_char(g, ch, "         Your voice desc: '%s'\r\n", char_get_voice(ch));
    }
    if (char_get_distfea(ch) == DISTFEA_EYE) {
      send_to_char(g, ch, "         Your eyes are your most distinctive feature.\r\n");
    }
    if (char_get_preference(ch) == 0) {
      send_to_char(g, ch, "         You preferred a balanced form of fighting.\r\n");
    } else if (char_get_preference(ch) == PREFERENCE_KI) {
      send_to_char(g, ch, "         You preferred a ki dominate form of fighting.\r\n");
    } else if (char_get_preference(ch) == PREFERENCE_WEAPON) {
      send_to_char(g, ch, "         You preferred a weapon dominate form of fighting.\r\n");
    } else if (char_get_preference(ch) == PREFERENCE_H2H) {
      send_to_char(g, ch, "         You preferred a body dominate form of fighting.\r\n");
    } else if (char_get_preference(ch) == PREFERENCE_THROWING) {
      send_to_char(g, ch, "         You preferred a throwing dominate form of fighting.\r\n");
    }
    if (char_get_distfea(ch) == DISTFEA_HAIR && !char_is_demon(ch) && !char_is_majin(ch) && !char_is_icer(ch) && !char_is_namek(ch)) {
      send_to_char(g, ch, "         Your hair is your most distinctive feature.\r\n");
    } else if (char_get_distfea(ch) == DISTFEA_HAIR && char_is_demon(ch)) {
      send_to_char(g, ch, "         Your horns are your most distinctive feature.\r\n");
    } else if (char_get_distfea(ch) == DISTFEA_HAIR && char_is_majin(ch)) {
      send_to_char(g, ch, "         Your forelock is your most distinctive feature.\r\n");
    } else if (char_get_distfea(ch) == DISTFEA_HAIR && char_is_icer(ch)) {
      send_to_char(g, ch, "         Your horns are your most distinctive feature.\r\n");
    } else if (char_get_distfea(ch) == DISTFEA_HAIR && char_is_namek(ch)) {
      send_to_char(g, ch, "         Your antennae are your most distinctive feature.\r\n");
    }

    if (char_get_distfea(ch) == DISTFEA_SKIN) {
      send_to_char(g, ch, "         Your skin is your most distinctive feature.\r\n");
    }
    if (char_get_distfea(ch) == DISTFEA_HEIGHT) {
      send_to_char(g, ch, "         Your height is your most distinctive feature.\r\n");
    }
    if (char_get_distfea(ch) == DISTFEA_WEIGHT) {
      send_to_char(g, ch, "         Your weight is your most distinctive feature.\r\n");
    }
    if (char_get_equipment(ch, WEAR_EYE)) {
      struct obj_data *obj = char_get_equipment(ch, WEAR_EYE);
      if (obj_get_scoutfreq(obj) == 0) {
        obj_set_scoutfreq(obj, 1);
      }
      send_to_char(g, ch, "         Your scouter is on frequency @G%d@n\r\n", obj_get_scoutfreq(obj));
      obj = nullptr;
    }
    if (char_get_charge(ch) > 0) {
      send_to_char(g, ch, "         You have @C%s@n ki charged.\r\n", add_commas(char_get_charge(ch)));
    }
    if (char_get_kaioken(ch) > 0) {
      send_to_char(g, ch, "         You are focusing Kaioken x %d.\r\n", char_get_kaioken(ch));
    }
    if (char_aff_flagged(ch, AFF_SANCTUARY)) {
      send_to_char(g, ch, "         You are surrounded by a barrier @D(@Y%s@D)@n\r\n", add_commas(char_get_barrier(ch)));
    }
    if (char_aff_flagged(ch, AFF_FIRESHIELD)) {
      send_to_char(g, ch, "         You are surrounded by flames!@n\r\n");
    }
    if (char_get_suppression(ch) > 0) {
      send_to_char(g, ch, "         You are suppressing current PL to %" PRId64 ".\r\n", char_get_suppression(ch));
    }
    if (char_is_majin(ch)) {
      send_to_char(g, ch, "         You have ingested %d people.\r\n", char_get_absorbs(ch));
    }
    if (char_is_bio(ch)) {
      send_to_char(g, ch, "         You have %d absorbs left.\r\n", char_get_absorbs(ch));
    }
    send_to_char(g, ch, "         You have %s colored aura.\r\n", aura_types[char_get_aura(ch)]);

    if (char_get_level(ch) < 100) {
      if ((char_is_android(ch) && char_plr_flagged(ch, PLR_ABSORB)) || (!char_is_android(ch) && !char_is_bio(ch) && !char_is_majin(ch))) {
        send_to_char(g, ch, "         @R%s@n to SC a stat this level.\r\n", add_commas(calc_soft_cap(ch)));
      } else {
        send_to_char(g, ch, "         @R%s@n in PL/KI/ST combined to SC this level.\r\n", add_commas(calc_soft_cap(ch)));
      }
    } else {
      send_to_char(g, ch, "         Your strengths are potentially limitless.\r\n");
    }

    if (char_get_forgeting(ch) != 0) {
      send_to_char(g, ch, "         @MForgetting @D[@m%s - %s@D]@n\r\n", g->spell_info[char_get_forgeting(ch)].name, forget_level[char_get_forgetcount(ch)]);
    } else {
      send_to_char(g, ch, "         @MForgetting @D[@mNothing.@D]@n\r\n");
    }

    if (char_get_skill(ch, SKILL_DAGGER) > 0) {
      if (char_get_backstabcool(ch) > 0) {
        send_to_char(g, ch, "         @yYou can't preform a backstab yet.@n\r\n");
      } else {
        send_to_char(g, ch, "         @YYou can backstab.@n\r\n");
      }
    }

    if (char_get_feature(ch)) {
      send_to_char(g, ch, "         Extra Feature: @C%s@n\r\n", char_get_feature(ch));
    }

    if (char_get_rdisplay(ch)) {
      if (strcmp(char_get_rdisplay(ch), "Empty")) {
        send_to_char(g, ch, "         Room Display: @C...%s@n\r\n", char_get_rdisplay(ch));
      }
    }

    send_to_char(g, ch, "\r\n@D<@b-------------------------@D[@BCondition@D]@b--------------------------@D>@n\r\n");

    if (char_get_bonuses(ch, BONUS_INSOMNIAC)) {
      send_to_char(g, ch, "You can not sleep.\r\n");
    } else {
      if (char_get_sleeptime(ch) > 6 && char_get_position(ch) != POS_SLEEPING) {
        send_to_char(g, ch, "You are well rested.\r\n");
      } else if (char_get_sleeptime(ch) > 6 && char_get_position(ch) == POS_SLEEPING) {
        send_to_char(g, ch, "You are getting the rest you need.\r\n");
      } else if (char_get_sleeptime(ch) > 4) {
        send_to_char(g, ch, "You are rested.\r\n");
      } else if (char_get_sleeptime(ch) > 2) {
        send_to_char(g, ch, "You are not sleepy.\r\n");
      } else if (char_get_sleeptime(ch) >= 1) {
        send_to_char(g, ch, "You are getting a little sleepy.\r\n");
      } else if (char_get_sleeptime(ch) == 0) {
        send_to_char(g, ch, "You could sleep at any time.\r\n");
      }
    }

    if (char_get_relax_count(ch) > 464) {
      send_to_char(g, ch, "You are far too at ease to train hard like you should. Get out of the house more often.\r\n");
    } else if (char_get_relax_count(ch) > 232) {
      send_to_char(g, ch, "You are too at ease to train hard like you should. Get out of the house more often.\r\n");
    } else if (char_get_relax_count(ch) > 116) {
      send_to_char(g, ch, "You are a bit at ease and your training suffers. Get out of the house more often.\r\n");
    }

    if (char_get_mimic(ch) > 0) {
      send_to_char(g, ch, "You are mimicing the general appearance of %s %s\r\n", AN(juggleRaceName(ch, false)), juggleRaceName(ch, false));
    }
    if (char_is_mutant(ch)) {
      send_to_char(g, ch, "Your Mutations:\r\n");
      if (char_get_genome(ch, 0) == 1) {
        send_to_char(g, ch, "  Extreme Speed.\r\n");
      }
      if (char_get_genome(ch, 0) == 2) {
        send_to_char(g, ch, "  Increased Cell Regeneration.\r\n");
      }
      if (char_get_genome(ch, 0) == 3) {
        send_to_char(g, ch, "  Extreme Reflexes.\r\n");
      }
      if (char_get_genome(ch, 0) == 4) {
        send_to_char(g, ch, "  Infravision.\r\n");
      }
      if (char_get_genome(ch, 0) == 5) {
        send_to_char(g, ch, "  Natural Camo.\r\n");
      }
      if (char_get_genome(ch, 0) == 6) {
        send_to_char(g, ch, "  Limb Regen.\r\n");
      }
      if (char_get_genome(ch, 0) == 7) {
        send_to_char(g, ch, "  Poisonous (you can use the bite attack).\r\n");
      }
      if (char_get_genome(ch, 0) == 8) {
        send_to_char(g, ch, "  Rubbery Body.\r\n");
      }
      if (char_get_genome(ch, 0) == 9) {
        send_to_char(g, ch, "  Innate Telepathy.\r\n");
      }
      if (char_get_genome(ch, 0) == 10) {
        send_to_char(g, ch, "  Natural Energy.\r\n");
      }
      if (char_get_genome(ch, 1) == 1) {
        send_to_char(g, ch, "  Extreme Speed.\r\n");
      }
      if (char_get_genome(ch, 1) == 2) {
        send_to_char(g, ch, "  Increased Cell Regeneration.\r\n");
      }
      if (char_get_genome(ch, 1) == 3) {
        send_to_char(g, ch, "  Extreme Reflexes.\r\n");
      }
      if (char_get_genome(ch, 1) == 4) {
        send_to_char(g, ch, "  Infravision.\r\n");
      }
      if (char_get_genome(ch, 1) == 5) {
        send_to_char(g, ch, "  Natural Camo.\r\n");
      }
      if (char_get_genome(ch, 1) == 6) {
        send_to_char(g, ch, "  Limb Regen.\r\n");
      }
      if (char_get_genome(ch, 1) == 7) {
        send_to_char(g, ch, "  Poisonous (you can use the bite attack).\r\n");
      }
      if (char_get_genome(ch, 1) == 8) {
        send_to_char(g, ch, "  Rubbery Body.\r\n");
      }
      if (char_get_genome(ch, 1) == 9) {
        send_to_char(g, ch, "  Innate Telepathy.\r\n");
      }
      if (char_get_genome(ch, 1) == 10) {
        send_to_char(g, ch, "  Natural Energy.\r\n");
      }
    }
    if (char_is_bio(ch)) {
      send_to_char(g, ch, "Your genes carry:\r\n");
      if (char_get_genome(ch, 0) == 1) {
        send_to_char(g, ch, "  Human DNA.\r\n");
      }
      if (char_get_genome(ch, 0) == 2) {
        send_to_char(g, ch, "  Saiyan DNA.\r\n");
      }
      if (char_get_genome(ch, 0) == 3) {
        send_to_char(g, ch, "  Namek DNA.\r\n");
      }
      if (char_get_genome(ch, 0) == 4) {
        send_to_char(g, ch, "  Icer DNA.\r\n");
      }
      if (char_get_genome(ch, 0) == 5) {
        send_to_char(g, ch, "  Truffle DNA.\r\n");
      }
      if (char_get_genome(ch, 0) == 6) {
        send_to_char(g, ch, "  Arlian DNA.\r\n");
      }
      if (char_get_genome(ch, 0) == 7) {
        send_to_char(g, ch, "  Kai DNA.\r\n");
      }
      if (char_get_genome(ch, 0) == 8) {
        send_to_char(g, ch, "  Konatsu DNA.\r\n");
      }
      if (char_get_genome(ch, 1) == 1) {
        send_to_char(g, ch, "  Human DNA.\r\n");
      }
      if (char_get_genome(ch, 1) == 2) {
        send_to_char(g, ch, "  Saiyan DNA.\r\n");
      }
      if (char_get_genome(ch, 1) == 3) {
        send_to_char(g, ch, "  Namek DNA.\r\n");
      }
      if (char_get_genome(ch, 1) == 4) {
        send_to_char(g, ch, "  Icer DNA.\r\n");
      }
      if (char_get_genome(ch, 1) == 5) {
        send_to_char(g, ch, "  Truffle DNA.\r\n");
      }
      if (char_get_genome(ch, 1) == 6) {
        send_to_char(g, ch, "  Arlian DNA.\r\n");
      }
      if (char_get_genome(ch, 1) == 7) {
        send_to_char(g, ch, "  Kai DNA.\r\n");
      }
      if (char_get_genome(ch, 1) == 8) {
        send_to_char(g, ch, "  Konatsu DNA.\r\n");
      }
    }
    if (char_get_genome(ch, 0) == 11) {
      send_to_char(g, ch, "You have used kyodaika.\r\n");
    }
    if (char_prf_flagged(g, ch, PRF_NOPARRY)) {
      send_to_char(g, ch, "You have decided not to parry attacks.\r\n");
    }
    switch (char_get_position(ch)) {
      case POS_DEAD:
        send_to_char(g, ch, "You are DEAD!\r\n");
        break;
      case POS_MORTALLYW:
        send_to_char(g, ch, "You are mortally wounded! You should seek help!\r\n");
        break;
      case POS_INCAP:
        send_to_char(g, ch, "You are incapacitated, slowly fading away...\r\n");
        break;
      case POS_STUNNED:
        send_to_char(g, ch, "You are stunned! You can't move!\r\n");
        break;
      case POS_SLEEPING:
        send_to_char(g, ch, "You are sleeping.\r\n");
        break;
      case POS_RESTING:
        send_to_char(g, ch, "You are resting.\r\n");
        break;
      case POS_SITTING:
        send_to_char(g, ch, "You are sitting.\r\n");
        break;
      case POS_FIGHTING:
        send_to_char(g, ch, "You are fighting %s.\r\n", char_get_fighting(ch) ? char_pers(g, char_get_fighting(ch), ch) : "thin air");
        break;
      case POS_STANDING:
        send_to_char(g, ch, "You are standing.\r\n");
        break;
      default:
        send_to_char(g, ch, "You are floating.\r\n");
        break;
    }

    if (has_group(ch)) {
      send_to_char(g, ch, "@GGroup Victories@D: @w%s@n\r\n", add_commas(char_get_combatexpertise(ch)));
    }

    if (char_plr_flagged(ch, PLR_EYEC)) {
      send_to_char(g, ch, "Your eyes are closed.\r\n");
    }
    if (char_aff_flagged(ch, AFF_SNEAK)) {
      send_to_char(g, ch, "You are prepared to sneak where ever you go.\r\n");
    }
    if (char_plr_flagged(ch, PLR_DISGUISED)) {
      send_to_char(g, ch, "You have disguised your facial features.\r\n");
    }
    if (char_aff_flagged(ch, AFF_FLYING)) {
      send_to_char(g, ch, "You are flying.\r\n");
    }
    if (char_plr_flagged(ch, PLR_PILOTING)) {
      send_to_char(g, ch, "You are busy piloting a ship.\r\n");
    }
    if (char_get_powerattack(ch) > 0) {
      send_to_char(g, ch, "You are playing @y'@Y%s@y'@n.\r\n", song_types[char_get_powerattack(ch)]);
    }

    if (char_aff_flagged(ch, AFF_ZANZOKEN)) {
      send_to_char(g, ch, "You are prepared to zanzoken.\r\n");
    }
    if (char_aff_flagged(ch, AFF_HASS)) {
      send_to_char(g, ch, "Your arms are moving fast.\r\n");
    }
    if (char_aff_flagged(ch, AFF_INFUSE)) {
      send_to_char(g, ch, "Your ki will be infused in your next physical attack.\r\n");
    }
    if (char_plr_flagged(ch, PLR_TAILHIDE)) {
      send_to_char(g, ch, "Your tail is hidden!\r\n");
    }
    if (char_plr_flagged(ch, PLR_NOGROW)) {
      send_to_char(g, ch, "Your tail is no longer regrowing!\r\n");
    }
    if (is_affected(ch, AFF_SPECIAL_POSE)) {
      send_to_char(g, ch, "You are feeling confident from your pose earlier.\r\n");
    }
    if (is_affected(ch, AFF_HYDROZAP)) {
      send_to_char(g, ch, "You are effected by Kanso Suru.\r\n");
    }
    if (char_get_conditions(g, ch, DRUNK) > 15)
      send_to_char(g, ch, "You are extremely drunk.\r\n");
    else if (char_get_conditions(g, ch, DRUNK) > 10)
      send_to_char(g, ch, "You are pretty drunk.\r\n");
    else if (char_get_conditions(g, ch, DRUNK) > 4)
      send_to_char(g, ch, "You are drunk.\r\n");
    else if (char_get_conditions(g, ch, DRUNK) > 0)
      send_to_char(g, ch, "You have an alcoholic buzz.\r\n");

    if (char_get_affected(ch)) {
      int lasttype = 0;
      for (aff = char_get_affected(ch); aff; aff = aff->next) {
        if (!strcasecmp(skill_name(g, aff->type), "runic") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "Your Kenaz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "punch") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "Your Algiz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "knee") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "Your Oagaz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "slam") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "Your Wunjo rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "heeldrop") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "Your Purisaz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "special beam cannon") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "Your Laguz rune is still in effect! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "might") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "Your muscles are pumped! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "flex") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "You are more agile right now! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "bless") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "You have been blessed! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "curse") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "You have been cursed! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "healing glow") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "You have a healing glow enveloping your body! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "genius") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "You are smarter right now! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "enlighten") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "You are wiser right now! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "yoikominminken") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "You have been lulled to sleep! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "solar flare") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "You have been blinded! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "spirit control") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "You have full control of your spirit! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "!UNUSED!") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "You feel poison burning through your blood! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "tough skin") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "You have toughened skin right now! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "poison") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "You have been poisoned! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "warp pool") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "Weakened State! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "dark metamorphosis") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "Your Dark Metamorphosis is still in effect. (%2d Mud Hours)\r\n", aff->duration + 1);
        }
        if (!strcasecmp(skill_name(g, aff->type), "hayasa") && aff->type != lasttype) {
          lasttype = aff->type;
          send_to_char(g, ch, "Your body has been infused to move faster! (%2d Mud Hours)\r\n", aff->duration + 1);
        }
      }
    }

    if (char_aff_flagged(ch, AFF_KNOCKED))
      send_to_char(g, ch, "You have been knocked unconcious!\r\n");

    if (char_aff_flagged(ch, AFF_INVISIBLE))
      send_to_char(g, ch, "You are invisible.\r\n");

    if (char_aff_flagged(ch, AFF_DETECT_INVIS))
      send_to_char(g, ch, "You are sensitive to the presence of invisible things.\r\n");

    if (char_aff_flagged(ch, AFF_MBREAK))
      send_to_char(g, ch, "Your mind has been broken!\r\n");

    if (char_aff_flagged(ch, AFF_WITHER))
      send_to_char(g, ch, "You've been withered! You feel so weak...\r\n");

    if (char_aff_flagged(ch, AFF_SHOCKED))
      send_to_char(g, ch, "Your mind has been shocked!\r\n");

    if (char_aff_flagged(ch, AFF_CHARM))
      send_to_char(g, ch, "You have been charmed!\r\n");

    if (affected_by_spell(ch, SPELL_MAGE_ARMOR))
      send_to_char(g, ch, "You feel protected.\r\n");

    if (char_aff_flagged(ch, AFF_INFRAVISION))
      send_to_char(g, ch, "You can see in darkness with infravision.\r\n");

    if (char_prf_flagged(g, ch, PRF_SUMMONABLE))
      send_to_char(g, ch, "You are summonable by other players.\r\n");

    if (char_aff_flagged(ch, AFF_DETECT_ALIGN))
      send_to_char(g, ch, "You see into the hearts of others.\r\n");

    if (char_aff_flagged(ch, AFF_DETECT_MAGIC))
      send_to_char(g, ch, "You are sensitive to the magical nature of things.\r\n");

    if (char_aff_flagged(ch, AFF_SPIRIT))
      send_to_char(g, ch, "You have died and are part of the SPIRIT world!\r\n");

    if (char_prf_flagged(g, ch, PRF_NOGIVE))
      send_to_char(g, ch, "You are not accepting items being handed to you right now.\r\n");

    if (char_aff_flagged(ch, AFF_ETHEREAL))
      send_to_char(g, ch, "You are ethereal and cannot interact with normal space!\r\n");

    if (char_get_regen(ch) > 0) {
      send_to_char(g, ch, "Something is augmenting your regen rate by %s%d%s!\r\n", char_get_regen(ch) > 0 ? "+" : "-", char_get_regen(ch), "%");
    }

    if (char_get_asb(ch) > 0) {
      send_to_char(g, ch, "Something is augmenting your auto-skill training rate by %s%d%s!\r\n", char_get_asb(ch) > 0 ? "+" : "-", char_get_asb(ch), "%");
    }

    if (char_get_raw_lifebonus(ch) > 0) {
      send_to_char(g, ch, "Something is augmenting your Life Force Max by %s%d%s!\r\n", char_get_raw_lifebonus(ch) > 0 ? "+" : "-", char_get_raw_lifebonus(ch), "%");
    }

    if (char_plr_flagged(ch, PLR_FISHING))
      send_to_char(g, ch, "Current Fishing Pole Bonus @D[@C%d@D]@n\r\n", char_get_accuracy(ch));

    if (char_plr_flagged(ch, PLR_AURALIGHT))
      send_to_char(g, ch, "Aura Light is active.\r\n");
    send_to_char(g, ch, "@D<@b--------------------------------------------------------------@D>@n\r\n");
    send_to_char(g, ch, "To view your bonus/negative traits enter: status traits\r\n");
  } else if (!strcasecmp(arg, "traits")) {
    bonus_status(g, ch);
  } else {
    send_to_char(g, ch, "The only argument status takes is 'traits'. If you just want your status do not use an argument.\r\n");
  }
}
static const char *const list_bonuses[] = {
    "Thrifty     - -10% Shop Buy Cost and +10% Shop Sell Cost          ",                                                                                                                                                              /* Bonus 0 */
    "Prodigy     - +25% Experience Gained Until Level 80               ",                                                                                                                                                              /* Bonus 1 */
    "Quick Study - Character auto-trains skills faster                 ",                                                                                                                                                              /* Bonus 2 */
    "Die Hard    - Life Force's PL regen doubled, but cost is the same ",                                                                                                                                                              /* Bonus 3 */
    "Brawler     - Physical attacks do 20% more damage                 ",                                                                                                                                                              /* Bonus 4 */
    "Destroyer   - Damaged Rooms act as regen rooms for you            ",                                                                                                                                                              /* Bonus 5 */
    "Hard Worker - Physical activity bonuses + drains less stamina     ",                                                                                                                                                              /* Bonus 6 */
    "Healer      - Heal/First-aid/Vigor/Repair restore +10%            ",                                                                                                                                                              /* Bonus 7 */
    "Loyal       - +20% Experience When Grouped As Follower            ",                                                                                                                                                              /* Bonus 8 */
    "Brawny      - Strength gains +2 every 10 levels, Train STR + 75%  ",                                                                                                                                                              /* Bonus 9 */
    "Scholarly   - Intelligence gains +2 every 10 levels, Train INT + 75%",                                                                                                                                                            /* Bonus 10 */
    "Sage        - Wisdom gains +2 every 10 levels, Train WIS + 75%    ",                                                                                                                                                              /* Bonus 11 */
    "Agile       - Agility gains +2 every 10 levels, Train AGL + 75%   ",                                                                                                                                                              /* Bonus 12 */
    "Quick       - Speed gains +2 every 10 levels, Train SPD + 75%     ",                                                                                                                                                              /* Bonus 13 */
    "Sturdy      - Constitution +2 every 10 levels, Train CON + 75%    ",                                                                                                                                                              /* Bonus 14 */
    "Thick Skin  - -20% Physical and -10% ki dmg received              ",                                                                                                                                                              /* Bonus 15 */
    "Recipe Int. - Food cooked by you lasts longer/heals better        ",                                                                                                                                                              /* Bonus 16 */
    "Fireproof   - -50% Fire Dmg taken, -10% ki, immunity to burn      ",                                                                                                                                                              /* Bonus 17 */
    "Powerhitter - 15% critical hits will be x4 instead of x2          ",                                                                                                                                                              /* Bonus 18 */
    "Healthy     - 40% chance to recover from ill effects when sleeping",                                                                                                                                                              /* Bonus  19 */
    "Insomniac   - Can't Sleep. Immune to yoikominminken and paralysis ",                                                                                                                                                              /* Bonus  20 */
    "Evasive     - +15% to dodge rolls                                 ",                                                                                                                                                              /* Bonus  21 */
    "The Wall    - +20% chance to block                                ",                                                                                                                                                              /* Bonus  22 */
    "Accurate    - +20% chance to hit physical, +10% to hit with ki     ",                                                                                                                                                             /* Bonus  23 */
    "Energy Leech- -2% ki damage received for every 5 character levels,\n                  @cas long as you can take that ki to your charge pool.@D        ",                                                                          /* Bonus  24*/
    "Good Memory - +2 Skill Slots initially, +1 every 20 levels after  ",                                                                                                                                                              /* Bonus 25 */
    "Soft Touch  - Half damage for all hit locations                   ",                                                                                                                                                              /* Neg 26 */
    "Late Sleeper- Can only wake automatically. 33% every hour if maxed",                                                                                                                                                              /* Neg 27 */
    "Impulse Shop- +25% shop costs                                     ",                                                                                                                                                              /* Neg 28 */
    "Sickly      - Suffer from harmful effects longer                  ",                                                                                                                                                              /* Neg 29 */
    "Punching Bag- -15% to dodge rolls                                 ",                                                                                                                                                              /* Neg 30 */
    "Pushover    - -20% block chance                                   ",                                                                                                                                                              /* Neg 31 */
    "Poor D. Perc- -20% chance to hit with physical, -10% with ki       ",                                                                                                                                                             /* Neg 32 */
    "Thin Skin   - +20% physical and +10% ki damage received           ",                                                                                                                                                              /* Neg 33 */
    "Fireprone   - +50% Fire Dmg taken, +10% ki, always burned         ",                                                                                                                                                              /* Neg 34 */
    "Energy Int. - +2% ki damage received for every 5 character levels,\n                  @rif you have ki charged you have 10% chance to lose   \n                  it and to take 1/4th damage equal to it.@D                    ", /* Neg 35 */
    "Coward      - Can't Attack Enemy With 150% Your Powerlevel        ",                                                                                                                                                              /* Neg 36 */
    "Arrogant    - Cannot Suppress                                     ",                                                                                                                                                              /* Neg 37 */
    "Unfocused   - Charge concentration randomly breaks                ",                                                                                                                                                              /* Neg 38 */
    "Slacker     - Physical activity drains more stamina               ",                                                                                                                                                              /* Neg 39 */
    "Slow Learner- Character auto-trains skills slower                 ",                                                                                                                                                              /* Neg 40 */
    "Masochistic - Defense Skills Cap At 75                            ",                                                                                                                                                              /* Neg 41 */
    "Mute        - Can't use IC speech related commands                ",                                                                                                                                                              /* Neg 42 */
    "Wimp        - Strength is capped at 45                            ",                                                                                                                                                              /* Neg 43 */
    "Dull        - Intelligence is capped at 45                        ",                                                                                                                                                              /* Neg 44 */
    "Foolish     - Wisdom is capped at 45                              ",                                                                                                                                                              /* Neg 45 */
    "Clumsy      - Agility is capped at 45                             ",                                                                                                                                                              /* Neg 46 */
    "Slow        - Speed is capped at 45                               ",                                                                                                                                                              /* Neg 47 */
    "Frail       - Constitution capped at 45                           ",                                                                                                                                                              /* Neg 48 */
    "Sadistic    - Half Experience Gained For Quick Kills              ",                                                                                                                                                              /* Neg 49 */
    "Loner       - Can't Group with anyone, +5% train and +10% Phys    ",                                                                                                                                                              /* Neg 50 */
    "Bad Memory  - -5 Skill Slots                                      "                                                                                                                                                               /* Neg 51 */
};

/* Display What Bonuses/Negatives Player Has */
static void bonus_status(GameWorld *g, struct char_data *ch)
{
  int i, max = 52, count = 0;

  if (char_is_npc(ch))
    return;

  send_to_char(g, ch, "@CYour Traits@n\n@D-----------------------------@w\n");
  for (i = 0; i < max; i++) {
    if (i < 26) {
      if (char_get_bonuses(ch, i)) {
        send_to_char(g, ch, "@c%s@n\n", list_bonuses[i]);
        count++;
      }
    } else {
      if (i == 26) {
        send_to_char(g, ch, "\r\n");
      }
      if (char_get_bonuses(ch, i)) {
        send_to_char(g, ch, "@r%s@n\n", list_bonuses[i]);
        count++;
      }
    }
  }
  if (count <= 0) {
    send_to_char(g, ch, "@wNone.\r\n");
  }
  send_to_char(g, ch, "@D-----------------------------@n\r\n");
  return;
}

ACMD(do_inventory)
{
  send_to_char(g, ch, "@w              @YInventory\r\n@D-------------------------------------@w\r\n");
  if (!char_is_npc(ch)) {
    if (char_plr_flagged(ch, PLR_STOLEN)) {
      char_plr_set(ch, PLR_STOLEN, false);
      send_to_char(g, ch, "@r   --------------------------------------------------@n\n");
      send_to_char(g, ch, "@R    You notice that you have been robbed sometime recently!\n");
      send_to_char(g, ch, "@r   --------------------------------------------------@n\n");
      return;
    }
  }
  list_obj_to_char(g, char_get_inventory(ch), ch, SHOW_OBJ_SHORT, true);
  send_to_char(g, ch, "\n");
}

ACMD(do_equipment)
{
  int i;

  send_to_char(g, ch, "        @YEquipment Being Worn\r\n@D-------------------------------------@w\r\n");
  for (i = 1; i < NUM_WEARS; i++) {
    if (char_get_equipment(ch, i)) {
      if (char_can_see_obj(g, ch, char_get_equipment(ch, i)) && (i != WEAR_WIELD1 && i != WEAR_WIELD2)) {
        send_to_char(g, ch, "%s", wear_where[i]);
        show_obj_to_char(g, char_get_equipment(ch, i), ch, SHOW_OBJ_SHORT);

        if (obj_extra_flagged(char_get_equipment(ch, i), ITEM_SHEATH)) {
          struct obj_data *obj2 = nullptr, *next_obj = nullptr, *sheath = char_get_equipment(ch, i);
          for (obj2 = obj_get_contains(sheath); obj2; obj2 = next_obj) {
            next_obj = obj_get_next_content(obj2);
            if (obj2) {
              send_to_char(g, ch, "@D  ---- @YSheathed@D ----@c> @n");
              show_obj_to_char(g, obj2, ch, SHOW_OBJ_SHORT);
            }
          }
          obj2 = nullptr;
        }
      } else if (char_can_see_obj(g, ch, char_get_equipment(ch, i)) && (!char_plr_flagged(ch, PLR_THANDW))) {
        send_to_char(g, ch, "%s", wear_where[i]);
        show_obj_to_char(g, char_get_equipment(ch, i), ch, SHOW_OBJ_SHORT);
        if (obj_extra_flagged(char_get_equipment(ch, i), ITEM_SHEATH)) {
          struct obj_data *obj2 = nullptr, *next_obj = nullptr, *sheath = char_get_equipment(ch, i);
          for (obj2 = obj_get_contains(sheath); obj2; obj2 = next_obj) {
            next_obj = obj_get_next_content(obj2);
            if (obj2) {
              send_to_char(g, ch, "@D  ---- @YSheathed@D ----> @n");
              show_obj_to_char(g, obj2, ch, SHOW_OBJ_SHORT);
            }
          }
          obj2 = nullptr;
        }
      } else if (char_can_see_obj(g, ch, char_get_equipment(ch, i)) && (char_plr_flagged(ch, PLR_THANDW))) {
        send_to_char(g, ch, "@c<@CWielded by B. Hands@c>@n ");
        show_obj_to_char(g, char_get_equipment(ch, i), ch, SHOW_OBJ_SHORT);
      } else {
        send_to_char(g, ch, "%s", wear_where[i]);
        send_to_char(g, ch, "Something.\r\n");
      }
    } else {
      if (char_body_flagged(ch, i) && (i != WEAR_WIELD2)) {
        send_to_char(g, ch, "%s@wNothing.@n\r\n", wear_where[i]);
      } else if (char_body_flagged(ch, i) && (i == WEAR_WIELD2 && !char_plr_flagged(ch, PLR_THANDW))) {
        send_to_char(g, ch, "%s@wNothing.@n\r\n", wear_where[i]);
      }
    }
  }
}

ACMD(do_levels)
{
  char buf[MAX_STRING_LENGTH];
  size_t i, len = 0;
  int nlen;

  if (char_is_npc(ch)) {
    send_to_char(g, ch, "You ain't nothin' but a hound-dog.\r\n");
    return;
  }

  for (i = 1; i < 101; i++) {
    if (i == 100)
      nlen = snprintf(buf + len, sizeof(buf) - len, "[100] %8s          : \r\n", add_commas(level_exp(ch, 100)));
    else
      nlen = snprintf(buf + len, sizeof(buf) - len, "[%2zu] %8s-%-8s : \r\n", i,
                      add_commas(level_exp(ch, i)), add_commas(level_exp(ch, i + 1) - 1));
    if (len + nlen >= sizeof(buf) || nlen < 0)
      break;
    len += nlen;
  }

  send_to_char(g, ch, "%s", buf);
}

ACMD(do_consider)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *victim;
  int diff;

  one_argument(argument, buf);

  if (!(victim = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "Consider killing who?\r\n");
    return;
  }
  if (victim == ch) {
    send_to_char(g, ch, "Easy!  Very easy indeed!\r\n");
    return;
  }
  diff = (char_get_level(victim) - char_get_level(ch));

  if (diff <= -10)
    send_to_char(g, ch, "Now where did that chicken go?\r\n");
  else if (diff <= -5)
    send_to_char(g, ch, "You could do it with a needle!\r\n");
  else if (diff <= -2)
    send_to_char(g, ch, "Easy.\r\n");
  else if (diff <= -1)
    send_to_char(g, ch, "Fairly easy.\r\n");
  else if (diff == 0)
    send_to_char(g, ch, "The perfect match!\r\n");
  else if (diff <= 1)
    send_to_char(g, ch, "You could probably manage it.\r\n");
  else if (diff <= 2)
    send_to_char(g, ch, "You might take a beating.\r\n");
  else if (diff <= 3)
    send_to_char(g, ch, "You MIGHT win, maybe.\r\n");
  else if (diff <= 5)
    send_to_char(g, ch, "Do you feel lucky? You better.\r\n");
  else if (diff <= 10)
    send_to_char(g, ch, "Better bring some tough backup!\r\n");
  else if (diff <= 25)
    send_to_char(g, ch, "Maybe if they are allergic to you, otherwise your last words will be 'Oh shit.'\r\n");
  else
    send_to_char(g, ch, "No chance.\r\n");
}

ACMD(do_diagnose)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_vis(g, ch, buf, nullptr, FIND_CHAR_ROOM)))
      send_to_char(g, ch, "%s", CONFIG_NOPERSON);
    else {
      send_to_char(g, ch, "%s", char_get_sex(vict) == SEX_MALE ? "He " : (char_get_sex(vict) == SEX_FEMALE ? "She " : "It "));
      diag_char_to_char(g, vict, ch);
    }
  } else {
    if (char_get_fighting(ch)) {
      send_to_char(g, ch, "%s", char_get_sex(char_get_fighting(ch)) == SEX_MALE ? "He " : (char_get_sex(char_get_fighting(ch)) == SEX_FEMALE ? "She " : "It "));
      diag_char_to_char(g, char_get_fighting(ch), ch);
    } else {
      send_to_char(g, ch, "Diagnose who?\r\n");
    }
  }
}
