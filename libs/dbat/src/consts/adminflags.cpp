#include <array>
#include <iterator>
#include <string_view>

#include "dbat/consts/adminflags.hpp"

/* ADM_x */

/* Administrative flags, for flag_names() */
static constexpr auto admin_bits_entries = std::to_array<std::string_view>({
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
});
static_assert(std::size(admin_bits_entries) == flag_count<admin_flag>);
constinit const name_table admin_bits(admin_bits_entries);
