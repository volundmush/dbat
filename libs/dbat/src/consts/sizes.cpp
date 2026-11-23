#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/sizes.hpp"

/* Taken the SRD under OGL, see ../doc/srd.txt for information */
static constexpr std::string_view size_names_entries[] = {
    "fine",
    "diminutive",
    "tiny",
    "small",
    "medium",
    "large",
    "huge",
    "gargantuan",
    "colossal"};
static_assert(std::size(size_names_entries) == NUM_SIZES);
constinit const name_table size_names(size_names_entries);