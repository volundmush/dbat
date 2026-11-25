#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table affected_bits; /* flag_count<aff_flag> of them */

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



