#include "dbat/game/HasExtraDescriptions.hpp"
#include "dbat/game/handler.hpp"

ExtraDescriptionViews HasExtraDescriptions::getExtraDescription() const
{
    ExtraDescriptionViews out;
    for(const auto &ed : extra_descriptions) {
        out.emplace_back(std::string_view(ed.first), std::string_view(ed.second));
    }
    return out;
}


std::optional<ExtraDescriptionView> find_exdesc(std::string_view word, std::span<ExtraDescriptionView> hed) {
    if (hed.empty())
        return std::nullopt;

    for (const auto& exdesc : hed) {
        if (isname(word, exdesc.first)) {
            return exdesc;
        }
    }
    return std::nullopt;
}

std::optional<ExtraDescriptionView> find_exdesc(std::string_view word, HasExtraDescriptions* hed) {
    if (!hed)
        return std::nullopt;
    
    auto exdescs = hed->getExtraDescription();
    return find_exdesc(word, exdescs);
}