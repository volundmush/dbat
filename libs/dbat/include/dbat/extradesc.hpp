#pragma once
#include <list>
#include <string>

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
  std::string keyword;     /* Keyword in look/examine */
  std::string description; /* What to see             */
};

/* An object's or room's extra descriptions, newest first, as the files list them. A list, so that OLC can hold one
 * while it edits it. */
using ex_desc_list = std::list<extra_descr_data>;

/* The description after desc in list, or null */
inline extra_descr_data *ex_desc_next(ex_desc_list &list, const extra_descr_data *desc)
{
  for (auto it = list.begin(); it != list.end(); ++it)
    if (&*it == desc)
      return ++it == list.end() ? nullptr : &*it;
  return nullptr;
}

/* Removes desc from list */
inline void ex_desc_remove(ex_desc_list &list, const extra_descr_data *desc)
{
  list.remove_if([desc](const extra_descr_data &each) { return &each == desc; });
}
