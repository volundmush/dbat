#include "dbat/game/Destination.hpp"
#include "dbat/game/constants.hpp"
#include <nlohmann/json.hpp>

std::optional<Destination> Destination::getReverse() const
{
    return getExit(rev_dir.at(dir));
}

constexpr int EXIT_ISDOOR = (1 << 0);    /* Exit is a door		*/
constexpr int EXIT_CLOSED = (1 << 1);    /* The door is closed	*/
constexpr int EXIT_LOCKED = (1 << 2);    /* The door is locked	*/
constexpr int EXIT_PICKPROOF = (1 << 3); /* Lock can't be picked	*/
constexpr int EXIT_SECRET = (1 << 4);    /* The door is hidden        */

void Destination::legacyExitFlags(int flags) {
    exit_flags.clear();
    exit_flags.set(ExitFlag::isdoor, flags & EXIT_ISDOOR);
    exit_flags.set(ExitFlag::closed, flags & EXIT_CLOSED);
    exit_flags.set(ExitFlag::locked, flags & EXIT_LOCKED);
    exit_flags.set(ExitFlag::pickproof, flags & EXIT_PICKPROOF);
    exit_flags.set(ExitFlag::secret, flags & EXIT_SECRET);
}

void to_json(nlohmann::json &j, const Destination &e)
{
    to_json(j, static_cast<const Location &>(e));
    j["dir"] = e.dir;
    if (!e.general_description.empty())
        j["general_description"] = e.general_description;
    if (!e.keyword.empty())
        j["keyword"] = e.keyword;
    if(e.exit_flags) j["exit_flags"] = e.exit_flags;
    if (e.key > 0)
        j["key"] = e.key;
    if (e.dclock)
        j["dclock"] = e.dclock;
    if (e.dchide)
        j["dchide"] = e.dchide;
}

void from_json(const nlohmann::json &j, Destination &e)
{
    from_json(j, static_cast<Location &>(e));
    if (j.contains(+"dir"))
        e.dir = j["dir"].get<Direction>();
    if (j.contains(+"general_description"))
        e.general_description = j["general_description"].get<std::string>();
    if (j.contains(+"keyword"))
        e.keyword = j["keyword"].get<std::string>();
    if (j.contains(+"exit_flags"))
        e.exit_flags = j["exit_flags"].get<FlagHandler<ExitFlag>>();
    if (j.contains(+"key"))
        e.key = j["key"];
    if (j.contains(+"dclock"))
        e.dclock = j["dclock"];
    if (j.contains(+"dchide"))
        e.dchide = j["dchide"];
}
