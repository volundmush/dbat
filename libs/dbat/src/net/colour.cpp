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

#include <cstring>

#include "dbat/utils.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/screen.hpp"

#include "comm_internal.hpp"

/* Color replacement arrays. Orig. Renx -- 011100, now modified */
static const char *const ANSI[] = {
    "@",
    AA_NORMAL,
    AA_NORMAL ANSISEPSTR AF_BLACK,
    AA_NORMAL ANSISEPSTR AF_BLUE,
    AA_NORMAL ANSISEPSTR AF_GREEN,
    AA_NORMAL ANSISEPSTR AF_CYAN,
    AA_NORMAL ANSISEPSTR AF_RED,
    AA_NORMAL ANSISEPSTR AF_MAGENTA,
    AA_NORMAL ANSISEPSTR AF_YELLOW,
    AA_NORMAL ANSISEPSTR AF_WHITE,
    AA_BOLD ANSISEPSTR AF_BLACK,
    AA_BOLD ANSISEPSTR AF_BLUE,
    AA_BOLD ANSISEPSTR AF_GREEN,
    AA_BOLD ANSISEPSTR AF_CYAN,
    AA_BOLD ANSISEPSTR AF_RED,
    AA_BOLD ANSISEPSTR AF_MAGENTA,
    AA_BOLD ANSISEPSTR AF_YELLOW,
    AA_BOLD ANSISEPSTR AF_WHITE,
    AB_BLACK,
    AB_BLUE,
    AB_GREEN,
    AB_CYAN,
    AB_RED,
    AB_MAGENTA,
    AB_YELLOW,
    AB_WHITE,
    AA_BLINK,
    AA_UNDERLINE,
    AA_BOLD,
    AA_REVERSE,
    "!"};

static const char CCODE[] = "@ndbgcrmywDBGCRMYW01234567luoex!";
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
static const char RANDOM_COLORS[] = "bgcrmywBGCRMWY";

#define NEW_STRING_LENGTH (size_t)(dest_char - save_pos)
size_t proc_colors(GameWorld *g, char *txt, size_t maxlen, int parse, const std::string *choices)
{
  char *dest_char, *source_char, *save_pos;
  const char *color_char, *replacement = nullptr;
  int i, temp_color;
  size_t wanted;

  if (!txt || !strchr(txt, '@')) /* skip out if no color codes     */
    return strlen(txt);

  source_char = txt;
  dest_char   = str_alloc(maxlen);
  save_pos = dest_char;
  /* Leave room for the terminating NUL: every step below writes at most one byte, except a color substitution,
   * which checks its own length. */
  for (; *source_char && (NEW_STRING_LENGTH + 1 < maxlen);) {
    /* no color code - just copy */
    if (*source_char != '@') {
      *dest_char++ = *source_char++;
      continue;
    }

    /* if we get here we have a color code */

    source_char++; /* source_char now points to the code */

    /* look for a random color code picks a random number between 1 and 14 */
    if (*source_char == 'x') {
      temp_color   = rand_number(g, 0, 13);
      *source_char = RANDOM_COLORS[temp_color];
    }

    if (*source_char == '\0') { /* string was terminated with color code - just put it in */
      *dest_char++ = '@';
      /* source_char will now point to '\0' in the for() check */
      continue;
    }

    if (!parse) { /* not parsing, just skip the code, unless it's @@ */
      if (*source_char == '@') {
        *dest_char++ = '@';
      }
      if (*source_char == '[') { /* Multi-character code */
        source_char++;
        while (*source_char && isdigit(*source_char))
          source_char++;
        if (!*source_char)
          source_char--;
      }
      source_char++; /* skip to next (non-colorcode) char */
      continue;
    }

    /* parse the color code */
    if (*source_char == '[') { /* User configurable color */
      source_char++;
      if (*source_char) {
        i = atoi(source_char);
        if (i < 0 || i >= NUM_COLOR)
          i = COLOR_NORMAL;
        replacement = default_color_choices[i];
        if (choices && !choices[i].empty())
          replacement = choices[i].c_str();
        while (*source_char && isdigit(*source_char))
          source_char++;
        if (!*source_char)
          source_char--;
      }
    } else if (*source_char == 'n') {
      replacement = default_color_choices[COLOR_NORMAL];
      if (choices && !choices[COLOR_NORMAL].empty())
        replacement = choices[COLOR_NORMAL].c_str();
    } else {
      for (i = 0; CCODE[i] != '!'; i++) { /* do we find it ? */
        if ((*source_char) == CCODE[i]) { /* if so :*/
          replacement = ANSI[i];
          break;
        }
      }
    }
    if (replacement) {
      if (NEW_STRING_LENGTH + strlen(replacement) + strlen(ANSISTART) + 1 < maxlen) { /* only substitute if there's room for the whole code */
        if (isdigit(replacement[0]))
          for (color_char = ANSISTART; *color_char;)
            *dest_char++ = *color_char++;
        for (color_char = replacement; *color_char;)
          *dest_char++ = *color_char++;
        if (isdigit(replacement[0]))
          *dest_char++ = ANSIEND;
      }
      replacement = nullptr;
    }
    /* If we couldn't find any correct color code, or we found it and
    * substituted above, let's just process the next character.
    * - Welcor
    */
    source_char++;

  } /* for loop */

  /* make sure output is NULL - terminated */
  *dest_char = '\0';

  wanted = strlen(source_char); /* see if we wanted more space */
  strncpy(txt, save_pos, maxlen - 1);
  wanted += NEW_STRING_LENGTH;
  free(save_pos); /* plug memory leak */

  return wanted;
}
#undef NEW_STRING_LENGTH
