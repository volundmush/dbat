#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table action_bits; /* NUM_MOB_FLAGS of them */

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

/* The old names, until every use says mob_flag:: */
constexpr mob_flag MOB_SPEC         = mob_flag::spec;
constexpr mob_flag MOB_SENTINEL     = mob_flag::sentinel;
constexpr mob_flag MOB_NOSCAVENGER  = mob_flag::noscavenger;
constexpr mob_flag MOB_ISNPC        = mob_flag::isnpc;
constexpr mob_flag MOB_AWARE        = mob_flag::aware;
constexpr mob_flag MOB_AGGRESSIVE   = mob_flag::aggressive;
constexpr mob_flag MOB_STAY_ZONE    = mob_flag::stay_zone;
constexpr mob_flag MOB_WIMPY        = mob_flag::wimpy;
constexpr mob_flag MOB_AGGR_EVIL    = mob_flag::aggr_evil;
constexpr mob_flag MOB_AGGR_GOOD    = mob_flag::aggr_good;
constexpr mob_flag MOB_AGGR_NEUTRAL = mob_flag::aggr_neutral;
constexpr mob_flag MOB_MEMORY       = mob_flag::memory;
constexpr mob_flag MOB_HELPER       = mob_flag::helper;
constexpr mob_flag MOB_NOCHARM      = mob_flag::nocharm;
constexpr mob_flag MOB_NOSUMMON     = mob_flag::nosummon;
constexpr mob_flag MOB_NOSLEEP      = mob_flag::nosleep;
constexpr mob_flag MOB_AUTOBALANCE  = mob_flag::autobalance;
constexpr mob_flag MOB_NOBLIND      = mob_flag::noblind;
constexpr mob_flag MOB_NOKILL       = mob_flag::nokill;
constexpr mob_flag MOB_NOTDEADYET   = mob_flag::notdeadyet;
constexpr mob_flag MOB_MOUNTABLE    = mob_flag::mountable;
constexpr mob_flag MOB_RARM         = mob_flag::rarm;
constexpr mob_flag MOB_LARM         = mob_flag::larm;
constexpr mob_flag MOB_RLEG         = mob_flag::rleg;
constexpr mob_flag MOB_LLEG         = mob_flag::lleg;
constexpr mob_flag MOB_HEAD         = mob_flag::head;
constexpr mob_flag MOB_JUSTDESC     = mob_flag::justdesc;
constexpr mob_flag MOB_HUSK         = mob_flag::husk;
constexpr mob_flag MOB_SPAR         = mob_flag::spar;
constexpr mob_flag MOB_DUMMY        = mob_flag::dummy;
constexpr mob_flag MOB_ABSORB       = mob_flag::absorb;
constexpr mob_flag MOB_REPAIR       = mob_flag::repair;
constexpr mob_flag MOB_NOPOISON     = mob_flag::nopoison;
constexpr mob_flag MOB_KNOWKAIO     = mob_flag::knowkaio;
constexpr mob_flag MOB_POWERUP      = mob_flag::powerup;

constexpr int NUM_MOB_FLAGS = static_cast<int>(flag_count<mob_flag>);

