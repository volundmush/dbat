/*
 * Characters: mobs made from a prototype, with the rolls that make each one different, players set up when they
 * are created, and characters cleared and freed; and the prototype search of "vnum".
 */
#include "dbat/characters.hpp"
#include "dbat/iterate.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/affect.hpp"
#include "dbat/search.hpp"
#include "dbat/players.hpp"
#include "dbat/random.hpp"
#include "dbat/objsave.hpp"
#include "dbat/local_limits.hpp"

static void dragon_level(GameWorld *g, struct char_data *ch)
{
  int level = 0, count = 0;

  for (descriptor_data *d : each_desc(g)) {
    if (desc_is_playing(d) && char_get_admlevel(d->character) < 1) {
      level += char_get_level(d->character);
      count += 1;
    }
  }

  if (level > 0 && count > 0) {
    level = level / count;
  } else {
    level = rand_number(g, 60, 110);
  }

  if (level < 50) {
    level = rand_number(g, 40, 60);
  }

  char_set_race_level(ch, 0);
  char_set_race_level(ch, level + rand_number(g, 5, 20));
}

void mob_stats(GameWorld *g, struct char_data *mob)
{
  int start = char_get_level(mob) * 0.5, finish = char_get_level(mob);

  if (finish < 20)
    finish = 20;

  if (!char_is_humanoid(mob)) {
    char_set_real_abils_str(mob, rand_number(g, start, finish));
    char_set_real_abils_intel(mob, rand_number(g, start, finish) - 30);
    char_set_real_abils_wis(mob, rand_number(g, start, finish) - 30);
    char_set_real_abils_dex(mob, rand_number(g, start + 5, finish));
    char_set_real_abils_con(mob, rand_number(g, start + 5, finish));
    char_set_real_abils_cha(mob, rand_number(g, start, finish));
  } else {
    if (char_is_saiyan(mob)) {
      char_set_real_abils_str(mob, rand_number(g, start + 10, finish));
      char_set_real_abils_intel(mob, rand_number(g, start, finish - 10));
      char_set_real_abils_wis(mob, rand_number(g, start, finish - 5));
      char_set_real_abils_dex(mob, rand_number(g, start, finish));
      char_set_real_abils_con(mob, rand_number(g, start + 5, finish));
      char_set_real_abils_cha(mob, rand_number(g, start + 5, finish));
    } else if (char_is_konatsu(mob)) {
      char_set_real_abils_str(mob, rand_number(g, start, finish - 10));
      char_set_real_abils_intel(mob, rand_number(g, start, finish));
      char_set_real_abils_wis(mob, rand_number(g, start, finish));
      char_set_real_abils_dex(mob, rand_number(g, start + 10, finish));
      char_set_real_abils_con(mob, rand_number(g, start, finish));
      char_set_real_abils_cha(mob, rand_number(g, start, finish));
    } else if (char_is_android(mob)) {
      char_set_real_abils_str(mob, rand_number(g, start, finish));
      char_set_real_abils_intel(mob, rand_number(g, start, finish));
      char_set_real_abils_wis(mob, rand_number(g, start, finish - 10));
      char_set_real_abils_dex(mob, rand_number(g, start, finish));
      char_set_real_abils_con(mob, rand_number(g, start, finish));
      char_set_real_abils_cha(mob, rand_number(g, start, finish));
    } else if (char_is_majin(mob)) {
      char_set_real_abils_str(mob, rand_number(g, start, finish));
      char_set_real_abils_intel(mob, rand_number(g, start, finish - 10));
      char_set_real_abils_wis(mob, rand_number(g, start, finish - 5));
      char_set_real_abils_dex(mob, rand_number(g, start, finish));
      char_set_real_abils_con(mob, rand_number(g, start + 15, finish));
      char_set_real_abils_cha(mob, rand_number(g, start, finish));
    } else if (char_is_truffle(mob)) {
      char_set_real_abils_str(mob, rand_number(g, start, finish - 10));
      char_set_real_abils_intel(mob, rand_number(g, start + 15, finish));
      char_set_real_abils_wis(mob, rand_number(g, start, finish));
      char_set_real_abils_dex(mob, rand_number(g, start, finish));
      char_set_real_abils_con(mob, rand_number(g, start, finish));
      char_set_real_abils_cha(mob, rand_number(g, start, finish));
    } else if (char_is_icer(mob)) {
      char_set_real_abils_str(mob, rand_number(g, start + 5, finish));
      char_set_real_abils_intel(mob, rand_number(g, start, finish));
      char_set_real_abils_wis(mob, rand_number(g, start, finish));
      char_set_real_abils_dex(mob, rand_number(g, start, finish));
      char_set_real_abils_con(mob, rand_number(g, start, finish));
      char_set_real_abils_cha(mob, rand_number(g, start + 10, finish));
    } else {
      char_set_real_abils_str(mob, rand_number(g, start, finish));
      char_set_real_abils_intel(mob, rand_number(g, start, finish));
      char_set_real_abils_wis(mob, rand_number(g, start, finish));
      char_set_real_abils_dex(mob, rand_number(g, start, finish));
      char_set_real_abils_con(mob, rand_number(g, start, finish));
      char_set_real_abils_cha(mob, rand_number(g, start, finish));
    }
  }

  if (char_get_real_abils_str(mob) > 100)
    char_set_real_abils_str(mob, 100);
  else if (char_get_real_abils_str(mob) < 5)
    char_set_real_abils_str(mob, rand_number(g, 5, 8));

  if (char_get_real_abils_intel(mob) > 100)
    char_set_real_abils_intel(mob, 100);
  else if (char_get_real_abils_intel(mob) < 5)
    char_set_real_abils_intel(mob, rand_number(g, 5, 8));

  if (char_get_real_abils_wis(mob) > 100)
    char_set_real_abils_wis(mob, 100);
  else if (char_get_real_abils_wis(mob) < 5)
    char_set_real_abils_wis(mob, rand_number(g, 5, 8));

  if (char_get_real_abils_con(mob) > 100)
    char_set_real_abils_con(mob, 100);
  else if (char_get_real_abils_con(mob) < 5)
    char_set_real_abils_con(mob, rand_number(g, 5, 8));

  if (char_get_real_abils_cha(mob) > 100)
    char_set_real_abils_cha(mob, 100);
  else if (char_get_real_abils_cha(mob) < 5)
    char_set_real_abils_cha(mob, rand_number(g, 5, 8));

  if (char_get_real_abils_dex(mob) > 100)
    char_set_real_abils_dex(mob, 100);
  else if (char_get_real_abils_dex(mob) < 5)
    char_set_real_abils_dex(mob, rand_number(g, 5, 8));
}

int vnum_mobile(GameWorld *g, std::string_view searchname, struct char_data *ch)
{
  int found = 0;

  for (mob_vnum nr : g->mob_proto.vnums())
    if (isname(searchname, char_get_alias(mob_proto_of(g, nr))))
      send_to_char(g, ch, "{:>3d}. [{:>5d}] {:<40} {}\r\n",
                   ++found, mob_index_of(g, nr)->vnum, char_get_sdesc(mob_proto_of(g, nr)),
                   !char_get_proto_script(mob_proto_of(g, nr)).empty() ? "[TRIG]" : "");

  return (found);
}

/* create a character, and add it to the char list */

/* create a new mobile from a prototype */
struct char_data *read_mobile(GameWorld *g, mob_vnum nr)
{
  mob_vnum i = nr;
  struct char_data *mob;

  if (!mob_proto_of(g, i)) {
    mud_log("WARNING: Mobile vnum {:d} does not exist in database.", nr);
    return (nullptr);
  }

  mob = new char_data();
  clear_char(mob);
  *mob = (*mob_proto_of(g, i));

  if (char_is_hoshijin(mob) && char_get_sex(mob) == SEX_MALE) {
    char_set_hairl(mob, 0);
    char_set_hairc(mob, 0);
    char_set_hairs(mob, 0);
  } else {
    char_set_hairl(mob, rand_number(g, 0, 4));
    char_set_hairc(mob, rand_number(g, 1, 13));
    char_set_hairs(mob, rand_number(g, 1, 11));
  }

  char_set_eye(mob, rand_number(g, 0, 11));

  char_set_absorbs(mob, 0);
  char_set_absorbing(mob, nullptr);
  char_set_absorbby(mob, nullptr);
  char_set_sits(mob, nullptr);
  char_set_blocked(mob, nullptr);
  char_set_blocks(mob, nullptr);

  if (!char_is_human(mob) && !char_is_saiyan(mob) && !char_is_halfbreed(mob) && !char_is_namek(mob)) {
    char_set_skin(mob, rand_number(g, 0, 11));
  }
  if (char_is_namek(mob)) {
    char_set_skin(mob, 2);
  }
  if (char_is_human(mob) || char_is_saiyan(mob) || char_is_halfbreed(mob)) {
    if (rand_number(g, 1, 5) <= 2) {
      char_set_skin(mob, rand_number(g, 0, 1));
    } else if (rand_number(g, 1, 5) <= 4) {
      char_set_skin(mob, rand_number(g, 4, 5));
    } else if (rand_number(g, 1, 5) <= 5) {
      char_set_skin(mob, rand_number(g, 9, 10));
    }
  }
  if (char_is_saiyan(mob)) {
    char_set_hairc(mob, rand_number(g, 1, 2));
    char_set_eye(mob, 1);
  }

  if (char_get_mob_vnum(g, mob) >= 81 && char_get_mob_vnum(g, mob) <= 87) {
    dragon_level(g, mob);
  }

  int64_t mult = 0;

  switch (char_get_level(mob)) {
    case 1:
      mult = rand_number(g, 50, 80);
      break;
    case 2:
      mult = rand_number(g, 90, 120);
      break;
    case 3:
      mult = rand_number(g, 100, 140);
      break;
    case 4:
      mult = rand_number(g, 120, 180);
      break;
    case 5:
      mult = rand_number(g, 200, 250);
      break;
    case 6:
      mult = rand_number(g, 240, 300);
      break;
    case 7:
      mult = rand_number(g, 280, 350);
      break;
    case 8:
      mult = rand_number(g, 320, 400);
      break;
    case 9:
      mult = rand_number(g, 380, 480);
      break;
    case 10:
      mult = rand_number(g, 500, 600);
      break;
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      mult = rand_number(g, 1200, 1600);
      break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
      mult = rand_number(g, 2400, 3000);
      break;
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
      mult = rand_number(g, 5500, 8000);
      break;
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
      mult = rand_number(g, 10000, 14000);
      break;
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
      mult = rand_number(g, 16000, 20000);
      break;
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
      mult = rand_number(g, 22000, 30000);
      break;
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
      mult = rand_number(g, 50000, 70000);
      break;
    case 46:
    case 47:
    case 48:
    case 49:
    case 50:
      mult = rand_number(g, 95000, 140000);
      break;
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
      mult = rand_number(g, 180000, 250000);
      break;
    case 56:
    case 57:
    case 58:
    case 59:
    case 60:
      mult = rand_number(g, 400000, 480000);
      break;
    case 61:
    case 62:
    case 63:
    case 64:
    case 65:
      mult = rand_number(g, 700000, 900000);
      break;
    case 66:
    case 67:
    case 68:
    case 69:
    case 70:
      mult = rand_number(g, 1400000, 1600000);
      break;
    case 71:
    case 72:
    case 73:
    case 74:
    case 75:
      mult = rand_number(g, 2200000, 2500000);
      break;
    case 76:
    case 77:
    case 78:
    case 79:
    case 80:
      mult = rand_number(g, 3000000, 3500000);
      break;
    case 81:
    case 82:
    case 83:
    case 84:
    case 85:
      mult = rand_number(g, 4250000, 4750000);
      break;
    case 86:
    case 87:
    case 88:
    case 89:
    case 90:
      mult = rand_number(g, 6500000, 8500000);
      break;
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
      mult = rand_number(g, 15000000, 18000000);
      break;
    case 96:
    case 97:
    case 98:
    case 99:
    case 100:
      mult = rand_number(g, 22000000, 30000000);
      break;
    case 101:
      mult = rand_number(g, 32000000, 40000000);
      break;
    case 102:
      mult = rand_number(g, 42000000, 55000000);
      break;
    case 103:
      mult = rand_number(g, 80000000, 95000000);
      break;
    case 104:
      mult = rand_number(g, 150000000, 200000000);
      break;
    case 105:
      mult = rand_number(g, 220000000, 250000000);
      break;
    case 106:
    case 107:
    case 108:
    case 109:
    case 110:
      mult = rand_number(g, 500000000, 750000000);
      break;
    case 111:
    case 112:
    case 113:
    case 114:
    case 115:
    case 116:
    case 117:
    case 118:
    case 119:
    case 120:
      mult = rand_number(g, 800000000, 900000000);
      break;
    default:
      if (char_get_level(mob) >= 150) {
        mult = rand_number(g, 1500000000, 2000000000);
      } else {
        mult = rand_number(g, 1250000000, 1500000000);
      }
      break;
  }

  char_set_lastpl(mob, game_now(g));
  bool autoset = false;
  if (getBasePL(mob) <= 1) {
    autoset     = true;
    char_set_basepl(mob, char_get_level(mob) * mult);
    if (char_get_level(mob) > 140) {
      char_set_basepl(mob, getBasePL(mob) * 8);
    } else if (char_get_level(mob) > 130) {
      char_set_basepl(mob, getBasePL(mob) * 6);
    } else if (char_get_level(mob) > 120) {
      char_set_basepl(mob, getBasePL(mob) * 3);
    } else if (char_get_level(mob) > 110) {
      char_set_basepl(mob, getBasePL(mob) * 2);
    }
  }
  if (autoset) {
    char_set_baseki(mob, char_get_level(mob) * mult);
    if (char_get_level(mob) > 140) {
      char_set_baseki(mob, getBaseKI(mob) * 8);
    } else if (char_get_level(mob) > 130) {
      char_set_baseki(mob, getBaseKI(mob) * 6);
    } else if (char_get_level(mob) > 120) {
      char_set_baseki(mob, getBaseKI(mob) * 3);
    } else if (char_get_level(mob) > 110) {
      char_set_baseki(mob, getBaseKI(mob) * 2);
    }
  }
  if (autoset) {
    char_set_basest(mob, char_get_level(mob) * mult);
    if (char_get_level(mob) > 140) {
      char_set_basest(mob, getBaseST(mob) * 8);
    } else if (char_get_level(mob) > 130) {
      char_set_basest(mob, getBaseST(mob) * 6);
    } else if (char_get_level(mob) > 120) {
      char_set_basest(mob, getBaseST(mob) * 3);
    } else if (char_get_level(mob) > 110) {
      char_set_basest(mob, getBaseST(mob) * 2);
    }
  }
  if (char_get_mob_vnum(g, mob) == 2245) {
    char_set_basepl(mob, rand_number(g, 1, 4));
    char_set_baseki(mob, rand_number(g, 1, 4));
    char_set_basest(mob, rand_number(g, 1, 4));
  }

  int base = 0;
  switch (char_get_level(mob)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      base = rand_number(g, 80, 120);
      break;
    case 6:
      base = rand_number(g, 200, 280);
      break;
    case 7:
      base = rand_number(g, 250, 350);
      break;
    case 8:
      base = rand_number(g, 275, 375);
      break;
    case 9:
      base = rand_number(g, 300, 400);
      break;
    case 10:
      base = rand_number(g, 325, 450);
      break;
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      base = rand_number(g, 500, 700);
      break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
      base = rand_number(g, 700, 1000);
      break;
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
      base = rand_number(g, 1000, 1200);
      break;
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
      base = rand_number(g, 1200, 1400);
      break;
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
      base = rand_number(g, 1400, 1600);
      break;
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
      base = rand_number(g, 1600, 1800);
      break;
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
      base = rand_number(g, 1800, 2000);
      break;
    case 46:
    case 47:
    case 48:
    case 49:
    case 50:
      base = rand_number(g, 2000, 2200);
      break;
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
      base = rand_number(g, 2200, 2500);
      break;
    case 56:
    case 57:
    case 58:
    case 59:
    case 60:
      base = rand_number(g, 2500, 2800);
      break;
    case 61:
    case 62:
    case 63:
    case 64:
    case 65:
      base = rand_number(g, 2800, 3000);
      break;
    case 66:
    case 67:
    case 68:
    case 69:
    case 70:
      base = rand_number(g, 3000, 3200);
      break;
    case 71:
    case 72:
    case 73:
    case 74:
    case 75:
      base = rand_number(g, 3200, 3500);
      break;
    case 76:
    case 77:
    case 78:
    case 79:
      base = rand_number(g, 3500, 3800);
      break;
    case 80:
    case 81:
    case 82:
    case 83:
    case 84:
    case 85:
      base = rand_number(g, 4000, 4500);
      break;
    case 86:
    case 87:
    case 88:
    case 89:
    case 90:
      base = rand_number(g, 4500, 5500);
      break;
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
      base = rand_number(g, 5500, 7000);
      break;
    case 96:
    case 97:
    case 98:
    case 99:
      base = rand_number(g, 8000, 10000);
      break;
    case 100:
      base = rand_number(g, 10000, 15000);
      break;
    case 101:
      base = rand_number(g, 15000, 25000);
      break;
    case 102:
      base = rand_number(g, 35000, 40000);
      break;
    case 103:
      base = rand_number(g, 40000, 50000);
      break;
    case 104:
      base = rand_number(g, 60000, 80000);
      break;
    case 105:
      base = rand_number(g, 80000, 100000);
      break;
    default:
      base = rand_number(g, 130000, 180000);
      break;
  }
  char_set_cooldown(mob, 0);
  if (char_get_gold(mob) <= 0 && !char_mob_flagged(mob, mob_flag::dummy)) {
    if (char_get_level(mob) < 4) {
      char_set_gold(mob, char_get_level(mob) * rand_number(g, 1, 2));
    } else if (char_get_level(mob) < 10) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 1, 2)) - 1);
    } else if (char_get_level(mob) < 20) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 1, 3)) - 2);
    } else if (char_get_level(mob) < 30) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 1, 3)) - 4);
    } else if (char_get_level(mob) < 40) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 1, 3)) - 6);
    } else if (char_get_level(mob) < 50) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 2, 3)) - 25);
    } else if (char_get_level(mob) < 60) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 2, 3)) - 40);
    } else if (char_get_level(mob) < 70) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 2, 3)) - 50);
    } else if (char_get_level(mob) < 80) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 2, 4)) - 60);
    } else if (char_get_level(mob) < 90) {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 2, 4)) - 70);
    } else {
      char_set_gold(mob, (char_get_level(mob) * rand_number(g, 3, 4)) - 85);
    }
    if (!char_is_humanoid(mob)) {
      char_set_gold(mob, char_get_gold(mob) * 0.5);
      if (char_get_gold(mob) <= 0)
        char_set_gold(mob, 1);
    }
  }
  if (char_get_exp(mob) <= 0 && !char_mob_flagged(mob, mob_flag::dummy)) {
    char_set_exp(mob, (int64_t)char_get_level(mob) * base);
    char_set_exp(mob, char_get_exp(mob) * .9);
    char_set_exp(mob, char_get_exp(mob) + (char_get_level(mob) / 2));
    char_set_exp(mob, char_get_exp(mob) + (char_get_level(mob) / 3));
    if (char_is_dragon(mob)) {
      char_set_exp(mob, char_get_exp(mob) * (1.4));
    } else if (char_is_android(mob)) {
      char_set_exp(mob, char_get_exp(mob) * (1.25));
    } else if (char_is_saiyan(mob)) {
      char_set_exp(mob, char_get_exp(mob) * (1.1));
    } else if (char_is_bio(mob)) {
      char_set_exp(mob, char_get_exp(mob) * (1.2));
    } else if (char_is_majin(mob)) {
      char_set_exp(mob, char_get_exp(mob) * (1.25));
    } else if (char_is_demon(mob)) {
      char_set_exp(mob, char_get_exp(mob) * (1.1));
    } else if (char_get_class(mob) == CLASS_SHADOWDANCER) {
      char_set_exp(mob, char_get_exp(mob) * (2));
    }
    if (char_get_class(mob) == CLASS_NPC_COMMONER && char_is_humanoid(mob) && !char_is_dragon(mob)) {
      if (!char_is_android(mob) && !char_is_saiyan(mob) && !char_is_bio(mob) && !char_is_majin(mob)) {
        char_set_exp(mob, char_get_exp(mob) * (0.75));
      }
    }

    if (char_get_level(mob) > 90) {
      char_set_exp(mob, char_get_exp(mob) * .7);
    } else if (char_get_level(mob) > 80) {
      char_set_exp(mob, char_get_exp(mob) * .75);
    } else if (char_get_level(mob) > 70) {
      char_set_exp(mob, char_get_exp(mob) * .8);
    } else if (char_get_level(mob) > 60) {
      char_set_exp(mob, char_get_exp(mob) * .85);
    } else if (char_get_level(mob) > 40) {
      char_set_exp(mob, char_get_exp(mob) * .9);
    } else if (char_get_level(mob) > 30) {
      char_set_exp(mob, char_get_exp(mob) * .95);
    }

    if (char_get_exp(mob) > 20000000) {
      char_set_exp(mob, 20000000);
    }
    if (char_get_exp(mob) < 0) {
      char_set_exp(mob, 0);
    }
  }

  char_set_time_birth(mob, game_now(g) - birth_age(g, mob));
  char_set_time_logon(mob, game_now(g));
  char_set_time_created(mob, char_get_time_logon(mob)); /* why not */
  char_set_time_maxage(mob, char_get_time_birth(mob) + max_age(mob));
  char_set_time_played(mob, 0);
  char_set_time_logon(mob, game_now(g));
  char_set_hometown(mob, NOWHERE);

  if (char_is_humanoid(mob)) {
    char_mob_set(mob, mob_flag::rarm, true);
    char_mob_set(mob, mob_flag::larm, true);
    char_mob_set(mob, mob_flag::rleg, true);
    char_mob_set(mob, mob_flag::lleg, true);
  }

  mob_index_of(g, i)->number++;

  char_set_id(mob, dg_new_mob_uid(g));
  char_register(g, mob);

  copy_proto_script(mob_proto_of(g, i), mob);
  assign_triggers(g, mob);
  racial_body_parts(mob);

  if (char_get_mob_vnum(g, mob) >= 800 && char_get_mob_vnum(g, mob) <= 805) {
    g->number_of_assassins += 1;
  }

  return (mob);
}

/* release memory allocated for a char struct */
void free_char(GameWorld *g, struct char_data *ch)
{
  /* first: removing an affect can change skills, which live in the player data freed below */
  while (!char_affects(ch).empty())
    affect_remove(g, ch, &char_affects(ch).front());

  /* Containers (affects aside, which change what they affect), strings, the trigger list and player_specials go
     with ch. */
  /* free any assigned scripts */
  if (char_get_script(ch))
    extract_script(g, ch);

  if (char_get_desc(ch))
    char_get_desc(ch)->character = nullptr;

  char_unregister(g, ch); /* if it is the one registered under its id */
  char_unlink_from_combat_list(g, ch);
  char_unlink_from_affect_list(g, ch);

  delete ch;
}

/* clear some of the the working variables of a char */
void reset_char(GameWorld *g, struct char_data *ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    char_set_equipment(ch, i, nullptr);

  char_followers_slot(ch)->clear();
  char_set_master(ch, nullptr);
  char_set_in_room(ch, NOWHERE);
  char_inventory_slot(ch)->clear();
  char_set_fighting(ch, nullptr);
  char_set_position(ch, POS_STANDING);
  char_set_mob_specials_default_pos(ch, POS_STANDING);
  char_set_carry_weight(ch, 0);
  char_set_carry_items(ch, 0);
  char_set_time_logon(ch, game_now(g));

  char_set_last_tell(g, ch, NOBODY);
}

/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data *ch)
{
  *ch = char_data{}; /* each field's initial value: it holds containers, so it must never be memset */

  char_set_in_room(ch, NOWHERE);
  char_set_pfilepos(ch, -1);
  char_set_nr(ch, NOBODY);
  char_set_was_in_room(ch, NOWHERE);
  char_set_position(ch, POS_STANDING);
  char_set_mob_specials_default_pos(ch, POS_STANDING);

  char_set_size(ch, SIZE_UNDEFINED);
}

/*
 * Called during character creation after picking character class
 * (and then never again for that character).
 */
void init_char(GameWorld *g, struct char_data *ch)
{
  int i;

  /* create a player_special structure */
  if (char_get_player_specials(ch) == nullptr) {
    char_make_player_specials(ch);
  }

  char_set_admlevel(ch, ADMLVL_NONE);
  char_set_absorbs(ch, 0);
  char_set_absorbing(ch, nullptr);
  char_set_absorbby(ch, nullptr);
  char_set_sits(ch, nullptr);
  char_set_blocked(ch, nullptr);
  char_set_blocks(ch, nullptr);

  /* If this is our first player make him LVL_IMPL. */
  if (g->top_of_p_table == 0) {
    admin_set(g, ch, ADMLVL_IMPL);

    /* The implementor never goes through do_start(). */
    char_set_baseki(ch, 1000);
    char_set_basepl(ch, 1000);
    char_set_basest(ch, 1000);
  }

  set_title(g, ch, "");
  char_set_sdesc(ch, "");
  char_set_ldesc(ch, "");
  char_set_ddesc(ch, "");

  /*ch->time.birth = time(0) - birth_age(ch);*/
  char_set_time_created(ch, game_now(g));
  char_set_time_logon(ch, char_get_time_created(ch));
  char_set_time_maxage(ch, char_get_time_birth(ch) + max_age(ch));
  char_set_time_played(ch, 0);

  char_set_hometown(ch, 1);
  char_set_armor(ch, 0);

  set_height_and_weight_by_race(g, ch);

  if ((i = get_ptable_by_name(g, char_get_name(ch))) != -1) {
    char_set_idnum(ch, ++g->top_idnum);
    g->player_table[i].id = char_get_idnum(ch);
  } else
    mud_log("SYSERR: init_char: Character '{}' not found in player table.", char_get_name(ch));

  for (i = 0; i < SKILL_TABLE_SIZE; i++) {
    if (char_get_admlevel(ch) < ADMLVL_IMPL)
      char_set_skills_base(ch, i, 0);
    else
      char_set_skills_base(ch, i, 100);
    char_set_skills_mod(ch, i, 0);
  }

  char_get_affected_by(ch) = {};

  for (i = 0; i < 3; i++)
    char_set_conditions(g, ch, i, (char_get_admlevel(ch) == ADMLVL_IMPL ? -1 : 24));

  char_set_load_room(g, ch, NOWHERE);
  char_set_speaking(ch, SKILL_LANG_COMMON);
  char_set_feat_points(g, ch, 1);
  // initialize plrobjs so it doesn't complain on startup.
  Crash_crashsave(g, ch);
}
