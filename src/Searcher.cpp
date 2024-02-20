#include "dbat/structs.h"
#include "dbat/utils.h"

Searcher& Searcher::setAllowAll(bool allow) {
    allowAll = allow;
    return *this;
}

Searcher& Searcher::setAllowSelf(bool allow) {
    allowSelf = allow;
    return *this;
}

Searcher& Searcher::setAllowHere(bool allow) {
    allowHere = allow;
    return *this;
} 

Searcher& Searcher::setAllowRecurse(bool allow) {
    allowRecurse = allow;
    return *this;
}

Searcher& Searcher::setAllowAsterisk(bool allow) {
    allowAsterisk = allow;
    return *this;
}


std::vector<GameEntity*> Searcher::search() {
    trim(args);
    if(args.empty()) return {};
    if(allowSelf && iequals(args, "self")) return {caller};
    if(allowHere && iequals(args, "here")) return {caller->getLocation()};

    auto candidates = doSearch();
    
    int counter = 0;
    int prefix = 1;
    bool allMode = false;
    std::string targetName;
    
    // args might be formatted like "blah" or like "5.blah" or "all.blah".
    // We need to split by the first . if it exists.
    // Then we check if it's a number or all.

    if(auto dot = args.find('.'); dot != std::string::npos) {
        auto prefixStr = args.substr(0, dot);
        if(iequals(prefixStr, "all")) {
            allMode = allowAll;
            if(!allMode) {
                caller->sendLine("You are not allowed to use 'all' in this context.");
                return {};
            }
        } else {
            prefix = std::stoi(prefixStr);
        }
        targetName = args.substr(dot+1);
    } else {
        targetName = args;
    }

    if(allowAsterisk && iequals(targetName, "*")) {
        return candidates;
    }

    for(auto c : candidates) {
        auto keywords = c->getKeywords(caller);

        for(auto k : keywords) {
            if(iequals(k, targetName)) {
                if(counter == prefix) {
                    return {c};
                }
                counter++;
                break;
            }
        }

    }

    return {};
}

GameEntity* Searcher::getOne() {
    auto results = search();
    if(results.size() == 1) return results.front();
    return nullptr;
}