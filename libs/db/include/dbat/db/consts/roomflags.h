#pragma once


/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK		0   /* Dark			*/
#define ROOM_DEATH		1   /* Death trap		*/
#define ROOM_NOMOB		2   /* MOBs not allowed		*/
#define ROOM_INDOORS	3   /* Indoors			*/
#define ROOM_PEACEFUL	4   /* Violence not allowed	*/
#define ROOM_SOUNDPROOF	5   /* Shouts, gossip blocked	*/
#define ROOM_NOTRACK	6   /* Track won't go through	*/
#define ROOM_NOINSTANT	7   /* IT not allowed		*/
#define ROOM_TUNNEL		8   /* room for only 1 pers	*/
#define ROOM_PRIVATE	9   /* Can't teleport in		*/
#define ROOM_GODROOM	10  /* LVL_GOD+ only allowed	*/
#define ROOM_HOUSE		11  /* (R) Room is a house	*/
#define ROOM_HOUSE_CRASH 12  /* (R) House needs saving	*/
#define ROOM_ATRIUM		13  /* (R) The door to a house	*/
#define ROOM_OLC		14  /* (R) Modifyable/!compress	*/
#define ROOM_BFS_MARK	15  /* (R) breath-first srch mrk	*/
#define ROOM_VEHICLE    16  /* Requires a vehicle to pass       */
#define ROOM_UNDERGROUND        17  /* Room is below ground      */
#define ROOM_CURRENT     	18  /* Room move with random currents	*/
#define ROOM_TIMED_DT     	19  /* Room has a timed death trap  	*/
#define ROOM_EARTH              20  /* Room is on Earth */
#define ROOM_VEGETA             21  /* Room is on Vegeta */
#define ROOM_FRIGID             22  /* Room is on Frigid */
#define ROOM_KONACK             23  /* Room is on Konack */
#define ROOM_NAMEK              24  /* Room is on Namek */
#define ROOM_NEO                25  /* Room is on Neo */
#define ROOM_AL                 26  /* Room is on AL */
#define ROOM_SPACE              27  /* Room is on Space */
#define ROOM_HELL               28  /* Room is Punishment Hell*/
#define ROOM_REGEN              29  /* Better regen */
#define ROOM_RHELL              30  /* Room is HELLLLLLL */
#define ROOM_GRAVITYX10         31  /* For rooms that have 10x grav */
#define ROOM_AETHER		32  /* Room is on Aether */
#define ROOM_HBTC               33  /* Room is extra special training area */
#define ROOM_PAST               34  /* Inside the pendulum room */
#define ROOM_CBANK              35  /* This room is a clan bank */
#define ROOM_SHIP               36  /* This room is a private ship room */
#define ROOM_YARDRAT            37  /* This room is on planet Yardrat   */
#define ROOM_KANASSA            38  /* This room is on planet Kanassa   */
#define ROOM_ARLIA              39  /* This room is on planet Arlia     */
#define ROOM_AURA               40  /* This room has an aura around it  */
#define ROOM_EORBIT             41  /* Earth Orbit                      */
#define ROOM_FORBIT             42  /* Frigid Orbit                     */
#define ROOM_KORBIT             43  /* Konack Orbit                     */
#define ROOM_NORBIT             44  /* Namek  Orbit                     */
#define ROOM_VORBIT             45  /* Vegeta Orbit                     */
#define ROOM_AORBIT             46  /* Aether Orbit                     */
#define ROOM_YORBIT             47  /* Yardrat Orbit                    */
#define ROOM_KANORB             48  /* Kanassa Orbit                    */
#define ROOM_ARLORB             49  /* Arlia Orbit                      */
#define ROOM_NEBULA             50  /* Nebulae                          */
#define ROOM_ASTERO             51  /* Asteroid                         */
#define ROOM_WORMHO             52  /* Wormhole                         */
#define ROOM_STATION            53  /* Space Station                    */
#define ROOM_STAR               54  /* Is a star                        */
#define ROOM_CERRIA             55  /* This room is on planet Cerria    */
#define ROOM_CORBIT             56  /* This room is in Cerria's Orbit   */
#define ROOM_BEDROOM            57  /* +25% regen                       */
#define ROOM_WORKOUT            58  /* Workout Room                     */
#define ROOM_GARDEN1            59  /* 8 plant garden                   */
#define ROOM_GARDEN2            60  /* 20 plant garden                  */
#define ROOM_FERTILE1           61
#define ROOM_FERTILE2           62
#define ROOM_FISHING            63
#define ROOM_FISHFRESH          64
#define ROOM_CANREMODEL         65

#define NUM_ROOM_FLAGS          66

#define RF_ARRAY_MAX    4

extern const char *room_bits[NUM_ROOM_FLAGS+1];