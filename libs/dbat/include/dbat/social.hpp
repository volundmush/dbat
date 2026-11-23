#pragma once
#include <optional>
#include <string>
#include <string_view>

/* A social's message: none ("#" in the file) is not the same as an empty line, which it keeps */
using social_message = std::optional<std::string>;

/* m as text, empty for none */
inline std::string_view message_text(const social_message &m) { return m ? std::string_view(*m) : std::string_view(); }

/* used in the socials */
struct social_messg {
  int act_nr{};
  std::string command;       /* holds copy of activating command */
  std::string sort_as;       /* holds a copy of a similar command or
                               * abbreviation to sort by for the parser */
  int hide{};                /* ? */
  int min_victim_position{}; /* Position of victim */
  int min_char_position{};   /* Position of char */
  int min_level_char{};      /* Minimum level of socialing char */

  /* No argument was supplied */
  social_message char_no_arg;
  social_message others_no_arg;

  /* An argument was there, and a victim was found */
  social_message char_found;
  social_message others_found;
  social_message vict_found;

  /* An argument was there, as well as a body part, and a victim was found */
  social_message char_body_found;
  social_message others_body_found;
  social_message vict_body_found;

  /* An argument was there, but no victim was found */
  social_message not_found;

  /* The victim turned out to be the character */
  social_message char_auto;
  social_message others_auto;

  /* If the char cant be found search the char's inven and do these: */
  social_message char_obj_found;
  social_message others_obj_found;
};
