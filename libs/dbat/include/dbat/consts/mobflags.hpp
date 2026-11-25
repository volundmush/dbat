#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table action_bits; /* flag_count<mob_flag> of them */

/* Mobile flags: char_data::mob_flags */
enum class mob_flag : uint8_t {
  spec         = 0,   /* Mob has a callable spec-proc   	*/
  sentinel     = 1,   /* Mob should not move            	*/
  noscavenger  = 2,   /* Mob won't pick up items from rooms*/
  isnpc        = 3,   /* (R) Automatically set on all Mobs */
  aware        = 4,   /* Mob can't be backstabbed          */
  aggressive   = 5,   /* Mob auto-attacks everybody nearby	*/
  stay_zone    = 6,   /* Mob shouldn't wander out of zone  */
  wimpy        = 7,   /* Mob flees if severely injured  	*/
  aggr_evil    = 8,   /* Auto-attack any evil PC's		*/
  aggr_good    = 9,   /* Auto-attack any good PC's      	*/
  aggr_neutral = 10,  /* Auto-attack any neutral PC's   	*/
  memory       = 11,  /* remember attackers if attacked    */
  helper       = 12,  /* attack PCs fighting other NPCs    */
  nocharm      = 13,  /* Mob can't be charmed         	*/
  nosummon     = 14,  /* Mob can't be summoned             */
  nosleep      = 15,  /* Mob can't be slept           	*/
  autobalance  = 16,  /* Mob stats autobalance		*/
  noblind      = 17,  /* Mob can't be blinded         	*/
  nokill       = 18,  /* Mob can't be killed               */
  notdeadyet   = 19,  /* (R) Mob being extracted.          */
  mountable    = 20,  /* Mob is mountable.			*/
  rarm         = 21,  /* Player has a right arm            */
  larm         = 22,  /* Player has a left arm             */
  rleg         = 23,  /* Player has a right leg            */
  lleg         = 24,  /* Player has a left leg             */
  head         = 25,  /* Player has a head                 */
  justdesc     = 26,  /* Mob doesn't use auto desc         */
  husk         = 27,  /* Is an extracted Husk              */
  spar         = 28,  /* This is mob sparring              */
  dummy        = 29,  /* This mob will not fight back      */
  absorb       = 30,  /* Absorb type android               */
  repair       = 31,  /* Repair type android               */
  nopoison     = 32,  /* No poison                         */
  knowkaio     = 33,  /* Knows kaioken                     */
  powerup      = 34,  /* Is powering up                    */
  count_       = 35,
};
template <> struct flag_traits<mob_flag> {
  static constexpr const name_table &names = action_bits;
};


