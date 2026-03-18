#pragma once
#include <vector>
#include <string>
#include <nlohmann/json_fwd.hpp>

struct help_index_element {
    std::string index{};      /*Future Use */
    std::string keywords{};   /*Keyword Place holder and sorter */
    std::string entry{};      /*Entries for help files with Keywords at very top*/
    int duplicate{};    /*Duplicate entries for multple keywords*/
    int min_level{};    /*Min Level to read help entry*/
};

extern std::vector<help_index_element> help_table;
extern help_index_element *get_help(std::string_view name, int level);

int search_help(std::string_view argument, int level);

void to_json(nlohmann::json& j, const help_index_element& a);
void from_json(const nlohmann::json& j, help_index_element& a);
