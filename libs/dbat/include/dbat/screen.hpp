#pragma once
#include "dbat/consts/types.hpp"

#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"
#define KNUL ""

/* conditional color.  pass it a pointer to a char_data and a color level. */
constexpr int C_OFF = 0;
constexpr int C_ON  = 1;
constexpr int C_BRI = C_ON; /* Compatibility hack */
constexpr int C_NRM = C_ON; /* Compatibility hack */
constexpr int C_CMP = C_ON; /* Compatibility hack */

#define COLOR_CHOICES(ch) (char_is_npc(ch) ? nullptr : char_get_player_specials(ch) ? char_get_player_specials(ch)->color_choices \
                                                                           : nullptr)
#define _clrlevel(g, ch)  (!char_is_npc(ch) ? (char_prf_flagged(g, (ch), PRF_COLOR) ? 1 : 0) : 0)
#define clr(g, ch, lvl)   (_clrlevel(g, ch) >= (lvl))
#define CCNRM(g, ch, lvl) (clr(g, (ch), (lvl)) ? KNRM : KNUL)
#define CCRED(g, ch, lvl) (clr(g, (ch), (lvl)) ? KRED : KNUL)
#define CCGRN(g, ch, lvl) (clr(g, (ch), (lvl)) ? KGRN : KNUL)
#define CCYEL(g, ch, lvl) (clr(g, (ch), (lvl)) ? KYEL : KNUL)
#define CCBLU(g, ch, lvl) (clr(g, (ch), (lvl)) ? KBLU : KNUL)
#define CCMAG(g, ch, lvl) (clr(g, (ch), (lvl)) ? KMAG : KNUL)
#define CCCYN(g, ch, lvl) (clr(g, (ch), (lvl)) ? KCYN : KNUL)
#define CCWHT(g, ch, lvl) (clr(g, (ch), (lvl)) ? KWHT : KNUL)

#define COLOR_LEV(g, ch) (_clrlevel(g, ch))

#define QNRM CCNRM(g, ch, C_BRI)
#define QRED CCRED(g, ch, C_BRI)
#define QGRN CCGRN(g, ch, C_BRI)
#define QYEL CCYEL(g, ch, C_BRI)
#define QBLU CCBLU(g, ch, C_BRI)
#define QMAG CCMAG(g, ch, C_BRI)
#define QCYN CCCYN(g, ch, C_BRI)
#define QWHT CCWHT(g, ch, C_BRI)

#define ANSISTART "\x1B["
constexpr int ANSISEP = ';';
#define ANSISEPSTR ";"
constexpr int ANSIEND = 'm';
#define ANSIENDSTR "m"

/* Attributes */
#define AA_NORMAL    "0"
#define AA_BOLD      "1"
#define AA_UNDERLINE "4"
#define AA_BLINK     "5"
#define AA_REVERSE   "7"
#define AA_INVIS     "8"
/* Foreground colors */
#define AF_BLACK   "30"
#define AF_RED     "31"
#define AF_GREEN   "32"
#define AF_YELLOW  "33"
#define AF_BLUE    "34"
#define AF_MAGENTA "35"
#define AF_CYAN    "36"
#define AF_WHITE   "37"
/* Background colors */
#define AB_BLACK   "40"
#define AB_RED     "41"
#define AB_GREEN   "42"
#define AB_YELLOW  "43"
#define AB_BLUE    "44"
#define AB_MAGENTA "45"
#define AB_CYAN    "46"
#define AB_WHITE   "47"

