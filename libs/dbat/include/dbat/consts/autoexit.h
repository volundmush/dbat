#pragma once
/* Player autoexit levels: used as an index to exitlevels           */
constexpr int EXIT_OFF        = 0; /* Autoexit off                     */
constexpr int EXIT_NORMAL     = 1; /* Brief display (stock behaviour)  */
constexpr int EXIT_NA         = 2; /* Not implemented - do not use     */
constexpr int EXIT_COMPLETE   = 3; /* Full display                     */

#define char_exit_level(ch) (!char_is_npc(ch) ? (char_prf_flagged((ch),PRF_AUTOEXIT) ? 1 : 0 ) + (char_prf_flagged((ch),PRF_FULL_EXIT) ? 2 : 0 ) : 0 )
#define char_exit_level(ch) (char_exit_level(ch))