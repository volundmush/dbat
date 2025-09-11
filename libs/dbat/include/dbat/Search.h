#pragma once
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <variant>

#include "Result.h"
#include "Location.h"
#include "handler.h"

struct Character;
struct Object;
struct HasInteractive;
struct HasInventory;
struct HasEquipment;

struct SearchWorldObjects {};
struct SearchWorldCharacters {};

using SearchTarget = std::variant<HasInventory*, HasEquipment*, Location, SearchWorldObjects, SearchWorldCharacters>;

// A generic searcher that's used for commands like "get" and "look" among others.
// It sequentially searches provided places until it gets a match. The order that
// .addInventory(), .addEquipment(), etc, are called is important.
class SearchManager {
    Character* searcher;
    // it may seem strange to use Location here but since it can store an entt::entity and also represent
    // a Location, it's perfect. Ignore the weird name.
    std::vector<SearchTarget> searchSequence;
    // a custom filter.
    std::function<bool(Character* searcher, HasInteractive* target)> filterFunc{nullptr};

    // if true, enables "me" and "self" (case insensitive) to instantly return the searcher.
    // That only works in findFirst mode though.
    bool allowSelf{true};

    // this will be set true by calling addWorld. It is only useful for searching for things by their
    // Entity ID or if allowSelf is true.
    bool isGlobal{false};

    // We'll run something like isname() here but probably generate the keywords dynamically based on
    // the searcher's perspective of candidate.
    bool isNameMatch(HasInteractive* candidate, std::string_view needle);

    std::vector<std::weak_ptr<HasInteractive>> gatherCandidates(const SearchTarget& type);

    template<typename T = HasInteractive>
    std::shared_ptr<T> findFirstHelper(int& number, std::string_view needle, std::vector<std::weak_ptr<HasInteractive>> candidates) {
        for (const auto& candidate : candidates) {
            auto entity = dynamic_cast<T*>(candidate.lock().get());
            if(!entity) continue;
            if(!entity->isVisibleTo(searcher)) continue;
            if(filterFunc && !filterFunc(searcher, entity)) continue;
            if(boost::iequals(needle, "*") || isNameMatch(entity, needle)) number--;
            if(number <= 0) return entity;
        }
        return nullptr;
    }

    template<typename T = HasInteractive, typename... Cs>
    std::weak_ptr<T> findFirst(std::string_view needle, int& number) {
        // iterate through searchSequences the same way that findAll does and keep running
        // findFirstHelper until we get a non-null entity or run out of searchsequences...
        for(const auto& stype : searchSequence) {
            auto candidates = gatherCandidates(stype);
            auto found = findFirstHelper<T>(number, needle, candidates);
            if(found) {
                return found;
            }
        }
        // fallback...
        return nullptr;
    }

    template<typename T>
    std::vector<std::weak_ptr<T>> findAllHelper(std::string_view needle, std::vector<std::weak_ptr<HasInteractive>> candidates) {
        std::vector<std::weak_ptr<T>> out;
        for(const auto& candidate : candidates) {
            auto entity = std::dynamic_pointer_cast<T>(candidate.lock());
            if(!entity) continue;
            if(!entity->isVisibleTo(searcher)) continue;
            if(filterFunc && !filterFunc(searcher, entity)) continue;
            if(boost::iequals(needle, "*") || isNameMatch(entity, needle)) {
                out.push_back(entity);
            }
        }
        return out;
    }

    public:
    SearchManager(Character* searcher) : searcher(searcher) {}

    SearchManager& addInventory(HasInventory* ent);
    SearchManager& addEquipment(HasEquipment* ent);
    SearchManager& addLocation(const Location& loc);
    SearchManager& addWorldObjects();
    SearchManager& addWorldCharacters();
    SearchManager& withFilter(std::function<bool(Character* searcher, HasInteractive* target)> filter);

    template<typename T = HasInteractive>
    Result<std::vector<std::weak_ptr<T>>> findAll(std::string_view needle) {
        std::vector<std::weak_ptr<T>> results;
        boost::trim(needle);

        for(const auto& stype : searchSequence) {
            auto candidates = gatherCandidates(stype);
            auto found = findAllHelper<T>(needle, candidates);
            results.insert(results.end(), found.begin(), found.end());
        }

        if(results.empty()) {
            return err("No matches found for '{}'.", needle);
        }
        return results;
    }

    template<typename T = HasInteractive>
    Result<std::weak_ptr<T>> findOne(std::string_view needle) {
        boost::trim(needle);
        if(isGlobal && boost::istarts_with(needle, "#")) {
            // This is a global search for an EntityID. Only admin should be able to do this...
            // TODO: implement this and add more checks.
        }

        // we'll either be searching for the first or th n'th thing...
        auto [num, search] = splitSearchNumber(needle);

        auto res = findFirst<T>(search, num);
        if(!res) {
            return err("No match found for '{}'.", needle);
        }
        return res;
    }


    template<typename T = HasInteractive>
    Result<std::vector<std::weak_ptr<T>>> find(std::string_view needle) {
        if(boost::istarts_with(needle, "all.") || boost::istarts_with(needle, "*.")) {
            // rest should be after the dot...
            auto rest = needle.substr(needle.find('.') + 1);
            boost::trim(rest);
            return findAll<T>(rest);
        }
        auto result = findOne<T>(needle);
        if(result) {
            std::vector<std::weak_ptr<T>> out;
            out.push_back(result);
            return out;
        }
        return err(result.error());
    }

};