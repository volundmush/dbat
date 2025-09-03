#pragma once
#include <string>

struct help_index_element {
    char *index{};      /*Future Use */
    char *keywords{};   /*Keyword Place holder and sorter */
    char *entry{};      /*Entries for help files with Keywords at very top*/
    int duplicate{};    /*Duplicate entries for multple keywords*/
    int min_level{};    /*Min Level to read help entry*/
};

extern struct help_index_element *help_table;
extern int top_of_helpt;
extern help_index_element *get_help(const std::string& name, int level);

extern void free_help_table();