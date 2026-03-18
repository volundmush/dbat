#pragma once
#include <unordered_set>
#include "const/Race.hpp"
#include "const/Sensei.hpp"
#include "const/CharacterProperties.hpp"
#include "Flags.hpp"
#include <nlohmann/json_fwd.hpp>

struct HasPicky {
    FlagHandler<MoralAlign> only_alignment, not_alignment;    /* Neutral, lawful, etc.		*/
    FlagHandler<Sensei> only_sensei, not_sensei;    /* Only these classes can shop here	*/
    FlagHandler<Race> only_race, not_race;    /* Only these races can shop here	*/
};

inline std::string format_as(const HasPicky& hp) {
    std::vector<std::string> parts;
    auto addFlags = [&](const std::string& label, const auto& flags) {
        parts.emplace_back(fmt::format("    {}: [{}]", label, flags.getFlagNames()));
    };
    addFlags("Only Align", hp.only_alignment);
    addFlags("Not Align", hp.not_alignment);
    addFlags("Only Sensei", hp.only_sensei);
    addFlags("Not Sensei", hp.not_sensei);
    addFlags("Only Race", hp.only_race);
    addFlags("Not Race", hp.not_race);
    return fmt::format("Picky Data:\r\n{}", fmt::join(parts, "\r\n"));
}

using picky_data = HasPicky;

void to_json(nlohmann::json& j, const picky_data& p);
void from_json(const nlohmann::json& j, picky_data& p);
