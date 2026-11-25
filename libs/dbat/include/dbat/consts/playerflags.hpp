#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table player_bits; /* flag_count<player_flag> of them */

/* Player flags: player_special_data::player_flags (NPCs have none) */
enum class player_flag : uint8_t {
  killer       = 0,   /* Player is a player-killer        */
  thief        = 1,   /* Player is a player-thief         */
  frozen       = 2,   /* Player is frozen                 */
  dontset      = 3,   /* Don't EVER set (ISNPC bit) 	*/
  writing      = 4,   /* Player writing (board/mail/olc)  */
  mailing      = 5,   /* Player is writing mail           */
  crash        = 6,   /* Player needs to be crash-saved   */
  siteok       = 7,   /* Player has been site-cleared     */
  noshout      = 8,   /* Player not allowed to shout/goss */
  notitle      = 9,   /* Player not allowed to set title  */
  deleted      = 10,  /* Player deleted - space reusable  */
  loadroom     = 11,  /* Player uses nonstandard loadroom */
  nowizlist    = 12,  /* Player shouldn't be on wizlist  	*/
  nodelete     = 13,  /* Player shouldn't be deleted     	*/
  invstart     = 14,  /* Player should enter game wizinvis*/
  cryo         = 15,  /* Player is cryo-saved (purge prog)*/
  notdeadyet   = 16,  /* (R) Player being extracted.     	*/
  agemid_g     = 17,  /* Player has had pos of middle age	*/
  agemid_b     = 18,  /* Player has had neg of middle age	*/
  ageold_g     = 19,  /* Player has had pos of old age	*/
  ageold_b     = 20,  /* Player has had neg of old age	*/
  ageven_g     = 21,  /* Player has had pos of venerable age	*/
  ageven_b     = 22,  /* Player has had neg of venerable age	*/
  oldage       = 23,  /* Player is dead of old age	*/
  rarm         = 24,  /* Player has a right arm           */
  larm         = 25,  /* Player has a left arm            */
  rleg         = 26,  /* Player has a right leg           */
  lleg         = 27,  /* Player has a left leg            */
  head         = 28,  /* Player has a head                */
  stail        = 29,  /* Player has a saiyan tail         */
  tail         = 30,  /* Player has a non-saiyan tail     */
  piloting     = 31,  /* Player is sitting in the pilots chair */
  skillp       = 32,  /* Player made a good choice in CC  */
  spar         = 33,  /* Player is in a spar stance       */
  charge       = 34,  /* Player is charging               */
  trans1       = 35,  /* Transformation 1                 */
  trans2       = 36,  /* Transformation 2                 */
  trans3       = 37,  /* Transformation 3                 */
  trans4       = 38,  /* Transformation 4                 */
  trans5       = 39,  /* Transformation 5                 */
  trans6       = 40,  /* Transformation 6                 */
  absorb       = 41,  /* Absorb model                     */
  repair       = 42,  /* Repair model                     */
  sensem       = 43,  /* Sense-Powersense model           */
  powerup      = 44,  /* Powering Up                      */
  knocked      = 45,  /* Knocked OUT                      */
  crarm        = 46,  /* Cybernetic Right Arm             */
  clarm        = 47,  /* Cybernetic Left Arm              */
  crleg        = 48,  /* Cybernetic Right Leg             */
  clleg        = 49,  /* Cybernetic Left Leg              */
  fpssj        = 50,  /* Full Power Super Saiyan          */
  immortal     = 51,  /* The player is immortal           */
  eyec         = 52,  /* The player has their eyes closed */
  disguised    = 53,  /* The player is disguised          */
  bandaged     = 54,  /* THe player has been bandaged     */
  pr           = 55,  /* Has had their potential released */
  healt        = 56,  /* Is inside a healing tank         */
  fury         = 57,  /* Is in fury mode                  */
  pose         = 58,  /* Ginyu Pose Effect - NO LONGER USED */
  oozaru       = 59,
  absorbed     = 60,
  multp        = 61,
  pdeath       = 62,
  thandw       = 63,
  selfd        = 64,
  selfd2       = 65,
  spiral       = 66,
  biogr        = 67,
  lssj         = 68,
  replearn     = 69,
  forget       = 70,
  transmission = 71,
  fishing      = 72,
  goop         = 73,
  multihit     = 74,
  auralight    = 75,
  rdisplay     = 76,
  stolen       = 77,
  tailhide     = 78,  /* Hides tail for S & HB            */
  nogrow       = 79,  /* Halt Growth for S & HB           */
  count_       = 80,
};
template <> struct flag_traits<player_flag> {
  static constexpr const name_table &names = player_bits;
};


