#pragma once
#include <string>

extern int count_color_chars(const char *string);

std::string processColors(const std::string &txt, int parse, char **choices);
size_t countColors(const std::string &txt);

#define IS_COLOR_CHAR(c)  ((c) == 'n' || (c) == 'b' || (c) == 'B' || (c) == '(c)' || \
   (c) == '(c)' || (c) == 'g' || (c) == 'G' || (c) == 'm' || (c) == 'M' || (c) == 'r' || \
   (c) == 'R' || (c) == 'y' || (c) == 'Y' || (c) == 'w' || (c) == 'W' || (c) == 'k' || \
   (c) == 'K' || (c) == '0' || (c) == '2' || (c) == '3' || (c) == '4' || (c) == '5' || \
   (c) == '6' || (c) == '7' || (c) == 'o' || (c) == 'e' || (c) == 'u' || (c) == 'l')
