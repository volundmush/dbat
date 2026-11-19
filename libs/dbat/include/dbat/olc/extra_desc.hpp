#pragma once
#include <string_view>

#include "dbat/consts/types.hpp"
#include "dbat/extradesc.hpp"

/*
 * The extra descriptions submenu of redit and oedit: one description of a list at a time, its keywords and text,
 * and on to the next. It edits the list of the owning editor's copy of the room or object.
 */
struct extra_desc_editor {
  enum class mode { menu, keyword, description };

  /* How one editor's submenu reads and behaves: each had its own */
  struct style {
    void (*show)(GameWorld *g, struct descriptor_data *d, const extra_descr_data &desc, bool has_next);
    std::string_view keyword_prompt;
    std::string_view description_prompt;
    bool says_unfinished; /* going on from an unfinished description says why it can't, rather than nothing */
    bool others_leave;    /* a number not on the menu leaves it, as 0 does, rather than showing it again */
    int contexts[3];      /* the context help while asking for each mode */
  };

  const style *how{};
  ex_desc_list *list{};
  extra_descr_data *editing{};
  mode asking = mode::menu;

  /* Opens the submenu on the first description of list, making one if there is none. */
  void open(GameWorld *g, struct descriptor_data *d, ex_desc_list &descs);

  /* A line typed at the submenu; false when the builder leaves it, for the owner's menu. */
  bool parse(GameWorld *g, struct descriptor_data *d, std::string_view line);

  /* The text editor writing the description has closed. */
  void text_done(GameWorld *g, struct descriptor_data *d) { menu(g, d); }

  int help_context() const { return how->contexts[static_cast<int>(asking)]; }

private:
  void menu(GameWorld *g, struct descriptor_data *d);
  bool finished() const { return !editing->keyword.empty() && !editing->description.empty(); }
};
