#include "dbat/game/Typedefs.hpp"
#include "dbat/game/Help.hpp"
#include <nlohmann/json.hpp>

#include <boost/algorithm/string.hpp>

std::vector<help_index_element> help_table;

int search_help(std::string_view argument, int level)
{
    if (help_table.empty() || argument.empty())
        return NOTHING;

    for(auto i = 0; i < help_table.size(); ++i) {
        if(boost::iequals(help_table[i].keywords, argument)) {
            if(level >= help_table[i].min_level) {
                return i;
            }
        }
    }
    return NOTHING;
}

struct help_index_element *get_help(std::string_view name, int level) {
    auto results = search_help(name, level);
    if (results != NOTHING)
        return &help_table[results];
    
    return nullptr;
}

void to_json(nlohmann::json &j, const help_index_element &a)
{
    j["index"] = a.index;
    if (!a.keywords.empty())
        j["keywords"] = a.keywords;
    if (!a.entry.empty())
        j["entry"] = a.entry;
    if (a.duplicate != NOTHING)
        j["duplicate"] = a.duplicate;
    if (a.min_level != 0)
        j["min_level"] = a.min_level;
}

void from_json(const nlohmann::json &j, help_index_element &a)
{
    if (j.contains(+"index")) j.at(+"index").get_to(a.index);
    if (j.contains(+"keywords")) j.at(+"keywords").get_to(a.keywords);
    if (j.contains(+"entry")) j.at(+"entry").get_to(a.entry);
    if (j.contains(+"duplicate")) j.at(+"duplicate").get_to(a.duplicate);
    if (j.contains(+"min_level")) j.at(+"min_level").get_to(a.min_level);
}
