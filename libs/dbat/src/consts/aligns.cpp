#include <array>
#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/aligns.hpp"

/* Alignments */
/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
static constexpr auto alignments_entries = std::to_array<std::string_view>({
    "Saintly",
    "Valiant",
    "Hero",
    "Do-gooder",
    "Neutral",
    "Crook",
    "Villain",
    "Terrible",
    "Horribly Evil",
});
static_assert(std::size(alignments_entries) == NUM_ALIGNS);
constinit const name_table alignments(alignments_entries);
