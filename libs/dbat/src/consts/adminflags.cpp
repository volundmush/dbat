#include <iterator>
#include <string_view>

#include "dbat/consts/adminflags.hpp"

/* ADM_x */

/* Administrative flags, for flag_names() */
static constexpr std::string_view admin_bits_entries[] = {
    "TELLALL",
    "SEEINV",
    "SEESECRET",
    "KNOWWEATHER",
    "FULLWHERE",
    "MONEY",
    "EATANYTHING",
    "NOPOISON",
    "WALKANYWHERE",
    "NOKEYS",
    "INSTANTKILL",
    "NOSTEAL",
    "TRANSALL",
    "SWITCHMORTAL",
    "FORCEMASS",
    "ALLHOUSES",
    "NODAMAGE",
    "ALLSHOPS",
    "CEDIT",
};
static_assert(std::size(admin_bits_entries) == NUM_ADMFLAGS);
constinit const name_table admin_bits(admin_bits_entries);
