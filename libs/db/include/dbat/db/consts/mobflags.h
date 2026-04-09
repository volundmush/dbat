#pragma once



/* Mobile flags: used by char_data.act */
#define MOB_SPEC		0  /* Mob has a callable spec-proc   	*/
#define MOB_SENTINEL		1  /* Mob should not move            	*/
#define MOB_NOSCAVENGER		2  /* Mob won't pick up items from rooms*/
#define MOB_ISNPC		3  /* (R) Automatically set on all Mobs */
#define MOB_AWARE		4  /* Mob can't be backstabbed          */
#define MOB_AGGRESSIVE		5  /* Mob auto-attacks everybody nearby	*/
#define MOB_STAY_ZONE		6  /* Mob shouldn't wander out of zone  */
#define MOB_WIMPY		7  /* Mob flees if severely injured  	*/
#define MOB_AGGR_EVIL		8  /* Auto-attack any evil PC's		*/
#define MOB_AGGR_GOOD		9  /* Auto-attack any good PC's      	*/
#define MOB_AGGR_NEUTRAL	10 /* Auto-attack any neutral PC's   	*/
#define MOB_MEMORY		11 /* remember attackers if attacked    */
#define MOB_HELPER		12 /* attack PCs fighting other NPCs    */
#define MOB_NOCHARM		13 /* Mob can't be charmed         	*/
#define MOB_NOSUMMON		14 /* Mob can't be summoned             */
#define MOB_NOSLEEP		15 /* Mob can't be slept           	*/
#define MOB_AUTOBALANCE		16 /* Mob stats autobalance		*/
#define MOB_NOBLIND		17 /* Mob can't be blinded         	*/
#define MOB_NOKILL		18 /* Mob can't be killed               */
#define MOB_NOTDEADYET		19 /* (R) Mob being extracted.          */
#define MOB_MOUNTABLE		20 /* Mob is mountable.			*/
#define MOB_RARM                21 /* Player has a right arm            */
#define MOB_LARM                22 /* Player has a left arm             */
#define MOB_RLEG                23 /* Player has a right leg            */
#define MOB_LLEG                24 /* Player has a left leg             */
#define MOB_HEAD                25 /* Player has a head                 */
#define MOB_JUSTDESC            26 /* Mob doesn't use auto desc         */
#define MOB_HUSK                27 /* Is an extracted Husk              */
#define MOB_SPAR                28 /* This is mob sparring              */
#define MOB_DUMMY               29 /* This mob will not fight back      */
#define MOB_ABSORB              30 /* Absorb type android               */
#define MOB_REPAIR              31 /* Repair type android               */
#define MOB_NOPOISON            32 /* No poison                         */
#define MOB_KNOWKAIO            33 /* Knows kaioken                     */
#define MOB_POWERUP             34 /* Is powering up                    */

#define NUM_MOB_FLAGS 35

extern const char *action_bits[NUM_MOB_FLAGS+1];