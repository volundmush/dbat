/***************************************************** 
* maputils.c --- implementation file for ascii maps  * 
*				     		     * 
* Kyle Goodwin, (c) 1998 All Rights Reserved         * 
* vbmasta@earthlink.net - Head Implemenor FirocMUD   * 
*				     		     * 
* Paolo Libardi - pinkpallin@libero.it		     * 
*****************************************************/
#include <cerrno>

#include "dbat/iterate.hpp"
#include "dbat/maputils.hpp"
#include "dbat/utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/db.hpp"
#include "dbat/vehicles.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"


static void ping_ship(GameWorld *g, obj_vnum vnum, obj_vnum vnum2)
{

  struct obj_data *controls = nullptr, *obj = nullptr;
  int found = false;

  if (vnum2 == -1) {
    return;
  }

  for (char_data *tch : each_char(g)) {
    if (found == false) {
      if (!(obj = find_control(g, tch))) {
        continue;
      }
      if (obj_get_value(obj, 0) == vnum && vnum != vnum2) {
        controls = obj;
        found    = true;
      }
    }
  }

  if (found == true) {
    send_to_room(g, obj_get_in_room(controls), "@D[@RALERT@D: @YAn unknown radar signal has been detected!@D]@n");
  }
}

static int checkship(GameWorld *g, room_vnum rnum, obj_vnum vnum)
{
  int there = false;

  for (obj_data *item : room_contents(g, rnum)) {
    if (!room_flagged(g, rnum, ROOM_NEBULA)) {
      if (obj_get_type_flag(item) == ITEM_VEHICLE && there != true) {
        there = true;
        ping_ship(g, obj_get_vnum(g, item), vnum);
      }
    }
  }

  return there;
}

static std::string getmapchar(GameWorld *g, room_vnum rnum, struct char_data *ch, room_vnum start, obj_vnum vnum)
{
  std::string mapchar;
  int there = false, enemy = false;

  if (rnum == start) {
    there = true;
  }
  if (checkship(g, rnum, vnum)) {
    enemy = true;
  }

  if (rnum == char_get_radar1(ch) || rnum == char_get_radar2(ch) || rnum == char_get_radar3(ch)) {
    if (there) {
      mapchar = "@WB@RX";
    } else if (enemy == true) {
      mapchar = "@WB@r#";
    } else {
      mapchar = "@WBB";
    }
  } /* End Buoy */
  else if (room_flagged(g, rnum, ROOM_EORBIT)) {
    if (there) {
      mapchar = "@GE@RX";
    } else if (enemy == true) {
      mapchar = "@GE@r#";
    } else {
      mapchar = "@GEE";
    }
  } /* End Earth Character */
  else if (room_flagged(g, rnum, ROOM_CORBIT)) {
    if (there) {
      mapchar = "@MC@RX";
    } else if (enemy == true) {
      mapchar = "@MC@r#";
    } else {
      mapchar = "@MCC";
    }
  } /* End Cerria Character */
  else if (room_flagged(g, rnum, ROOM_FORBIT)) {
    if (there) {
      mapchar = "@CF@RX";
    } else if (enemy == true) {
      mapchar = "@CF@r#";
    } else {
      mapchar = "@CFF";
    }
  } /* End Frigid Character */
  else if (room_flagged(g, rnum, ROOM_KORBIT)) {
    if (there) {
      mapchar = "@mK@RX";
    } else if (enemy == true) {
      mapchar = "@mK@r#";
    } else {
      mapchar = "@mKK";
    }
  } /* End Konack Character */
  else if (room_flagged(g, rnum, ROOM_NORBIT)) {
    if (there) {
      mapchar = "@gN@RX";
    } else if (enemy == true) {
      mapchar = "@gN@r#";
    } else {
      mapchar = "@gNN";
    }
  } /* End Namek Character */
  else if (rnum == 50772) {
    if (there) {
      mapchar = "@cZ@RX";
    } else if (enemy == true) {
      mapchar = "@cZ@r#";
    } else {
      mapchar = "@cZZ";
    }
  } /* End Zenith Character */
  else if (room_flagged(g, rnum, ROOM_VORBIT)) {
    if (there) {
      mapchar = "@YV@RX";
    } else if (enemy == true) {
      mapchar = "@YV@r#";
    } else {
      mapchar = "@YVV";
    }
  } /* End Vegeta Character */
  else if (room_flagged(g, rnum, ROOM_AORBIT)) {
    if (there) {
      mapchar = "@BA@RX";
    } else if (enemy == true) {
      mapchar = "@BA@r#";
    } else {
      mapchar = "@BAA";
    }
  } /* End Aether Character */
  else if (room_flagged(g, rnum, ROOM_YORBIT)) {
    if (there) {
      mapchar = "@MY@RX";
    } else if (enemy == true) {
      mapchar = "@MY@r#";
    } else {
      mapchar = "@MYY";
    }
  } /* End Yardrat Character */
  else if (room_flagged(g, rnum, ROOM_KANORB)) {
    if (there) {
      mapchar = "@CK@RX";
    } else if (enemy == true) {
      mapchar = "@CK@r#";
    } else {
      mapchar = "@CKK";
    }
  } /* End Kanassa Character */
  else if (room_flagged(g, rnum, ROOM_ARLORB)) {
    if (there) {
      mapchar = "@mA@RX";
    } else if (enemy == true) {
      mapchar = "@mA@r#";
    } else {
      mapchar = "@mAA";
    }
  } /* End Arlia Character */
  else if (room_flagged(g, rnum, ROOM_NEBULA)) {
    if (there) {
      mapchar = "@m&@RX";
    } else if (enemy == true) {
      mapchar = "@m&@r#";
    } else {
      mapchar = "@m&&";
    }
  } /* End Nebula Character */
  else if (rnum == 38028) {
    if (there) {
      mapchar = "@yQ@RX";
    } else if (enemy == true) {
      mapchar = "@yQ@r#";
    } else {
      mapchar = "@yQQ";
    }
  } /* End Namek Character */
  else if (room_flagged(g, rnum, ROOM_ASTERO)) {
    if (there) {
      mapchar = "@y:@RX";
    } else if (enemy == true) {
      mapchar = "@y:@r#";
    } else {
      mapchar = "@y::";
    }
  } /* End Asteroid Character */
  else if (room_flagged(g, rnum, ROOM_WORMHO)) {
    if (there) {
      mapchar = "@b@1*@RX@n";
    } else if (enemy == true) {
      mapchar = "@b@1*@r#@n";
    } else {
      mapchar = "@b@1**@n";
    }
  } /* End Wormhole Character */
  else if (room_flagged(g, rnum, ROOM_STATION)) {
    if (there) {
      mapchar = "@DS@RX";
    } else if (enemy == true) {
      mapchar = "@DS@r#";
    } else {
      mapchar = "@DSS";
    }
  } /* End Station Character */
  else if (room_flagged(g, rnum, ROOM_STAR)) {
    if (there) {
      mapchar = "@6 @RX@n";
    } else if (enemy == true) {
      mapchar = "@6 @r#@n";
    } else {
      mapchar = "@6  @n";
    }
  } /* End Star Character */
  else {
    if (there) {
      mapchar = "@w @RX";
    } else if (enemy == true) {
      mapchar = "@w @r#";
    } else {
      int color = rand_number(g, 1, 30);
      if (rand_number(g, 1, 40) == 2) {
        mapchar = str_format("{}. ", color > 15 ? "@w" : (color >= 7 ? "@Y" : (color > 3 ? "@R" : "@B")));
      } else if (rand_number(g, 1, 40) == 2) {
        mapchar = str_format("{} .", color > 15 ? "@w" : (color >= 7 ? "@Y" : (color > 3 ? "@R" : "@B")));
      } else {
        mapchar = "@w  ";
      }
    }
  } /* End default Character */
  return mapchar;
}

static MapStruct findcoord(GameWorld *g, room_vnum rnum)
{
  int x, y;
  MapStruct coords;

  coords.x = 0;
  coords.y = 0;

  for (y = 0; y <= MAP_ROWS; y++) {
    for (x = 0; x <= MAP_COLS; x++) {
      if (g->mapnums[y][x] == rnum) {
        coords.y = y;
        coords.x = x;
        return coords;
      }
    }
  }

  mud_log("SYSERR: findcoord for non-map rnum");
  return coords;
}

void printmap(GameWorld *g, room_vnum rnum, struct char_data *ch, int type, obj_vnum vnum)
{
  int x = 0, lasty = -1;
  int y = 0;
  int sightradius;
  int count = 0, initline = 0;
  std::string buf;
  char buf2[512];
  MapStruct coord;

  room_vnum start = rnum;

  coord = findcoord(g, rnum);
  buf   = "\n";
  if (type == 0) {
    sightradius = 12;
  } else {
    sightradius = 4;
  }

  if (type == 0) {
    send_to_char(g, ch, "@b______________________________________________________________________@n\n");
  }

  for (y = coord.y - sightradius; y <= coord.y + sightradius; y++) {
    if (type == 0) {
      if (count == initline) {
        buf += "@b     [@CR. Key@b]     | ";
      } else if (count == initline + 1) {
        buf += "@GEE@D:@w Earth@b         | ";
      } else if (count == initline + 2) {
        buf += "@gNN@D:@w Namek@b         | ";
      } else if (count == initline + 3) {
        buf += "@YVV@D:@w Vegeta@b        | ";
      } else if (count == initline + 4) {
        buf += "@CFF@D:@w Frigid@b        | ";
      } else if (count == initline + 5) {
        buf += "@mKK@D:@w Konack@b        | ";
      } else if (count == initline + 6) {
        buf += "@BAA@D:@w Aether@b        | ";
      } else if (count == initline + 7) {
        buf += "@MYY@D:@w Yardrat@b       | ";
      } else if (count == initline + 8) {
        buf += "@CKK@D:@w Kanassa@b       | ";
      } else if (count == initline + 9) {
        buf += "@mAA@D:@w Arlia@b         | ";
      } else if (count == initline + 10) {
        buf += "@cZZ@D:@w Zenith@b        | ";
      } else if (count == initline + 11) {
        buf += "@MCC@D:@w Cerria@b        | ";
      } else if (count == initline + 12) {
        buf += "@WBB@D:@w Buoy@b          | ";
      } else if (count == initline + 13) {
        buf += "@m&&@D:@w Nebula@b        | ";
      } else if (count == initline + 14) {
        buf += "@yQQ@D:@w Asteroid@b      | ";
      } else if (count == initline + 15) {
        buf += "@y::@D:@w Asteroid Field@b| ";
      } else if (count == initline + 16) {
        buf += "@b@1**@n@D:@w Wormhole@b      | ";
      } else if (count == initline + 17) {
        buf += "@DSS@D:@w S. Station@b    | ";
      } else if (count == initline + 18) {
        buf += " @r#@D:@w Unknown Ship@b  | ";
      } else if (count == initline + 19) {
        buf += "@6  @n@D:@w Star@b          | ";
      } else {
        buf += "                  @b| ";
      }
      count++;
    } else {
      if (count == 0) {
        buf += "      @RCompass@n           ";
      } else if (count == 2) {
        sprintf(buf2, "@w       @w|%s@w|            ", (room_get_dir_option(g, rnum, 0) ? " @CN " : "   "));
        buf += buf2;
      } else if (count == 3) {
        sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|      ", (room_get_dir_option(g, rnum, 6) ? " @CNW" : "   "), (room_get_dir_option(g, rnum, 4) ? " @YU " : "   "), (room_get_dir_option(g, rnum, 7) ? "@CNE " : "   "));
        buf += buf2;
      } else if (count == 4) {
        sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|      ", (room_get_dir_option(g, rnum, 3) ? "  @CW" : "   "), (room_get_dir_option(g, rnum, 10) ? "@m I " : (room_get_dir_option(g, rnum, 11) ? "@mOUT" : "   ")), (room_get_dir_option(g, rnum, 1) ? "@CE  " : "   "));
        buf += buf2;
      } else if (count == 5) {
        sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|      ", (room_get_dir_option(g, rnum, 9) ? " @CSW" : "   "), (room_get_dir_option(g, rnum, 5) ? " @YD " : "   "), (room_get_dir_option(g, rnum, 8) ? "@CSE " : "   "));
        buf += buf2;
      } else if (count == 6) {
        sprintf(buf2, "@w       @w|%s@w|            ", (room_get_dir_option(g, rnum, 2) ? " @CS " : "   "));
        buf += buf2;
      } else {
        buf += "                        ";
      }
      count++;
    }
    for (x = coord.x - sightradius; x <= coord.x + sightradius; x++) {
      if (x == coord.x && y == coord.y) {
        buf += getmapchar(g, g->mapnums[y][x], ch, start, vnum);
      } else if (x > MAP_COLS || x < 0) {
        if (lasty != true && y > -1 && y < 200) {
          buf += "@D?";
          lasty = true;
        }
      } else if (y > MAP_ROWS || y < 0) {
        if (y == -1 || y == 200) {
          buf += "@D??";
        }
      } else
        buf += getmapchar(g, g->mapnums[y][x], ch, start, vnum);
    }
    buf += "\n";
    lasty = false;
  }

  send_to_char(g, ch, buf);
  *buf2 = '\0';
  buf.clear();
  if (type == 0) {
    send_to_char(g, ch, "\n@b______________________________________________________________________@n");
  }
}

/*Update things that need it. Just space for the moment - Iovan */
void update_space(GameWorld *g)
{
  FILE *mapfile;
  int rowcounter, colcounter;
  room_vnum vnum_read;

  mud_log("Updated Space Map. ");

  //Load the map vnums from a file into an array
  if (!(mapfile = fopen("../data/surface.map", "r"))) {
    mud_log("SYSERR: update_space: can't open ../data/surface.map: {}", strerror(errno));
    return;
  }

  for (rowcounter = 0; rowcounter <= MAP_ROWS; rowcounter++) {
    for (colcounter = 0; colcounter <= MAP_COLS; colcounter++) {
      g->mapnums[rowcounter][colcounter] = fscanf(mapfile, "%d", &vnum_read) == 1 && room_of(g, vnum_read) ? vnum_read : NOWHERE;
    }
  }

  fclose(mapfile);
}
