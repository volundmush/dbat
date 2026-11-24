#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table player_bits; /* NUM_PLR_FLAGS of them */

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

/* The old names, until every use says player_flag:: */
constexpr player_flag PLR_KILLER       = player_flag::killer;
constexpr player_flag PLR_THIEF        = player_flag::thief;
constexpr player_flag PLR_FROZEN       = player_flag::frozen;
constexpr player_flag PLR_DONTSET      = player_flag::dontset;
constexpr player_flag PLR_WRITING      = player_flag::writing;
constexpr player_flag PLR_MAILING      = player_flag::mailing;
constexpr player_flag PLR_CRASH        = player_flag::crash;
constexpr player_flag PLR_SITEOK       = player_flag::siteok;
constexpr player_flag PLR_NOSHOUT      = player_flag::noshout;
constexpr player_flag PLR_NOTITLE      = player_flag::notitle;
constexpr player_flag PLR_DELETED      = player_flag::deleted;
constexpr player_flag PLR_LOADROOM     = player_flag::loadroom;
constexpr player_flag PLR_NOWIZLIST    = player_flag::nowizlist;
constexpr player_flag PLR_NODELETE     = player_flag::nodelete;
constexpr player_flag PLR_INVSTART     = player_flag::invstart;
constexpr player_flag PLR_CRYO         = player_flag::cryo;
constexpr player_flag PLR_NOTDEADYET   = player_flag::notdeadyet;
constexpr player_flag PLR_AGEMID_G     = player_flag::agemid_g;
constexpr player_flag PLR_AGEMID_B     = player_flag::agemid_b;
constexpr player_flag PLR_AGEOLD_G     = player_flag::ageold_g;
constexpr player_flag PLR_AGEOLD_B     = player_flag::ageold_b;
constexpr player_flag PLR_AGEVEN_G     = player_flag::ageven_g;
constexpr player_flag PLR_AGEVEN_B     = player_flag::ageven_b;
constexpr player_flag PLR_OLDAGE       = player_flag::oldage;
constexpr player_flag PLR_RARM         = player_flag::rarm;
constexpr player_flag PLR_LARM         = player_flag::larm;
constexpr player_flag PLR_RLEG         = player_flag::rleg;
constexpr player_flag PLR_LLEG         = player_flag::lleg;
constexpr player_flag PLR_HEAD         = player_flag::head;
constexpr player_flag PLR_STAIL        = player_flag::stail;
constexpr player_flag PLR_TAIL         = player_flag::tail;
constexpr player_flag PLR_PILOTING     = player_flag::piloting;
constexpr player_flag PLR_SKILLP       = player_flag::skillp;
constexpr player_flag PLR_SPAR         = player_flag::spar;
constexpr player_flag PLR_CHARGE       = player_flag::charge;
constexpr player_flag PLR_TRANS1       = player_flag::trans1;
constexpr player_flag PLR_TRANS2       = player_flag::trans2;
constexpr player_flag PLR_TRANS3       = player_flag::trans3;
constexpr player_flag PLR_TRANS4       = player_flag::trans4;
constexpr player_flag PLR_TRANS5       = player_flag::trans5;
constexpr player_flag PLR_TRANS6       = player_flag::trans6;
constexpr player_flag PLR_ABSORB       = player_flag::absorb;
constexpr player_flag PLR_REPAIR       = player_flag::repair;
constexpr player_flag PLR_SENSEM       = player_flag::sensem;
constexpr player_flag PLR_POWERUP      = player_flag::powerup;
constexpr player_flag PLR_KNOCKED      = player_flag::knocked;
constexpr player_flag PLR_CRARM        = player_flag::crarm;
constexpr player_flag PLR_CLARM        = player_flag::clarm;
constexpr player_flag PLR_CRLEG        = player_flag::crleg;
constexpr player_flag PLR_CLLEG        = player_flag::clleg;
constexpr player_flag PLR_FPSSJ        = player_flag::fpssj;
constexpr player_flag PLR_IMMORTAL     = player_flag::immortal;
constexpr player_flag PLR_EYEC         = player_flag::eyec;
constexpr player_flag PLR_DISGUISED    = player_flag::disguised;
constexpr player_flag PLR_BANDAGED     = player_flag::bandaged;
constexpr player_flag PLR_PR           = player_flag::pr;
constexpr player_flag PLR_HEALT        = player_flag::healt;
constexpr player_flag PLR_FURY         = player_flag::fury;
constexpr player_flag PLR_POSE         = player_flag::pose;
constexpr player_flag PLR_OOZARU       = player_flag::oozaru;
constexpr player_flag PLR_ABSORBED     = player_flag::absorbed;
constexpr player_flag PLR_MULTP        = player_flag::multp;
constexpr player_flag PLR_PDEATH       = player_flag::pdeath;
constexpr player_flag PLR_THANDW       = player_flag::thandw;
constexpr player_flag PLR_SELFD        = player_flag::selfd;
constexpr player_flag PLR_SELFD2       = player_flag::selfd2;
constexpr player_flag PLR_SPIRAL       = player_flag::spiral;
constexpr player_flag PLR_BIOGR        = player_flag::biogr;
constexpr player_flag PLR_LSSJ         = player_flag::lssj;
constexpr player_flag PLR_REPLEARN     = player_flag::replearn;
constexpr player_flag PLR_FORGET       = player_flag::forget;
constexpr player_flag PLR_TRANSMISSION = player_flag::transmission;
constexpr player_flag PLR_FISHING      = player_flag::fishing;
constexpr player_flag PLR_GOOP         = player_flag::goop;
constexpr player_flag PLR_MULTIHIT     = player_flag::multihit;
constexpr player_flag PLR_AURALIGHT    = player_flag::auralight;
constexpr player_flag PLR_RDISPLAY     = player_flag::rdisplay;
constexpr player_flag PLR_STOLEN       = player_flag::stolen;
constexpr player_flag PLR_TAILHIDE     = player_flag::tailhide;
constexpr player_flag PLR_NOGROW       = player_flag::nogrow;

constexpr int NUM_PLR_FLAGS = static_cast<int>(flag_count<player_flag>);

