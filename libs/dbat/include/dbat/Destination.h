#pragma once
#include "templates.h"
#include "Location.h"

struct Destination : public Location {
    using Location::Location;
    using Location::operator=;
    Destination() = default;
    Destination(const Location& loc) : Location(loc) {}
    Direction dir{Direction::north}; /* Direction of the exit */

    std::string general_description{};       /* When look DIR.			*/
    std::string keyword{};        /* for open/close			*/

    FlagHandler<ExitFlag> exit_flags{}; /* Exit flags			*/

    obj_vnum key{NOTHING};        /* Key's number (-1 for no key)		*/

    int dclock{};            /* DC to pick the lock			*/
    int dchide{};            /* DC to find hidden			*/

    // this field deliberately not serialized.
    bool generated{false};

    std::optional<Destination> getReverse() const;

    void legacyExitFlags(int flags);
};

template <>
struct fmt::formatter<Destination> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Destination& z, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{} Exit {} to {}", z.generated ? "Generated" : "Direct", z.dir, z.getLocID());
    }
};

#define EXIT(ch, door)  ((ch)->location.getExit(static_cast<Direction>((door))))
#define W_EXIT(room, num)     (get_room((room))->exits.contains(static_cast<Direction>((num))))
#define EXIT_FLAGGED(exit, flag) ((exit)->exit_flags[(flag)])