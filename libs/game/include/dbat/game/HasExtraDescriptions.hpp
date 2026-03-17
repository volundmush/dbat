#pragma once
#include <string>
#include <vector>
#include <optional>
#include <span>

#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>


using ExtraDescription = std::pair<std::string, std::string>;
using ExtraDescriptions = std::vector<ExtraDescription>;

using ExtraDescriptionView = std::pair<std::string_view, std::string_view>;
using ExtraDescriptionViews = std::vector<ExtraDescriptionView>;


struct HasExtraDescriptions {
    ExtraDescriptionViews getExtraDescription() const; // Returns the extra description data.
    ExtraDescriptions extra_descriptions; // Extra descriptions for this unit.
};

inline std::string format_as(const HasExtraDescriptions& hed) {
    std::string result = "Extra Descriptions:\r\n";
    int line = 0;
    for(auto &ed : hed.extra_descriptions) {
        result += fmt::format("   {}: [{}] {}\r\n", line++, ed.first, ed.second);
    }
    return result;
}

std::optional<ExtraDescriptionView> find_exdesc(std::string_view word, std::span<ExtraDescriptionView> hed);
std::optional<ExtraDescriptionView> find_exdesc(std::string_view word, HasExtraDescriptions* hed);

void to_json(nlohmann::json& j, const HasExtraDescriptions& hed);
void from_json(const nlohmann::json& j, HasExtraDescriptions& hed);
