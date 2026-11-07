/* items.c: item crafting, repair and use
 * Split from act.other.c (CircleMUD/DBAT; see license.doc). */
#include <cstdlib>

#include <sys/stat.h>

#include "dbat/iterate.hpp"
#include "dbat/act.other.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/shadowdragons.hpp"
#include "dbat/consts/auction.hpp"
#include "dbat/consts/aligns.hpp"
#include "dbat/utils.hpp"
#include "dbat/flags.hpp"
#include "dbat/config.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/zone_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/search.hpp"
#include "dbat/random.hpp"
#include "dbat/affect.hpp"
#include "dbat/relocate.hpp"
#include "dbat/extract.hpp"
#include "dbat/races.hpp"
#include "dbat/fileop.hpp"
#include "dbat/comm.hpp"
#include "dbat/handler.hpp"
#include "dbat/broadcast.hpp"
#include "dbat/combat.hpp"
#include "dbat/act.misc.hpp"
#include "dbat/weather.hpp"
#include "dbat/act.item.hpp"
#include "dbat/act.wizard.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/act.movement.hpp"
#include "dbat/obj_edit.hpp"
#include "dbat/graph.hpp"
#include "dbat/alias.hpp"
#include "dbat/spells.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/fight.hpp"
#include "dbat/class.hpp"
#include "dbat/shop.hpp"
#include "dbat/feats.hpp"
#include "dbat/guild.hpp"
#include "dbat/dg_scripts.hpp"
#include "dbat/objsave.hpp"
#include "dbat/mail.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/game_world.hpp"

#include "other_internal.hpp"

static void boost_obj(GameWorld *g, struct obj_data *obj, struct char_data *ch, int type);

/* This handles increasing the stats of a created object based on the skill *
 * and/or stats of the user.                                                */
static void boost_obj(GameWorld *g, struct obj_data *obj, struct char_data *ch, int type)
{

  if (!obj || !ch)
    return;

  int boost = 0;

  if (char_get_level(ch) >= 100) {
    boost = 100;
  } else if (char_get_level(ch) >= 90) {
    boost = 90;
  } else if (char_get_level(ch) >= 80) {
    boost = 80;
  } else if (char_get_level(ch) >= 70) {
    boost = 70;
  } else if (char_get_level(ch) >= 60) {
    boost = 60;
  } else if (char_get_level(ch) >= 50) {
    boost = 50;
  } else if (char_get_level(ch) >= 40) {
    boost = 40;
  } else if (char_get_level(ch) >= 30) {
    boost = 30;
  }

  switch (type) {       /* Main switch of boost_obj */
    case 0:             /* This object is a piece of worn equipment, not a weapon. */
      if (boost != 0) { /* Change it if it qualifies */
        obj_set_level(obj, boost);
        obj_get_affected(obj)[0].location = 17;
        obj_get_affected(obj)[0].modifier += (boost * char_get_level(ch));
        if (obj_get_vnum(g, obj) == 91) {
          obj_get_affected(obj)[1].location = 1;
          obj_get_affected(obj)[1].modifier = (boost / 20);
        } else {
          obj_get_affected(obj)[1].location = 3;
          obj_get_affected(obj)[1].modifier = (boost / 20);
        }
      }
      break;
    case 1: /* This object is a weapon. */
      switch (boost) {
        case 30:
          obj_extra_set(obj, ITEM_WEAPLVL2, true);
          break;
        case 40:
        case 50:
          obj_extra_set(obj, ITEM_WEAPLVL3, true);
          break;
        case 60:
        case 70:
        case 80:
        case 90:
          obj_extra_set(obj, ITEM_WEAPLVL4, true);
          break;
        case 100:
          obj_extra_set(obj, ITEM_WEAPLVL5, true);
          break;
        default:
          obj_extra_set(obj, ITEM_WEAPLVL1, true);
          break;
      }
      if (boost != 0) {
        obj_set_level(obj, boost);
        obj_get_affected(obj)[0].location = 1;
        obj_get_affected(obj)[0].modifier = (boost / 20);
      }
      break;
  } /* End of main switch */
}

ACMD(do_form)
{
  int skill = 0, senzu = false, bag = false, light = false, sword = false, mattress = false, gi = false, pants = false, kachin = false, boost = false, shuriken = false;
  int clothes = false, wrist = false, boots = false, level = 0;
  double discount = 1.0;
  int64_t cost    = 0;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH], clam[MAX_INPUT_LENGTH];

  half_chop(argument, arg, clam);

  half_chop(clam, arg2, arg3);

  if (!know_skill(g, ch, SKILL_CREATE)) {
    return;
  }

  /* -- code disabled as of 10/24/2021
  if (GET_COOLDOWN(ch) > 0) {
   send_to_char(ch, "You must wait a short period before concentrating again.\r\n");
   return;
  }
   */

  skill = char_get_skill(ch, SKILL_CREATE);

  if (skill >= 100) {
    boost = true;
  }
  if (skill >= 90) {
    kachin = true;
  }
  if (skill >= 80) {
    senzu = true;
  }
  if (skill >= 70) {
    shuriken = true;
  }
  if (skill >= 60) {
    clothes = true;
  }
  if (skill >= 50) {
    sword = true;
    gi    = true;
    pants = true;
    wrist = true;
    boots = true;
  }
  if (skill >= 40) {
    mattress = true;
  }
  if (skill >= 30) {
    bag = true;
  }
  if (skill >= 20) {
    light = true;
  }

  if (char_get_skill(ch, SKILL_CONCENTRATION)) {
    if (char_get_skill(ch, SKILL_CONCENTRATION) >= 100) {
      discount = 0.5;
    } else if (char_get_skill(ch, SKILL_CONCENTRATION) >= 90) {
      discount = 0.6;
    } else if (char_get_skill(ch, SKILL_CONCENTRATION) >= 80) {
      discount = 0.65;
    } else if (char_get_skill(ch, SKILL_CONCENTRATION) >= 70) {
      discount = 0.7;
    } else if (char_get_skill(ch, SKILL_CONCENTRATION) >= 60) {
      discount = 0.75;
    } else if (char_get_skill(ch, SKILL_CONCENTRATION) >= 50) {
      discount = 0.8;
    } else if (char_get_skill(ch, SKILL_CONCENTRATION) >= 40) {
      discount = 0.85;
    } else if (char_get_skill(ch, SKILL_CONCENTRATION) >= 30) {
      discount = 0.9;
    } else if (char_get_skill(ch, SKILL_CONCENTRATION) >= 20) {
      discount = 0.95;
    }
  }

  if (!*arg) {
    send_to_char(g, ch, "What do you want to create?\r\n"
                     "@GCreation @WMenu@n\r\n"
                     "@D---------------@n\r\n"
                     "@wcreate food\r\n"
                     "create water\r\n"
                     "%s%s%s%s%s%s%s%s%s%s%s%s%s\r\n",
                 light ? "create light\r\n" : "", bag ? "create bag\r\n" : "", mattress ? "create mattress\r\n" : "",
                 sword ? "create weapon (sword | club | dagger | spear | gun )\r\n" : "", pants ? "create pants\r\n" : "", gi ? "create gi\r\n" : "",
                 wrist ? "create wristband\r\n" : "", boots ? "create boots\r\n" : "", clothes ? "create clothesbeam (target)\r\n" : "", shuriken ? "create shuriken\r\n" : "", senzu ? "create senzu\r\n" : "",
                 kachin ? "create kachin\r\n" : "", boost ? "create elixir\r\n" : "");
    return;
  }
  reveal_hiding(g, ch, 0);
  if (!(strcmp(arg, "food"))) {
    cost = getMaxKI(g, ch) / MAX(skill / 2, 1);
    cost *= discount;

    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    if (!*arg2) {
      send_to_char(g, ch, "Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | highest)\r\n");
      send_to_char(g, ch, "If you are capable you will make it. If not you will make a low quality version.\r\n");
    } else if (*arg2) {
      if (!strcasecmp(arg2, "highest") && skill >= 100) {
        level = 4;
      } else if (!strcasecmp(arg2, "high") && skill >= 75) {
        level = 3;
      } else if (!strcasecmp(arg2, "mid") && skill >= 50) {
        level = 2;
      } else {
        level = 1;
      }
    }
    if (level == 4) {
      obj = read_object(g, 1512);
      add_unique_id(g, obj);
    } else if (level == 3) {
      obj = read_object(g, 1511);
      add_unique_id(g, obj);
    } else if (level == 2) {
      obj = read_object(g, 1510);
      add_unique_id(g, obj);
    } else {
      obj = read_object(g, 70);
      add_unique_id(g, obj);
    }
    obj_to_char(g, obj, ch);
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    return;
  }
  if (!(strcmp(arg, "water"))) {
    cost = getMaxKI(g, ch) / (skill * 2);
    cost *= discount;

    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    if (!*arg2) {
      send_to_char(g, ch, "Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | highest)\r\n");
      send_to_char(g, ch, "If you are capable you will make it. If not you will make a low quality version.\r\n");
    } else if (*arg2) {
      if (!strcasecmp(arg2, "highest") && skill >= 100) {
        level = 4;
      } else if (!strcasecmp(arg2, "high") && skill >= 75) {
        level = 3;
      } else if (!strcasecmp(arg2, "mid") && skill >= 50) {
        level = 2;
      } else {
        level = 1;
      }
    }
    if (level == 4) {
      obj = read_object(g, 1515);
      add_unique_id(g, obj);
    } else if (level == 3) {
      obj = read_object(g, 1514);
      add_unique_id(g, obj);
    } else if (level == 2) {
      obj = read_object(g, 1513);
      add_unique_id(g, obj);
    } else {
      obj = read_object(g, 71);
      add_unique_id(g, obj);
    }
    obj_to_char(g, obj, ch);
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    return;
  }
  if (!(strcmp(arg, "bag"))) {
    cost = getMaxKI(g, ch) / (skill * 2);
    cost *= discount;

    if (bag == false) {
      send_to_char(g, ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    obj = read_object(g, 319);
    add_unique_id(g, obj);
    obj_to_char(g, obj, ch);
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    return;
  }
  if (!(strcmp(arg, "mattress"))) {
    cost = getMaxKI(g, ch) / skill;
    cost *= discount;

    if (mattress == false) {
      send_to_char(g, ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    obj = read_object(g, 16);
    add_unique_id(g, obj);
    obj_to_char(g, obj, ch); // cooldown removed on 10/24/2021
    reveal_hiding(g, ch, 0); //GET_COOLDOWN(ch) = 10;
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    return;
  }
  if (!(strcmp(arg, "weapon"))) {
    cost = getMaxKI(g, ch) / 5;
    cost *= discount;

    if (sword == false) {
      send_to_char(g, ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    if (!*arg2) {
      send_to_char(g, ch, "What type of weapon?\r\nSyntax: create weapon (sword | club | spear | dagger | gun)\r\n");
      return;
    }
    if (!*arg3) {
      send_to_char(g, ch, "Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | higher | highest)\r\n");
      send_to_char(g, ch, "If you are capable you will make it. If not you will make a low quality version.\r\n");
    } else if (*arg3) {
      if (!strcasecmp(arg3, "highest") && skill >= 100) {
        level = 5;
      } else if (!strcasecmp(arg3, "higher") && skill >= 75) {
        level = 4;
      } else if (!strcasecmp(arg3, "high") && skill >= 50) {
        level = 3;
      } else if (!strcasecmp(arg3, "mid") && skill >= 30) {
        level = 2;
      } else {
        level = 1;
      }
    }
    if (!strcasecmp(arg2, "sword")) {
      if (level == 5) {
        obj = read_object(g, 1519);
        add_unique_id(g, obj);
      } else if (level == 4) {
        obj = read_object(g, 1518);
        add_unique_id(g, obj);
      } else if (level == 3) {
        obj = read_object(g, 1517);
        add_unique_id(g, obj);
      } else if (level == 2) {
        obj = read_object(g, 1516);
        add_unique_id(g, obj);
      } else {
        obj = read_object(g, 90);
        add_unique_id(g, obj);
      }
    } else if (!strcasecmp(arg2, "dagger")) {
      if (level == 5) {
        obj = read_object(g, 1540);
        add_unique_id(g, obj);
      } else if (level == 4) {
        obj = read_object(g, 1539);
        add_unique_id(g, obj);
      } else if (level == 3) {
        obj = read_object(g, 1538);
        add_unique_id(g, obj);
      } else if (level == 2) {
        obj = read_object(g, 1537);
        add_unique_id(g, obj);
      } else {
        obj = read_object(g, 1536);
        add_unique_id(g, obj);
      }
    } else if (!strcasecmp(arg2, "club")) {
      if (level == 5) {
        obj = read_object(g, 1545);
        add_unique_id(g, obj);
      } else if (level == 4) {
        obj = read_object(g, 1544);
        add_unique_id(g, obj);
      } else if (level == 3) {
        obj = read_object(g, 1543);
        add_unique_id(g, obj);
      } else if (level == 2) {
        obj = read_object(g, 1542);
        add_unique_id(g, obj);
      } else {
        obj = read_object(g, 1541);
        add_unique_id(g, obj);
      }
    } else if (!strcasecmp(arg2, "spear")) {
      if (level == 5) {
        obj = read_object(g, 1550);
        add_unique_id(g, obj);
      } else if (level == 4) {
        obj = read_object(g, 1549);
        add_unique_id(g, obj);
      } else if (level == 3) {
        obj = read_object(g, 1548);
        add_unique_id(g, obj);
      } else if (level == 2) {
        obj = read_object(g, 1547);
        add_unique_id(g, obj);
      } else {
        obj = read_object(g, 1546);
        add_unique_id(g, obj);
      }
    } else if (!strcasecmp(arg2, "gun")) {
      if (level == 5) {
        obj = read_object(g, 1555);
        add_unique_id(g, obj);
      } else if (level == 4) {
        obj = read_object(g, 1554);
        add_unique_id(g, obj);
      } else if (level == 3) {
        obj = read_object(g, 1553);
        add_unique_id(g, obj);
      } else if (level == 2) {
        obj = read_object(g, 1552);
        add_unique_id(g, obj);
      } else {
        obj = read_object(g, 1551);
        add_unique_id(g, obj);
      }
    } else {
      send_to_char(g, ch, "What type of weapon?\r\nSyntax: create weapon (sword | club | spear | dagger | gun)\r\n");
      return;
    }
    obj_to_char(g, obj, ch);
    obj_set_size(obj, get_size(ch));
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    return;
  }
  if (!(strcmp(arg, "clothesbeam"))) {
    cost = getMaxKI(g, ch) / 2;
    cost *= discount;

    if (clothes == false) {
      send_to_char(g, ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    if (!*arg2) {
      send_to_char(g, ch, "Who do you want to hit with clothesbeam?\r\nSyntax: create clothesbeam (target)\r\n");
      return;
    }

    struct char_data *vict = nullptr;

    if (!(vict = get_char_vis(g, ch, arg2, nullptr, FIND_CHAR_ROOM))) {
      send_to_char(g, ch, "Clothesbeam who?\r\nSyntax: create clothesbeam (target)\r\n");
      return;
    }

    if (char_get_master(g, vict) != ch) {
      send_to_char(g, ch, "They must be following you first.\r\n");
      return;
    }
    obj = read_object(g, 92); /* gi */
    add_unique_id(g, obj);
    boost_obj(g, obj, ch, 0);
    obj_to_char(g, obj, vict);
    obj_set_size(obj, get_size(vict));
    obj = read_object(g, 91); /* pants */
    add_unique_id(g, obj);
    boost_obj(g, obj, ch, 0);
    obj_to_char(g, obj, vict);
    obj_set_size(obj, get_size(vict));
    obj = read_object(g, 1528); /* wrist */
    add_unique_id(g, obj);
    boost_obj(g, obj, ch, 0);
    obj_to_char(g, obj, vict);
    obj_set_size(obj, get_size(vict));
    obj = read_object(g, 1528); /* wrist */
    add_unique_id(g, obj);
    boost_obj(g, obj, ch, 0);
    obj_to_char(g, obj, vict);
    obj_set_size(obj, get_size(vict));
    obj = read_object(g, 1532); /* boots */
    add_unique_id(g, obj);
    boost_obj(g, obj, ch, 0);
    obj_to_char(g, obj, vict);
    obj_set_size(obj, get_size(vict));
    do_wear(g, vict, writable{"all"}, 0, 0);
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    return;
  }
  if (!(strcmp(arg, "gi"))) {
    cost = getMaxKI(g, ch) / 5;
    cost *= discount;
    if (gi == false) {
      send_to_char(g, ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    obj = read_object(g, 92);
    add_unique_id(g, obj);
    boost_obj(g, obj, ch, 0);
    obj_to_char(g, obj, ch);
    obj_set_size(obj, get_size(ch));
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    return;
  }
  if (!(strcmp(arg, "shuriken"))) {
    cost = getMaxKI(g, ch) / 4;
    cost *= discount;

    if (shuriken == false) {
      send_to_char(g, ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    obj = read_object(g, 19053);
    add_unique_id(g, obj);
    obj_to_char(g, obj, ch);
    obj_extra_set(obj, ITEM_NORENT, true);
    obj_extra_set(obj, ITEM_NOSELL, true);
    obj_set_size(obj, get_size(ch));
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    return;
  }
  if (!(strcmp(arg, "pants"))) {
    cost = getMaxKI(g, ch) / 5;
    cost *= discount;

    if (pants == false) {
      send_to_char(g, ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    obj = read_object(g, 91);
    add_unique_id(g, obj);
    boost_obj(g, obj, ch, 0);
    obj_to_char(g, obj, ch);
    obj_set_size(obj, get_size(ch));
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    return;
  }
  if (!(strcmp(arg, "wristband"))) {
    cost = getMaxKI(g, ch) / 5;
    cost *= discount;

    if (wrist == false) {
      send_to_char(g, ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    obj = read_object(g, 1528);
    add_unique_id(g, obj);
    boost_obj(g, obj, ch, 0);
    obj_to_char(g, obj, ch);
    obj_set_size(obj, get_size(ch));
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    return;
  }
  if (!(strcmp(arg, "boots"))) {
    cost = getMaxKI(g, ch) / 5;
    cost *= discount;

    if (boots == false) {
      send_to_char(g, ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    obj = read_object(g, 1532);
    add_unique_id(g, obj);
    boost_obj(g, obj, ch, 0);
    obj_to_char(g, obj, ch);
    obj_set_size(obj, get_size(ch));
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    return;
  }
  if (!(strcmp(arg, "light"))) {
    cost = getMaxKI(g, ch) / (skill * 2);
    cost *= discount;

    if (light == false) {
      send_to_char(g, ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    obj = read_object(g, 72);
    add_unique_id(g, obj);
    obj_to_char(g, obj, ch);
    obj_set_size(obj, get_size(ch));
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    return;
  }
  if (!(strcmp(arg, "kachin"))) {
    cost = getMaxKI(g, ch) - 1;
    cost *= discount;

    if (kachin == false) {
      send_to_char(g, ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    obj = read_object(g, 87);
    add_unique_id(g, obj);
    obj_to_room(g, obj, char_get_in_room(ch));
    obj_set_size(obj, get_size(ch));
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    return;
  }
  if (!(strcmp(arg, "elixir"))) {
    cost = getMaxKI(g, ch) - 1;
    cost *= discount;

    if (boost == false) {
      send_to_char(g, ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    if (getCurPL(g, ch) < getEffMaxPL(g, ch)) {
      send_to_char(g, ch, "You need to be at full powerlevel to create %s\r\n", arg);
      return;
    }
    if (char_get_practice_points(g, ch) < 10) {
      send_to_char(g, ch, "You do not have enough PS to create %s, you need at least 10.\r\n", arg);
      return;
    }
    obj = read_object(g, 86);
    add_unique_id(g, obj);
    obj_to_room(g, obj, char_get_in_room(ch));
    obj_set_size(obj, get_size(ch));
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    decCurHealthPercent(g, ch, 1, 1);
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (10));
    return;
  }
  if (!(strcmp(arg, "senzu"))) {
    cost          = getMaxKI(g, ch);
    int64_t cost2 = getEffMaxPL(g, ch) - 1;

    if (senzu == false) {
      send_to_char(g, ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(g, ch)) < cost) {
      send_to_char(g, ch, "You do not have enough ki to create %s, you need full ki.\r\n", arg);
      return;
    }
    if (getCurPL(g, ch) <= cost2) {
      send_to_char(g, ch, "You do not have enough powerlevel to create %s, you need to be at full.\r\n", arg);
      return;
    }
    if ((getCurST(g, ch)) < getMaxST(g, ch)) {
      send_to_char(g, ch, "You do not have enough stamina to create %s, you need to be at full.\r\n", arg);
      return;
    }
    if (char_get_practice_points(g, ch) < 50) {
      send_to_char(g, ch, "You do not have enough PS to create %s, you need at least 50.\r\n", arg);
      return;
    }
    obj = read_object(g, 1);
    add_unique_id(g, obj);
    obj_to_char(g, obj, ch);
    reveal_hiding(g, ch, 0);
    char_set_con_cooldown(ch, 10);
    act(g, "You hold out your hand and create $p out of your ki!", true, ch, obj, nullptr, TO_CHAR);
    act(g, "$n holds out $s hand and creates $p out of thin air!", true, ch, obj, nullptr, TO_ROOM);
    decCurKI(g, ch, cost, 0);
    decCurHealth(g, ch, cost2, 0);
    decCurSTPercent(g, ch, 1, 1);
    char_set_practice_points(g, ch, char_get_practice_points(g, ch) - (50));
    return;
  }
  send_to_char(g, ch, "Create what?\r\n");
  return;
}

ACMD(do_recharge)
{

  if (char_is_npc(ch) || !char_is_android(ch)) {
    send_to_char(g, ch, "Only androids can use recharge\r\n");
    return;
  }

  if (char_get_con_cooldown(ch) > 0) {
    send_to_char(g, ch, "You must wait a short period before your nanites can convert your ki.\r\n");
    return;
  }

  if (!char_plr_flagged(ch, PLR_REPAIR)) {
    send_to_char(g, ch, "You are not a repair model android.\r\n");
    return;
  }
  int64_t cost = 0;

  cost = getMaxST(g, ch) / 20;

  if ((getCurKI(g, ch)) < cost) {
    send_to_char(g, ch, "You do not have enough ki to recharge your stamina.\r\n");
    return;
  }
  if ((getCurST(g, ch)) >= getMaxST(g, ch)) {
    send_to_char(g, ch, "Your energy reserves are already full.\r\n");
    return;
  }
  reveal_hiding(g, ch, 0);
  char_set_con_cooldown(ch, 10);
  act(g, "You focus your ki into your energy reserves, recharging them some.", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "$n stops and glows green briefly.", true, ch, nullptr, nullptr, TO_ROOM);
  decCurKI(g, ch, cost, 0);
  if ((getCurST(g, ch)) + (cost * 2) < getMaxST(g, ch)) {
    incCurST(g, ch, cost * 2, true);
  } else {
    restoreST(ch, false);
    send_to_char(g, ch, "You are fully recharged now.\r\n");
  }
  char_set_wait(ch, PULSE_2SEC);
}

ACMD(do_srepair)
{
  int i;

  if (!char_is_android(ch)) {
    send_to_char(g, ch, "Only androids can use repair, maybe you want 'fix' instead?\r\n");
    return;
  }

  if (char_get_con_cooldown(ch) > 0) {
    send_to_char(g, ch, "You must wait a short period before your nanites can repair you.\r\n");
    return;
  }

  if (!char_is_npc(ch) && !char_plr_flagged(ch, PLR_REPAIR)) {
    send_to_char(g, ch, "You are not a repair model android.\r\n");
    return;
  }
  int64_t cost = 0, heal = 0;

  cost = getEffMaxPL(g, ch) / 40;

  if ((getCurST(g, ch)) < cost) {
    send_to_char(g, ch, "You do not have enough stamina to repair yourself.\r\n");
    return;
  }
  if (getCurPL(g, ch) >= getEffMaxPL(g, ch)) {
    send_to_char(g, ch, "You are already at full functionality and do not require repairs.\r\n");
    return;
  }
  reveal_hiding(g, ch, 0);
  char_set_con_cooldown(ch, 10);
  act(g, "You repair some of your outer casings and internal systems, with the small nano-robots contained in your body.", true, ch, nullptr, nullptr, TO_CHAR);
  act(g, "$n stops a moment as small glowing particles move across $s body.", true, ch, nullptr, nullptr, TO_ROOM);

  int repaired = false;
  if (!char_is_npc(ch)) {
    for (i = 0; i < NUM_WEARS; i++) {
      if (char_get_equipment(ch, i)) {
        if (obj_get_value(char_get_equipment(ch, i), VAL_ALL_HEALTH) < 100) {
          obj_set_value(char_get_equipment(ch, i), VAL_ALL_HEALTH, obj_get_value(char_get_equipment(ch, i), VAL_ALL_HEALTH) + (20));
          if (obj_get_value(char_get_equipment(ch, i), VAL_ALL_HEALTH) > 100) {
            obj_set_value(char_get_equipment(ch, i), VAL_ALL_HEALTH, 100);
          }
          if (obj_extra_flagged(char_get_equipment(ch, i), ITEM_BROKEN)) {
            obj_extra_set(char_get_equipment(ch, i), ITEM_BROKEN, false);
          }
          repaired = true;
        }
      }
    }
  }

  if (repaired == true) {
    send_to_char(g, ch, "@GYour nano-robots also repair all of your equipment a little bit.@n\r\n");
  }
  decCurST(g, ch, cost, 0);
  heal = cost * 2;
  if (char_get_bonuses(ch, BONUS_HEALER) > 0) {
    heal += heal * .25;
  }

  if (incCurHealth(g, ch, heal, true) == getEffMaxPL(g, ch)) {
    send_to_char(g, ch, "You are fully repaired now.\r\n");
  }

  if (!char_is_npc(ch) && rand_number(g, 1, 3) == 2 && (getCurKI(g, ch)) < getMaxKI(g, ch)) {
    send_to_char(g, ch, "@GThe repairs have managed to relink power reserves and boost your current energy level.@n\r\n");
    incCurKI(g, ch, cost, true);
  }
  char_set_wait(ch, PULSE_2SEC);
}

ACMD(do_upgrade)
{
  int count = 0, bonus = 0, cost = 0;
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (char_is_npc(ch) || !char_is_android(ch)) {
    send_to_char(g, ch, "You are not an android!\r\n");
    return;
  }

  if (!*arg) {
    if (!char_plr_flagged(ch, PLR_ABSORB)) {
      send_to_char(g, ch, "@c--------@D[@rUpgrade Menu@D]@c--------\r\n"
                       "@cUpgrade @RPowerlevel@D: @Y75 @WPoints\r\n"
                       "@cUpgrade @CKi        @D: @Y40 @WPoints\r\n"
                       "@cUpgrade @GStamina   @D: @Y50 @WPoints\r\n"
                       "@D            -----------\r\n");
    }
    send_to_char(g, ch, "@cAugment @RPowerlevel\r\n"
                     "@cAugment @CKi\r\n"
                     "@cAugment @GStamina\r\n"
                     "@WCurrent Upgrade Points @D[@y%s@D]@n\r\n",
                 add_commas(char_get_upgrade(ch)));
    return;
  }

  if (!strcasecmp("augment", arg)) {
    struct obj_data *obj = nullptr;
    int64_t gain         = 0;
    if (char_get_level(ch) < 80) {
      send_to_char(g, ch, "You need to be at least level 80 to use these kits.\r\n");
      return;
    }
    if (!(obj = get_obj_in_list_vis(g, ch, writable{"Augmentation"}, nullptr, char_get_inventory(ch)))) {
      send_to_char(g, ch, "You don't have a Circuit Augmentation Kit.\r\n");
      return;
    }
    switch (char_get_level(ch)) { /* R: Original was GET_LEVEL(ch) */
      case 80:
        gain = getEffMaxPL(g, ch) * 0.005;
        break;
      case 81:
      case 82:
      case 83:
      case 84:
      case 85:
      case 86:
      case 87:
      case 88:
      case 89:
      case 90:
        gain = getEffMaxPL(g, ch) * 0.005; /* R: Original was GET_LEVEL(ch) * 2000; */
        break;
      case 91:
      case 92:
      case 93:
      case 94:
      case 95:
      case 96:
      case 97:
      case 98:
      case 99:
        gain = getEffMaxPL(g, ch) * 0.005; /* R: Original was GET_LEVEL(ch) * 4000; */
        break;
      case 100:
        gain = getEffMaxPL(g, ch) * 0.005; /* R: Original was GET_LEVEL(ch) * 100000 */
        if (gain > 10000000) {
          gain = 10000000;
        }
        break;
    }
    if (!strcasecmp("powerlevel", arg2)) {
      obj_from_char(g, obj);
      extract_obj(g, obj);
      act(g, "@WYou install the circuits and upgrade your maximum powerlevel.@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@W installs some circuits and upgrades $s systems.@n", true, ch, nullptr, nullptr, TO_ROOM);
      gainBasePL(ch, gain, true);
      send_to_char(g, ch, "@gGain @D[@G+%s@D]\r\n", add_commas(gain));
      return;
    }
    if (!strcasecmp("ki", arg2)) {
      obj_from_char(g, obj);
      extract_obj(g, obj);
      act(g, "@WYou install the circuits and upgrade your maximum ki.@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@W installs some circuits and upgrades $s systems.@n", true, ch, nullptr, nullptr, TO_ROOM);
      gainBaseKI(ch, gain, true);
      send_to_char(g, ch, "@gGain @D[@G+%s@D]\r\n", add_commas(gain));
      return;
    }
    if (!strcasecmp("stamina", arg2)) {
      obj_from_char(g, obj);
      extract_obj(g, obj);
      act(g, "@WYou install the circuits and upgrade your maximum stamina.@n", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "@C$n@W installs some circuits and upgrades $s systems.@n", true, ch, nullptr, nullptr, TO_ROOM);
      gainBaseST(ch, gain, true);
      send_to_char(g, ch, "@gGain @D[@G+%s@D]\r\n", add_commas(gain));
      return;
    }
    send_to_char(g, ch, "What do you want to augment? Powerlevel, ki, or stamina?\r\n");
    return;
  }

  if (char_plr_flagged(ch, PLR_ABSORB)) {
    send_to_char(g, ch, "You are an absorb model and can only upgrade with augmentation kits.\r\n");
    return;
  }

  if (is_soft_cap(ch, 0, 1.0)) {
    send_to_char(g, ch, "@mYou are unable to spend anymore UGP right now (Softcap)@n\r\n");
    return;
  }

  if (!*arg2 && (!strcasecmp("powerlevel", arg) || !strcasecmp("ki", arg) || !strcasecmp("stamina", arg))) {
    send_to_char(g, ch, "How many times do you want to increase %s?", arg);
    return;
  }

  if (atoi(arg2) <= 0 && (!strcasecmp("powerlevel", arg) || !strcasecmp("ki", arg) || !strcasecmp("stamina", arg))) {
    send_to_char(g, ch, "It needs to be between 1-1000\r\n");
    return;
  }

  if (atoi(arg2) > 1000 && (!strcasecmp("powerlevel", arg) || !strcasecmp("ki", arg) || !strcasecmp("stamina", arg))) {
    send_to_char(g, ch, "It needs to be between 1-1000\r\n");
    return;
  }

  if (!strcasecmp("powerlevel", arg)) {
    count = atoi(arg2);
    while (count > 0) {
      if (char_get_level(ch) >= 90) {
        bonus += char_get_level(ch) * 5000;
      } else if (char_get_level(ch) >= 80) {
        bonus += char_get_level(ch) * 2500;
      } else if (char_get_level(ch) >= 70) {
        bonus += char_get_level(ch) * 2000;
      } else if (char_get_level(ch) >= 60) {
        bonus += char_get_level(ch) * 1300;
      } else if (char_get_level(ch) >= 60) {
        bonus += char_get_level(ch) * 1200;
      } else if (char_get_level(ch) >= 50) {
        bonus += char_get_level(ch) * 500;
      } else if (char_get_level(ch) >= 25) {
        bonus += char_get_level(ch) * 250;
      } else {
        bonus += char_get_level(ch) * 150;
      }
      cost += 75;
      count--;
    }
    if (cost > char_get_upgrade(ch)) {
      send_to_char(g, ch, "You need %s upgrade points, and only have %s.\r\n", add_commas(cost), add_commas(char_get_upgrade(ch)));
      return;
    }
    if (is_soft_cap(ch, bonus, 1.0)) {
      send_to_char(g, ch, "@mYou can't spend that much UGP on it as it will go over your softcap.@n\r\n");
      return;
    }
    char_set_upgrade(ch, char_get_upgrade(ch) - (cost));
    send_to_char(g, ch, "You upgrade your system and gain %s %s!", add_commas(bonus), arg);
    gainBasePL(ch, bonus, true);
  } else if (!strcasecmp("ki", arg)) {
    count = atoi(arg2);
    while (count > 0) {
      if (char_get_level(ch) >= 90) {
        bonus += char_get_level(ch) * 3650;
      } else if (char_get_level(ch) >= 80) {
        bonus += char_get_level(ch) * 2450;
      } else if (char_get_level(ch) >= 70) {
        bonus += char_get_level(ch) * 1800;
      } else if (char_get_level(ch) >= 60) {
        bonus += char_get_level(ch) * 1250;
      } else if (char_get_level(ch) >= 60) {
        bonus += char_get_level(ch) * 1150;
      } else if (char_get_level(ch) >= 50) {
        bonus += char_get_level(ch) * 400;
      } else if (char_get_level(ch) >= 25) {
        bonus += char_get_level(ch) * 200;
      } else {
        bonus += char_get_level(ch) * 120;
      }
      cost += 40;
      count--;
    }
    if (cost > char_get_upgrade(ch)) {
      send_to_char(g, ch, "You need %s upgrade points, and only have %s.\r\n", add_commas(cost), add_commas(char_get_upgrade(ch)));
      return;
    }
    if (is_soft_cap(ch, bonus, 1.0)) {
      send_to_char(g, ch, "@mYou can't spend that much UGP on it as it will go over your softcap.@n\r\n");
      return;
    }
    char_set_upgrade(ch, char_get_upgrade(ch) - (cost));
    send_to_char(g, ch, "You upgrade your system and gain %s %s!", add_commas(bonus), arg);
    gainBaseKI(ch, bonus, true);
  } else if (!strcasecmp("stamina", arg)) {
    count = atoi(arg2);
    while (count > 0) {
      if (char_get_level(ch) >= 90) {
        bonus += char_get_level(ch) * 3650;
      } else if (char_get_level(ch) >= 80) {
        bonus += char_get_level(ch) * 2450;
      } else if (char_get_level(ch) >= 70) {
        bonus += char_get_level(ch) * 1800;
      } else if (char_get_level(ch) >= 60) {
        bonus += char_get_level(ch) * 1250;
      } else if (char_get_level(ch) >= 60) {
        bonus += char_get_level(ch) * 1150;
      } else if (char_get_level(ch) >= 50) {
        bonus += char_get_level(ch) * 500;
      } else if (char_get_level(ch) >= 25) {
        bonus += char_get_level(ch) * 200;
      } else {
        bonus += char_get_level(ch) * 120;
      }
      cost += 50;
      count--;
    }
    if (cost > char_get_upgrade(ch)) {
      send_to_char(g, ch, "You need %s upgrade points, and only have %s.\r\n", add_commas(cost), add_commas(char_get_upgrade(ch)));
      return;
    }
    if (is_soft_cap(ch, bonus, 1.0)) {
      send_to_char(g, ch, "@mYou can't spend that much UGP on it as it will go over your softcap.@n\r\n");
      return;
    }
    char_set_upgrade(ch, char_get_upgrade(ch) - (cost));
    send_to_char(g, ch, "You upgrade your system and gain %s %s!", add_commas(bonus), arg);
    gainBaseST(ch, bonus, true);
  } else {
    send_to_char(g, ch, "That is not a valid upgrade option.\r\n");
    return;
  }
}

ACMD(do_ingest)
{

  if (char_is_majin(ch)) {
    struct char_data *vict;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (!*arg) {
      send_to_char(g, ch, "Who do you want to ingest?\r\n");
      return;
    }

    if (!(vict = get_char_vis(g, ch, arg, nullptr, FIND_CHAR_ROOM))) {
      send_to_char(g, ch, "Ingest who?\r\n");
      return;
    }
    if (!can_kill(g, ch, vict, nullptr, 0)) {
      return;
    }
    if (char_get_absorbby(g, vict)) {
      send_to_char(g, ch, "%s is already absorbing from them!", char_get_name(char_get_absorbby(g, vict)));
      return;
    }
    if (char_get_absorbs(ch) > 3) {
      send_to_char(g, ch, "You already have already ingested 4 people.\r\n");
      return;
    }
    if (char_get_level(ch) < 25) {
      send_to_char(g, ch, "You can't ingest yet.\r\n");
      return;
    }
    if (char_get_level(ch) < 100 && char_get_level(ch) >= 75 && char_get_absorbs(ch) == 3) {
      send_to_char(g, ch, "You already have ingested as much as you can. You'll have to get more experienced.\r\n");
      return;
    }
    if (char_get_level(ch) < 75 && char_get_level(ch) >= 50 && char_get_absorbs(ch) == 2) {
      send_to_char(g, ch, "You already have ingested as much as you can. You'll have to get more experienced.\r\n");
      return;
    }
    if (char_get_level(ch) < 50 && char_get_level(ch) >= 25 && char_get_absorbs(ch) == 1) {
      send_to_char(g, ch, "You already have ingested as much as you can. You'll have to get more experienced.\r\n");
      return;
    }

    if (getEffMaxPL(g, vict) >= (getBasePL(ch)) * 3) {
      send_to_char(g, ch, "You are too weak to ingest them into your body!\r\n");
      return;
    }
    if (char_aff_flagged(vict, AFF_SANCTUARY)) {
      send_to_char(g, ch, "You can't ingest them, they have a barrier!\r\n");
      return;
    }
    reveal_hiding(g, ch, 0);
    if (char_aff_flagged(vict, AFF_ZANZOKEN) && (getCurST(g, vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
      act(g, "@C$N@c disappears, avoiding your attempted ingestion!@n", false, ch, nullptr, vict, TO_CHAR);
      act(g, "@cYou disappear, avoiding @C$n's@c attempted @ringestion@c before reappearing!@n", false, ch, nullptr, vict, TO_VICT);
      act(g, "@C$N@c disappears, avoiding @C$n's@c attempted @ringestion@c before reappearing!@n", false, ch, nullptr, vict, TO_NOTVICT);
      char_aff_set(vict, AFF_ZANZOKEN, false);
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    if (char_get_speedi(g, ch) + rand_number(g, 1, 5) < char_get_speedi(g, ch) + rand_number(g, 1, 5)) {
      act(g, "@WYou fling a piece of goo at @c$N@W, and try to ingest $M! $E manages to avoid your blob of goo though!@n", true, ch, nullptr, vict, TO_CHAR);
      act(g, "@C$n@W flings a piece of goo at you, you manage to avoid it though!@n", true, ch, nullptr, vict, TO_VICT);
      act(g, "@C$n@w flings a piece of goo at @c$N@W, but the goo misses $M@W!@n", true, ch, nullptr, vict, TO_NOTVICT);
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    act(g, "@WYou flings a piece of goo at @c$N@W! The goo engulfs $M and then returns to your body!@n", true, ch, nullptr, vict, TO_CHAR);
    act(g, "@C$n@W flings a piece of goo at you! The goo engulfs your body and then returns to @C$n@W!@n", true, ch, nullptr, vict, TO_VICT);
    act(g, "@C$n@w flings a piece of goo at @c$N@W! The goo engulfs $M and then return to @C$n@W!@n", true, ch, nullptr, vict, TO_NOTVICT);
    char_set_absorbs(ch, char_get_absorbs(ch) + (1));
    int64_t pl   = (getBasePL(vict)) / 6;
    int64_t stam = (getBaseST(vict)) / 6;
    int64_t ki   = (getBaseKI(vict)) / 6;
    gainBasePL(ch, pl, true);
    gainBaseST(ch, stam, true);
    gainBaseKI(ch, ki, true);
    if (!char_is_npc(vict) && !char_is_npc(ch)) {
      send_to_imm(g, "[PK] %s killed %s at room [%d]\r\n", char_get_name(ch), char_get_name(vict), char_get_in_room(vict));
      char_plr_set(vict, PLR_ABSORBED, true);
    }
    send_to_char(g, ch, "@D[@mINGEST@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n\r\n", add_commas(pl), add_commas(ki), add_commas(stam));
    if (rand_number(g, 1, 3) == 3) {
      send_to_char(g, ch, "You get %s's eye color.\r\n", char_get_name(vict));
      char_set_eye(ch, char_get_eye(vict));
    } else if (rand_number(g, 1, 3) == 3) {
      send_to_char(g, ch, "%s changes your height.\r\n", char_get_name(vict));
      if (char_get_pc_height(g, ch) > char_get_pc_height(g, vict)) {
        char_set_height(ch, char_get_height(ch) - (((char_get_pc_height(g, ch) - char_get_pc_height(g, vict)) / 2)));
      } else if (char_get_pc_height(g, ch) < char_get_pc_height(g, vict)) {
        char_set_height(ch, char_get_height(ch) + (((char_get_pc_height(g, vict) - char_get_pc_height(g, ch)) / 2)));
      } else {
        char_set_height(ch, char_get_pc_height(g, vict));
      }
    } else if (rand_number(g, 1, 3) == 3) {
      send_to_char(g, ch, "%s changes your weight.\r\n", char_get_name(vict));
      if (char_get_pc_weight(g, ch) > char_get_pc_weight(g, vict)) {
        char_set_weight(ch, char_get_weight(ch) - (((char_get_pc_weight(g, ch) - char_get_pc_weight(g, vict)) / 2)));
      } else if (char_get_pc_weight(g, ch) < char_get_pc_weight(g, vict)) {
        char_set_weight(ch, char_get_weight(ch) + (((char_get_pc_weight(g, vict) - char_get_pc_weight(g, ch)) / 2)));
      } else {
        char_set_weight(ch, char_get_pc_weight(g, vict));
      }
    } else {
      send_to_char(g, ch, "Your forelock length changes because of %s.\r\n", char_get_name(vict));
      char_set_hairl(ch, char_get_hairl(vict));
    }
    handle_ingest_learn(g, ch, vict);
    die(g, vict, nullptr);
    return;
  } // End of ingest

  else {
    send_to_char(g, ch, "You are not a majin, you can not ingest.\r\n");
    return;
  } // Error
}

ACMD(do_plant)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int roll = 0, detect = 0, fail = 0;

  if (room_flagged(g, char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(g, ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  two_arguments(argument, obj_name, vict_name);

  if (!(vict = get_char_vis(g, ch, vict_name, nullptr, FIND_CHAR_ROOM))) {
    send_to_char(g, ch, "Plant what on who?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(g, ch, "Come on now, that's rather stupid!\r\n");
    return;
  }
  if (char_mob_flagged(vict, MOB_NOKILL) && char_get_admlevel(ch) == ADMLVL_NONE) {
    send_to_char(g, ch, "That isn't such a good idea...\r\n");
    return;
  }

  roll = roll_skill(g, ch, SKILL_SLEIGHT_OF_HAND) + rand_number(g, 1, 3);
  fail = rand_number(g, 1, 105);

  if (char_get_position(vict) < POS_SLEEPING)
    detect = 0;
  else
    detect = (roll_skill(g, vict, SKILL_SPOT) + rand_number(g, 1, 3));

  /* NO NO With Imp's and Shopkeepers, and if player planting is not allowed */
  if ((char_adm_flagged(vict, ADM_NOSTEAL) || char_get_mob_spec(g, vict) == shop_keeper) && char_get_admlevel(ch) < 5)
    roll = -10; /* Failure */

  if (!(obj = get_obj_in_list_vis(g, ch, obj_name, nullptr, char_get_inventory(ch)))) {
    send_to_char(g, ch, "You don't have that to plant on them.\r\n");
    return;
  }
  if (roll <= detect && roll <= fail) {
    reveal_hiding(g, ch, 0);
    act(g, "@C$n@w tries to plant $p@w on you!@n", true, ch, obj, vict, TO_VICT);
    act(g, "@C$n@w tries to plant $p@w on @c$N@w!@n", true, ch, obj, vict, TO_NOTVICT);
    act(g, "@wYou try and fail to plant $p@w on @c$N@w, and $E notices!@n", true, ch, obj, vict, TO_CHAR);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
  if (roll <= fail) {
    act(g, "@wYou try and fail to plant $p@w on @c$N@w! However no one seemed to notice.@n", true, ch, obj, vict, TO_CHAR);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
  if (obj_get_weight(obj) + (getCurCarriedWeight(vict)) > getMaxCarryWeight(g, vict)) {
    reveal_hiding(g, ch, 0);
    act(g, "@C$n@w tries to plant $p@w on you!@n", true, ch, obj, vict, TO_VICT);
    act(g, "@C$n@w tries to plant $p@w on @c$N@w!@n", true, ch, obj, vict, TO_NOTVICT);
    act(g, "@wYou try and fail to plant $p@w on @c$N@w because $E can't carry the weight. It seems $E noticed the attempt!@n", true, ch, obj, vict, TO_CHAR);
    char_set_wait(ch, PULSE_2SEC);
  } else if (roll <= detect) {
    act(g, "@cYou feel like the weight of your inventory has changed.@n", true, ch, obj, vict, TO_VICT);
    act(g, "@c$N@w looks around after feeling $S pockets.@n", true, ch, obj, vict, TO_NOTVICT);
    act(g, "@wYou plant $p@w on @c$N@w! @c$N @wseems to notice the change in weight in their inventory.@n", true, ch, obj, vict, TO_CHAR);
    obj_from_char(g, obj);
    obj_to_char(g, obj, vict);
    char_set_wait(ch, PULSE_2SEC);
    return;
  } else {
    act(g, "@wYou plant $p@w on @c$N@w! No one noticed, whew....@n", true, ch, obj, vict, TO_CHAR);
    obj_from_char(g, obj);
    obj_to_char(g, obj, vict);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
}

ACMD(do_forgery)
{

  struct obj_data *obj2, *obj3 = nullptr;
  struct obj_data *obj4 = nullptr;
  int found = false;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_is_npc(ch)) {
    return;
  }

  if (!know_skill(g, ch, SKILL_FORGERY)) {
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "Okay, make a forgery of what?\r\n");
    return;
  }

  if (!(obj2 = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
    send_to_char(g, ch, "You want to make a fake copy of what?\r\n");
    return;
  }

  for (obj_data *obj : char_inventory(g, ch)) {
    if (found == false && obj_get_vnum(g, obj) == 19 && (!obj_extra_flagged(obj, ITEM_BROKEN) && !obj_extra_flagged(obj, ITEM_FORGED))) {
      found = true;
      obj4  = obj;
    }
  }

  if (found == false || obj4 == nullptr) {
    send_to_char(g, ch, "You need a forgery kit.\r\n");
    return;
  }

  if (obj_get_vnum(g, obj2) == 19) {
    send_to_char(g, ch, "You can't duplicate a forgery kit.\r\n");
    return;
  }

  if (obj_extra_flagged(obj2, ITEM_FORGED)) {
    send_to_char(g, ch, "%s is forgery, there is no reason to make a fake of a fake!\r\n", obj_get_short_description(obj2));
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  if (obj_extra_flagged(obj2, ITEM_BROKEN)) {
    send_to_char(g, ch, "%s is broken, there is no reason to make a fake of this mess!\r\n", obj_get_short_description(obj2));
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  if (obj_get_vnum(g, obj2) >= 60000 || obj_get_vnum(g, obj2) == 0) {
    send_to_char(g, ch, "You can not make a forgery of that! It's far too squishy....");
    return;
  }
  if (obj_get_vnum(g, obj2) >= 18800 && obj_get_vnum(g, obj2) <= 18999) {
    send_to_char(g, ch, "You can not make a forgery of that!\r\n");
    return;
  }
  if (obj_get_vnum(g, obj2) >= 19080 && obj_get_vnum(g, obj2) <= 19199) {
    send_to_char(g, ch, "You can not make a forgery of that!\r\n");
    return;
  }
  if (obj_get_vnum(g, obj2) >= 4 && obj_get_vnum(g, obj2) <= 6) {
    send_to_char(g, ch, "You can not make a forgery of that!\r\n");
    return;
  }

  if (obj_extra_flagged(obj2, ITEM_PROTECTED)) {
    send_to_char(g, ch, "You don't know where to begin with this work of ART.\r\n");
    return;
  }

  reveal_hiding(g, ch, 0);
  act(g, "@c$n@w looks at $p, begins to work on forging a fake copy of it.@n", true, ch, obj2, nullptr, TO_ROOM);
  improve_skill(g, ch, SKILL_FORGERY, 1);
  if (char_get_skill(ch, SKILL_FORGERY) < axion_dice(g, 0)) {
    if (rand_number(g, 1, 10) >= 9) { /* Uh oh */
      send_to_char(g, ch, "In the middle of creating a forgery of %s you screw up. The fabrication unit built into the forgery kit melts and bonds with the original. You clumsy mistake with the Estex Titanium drill has broken both.\r\n", obj_get_short_description(obj2));
      extract_obj(g, obj4);
      extract_obj(g, obj2);
      return;
    }
    send_to_char(g, ch, "You start to make a forgery of %s but screw up and waste your forgery kit..\r\n", obj_get_short_description(obj2));
    act(g, "@c$n@w tried to duplicate $p but screws up somehow.@n", true, ch, obj2, nullptr, TO_ROOM);
    obj_from_char(g, obj4);
    extract_obj(g, obj4);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  obj_vnum loadn = obj_get_vnum(g, obj2);

  obj3 = read_object(g, loadn);
  add_unique_id(g, obj3);
  obj_to_char(g, obj3, ch);

  /* Set Object Variables */
  obj_extra_set(obj3, ITEM_FORGED, true);
  obj_set_weight(obj3, rand_number(g, obj_get_weight(obj3) / 2, obj_get_weight(obj3)));

  obj_from_char(g, obj4);
  extract_obj(g, obj4);
  send_to_char(g, ch, "You make an excellent forgery of %s@n!\r\n", obj_get_short_description(obj2));
  act(g, "@c$n@w makes a perfect forgery of $p.@n", true, ch, obj2, nullptr, TO_ROOM);
  char_set_wait(ch, PULSE_2SEC);
}

ACMD(do_appraise)
{
  int i, found;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];

  one_argument(argument, arg);

  if (char_is_npc(ch)) {
    return;
  }

  if (!know_skill(g, ch, SKILL_APPRAISE)) {
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "Okay, appraise what?\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
    send_to_char(g, ch, "You want to appraise what?\r\n");
    return;
  }

  reveal_hiding(g, ch, 0);
  act(g, "@c$n@w looks at $p, turning it over in $s hands.@n", true, ch, obj, nullptr, TO_ROOM);
  improve_skill(g, ch, SKILL_APPRAISE, 1);
  if (char_get_skill(ch, SKILL_APPRAISE) < axion_dice(g, -10)) {
    send_to_char(g, ch, "You fail to perceive the worth of %s..\r\n", obj_get_short_description(obj));
    act(g, "@c$n@w looks stumped about $p.@n", true, ch, obj, nullptr, TO_ROOM);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  if (obj_extra_flagged(obj, ITEM_BROKEN)) {
    send_to_char(g, ch, "%s is broken!\r\n", obj_get_short_description(obj));
    act(g, "@c$n@w looks at $p and frowns.@n", true, ch, obj, nullptr, TO_ROOM);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  if (obj_extra_flagged(obj, ITEM_FORGED)) {
    send_to_char(g, ch, "%s is fake and worthless!\r\n", obj_get_short_description(obj));
    act(g, "@c$n@w looks at $p with an angry face.@n", true, ch, obj, nullptr, TO_ROOM);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
  found         = false;
  int displevel = obj_get_level(obj);

  if (obj_get_type_flag(obj) == ITEM_WEAPON && obj_extra_flagged(obj, ITEM_CUSTOM))
    displevel = 20;

  send_to_char(g, ch, "%s is worth: %s\r\nMin Lvl: %d\r\n", obj_get_short_description(obj), add_commas(obj_get_cost(obj)), displevel);
  if (obj_get_type_flag(obj) == ITEM_WEAPON) {
    if (obj_extra_flagged(obj, ITEM_WEAPLVL1)) {
      send_to_char(g, ch, "Weapon Level: 1\nDamage Bonus: 5%s\r\n", "%");
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL2)) {
      send_to_char(g, ch, "Weapon Level: 2\nDamage Bonus: 10%s\r\n", "%");
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL3)) {
      send_to_char(g, ch, "Weapon Level: 3\nDamage Bonus: 20%s\r\n", "%");
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL4)) {
      send_to_char(g, ch, "Weapon Level: 4\nDamage Bonus: 30%s\r\n", "%");
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL5)) {
      send_to_char(g, ch, "Weapon Level: 5\nDamage Bonus: 50%s\r\n", "%");
    }
  }
  send_to_char(g, ch, "Size: %s\r\n", size_names[obj_get_size(obj)]);
  if (obj_extra_flagged(obj, ITEM_SLOT1) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(g, ch, "Token Slots  : @m0/1@n\n");
  } else if (obj_extra_flagged(obj, ITEM_SLOT1) && obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(g, ch, "Token Slots  : @m1/1@n\n");
  } else if (obj_extra_flagged(obj, ITEM_SLOT2) && !obj_extra_flagged(obj, ITEM_SLOT_ONE) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(g, ch, "Token Slots  : @m0/2@n\n");
  } else if (obj_extra_flagged(obj, ITEM_SLOT2) && obj_extra_flagged(obj, ITEM_SLOT_ONE) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(g, ch, "Token Slots  : @m1/2@n\n");
  } else if (obj_extra_flagged(obj, ITEM_SLOT2) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(g, ch, "Token Slots  : @m2/2@n\n");
  }
  send_to_char(g, ch, "Bonuses:");
  act(g, "@c$n@w looks at $p and nods, a satisfied look on $s face.@n", true, ch, obj, nullptr, TO_ROOM);
  int percent = false;
  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    if (obj_get_affected(obj)[i].modifier) {
      if (obj_get_affected(obj)[i].location == APPLY_REGEN || obj_get_affected(obj)[i].location == APPLY_TRAIN || obj_get_affected(obj)[i].location == APPLY_LIFEMAX) {
        percent = true;
      }
      sprinttype(obj_get_affected(obj)[i].location, apply_types, buf, sizeof(buf));
      send_to_char(g, ch, "%s %+d%s to %s", found++ ? "," : "", obj_get_affected(obj)[i].modifier, percent == true ? "%" : "", buf);
      percent = false;
      switch (obj_get_affected(obj)[i].location) {
        case APPLY_FEAT:
          send_to_char(g, ch, " (%s)", g->feat_list[obj_get_affected(obj)[i].specific].name);
          break;
        case APPLY_SKILL:
          send_to_char(g, ch, " (%s)", g->spell_info[obj_get_affected(obj)[i].specific].name);
          break;
      }
    }
  }
  if (!found)
    send_to_char(g, ch, " None");
  char buf2[MAX_STRING_LENGTH];
  sprintbitarray(obj_get_bitvector(obj), affected_bits, AF_ARRAY_MAX, buf2, sizeof(buf2));
  send_to_char(g, ch, "\nSpecial: %s\r\n", buf2);

  char_set_wait(ch, PULSE_2SEC);
}

ACMD(do_disguise)
{
  int skill = 0, roll = 0;

  if (char_is_npc(ch)) {
    send_to_char(g, ch, "You forgot your disguise off in mobland.\r\n");
    return;
  }

  if (char_plr_flagged(ch, PLR_DISGUISED)) {
    send_to_char(g, ch, "You stop disguising yourself.\r\n");
    char_plr_set(ch, PLR_DISGUISED, false);
    act(g, "@C$n @wpulls off $s disguise and reveals $mself!", true, ch, nullptr, nullptr, TO_ROOM);
    return;
  }

  if (!know_skill(g, ch, SKILL_DISGUISE)) {
    return;
  }

  if (!char_get_equipment(ch, WEAR_HEAD)) {
    send_to_char(g, ch, "You can't disguise your identity without anything on your head.\r\n");
    return;
  }

  if ((getCurST(g, ch)) < (getCurST(g, ch)) / 50) {
    send_to_char(g, ch, "You are too tired to try that right now.\r\n");
    return;
  }

  skill = char_get_skill(ch, SKILL_DISGUISE);
  roll  = axion_dice(g, -10);

  if (skill > roll) {
    send_to_char(g, ch, "You managed to disguise yourself with some skilled manipulation of your headwear.\r\n");
    act(g, "@C$n @wmanages to disguise $mself with some skilled manipulation of $s headwear.", true, ch, nullptr, nullptr, TO_ROOM);
    char_plr_set(ch, PLR_DISGUISED, true);
    return;
  }
  send_to_char(g, ch, "You finish attempting to disguise yourself, but realize you failed and need to try again.\r\n");
  act(g, "@C$n @wattempts and fails to disguise $mself properly and must try again.", true, ch, nullptr, nullptr, TO_ROOM);
  decCurST(g, ch, getMaxST(g, ch) / 50, 0);
  return;
}

ACMD(do_use)
{
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  struct obj_data *mag_item = nullptr;

  half_chop(argument, arg, buf);
  if (!*arg) {
    send_to_char(g, ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }

  if (!mag_item) {
    switch (subcmd) {
      case SCMD_RECITE:
      case SCMD_QUAFF:
        if (!(mag_item = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
          send_to_char(g, ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
          return;
        }
        break;
      case SCMD_USE:
        if (!(mag_item = get_obj_in_list_vis(g, ch, arg, nullptr, char_get_inventory(ch)))) {
          send_to_char(g, ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
          return;
        }
        break;
      default:
        mud_log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
        /*  SYSERR_DESC:
       *  This is the same as the unhandled case in do_gen_ps(), but in the
       *  function which handles 'quaff', 'recite', and 'use'.
       */
        return;
    }
  }
  int refreshed;
  switch (subcmd) {
    case SCMD_QUAFF:
      if (obj_get_type_flag(mag_item) != ITEM_POTION) {
        send_to_char(g, ch, "You can only swallow beans.\r\n");
        return;
      }
      if (char_is_android(ch)) {
        send_to_char(g, ch, "You can't swallow beans, you are an android.\r\n");
        return;
      }
      if (obj_extra_flagged(mag_item, ITEM_FORGED)) {
        send_to_char(g, ch, "You can't swallow that, it is fake!\r\n");
        return;
      }
      if (obj_extra_flagged(mag_item, ITEM_BROKEN)) {
        send_to_char(g, ch, "You can't swallow that, it is broken!\r\n");
        return;
      }
      break;
    case SCMD_RECITE:
      if (obj_get_type_flag(mag_item) != ITEM_SCROLL) {
        send_to_char(g, ch, "You can only recite scrolls.\r\n");
        return;
      }
      break;
    case SCMD_USE:
      if (char_is_android(ch)) {
        send_to_char(g, ch, "You are not biological enough to use these, Tincan.\r\n");
        return;
      }
      switch (obj_get_vnum(g, mag_item)) {
        case 381:
          if ((getCurST(g, ch)) >= getMaxST(g, ch)) {
            send_to_char(g, ch, "Your stamina is full.\r\n");
            return;
          }
          act(g, "@WYou place the $p@W against your chest and feel a rush of stamina as it automatically administers the dose.@n", true, ch, mag_item, nullptr, TO_CHAR);
          act(g, "@C$n@W places an $p@W against $s chest and a loud click is heard.@n", true, ch, mag_item, nullptr, TO_ROOM);
          if (char_get_skill(ch, SKILL_FIRST_AID) > 0) {
            send_to_char(g, ch, "@CYour skill in First Aid has helped increase the use of the injector. You gain more stamina as a result.@n\r\n");
            incCurST(g, ch, getMaxST(g, ch) * .25, true);
          } else {
            incCurST(g, ch, getMaxST(g, ch) * .1, true);
          }
          extract_obj(g, mag_item);
          return;
        case 382:
          if (char_aff_flagged(ch, AFF_BURNED)) {
            act(g, "@WYou gently apply the salve to your burns.@n", true, ch, mag_item, nullptr, TO_CHAR);
            act(g, "@C$n@W gently applies a burn salve to $s burns.@n", true, ch, mag_item, nullptr, TO_ROOM);
            char_aff_set(ch, AFF_BURNED, false);
            extract_obj(g, mag_item);
          } else {
            send_to_char(g, ch, "You are not burned.\r\n");
          }
          return;
        case 383:
          if (char_aff_flagged(ch, AFF_POISON)) {
            act(g, "@WYou place the $p@W against your neck and feel a rush of relief as the antitoxiin enters your bloodstream.@n", true, ch, mag_item, nullptr, TO_CHAR);
            act(g, "@C$n@W places an $p@W against $s neck and a loud click is heard.@n", true, ch, mag_item, nullptr, TO_ROOM);
            null_affect(g, ch, AFF_POISON);
            extract_obj(g, mag_item);
          } else {
            send_to_char(g, ch, "You are not poisoned.\r\n");
          }
          return;
        case 385:
          act(g, "@WYou drink the contents of the vial before disposing of it.@n", true, ch, mag_item, nullptr, TO_CHAR);
          act(g, "@C$n@W dinks a $p and then disposes of it.@n", true, ch, mag_item, nullptr, TO_ROOM);
          if (char_aff_flagged(ch, AFF_BLIND)) {
            act(g, "@WYour eyesight has returned!@n", true, ch, mag_item, nullptr, TO_CHAR);
            act(g, "@C$n@W eyesight seems to have returned.@n", true, ch, mag_item, nullptr, TO_ROOM);
            null_affect(g, ch, AFF_BLIND);
          }
          refreshed = false;

          if (getCurPL(g, ch) <= getEffMaxPL(g, ch) * 0.99) {
            incCurHealth(g, ch, large_rand(g, getEffMaxPL(g, ch) * 0.08, getEffMaxPL(g, ch) * 0.16), true);
            refreshed = true;
          } else if ((getCurKI(g, ch)) <= getEffMaxPL(g, ch) * 0.99) {
            incCurKI(g, ch, large_rand(g, getMaxKI(g, ch) * 0.08, getMaxKI(g, ch) * 0.16), true);
            refreshed = true;
          } else if ((getCurST(g, ch)) <= getMaxST(g, ch) * 0.99) {
            incCurST(g, ch, large_rand(g, getMaxST(g, ch) * 0.08, getMaxST(g, ch) * 0.16), true);
            refreshed = true;
          }
          if (refreshed == true) {
            send_to_char(g, ch, "@CYou feel refreshed!\r\n");
          }
          extract_obj(g, mag_item);
          return;
        default:
          send_to_char(g, ch, "That is not something you can apparently use.\r\n");
          return;
      }
      break;
  }

  mag_objectmagic(g, ch, mag_item, buf);
}

ACMD(do_value)
{
  char arg[MAX_INPUT_LENGTH];
  int value_lev;

  one_argument(argument, arg);

  if (!*arg) {
    switch (subcmd) {
      case SCMD_WIMPY:
        if (char_get_wimp_level(g, ch)) {
          send_to_char(g, ch, "Your current wimp level is %d powerlevel.\r\n", char_get_wimp_level(g, ch));
          return;
        }
        send_to_char(g, ch, "At the moment, you're not a wimp.  (sure, sure...)\r\n");
        return;
        break;
    }
  }

  if (isdigit(*arg)) {
    switch (subcmd) {
      case SCMD_WIMPY:
        /* 'wimp_level' is a player_special. -gg 2/25/98 */
        if (char_is_npc(ch))
          return;
        if ((value_lev = atoi(arg)) != 0) {
          if (value_lev < 0)
            send_to_char(g, ch, "Heh, heh, heh.. we are jolly funny today, eh?\r\n");
          else if (value_lev > getEffMaxPL(g, ch))
            send_to_char(g, ch, "That doesn't make much sense, now does it?\r\n");
          else if (value_lev > (getEffMaxPL(g, ch) * 0.5))
            send_to_char(g, ch, "You can't set your wimp level above half your powerlevel.\r\n");
          else {
            send_to_char(g, ch, "Okay, you'll wimp out if you drop below %d powerlevel.\r\n", value_lev);
            char_set_wimp_level(g, ch, value_lev);
          }
        } else {
          send_to_char(g, ch, "Okay, you'll now tough out fights to the bitter end.\r\n");
          char_set_wimp_level(g, ch, 0);
        }
        break;
      default:
        mud_log("Unknown subcmd to do_value %d called by %s", subcmd, char_get_name(ch));
        break;
    }
  } else
    send_to_char(g, ch, "Specify a value.  (0 to disable)\r\n");
}

ACMD(do_compare)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj1, *obj2;
  struct char_data *tchar;
  int value1 = 0, value2 = 0, o1, o2;
  const char *msg = nullptr;

  two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2) {
    send_to_char(g, ch, "Compare what to what?\n\r");
    return;
  }

  o1 = generic_find(g, arg1, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &tchar, &obj1);
  o2 = generic_find(g, arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &tchar, &obj2);

  if (!o1 || !o2) {
    send_to_char(g, ch, "You do not have that item.\r\n");
    return;
  }
  if (obj1 == obj2) {
    msg = "You compare $p to itself.  It looks about the same.";
  } else if (obj_get_type_flag(obj1) != obj_get_type_flag(obj2)) {
    msg = "You can't compare $p and $P.";
  } else {
    switch (obj_get_type_flag(obj1)) {
      default:
        msg = "You can't compare $p and $P.";
        break;
      case ITEM_ARMOR:
        value1 = obj_get_value(obj1, VAL_ARMOR_APPLYAC);
        value2 = obj_get_value(obj2, VAL_ARMOR_APPLYAC);
        break;
      case ITEM_WEAPON:
        value1 = (1 + obj_get_value(obj1, VAL_WEAPON_DAMSIZE)) * obj_get_value(obj1, VAL_WEAPON_DAMDICE);
        value2 = (1 + obj_get_value(obj2, VAL_WEAPON_DAMSIZE)) * obj_get_value(obj2, VAL_WEAPON_DAMDICE);
        break;
    }
  }

  if (msg == nullptr) {
    if (value1 == value2)
      msg = "$p and $P look about the same.";
    else if (value1 > value2)
      msg = "$p looks better than $P.";
    else
      msg = "$p looks worse than $P.";
  }

  act(g, msg, false, ch, obj1, obj2, TO_CHAR);
  return;
}

ACMD(do_break)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct char_data *dummy = nullptr;
  int cmbrk;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(g, ch, "Usually you break SOMETHING.\r\n");
    return;
  }

  if (!(cmbrk = generic_find(g, arg, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    send_to_char(g, ch, "Can't seem to find what you want to break!\r\n");
    return;
  }

  if (obj_extra_flagged(obj, ITEM_BROKEN)) {
    send_to_char(g, ch, "Seems like it's already broken!\r\n");
    return;
  }

  /* Ok, break it! */
  send_to_char(g, ch, "You ruin %s.\r\n", obj_get_short_description(obj));
  act(g, "$n ruins $p.", false, ch, obj, nullptr, TO_ROOM);
  obj_set_value(obj, VAL_ALL_HEALTH, 0);
  obj_extra_toggle(obj, ITEM_BROKEN);

  return;
}

ACMD(do_fix)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj, *obj4 = nullptr;
  struct char_data *dummy = nullptr;
  int cmbrk, found = false, self = false, custom = false;

  one_argument(argument, arg);

  if (!know_skill(g, ch, SKILL_REPAIR)) {
    return;
  }

  if (!*arg) {
    send_to_char(g, ch, "Usually you fix SOMETHING.\r\n");
    return;
  }

  if (!strcasecmp("self", arg)) {
    if (!char_is_android(ch)) {
      send_to_char(g, ch, "Only androids can fix their bodies with repair kits.\r\n");
      return;
    }
    self = true;
  }

  if (self == false) {
    if (!(cmbrk = generic_find(g, arg, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &dummy, &obj))) {
      send_to_char(g, ch, "Can't seem to find what you want to fix!\r\n");
      return;
    }

    if ((cmbrk) && obj_get_value(obj, VAL_ALL_HEALTH) >= 100) {
      send_to_char(g, ch, "But it isn't even damaged!\r\n");
      return;
    }

    if (obj_extra_flagged(obj, ITEM_FORGED)) {
      send_to_char(g, ch, "That is fake, why bother fixing it?\r\n");
      return;
    }

    switch (obj_get_value(obj, VAL_ALL_MATERIAL)) {
      case MATERIAL_ORGANIC:
      case MATERIAL_FOOD:
      case MATERIAL_PAPER:
      case MATERIAL_LIQUID:
        send_to_char(g, ch, "You can't repair that.\r\n");
        return;
        break;
    }

    if (obj_get_vnum(g, obj) == 20099 || obj_get_vnum(g, obj) == 20098) {
      custom = true;
    }
  }

  for (obj_data *rep : char_inventory(g, ch)) {
    if (custom == false) {
      if (found == false && obj_get_vnum(g, rep) == 48 && (!obj_extra_flagged(rep, ITEM_BROKEN) && !obj_extra_flagged(rep, ITEM_FORGED))) {
        found = true;
        obj4  = rep;
      }
    } else {
      if (found == false && obj_get_vnum(g, rep) == 13593 && (!obj_extra_flagged(rep, ITEM_BROKEN) && !obj_extra_flagged(rep, ITEM_FORGED))) {
        found = true;
        obj4  = rep;
      }
    }
  }

  if (found == false && custom == false) {
    send_to_char(g, ch, "You do not even have a repair kit.\r\n");
    return;
  }
  if (found == false && custom == true) {
    send_to_char(g, ch, "You do not even have a Nano-tech Repair Orb.\r\n");
    return;
  }

  if (self == false) {
    if (char_get_skill(ch, SKILL_REPAIR) < axion_dice(g, 0)) {
      act(g, "You try to repair $p but screw up..", true, ch, obj, nullptr, TO_CHAR);
      act(g, "$n tries to repair $p but screws up..", true, ch, obj, nullptr, TO_ROOM);
      extract_obj(g, obj4);
      improve_skill(g, ch, SKILL_REPAIR, 1);
      char_set_wait(ch, PULSE_2SEC);
      return;
    }

    if (obj_get_value(obj, VAL_ALL_HEALTH) + char_get_skill(ch, SKILL_REPAIR) < 100) {
      send_to_char(g, ch, "You repair %s a bit.\r\n", obj_get_short_description(obj));
      act(g, "$n repairs $p a bit.", false, ch, obj, nullptr, TO_ROOM);
      obj_set_value(obj, VAL_ALL_HEALTH, obj_get_value(obj, VAL_ALL_HEALTH) + (char_get_skill(ch, SKILL_REPAIR)));
      obj_extra_set(obj, ITEM_BROKEN, false);
    } else {
      send_to_char(g, ch, "You repair %s completely.\r\n", obj_get_short_description(obj));
      act(g, "$n repairs $p completely.", false, ch, obj, nullptr, TO_ROOM);
      obj_set_value(obj, VAL_ALL_HEALTH, 100);
      obj_extra_set(obj, ITEM_BROKEN, false);
    }
    if (obj_get_carried_by(obj) == nullptr && !char_plr_flagged(ch, PLR_REPLEARN) && (level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0 || char_get_level(ch) >= 100)) {
      int64_t gain = (level_exp(ch, char_get_level(ch) + 1) * 0.0003) * char_get_skill(ch, SKILL_REPAIR);
      send_to_char(g, ch, "@mYou've learned a bit from repairing it. @D[@gEXP@W: @G+%s@D]@n\r\n", add_commas(gain));
      char_plr_set(ch, PLR_REPLEARN, true);
      gain_exp(g, ch, gain);
    } else if (rand_number(g, 2, 12) >= 10 && char_plr_flagged(ch, PLR_REPLEARN)) {
      char_plr_set(ch, PLR_REPLEARN, false);
      send_to_char(g, ch, "@mYou think you might be on to something...@n\r\n");
    }
    improve_skill(g, ch, SKILL_REPAIR, 1);
    extract_obj(g, obj4);
    char_set_wait(ch, PULSE_2SEC);
    return;
  } else { /* For androids repairing themselves */

    if (getCurPL(g, ch) >= getEffMaxPL(g, ch)) {
      send_to_char(g, ch, "Your body is already in peak condition.\r\n");
      return;
    }
    if (char_get_skill(ch, SKILL_REPAIR) < axion_dice(g, 0)) {
      act(g, "You try to repair your body but screw up..", true, ch, nullptr, nullptr, TO_CHAR);
      act(g, "$n tries to repair $s body but screws up..", true, ch, nullptr, nullptr, TO_ROOM);
      extract_obj(g, obj4);
      improve_skill(g, ch, SKILL_REPAIR, 1);
      char_set_wait(ch, PULSE_5SEC);
      return;
    }
    act(g, "You use the repair kit to fix part of your body...", true, ch, nullptr, nullptr, TO_CHAR);
    act(g, "$n works on their body with a repair kit.", true, ch, nullptr, nullptr, TO_ROOM);
    int64_t mult = char_get_skill(ch, SKILL_REPAIR);
    int64_t add  = (getEffMaxPL(g, ch) * 0.005 + 10) * mult;

    extract_obj(g, obj4);
    if (incCurHealth(g, ch, add, true) == getEffMaxPL(g, ch)) {
      send_to_char(g, ch, "Your body has been totally repaired.\r\n");
      char_set_wait(ch, PULSE_5SEC);
    } else {
      send_to_char(g, ch, "Your body still needs some work done to it.\r\n");
      char_set_wait(ch, PULSE_5SEC);
    }
  }
}
