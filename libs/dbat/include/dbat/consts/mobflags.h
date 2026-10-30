#pragma once



/* Mobile flags: used by char_data.act */
constexpr int MOB_SPEC		= 0; /* Mob has a callable spec-proc   	*/
constexpr int MOB_SENTINEL		= 1; /* Mob should not move            	*/
constexpr int MOB_NOSCAVENGER		= 2; /* Mob won't pick up items from rooms*/
constexpr int MOB_ISNPC		= 3; /* (R) Automatically set on all Mobs */
constexpr int MOB_AWARE		= 4; /* Mob can't be backstabbed          */
constexpr int MOB_AGGRESSIVE		= 5; /* Mob auto-attacks everybody nearby	*/
constexpr int MOB_STAY_ZONE		= 6; /* Mob shouldn't wander out of zone  */
constexpr int MOB_WIMPY		= 7; /* Mob flees if severely injured  	*/
constexpr int MOB_AGGR_EVIL		= 8; /* Auto-attack any evil PC's		*/
constexpr int MOB_AGGR_GOOD		= 9; /* Auto-attack any good PC's      	*/
constexpr int MOB_AGGR_NEUTRAL	= 10; /* Auto-attack any neutral PC's   	*/
constexpr int MOB_MEMORY		= 11; /* remember attackers if attacked    */
constexpr int MOB_HELPER		= 12; /* attack PCs fighting other NPCs    */
constexpr int MOB_NOCHARM		= 13; /* Mob can't be charmed         	*/
constexpr int MOB_NOSUMMON		= 14; /* Mob can't be summoned             */
constexpr int MOB_NOSLEEP		= 15; /* Mob can't be slept           	*/
constexpr int MOB_AUTOBALANCE		= 16; /* Mob stats autobalance		*/
constexpr int MOB_NOBLIND		= 17; /* Mob can't be blinded         	*/
constexpr int MOB_NOKILL		= 18; /* Mob can't be killed               */
constexpr int MOB_NOTDEADYET		= 19; /* (R) Mob being extracted.          */
constexpr int MOB_MOUNTABLE		= 20; /* Mob is mountable.			*/
constexpr int MOB_RARM                = 21; /* Player has a right arm            */
constexpr int MOB_LARM                = 22; /* Player has a left arm             */
constexpr int MOB_RLEG                = 23; /* Player has a right leg            */
constexpr int MOB_LLEG                = 24; /* Player has a left leg             */
constexpr int MOB_HEAD                = 25; /* Player has a head                 */
constexpr int MOB_JUSTDESC            = 26; /* Mob doesn't use auto desc         */
constexpr int MOB_HUSK                = 27; /* Is an extracted Husk              */
constexpr int MOB_SPAR                = 28; /* This is mob sparring              */
constexpr int MOB_DUMMY               = 29; /* This mob will not fight back      */
constexpr int MOB_ABSORB              = 30; /* Absorb type android               */
constexpr int MOB_REPAIR              = 31; /* Repair type android               */
constexpr int MOB_NOPOISON            = 32; /* No poison                         */
constexpr int MOB_KNOWKAIO            = 33; /* Knows kaioken                     */
constexpr int MOB_POWERUP             = 34; /* Is powering up                    */

constexpr int NUM_MOB_FLAGS = 35;

extern const char *action_bits[NUM_MOB_FLAGS+1];