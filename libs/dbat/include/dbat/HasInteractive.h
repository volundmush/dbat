#pragma once
#include <vector>
#include <string>

struct HasInteractive {
    virtual std::vector<std::string> getInteractivityKeywords(struct Character* viewer) = 0;
    virtual std::string getDisplayName(struct Character* viewer, bool capitalizeArticle = false) = 0;
    virtual bool isVisibleTo(Character* viewer) = 0;
};