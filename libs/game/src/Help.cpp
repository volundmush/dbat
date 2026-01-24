#include "dbat/game/Typedefs.hpp"
#include "dbat/game/Help.hpp"

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