#include "dbat/game/Search.hpp"
#include "dbat/game/Character.hpp"
#include "dbat/game/Object.hpp"
#include "dbat/game/handler.hpp"
#include "dbat/game/AbstractLocation.hpp"

bool SearchManager::isNameMatch(HasInteractive* candidate, std::string_view needle) {
    auto keywords = candidate->getInteractivityKeywords(searcher);
    std::vector<std::string_view> kwviews;
    kwviews.reserve(keywords.size());
    for(auto& kw : keywords) {
        kwviews.emplace_back(kw);
    }
    return isname(needle, kwviews);
}

std::vector<std::weak_ptr<HasInteractive>> SearchManager::gatherCandidates(const SearchTarget& type) {
    std::vector<std::weak_ptr<HasInteractive>> out;
    if(std::holds_alternative<HasInventory*>(type)) {
        auto inv = std::get<HasInventory*>(type);
        for(const auto& item : inv->getInventory()) {
            out.emplace_back(item);
        }
    } else if(std::holds_alternative<HasEquipment*>(type)) {
        auto eq = std::get<HasEquipment*>(type);
        for(const auto& [slot, item] : eq->getEquipment()) {
            if(item) out.emplace_back(item->shared_from_this());
        }
    } else if(std::holds_alternative<Location>(type)) {
        auto loc = std::get<Location>(type);
        if(auto al = loc.getLoc()) {
            auto con = al->getContents<HasInteractive>(loc.position);
            out = con.snapshot_weak();
        }
    } else if(std::holds_alternative<SearchWorldObjects>(type)) {
        isGlobal = true;
        for(const auto& [id, obj] : Object::registry) {
            out.emplace_back(obj);
        }
    } else if(std::holds_alternative<SearchWorldCharacters>(type)) {
        isGlobal = true;
        for(const auto& [id, ch] : Character::registry) {
            out.emplace_back(ch);
        }
    }
    return out;
}
