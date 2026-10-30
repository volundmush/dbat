#pragma once


/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
constexpr int ROOM_DARK		= 0; /* Dark			*/
constexpr int ROOM_DEATH		= 1; /* Death trap		*/
constexpr int ROOM_NOMOB		= 2; /* MOBs not allowed		*/
constexpr int ROOM_INDOORS	= 3; /* Indoors			*/
constexpr int ROOM_PEACEFUL	= 4; /* Violence not allowed	*/
constexpr int ROOM_SOUNDPROOF	= 5; /* Shouts, gossip blocked	*/
constexpr int ROOM_NOTRACK	= 6; /* Track won't go through	*/
constexpr int ROOM_NOINSTANT	= 7; /* IT not allowed		*/
constexpr int ROOM_TUNNEL		= 8; /* room for only 1 pers	*/
constexpr int ROOM_PRIVATE	= 9; /* Can't teleport in		*/
constexpr int ROOM_GODROOM	= 10; /* LVL_GOD+ only allowed	*/
constexpr int ROOM_HOUSE		= 11; /* (R) Room is a house	*/
constexpr int ROOM_HOUSE_CRASH = 12; /* (R) House needs saving	*/
constexpr int ROOM_ATRIUM		= 13; /* (R) The door to a house	*/
constexpr int ROOM_OLC		= 14; /* (R) Modifyable/!compress	*/
constexpr int ROOM_BFS_MARK	= 15; /* (R) breath-first srch mrk	*/
constexpr int ROOM_VEHICLE    = 16; /* Requires a vehicle to pass       */
constexpr int ROOM_UNDERGROUND        = 17; /* Room is below ground      */
constexpr int ROOM_CURRENT     	= 18; /* Room move with random currents	*/
constexpr int ROOM_TIMED_DT     	= 19; /* Room has a timed death trap  	*/
constexpr int ROOM_EARTH              = 20; /* Room is on Earth */
constexpr int ROOM_VEGETA             = 21; /* Room is on Vegeta */
constexpr int ROOM_FRIGID             = 22; /* Room is on Frigid */
constexpr int ROOM_KONACK             = 23; /* Room is on Konack */
constexpr int ROOM_NAMEK              = 24; /* Room is on Namek */
constexpr int ROOM_NEO                = 25; /* Room is on Neo */
constexpr int ROOM_AL                 = 26; /* Room is on AL */
constexpr int ROOM_SPACE              = 27; /* Room is on Space */
constexpr int ROOM_HELL               = 28; /* Room is Punishment Hell*/
constexpr int ROOM_REGEN              = 29; /* Better regen */
constexpr int ROOM_RHELL              = 30; /* Room is HELLLLLLL */
constexpr int ROOM_GRAVITYX10         = 31; /* For rooms that have 10x grav */
constexpr int ROOM_AETHER		= 32; /* Room is on Aether */
constexpr int ROOM_HBTC               = 33; /* Room is extra special training area */
constexpr int ROOM_PAST               = 34; /* Inside the pendulum room */
constexpr int ROOM_CBANK              = 35; /* This room is a clan bank */
constexpr int ROOM_SHIP               = 36; /* This room is a private ship room */
constexpr int ROOM_YARDRAT            = 37; /* This room is on planet Yardrat   */
constexpr int ROOM_KANASSA            = 38; /* This room is on planet Kanassa   */
constexpr int ROOM_ARLIA              = 39; /* This room is on planet Arlia     */
constexpr int ROOM_AURA               = 40; /* This room has an aura around it  */
constexpr int ROOM_EORBIT             = 41; /* Earth Orbit                      */
constexpr int ROOM_FORBIT             = 42; /* Frigid Orbit                     */
constexpr int ROOM_KORBIT             = 43; /* Konack Orbit                     */
constexpr int ROOM_NORBIT             = 44; /* Namek  Orbit                     */
constexpr int ROOM_VORBIT             = 45; /* Vegeta Orbit                     */
constexpr int ROOM_AORBIT             = 46; /* Aether Orbit                     */
constexpr int ROOM_YORBIT             = 47; /* Yardrat Orbit                    */
constexpr int ROOM_KANORB             = 48; /* Kanassa Orbit                    */
constexpr int ROOM_ARLORB             = 49; /* Arlia Orbit                      */
constexpr int ROOM_NEBULA             = 50; /* Nebulae                          */
constexpr int ROOM_ASTERO             = 51; /* Asteroid                         */
constexpr int ROOM_WORMHO             = 52; /* Wormhole                         */
constexpr int ROOM_STATION            = 53; /* Space Station                    */
constexpr int ROOM_STAR               = 54; /* Is a star                        */
constexpr int ROOM_CERRIA             = 55; /* This room is on planet Cerria    */
constexpr int ROOM_CORBIT             = 56; /* This room is in Cerria's Orbit   */
constexpr int ROOM_BEDROOM            = 57; /* +25% regen                       */
constexpr int ROOM_WORKOUT            = 58; /* Workout Room                     */
constexpr int ROOM_GARDEN1            = 59; /* 8 plant garden                   */
constexpr int ROOM_GARDEN2            = 60; /* 20 plant garden                  */
constexpr int ROOM_FERTILE1           = 61;
constexpr int ROOM_FERTILE2           = 62;
constexpr int ROOM_FISHING            = 63;
constexpr int ROOM_FISHFRESH          = 64;
constexpr int ROOM_CANREMODEL         = 65;

constexpr int NUM_ROOM_FLAGS          = 66;

constexpr int RF_ARRAY_MAX    = 4;

extern const char *room_bits[NUM_ROOM_FLAGS+1];