#include "dbat/game/Destination.hpp"
#include "dbat/game/constants.hpp"

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