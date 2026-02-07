#include <variant>
#include <ranges>

#include "dbat/game/CharacterUtils.hpp"
#include "dbat/game/RoomUtils.hpp"
#include "dbat/game/ObjectUtils.hpp"
#include "dbat/game/utils.hpp"
#include "dbat/game/Zone.hpp"
#include "dbat/game/Parse.hpp"
#include "dbat/game/ID.hpp"
#include "dbat/game/interpreter.hpp"
#include "volcano/util/Enum.hpp"
#include "dbat/game/Flags.hpp"

#include "dbat/game/const/WearSlot.hpp"


ACMD(do_mush_foreach) {
    if (cdata.lstrim.empty() || cdata.rstrim.empty()) {
        ch->sendText("Usage: .foreach <list>=<command with {} placeholder>\r\n");
        return;
    }
    std::vector<std::string_view> items;
    boost::split(items, cdata.lstrim, boost::is_space(), boost::token_compress_on);

    for(auto item : items) {
        if(item.empty()) continue;
        std::string cmdline;
        try {
            cmdline = fmt::format(fmt::runtime(cdata.rstrim), item);
        } catch (const std::exception& e) {
            ch->sendText(fmt::format("Error processing item '{}': {}\r\n", item, e.what()));
        }
        command_interpreter(ch, cmdline.data());
    }
}

enum class ZoneOp
{
    Create,
    Delete,
    Rename,
    Desc,
    Reset,
    Flags,
    Parent,
    List,
    Help,
    AddRooms,
    Examine,
    Launch,
    Dock,
    Land
};

static const std::unordered_map<std::string, ZoneOp> kOps{
    {"create", ZoneOp::Create},
    {"delete", ZoneOp::Delete},
    {"rename", ZoneOp::Rename},
    {"reset", ZoneOp::Reset},
    {"flags", ZoneOp::Flags},
    {"parent", ZoneOp::Parent},
    {"list", ZoneOp::List},
    {"help", ZoneOp::Help},
    {"addrooms", ZoneOp::AddRooms},
    {"examine", ZoneOp::Examine},
    {"launch", ZoneOp::Launch},
    {"dock", ZoneOp::Dock},
    {"land", ZoneOp::Land},
};

constexpr std::string_view mushZoneHelp = R"(
MUSH-style Zone Editor Commands:
Alias: .z

.zone <id>
    Examine a zone.

.zone/create <name>[=<parent ID>] 
    Create a new zone with optional parent.

.zone/delete <id>=YES
    Delete a zone. It must be totally empty.

.zone/rename <id>=<new>
    Rename a zone.

.zone/reset <id>
    Reset a zone.

.zone/flags <id>=[[+|-]flag...]
    Set zone flags. Example: .zone/flags 50=+dark -cave
    See .choices/ZoneFlag

.zone/parent <id>=<new parent id> | NONE
    Set or clear zone parent.

.zone/list
    List all zones.

.zone/help
    Show this help.

.zone/addrooms [<number>|<from>-<to>]...=<id>
    Add rooms to a zone. example: .zone/addrooms 20 21 99-120=50

See .location/help for information on LocationIDs.

Launch, dock, and land are implicitly inherited by descendants, as the commands
look up the ancestor chain to find the first with a valid entry.

.zone/launch <id>=<LocationID> | NONE
   This is used by "fly space" and "pilot launch".

.zone/dock <id>=<name>,<LocationID> | NONE
   Set or remove a named landing spot for ships.
   <name> is CASE SENSITIVE when setting/clearing.

.zone/land <id>=<name>,<LocationID> | NONE
   Set or remove a named landing spot for characters.
   <name> is CASE SENSITIVE when setting/clearing.

Note: For this to work, the launch destination needs to be a member of
that particular zone. Otherwise, the dock and land locations won't be
properly detected by the commands.

)";

Result<ObjectPrototype*> getObjProto(obj_vnum vnum) {
    auto found = obj_proto.find(vnum);
    if(found == obj_proto.end()) {
        return err("Obj Proto not found.");
    }
    return found->second.get();
}

Result<ObjectPrototype*> getObjProto(std::string_view arg) {
    auto numRes = parseNumber<obj_vnum>(arg, "Obj Vnum");
    if(!numRes) {
        return err(numRes.error());
    }
    return getObjProto(numRes.value());
}

Result<CharacterPrototype*> getMobProto(mob_vnum vnum) {
    auto found = mob_proto.find(vnum);
    if(found == mob_proto.end()) {
        return err("Mob Proto not found.");
    }
    return found->second.get();
}

Result<CharacterPrototype*> getMobProto(std::string_view arg) {
    auto numRes = parseNumber<mob_vnum>(arg, "Mob Vnum");
    if(!numRes) {
        return err(numRes.error());
    }
    return getMobProto(numRes.value());
}

ACMD(do_mush_zone)
{

    std::string_view op = cdata.switches.empty() ? "examine" : cdata.switches[0];

    auto oper = volcano::util::partialMatch(op, kOps);

    if (!oper)
    {
        ch->sendFmt(oper.error());
        return;
    }

    auto operation = oper.value()->second;
    switch (operation)
    {
    case ZoneOp::Create:
    {
        auto res = validateZoneName(cdata.lstrim);
        if (!res)
        {
            ch->sendText(res.error());
            return;
        }
        Zone *parent = nullptr;
        if (!cdata.rstrim.empty())
        {
            auto parentRes = getZone(cdata.rstrim, ch);
            if (!parentRes)
            {
                ch->sendText(parentRes.error());
                return;
            }
            parent = parentRes.value();
        }
        auto newid = getNextID(lastZoneID, zone_table);
        auto z = std::make_shared<Zone>();
        zone_table.emplace(newid, z);
        z->name = res.value();
        z->number = newid;
        if (parent)
        {
            z->parent = parent->number;
            parent->children.insert(newid);
            ch->sendFmt("{} created. Its parent is {}'\r\n", *z, *parent);
        }
        else
        {
            ch->sendFmt("{} created. It has no parent.\r\n", *z);
        }
        return;
    }
    case ZoneOp::Delete:
    {
        auto zRes = getZone(cdata.lstrim, ch);
        if (!zRes)
        {
            ch->sendText(zRes.error());
            return;
        }
        auto z = zRes.value();
        auto zCan = z->canBeDeletedBy(ch);
        if (!zCan)
        {
            ch->sendText(zCan.error());
            return;
        }
        // TODO: finish sanitizing zone deletion.
        // zone_table.erase(z->number);
        // ch->sendFmt("Zone {} '{}' deleted.\r\n", z->number, z->name);
        return;
    }
    case ZoneOp::Rename:
    {
        auto res = getZone(cdata.lstrim, ch);
        if (!res)
        {
            ch->sendText(res.error());
            return;
        }
        auto z = res.value();
        auto oldname = z->name;
        auto nameRes = validateZoneName(cdata.rstrim);
        if (!nameRes)
        {
            ch->sendText(nameRes.error());
            return;
        }
        z->name = nameRes.value();
        ch->sendFmt("Renamed, now {}. Old name was: {}\r\n", *z, oldname);
        return;
    }
    case ZoneOp::Reset:
    {
        auto res = getZone(cdata.lstrim, ch);
        if (!res)
        {
            ch->sendText(res.error());
            return;
        }
        auto z = res.value();
        z->reset();
        ch->sendFmt("{} reset.\r\n", *z);
        return;
    }
    case ZoneOp::Flags:
    {
        auto res = getZone(cdata.lstrim, ch);
        if (!res)
        {
            ch->sendText(res.error());
            return;
        }
        auto fres = handleFlagOps<ZoneFlag>(res.value()->zone_flags, cdata.rstrim, "Zone Flags");
        if (!fres)
        {
            ch->sendText(fres.error());
            return;
        }
        ch->sendFmt("For {}: {}", *res.value(), fres.value());
    }
    case ZoneOp::List:
        list_zones(ch);
        return;
    case ZoneOp::Examine:
    {
        auto res = getZone(cdata.lstrim, ch);
        if (!res)
        {
            ch->sendText(res.error());
            return;
        }
        auto z = res.value();
        print_zone(ch, z->number);
        return;
    }
    case ZoneOp::Parent:
    {
        auto res = getZone(cdata.lstrim, ch);
        if (!res)
        {
            ch->sendText(res.error());
            return;
        }
        auto z = res.value();
        if (cdata.rstrim.empty())
        {
            ch->sendText("Set parent to what? Use NONE to clear.");
            return;
        }
        if (boost::iequals(cdata.rstrim, "NONE"))
        {
            auto par = z->getParent();
            if (!par)
            {
                ch->sendText("This zone has no parent.\r\n");
                return;
            }
            par->children.erase(z->number);
            z->parent = NOTHING;
            ch->sendFmt("{} parent cleared.", *z);
            return;
        }
        auto parentRes = getZone(cdata.rstrim, ch);
        if (!parentRes)
        {
            ch->sendText(parentRes.error());
            return;
        }
        auto parent = parentRes.value();
        if(auto par = z->getParent())
        {
            par->children.erase(z->number);
            z->parent = NOTHING;
        }
        z->parent = parent->number;
        parent->children.insert(z->number);
        ch->sendFmt("{} parent set to: {}", *z, *parent);
        return;
    }
    case ZoneOp::AddRooms:
    {
        auto res = getZone(cdata.rstrim, ch);
        if (!res)
        {
            ch->sendText(res.error());
            return;
        }
        auto z = res.value();
        // the lsargs should be a space-delimited sequence of numbers >0...
        auto ranges = parseRanges<room_vnum>(cdata.lstrim);
        if (!ranges)
        {
            ch->sendText(ranges.error());
            return;
        }
        bool addedAny = false;
        for (auto i : ranges.value())
        {
            auto r = get_room(i);
            if (!r)
            {
                ch->sendFmt("Room {} does not exist.\r\n", i);
                continue;
            }
            auto sh = r->shared_from_this();
            if (auto already_zone = sh->getZone(); already_zone)
            {
                already_zone->rooms.remove(sh);
            }
            z->rooms.add(sh);
            addedAny = true;
            sh->zone.reset(z);
            ch->sendFmt("Added {} to {}\r\n", *r, *z);
        }
        if(addedAny) {
            z->sortRooms();
        }
        return;
    }
    case ZoneOp::Help:
    {
        ch->sendText(mushZoneHelp);
        return;
    }
    case ZoneOp::Launch:
    {
        auto res = getZone(cdata.lstrim, ch);
        if (!res)
        {
            ch->sendText(res.error());
            return;
        }
        auto z = res.value();
        if (cdata.rstrim.empty())
        {
            if (!z->launchDestination.empty())
            {
                ch->sendFmt("Current launch location: {}\r\n", z->launchDestination);
            }
            else
            {
                ch->sendText("This zone has no launch location.\r\n");
            }
            return;
        }
        if(boost::iequals(cdata.rstrim, "NONE"))
        {
            z->launchDestination.clear();
            ch->sendFmt("{} Launch location cleared.\r\n", *z);
            return;
        }
        auto locRes = getLocation(cdata.rstrim, ch);
        if (!locRes)
        {
            ch->sendText(locRes.error());
            return;
        }
        auto loc = locRes.value();
        z->launchDestination = loc.getLocID();
        ch->sendFmt("{} Launch location set to: {}\r\n", *z, loc);
        return;
    }
    case ZoneOp::Land: {
        // this and Dock work very similarly to Launch, except they're targeting a
        // std::unordered_map<std::string, std::string> landingSpots, dockingSpots
        auto res = getZone(cdata.lstrim, ch);
        if (!res)
        {
            ch->sendText(res.error());
            return;
        }
        auto z = res.value();
        if (cdata.rstrim.empty())
        {
            if (!z->landingSpots.empty())
            {
                ch->sendFmt("Current landing spots:\r\n");
                for (const auto &[dir, loc] : z->landingSpots)
                {
                    auto l = Location(loc);
                    ch->sendFmt("  {}: {}\r\n", dir, l);
                }
            }
            else
            {
                ch->sendText("This zone has no landing spots.\r\n");
            }
            return;
        }
        // The rsargs should be a <target>,<LocationID> | NONE
        std::vector<std::string> split;
        boost::split(split, cdata.rstrim, boost::is_any_of(","));
        for(auto &s : split)
        {
            boost::trim(s);
        }
        if(split.size() != 2) {
            ch->sendText("Invalid format. Use <target>,<LocationID> or NONE.\r\n");
            return;
        }
        if(boost::iequals(split[1], "NONE"))
        {
            z->landingSpots.erase(split[0]);
            ch->sendFmt("{} Landing spot {} cleared.\r\n", *z, split[0]);
            return;
        }
        auto locRes = getLocation(split[1], ch);
        if (!locRes)
        {
            ch->sendText(locRes.error());
            return;
        }
        auto loc = locRes.value();
        z->landingSpots[split[0]] = loc.getLocID();
        ch->sendFmt("{} Landing spot {} set to: {}\r\n", *z, split[0], loc);
        return;
    }
    case ZoneOp::Dock: {
        auto res = getZone(cdata.lstrim, ch);
        if (!res)
        {
            ch->sendText(res.error());
            return;
        }
        auto z = res.value();
        if (cdata.rstrim.empty())
        {
            if (!z->dockingSpots.empty())
            {
                ch->sendFmt("Current docking spots:\r\n");
                for (const auto &[dir, loc] : z->dockingSpots)
                {
                    auto l = Location(loc);
                    ch->sendFmt("  {}: {}\r\n", dir, l);
                }
            }
            else
            {
                ch->sendText("This zone has no docking spots.\r\n");
            }
            return;
        }
        // The rsargs should be a <target>,<LocationID> | NONE
        std::vector<std::string> split;
        boost::split(split, cdata.rstrim, boost::is_any_of(","));
        for(auto &s : split)
        {
            boost::trim(s);
        }
        if(split.size() != 2) {
            ch->sendText("Invalid format. Use <target>,<LocationID> or NONE.\r\n");
            return;
        }
        if(boost::iequals(split[1], "NONE"))
        {
            z->dockingSpots.erase(split[0]);
            ch->sendFmt("{} Docking spot {} cleared.\r\n", *z, split[0]);
            return;
        }
        auto locRes = getLocation(split[1], ch);
        if (!locRes)
        {
            ch->sendText(locRes.error());
            return;
        }
        auto loc = locRes.value();
        z->dockingSpots[split[0]] = loc.getLocID();
        ch->sendFmt("{} Docking spot {} set to: {}\r\n", *z, split[0], loc);
        return;
    }
    default:
        ch->sendText("Oops?!\r\n");
        break;
    }
}

constexpr std::string_view mushExitsHelp = R"(
MUSH-style Exits Editor
=============================================================================
The following exit directions are available:
- north, east, south, west, up, down, northeast, southeast, southwest, 
- northwest, inside, outside

This command always targets the exits in your current location.

See .location/help for information on LocationIDs.

It is usually more efficient to use buildwalk to create lots of connected
rooms. This command is for performing specific edits like setting keys.

It is not recommended to override auto-generated exits in grid areas that
simply connect coordinates. It could become very confusing.

Alias: .ex

.exit
    Display exits in current location.
    Automatically generated exits in grid areas will be marked out.

.exit/destination <direction>=<LocationID>
    Create/open or re-link an exit.
    In a grid area this will create an exit override that can lead anywhere, 
    but this is best used only on edges. The automapper will become very
    confused otherwise if default bounds are in play.

.exit/key <direction>=<key vnum>
   The object vnum that'll be used as a key. 
   Set to NONE or -1 to clear.

.exit/dclock <direction>=<difficulty>
    The difficulty number for picking the lock. 0 by default.

.exit/dchide <direction>=<difficulty>
    How easy it is to search for the exit if hidden. 0 By default.

.exit/flags <direction>=<flagset>...
    Choices: See .choices/ExitFlag
    Flagset can look like: +isdoor +closed -secret

.exit/clear <direction>
    Delete/close/wipe an exit.
    This won't do anything in a grid area where you're in default 
    bounds without overrides.

.exit/help
    Display this help text.

)";

// TODO: Replace ExitInfo with ExitFlags.

enum class ExitOp : uint8_t
{
    List,
    Destination,
    Key,
    DCLock,
    DCHide,
    Flags,
    Clear,
    Help
};

static class std::unordered_map<std::string, ExitOp> kExitOps{
    {"list", ExitOp::List},
    {"destination", ExitOp::Destination},
    {"key", ExitOp::Key},
    {"dclock", ExitOp::DCLock},
    {"dchide", ExitOp::DCHide},
    {"flags", ExitOp::Flags},
    {"clear", ExitOp::Clear},
    {"help", ExitOp::Help}};

ACMD(do_mush_exit)
{
    std::string_view op = cdata.switches.empty() ? "list" : cdata.switches[0];

    auto oper = volcano::util::partialMatch(op, kExitOps);

    if (!oper)
    {
        ch->sendFmt(oper.error());
        return;
    }

    auto operation = oper.value()->second;

    switch (operation)
    {
    case ExitOp::Help:
        ch->sendText(mushExitsHelp);
        return;
    case ExitOp::List:
    {
        ch->sendText("Exits:\r\n");
        for (auto &[d, e] : ch->location.getExits())
        {
            ch->sendFmt("{}\r\n", e);
        }
        return;
    }
    case ExitOp::Clear:
    {
        auto dirRes = volcano::util::chooseEnum<Direction>(cdata.lstrim, "Direction");
        if (!dirRes)
        {
            ch->sendText(dirRes.error());
            return;
        }
        auto dir = dirRes.value();
        auto ex = ch->location.getExit(dir);
        if (!ex)
        {
            ch->sendFmt("There is no {} exit.\r\n", dir);
            return;
        }
        if (ex->generated)
        {
            ch->sendFmt("You cannot clear the generated {} exit.\r\n", dir);
            return;
        }
        auto e = ex.value();
        ch->sendFmt("Clearing exit {}:\r\n", dir);
        ch->location.deleteExit(dir);
        return;
    }
    case ExitOp::Destination:
    {
        auto dirRes = volcano::util::chooseEnum<Direction>(cdata.lstrim, "Direction");
        if (!dirRes)
        {
            ch->sendText(dirRes.error());
            return;
        }
        auto dir = dirRes.value();

        auto ex = ch->location.getExit(dir);

        auto locRes = getLocation(cdata.rstrim, ch);
        if (!locRes)
        {
            ch->sendText(locRes.error());
            return;
        }
        if (!ex)
        {
            // if the exit doesn't exist, we'll emplace it.
            ex.emplace();
            ex->dir = dir;
        }
        auto &e = ex.value();
        e = locRes.value();
        ch->location.replaceExit(e);

        ch->sendFmt("{} Destination is now: {}\r\n", *ex, e);

        return;
    }
    case ExitOp::Key:
    {
        auto dirRes = volcano::util::chooseEnum<Direction>(cdata.lstrim, "Direction");
        if (!dirRes)
        {
            ch->sendText(dirRes.error());
            return;
        }
        auto dir = dirRes.value();

        auto ex = ch->location.getExit(dir);
        if (!ex)
        {
            ch->sendFmt("There is no {} exit.\r\n", dir);
            return;
        }
        if (boost::iequals(cdata.rstrim, "NONE") || cdata.rstrim == "-1")
        {
            ex->key = NOTHING;
            ch->sendFmt("{} Key set to: NOTHING.\r\n", *ex);
            return;
        }
        auto numRes = parseNumber<obj_vnum>(cdata.rstrim, "Object ID");
        if (!numRes)
        {
            ch->sendText(numRes.error());
            return;
        }
        auto ov = numRes.value();
        auto found = obj_proto.find(ov);
        if (found == obj_proto.end())
        {
            ch->sendFmt("Object ID {} not found.\r\n", ov);
            return;
        }
        ex->key = numRes.value();
        ch->sendFmt("{} Key set to: {}\r\n", *ex, *found->second);
        ch->location.replaceExit(*ex);
        return;
    }
    case ExitOp::Flags:
    {
        auto dirRes = volcano::util::chooseEnum<Direction>(cdata.lstrim, "Direction");
        if (!dirRes)
        {
            ch->sendText(dirRes.error());
            return;
        }
        auto dir = dirRes.value();

        auto ex = ch->location.getExit(dir);
        if (!ex)
        {
            ch->sendFmt("There is no {} exit.\r\n", dir);
            return;
        }
        auto res = handleFlagOps<ExitFlag>(ex->exit_flags, cdata.rstrim, "Exit Flags");
        if (!res)
        {
            ch->sendText(res.error());
            return;
        }
        ch->sendText(res.value());
        ch->location.replaceExit(*ex);
        return;
    }
    case ExitOp::DCLock:
    {
        auto dirRes = volcano::util::chooseEnum<Direction>(cdata.lstrim, "Direction");
        if (!dirRes)
        {
            ch->sendText(dirRes.error());
            return;
        }
        auto dir = dirRes.value();

        auto ex = ch->location.getExit(dir);
        if (!ex)
        {
            ch->sendFmt("There is no {} exit.\r\n", dir);
            return;
        }

        auto resNum = parseNumber(cdata.rstrim, "DC Lock");
        if (!resNum)
        {
            ch->sendText(resNum.error());
            return;
        }
        ex->dclock = resNum.value();
        ch->sendFmt("{} DC Lock set to: {}\r\n", *ex, ex->dclock);
        ch->location.replaceExit(*ex);
        return;
    }
    case ExitOp::DCHide:
    {
        auto dirRes = volcano::util::chooseEnum<Direction>(cdata.lstrim, "Direction");
        if (!dirRes)
        {
            ch->sendText(dirRes.error());
            return;
        }
        auto dir = dirRes.value();

        auto ex = ch->location.getExit(dir);
        if (!ex)
        {
            ch->sendFmt("There is no {} exit.\r\n", dir);
            return;
        }

        auto resNum = parseNumber(cdata.rstrim, "DC Hide");
        if (!resNum)
        {
            ch->sendText(resNum.error());
            return;
        }
        ex->dchide = resNum.value();
        ch->sendFmt("{} DC Hide set to: {}\r\n", *ex, ex->dchide);
        ch->location.replaceExit(*ex);
        return;
    }
    }
}

enum class ChoiceOp : uint8_t
{
    Sensei,
    Race,
    Form,
    WhereFlag,
    RoomFlag,
    ZoneFlag,
    ExitFlag,
    SectorType,
    Size,
    Sex,
    Appearance,
    CharacterFlag,
    PlayerFlag,
    MobFlag,
    PrefFlag,
    AffectFlag,
    ItemType,
    WearFlag,
    WearSlot,
    ItemFlag,
    AdminFlag,
    Direction,
};

ACMD(do_mush_choices)
{
    std::string_view op = cdata.switches.empty() ? "" : cdata.switches[0];
    if (op.empty())
    {
        ch->sendText("Usage: .choices/<type>\r\n");
        auto choices = volcano::util::getEnumNameList<ChoiceOp>();
        ch->sendFmt("Available Types: {}\r\n", fmt::join(choices, ", "));
        return;
    }

    auto oper = volcano::util::chooseEnum<ChoiceOp>(op, "Choice");

    if (!oper)
    {
        ch->sendFmt(oper.error());
        return;
    }

    auto choice = oper.value();

    switch (choice)
    {
    case ChoiceOp::Sensei:
        ch->sendFmt("Sensei Choices: {}", fmt::join(volcano::util::getEnumNameList<Sensei>(), ", "));
        return;
    case ChoiceOp::Race:
        ch->sendFmt("Race Choices: {}", fmt::join(volcano::util::getEnumNameList<Race>(), ", "));
        return;
    case ChoiceOp::Form:
        ch->sendFmt("Form Choices: {}", fmt::join(volcano::util::getEnumNameList<Form>(), ", "));
        return;
    case ChoiceOp::WhereFlag:
        ch->sendFmt("Where Flag Choices: {}", fmt::join(volcano::util::getEnumNameList<WhereFlag>(), ", "));
        return;
    case ChoiceOp::RoomFlag:
        ch->sendFmt("Room Flag Choices: {}", fmt::join(volcano::util::getEnumNameList<RoomFlag>(), ", "));
        return;
    case ChoiceOp::ZoneFlag:
        ch->sendFmt("Zone Flag Choices: {}", fmt::join(volcano::util::getEnumNameList<ZoneFlag>(), ", "));
        return;
    case ChoiceOp::ExitFlag:
        ch->sendFmt("Exit Flag Choices: {}", fmt::join(volcano::util::getEnumNameList<ExitFlag>(), ", "));
        return;
    case ChoiceOp::SectorType:
        ch->sendFmt("Sector Type Choices: {}", fmt::join(volcano::util::getEnumNameList<SectorType>(), ", "));
        return;
    case ChoiceOp::Size:
        ch->sendFmt("Size Choices: {}", fmt::join(volcano::util::getEnumNameList<Size>(), ", "));
        return;
    case ChoiceOp::Sex:
        ch->sendFmt("Sex Choices: {}", fmt::join(volcano::util::getEnumNameList<Sex>(), ", "));
        return;
    case ChoiceOp::Appearance:
        ch->sendFmt("Appearance Choices: {}", fmt::join(volcano::util::getEnumNameList<Appearance>(), ", "));
        return;
    case ChoiceOp::CharacterFlag:
        ch->sendFmt("Character Flag Choices: {}", fmt::join(volcano::util::getEnumNameList<CharacterFlag>(), ", "));
        return;
    case ChoiceOp::PlayerFlag:
        ch->sendFmt("Player Flag Choices: {}", fmt::join(volcano::util::getEnumNameList<PlayerFlag>(), ", "));
        return;
    case ChoiceOp::MobFlag:
        ch->sendFmt("Mob Flag Choices: {}", fmt::join(volcano::util::getEnumNameList<MobFlag>(), ", "));
        return;
    case ChoiceOp::PrefFlag:
        ch->sendFmt("Pref Flag Choices: {}", fmt::join(volcano::util::getEnumNameList<PrefFlag>(), ", "));
        return;
    case ChoiceOp::AffectFlag:
        ch->sendFmt("Affect Flag Choices: {}", fmt::join(volcano::util::getEnumNameList<AffectFlag>(), ", "));
        return;
    case ChoiceOp::ItemType:
        ch->sendFmt("Item Type Choices: {}", fmt::join(volcano::util::getEnumNameList<ItemType>(), ", "));
        return;
    case ChoiceOp::WearFlag:
        ch->sendFmt("Wear Flag Choices: {}", fmt::join(volcano::util::getEnumNameList<WearFlag>(), ", "));
        return;
    case ChoiceOp::WearSlot:
        ch->sendFmt("Wear Slot Choices: {}", fmt::join(volcano::util::getEnumNameList<WearSlot>(), ", "));
        return;
    case ChoiceOp::ItemFlag:
        ch->sendFmt("Item Flag Choices: {}", fmt::join(volcano::util::getEnumNameList<ItemFlag>(), ", "));
        return;
    case ChoiceOp::AdminFlag:
        ch->sendFmt("Admin Flag Choices: {}", fmt::join(volcano::util::getEnumNameList<AdminFlag>(), ", "));
        return;
    case ChoiceOp::Direction:
        ch->sendFmt("Direction Choices: {}", fmt::join(volcano::util::getEnumNameList<Direction>(), ", "));
        return;
    }
}

constexpr std::string_view mushLocationHelp = R"(
MUSH-style Location Editor
=============================================================================
Alias: .lo

A LocationID is a string representing the unique identifier for a location.
The first letter is the type (R = Room, A = Area, S = Structure)
It's followed by a : then the ID of that thing.
Non-Rooms use a coordinate system.
So, a Room might be R:50 and an Area location could be A:2:3:9:-2
Pure numbers like 50 default to Rooms.
A LocationID of "here" will default to your current location for convenience.

.location [<LocationID>]
    Display information about the given Location.
    Defaults to your current location if not provided.

.location/name <LocationID>=<new name> | NONE
    Set the Name for a Location. Or clear it.
    GridAreas with a cleared name fallback to the GridArea's name.

.location/description <LocationID>=<Description> | NONE
    Set the Look Description for the Location. Or clear it.
    use @@/ for a newline and @@- for a tab, all other colors work fine.
    GridAreas with a cleared description fallback to the GridArea's description.

.location/flags <LocationID>=<flags> | NONE
    Set the Room Flags for a Location. Or clear it.
    Flags are entered as so: +DARK +INDOORS -CAVE
    See .choices/RoomFlags

.location/sector <LocationID>=<sector> | NONE
    Set the Sector Type for a Location. Or clear it.
    GridAreas with a cleared sector fallback to the GridArea's defaultSector.

)";

enum class LocationOp : uint8_t
{
    Examine,
    Name,
    Description,
    Flags,
    Sector,
    Help
};

ACMD(do_mush_location)
{
    std::string_view op = cdata.switches.empty() ? "examine" : cdata.switches[0];

    auto oper = volcano::util::chooseEnum<LocationOp>(op, "Location Operation");

    if (!oper)
    {
        ch->sendFmt(oper.error());
        return;
    }

    auto operation = oper.value();
    switch(operation) {
        case LocationOp::Help: {
            ch->sendText(mushLocationHelp);
            return;
        }
        case LocationOp::Examine: {
            auto locRes = getLocation(cdata.lstrim.empty() ? "here" : cdata.lstrim, ch);
            if (!locRes) {
                ch->sendText(locRes.error());
                return;
            }
            auto loc = locRes.value();
            ch->sendFmt("{}\r\n", loc.renderDiagnostics(ch));
            return;
        }
        case LocationOp::Name: {
            auto locRes = getLocation(cdata.lstrim, ch);
            if (!locRes) {
                ch->sendText(locRes.error());
                return;
            }
            auto loc = locRes.value();
            if (cdata.rstrim.empty()) {
                ch->sendFmt("Current name: {}\r\n", loc.getName());
                return;
            }
            if (boost::iequals(cdata.rstrim, "NONE")) {
                loc.setString("name", "");
                ch->sendFmt("{} Name cleared.\r\n", loc);
                return;
            }
            loc.setString("name", std::string(cdata.rstrim));
            ch->sendFmt("{} Name set to: {}\r\n", loc, loc.getName());
            return;
        }
        case LocationOp::Description: {
            auto locRes = getLocation(cdata.lstrim, ch);
            if (!locRes) {
                ch->sendText(locRes.error());
                return;
            }
            auto loc = locRes.value();
            if (cdata.rstrim.empty()) {
                ch->sendFmt("Current description: {}\r\n", loc.getLookDescription());
                return;
            }
            if (boost::iequals(cdata.rstrim, "NONE")) {
                loc.setString("look_description", "");
                ch->sendFmt("{} Look description cleared.\r\n", loc);
                return;
            }
            loc.setString("look_description", std::string(cdata.rstrim));
            ch->sendFmt("{} Look description set to: {}\r\n", loc, loc.getLookDescription());
            return;
        }
        case LocationOp::Sector: {
            auto locRes = getLocation(cdata.lstrim, ch);
            if (!locRes) {
                ch->sendText(locRes.error());
                return;
            }
            auto loc = locRes.value();
            if (cdata.rstrim.empty()) {
                ch->sendFmt("Current sector: {}\r\n", loc.getSectorType());
                return;
            }
            auto sectorRes = volcano::util::chooseEnum<SectorType>(cdata.rstrim, "Sector Type");
            if (!sectorRes) {
                ch->sendText(sectorRes.error());
                return;
            }
            auto sec = sectorRes.value();
            loc.setSectorType(sec);
            ch->sendFmt("{} Sector set to: {}\r\n", loc, sec);
            return;
        }
        case LocationOp::Flags: {
            auto locRes = getLocation(cdata.lstrim, ch);
            if (!locRes) {
                ch->sendText(locRes.error());
                return;
            }
            auto loc = locRes.value();
            auto &cf = loc.getRoomFlags();
            auto res = handleFlagOps<RoomFlag>(cf, cdata.rstrim, "Room Flags");
            if (!res) {
                ch->sendText(res.error());
                return;
            }
            ch->sendFmt("For {}: {}\r\n", loc, res.value());
            return;
        }
    }
}


constexpr std::string_view mushResetHelp = R"(
MUSH-style Zone Reset Editor
=============================================================================
Alias: .res

This command manages the reset commands in your current location.

See .location/help for details on LocationIDs.

For <command> entries, they obey the following rules:
<chance>
    a value from 0 to 100. Percent chance of it happening.
    a 0d100 is rolled and if it is <= <chance> the rule will execute.

<if>
   A boolean value (0 or 1). 0, this command always executes. If 1, it will
   only execute if the previous command was successful.

<MaxWorld>
   If the world already contains at least this many, this rule will not run.
   0 disables this check.

<MaxSpawn>
   If the world already contains at least this many, which spawned at this
   location, this rule will not run.
   0 disables this check.

The following <commands> are available:

MOB,<vnum>,<MaxSpawn>,<MaxWorld>,<chance>,<if>
    Places a mob in the location.
OBJ,<vnum>,<MaxSpawn>,<MaxWorld>,<chance>,<if>
    Places an object in the location.
GIVE,<vnum>,<targetVnum>,<chance>,<if>
    Gives an object to the last MOB <targetVnum> spawned in this sequence.
EQUIP,<vnum>,<slot>,<chance>,<if>
    Attempts to equip the last spawned mob with OBJ <vnum>. It will go to
    inventory if that fails. see .choices/WearSlot for <slot> options.
PUT,<vnum>,<targetVnum>,<chance>,<if>
    Puts object in the inventory of the last OBJ <targetVnum> spawned in this
    sequence.
REMOVE,<vnum>
    Remove one instance of OBJ <vnum> if present.
DOOR,<direction>,<state>
    If exit in <direction> exists, set its state.
    See .choices/direction for Directions.
    <state>: 0 = open, 1 = closed, 2 = closed and locked.
TRIGGER,<vnum>,<type>
    Assign DgScript <vnum> to <type>. 0 = character, 1 = object, 2 = room.
    Will target the last spawned obj/character as appropriate.
VARIABLE,<type>,<name>,<value>
    Set a script variable to the <type> same as TRIGGER sort.
    Name should be alphanumeric with no spaces like "gotfood".
    Commas are not supported in the <value>

.reset [<LocationID>]
    Display the reset commands for a given Location, or your current one if
    not provided.

.reset/add <LocationID>=<command>
    Add a reset command for a given Location. It will be at the bottom.

.reset/insert <LocationID>=<n>,<command>
    Insert a reset command at given index. 0 puts it at the top and
    pushes everything else down.

.reset/replace <LocationID>=<n>,<command>
    Replaces the reset command at index <n>.

.reset/delete <LocationID>=<n>
    Delete a given reset command by index.
    Index starts at 0. 0 is first entry.

.reset/execute [<LocationID>]
    Executes the reset commands for a given Location, or your current one
    if not provided. Used mainly for testing. Use the PURGE command to clear
    everything in the room.
)";

enum class ResOp : uint8_t {
    Examine,
    Add,
    Insert,
    Replace,
    Delete,
    Execute,
    Help
};

ACMD(do_mush_reset) {
    std::string_view op = cdata.switches.empty() ? "examine" : cdata.switches[0];

    auto oper = volcano::util::chooseEnum<ResOp>(op, "Reset Operation");

    if (!oper)
    {
        ch->sendFmt(oper.error());
        return;
    }

    std::vector<std::string> split;
    boost::split(split, cdata.rstrim, boost::is_space(), boost::token_compress_on);
    for(auto &s : split) boost::trim(s);
    auto slice = split | std::views::drop(1);
    std::vector<std::string> cmdArgs{slice.begin(), slice.end()};

    auto operation = oper.value();
    switch(operation) {
        case ResOp::Help: {
            ch->sendText(mushResetHelp);
            return;
        }
        case ResOp::Examine: {
            auto locRes = getLocation(cdata.lstrim.empty() ? "here" : cdata.lstrim, ch);
            if (!locRes) {
                ch->sendText(locRes.error());
                return;
            }
            auto loc = locRes.value();
            auto rcm = loc.getResetCommands();
            if(rcm.empty()) {
                ch->sendText("No reset commands found.\r\n");
                return;
            }
            int line = 0;
            ch->sendText("Reset Commands for here:\r\n");
            for(auto &c : rcm) {
                ch->sendFmt("{}: {}@n\r\n", line++, c);
            }
            return;
        }
        case ResOp::Add: {
            auto locRes = getLocation(cdata.lstrim.empty() ? "here" : cdata.lstrim, ch);
            if (!locRes) {
                ch->sendText(locRes.error());
                return;
            }
            auto loc = locRes.value();
            auto rcm = loc.getResetCommands();
            // we already did the argument split earlier...
            auto cmdRes = parseResetCommand(split);
            if(!cmdRes) {
                ch->sendText(cmdRes.error());
                return;
            }
            rcm.emplace_back(cmdRes.value());
            loc.setResetCommands(rcm);
            ch->sendFmt("Added Reset Command: {}\r\n", cmdRes.value());
            return;
        }
        case ResOp::Delete: {
            if(split.size() < 1) {
                ch->sendText("Delete what?");
                return;
            }
            auto locRes = getLocation(split[0], ch);
            if (!locRes) {
                ch->sendText(locRes.error());
                return;
            }
            auto loc = locRes.value();
            auto rcm = loc.getResetCommands();
            int index = parseNumber<int>(cdata.rstrim, "Reset Command Index").value_or(-1);
            if (index < 0 || index >= static_cast<int>(rcm.size())) {
                ch->sendText("Invalid index.\r\n");
                return;
            }
            auto iter = rcm.begin() + index;
            ch->sendFmt("Deleted Reset Command {}: {}.\r\n", index, *iter);
            rcm.erase(iter);
            loc.setResetCommands(rcm);
            return;
        }
        case ResOp::Insert: {
            if(split.size() < 1) {
                ch->sendText("Insert to where?");
                return;
            }
            auto locRes = getLocation(split[0], ch);
            if (!locRes) {
                ch->sendText(locRes.error());
                return;
            }
            auto loc = locRes.value();
            auto rcm = loc.getResetCommands();
            int index = parseNumber<int>(cmdArgs[0], "Reset Command Index").value_or(-1);
            if (index < 0 || index > static_cast<int>(rcm.size())) {
                ch->sendText("Invalid index.\r\n");
                return;
            }
            cmdArgs.erase(cmdArgs.begin());
            auto cmdRes = parseResetCommand(cmdArgs);
            if(!cmdRes) {
                ch->sendText(cmdRes.error());
                return;
            }
            rcm.insert(rcm.begin() + index, cmdRes.value());
            loc.setResetCommands(rcm);
            ch->sendFmt("Inserted Reset Command at {}: {}\r\n", index, cmdRes.value());
            return;
        }
        case ResOp::Execute: {
            auto locRes = getLocation(cdata.lstrim.empty() ? "here" : cdata.lstrim, ch);
            if (!locRes) {
                ch->sendText(locRes.error());
                return;
            }
            auto loc = locRes.value();
            auto rcm = loc.getResetCommands();
            if (rcm.empty()) {
                ch->sendText("No reset commands found.\r\n");
                return;
            }
            loc.executeResetCommands(rcm);
            ch->sendFmt("Executing Reset Commands for {}\r\n", loc);
            return;
        }
        case ResOp::Replace: {
            if(split.size() < 1) {
                ch->sendText("Replace in where?");
                return;
            }
            auto locRes = getLocation(split[0], ch);
            if (!locRes) {
                ch->sendText(locRes.error());
                return;
            }
            auto loc = locRes.value();
            auto rcm = loc.getResetCommands();
            int index = parseNumber<int>(cmdArgs[0], "Reset Command Index").value_or(-1);
            if (index < 0 || index >= static_cast<int>(rcm.size())) {
                ch->sendText("Invalid index.\r\n");
                return;
            }
            cmdArgs.erase(cmdArgs.begin());
            auto cmdRes = parseResetCommand(cmdArgs);
            if(!cmdRes) {
                ch->sendText(cmdRes.error());
                return;
            }
            rcm[index] = cmdRes.value();
            loc.setResetCommands(rcm);
            ch->sendFmt("Replaced Reset Command at {}: {}\r\n", index, cmdRes.value());
            return;
        }
    }
}

enum class HasMudStringsOp : uint8_t {
    Name,
    ShortDesc,
    RoomDesc,
    LookDesc,
};

Result<std::string> handleMudStrings(HasMudStrings* ms, HasMudStringsOp op, std::string arg) {
    std::string* target;
    std::string fieldName;
    switch(op) {
        case HasMudStringsOp::Name:
            target = &ms->name;
            fieldName = "Name";
            break;
        case HasMudStringsOp::ShortDesc:
            target = &ms->short_description;
            fieldName = "Short Description";
            break;
        case HasMudStringsOp::RoomDesc:
            target = &ms->room_description;
            fieldName = "Room Description";
            break;
        case HasMudStringsOp::LookDesc:
            target = &ms->look_description;
            fieldName = "Look Description";
            break;
        default:
            return err("Invalid operation.");
    }
    if(arg.empty()) {
        return fmt::format("Current {}: {}", fieldName, *target);
    }
    if(boost::iequals(arg, "NONE")) {
        *target = "";
        return fmt::format("{} cleared.", fieldName);
    }
    *target = arg;
    return fmt::format("{} set to: {}", fieldName, *target);
}

enum class HasExtraDescOps {
    ListExtraDescs,
    AddExtraDesc,
    RemoveExtraDesc,
    ClearExtraDescs
};

Result<std::string> handleExtraDescs(HasExtraDescriptions* he, HasExtraDescOps op, std::string_view arg) {
    // when using add, arg will be split by a | into keywords and description.
    // When using remove, arg is the index# to remove.
    switch(op) {
        case HasExtraDescOps::ListExtraDescs: {
            if(he->extra_descriptions.empty()) {
                return "No extra descriptions found.";
            }
            std::string result = "Extra Descriptions:\r\n";
            int line = 0;
            for(auto &ed : he->extra_descriptions) {
                result += fmt::format("{}: [{}] {}\r\n", line++, ed.first, ed.second);
            }
            return result;
        }
        case HasExtraDescOps::ClearExtraDescs: {
            he->extra_descriptions.clear();
            return "All extra descriptions cleared.";
        }
        case HasExtraDescOps::AddExtraDesc: {
            // arg should be <keyword>|<description>
            std::vector<std::string> split;
            boost::split(split, arg, boost::is_any_of("|"));
            if(split.size() != 2) {
                return err("Invalid format. Use <keyword>|<description>.");
            }
            auto keyword = split[0];
            auto description = split[1];
            boost::trim(keyword);
            boost::trim(description);
            if(keyword.empty() || description.empty()) {
                return err("Keyword and description cannot be empty.");
            }
            he->extra_descriptions.emplace_back(keyword, description);
            return fmt::format("Added extra description: [{}] {}", keyword, description);
        }
        case HasExtraDescOps::RemoveExtraDesc: {
            int index = parseNumber<int>(arg, "Extra Description Index").value_or(-1);
            if(index < 0 || index >= static_cast<int>(he->extra_descriptions.size())) {
                return err("Invalid index.");
            }
            auto iter = he->extra_descriptions.begin() + index;
            auto keyword = iter->first;
            he->extra_descriptions.erase(iter);
            return fmt::format("Removed extra description: [{}]", keyword);
        }
        default:
            return err("Invalid operation.");
    }
}

enum class HasProtoScriptOps {
    ListScripts,
    AddScript,
    RemoveScript,
    ClearScripts,
};

Result<std::string> handleProtoScripts(HasProtoScript* hp, HasProtoScriptOps op, std::string_view arg) {
    // when using add, arg will be split by a | into vnum and type.
    // When using remove, arg is the index# to remove.
    switch(op) {
        case HasProtoScriptOps::ListScripts: {
            if(hp->proto_script.empty()) {
                return "No proto scripts found.";
            }
        }
        case HasProtoScriptOps::ClearScripts: {
            hp->proto_script.clear();
            return "All proto scripts cleared.";
        }
        case HasProtoScriptOps::AddScript: {
            // arg should be <vnum>|<type>
            std::vector<std::string> split;
            boost::split(split, arg, boost::is_any_of("|"));
            if(split.size() != 2) {
                return err("Invalid format. Use <vnum>|<type>.");
            }
            auto vnumStr = split[0];
            auto typeStr = split[1];
            boost::trim(vnumStr);
            boost::trim(typeStr);
            if(vnumStr.empty() || typeStr.empty()) {
                return err("Vnum and type cannot be empty.");
            }
            auto vnumRes = parseNumber<trig_vnum>(vnumStr, "Script Vnum");
            if(!vnumRes) {
                return err(vnumRes.error());
            }
            if(!trig_index.contains(vnumRes.value())) {
                return err("Script Vnum not found.");
            }
            hp->proto_script.emplace_back(vnumRes.value());
            return fmt::format("Added proto script: [vnum: {}]", vnumRes.value());
        }
        case HasProtoScriptOps::RemoveScript: {
            int index = parseNumber<int>(arg, "Proto Script Index").value_or(-1);
            if(index < 0 || index >= static_cast<int>(hp->proto_script.size())) {
                return err("Invalid index.");
            }
            auto iter = hp->proto_script.begin() + index;
            auto vnum = *iter;
            hp->proto_script.erase(iter);
            return fmt::format("Removed proto script: [vnum: {}]", vnum);
        }
        default:
            return err("Invalid operation.");
        }
}

enum class CharacterBaseOps {
    Race,
    Model,
    Sensei,
    Sex,
    Size,
    CharacterFlags,
    MobFlags,
    BioGenomes,
    Mutations,
    AffectFlags
};

using CharacterBaseOpChoice = std::variant<HasMudStringsOp, HasExtraDescOps, CharacterBaseOps>;

Result<CharacterBaseOpChoice> parseCharacterBaseOp(std::string_view op) {
    auto mudStrOp = volcano::util::chooseEnum<HasMudStringsOp>(op, "MudStrings Operation");
    if(mudStrOp) return mudStrOp.value();
    auto exDescOp = volcano::util::chooseEnum<HasExtraDescOps>(op, "ExtraDesc Operation");
    if(exDescOp) return exDescOp.value();
    auto charBaseOp = volcano::util::chooseEnum<CharacterBaseOps>(op, "CharacterBase Operation");
    if(charBaseOp) return charBaseOp.value();
    return err("Invalid operation.");
}

Result<std::string> handleCharacterBaseOps(CharacterBase* cb, CharacterBaseOpChoice op, CommandData& cdata) {
    if(std::holds_alternative<HasMudStringsOp>(op)) {
        auto msOp = std::get<HasMudStringsOp>(op);
        return handleMudStrings(cb, msOp, std::string(cdata.rstrim));
    } else if(std::holds_alternative<HasExtraDescOps>(op)) {
        auto edOp = std::get<HasExtraDescOps>(op);
        return handleExtraDescs(cb, edOp, cdata.rstrim);
    } else if(std::holds_alternative<CharacterBaseOps>(op)) {
        auto cbOp = std::get<CharacterBaseOps>(op);
        switch(cbOp) {
            case CharacterBaseOps::Race: {
                return volcano::util::handleSetEnum<Race>(cb->race, cdata.rstrim, "Race");
            }
            case CharacterBaseOps::Model: {
                // only Androids use model...
                if(cb->race != Race::android) {
                    return err("Only Androids have a Model.");
                }
                AndroidModel model;
                auto res = volcano::util::handleSetEnum<AndroidModel>(model, cdata.rstrim, "Model");
                if(res) {
                    cb->model = model;
                }
                return res;
            }
            case CharacterBaseOps::Sensei:
                return volcano::util::handleSetEnum<Sensei>(cb->sensei, cdata.rstrim, "Sensei");
            case CharacterBaseOps::Sex:
                return volcano::util::handleSetEnum<Sex>(cb->sex, cdata.rstrim, "Sex");
            case CharacterBaseOps::Size:
                return volcano::util::handleSetEnum<Size>(cb->size, cdata.rstrim, "Size");
            case CharacterBaseOps::CharacterFlags:
                return handleFlagOps<CharacterFlag>(cb->character_flags, cdata.rstrim, "Character Flags");
            case CharacterBaseOps::MobFlags:
                return handleFlagOps<MobFlag>(cb->mob_flags, cdata.rstrim, "Mob Flags");
            case CharacterBaseOps::BioGenomes:
                return handleFlagOps<Race>(cb->bio_genomes, cdata.rstrim, "BioGenomes");
            case CharacterBaseOps::Mutations:
                return handleFlagOps<Mutation>(cb->mutations, cdata.rstrim, "Mutations");
            case CharacterBaseOps::AffectFlags:
                return handleFlagOps<AffectFlag>(cb->affect_flags, cdata.rstrim, "Affect Flags");
        }
    }
    return err("Invalid operation.");
}

constexpr std::string_view mushProtoHelp = R"(
MUSH-style Mobile (mob) Prototype Editor
=============================================================================
Alias: .mp

This command manages the mobile prototypes in the game.
Remember that the /switches can partial match.

.mproto <vnum>
    Display information about the given Mob Prototype.

.mproto/create <vnum>
    Create a new Mob Prototype with the given Vnum.

.mproto/delete <vnum>=YES
    Delete the given Mob Prototype. You must type YES to confirm.

.mproto/list <startVnum>=<endVnum>
    List all Mob Prototypes in the given range.

.mproto/stat <vnum>/[<stat>[=<value>]]
    Set a stat on the given Mob Prototype. Omit the value to see options.
    Examples:
    .mproto/stat 50 to view all stats that mob proto 50 can have. 
    .mproto/stat 50/strength to view info about strength,
    .mproto/stat 50/strength=10 to set strength to 10.

.mproto/name <vnum>=<name>
    Set the Name for a Mob Prototype. This is really the keywords used for
    searching.

.mproto/shortdesc <vnum>=<short description>
    Set the Short Description for a Mob Prototype. This is what people see when
    the mobile is used in action descriptions.

.mproto/lookdesc <vnum>=<look description>
    Set the Look Description for a Mob Prototype. This is what people see when
    they look at the mobile.

.mproto/roomdesc <vnum>=<room description>
    Set the Room Description for a Mob Prototype. This is what people see when
    the mobile is listed amongst others in a location.

.mproto/listextradesc <vnum>
    List all Extra Descriptions for the given Mob Prototype.

.mproto/addextradesc <vnum>=<keyword>|<description>
    Add an Extra Description to the given Mob Prototype. Separate keywords and
    description with a | (pipe). The | character may not appear in either field.

.mproto/removeextradesc <vnum>=<index>
    Remove the Extra Description at the given index from the Mob Prototype.

.mproto/clearextradesc <vnum>
    Remove all Extra Descriptions from the Mob Prototype.

.mproto/listscripts <vnum>
    List all DgScripts assigned to the given Mob Prototype.

.mproto/addscript <vnum>=<scriptVnum>|<index>
    Add a DgScript to the given Mob Prototype. Separate vnum and index with
    a | (pipe). use index -1 to append to the end, otherwise it will insert at
    given index and 'push down' anything after it. The order does matter as the
    scripts are processed in that order.

.mproto/removescript <vnum>=<index>
    Remove the DgScript at the given index from the Mob Prototype.

.mproto/clearscripts <vnum>
    Remove all DgScripts from the Mob Prototype.

.mproto/race <vnum>=<race>
    Set the Race for the given Mob Prototype.

.mproto/model <vnum>=<model>
    Set the Model for the given Mob Prototype. Only Androids have a model.

.mproto/sensei <vnum>=<sensei>
    Set the Sensei for the given Mob Prototype.

.mproto/sex <vnum>=<sex>
    Set the sex for the given Mob Prototype.

.mproto/size <vnum>=<size>
    Set the size for the given Mob Prototype.

.mproto/characterflags <vnum>=<flags>
    Set the Character Flags for the given Mob Prototype.
    Use + or - to add or remove flags. Example: +FLAG1 -FLAG2
    See .choices/CharacterFlags

.mproto/mobflags <vnum>=<flags>
    Set the Mob Flags for the given Mob Prototype.
    See .choices/MobFlags

.mproto/affectflags <vnum>=<flags>
    Set the Affect Flags for the given Mob Prototype.
    See .choices/AffectFlags

.mproto/biogenomes <vnum>=<flags>
    Set the BioGenomes for the given Mob Prototype.
    See .choices/Race
    Note: Meant for Bio-Androids, but will work on any mob.

.mproto/mutations <vnum>=<flags>
    Set the Mutations for the given Mob Prototype.
    See .choices/Mutations
    Note: Meant for Mutants, but will work on any mob.

)";

enum class MobProtoOps {
    ExamineProto,
    CreateProto,
    ListProto,
    DeleteProto,
    StatProto,
    Help
};



void handleMobProtoOps(Character *ch, MobProtoOps op, CommandData& cdata) {
    switch(op) {
        case MobProtoOps::ExamineProto: {
            return;
        }
        case MobProtoOps::ListProto: {
            ch->sendText("Not implemented yet.\r\n");
            return;
        }
        case MobProtoOps::CreateProto: {
            auto numRes = parseNumber<mob_vnum>(cdata.lstrim, "Mob Vnum");
            if(!numRes) {
                ch->sendText(numRes.error());
                return;
            }
            auto mv = numRes.value();
            if(mob_proto.contains(mv)) {
                ch->sendFmt("Mob Proto {} already exists.\r\n", mv);
                return;
            }
            auto newMob = std::make_shared<CharacterPrototype>();
            newMob->vn = mv;
            newMob->name = "unnamed mobile";
            newMob->short_description = "an unnamed mobile";
            newMob->look_description = "You see nothing special.\r\n";
            newMob->room_description = "An unnamed mobile is here. Someone nag a builder.\r\n";
            mob_proto.emplace(mv, newMob);
            ch->sendFmt("Created new Mob Proto: {}\r\n", *newMob);
            return;
        }
        case MobProtoOps::DeleteProto: {
            ch->sendText("Not implemented yet.\r\n");
            return;
        }
        case MobProtoOps::StatProto: {
            ch->sendText("Not implemented yet.\r\n");
            return;
        }
        case MobProtoOps::Help: {
            ch->sendText(mushProtoHelp);
            return;
        }
    }
}



ACMD(do_mush_mproto) {
    std::string_view op = cdata.switches.empty() ? "" : cdata.switches[0];

    auto oper = volcano::util::chooseEnum<MobProtoOps>(op, "MobProto Operation");
    if(oper) {
        handleMobProtoOps(ch, oper.value(), cdata);
        return;
    }

    // Or are we adding a Script Prototype Vnum to it...?
    auto protoSc = volcano::util::chooseEnum<HasProtoScriptOps>(op, "ProtoScript Operation");
    if(protoSc) {
        auto mobRes = getMobProto(cdata.lstrim);
        if(!mobRes) {
            ch->sendText(mobRes.error());
            return;
        }
        auto res = handleProtoScripts(mobRes.value(), protoSc.value(), cdata.rstrim);
        if(!res) {
            ch->sendText(res.error());
            return;
        }
        ch->sendFmt("{} Modified: {}\r\n", *mobRes.value(), res.value());
        return;
    }

    auto charBaseOpRes = parseCharacterBaseOp(op);
    if(charBaseOpRes) {
        auto mobRes = getMobProto(cdata.lstrim);
        if(!mobRes) {
            ch->sendText(mobRes.error());
            return;
        }
        auto res = handleCharacterBaseOps(mobRes.value(), charBaseOpRes.value(), cdata);
        if(!res) {
            ch->sendText(res.error());
        } else {
            ch->sendFmt("{} Modified: {}\r\n", *mobRes.value(), res.value());
        }
        return;
    }
    ch->sendText("Invalid Operation. See .mproto/help\r\n");

}

enum class HasPickyOps {
    OnlyAlign,
    NotAlign,
    OnlySensei,
    NotSensei,
    OnlyRace,
    NotRace
};

Result<std::string> handlePickyOps(HasPicky* hp, HasPickyOps op, std::string_view arg) {
    switch(op) {
        case HasPickyOps::OnlyAlign:
            return handleFlagOps<MoralAlign>(hp->only_alignment, arg, "Only Alignments");
        case HasPickyOps::NotAlign:
            return handleFlagOps<MoralAlign>(hp->not_alignment, arg, "Not Alignments");
        case HasPickyOps::OnlySensei:
            return handleFlagOps<Sensei>(hp->only_sensei, arg, "Only Sensei");
        case HasPickyOps::NotSensei:
            return handleFlagOps<Sensei>(hp->not_sensei, arg, "Not Sensei");
        case HasPickyOps::OnlyRace:
            return handleFlagOps<Race>(hp->only_race, arg, "Only Races");
        case HasPickyOps::NotRace:
            return handleFlagOps<Race>(hp->not_race, arg, "Not Races");
        default:
            return err("Invalid operation.");
    }
    return err("Invalid operation.");
}

enum class ObjectBaseOps {
    ItemType,
    Affected,
    WearFlags,
    ItemFlags,
    AffectFlags,
    Size
};

using ObjectBaseOpChoice = std::variant<HasMudStringsOp, HasExtraDescOps, HasPickyOps, ObjectBaseOps>;

Result<ObjectBaseOpChoice> parseObjectBaseOp(std::string_view op) {
    auto mudStrOp = volcano::util::chooseEnum<HasMudStringsOp>(op, "MudStrings Operation");
    if(mudStrOp) return mudStrOp.value();
    auto exDescOp = volcano::util::chooseEnum<HasExtraDescOps>(op, "ExtraDesc Operation");
    if(exDescOp) return exDescOp.value();
    auto pickyOp = volcano::util::chooseEnum<HasPickyOps>(op, "Picky Operation");
    if(pickyOp) return pickyOp.value();
    auto objBaseOp = volcano::util::chooseEnum<ObjectBaseOps>(op, "ObjectBase Operation");
    if(objBaseOp) return objBaseOp.value();
    return err("Invalid operation.");
}

Result<std::string> handleObjectBaseOps(ObjectBase* ob, ObjectBaseOpChoice op, CommandData& cdata) {
    if(std::holds_alternative<HasMudStringsOp>(op)) {
        auto msOp = std::get<HasMudStringsOp>(op);
        return handleMudStrings(ob, msOp, std::string(cdata.rstrim));
    } else if(std::holds_alternative<HasExtraDescOps>(op)) {
        auto edOp = std::get<HasExtraDescOps>(op);
        return handleExtraDescs(ob, edOp, cdata.rstrim);
    } else if(std::holds_alternative<HasPickyOps>(op)) {
        auto pOp = std::get<HasPickyOps>(op);
        return handlePickyOps(ob, pOp, cdata.rstrim);
    } else if(std::holds_alternative<ObjectBaseOps>(op)) {
        auto obOp = std::get<ObjectBaseOps>(op);
        switch(obOp) {
            case ObjectBaseOps::ItemType:
                return volcano::util::handleSetEnum<ItemType>(ob->type_flag, cdata.rstrim, "Item Type");
            case ObjectBaseOps::Affected: {
                //return handleFlagOps<AffectLocation>(ob->affected, cdata.rstrim, "Affected");
                return err("Not implemented yet.");
            }
            case ObjectBaseOps::WearFlags:
                return handleFlagOps<WearFlag>(ob->wear_flags, cdata.rstrim, "Wear Flags");
            case ObjectBaseOps::ItemFlags:
                return handleFlagOps<ItemFlag>(ob->item_flags, cdata.rstrim, "Item Flags");
            case ObjectBaseOps::AffectFlags:
                return handleFlagOps<AffectFlag>(ob->affect_flags, cdata.rstrim, "Affect Flags");
            case ObjectBaseOps::Size:
                return volcano::util::handleSetEnum<Size>(ob->size, cdata.rstrim, "Size");
        }
    }
    return err("Invalid operation.");
}

constexpr std::string_view mushOProtoHelp = R"(
MUSH-style Object (obj) Prototype Editor
=============================================================================
Alias: .op

This command manages the mobile prototypes in the game.
Remember that the /switches can partial match.

.oproto <vnum>
    Display information about the given Prototype.

.oproto/create <vnum>
    Create a new Prototype with the given Vnum.

.oproto/delete <vnum>=YES
    Delete the given Prototype. You must type YES to confirm.

.oproto/list <startVnum>=<endVnum>
    List all Prototypes in the given range.

.oproto/stat <vnum>/[<stat>[=<value>]]
    Set a stat on the given Prototype. Omit the value to see options.
    Examples:
    .oproto/stat 50 to view all stats that proto 50 can have. 
    .oproto/stat 50/strength to view info about strength,
    .oproto/stat 50/strength=10 to set strength to 10.

.oproto/name <vnum>=<name>
    Set the Name for a Prototype. This is really the keywords used for
    searching.

.oproto/shortdesc <vnum>=<short description>
    Set the Short Description for a Prototype. This is what people see when
    the mobile is used in action descriptions.

.oproto/lookdesc <vnum>=<look description>
    Set the Look Description for a Prototype. This is what people see when
    they look at the mobile.

.oproto/roomdesc <vnum>=<room description>
    Set the Room Description for a Prototype. This is what people see when
    the mobile is listed amongst others in a location.

.oproto/listextradesc <vnum>
    List all Extra Descriptions for the given Prototype.

.oproto/addextradesc <vnum>=<keyword>|<description>
    Add an Extra Description to the given Prototype. Separate keywords and
    description with a | (pipe). The | character may not appear in either field.

.oproto/removeextradesc <vnum>=<index>
    Remove the Extra Description at the given index from the Prototype.

.oproto/clearextradesc <vnum>
    Remove all Extra Descriptions from the Prototype.

.oproto/listscripts <vnum>
    List all DgScripts assigned to the given Prototype.

.oproto/addscript <vnum>=<scriptVnum>|<index>
    Add a DgScript to the given Prototype. Separate vnum and index with
    a | (pipe). use index -1 to append to the end, otherwise it will insert at
    given index and 'push down' anything after it. The order does matter as the
    scripts are processed in that order.

.oproto/removescript <vnum>=<index>
    Remove the DgScript at the given index from the Prototype.

.oproto/clearscripts <vnum>
    Remove all DgScripts from the Prototype.

.oproto/itemtype <vnum>=<itemtype>
    Set the Item Type for the given Prototype.

.oproto/size <vnum>=<size>
    Set the size for the given Prototype.

.oproto/itemflags <vnum>=<flags>
    Set the Item Flags for the given Prototype.
    Use + or - to add or remove flags. Example: +FLAG1 -FLAG2
    See .choices/ItemFlags

.oproto/wearflags <vnum>=<flags>
    Set the Wear Flags for the given Prototype.
    Remember: WearFlag::take is needed for an item to be picked up.
    See .choices/WearFlags

.oproto/affectflags <vnum>=<flags>
    Set the Affect Flags for the given Prototype.
    See .choices/AffectFlags

.oproto/addaffected <vnum>=<location>,<specific>,<modifier>
    Add an Affect to the given Prototype. Location, Specific, and Modifier
    are all numbers. Must study the Apply code to know what to put here, sorry.

.oproto/removeaffected <vnum>=<index>
    Remove the Affect at the given index from the Prototype.

)";

enum class ObjProtoOps {
    ExamineProto,
    CreateProto,
    ListProto,
    DeleteProto,
    StatProto,
    Help
};

void handleObjProtoOps(Character *ch, ObjProtoOps op, CommandData& cdata) {
    switch(op) {
        case ObjProtoOps::ExamineProto: {
            return;
        }
        case ObjProtoOps::ListProto: {
            ch->sendText("Not implemented yet.\r\n");
            return;
        }
        case ObjProtoOps::CreateProto: {
            auto numRes = parseNumber<obj_vnum>(cdata.lstrim, "Object Vnum");
            if(!numRes) {
                ch->sendText(numRes.error());
                return;
            }
            auto ov = numRes.value();
            if(obj_proto.contains(ov)) {
                ch->sendFmt("Object Proto {} already exists.\r\n", ov);
                return;
            }
            auto newObj = std::make_shared<ObjectPrototype>();
            newObj->vn = ov;
            newObj->name = "unnamed object";
            newObj->short_description = "an unnamed object";
            newObj->look_description = "You see nothing special.\r\n";
            obj_proto.emplace(ov, newObj);
            ch->sendFmt("Created new Object Proto: {}\r\n", *newObj);
            return;
        }
        case ObjProtoOps::DeleteProto: {
            ch->sendText("Not implemented yet.\r\n");
            return;
        }
        case ObjProtoOps::StatProto: {
            ch->sendText("Not implemented yet.\r\n");
            return;
        }
        case ObjProtoOps::Help: {
            ch->sendText(mushOProtoHelp);
            return;
        }
    }
}

ACMD(do_mush_oproto) {
    std::string_view op = cdata.switches.empty() ? "" : cdata.switches[0];

    auto oper = volcano::util::chooseEnum<ObjProtoOps>(op, "ObjProto Operation");
    if(oper) {
        handleObjProtoOps(ch, oper.value(), cdata);
        return;
    }

    // Or are we adding a Script Prototype Vnum to it...?
    auto protoSc = volcano::util::chooseEnum<HasProtoScriptOps>(op, "ProtoScript Operation");
    if(protoSc) {
        auto objRes = getObjProto(cdata.lstrim);
        if(!objRes) {
            ch->sendText(objRes.error());
            return;
        }
        auto res = handleProtoScripts(objRes.value(), protoSc.value(), cdata.rstrim);
        if(!res) {
            ch->sendText(res.error());
            return;
        }
        ch->sendFmt("{} Modified: {}\r\n", *objRes.value(), res.value());
        return;
    }

    auto objBaseOpRes = parseObjectBaseOp(op);
    if(objBaseOpRes) {
        auto objRes = getObjProto(cdata.lstrim);
        if(!objRes) {
            ch->sendText(objRes.error());
            return;
        }
        auto res = handleObjectBaseOps(objRes.value(), objBaseOpRes.value(), cdata);
        if(!res) {
            ch->sendText(res.error());
        } else {
            ch->sendFmt("{} Modified: {}\r\n", *objRes.value(), res.value());
        }
        return;
    }
    ch->sendText("Invalid Operation. See .oproto/help\r\n");

}