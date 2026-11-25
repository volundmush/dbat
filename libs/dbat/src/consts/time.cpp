#include <array>
#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/time.hpp"

/* Not used by type_name(). */
static constexpr std::array<std::string_view, NUM_WEEK_DAYS> weekdays_entries = {{
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"}};
static_assert(std::size(weekdays_entries) == NUM_WEEK_DAYS);
constinit const name_table weekdays(weekdays_entries);

/* Not used by type_name(). */
static constexpr std::array<std::string_view, NUM_MONTHS> month_name_entries = {{
    "Month of January", /* 0 */
    "Month of February",
    "Month of March",
    "Month of April",
    "Month of May",
    "Month of June",
    "Month of July",
    "Month of August",
    "Month of September",
    "Month of October",
    "Month of November",
    "Month of December"}};
static_assert(std::size(month_name_entries) == NUM_MONTHS);
constinit const name_table month_name(month_name_entries);
