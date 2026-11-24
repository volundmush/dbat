#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table affected_bits; /* NUM_AFF_FLAGS of them */

/* Affect bits: used in char_data.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
enum class aff_flag : uint8_t {
  dontuse        = 0,   /* DON'T USE! 		*/
  blind          = 1,   /* (R) Char is blind         */
  invisible      = 2,   /* Char is invisible         */
  detect_align   = 3,   /* Char is sensitive to align*/
  detect_invis   = 4,   /* Char can see invis chars  */
  detect_magic   = 5,   /* Char is sensitive to magic*/
  sense_life     = 6,   /* Char can sense hidden life*/
  waterwalk      = 7,   /* Char can walk on water    */
  sanctuary      = 8,   /* Char protected by sanct.  */
  group          = 9,   /* (R) Char is grouped       */
  curse          = 10,  /* Char is cursed            */
  infravision    = 11,  /* Char can see in dark      */
  poison         = 12,  /* (R) Char is poisoned      */
  weakened_state = 13,  /* Char protected from evil  */
  protect_good   = 14,  /* Char protected from good  */
  sleep          = 15,  /* (R) Char magically asleep */
  notrack        = 16,  /* Char can't be tracked     */
  undead         = 17,  /* Char is undead 		*/
  paralyze       = 18,  /* Char is paralized		*/
  sneak          = 19,  /* Char can move quietly     */
  hide           = 20,  /* Char is hidden            */
  unused20       = 21,  /* Room for future expansion */
  charm          = 22,  /* Char is charmed         	*/
  flying         = 23,  /* Char is flying         	*/
  waterbreath    = 24,  /* Char can breath non O2    */
  angelic        = 25,  /* Char is an angelic being  */
  ethereal       = 26,  /* Char is ethereal          */
  magiconly      = 27,  /* Char only hurt by magic   */
  nextpartial    = 28,  /* Next action cannot be full*/
  nextnoaction   = 29,  /* Next action cannot attack (took full action between rounds) */
  stunned        = 30,  /* Char is stunned		*/
  tamed          = 31,  /* Char has been tamed	*/
  cdeath         = 32,  /* Char is undergoing creeping death */
  spirit         = 33,  /* Char has no body          */
  stoneskin      = 34,  /* Char has temporary DR     */
  summoned       = 35,  /* Char is summoned (i.e. transient */
  celestial      = 36,  /* Char is celestial         */
  fiendish       = 37,  /* Char is fiendish          */
  fire_shield    = 38,  /* Char has fire shield      */
  low_light      = 39,  /* Char has low light eyes   */
  zanzoken       = 40,  /* Char is ready to zanzoken */
  knocked        = 41,  /* Char is knocked OUT!      */
  might          = 42,  /* Strength +3               */
  flex           = 43,  /* Agility +3                */
  genius         = 44,  /* Intelligence +3           */
  bless          = 45,  /* Bless for better regen    */
  burnt          = 46,  /* Disintergrated corpse     */
  burned         = 47,  /* Burned by honoo or similar skill */
  mbreak         = 48,  /* Can't charge while flagged */
  hass           = 49,  /* Does double punch damage  */
  future         = 50,  /* Future Sight */
  para           = 51,  /* Real Paralyze */
  infuse         = 52,  /* Ki infused attacks */
  enlighten      = 53,  /* Enlighten */
  frozen         = 54,  /* They got frozededed */
  fireshield     = 55,  /* They have a blazing personality */
  ensnared       = 56,  /* They have silk ensnaring their arms! */
  hayasa         = 57,  /* They are speedy!                */
  pursuit        = 58,  /* Being followed */
  wither         = 59,  /* Their body is withered */
  hydrozap       = 60,  /* Custom Skill Kanso Suru */
  position       = 61,  /* Better combat position */
  shocked        = 62,  /* Psychic Shock          */
  metamorph      = 63,  /* Metamorphisis, Demon's Ripoff Custom Skill */
  healglow       = 64,  /* Healing Glow */
  earmor         = 65,  /* Ethereal Armor */
  echains        = 66,  /* Ethereal Chains */
  wunjo          = 67,  /* Wunjo rune */
  potent         = 68,  /* Purisaz rune */
  ashed          = 69,  /* Leaves ash */
  puked          = 70,
  liquefied      = 71,
  shell          = 72,
  immunity       = 73,
  spiritcontrol  = 74,
  kyodaika       = 75,  /* Kyodaika */
  starphase      = 76,
  special_pose   = 77,
  shadow_stitch  = 78,
  count_         = 79,
};
template <> struct flag_traits<aff_flag> {
  static constexpr const name_table &names = affected_bits;
};

/* The old names, until every use says aff_flag:: */
constexpr aff_flag AFF_DONTUSE        = aff_flag::dontuse;
constexpr aff_flag AFF_BLIND          = aff_flag::blind;
constexpr aff_flag AFF_INVISIBLE      = aff_flag::invisible;
constexpr aff_flag AFF_DETECT_ALIGN   = aff_flag::detect_align;
constexpr aff_flag AFF_DETECT_INVIS   = aff_flag::detect_invis;
constexpr aff_flag AFF_DETECT_MAGIC   = aff_flag::detect_magic;
constexpr aff_flag AFF_SENSE_LIFE     = aff_flag::sense_life;
constexpr aff_flag AFF_WATERWALK      = aff_flag::waterwalk;
constexpr aff_flag AFF_SANCTUARY      = aff_flag::sanctuary;
constexpr aff_flag AFF_GROUP          = aff_flag::group;
constexpr aff_flag AFF_CURSE          = aff_flag::curse;
constexpr aff_flag AFF_INFRAVISION    = aff_flag::infravision;
constexpr aff_flag AFF_POISON         = aff_flag::poison;
constexpr aff_flag AFF_WEAKENED_STATE = aff_flag::weakened_state;
constexpr aff_flag AFF_PROTECT_GOOD   = aff_flag::protect_good;
constexpr aff_flag AFF_SLEEP          = aff_flag::sleep;
constexpr aff_flag AFF_NOTRACK        = aff_flag::notrack;
constexpr aff_flag AFF_UNDEAD         = aff_flag::undead;
constexpr aff_flag AFF_PARALYZE       = aff_flag::paralyze;
constexpr aff_flag AFF_SNEAK          = aff_flag::sneak;
constexpr aff_flag AFF_HIDE           = aff_flag::hide;
constexpr aff_flag AFF_UNUSED20       = aff_flag::unused20;
constexpr aff_flag AFF_CHARM          = aff_flag::charm;
constexpr aff_flag AFF_FLYING         = aff_flag::flying;
constexpr aff_flag AFF_WATERBREATH    = aff_flag::waterbreath;
constexpr aff_flag AFF_ANGELIC        = aff_flag::angelic;
constexpr aff_flag AFF_ETHEREAL       = aff_flag::ethereal;
constexpr aff_flag AFF_MAGICONLY      = aff_flag::magiconly;
constexpr aff_flag AFF_NEXTPARTIAL    = aff_flag::nextpartial;
constexpr aff_flag AFF_NEXTNOACTION   = aff_flag::nextnoaction;
constexpr aff_flag AFF_STUNNED        = aff_flag::stunned;
constexpr aff_flag AFF_TAMED          = aff_flag::tamed;
constexpr aff_flag AFF_CDEATH         = aff_flag::cdeath;
constexpr aff_flag AFF_SPIRIT         = aff_flag::spirit;
constexpr aff_flag AFF_STONESKIN      = aff_flag::stoneskin;
constexpr aff_flag AFF_SUMMONED       = aff_flag::summoned;
constexpr aff_flag AFF_CELESTIAL      = aff_flag::celestial;
constexpr aff_flag AFF_FIENDISH       = aff_flag::fiendish;
constexpr aff_flag AFF_FIRE_SHIELD    = aff_flag::fire_shield;
constexpr aff_flag AFF_LOW_LIGHT      = aff_flag::low_light;
constexpr aff_flag AFF_ZANZOKEN       = aff_flag::zanzoken;
constexpr aff_flag AFF_KNOCKED        = aff_flag::knocked;
constexpr aff_flag AFF_MIGHT          = aff_flag::might;
constexpr aff_flag AFF_FLEX           = aff_flag::flex;
constexpr aff_flag AFF_GENIUS         = aff_flag::genius;
constexpr aff_flag AFF_BLESS          = aff_flag::bless;
constexpr aff_flag AFF_BURNT          = aff_flag::burnt;
constexpr aff_flag AFF_BURNED         = aff_flag::burned;
constexpr aff_flag AFF_MBREAK         = aff_flag::mbreak;
constexpr aff_flag AFF_HASS           = aff_flag::hass;
constexpr aff_flag AFF_FUTURE         = aff_flag::future;
constexpr aff_flag AFF_PARA           = aff_flag::para;
constexpr aff_flag AFF_INFUSE         = aff_flag::infuse;
constexpr aff_flag AFF_ENLIGHTEN      = aff_flag::enlighten;
constexpr aff_flag AFF_FROZEN         = aff_flag::frozen;
constexpr aff_flag AFF_FIRESHIELD     = aff_flag::fireshield;
constexpr aff_flag AFF_ENSNARED       = aff_flag::ensnared;
constexpr aff_flag AFF_HAYASA         = aff_flag::hayasa;
constexpr aff_flag AFF_PURSUIT        = aff_flag::pursuit;
constexpr aff_flag AFF_WITHER         = aff_flag::wither;
constexpr aff_flag AFF_HYDROZAP       = aff_flag::hydrozap;
constexpr aff_flag AFF_POSITION       = aff_flag::position;
constexpr aff_flag AFF_SHOCKED        = aff_flag::shocked;
constexpr aff_flag AFF_METAMORPH      = aff_flag::metamorph;
constexpr aff_flag AFF_HEALGLOW       = aff_flag::healglow;
constexpr aff_flag AFF_EARMOR         = aff_flag::earmor;
constexpr aff_flag AFF_ECHAINS        = aff_flag::echains;
constexpr aff_flag AFF_WUNJO          = aff_flag::wunjo;
constexpr aff_flag AFF_POTENT         = aff_flag::potent;
constexpr aff_flag AFF_ASHED          = aff_flag::ashed;
constexpr aff_flag AFF_PUKED          = aff_flag::puked;
constexpr aff_flag AFF_LIQUEFIED      = aff_flag::liquefied;
constexpr aff_flag AFF_SHELL          = aff_flag::shell;
constexpr aff_flag AFF_IMMUNITY       = aff_flag::immunity;
constexpr aff_flag AFF_SPIRITCONTROL  = aff_flag::spiritcontrol;
constexpr aff_flag AFF_KYODAIKA       = aff_flag::kyodaika;
constexpr aff_flag AFF_STARPHASE      = aff_flag::starphase;
constexpr aff_flag AFF_SPECIAL_POSE   = aff_flag::special_pose;
constexpr aff_flag AFF_SHADOW_STITCH  = aff_flag::shadow_stitch;

constexpr int NUM_AFF_FLAGS = static_cast<int>(flag_count<aff_flag>);

constexpr int AF_ARRAY_MAX = 4;

