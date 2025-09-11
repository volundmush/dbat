#include "dbat/UID.h"
#include <regex>

#include "dbat/Room.h"
#include "dbat/Object.h"
#include "dbat/Character.h"
#include "dbat/HasDgScripts.h"

// ^#(?<id>\d+)(?::(?<generation>\d+)?)?
static std::regex uid_regex(R"(^#(R|O|C)(\d+)(!)?)", std::regex::icase);

bool isUID(std::string_view uid) {
    return std::regex_match(std::string(uid), uid_regex);
}

std::shared_ptr<HasDgScripts> resolveUID(std::string_view uid) {
    // First we need to check if it matches or not.
    std::smatch match;

    std::string tomatch(uid);

    if(!std::regex_search(tomatch, match, uid_regex)) {
        return nullptr;
    }

    std::string letter = match[1].str();// First capture group
    boost::to_upper(letter);
    int64_t id = std::stoll(match[2].str()); // Second capture group
    bool active = match[3].matched; // Third capture group

    if(letter == "R") {
        // Room
        if(auto find = Room::registry.find(id); find != Room::registry.end()) {
            if(active && !find->second->isActive()) return nullptr;
            return find->second;
        }
    } else if(letter == "O") {
        // Object
        if(auto find = Object::registry.find(id); find != Object::registry.end()) {
            if(active && !find->second->isActive()) return nullptr;
            return find->second;
        }
    } else if(letter == "C") {
        // Character
        if(auto find = Character::registry.find(id); find != Character::registry.end()) {
            if(active && !find->second->isActive()) return nullptr;
            return find->second;
        }
    }

    return nullptr;
}