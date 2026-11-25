/*************************************************************************
*   File: colour.cpp                                    Part of CircleMUD *
*  Usage: Colour codes                                                   *
*         (formerly part of comm.c)                                      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>

#include "dbat/utils.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/random.hpp"
#include "dbat/screen.hpp"

#include "dbat/io/reader.hpp"

#include "comm_internal.hpp"

/*
  Codes are:      @n - normal
  @d - black      @D - gray           @0 - background black
  @b - blue       @B - bright blue    @1 - background blue
  @g - green      @G - bright green   @2 - background green
  @c - cyan       @C - bright cyan    @3 - background cyan
  @r - red        @R - bright red     @4 - background red
  @m - magneta    @M - bright magneta @5 - background magneta
  @y - yellow     @Y - bright yellow  @6 - background yellow
  @w - white      @W - bright white   @7 - background white
  @x - random
Extra codes:      @l - blink          @o - bold
  @u - underline  @e - reverse video  @@ - single @

  @[num] - user color choice num, [] are required
*/
namespace {

/* Each code and what it becomes: a replacement starting with a digit is the inside of an ANSI escape */
struct colour_code {
  char code;
  std::string_view ansi{};
};
constexpr colour_code CODES[] = {
    {'@', "@"},
    {'d', AA_NORMAL ANSISEPSTR AF_BLACK},
    {'b', AA_NORMAL ANSISEPSTR AF_BLUE},
    {'g', AA_NORMAL ANSISEPSTR AF_GREEN},
    {'c', AA_NORMAL ANSISEPSTR AF_CYAN},
    {'r', AA_NORMAL ANSISEPSTR AF_RED},
    {'m', AA_NORMAL ANSISEPSTR AF_MAGENTA},
    {'y', AA_NORMAL ANSISEPSTR AF_YELLOW},
    {'w', AA_NORMAL ANSISEPSTR AF_WHITE},
    {'D', AA_BOLD ANSISEPSTR AF_BLACK},
    {'B', AA_BOLD ANSISEPSTR AF_BLUE},
    {'G', AA_BOLD ANSISEPSTR AF_GREEN},
    {'C', AA_BOLD ANSISEPSTR AF_CYAN},
    {'R', AA_BOLD ANSISEPSTR AF_RED},
    {'M', AA_BOLD ANSISEPSTR AF_MAGENTA},
    {'Y', AA_BOLD ANSISEPSTR AF_YELLOW},
    {'W', AA_BOLD ANSISEPSTR AF_WHITE},
    {'0', AB_BLACK},
    {'1', AB_BLUE},
    {'2', AB_GREEN},
    {'3', AB_CYAN},
    {'4', AB_RED},
    {'5', AB_MAGENTA},
    {'6', AB_YELLOW},
    {'7', AB_WHITE},
    {'l', AA_BLINK},
    {'u', AA_UNDERLINE},
    {'o', AA_BOLD},
    {'e', AA_REVERSE},
};

/* What @x picks from */
constexpr std::string_view RANDOM_COLORS = "bgcrmywBGCRMWY";

/* A user's colour choice i (@n is choice COLOR_NORMAL), or the default (always, when choices is empty) */
std::string_view choice(std::span<const std::string> choices, int i)
{
  return !choices.empty() && !choices[i].empty() ? until_nul(choices[i]) : std::string_view(default_color_choices[i]);
}

/* The number atoi() reads at txt[at]: leading spaces, a sign, then digits (atoi() is strtol() cut to an int) */
int number_at(std::string_view txt, size_t at) { return static_cast<int>(io::c_atol(txt.substr(at))); }

} // namespace

colourized colourize(GameWorld *g, std::string_view txt, bool parse, std::span<const std::string> choices, size_t maxlen)
{
  if (txt.find('@') == std::string_view::npos) /* skip out if no color codes */
    return {std::string(txt), txt.size()};

  std::string out;
  size_t at = 0;
  /* Leave room for what was the terminating NUL: every step below writes at most one byte, except a color
   * substitution, which checks its own length. */
  while (at < txt.size() && out.size() + 1 < maxlen) {
    /* no color code - just copy */
    if (txt[at] != '@') {
      out += txt[at++];
      continue;
    }

    /* if we get here we have a color code */
    char code = ++at < txt.size() ? txt[at] : '\0';

    /* look for a random color code picks a random number between 1 and 14 */
    if (code == 'x')
      code = RANDOM_COLORS[rand_number(g, 0, 13)];

    if (code == '\0') { /* string was terminated with color code - just put it in */
      out += '@';
      continue;
    }

    /* the end of a @[num] code: past its digits, or at its last character if they end the text */
    auto skip_digits = [&] {
      while (at < txt.size() && isdigit(txt[at]))
        at++;
      if (at == txt.size())
        at--;
    };

    if (!parse) { /* not parsing, just skip the code, unless it's @@ */
      if (code == '@')
        out += '@';
      if (code == '[') { /* Multi-character code */
        at++;
        skip_digits();
      }
      at++; /* skip to next (non-colorcode) char */
      continue;
    }

    /* parse the color code */
    std::string_view replacement; /* none, if empty: an empty one would add nothing */
    if (code == '[') { /* User configurable color */
      if (++at == txt.size())
        break; /* "@[" ending the text: nothing more (it once read on past the end, into what was there before) */
      int i = number_at(txt, at); /* quirk: atoi()'s rules, leading spaces and a sign too */
      if (i < 0 || i >= NUM_COLOR)
        i = COLOR_NORMAL;
      replacement = choice(choices, i);
      skip_digits();
    } else if (code == 'n') {
      replacement = choice(choices, COLOR_NORMAL);
    } else {
      for (const colour_code &c : CODES) /* do we find it ? */
        if (c.code == code) {
          replacement = c.ansi;
          break;
        }
    }
    if (!replacement.empty()) {
      std::string_view r = replacement;
      if (out.size() + r.size() + std::string_view(ANSISTART).size() + 1 < maxlen) { /* only substitute if there's room for the whole code */
        if (isdigit(r[0]))
          out += ANSISTART;
        out += r;
        if (isdigit(r[0]))
          out += (char)ANSIEND;
      }
    }
    /* If we couldn't find any correct color code, or we found it and
    * substituted above, let's just process the next character.
    * - Welcor
    */
    at++;
  }

  /* see if we wanted more space */
  size_t wanted = out.size() + (txt.size() - std::min(at, txt.size()));
  return {std::move(out), wanted};
}
