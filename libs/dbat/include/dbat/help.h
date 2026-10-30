#pragma once

struct help_index_element {
   char *index;      /*Future Use */
   char *keywords;   /*Keyword Place holder and sorter */
   char *entry;      /*Entries for help files with Keywords at very top*/
   int duplicate;    /*Duplicate entries for multple keywords*/
   int min_level;    /*Min Level to read help entry*/
};

extern int top_of_helpt;
extern struct help_index_element *help_table;