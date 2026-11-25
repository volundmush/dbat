#include <array>
#include <string_view>
#include "dbat/name_table.hpp"
#include "dbat/consts/appearance.hpp"

/* For Distinguishing Features */
static constexpr auto eye_types_entries = std::to_array<std::string_view>({
    "blue",
    "black",
    "green",
    "brown",
    "red",
    "aqua",
    "pink",
    "purple",
    "crimson",
    "gold",
    "amber",
    "emerald"});
static_assert(std::size(eye_types_entries) == 12);
constinit const name_table eye_types(eye_types_entries);

static constexpr std::array<std::string_view, 9> aura_types_entries = {{
    "white",
    "blue",
    "red",
    "green",
    "pink",
    "purple",
    "yellow",
    "black",
    "orange"}};
static_assert(std::size(aura_types_entries) == 9);
constinit const name_table aura_types(aura_types_entries);

static constexpr auto hairl_types_entries = std::to_array<std::string_view>({
    "bald",
    "short",
    "medium",
    "long",
    "really long"});
static_assert(std::size(hairl_types_entries) == 5);
constinit const name_table hairl_types(hairl_types_entries);

static constexpr auto FHA_types_entries = std::to_array<std::string_view>({
    "stubby",
    "short",
    "medium",
    "long",
    "really long"});
static_assert(std::size(FHA_types_entries) == 5);
constinit const name_table FHA_types(FHA_types_entries);

static constexpr auto hairc_types_entries = std::to_array<std::string_view>({
    "headed",
    "black",
    "brown",
    "blonde",
    "grey",
    "red",
    "orange",
    "green",
    "blue",
    "pink",
    "purple",
    "silver",
    "crimson",
    "white"});
static_assert(std::size(hairc_types_entries) == 14);
constinit const name_table hairc_types(hairc_types_entries);

static constexpr auto hairs_types_entries = std::to_array<std::string_view>({
    "",
    "with a plain look",
    "in a mohawk",
    "with a spiky look",
    "with a curly look",
    "with an uneven look",
    "in a ponytail",
    "in an afro",
    "with a fade look",
    "in a crew cut",
    "with a feathered look",
    "in dred locks"});
static_assert(std::size(hairs_types_entries) == 12);
constinit const name_table hairs_types(hairs_types_entries);

static constexpr auto skin_types_entries = std::to_array<std::string_view>({
    "white",
    "black",
    "green",
    "orange",
    "yellow",
    "red",
    "grey",
    "blue",
    "aqua",
    "pink",
    "purple",
    "tan"});
static_assert(std::size(skin_types_entries) == 12);
constinit const name_table skin_types(skin_types_entries);
