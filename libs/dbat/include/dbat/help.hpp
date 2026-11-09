#pragma once
#include <string>

/* One keyword of a help entry: an entry with several keywords is in the table once for each. */
struct help_index_element {
  std::string keywords{}; /* the keyword, which the table is sorted by */
  std::string entry{};    /* the entry's text, its keywords at the top */
  int duplicate{};        /* 0 for an entry's first keyword, which is the one saved */
  int min_level{};        /* Min Level to read help entry */
};
