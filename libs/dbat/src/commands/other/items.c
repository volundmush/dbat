/* items.c: item crafting, repair and use
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
#include "dbat/local_limits.h"
#include "other_internal.h"

static void boost_obj(struct obj_data *obj, struct char_data *ch, int type);

/* This handles increasing the stats of a created object based on the skill *
 * and/or stats of the user.                                                */
static void boost_obj(struct obj_data *obj, struct char_data *ch, int type)
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
        obj->affected[0].location = 17;
        obj->affected[0].modifier += (boost * char_get_level(ch));
        if (obj_get_vnum(obj) == 91) {
          obj->affected[1].location = 1;
          obj->affected[1].modifier = (boost / 20);
        } else {
          obj->affected[1].location = 3;
          obj->affected[1].modifier = (boost / 20);
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
        obj->affected[0].location = 1;
        obj->affected[0].modifier = (boost / 20);
      }
      break;
  } /* End of main switch */
}

ACMD(do_form)
{
  int skill = 0, senzu = FALSE, bag = FALSE, light = FALSE, sword = FALSE, mattress = FALSE, gi = FALSE, pants = FALSE, kachin = FALSE, boost = FALSE, shuriken = FALSE;
  int clothes = FALSE, wrist = FALSE, boots = FALSE, level = 0;
  double discount = 1.0;
  int64_t cost    = 0;
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH], clam[MAX_INPUT_LENGTH];

  half_chop(argument, arg, clam);

  half_chop(clam, arg2, arg3);

  if (!know_skill(ch, SKILL_CREATE)) {
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
    boost = TRUE;
  }
  if (skill >= 90) {
    kachin = TRUE;
  }
  if (skill >= 80) {
    senzu = TRUE;
  }
  if (skill >= 70) {
    shuriken = TRUE;
  }
  if (skill >= 60) {
    clothes = TRUE;
  }
  if (skill >= 50) {
    sword = TRUE;
    gi    = TRUE;
    pants = TRUE;
    wrist = TRUE;
    boots = TRUE;
  }
  if (skill >= 40) {
    mattress = TRUE;
  }
  if (skill >= 30) {
    bag = TRUE;
  }
  if (skill >= 20) {
    light = TRUE;
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
    send_to_char(ch, "What do you want to create?\r\n"
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
  reveal_hiding(ch, 0);
  if (!(strcmp(arg, "food"))) {
    cost = getMaxKI(ch) / (skill / 2);
    cost *= discount;

    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    } else {
      if (!*arg2) {
        send_to_char(ch, "Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | highest)\r\n");
        send_to_char(ch, "If you are capable you will make it. If not you will make a low quality version.\r\n");
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
        obj = read_object(1512, VIRTUAL);
        add_unique_id(obj);
      } else if (level == 3) {
        obj = read_object(1511, VIRTUAL);
        add_unique_id(obj);
      } else if (level == 2) {
        obj = read_object(1510, VIRTUAL);
        add_unique_id(obj);
      } else {
        obj = read_object(70, VIRTUAL);
        add_unique_id(obj);
      }
      obj_to_char(obj, ch);
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      return;
    }
  } else if (!(strcmp(arg, "water"))) {
    cost = getMaxKI(ch) / (skill * 2);
    cost *= discount;

    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    } else {
      if (!*arg2) {
        send_to_char(ch, "Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | highest)\r\n");
        send_to_char(ch, "If you are capable you will make it. If not you will make a low quality version.\r\n");
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
        obj = read_object(1515, VIRTUAL);
        add_unique_id(obj);
      } else if (level == 3) {
        obj = read_object(1514, VIRTUAL);
        add_unique_id(obj);
      } else if (level == 2) {
        obj = read_object(1513, VIRTUAL);
        add_unique_id(obj);
      } else {
        obj = read_object(71, VIRTUAL);
        add_unique_id(obj);
      }
      obj_to_char(obj, ch);
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      return;
    }
  } else if (!(strcmp(arg, "bag"))) {
    cost = getMaxKI(ch) / (skill * 2);
    cost *= discount;

    if (bag == FALSE) {
      send_to_char(ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    } else {
      obj = read_object(319, VIRTUAL);
      add_unique_id(obj);
      obj_to_char(obj, ch);
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      return;
    }
  } else if (!(strcmp(arg, "mattress"))) {
    cost = getMaxKI(ch) / skill;
    cost *= discount;

    if (mattress == FALSE) {
      send_to_char(ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    } else {
      obj = read_object(16, VIRTUAL);
      add_unique_id(obj);
      obj_to_char(obj, ch); // cooldown removed on 10/24/2021
      reveal_hiding(ch, 0); //GET_COOLDOWN(ch) = 10;
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      return;
    }
  } else if (!(strcmp(arg, "weapon"))) {
    cost = getMaxKI(ch) / 5;
    cost *= discount;

    if (sword == FALSE) {
      send_to_char(ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    } else {
      if (!*arg2) {
        send_to_char(ch, "What type of weapon?\r\nSyntax: create weapon (sword | club | spear | dagger | gun)\r\n");
        return;
      }
      if (!*arg3) {
        send_to_char(ch, "Making lowest quality version of object. To make a higher quality use, Syntax: create (type) (mid | high | higher | highest)\r\n");
        send_to_char(ch, "If you are capable you will make it. If not you will make a low quality version.\r\n");
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
          obj = read_object(1519, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 4) {
          obj = read_object(1518, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 3) {
          obj = read_object(1517, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 2) {
          obj = read_object(1516, VIRTUAL);
          add_unique_id(obj);
        } else {
          obj = read_object(90, VIRTUAL);
          add_unique_id(obj);
        }
      } else if (!strcasecmp(arg2, "dagger")) {
        if (level == 5) {
          obj = read_object(1540, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 4) {
          obj = read_object(1539, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 3) {
          obj = read_object(1538, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 2) {
          obj = read_object(1537, VIRTUAL);
          add_unique_id(obj);
        } else {
          obj = read_object(1536, VIRTUAL);
          add_unique_id(obj);
        }
      } else if (!strcasecmp(arg2, "club")) {
        if (level == 5) {
          obj = read_object(1545, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 4) {
          obj = read_object(1544, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 3) {
          obj = read_object(1543, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 2) {
          obj = read_object(1542, VIRTUAL);
          add_unique_id(obj);
        } else {
          obj = read_object(1541, VIRTUAL);
          add_unique_id(obj);
        }
      } else if (!strcasecmp(arg2, "spear")) {
        if (level == 5) {
          obj = read_object(1550, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 4) {
          obj = read_object(1549, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 3) {
          obj = read_object(1548, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 2) {
          obj = read_object(1547, VIRTUAL);
          add_unique_id(obj);
        } else {
          obj = read_object(1546, VIRTUAL);
          add_unique_id(obj);
        }
      } else if (!strcasecmp(arg2, "gun")) {
        if (level == 5) {
          obj = read_object(1555, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 4) {
          obj = read_object(1554, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 3) {
          obj = read_object(1553, VIRTUAL);
          add_unique_id(obj);
        } else if (level == 2) {
          obj = read_object(1552, VIRTUAL);
          add_unique_id(obj);
        } else {
          obj = read_object(1551, VIRTUAL);
          add_unique_id(obj);
        }
      } else {
        send_to_char(ch, "What type of weapon?\r\nSyntax: create weapon (sword | club | spear | dagger | gun)\r\n");
        return;
      }
      obj_to_char(obj, ch);
      obj_set_size(obj, get_size(ch));
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      return;
    }
  } else if (!(strcmp(arg, "clothesbeam"))) {
    cost = getMaxKI(ch) / 2;
    cost *= discount;

    if (clothes == FALSE) {
      send_to_char(ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    if (!*arg2) {
      send_to_char(ch, "Who do you want to hit with clothesbeam?\r\nSyntax: create clothesbeam (target)\r\n");
      return;
    }

    struct char_data *vict = NULL;

    if (!(vict = get_char_vis(ch, arg2, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Clothesbeam who?\r\nSyntax: create clothesbeam (target)\r\n");
      return;
    }

    if (vict->master != ch) {
      send_to_char(ch, "They must be following you first.\r\n");
      return;
    } else {
      obj = read_object(92, VIRTUAL); /* gi */
      add_unique_id(obj);
      boost_obj(obj, ch, 0);
      obj_to_char(obj, vict);
      obj_set_size(obj, get_size(vict));
      obj = read_object(91, VIRTUAL); /* pants */
      add_unique_id(obj);
      boost_obj(obj, ch, 0);
      obj_to_char(obj, vict);
      obj_set_size(obj, get_size(vict));
      obj = read_object(1528, VIRTUAL); /* wrist */
      add_unique_id(obj);
      boost_obj(obj, ch, 0);
      obj_to_char(obj, vict);
      obj_set_size(obj, get_size(vict));
      obj = read_object(1528, VIRTUAL); /* wrist */
      add_unique_id(obj);
      boost_obj(obj, ch, 0);
      obj_to_char(obj, vict);
      obj_set_size(obj, get_size(vict));
      obj = read_object(1532, VIRTUAL); /* boots */
      add_unique_id(obj);
      boost_obj(obj, ch, 0);
      obj_to_char(obj, vict);
      obj_set_size(obj, get_size(vict));
      do_wear(vict, (char[]){"all"}, 0, 0);
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      return;
    }
  } else if (!(strcmp(arg, "gi"))) {
    cost = getMaxKI(ch) / 5;
    cost *= discount;
    if (gi == FALSE) {
      send_to_char(ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    } else {
      obj = read_object(92, VIRTUAL);
      add_unique_id(obj);
      boost_obj(obj, ch, 0);
      obj_to_char(obj, ch);
      obj_set_size(obj, get_size(ch));
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      return;
    }
  } else if (!(strcmp(arg, "shuriken"))) {
    cost = getMaxKI(ch) / 4;
    cost *= discount;

    if (shuriken == FALSE) {
      send_to_char(ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    } else {
      obj = read_object(19053, VIRTUAL);
      add_unique_id(obj);
      obj_to_char(obj, ch);
      obj_extra_set(obj, ITEM_NORENT, true);
      obj_extra_set(obj, ITEM_NOSELL, true);
      obj_set_size(obj, get_size(ch));
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      return;
    }
  } else if (!(strcmp(arg, "pants"))) {
    cost = getMaxKI(ch) / 5;
    cost *= discount;

    if (pants == FALSE) {
      send_to_char(ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    } else {
      obj = read_object(91, VIRTUAL);
      add_unique_id(obj);
      boost_obj(obj, ch, 0);
      obj_to_char(obj, ch);
      obj_set_size(obj, get_size(ch));
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      return;
    }
  } else if (!(strcmp(arg, "wristband"))) {
    cost = getMaxKI(ch) / 5;
    cost *= discount;

    if (wrist == FALSE) {
      send_to_char(ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    } else {
      obj = read_object(1528, VIRTUAL);
      add_unique_id(obj);
      boost_obj(obj, ch, 0);
      obj_to_char(obj, ch);
      obj_set_size(obj, get_size(ch));
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      return;
    }
  } else if (!(strcmp(arg, "boots"))) {
    cost = getMaxKI(ch) / 5;
    cost *= discount;

    if (boots == FALSE) {
      send_to_char(ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    } else {
      obj = read_object(1532, VIRTUAL);
      add_unique_id(obj);
      boost_obj(obj, ch, 0);
      obj_to_char(obj, ch);
      obj_set_size(obj, get_size(ch));
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      return;
    }
  } else if (!(strcmp(arg, "light"))) {
    cost = getMaxKI(ch) / (skill * 2);
    cost *= discount;

    if (light == FALSE) {
      send_to_char(ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    } else {
      obj = read_object(72, VIRTUAL);
      add_unique_id(obj);
      obj_to_char(obj, ch);
      obj_set_size(obj, get_size(ch));
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      return;
    }
  } else if (!(strcmp(arg, "kachin"))) {
    cost = getMaxKI(ch) - 1;
    cost *= discount;

    if (kachin == FALSE) {
      send_to_char(ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    } else {
      obj = read_object(87, VIRTUAL);
      add_unique_id(obj);
      obj_to_room(obj, char_get_in_room(ch));
      obj_set_size(obj, get_size(ch));
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      return;
    }
  } else if (!(strcmp(arg, "elixir"))) {
    cost = getMaxKI(ch) - 1;
    cost *= discount;

    if (boost == FALSE) {
      send_to_char(ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s\r\n", arg);
      return;
    }
    if (getCurPL(ch) < getEffMaxPL(ch)) {
      send_to_char(ch, "You need to be at full powerlevel to create %s\r\n", arg);
      return;
    } else if (char_get_practice_points(ch) < 10) {
      send_to_char(ch, "You do not have enough PS to create %s, you need at least 10.\r\n", arg);
      return;
    } else {
      obj = read_object(86, VIRTUAL);
      add_unique_id(obj);
      obj_to_room(obj, char_get_in_room(ch));
      obj_set_size(obj, get_size(ch));
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      decCurHealthPercent(ch, 1, 1);
      char_set_practice_points(ch, char_get_practice_points(ch) - (10));
      return;
    }
  } else if (!(strcmp(arg, "senzu"))) {
    cost          = getMaxKI(ch);
    int64_t cost2 = getEffMaxPL(ch) - 1;

    if (senzu == FALSE) {
      send_to_char(ch, "What do you want to create?\r\n");
      return;
    }
    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to create %s, you need full ki.\r\n", arg);
      return;
    } else if (getCurPL(ch) <= cost2) {
      send_to_char(ch, "You do not have enough powerlevel to create %s, you need to be at full.\r\n", arg);
      return;
    } else if ((getCurST(ch)) < getMaxST(ch)) {
      send_to_char(ch, "You do not have enough stamina to create %s, you need to be at full.\r\n", arg);
      return;
    } else if (char_get_practice_points(ch) < 50) {
      send_to_char(ch, "You do not have enough PS to create %s, you need at least 50.\r\n", arg);
      return;
    } else {
      obj = read_object(1, VIRTUAL);
      add_unique_id(obj);
      obj_to_char(obj, ch);
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You hold out your hand and create $p out of your ki!", TRUE, ch, obj, 0, TO_CHAR);
      act("$n holds out $s hand and creates $p out of thin air!", TRUE, ch, obj, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      decCurHealth(ch, cost2, 0);
      decCurSTPercent(ch, 1, 1);
      char_set_practice_points(ch, char_get_practice_points(ch) - (50));
      return;
    }
  } else {
    send_to_char(ch, "Create what?\r\n");
    return;
  }
}

ACMD(do_recharge)
{

  if (char_is_npc(ch) || !char_is_android(ch)) {
    send_to_char(ch, "Only androids can use recharge\r\n");
    return;
  }

  if (char_get_con_cooldown(ch) > 0) {
    send_to_char(ch, "You must wait a short period before your nanites can convert your ki.\r\n");
    return;
  }

  if (!char_plr_flagged(ch, PLR_REPAIR)) {
    send_to_char(ch, "You are not a repair model android.\r\n");
    return;
  } else {
    int64_t cost = 0;

    cost = getMaxST(ch) / 20;

    if ((getCurKI(ch)) < cost) {
      send_to_char(ch, "You do not have enough ki to recharge your stamina.\r\n");
      return;
    } else if ((getCurST(ch)) >= getMaxST(ch)) {
      send_to_char(ch, "Your energy reserves are already full.\r\n");
      return;
    } else {
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You focus your ki into your energy reserves, recharging them some.", TRUE, ch, 0, 0, TO_CHAR);
      act("$n stops and glows green briefly.", TRUE, ch, 0, 0, TO_ROOM);
      decCurKI(ch, cost, 0);
      if ((getCurST(ch)) + (cost * 2) < getMaxST(ch)) {
        incCurST(ch, cost * 2, true);
      } else {
        restoreST(ch, false);
        send_to_char(ch, "You are fully recharged now.\r\n");
      }
      char_set_wait(ch, PULSE_2SEC);
    }
  }
}

ACMD(do_srepair)
{
  int i;

  if (!char_is_android(ch)) {
    send_to_char(ch, "Only androids can use repair, maybe you want 'fix' instead?\r\n");
    return;
  }

  if (char_get_con_cooldown(ch) > 0) {
    send_to_char(ch, "You must wait a short period before your nanites can repair you.\r\n");
    return;
  }

  if (!char_is_npc(ch) && !char_plr_flagged(ch, PLR_REPAIR)) {
    send_to_char(ch, "You are not a repair model android.\r\n");
    return;
  } else {
    int64_t cost = 0, heal = 0;

    cost = getEffMaxPL(ch) / 40;

    if ((getCurST(ch)) < cost) {
      send_to_char(ch, "You do not have enough stamina to repair yourself.\r\n");
      return;
    } else if (getCurPL(ch) >= getEffMaxPL(ch)) {
      send_to_char(ch, "You are already at full functionality and do not require repairs.\r\n");
      return;
    } else {
      reveal_hiding(ch, 0);
      char_set_con_cooldown(ch, 10);
      act("You repair some of your outer casings and internal systems, with the small nano-robots contained in your body.", TRUE, ch, 0, 0, TO_CHAR);
      act("$n stops a moment as small glowing particles move across $s body.", TRUE, ch, 0, 0, TO_ROOM);

      int repaired = FALSE;
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
              repaired = TRUE;
            }
          }
        }
      }

      if (repaired == TRUE) {
        send_to_char(ch, "@GYour nano-robots also repair all of your equipment a little bit.@n\r\n");
      }
      decCurST(ch, cost, 0);
      heal = cost * 2;
      if (char_get_bonuses(ch, BONUS_HEALER) > 0) {
        heal += heal * .25;
      }

      if (incCurHealth(ch, heal, true) == getEffMaxPL(ch)) {
        send_to_char(ch, "You are fully repaired now.\r\n");
      }

      if (!char_is_npc(ch) && rand_number(1, 3) == 2 && (getCurKI(ch)) < getMaxKI(ch)) {
        send_to_char(ch, "@GThe repairs have managed to relink power reserves and boost your current energy level.@n\r\n");
        incCurKI(ch, cost, true);
      }
      char_set_wait(ch, PULSE_2SEC);
    }
  }
}

ACMD(do_upgrade)
{
  int count = 0, bonus = 0, cost = 0;
  char arg[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  two_arguments(argument, arg, arg2);

  if (char_is_npc(ch) || !char_is_android(ch)) {
    send_to_char(ch, "You are not an android!\r\n");
    return;
  }

  if (!*arg) {
    if (!char_plr_flagged(ch, PLR_ABSORB)) {
      send_to_char(ch, "@c--------@D[@rUpgrade Menu@D]@c--------\r\n"
                       "@cUpgrade @RPowerlevel@D: @Y75 @WPoints\r\n"
                       "@cUpgrade @CKi        @D: @Y40 @WPoints\r\n"
                       "@cUpgrade @GStamina   @D: @Y50 @WPoints\r\n"
                       "@D            -----------\r\n");
    }
    send_to_char(ch, "@cAugment @RPowerlevel\r\n"
                     "@cAugment @CKi\r\n"
                     "@cAugment @GStamina\r\n"
                     "@WCurrent Upgrade Points @D[@y%s@D]@n\r\n",
                 add_commas(char_get_upgrade(ch)));
    return;
  }

  if (!strcasecmp("augment", arg)) {
    struct obj_data *obj = NULL;
    int64_t gain         = 0;
    if (char_get_level(ch) < 80) {
      send_to_char(ch, "You need to be at least level 80 to use these kits.\r\n");
      return;
    }
    if (!(obj = get_obj_in_list_vis(ch, (char[]){"Augmentation"}, NULL, ch->carrying))) {
      send_to_char(ch, "You don't have a Circuit Augmentation Kit.\r\n");
      return;
    } else {
      switch (char_get_level(ch)) { /* R: Original was GET_LEVEL(ch) */
        case 80:
          gain = getEffMaxPL(ch) * 0.005;
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
          gain = getEffMaxPL(ch) * 0.005; /* R: Original was GET_LEVEL(ch) * 2000; */
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
          gain = getEffMaxPL(ch) * 0.005; /* R: Original was GET_LEVEL(ch) * 4000; */
          break;
        case 100:
          gain = getEffMaxPL(ch) * 0.005; /* R: Original was GET_LEVEL(ch) * 100000 */
          if (gain > 10000000) {
            gain = 10000000;
          }
          break;
      }
      if (!strcasecmp("powerlevel", arg2)) {
        obj_from_char(obj);
        extract_obj(obj);
        act("@WYou install the circuits and upgrade your maximum powerlevel.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W installs some circuits and upgrades $s systems.@n", TRUE, ch, 0, 0, TO_ROOM);
        gainBasePL(ch, gain, true);
        send_to_char(ch, "@gGain @D[@G+%s@D]\r\n", add_commas(gain));
        return;
      } else if (!strcasecmp("ki", arg2)) {
        obj_from_char(obj);
        extract_obj(obj);
        act("@WYou install the circuits and upgrade your maximum ki.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W installs some circuits and upgrades $s systems.@n", TRUE, ch, 0, 0, TO_ROOM);
        gainBaseKI(ch, gain, true);
        send_to_char(ch, "@gGain @D[@G+%s@D]\r\n", add_commas(gain));
        return;
      } else if (!strcasecmp("stamina", arg2)) {
        obj_from_char(obj);
        extract_obj(obj);
        act("@WYou install the circuits and upgrade your maximum stamina.@n", TRUE, ch, 0, 0, TO_CHAR);
        act("@C$n@W installs some circuits and upgrades $s systems.@n", TRUE, ch, 0, 0, TO_ROOM);
        gainBaseST(ch, gain, true);
        send_to_char(ch, "@gGain @D[@G+%s@D]\r\n", add_commas(gain));
        return;
      } else {
        send_to_char(ch, "What do you want to augment? Powerlevel, ki, or stamina?\r\n");
        return;
      }
    }
  }

  if (char_plr_flagged(ch, PLR_ABSORB)) {
    send_to_char(ch, "You are an absorb model and can only upgrade with augmentation kits.\r\n");
    return;
  }

  if (is_soft_cap(ch, 0, 1.0)) {
    send_to_char(ch, "@mYou are unable to spend anymore UGP right now (Softcap)@n\r\n");
    return;
  }

  if (!*arg2 && (!strcasecmp("powerlevel", arg) || !strcasecmp("ki", arg) || !strcasecmp("stamina", arg))) {
    send_to_char(ch, "How many times do you want to increase %s?", arg);
    return;
  }

  if (atoi(arg2) <= 0 && (!strcasecmp("powerlevel", arg) || !strcasecmp("ki", arg) || !strcasecmp("stamina", arg))) {
    send_to_char(ch, "It needs to be between 1-1000\r\n");
    return;
  }

  if (atoi(arg2) > 1000 && (!strcasecmp("powerlevel", arg) || !strcasecmp("ki", arg) || !strcasecmp("stamina", arg))) {
    send_to_char(ch, "It needs to be between 1-1000\r\n");
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
      send_to_char(ch, "You need %s upgrade points, and only have %s.\r\n", add_commas(cost), add_commas(char_get_upgrade(ch)));
      return;
    } else if (is_soft_cap(ch, bonus, 1.0)) {
      send_to_char(ch, "@mYou can't spend that much UGP on it as it will go over your softcap.@n\r\n");
      return;
    } else {
      char_set_upgrade(ch, char_get_upgrade(ch) - (cost));
      send_to_char(ch, "You upgrade your system and gain %s %s!", add_commas(bonus), arg);
      gainBasePL(ch, bonus, true);
    }
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
      send_to_char(ch, "You need %s upgrade points, and only have %s.\r\n", add_commas(cost), add_commas(char_get_upgrade(ch)));
      return;
    } else if (is_soft_cap(ch, bonus, 1.0)) {
      send_to_char(ch, "@mYou can't spend that much UGP on it as it will go over your softcap.@n\r\n");
      return;
    } else {
      char_set_upgrade(ch, char_get_upgrade(ch) - (cost));
      send_to_char(ch, "You upgrade your system and gain %s %s!", add_commas(bonus), arg);
      gainBaseKI(ch, bonus, true);
    }
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
      send_to_char(ch, "You need %s upgrade points, and only have %s.\r\n", add_commas(cost), add_commas(char_get_upgrade(ch)));
      return;
    } else if (is_soft_cap(ch, bonus, 1.0)) {
      send_to_char(ch, "@mYou can't spend that much UGP on it as it will go over your softcap.@n\r\n");
      return;
    } else {
      char_set_upgrade(ch, char_get_upgrade(ch) - (cost));
      send_to_char(ch, "You upgrade your system and gain %s %s!", add_commas(bonus), arg);
      gainBaseST(ch, bonus, true);
    }
  } else {
    send_to_char(ch, "That is not a valid upgrade option.\r\n");
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
      send_to_char(ch, "Who do you want to ingest?\r\n");
      return;
    }

    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "Ingest who?\r\n");
      return;
    }
    if (!can_kill(ch, vict, NULL, 0)) {
      return;
    }
    if (char_get_absorbby(vict)) {
      send_to_char(ch, "%s is already absorbing from them!", char_get_name(char_get_absorbby(vict)));
      return;
    }
    if (char_get_absorbs(ch) > 3) {
      send_to_char(ch, "You already have already ingested 4 people.\r\n");
      return;
    }
    if (char_get_level(ch) < 25) {
      send_to_char(ch, "You can't ingest yet.\r\n");
      return;
    }
    if (char_get_level(ch) < 100 && char_get_level(ch) >= 75 && char_get_absorbs(ch) == 3) {
      send_to_char(ch, "You already have ingested as much as you can. You'll have to get more experienced.\r\n");
      return;
    }
    if (char_get_level(ch) < 75 && char_get_level(ch) >= 50 && char_get_absorbs(ch) == 2) {
      send_to_char(ch, "You already have ingested as much as you can. You'll have to get more experienced.\r\n");
      return;
    }
    if (char_get_level(ch) < 50 && char_get_level(ch) >= 25 && char_get_absorbs(ch) == 1) {
      send_to_char(ch, "You already have ingested as much as you can. You'll have to get more experienced.\r\n");
      return;
    }

    if (getEffMaxPL(vict) >= (getBasePL(ch)) * 3) {
      send_to_char(ch, "You are too weak to ingest them into your body!\r\n");
      return;
    }
    if (char_aff_flagged(vict, AFF_SANCTUARY)) {
      send_to_char(ch, "You can't ingest them, they have a barrier!\r\n");
      return;
    }
    reveal_hiding(ch, 0);
    if (char_aff_flagged(vict, AFF_ZANZOKEN) && (getCurST(vict)) >= 1 && char_get_position(vict) != POS_SLEEPING) {
      act("@C$N@c disappears, avoiding your attempted ingestion!@n", FALSE, ch, 0, vict, TO_CHAR);
      act("@cYou disappear, avoiding @C$n's@c attempted @ringestion@c before reappearing!@n", FALSE, ch, 0, vict, TO_VICT);
      act("@C$N@c disappears, avoiding @C$n's@c attempted @ringestion@c before reappearing!@n", FALSE, ch, 0, vict, TO_NOTVICT);
      char_aff_set(vict, AFF_ZANZOKEN, false);
      char_set_wait(ch, PULSE_3SEC);
      return;
    }
    if (char_get_speedi(ch) + rand_number(1, 5) < char_get_speedi(ch) + rand_number(1, 5)) {
      act("@WYou fling a piece of goo at @c$N@W, and try to ingest $M! $E manages to avoid your blob of goo though!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W flings a piece of goo at you, you manage to avoid it though!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@w flings a piece of goo at @c$N@W, but the goo misses $M@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_wait(ch, PULSE_3SEC);
      return;
    } else {
      act("@WYou flings a piece of goo at @c$N@W! The goo engulfs $M and then returns to your body!@n", TRUE, ch, 0, vict, TO_CHAR);
      act("@C$n@W flings a piece of goo at you! The goo engulfs your body and then returns to @C$n@W!@n", TRUE, ch, 0, vict, TO_VICT);
      act("@C$n@w flings a piece of goo at @c$N@W! The goo engulfs $M and then return to @C$n@W!@n", TRUE, ch, 0, vict, TO_NOTVICT);
      char_set_absorbs(ch, char_get_absorbs(ch) + (1));
      int64_t pl   = (getBasePL(vict)) / 6;
      int64_t stam = (getBaseST(vict)) / 6;
      int64_t ki   = (getBaseKI(vict)) / 6;
      gainBasePL(ch, pl, true);
      gainBaseST(ch, stam, true);
      gainBaseKI(ch, ki, true);
      if (!char_is_npc(vict) && !char_is_npc(ch)) {
        send_to_imm("[PK] %s killed %s at room [%d]\r\n", char_get_name(ch), char_get_name(vict), rroom_get_vnum(char_get_in_room(vict)));
        char_plr_set(vict, PLR_ABSORBED, true);
      }
      send_to_char(ch, "@D[@mINGEST@D] @rPL@W: @D(@y%s@D) @cKi@W: @D(@y%s@D) @gSt@W: @D(@y%s@D)@n\r\n", add_commas(pl), add_commas(ki), add_commas(stam));
      if (rand_number(1, 3) == 3) {
        send_to_char(ch, "You get %s's eye color.\r\n", char_get_name(vict));
        char_set_eye(ch, char_get_eye(vict));
      } else if (rand_number(1, 3) == 3) {
        send_to_char(ch, "%s changes your height.\r\n", char_get_name(vict));
        if (char_get_pc_height(ch) > char_get_pc_height(vict)) {
          char_set_height(ch, char_get_height(ch) - (((char_get_pc_height(ch) - char_get_pc_height(vict)) / 2)));
        } else if (char_get_pc_height(ch) < char_get_pc_height(vict)) {
          char_set_height(ch, char_get_height(ch) + (((char_get_pc_height(vict) - char_get_pc_height(ch)) / 2)));
        } else {
          char_set_height(ch, char_get_pc_height(vict));
        }
      } else if (rand_number(1, 3) == 3) {
        send_to_char(ch, "%s changes your weight.\r\n", char_get_name(vict));
        if (char_get_pc_weight(ch) > char_get_pc_weight(vict)) {
          char_set_weight(ch, char_get_weight(ch) - (((char_get_pc_weight(ch) - char_get_pc_weight(vict)) / 2)));
        } else if (char_get_pc_weight(ch) < char_get_pc_weight(vict)) {
          char_set_weight(ch, char_get_weight(ch) + (((char_get_pc_weight(vict) - char_get_pc_weight(ch)) / 2)));
        } else {
          char_set_weight(ch, char_get_pc_weight(vict));
        }
      } else {
        send_to_char(ch, "Your forelock length changes because of %s.\r\n", char_get_name(vict));
        char_set_hairl(ch, char_get_hairl(vict));
      }
      handle_ingest_learn(ch, vict);
      die(vict, NULL);
      return;
    }
  } // End of ingest

  else {
    send_to_char(ch, "You are not a majin, you can not ingest.\r\n");
    return;
  } // Error
}

ACMD(do_plant)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[100], obj_name[100];
  int roll = 0, detect = 0, fail = 0;

  if (rroom_flagged(char_get_in_room(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  two_arguments(argument, obj_name, vict_name);

  if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Plant what on who?\r\n");
    return;
  } else if (vict == ch) {
    send_to_char(ch, "Come on now, that's rather stupid!\r\n");
    return;
  }
  if (char_mob_flagged(vict, MOB_NOKILL) && char_get_admlevel(ch) == ADMLVL_NONE) {
    send_to_char(ch, "That isn't such a good idea...\r\n");
    return;
  }

  roll = roll_skill(ch, SKILL_SLEIGHT_OF_HAND) + rand_number(1, 3);
  fail = rand_number(1, 105);

  if (char_get_position(vict) < POS_SLEEPING)
    detect = 0;
  else
    detect = (roll_skill(vict, SKILL_SPOT) + rand_number(1, 3));

  /* NO NO With Imp's and Shopkeepers, and if player planting is not allowed */
  if ((char_adm_flagged(vict, ADM_NOSTEAL) || char_get_mob_spec(vict) == shop_keeper) && char_get_admlevel(ch) < 5)
    roll = -10; /* Failure */

  if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, ch->carrying))) {
    send_to_char(ch, "You don't have that to plant on them.\r\n");
    return;
  }
  if (roll <= detect && roll <= fail) {
    reveal_hiding(ch, 0);
    act("@C$n@w tries to plant $p@w on you!@n", TRUE, ch, obj, vict, TO_VICT);
    act("@C$n@w tries to plant $p@w on @c$N@w!@n", TRUE, ch, obj, vict, TO_NOTVICT);
    act("@wYou try and fail to plant $p@w on @c$N@w, and $E notices!@n", TRUE, ch, obj, vict, TO_CHAR);
    char_set_wait(ch, PULSE_2SEC);
    return;
  } else if (roll <= fail) {
    act("@wYou try and fail to plant $p@w on @c$N@w! However no one seemed to notice.@n", TRUE, ch, obj, vict, TO_CHAR);
    char_set_wait(ch, PULSE_2SEC);
    return;
  } else if (obj_get_weight(obj) + (getCurCarriedWeight(vict)) > getMaxCarryWeight(vict)) {
    reveal_hiding(ch, 0);
    act("@C$n@w tries to plant $p@w on you!@n", TRUE, ch, obj, vict, TO_VICT);
    act("@C$n@w tries to plant $p@w on @c$N@w!@n", TRUE, ch, obj, vict, TO_NOTVICT);
    act("@wYou try and fail to plant $p@w on @c$N@w because $E can't carry the weight. It seems $E noticed the attempt!@n", TRUE, ch, obj, vict, TO_CHAR);
    char_set_wait(ch, PULSE_2SEC);
  } else if (roll <= detect) {
    act("@cYou feel like the weight of your inventory has changed.@n", TRUE, ch, obj, vict, TO_VICT);
    act("@c$N@w looks around after feeling $S pockets.@n", TRUE, ch, obj, vict, TO_NOTVICT);
    act("@wYou plant $p@w on @c$N@w! @c$N @wseems to notice the change in weight in their inventory.@n", TRUE, ch, obj, vict, TO_CHAR);
    obj_from_char(obj);
    obj_to_char(obj, vict);
    char_set_wait(ch, PULSE_2SEC);
    return;
  } else {
    act("@wYou plant $p@w on @c$N@w! No one noticed, whew....@n", TRUE, ch, obj, vict, TO_CHAR);
    obj_from_char(obj);
    obj_to_char(obj, vict);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
}

ACMD(do_forgery)
{

  struct obj_data *obj2, *obj3 = NULL;
  struct obj_data *obj, *obj4  = NULL, *next_obj;
  int found = FALSE;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (char_is_npc(ch)) {
    return;
  }

  if (!know_skill(ch, SKILL_FORGERY)) {
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Okay, make a forgery of what?\r\n");
    return;
  }

  if (!(obj2 = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You want to make a fake copy of what?\r\n");
    return;
  }

  for (obj = ch->carrying; obj; obj = next_obj) {
    next_obj = obj->next_content;
    if (found == FALSE && obj_get_vnum(obj) == 19 && (!obj_extra_flagged(obj, ITEM_BROKEN) && !obj_extra_flagged(obj, ITEM_FORGED))) {
      found = TRUE;
      obj4  = obj;
    }
  }

  if (found == FALSE || obj4 == NULL) {
    send_to_char(ch, "You need a forgery kit.\r\n");
    return;
  }

  if (obj_get_vnum(obj2) == 19) {
    send_to_char(ch, "You can't duplicate a forgery kit.\r\n");
    return;
  }

  if (obj_extra_flagged(obj2, ITEM_FORGED)) {
    send_to_char(ch, "%s is forgery, there is no reason to make a fake of a fake!\r\n", obj2->short_description);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  if (obj_extra_flagged(obj2, ITEM_BROKEN)) {
    send_to_char(ch, "%s is broken, there is no reason to make a fake of this mess!\r\n", obj2->short_description);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  if (obj_get_vnum(obj2) >= 60000 || obj_get_vnum(obj2) == 0) {
    send_to_char(ch, "You can not make a forgery of that! It's far too squishy....");
    return;
  }
  if (obj_get_vnum(obj2) >= 18800 && obj_get_vnum(obj2) <= 18999) {
    send_to_char(ch, "You can not make a forgery of that!\r\n");
    return;
  }
  if (obj_get_vnum(obj2) >= 19080 && obj_get_vnum(obj2) <= 19199) {
    send_to_char(ch, "You can not make a forgery of that!\r\n");
    return;
  }
  if (obj_get_vnum(obj2) >= 4 && obj_get_vnum(obj2) <= 6) {
    send_to_char(ch, "You can not make a forgery of that!\r\n");
    return;
  }

  if (obj_extra_flagged(obj2, ITEM_PROTECTED)) {
    send_to_char(ch, "You don't know where to begin with this work of ART.\r\n");
    return;
  }

  reveal_hiding(ch, 0);
  act("@c$n@w looks at $p, begins to work on forging a fake copy of it.@n", TRUE, ch, obj2, 0, TO_ROOM);
  improve_skill(ch, SKILL_FORGERY, 1);
  if (char_get_skill(ch, SKILL_FORGERY) < axion_dice(0)) {
    if (rand_number(1, 10) >= 9) { /* Uh oh */
      send_to_char(ch, "In the middle of creating a forgery of %s you screw up. The fabrication unit built into the forgery kit melts and bonds with the original. You clumsy mistake with the Estex Titanium drill has broken both.\r\n", obj2->short_description);
      extract_obj(obj4);
      extract_obj(obj2);
      return;
    }
    send_to_char(ch, "You start to make a forgery of %s but screw up and waste your forgery kit..\r\n", obj2->short_description);
    act("@c$n@w tried to duplicate $p but screws up somehow.@n", TRUE, ch, obj2, 0, TO_ROOM);
    obj_from_char(obj4);
    extract_obj(obj4);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  int loadn = obj_get_vnum(obj2);

  obj3 = read_object(loadn, VIRTUAL);
  add_unique_id(obj3);
  obj_to_char(obj3, ch);

  /* Set Object Variables */
  obj_extra_set(obj3, ITEM_FORGED, true);
  obj_set_weight(obj3, rand_number(obj_get_weight(obj3) / 2, obj_get_weight(obj3)));

  obj_from_char(obj4);
  extract_obj(obj4);
  send_to_char(ch, "You make an excellent forgery of %s@n!\r\n", obj2->short_description);
  act("@c$n@w makes a perfect forgery of $p.@n", TRUE, ch, obj2, 0, TO_ROOM);
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

  if (!know_skill(ch, SKILL_APPRAISE)) {
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Okay, appraise what?\r\n");
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You want to appraise what?\r\n");
    return;
  }

  reveal_hiding(ch, 0);
  act("@c$n@w looks at $p, turning it over in $s hands.@n", TRUE, ch, obj, 0, TO_ROOM);
  improve_skill(ch, SKILL_APPRAISE, 1);
  if (char_get_skill(ch, SKILL_APPRAISE) < axion_dice(-10)) {
    send_to_char(ch, "You fail to perceive the worth of %s..\r\n", obj->short_description);
    act("@c$n@w looks stumped about $p.@n", TRUE, ch, obj, 0, TO_ROOM);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  if (obj_extra_flagged(obj, ITEM_BROKEN)) {
    send_to_char(ch, "%s is broken!\r\n", obj->short_description);
    act("@c$n@w looks at $p and frowns.@n", TRUE, ch, obj, 0, TO_ROOM);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }

  if (obj_extra_flagged(obj, ITEM_FORGED)) {
    send_to_char(ch, "%s is fake and worthless!\r\n", obj->short_description);
    act("@c$n@w looks at $p with an angry face.@n", TRUE, ch, obj, 0, TO_ROOM);
    char_set_wait(ch, PULSE_2SEC);
    return;
  }
  found         = FALSE;
  int displevel = obj_get_level(obj);

  if (obj_get_type_flag(obj) == ITEM_WEAPON && obj_extra_flagged(obj, ITEM_CUSTOM))
    displevel = 20;

  send_to_char(ch, "%s is worth: %s\r\nMin Lvl: %d\r\n", obj->short_description, add_commas(obj_get_cost(obj)), displevel);
  if (obj_get_type_flag(obj) == ITEM_WEAPON) {
    if (obj_extra_flagged(obj, ITEM_WEAPLVL1)) {
      send_to_char(ch, "Weapon Level: 1\nDamage Bonus: 5%s\r\n", "%");
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL2)) {
      send_to_char(ch, "Weapon Level: 2\nDamage Bonus: 10%s\r\n", "%");
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL3)) {
      send_to_char(ch, "Weapon Level: 3\nDamage Bonus: 20%s\r\n", "%");
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL4)) {
      send_to_char(ch, "Weapon Level: 4\nDamage Bonus: 30%s\r\n", "%");
    } else if (obj_extra_flagged(obj, ITEM_WEAPLVL5)) {
      send_to_char(ch, "Weapon Level: 5\nDamage Bonus: 50%s\r\n", "%");
    }
  }
  send_to_char(ch, "Size: %s\r\n", size_names[obj_get_size(obj)]);
  if (obj_extra_flagged(obj, ITEM_SLOT1) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(ch, "Token Slots  : @m0/1@n\n");
  } else if (obj_extra_flagged(obj, ITEM_SLOT1) && obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(ch, "Token Slots  : @m1/1@n\n");
  } else if (obj_extra_flagged(obj, ITEM_SLOT2) && !obj_extra_flagged(obj, ITEM_SLOT_ONE) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(ch, "Token Slots  : @m0/2@n\n");
  } else if (obj_extra_flagged(obj, ITEM_SLOT2) && obj_extra_flagged(obj, ITEM_SLOT_ONE) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(ch, "Token Slots  : @m1/2@n\n");
  } else if (obj_extra_flagged(obj, ITEM_SLOT2) && !obj_extra_flagged(obj, ITEM_SLOTS_FILLED)) {
    send_to_char(ch, "Token Slots  : @m2/2@n\n");
  }
  send_to_char(ch, "Bonuses:");
  act("@c$n@w looks at $p and nods, a satisfied look on $s face.@n", TRUE, ch, obj, 0, TO_ROOM);
  int percent = FALSE;
  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    if (obj->affected[i].modifier) {
      if (obj->affected[i].location == APPLY_REGEN || obj->affected[i].location == APPLY_TRAIN || obj->affected[i].location == APPLY_LIFEMAX) {
        percent = TRUE;
      }
      sprinttype(obj->affected[i].location, apply_types, buf, sizeof(buf));
      send_to_char(ch, "%s %+d%s to %s", found++ ? "," : "", obj->affected[i].modifier, percent == TRUE ? "%" : "", buf);
      percent = FALSE;
      switch (obj->affected[i].location) {
        case APPLY_FEAT:
          send_to_char(ch, " (%s)", feat_list[obj->affected[i].specific].name);
          break;
        case APPLY_SKILL:
          send_to_char(ch, " (%s)", spell_info[obj->affected[i].specific].name);
          break;
      }
    }
  }
  if (!found)
    send_to_char(ch, " None");
  char buf2[MAX_STRING_LENGTH];
  sprintbitarray(obj_get_bitvector(obj), affected_bits, AF_ARRAY_MAX, buf2, sizeof(buf2));
  send_to_char(ch, "\nSpecial: %s\r\n", buf2);

  char_set_wait(ch, PULSE_2SEC);
}

ACMD(do_disguise)
{
  int skill = 0, roll = 0;

  if (char_is_npc(ch)) {
    send_to_char(ch, "You forgot your disguise off in mobland.\r\n");
    return;
  }

  if (char_plr_flagged(ch, PLR_DISGUISED)) {
    send_to_char(ch, "You stop disguising yourself.\r\n");
    char_plr_set(ch, PLR_DISGUISED, false);
    act("@C$n @wpulls off $s disguise and reveals $mself!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }

  if (!know_skill(ch, SKILL_DISGUISE)) {
    return;
  }

  if (!char_get_equipment(ch, WEAR_HEAD)) {
    send_to_char(ch, "You can't disguise your identity without anything on your head.\r\n");
    return;
  }

  if ((getCurST(ch)) < (getCurST(ch)) / 50) {
    send_to_char(ch, "You are too tired to try that right now.\r\n");
    return;
  }

  skill = char_get_skill(ch, SKILL_DISGUISE);
  roll  = axion_dice(-10);

  if (skill > roll) {
    send_to_char(ch, "You managed to disguise yourself with some skilled manipulation of your headwear.\r\n");
    act("@C$n @wmanages to disguise $mself with some skilled manipulation of $s headwear.", TRUE, ch, 0, 0, TO_ROOM);
    char_plr_set(ch, PLR_DISGUISED, true);
    return;
  } else {
    send_to_char(ch, "You finish attempting to disguise yourself, but realize you failed and need to try again.\r\n");
    act("@C$n @wattempts and fails to disguise $mself properly and must try again.", TRUE, ch, 0, 0, TO_ROOM);
    decCurST(ch, getMaxST(ch) / 50, 0);
    return;
  }
}

ACMD(do_use)
{
  char buf[100], arg[MAX_INPUT_LENGTH];
  struct obj_data *mag_item = NULL;

  half_chop(argument, arg, buf);
  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }

  if (!mag_item) {
    switch (subcmd) {
      case SCMD_RECITE:
      case SCMD_QUAFF:
        if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
          send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
          return;
        }
        break;
      case SCMD_USE:
        if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
          send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
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
        send_to_char(ch, "You can only swallow beans.\r\n");
        return;
      }
      if (char_is_android(ch)) {
        send_to_char(ch, "You can't swallow beans, you are an android.\r\n");
        return;
      }
      if (obj_extra_flagged(mag_item, ITEM_FORGED)) {
        send_to_char(ch, "You can't swallow that, it is fake!\r\n");
        return;
      }
      if (obj_extra_flagged(mag_item, ITEM_BROKEN)) {
        send_to_char(ch, "You can't swallow that, it is broken!\r\n");
        return;
      }
      break;
    case SCMD_RECITE:
      if (obj_get_type_flag(mag_item) != ITEM_SCROLL) {
        send_to_char(ch, "You can only recite scrolls.\r\n");
        return;
      }
      break;
    case SCMD_USE:
      if (char_is_android(ch)) {
        send_to_char(ch, "You are not biological enough to use these, Tincan.\r\n");
        return;
      } else {
        switch (obj_get_vnum(mag_item)) {
          case 381:
            if ((getCurST(ch)) >= getMaxST(ch)) {
              send_to_char(ch, "Your stamina is full.\r\n");
              return;
            }
            act("@WYou place the $p@W against your chest and feel a rush of stamina as it automatically administers the dose.@n", TRUE, ch, mag_item, 0, TO_CHAR);
            act("@C$n@W places an $p@W against $s chest and a loud click is heard.@n", TRUE, ch, mag_item, 0, TO_ROOM);
            if (char_get_skill(ch, SKILL_FIRST_AID) > 0) {
              send_to_char(ch, "@CYour skill in First Aid has helped increase the use of the injector. You gain more stamina as a result.@n\r\n");
              incCurST(ch, getMaxST(ch) * .25, true);
            } else {
              incCurST(ch, getMaxST(ch) * .1, true);
            }
            extract_obj(mag_item);
            return;
          case 382:
            if (char_aff_flagged(ch, AFF_BURNED)) {
              act("@WYou gently apply the salve to your burns.@n", TRUE, ch, mag_item, 0, TO_CHAR);
              act("@C$n@W gently applies a burn salve to $s burns.@n", TRUE, ch, mag_item, 0, TO_ROOM);
              char_aff_set(ch, AFF_BURNED, false);
              extract_obj(mag_item);
            } else {
              send_to_char(ch, "You are not burned.\r\n");
            }
            return;
          case 383:
            if (char_aff_flagged(ch, AFF_POISON)) {
              act("@WYou place the $p@W against your neck and feel a rush of relief as the antitoxiin enters your bloodstream.@n", TRUE, ch, mag_item, 0, TO_CHAR);
              act("@C$n@W places an $p@W against $s neck and a loud click is heard.@n", TRUE, ch, mag_item, 0, TO_ROOM);
              null_affect(ch, AFF_POISON);
              extract_obj(mag_item);
            } else {
              send_to_char(ch, "You are not poisoned.\r\n");
            }
            return;
          case 385:
            act("@WYou drink the contents of the vial before disposing of it.@n", TRUE, ch, mag_item, 0, TO_CHAR);
            act("@C$n@W dinks a $p and then disposes of it.@n", TRUE, ch, mag_item, 0, TO_ROOM);
            if (char_aff_flagged(ch, AFF_BLIND)) {
              act("@WYour eyesight has returned!@n", TRUE, ch, mag_item, 0, TO_CHAR);
              act("@C$n@W eyesight seems to have returned.@n", TRUE, ch, mag_item, 0, TO_ROOM);
              null_affect(ch, AFF_BLIND);
            }
            refreshed = FALSE;

            if (getCurPL(ch) <= getEffMaxPL(ch) * 0.99) {
              incCurHealth(ch, large_rand(getEffMaxPL(ch) * 0.08, getEffMaxPL(ch) * 0.16), true);
              refreshed = TRUE;
            } else if ((getCurKI(ch)) <= getEffMaxPL(ch) * 0.99) {
              incCurKI(ch, large_rand(getMaxKI(ch) * 0.08, getMaxKI(ch) * 0.16), true);
              refreshed = TRUE;
            } else if ((getCurST(ch)) <= getMaxST(ch) * 0.99) {
              incCurST(ch, large_rand(getMaxST(ch) * 0.08, getMaxST(ch) * 0.16), true);
              refreshed = TRUE;
            }
            if (refreshed == TRUE) {
              send_to_char(ch, "@CYou feel refreshed!\r\n");
            }
            extract_obj(mag_item);
            return;
          default:
            send_to_char(ch, "That is not something you can apparently use.\r\n");
            return;
        }
      }
      break;
  }

  mag_objectmagic(ch, mag_item, buf);
}

ACMD(do_value)
{
  char arg[MAX_INPUT_LENGTH];
  int value_lev;

  one_argument(argument, arg);

  if (!*arg) {
    switch (subcmd) {
      case SCMD_WIMPY:
        if (char_get_wimp_level(ch)) {
          send_to_char(ch, "Your current wimp level is %d powerlevel.\r\n", char_get_wimp_level(ch));
          return;
        } else {
          send_to_char(ch, "At the moment, you're not a wimp.  (sure, sure...)\r\n");
          return;
        }
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
            send_to_char(ch, "Heh, heh, heh.. we are jolly funny today, eh?\r\n");
          else if (value_lev > getEffMaxPL(ch))
            send_to_char(ch, "That doesn't make much sense, now does it?\r\n");
          else if (value_lev > (getEffMaxPL(ch) * 0.5))
            send_to_char(ch, "You can't set your wimp level above half your powerlevel.\r\n");
          else {
            send_to_char(ch, "Okay, you'll wimp out if you drop below %d powerlevel.\r\n", value_lev);
            char_set_wimp_level(ch, value_lev);
          }
        } else {
          send_to_char(ch, "Okay, you'll now tough out fights to the bitter end.\r\n");
          char_set_wimp_level(ch, 0);
        }
        break;
      default:
        mud_log("Unknown subcmd to do_value %d called by %s", subcmd, char_get_name(ch));
        break;
    }
  } else
    send_to_char(ch, "Specify a value.  (0 to disable)\r\n");
}

ACMD(do_compare)
{
  char arg1[100];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj1, *obj2;
  struct char_data *tchar;
  int value1 = 0, value2 = 0, o1, o2;
  const char *msg = NULL;

  two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2) {
    send_to_char(ch, "Compare what to what?\n\r");
    return;
  }

  o1 = generic_find(arg1, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &tchar, &obj1);
  o2 = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &tchar, &obj2);

  if (!o1 || !o2) {
    send_to_char(ch, "You do not have that item.\r\n");
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

  if (msg == NULL) {
    if (value1 == value2)
      msg = "$p and $P look about the same.";
    else if (value1 > value2)
      msg = "$p looks better than $P.";
    else
      msg = "$p looks worse than $P.";
  }

  act(msg, FALSE, ch, obj1, obj2, TO_CHAR);
  return;
}

ACMD(do_break)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct char_data *dummy = NULL;
  int cmbrk;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Usually you break SOMETHING.\r\n");
    return;
  }

  if (!(cmbrk = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    send_to_char(ch, "Can't seem to find what you want to break!\r\n");
    return;
  }

  if (obj_extra_flagged(obj, ITEM_BROKEN)) {
    send_to_char(ch, "Seems like it's already broken!\r\n");
    return;
  }

  /* Ok, break it! */
  send_to_char(ch, "You ruin %s.\r\n", obj->short_description);
  act("$n ruins $p.", FALSE, ch, obj, 0, TO_ROOM);
  obj_set_value(obj, VAL_ALL_HEALTH, 0);
  obj_extra_toggle(obj, ITEM_BROKEN);

  return;
}

ACMD(do_fix)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj, *obj4 = NULL, *rep, *next_obj;
  struct char_data *dummy = NULL;
  int cmbrk, found = FALSE, self = FALSE, custom = FALSE;

  one_argument(argument, arg);

  if (!know_skill(ch, SKILL_REPAIR)) {
    return;
  }

  if (!*arg) {
    send_to_char(ch, "Usually you fix SOMETHING.\r\n");
    return;
  }

  if (!strcasecmp("self", arg)) {
    if (!char_is_android(ch)) {
      send_to_char(ch, "Only androids can fix their bodies with repair kits.\r\n");
      return;
    } else {
      self = TRUE;
    }
  }

  if (self == FALSE) {
    if (!(cmbrk = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM, ch, &dummy, &obj))) {
      send_to_char(ch, "Can't seem to find what you want to fix!\r\n");
      return;
    }

    if ((cmbrk) && obj_get_value(obj, VAL_ALL_HEALTH) >= 100) {
      send_to_char(ch, "But it isn't even damaged!\r\n");
      return;
    }

    if (obj_extra_flagged(obj, ITEM_FORGED)) {
      send_to_char(ch, "That is fake, why bother fixing it?\r\n");
      return;
    }

    switch (obj_get_value(obj, VAL_ALL_MATERIAL)) {
      case MATERIAL_ORGANIC:
      case MATERIAL_FOOD:
      case MATERIAL_PAPER:
      case MATERIAL_LIQUID:
        send_to_char(ch, "You can't repair that.\r\n");
        return;
        break;
    }

    if (obj_get_vnum(obj) == 20099 || obj_get_vnum(obj) == 20098) {
      custom = TRUE;
    }
  }

  for (rep = ch->carrying; rep; rep = next_obj) {
    next_obj = rep->next_content;
    if (custom == FALSE) {
      if (found == FALSE && obj_get_vnum(rep) == 48 && (!obj_extra_flagged(rep, ITEM_BROKEN) && !obj_extra_flagged(rep, ITEM_FORGED))) {
        found = TRUE;
        obj4  = rep;
      }
    } else {
      if (found == FALSE && obj_get_vnum(rep) == 13593 && (!obj_extra_flagged(rep, ITEM_BROKEN) && !obj_extra_flagged(rep, ITEM_FORGED))) {
        found = TRUE;
        obj4  = rep;
      }
    }
  }

  if (found == FALSE && custom == FALSE) {
    send_to_char(ch, "You do not even have a repair kit.\r\n");
    return;
  } else if (found == FALSE && custom == TRUE) {
    send_to_char(ch, "You do not even have a Nano-tech Repair Orb.\r\n");
    return;
  }

  if (self == FALSE) {
    if (char_get_skill(ch, SKILL_REPAIR) < axion_dice(0)) {
      act("You try to repair $p but screw up..", TRUE, ch, obj, 0, TO_CHAR);
      act("$n tries to repair $p but screws up..", TRUE, ch, obj, 0, TO_ROOM);
      extract_obj(obj4);
      improve_skill(ch, SKILL_REPAIR, 1);
      char_set_wait(ch, PULSE_2SEC);
      return;
    }

    if (obj_get_value(obj, VAL_ALL_HEALTH) + char_get_skill(ch, SKILL_REPAIR) < 100) {
      send_to_char(ch, "You repair %s a bit.\r\n", obj->short_description);
      act("$n repairs $p a bit.", FALSE, ch, obj, 0, TO_ROOM);
      obj_set_value(obj, VAL_ALL_HEALTH, obj_get_value(obj, VAL_ALL_HEALTH) + (char_get_skill(ch, SKILL_REPAIR)));
      obj_extra_set(obj, ITEM_BROKEN, false);
    } else {
      send_to_char(ch, "You repair %s completely.\r\n", obj->short_description);
      act("$n repairs $p completely.", FALSE, ch, obj, 0, TO_ROOM);
      obj_set_value(obj, VAL_ALL_HEALTH, 100);
      obj_extra_set(obj, ITEM_BROKEN, false);
    }
    if (obj->carried_by == NULL && !char_plr_flagged(ch, PLR_REPLEARN) && (level_exp(ch, char_get_level(ch) + 1) - char_get_exp(ch) > 0 || char_get_level(ch) >= 100)) {
      int64_t gain = (level_exp(ch, char_get_level(ch) + 1) * 0.0003) * char_get_skill(ch, SKILL_REPAIR);
      send_to_char(ch, "@mYou've learned a bit from repairing it. @D[@gEXP@W: @G+%s@D]@n\r\n", add_commas(gain));
      char_plr_set(ch, PLR_REPLEARN, true);
      gain_exp(ch, gain);
    } else if (rand_number(2, 12) >= 10 && char_plr_flagged(ch, PLR_REPLEARN)) {
      char_plr_set(ch, PLR_REPLEARN, false);
      send_to_char(ch, "@mYou think you might be on to something...@n\r\n");
    }
    improve_skill(ch, SKILL_REPAIR, 1);
    extract_obj(obj4);
    char_set_wait(ch, PULSE_2SEC);
    return;
  } else { /* For androids repairing themselves */

    if (getCurPL(ch) >= getEffMaxPL(ch)) {
      send_to_char(ch, "Your body is already in peak condition.\r\n");
      return;
    } else if (char_get_skill(ch, SKILL_REPAIR) < axion_dice(0)) {
      act("You try to repair your body but screw up..", TRUE, ch, 0, 0, TO_CHAR);
      act("$n tries to repair $s body but screws up..", TRUE, ch, 0, 0, TO_ROOM);
      extract_obj(obj4);
      improve_skill(ch, SKILL_REPAIR, 1);
      char_set_wait(ch, PULSE_5SEC);
      return;
    } else {
      act("You use the repair kit to fix part of your body...", TRUE, ch, 0, 0, TO_CHAR);
      act("$n works on their body with a repair kit.", TRUE, ch, 0, 0, TO_ROOM);
      int64_t mult = char_get_skill(ch, SKILL_REPAIR);
      int64_t add  = (getEffMaxPL(ch) * 0.005 + 10) * mult;

      extract_obj(obj4);
      if (incCurHealth(ch, add, true) == getEffMaxPL(ch)) {
        send_to_char(ch, "Your body has been totally repaired.\r\n");
        char_set_wait(ch, PULSE_5SEC);
      } else {
        send_to_char(ch, "Your body still needs some work done to it.\r\n");
        char_set_wait(ch, PULSE_5SEC);
      }
    }
  }
}
