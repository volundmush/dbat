#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/directions.hpp"

/* cardinal directions */
static constexpr std::string_view dirs_entries[] = {
        "north",
        "east",
        "south",
        "west",
        "up",
        "down",
        "northwest",
        "northeast",
        "southeast",
        "southwest",
        "inside",
        "outside"};
static_assert(std::size(dirs_entries) == NUM_OF_DIRS);
constinit const name_table dirs(dirs_entries);
static constexpr std::string_view abbr_dirs_entries[] = {
        "n",
        "e",
        "s",
        "w",
        "u",
        "d",
        "nw",
        "ne",
        "se",
        "sw",
        "in",
        "out"};
static_assert(std::size(abbr_dirs_entries) == NUM_OF_DIRS);
constinit const name_table abbr_dirs(abbr_dirs_entries);

const int rev_dir[NUM_OF_DIRS] =
    {
        /* North */ SOUTH,
        /* East  */ WEST,
        /* South */ NORTH,
        /* West  */ EAST,
        /* Up    */ DOWN,
        /* Down  */ UP,
        /* NW    */ SOUTHEAST,
        /* NE    */ SOUTHWEST,
        /* SE    */ NORTHWEST,
        /* SW    */ NORTHEAST,
        /* In    */ OUTDIR,
        /* Out   */ INDIR};