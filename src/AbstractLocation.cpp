#include "dbat/structs.h"
#include <functional>
#include "dbat/structs.h"
#include "dbat/filter.h"
#include "dbat/utils.h"
#include "dbat/constants.h"
#include "dbat/fight.h"
#include "dbat/planet.h"
#include "dbat/act.informative.h"

std::vector<std::weak_ptr<Object>> AbstractLocation::getObjects()
{
    return getContents<Object>();
}

std::vector<std::weak_ptr<Character>> AbstractLocation::getPeople()
{
    return getContents<Character>();
}

std::vector<std::weak_ptr<Object>> AbstractLocation::getObjects(const Coordinates &coor)
{
    return getContents<Object>(coor);
}

std::vector<std::weak_ptr<Structure>> AbstractLocation::getStructures(const Coordinates &coor)
{
    return getContents<Structure>(coor);
}

std::vector<std::weak_ptr<Character>> AbstractLocation::getPeople(const Coordinates &coor)
{
    return getContents<Character>(coor);
}

void AbstractLocation::setRoomFlag(const Coordinates &coor, int flag, bool value)
{
    setRoomFlag(coor, static_cast<RoomFlag>(flag), value);
}

bool AbstractLocation::toggleRoomFlag(const Coordinates &coor, int flag)
{
    return toggleRoomFlag(coor, static_cast<RoomFlag>(flag));
}

bool AbstractLocation::getRoomFlag(const Coordinates &coor, int flag)
{
    return getRoomFlag(coor, static_cast<RoomFlag>(flag));
}

int AbstractLocation::getCookElement(const Coordinates &coor)
{
    int found = 0;
    auto con = getObjects(coor);
    for (auto obj : filter_raw(con))
    {
        if (GET_OBJ_TYPE(obj) == ITEM_CAMPFIRE)
        {
            found = 1;
        }
        else if (obj->getVnum() == 19093)
            return 2;
    }

    return found;
}

const std::vector<ExtraDescription> &AbstractLocation::getExtraDescription(const Coordinates &coor) const
{
    static std::vector<ExtraDescription> extraDescriptions;
    return extraDescriptions;
}

bool AbstractLocation::getIsDark(const Coordinates &coor)
{
    return false; // temporarily disabled.

    auto pe = getPeople(coor);
    for (auto c : filter_raw(pe))
    {
        if (c->isProvidingLight())
            return false;
    }

    if (getCookElement(coor))
        return false;

    if (getRoomFlag(coor, ROOM_NOINSTANT) && getRoomFlag(coor, ROOM_DARK))
    {
        return true;
    }
    if (getRoomFlag(coor, ROOM_NOINSTANT) && !getRoomFlag(coor, ROOM_DARK))
    {
        return false;
    }

    if (getRoomFlag(coor, ROOM_DARK))
        return true;

    if (getRoomFlag(coor, ROOM_INDOORS))
        return false;

    const auto tile = static_cast<int>(getSectorType(coor));

    if (tile == SECT_INSIDE || tile == SECT_CITY || tile == SECT_IMPORTANT || tile == SECT_SHOP)
        return false;

    if (tile == SECT_SPACE)
        return false;

    if (weather_info.sunlight == SUN_SET)
        return true;

    if (weather_info.sunlight == SUN_DARK)
        return true;

    return false;
}

void AbstractLocation::replaceExit(const Coordinates &coor, const Destination &dest)
{
    // Implementation for replacing an exit in the location data
}

void AbstractLocation::deleteExit(const Coordinates &coor, Direction dir)
{
    // Implementation for deleting an exit in the location data
}


void AbstractLocation::addToContents(const Coordinates &coor, const std::shared_ptr<HasLocation> &hl)
{
    contents.add(hl);
    hl->location.al = getSharedAbstractLocation();
    hl->location.position = coor;
    onAddToContents(coor, hl);
}

void AbstractLocation::onAddToContents(const Coordinates& coor, const std::shared_ptr<HasLocation>& hl) {

}


void AbstractLocation::removeFromContents(const std::shared_ptr<HasLocation> &hl)
{
    contents.remove(hl);
    hl->location = {};
    onRemoveFromContents(hl);
}

void AbstractLocation::onRemoveFromContents(const std::shared_ptr<HasLocation>& hl) {
    
}

void AbstractLocation::setRoomFlag(const Coordinates &coor, RoomFlag flag, bool value)
{
    getRoomFlags(coor).set(flag, value);
}

bool AbstractLocation::toggleRoomFlag(const Coordinates &coor, RoomFlag flag)
{
    return getRoomFlags(coor).toggle(flag);
}

bool AbstractLocation::getRoomFlag(const Coordinates &coor, RoomFlag flag)
{
    return getRoomFlags(coor).get(flag);
}

void AbstractLocation::setWhereFlag(const Coordinates &coor, WhereFlag flag, bool value)
{
    getWhereFlags(coor).set(flag, value);
}

bool AbstractLocation::toggleWhereFlag(const Coordinates &coor, WhereFlag flag)
{
    return getWhereFlags(coor).toggle(flag);
}

bool AbstractLocation::getWhereFlag(const Coordinates &coor, WhereFlag flag)
{
    return getWhereFlags(coor).get(flag);
}

static void display_room_flags(const Coordinates& coor, AbstractLocation *rm, Character *ch)
{
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];

    sprintf(buf, "%s", rm->getRoomFlags(coor).getFlagNames().c_str());
    sprinttype(static_cast<int>(rm->getSectorType(coor)), sector_types, buf2, sizeof(buf2));

    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
    {
        ch->sendText("\r\n@wO----------------------------------------------------------------------O@n\r\n");
    }

    ch->send_to("@wLocation: @G%-70s@w\r\n", rm->getName(coor));

    if(auto r = dynamic_cast<Room*>(rm))
    {
        if (auto sc = r->getScripts(); !sc.empty())
        {
            ch->sendText("@D[@GTriggers");
            for (auto t : filter_shared(sc))
                ch->send_to(" %d", t->getVnum());
            ch->sendText("@D] ");
        }
    }
    

    double grav = rm->getEnvironment(coor, ENV_GRAVITY);
    auto g = fmt::format("{}", grav);
    snprintf(buf3, sizeof(buf3), "@D[ @G%s@D] @wSector: @D[ @G%s @D] @wVnum: @D[@G%5d@D]@n Gravity: @D[@G%sx@D]@n", buf, buf2, rm->getLocVnum(), g.c_str());
    ch->send_to("@wFlags: %-70s@w\r\n", buf3);

    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
    {
        ch->sendText("@wO----------------------------------------------------------------------O@n\r\n");
    }
}

static void display_special_room_descriptions(const Coordinates& coor, AbstractLocation *rm, Character *ch)
{
    auto &rf = rm->getRoomFlags(coor);

    if (rf.get(ROOM_REGEN))
    {
        ch->sendText("@CA feeling of calm and relaxation fills this room.@n\r\n");
    }
    if (rf.get(ROOM_AURA))
    {
        ch->sendText("@GAn aura of @gregeneration@G surrounds this area.@n\r\n");
    }
    if (rm->getWhereFlag(coor, WhereFlag::hyperbolic_time_chamber))
    {
        ch->sendText("@rThis room feels like it operates in a different time frame.@n\r\n");
    }
}

static void display_dimension_info(const Coordinates& coor, AbstractLocation *rm, Character *ch)
{
    if (rm->getWhereFlag(coor, WhereFlag::neo_nirvana))
    {
        ch->sendText("@wPlanet: @WNeo Nirvana@n\r\n");
    }
    else if (rm->getWhereFlag(coor, WhereFlag::afterlife))
    {
        ch->sendText("@wDimension: @yA@Yf@yt@Ye@yr@Yl@yi@Yf@ye@n\r\n");
    }
    else if (rm->getRoomFlag(coor, ROOM_HELL))
    {
        ch->sendText("@wDimension: @RPunishment Hell@n\r\n");
    }
    else if (rm->getWhereFlag(coor, WhereFlag::afterlife_hell))
    {
        ch->sendText("@wDimension: @RH@re@Dl@Rl@n\r\n");
    }
}

static void display_room_info(const Coordinates& coor, AbstractLocation *rm, Character *ch)
{
    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
    {
        ch->sendText("@wO----------------------------------------------------------------------O@n\r\n");
    }

    ch->send_to("@wLocation: %-70s@n\r\n", rm->getName(coor));

    if (auto planet = getPlanet(rm->getLocVnum()); planet)
    {
        ch->send_to("@wPlanet: @G%s@n\r\n", getPlanetColorName(planet.value()).c_str());
    }
    else
    {
        display_dimension_info(coor, rm, ch);
    }

    double grav = rm->getEnvironment(coor, ENV_GRAVITY);
    if (grav <= 1.0)
    {
        ch->sendText("@wGravity: @WNormal@n\r\n");
    }
    else
    {
        auto g = fmt::format("{}", grav);
        ch->send_to("@wGravity: @W%sx@n\r\n", g.c_str());
    }

    display_special_room_descriptions(coor, rm, ch);

    if (!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_NODEC))
    {
        ch->sendText("@wO----------------------------------------------------------------------O@n\r\n");
    }
}

static void display_damage_description(Character *ch, int dmg, const char *surface)
{
    if (dmg <= 2)
    {
        ch->send_to("@wA small hole with chunks of debris that can be seen scarring the %s.@n", surface);
    }
    else if (dmg <= 4)
    {
        ch->send_to("@wA couple small holes with chunks of debris that can be seen scarring the %s.@n", surface);
    }
    else if (dmg <= 6)
    {
        ch->send_to("@wA few small holes with chunks of debris that can be seen scarring the %s.@n", surface);
    }
    else if (dmg <= 10)
    {
        ch->send_to("@wThere are several small holes with chunks of debris that can be seen scarring the %s.@n", surface);
    }
    else if (dmg <= 20)
    {
        ch->send_to("@wMany holes fill the %s of this area, many of which have burn marks.@n", surface);
    }
    else if (dmg <= 30)
    {
        ch->send_to("@wThe %s is severely damaged with many large holes.@n", surface);
    }
    else if (dmg <= 50)
    {
        ch->sendText("@wBattle damage covers the entire area. Displayed as a tribute to the battles that have been waged here.@n");
    }
    else if (dmg <= 75)
    {
        ch->sendText("@wThis entire area is falling apart, it has been damaged so badly.@n");
    }
    else if (dmg <= 99)
    {
        ch->sendText("@wThis area cannot withstand much more damage. Everything has been damaged so badly it is hard to recognize any particular details about their former quality.@n");
    }
    else if (dmg >= 100)
    {
        ch->sendText("@wThis area is completely destroyed. Nothing is recognizable. Chunks of debris litter the ground, filling up holes, and overflowing onto what is left of the ground. A haze of smoke is wafting through the air, creating a chilling atmosphere.@n");
    }
}

static void display_damage_description_forest(Character *ch, int dmg)
{
    if (dmg <= 2)
    {
        ch->sendText("@wA small tree sits in a little crater here.@n");
    }
    else if (dmg <= 4)
    {
        ch->sendText("@wTrees have been uprooted by craters in the ground.@n");
    }
    else if (dmg <= 6)
    {
        ch->sendText("@wSeveral trees have been reduced to chunks of debris and are laying in a few craters here.@n");
    }
    else if (dmg <= 10)
    {
        ch->sendText("@wA large patch of trees have been destroyed and are laying in craters here.@n");
    }
    else if (dmg <= 20)
    {
        ch->sendText("@wSeveral craters have merged into one large crater in one part of this forest.@n");
    }
    else if (dmg <= 30)
    {
        ch->sendText("@wThe open sky can easily be seen through a hole of trees destroyed and resting at the bottom of several craters here.@n");
    }
    else if (dmg <= 50)
    {
        ch->sendText("@wA good deal of burning tree pieces can be found strewn across the cratered ground here.@n");
    }
    else if (dmg <= 75)
    {
        ch->sendText("@wVery few trees are left standing in this area, replaced instead by large craters.@n");
    }
    else if (dmg <= 99)
    {
        ch->sendText("@wSingle solitary trees can be found still standing here or there in the area. The rest have been almost completely obliterated in recent conflicts.@n");
    }
    else if (dmg >= 100)
    {
        ch->sendText("@wOne massive crater fills this area. This desolate crater leaves no evidence of what used to be found in the area. Smoke slowly wafts into the sky from the central point of the crater, creating an oppressive atmosphere.@n");
    }
}

static void display_damage_description_mountain(Character *ch, int dmg)
{
    if (dmg <= 2)
    {
        ch->sendText("@wA small crater has been burned into the side of this mountain.@n");
    }
    else if (dmg <= 4)
    {
        ch->sendText("@wA couple craters have been burned into the side of this mountain.@n");
    }
    else if (dmg <= 6)
    {
        ch->sendText("@wBurned bits of boulders can be seen lying at the bottom of a few nearby craters.@n");
    }
    else if (dmg <= 10)
    {
        ch->sendText("@wSeveral bad craters can be seen in the side of the mountain here.@n");
    }
    else if (dmg <= 20)
    {
        ch->sendText("@wLarge boulders have rolled down the mountainside and collected in many nearby craters.@n");
    }
    else if (dmg <= 30)
    {
        ch->sendText("@wMany craters are covering the mountainside here.@n");
    }
    else if (dmg <= 50)
    {
        ch->sendText("@wThe mountain side has partially collapsed, shedding rubble down towards its base.@n");
    }
    else if (dmg <= 75)
    {
        ch->sendText("@wA peak of the mountain has been blown off, leaving behind a smoldering tip.@n");
    }
    else if (dmg <= 99)
    {
        ch->sendText("@wThe mountainside here has completely collapsed, shedding dangerous rubble down to its base.@n");
    }
    else if (dmg >= 100)
    {
        ch->sendText("@wHalf the mountain has been blown away, leaving a scarred and jagged rock in its place. Billowing smoke wafts up from several parts of the mountain, filling the nearby skies and blotting out the sun.@n");
    }
}

static void display_room_damage_description(const Coordinates& coor, AbstractLocation *rm, Character *ch)
{
    auto dmg = rm->getDamage(coor);
    auto sect = static_cast<int>(rm->getSectorType(coor));
    auto sunk = rm->getEnvironment(coor, ENV_WATER) >= 100.0;

    if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || rm->getRoomFlag(coor, ROOM_DEATH))
    {
        if (dmg <= 99 || (dmg == 100 && (sect == SECT_WATER_SWIM || sunk || sect == SECT_FLYING || sect == SECT_SHOP || sect == SECT_IMPORTANT)))
        {
            ch->send_to("@w%s@n", rm->getLookDescription(coor));
        }

        if (dmg > 0)
        {
            ch->sendText("\r\n");
            switch (sect)
            {
            case SECT_INSIDE:
                display_damage_description(ch, dmg, "floor");
                break;
            case SECT_CITY:
            case SECT_FIELD:
            case SECT_HILLS:
            case SECT_IMPORTANT:
                display_damage_description(ch, dmg, "ground");
                break;
            case SECT_FOREST:
                display_damage_description_forest(ch, dmg);
                break;
            case SECT_MOUNTAIN:
                display_damage_description_mountain(ch, dmg);
                break;
            default:
                break;
            }
            ch->sendText("\r\n");
        }

        auto ge = rm->getGroundEffect(coor);

        if (ge >= 1 && ge <= 5)
        {
            ch->sendText("@rLava@w is pooling in some places here...@n\r\n");
        }
        else if (ge >= 6)
        {
            ch->sendText("@RLava@r covers pretty much the entire area!@n\r\n");
        }
        else if (ge < 0)
        {
            ch->sendText("@cThe entire area is flooded with a @Cmystical@c cube of @Bwater!@n\r\n");
        }
    }
}

static void display_garden_info(const Coordinates& coor, AbstractLocation *rm, Character *ch)
{
    auto con = rm->getObjects(coor);
    auto &rf = rm->getRoomFlags(coor);
    if (rf.get(ROOM_GARDEN1))
    {
        ch->send_to("@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R8@D]@n\r\n", con.size());
    }
    else if (rf.get(ROOM_GARDEN2))
    {
        ch->send_to("@D[@GPlants Planted@D: @g%d@W, @GMAX@D: @R20@D]@n\r\n", con.size());
    }
    else if (rf.get(ROOM_HOUSE))
    {
        ch->send_to("@D[@GItems Stored@D: @g%d@D]@n\r\n", con.size());
    }
}

void AbstractLocation::displayLookFor(const Coordinates& coor, Character* ch) {
    if (!ch->desc)
        return;
    
    if (AFF_FLAGGED(ch, AFF_BLIND))
    {
        ch->sendText("You see nothing but infinite darkness...\r\n");
        return;
    }

    if (PLR_FLAGGED(ch, PLR_EYEC))
    {
        ch->sendText("You can't see a damned thing, your eyes are closed!\r\n");
        return;
    }

    if (PRF_FLAGGED(ch, PRF_ROOMFLAGS))
    {
        display_room_flags(coor, this, ch);
    }
    else
    {
        display_room_info(coor, this, ch);
    }

    display_room_damage_description(coor, this, ch);

    /* autoexits */
    if (!IS_NPC(ch))
    {
        Location loc;
        loc.position = coor;
        loc.al = getSharedAbstractLocation();
        if (PRF_FLAGGED(ch, PRF_NODEC))
        {
            do_auto_exits2(loc, ch);
        }
        else
        {
            do_auto_exits(loc, ch, EXIT_LEV(ch));
        }
    }

    display_garden_info(coor, this, ch);

    // retrieve all entities at these coordinates.
    auto con = getContents<HasLocation>(coor);

    std::map<std::string, std::vector<std::shared_ptr<HasLocation>>> categorized_contents;
    for (const auto& hl : filter_shared(con)) {
        // Filter by visibility...
        if(hl->getLocationVisibleTo(ch))
            categorized_contents[hl->getLocationDisplayCategory(ch)].emplace_back(hl);
    }

    for(auto& [cat, entities] : categorized_contents) {
        // it's impossible for entities to be empty here...
        ch->sendFmt("    {}:\r\n", cat);
        for(auto& ent : entities) {
            ent->displayLocationInfo(ch);
        }
    }
}