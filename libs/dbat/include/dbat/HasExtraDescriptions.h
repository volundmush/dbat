#pragma once
#include <string>
#include <vector>
#include <optional>
#include <span>

using ExtraDescription = std::pair<std::string, std::string>;
using ExtraDescriptions = std::vector<ExtraDescription>;

using ExtraDescriptionView = std::pair<std::string_view, std::string_view>;
using ExtraDescriptionViews = std::vector<ExtraDescriptionView>;


struct HasExtraDescriptions {
    ExtraDescriptionViews getExtraDescription() const; // Returns the extra description data.
    ExtraDescriptions extra_descriptions; // Extra descriptions for this unit.
};
std::optional<ExtraDescriptionView> find_exdesc(std::string_view word, std::span<ExtraDescriptionView> hed);
std::optional<ExtraDescriptionView> find_exdesc(std::string_view word, HasExtraDescriptions* hed);