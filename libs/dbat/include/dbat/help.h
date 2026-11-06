#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct help_index_element {
  char *index;    /*Future Use */
  char *keywords; /*Keyword Place holder and sorter */
  char *entry;    /*Entries for help files with Keywords at very top*/
  int duplicate;  /*Duplicate entries for multple keywords*/
  int min_level;  /*Min Level to read help entry*/
};

#ifdef __cplusplus
}
#endif
