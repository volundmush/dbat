/*****************************************************
 * maputils.c --- implementation file for ascii maps  *
 *				     		     *
 * Kyle Goodwin, (c) 1998 All Rights Reserved         *
 * vbmasta@earthlink.net - Head Implemenor FirocMUD   *
 *				     		     *
 * Paolo Libardi - pinkpallin@libero.it		     *
 *****************************************************/
#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/Destination.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/maputils.hpp"
//#include "dbat/game/send.hpp"
#include "dbat/game/comm.hpp"
//#include "dbat/game/interpreter.hpp"
#include "dbat/game/db.hpp"
#include "dbat/game/vehicles.hpp"
#include "dbat/util/FilterWeak.hpp"
#include "dbat/game/Random.hpp"

int mapnums[MAP_ROWS + 1][MAP_COLS + 1];

void ping_ship(int vnum, int vnum2)
{

    Character *tch, *next_ch;
    Object *controls = nullptr, *obj = nullptr;
    int found = false;

    if (vnum2 == -1)
    {
        return;
    }

    auto ac = characterSubscriptions.all("active");
    for (auto tch : dbat::util::filter_raw(ac))
    {
        if (!(obj = find_control(tch)))
        {
            continue;
        }
        if (GET_OBJ_VAL(obj, VAL_CONTROL_VEHICLE_VNUM) == vnum && vnum != vnum2)
        {
            controls = obj;
            break;
        }
    }

    if (controls)
    {
        controls->location.sendText("@D[@RALERT@D: @YAn unknown radar signal has been detected!@D]@n");
    }
}

int checkship(int rnum, int vnum)
{
    if (WHERE_FLAGGED(rnum, WhereFlag::nebula))
        return false;

    auto check = [](const auto &o)
    { return GET_OBJ_TYPE(o) == ITEM_VEHICLE; };
    if (auto found = Location(get_room(rnum)).searchObjects(check); found)
    {
        ping_ship(GET_OBJ_VNUM(found), vnum);
        return true;
    }

    return false;
}

struct RoomType
{
    WhereFlag flag;
    const char *thereChar;
    const char *enemyChar;
    const char *defaultChar;
};

const char *getMapCharForRoomType(bool there, bool enemy, const RoomType &roomType)
{
    if (there)
        return roomType.thereChar;
    if (enemy)
        return roomType.enemyChar;
    return roomType.defaultChar;
}

// Room type mapping
static const RoomType roomTypes[] = {
    {WhereFlag::earth_orbit, "@GE@RX", "@GE@r#", "@GEE"},
    {WhereFlag::cerria_orbit, "@MC@RX", "@MC@r#", "@MCC"},
    {WhereFlag::frigid_orbit, "@CF@RX", "@CF@r#", "@CFF"},
    {WhereFlag::konack_orbit, "@mK@RX", "@mK@r#", "@mKK"},
    {WhereFlag::namek_orbit, "@gN@RX", "@gN@r#", "@gNN"},
    {WhereFlag::vegeta_orbit, "@YV@RX", "@YV@r#", "@YVV"},
    {WhereFlag::aether_orbit, "@BA@RX", "@BA@r#", "@BAA"},
    {WhereFlag::yardrat_orbit, "@MY@RX", "@MY@r#", "@MYY"},
    {WhereFlag::kanassa_orbit, "@CK@RX", "@CK@r#", "@CKK"},
    {WhereFlag::arlia_orbit, "@mA@RX", "@mA@r#", "@mAA"},
    {WhereFlag::nebula, "@m&@RX", "@m&@r#", "@m&&"},
    {WhereFlag::asteroid, "@y:@RX", "@y:@r#", "@y::"},
    {WhereFlag::wormhole, "@b@1*@RX@n", "@b@1*@r#@n", "@b@1**@n"},
    {WhereFlag::space_station, "@DS@RX", "@DS@r#", "@DSS"},
    {WhereFlag::star, "@6 @RX@n", "@6 @r#@n", "@6  @n"},
};

char *getmapchar(int rnum, Character *ch, int start, int vnum)
{
    static char mapchar[50];
    bool there = (rnum == start);
    bool enemy = checkship(rnum, vnum);

    // Specific cases based on room vnum
    if (rnum == real_room(GET_RADAR1(ch)) || rnum == real_room(GET_RADAR2(ch)) || rnum == real_room(GET_RADAR3(ch)))
    {
        sprintf(mapchar, there ? "@WB@RX" : enemy ? "@WB@r#"
                                                  : "@WBB");
    }
    else if (rnum == 50772)
    {
        sprintf(mapchar, there ? "@cZ@RX" : enemy ? "@cZ@r#"
                                                  : "@cZZ");
    }
    else if (rnum == 38028)
    {
        sprintf(mapchar, there ? "@yQ@RX" : enemy ? "@yQ@r#"
                                                  : "@yQQ");
    }
    else
    {
        // Handle the room types
        bool handled = false;
        for (const auto &roomType : roomTypes)
        {
            if (WHERE_FLAGGED(rnum, roomType.flag))
            {
                sprintf(mapchar, "%s", getMapCharForRoomType(there, enemy, roomType));
                handled = true;
                break;
            }
        }

        if (!handled)
        {
            // Default case
            if (there)
            {
                sprintf(mapchar, "@w @RX");
            }
            else if (enemy)
            {
                sprintf(mapchar, "@w @r#");
            }
            else
            {
                int color = Random::get<int>(1, 30);
                int rnd = Random::get<int>(1, 40);
                if (rnd == 2)
                {
                    sprintf(mapchar, "%s. ", color > 15 ? "@w" : (color >= 7 ? "@Y" : (color > 3 ? "@R" : "@B")));
                }
                else if (rnd == 2)
                {
                    sprintf(mapchar, "%s .", color > 15 ? "@w" : (color >= 7 ? "@Y" : (color > 3 ? "@R" : "@B")));
                }
                else
                {
                    sprintf(mapchar, "@w  ");
                }
            }
        }
    }
    return mapchar;
}

MapStruct findcoord(int rnum)
{
    int x, y;
    MapStruct coords;

    coords.x = 0;
    coords.y = 0;

    for (y = 0; y <= MAP_ROWS; y++)
    {
        for (x = 0; x <= MAP_COLS; x++)
        {
            if (mapnums[y][x] == rnum)
            {
                coords.y = y;
                coords.x = x;
                return coords;
            }
        }
    }

    basic_mud_log("SYSERR: findcoord for non-map rnum");
    return coords;
}

void printmap(int rnum, Character *ch, int type, int vnum)
{
    int x = 0, lasty = -1;
    int y = 0;
    int sightradius;
    int count = 0, initline = 0;
    char buf[MAX_STRING_LENGTH * 2];
    char buf2[512];
    MapStruct coord;

    int start = rnum;

    coord = findcoord(rnum);
    strcpy(buf, "\n");
    if (type == 0)
    {
        sightradius = 12;
    }
    else
    {
        sightradius = 4;
    }

    if (type == 0)
    {
        ch->sendText("@b______________________________________________________________________@n\n");
    }

    for (y = coord.y - sightradius; y <= coord.y + sightradius; y++)
    {
        if (type == 0)
        {
            if (count == initline)
            {
                strcat(buf, "@b     [@CR. Key@b]     | ");
            }
            else if (count == initline + 1)
            {
                strcat(buf, "@GEE@D:@w Earth@b         | ");
            }
            else if (count == initline + 2)
            {
                strcat(buf, "@gNN@D:@w Namek@b         | ");
            }
            else if (count == initline + 3)
            {
                strcat(buf, "@YVV@D:@w Vegeta@b        | ");
            }
            else if (count == initline + 4)
            {
                strcat(buf, "@CFF@D:@w Frigid@b        | ");
            }
            else if (count == initline + 5)
            {
                strcat(buf, "@mKK@D:@w Konack@b        | ");
            }
            else if (count == initline + 6)
            {
                strcat(buf, "@BAA@D:@w Aether@b        | ");
            }
            else if (count == initline + 7)
            {
                strcat(buf, "@MYY@D:@w Yardrat@b       | ");
            }
            else if (count == initline + 8)
            {
                strcat(buf, "@CKK@D:@w Kanassa@b       | ");
            }
            else if (count == initline + 9)
            {
                strcat(buf, "@mAA@D:@w Arlia@b         | ");
            }
            else if (count == initline + 10)
            {
                strcat(buf, "@cZZ@D:@w Zenith@b        | ");
            }
            else if (count == initline + 11)
            {
                strcat(buf, "@MCC@D:@w Cerria@b        | ");
            }
            else if (count == initline + 12)
            {
                strcat(buf, "@WBB@D:@w Buoy@b          | ");
            }
            else if (count == initline + 13)
            {
                strcat(buf, "@m&&@D:@w Nebula@b        | ");
            }
            else if (count == initline + 14)
            {
                strcat(buf, "@yQQ@D:@w Asteroid@b      | ");
            }
            else if (count == initline + 15)
            {
                strcat(buf, "@y::@D:@w Asteroid Field@b| ");
            }
            else if (count == initline + 16)
            {
                strcat(buf, "@b@1**@n@D:@w Wormhole@b      | ");
            }
            else if (count == initline + 17)
            {
                strcat(buf, "@DSS@D:@w S. Station@b    | ");
            }
            else if (count == initline + 18)
            {
                strcat(buf, " @r#@D:@w Unknown Ship@b  | ");
            }
            else if (count == initline + 19)
            {
                strcat(buf, "@6  @n@D:@w Star@b          | ");
            }
            else
            {
                strcat(buf, "                  @b| ");
            }
            count++;
        }
        else
        {
            if (count == 0)
            {
                strcat(buf, "      @RCompass@n           ");
            }
            else if (count == 2)
            {
                sprintf(buf2, "@w       @w|%s@w|            ", (W_EXIT(rnum, 0) ? " @CN " : "   "));
                strcat(buf, buf2);
            }
            else if (count == 3)
            {
                sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|      ", (W_EXIT(rnum, 6) ? " @CNW" : "   "),
                        (W_EXIT(rnum, 4) ? " @YU " : "   "), (W_EXIT(rnum, 7) ? "@CNE " : "   "));
                strcat(buf, buf2);
            }
            else if (count == 4)
            {
                sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|      ", (W_EXIT(rnum, 3) ? "  @CW" : "   "),
                        (W_EXIT(rnum, 10) ? "@m I " : (W_EXIT(rnum, 11) ? "@mOUT" : "   ")),
                        (W_EXIT(rnum, 1) ? "@CE  " : "   "));
                strcat(buf, buf2);
            }
            else if (count == 5)
            {
                sprintf(buf2, "@w @w|%s@w| |%s@w| |%s@w|      ", (W_EXIT(rnum, 9) ? " @CSW" : "   "),
                        (W_EXIT(rnum, 5) ? " @YD " : "   "), (W_EXIT(rnum, 8) ? "@CSE " : "   "));
                strcat(buf, buf2);
            }
            else if (count == 6)
            {
                sprintf(buf2, "@w       @w|%s@w|            ", (W_EXIT(rnum, 2) ? " @CS " : "   "));
                strcat(buf, buf2);
            }
            else
            {
                strcat(buf, "                        ");
            }
            count++;
        }
        for (x = coord.x - sightradius; x <= coord.x + sightradius; x++)
        {
            if (x == coord.x && y == coord.y)
            {
                strcat(buf, getmapchar(mapnums[y][x], ch, start, vnum));
            }
            else if (x > MAP_COLS || x < 0)
            {
                if (lasty != true && y > -1 && y < 200)
                {
                    strcat(buf, "@D?");
                    lasty = true;
                }
            }
            else if (y > MAP_ROWS || y < 0)
            {
                if (y == -1 || y == 200)
                {
                    strcat(buf, "@D??");
                }
            }
            else
                strcat(buf, getmapchar(mapnums[y][x], ch, start, vnum));
        }
        strcat(buf, "\n");
        lasty = false;
    }

    ch->sendText(buf);
    *buf2 = '\0';
    *buf = '\0';
    if (type == 0)
    {
        ch->sendText("\n@b______________________________________________________________________@n");
    }
}
