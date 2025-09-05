#include "dbat/AbstractLocation.h"
#include "dbat/filter.h"
#include "dbat/ObjectUtils.h"
#include "dbat/Character.h"
#include "dbat/Structure.h"
#include "dbat/weather.h"

WeakBag<Object> AbstractLocation::getObjects()
{
    return getContents<Object>();
}

WeakBag<Character> AbstractLocation::getPeople()
{
    return getContents<Character>();
}

WeakBag<Object> AbstractLocation::getObjects(const Coordinates &coor)
{
    return getContents<Object>(coor);
}

WeakBag<Structure> AbstractLocation::getStructures(const Coordinates &coor)
{
    return getContents<Structure>(coor);
}

WeakBag<Character> AbstractLocation::getPeople(const Coordinates &coor)
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
    auto con = getObjects(coor).snapshot_weak();
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

std::optional<double> AbstractLocation::getEnvironment(const Coordinates &coor, int type) const
{
    return std::nullopt;
}

ExtraDescriptionViews AbstractLocation::getExtraDescription(const Coordinates &coor) const
{
    static ExtraDescriptionViews extraDescriptions;
    return extraDescriptions;
}

bool AbstractLocation::getIsDark(const Coordinates &coor)
{
    return false; // temporarily disabled.

    auto pe = getPeople(coor).snapshot_weak();
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
    onAddToContents(coor, hl);
}

void AbstractLocation::onAddToContents(const Coordinates& coor, const std::shared_ptr<HasLocation>& hl) {

}


void AbstractLocation::removeFromContents(const std::shared_ptr<HasLocation> &hl)
{
    contents.remove(hl);
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

std::optional<std::string> AbstractLocation::getTileDisplayOverride(const Coordinates &coor) const
{
    return std::nullopt;
}


bool AbstractLocation::validCoordinates(const Coordinates& coor) const {
    return true;
}

bool AbstractLocation::buildwalk(const Coordinates& coor, Character* ch, Direction dir) {
    return false;
}