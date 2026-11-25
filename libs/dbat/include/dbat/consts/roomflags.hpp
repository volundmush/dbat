#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table room_bits; /* flag_count<room_flag> of them */

/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
enum class room_flag : uint8_t {
  dark        = 0,   /* Dark			*/
  death       = 1,   /* Death trap		*/
  nomob       = 2,   /* MOBs not allowed		*/
  indoors     = 3,   /* Indoors			*/
  peaceful    = 4,   /* Violence not allowed	*/
  soundproof  = 5,   /* Shouts, gossip blocked	*/
  notrack     = 6,   /* Track won't go through	*/
  noinstant   = 7,   /* IT not allowed		*/
  tunnel      = 8,   /* room for only 1 pers	*/
  private_    = 9,   /* Can't teleport in		*/
  godroom     = 10,  /* LVL_GOD+ only allowed	*/
  house       = 11,  /* (R) Room is a house	*/
  house_crash = 12,  /* (R) House needs saving	*/
  atrium      = 13,  /* (R) The door to a house	*/
  olc         = 14,  /* (R) Modifyable/!compress	*/
  bfs_mark    = 15,  /* (R) breath-first srch mrk	*/
  vehicle     = 16,  /* Requires a vehicle to pass       */
  underground = 17,  /* Room is below ground      */
  current     = 18,  /* Room move with random currents	*/
  timed_dt    = 19,  /* Room has a timed death trap  	*/
  earth       = 20,  /* Room is on Earth */
  vegeta      = 21,  /* Room is on Vegeta */
  frigid      = 22,  /* Room is on Frigid */
  konack      = 23,  /* Room is on Konack */
  namek       = 24,  /* Room is on Namek */
  neo         = 25,  /* Room is on Neo */
  al          = 26,  /* Room is on AL */
  space       = 27,  /* Room is on Space */
  hell        = 28,  /* Room is Punishment Hell*/
  regen       = 29,  /* Better regen */
  rhell       = 30,  /* Room is HELLLLLLL */
  gravityx10  = 31,  /* For rooms that have 10x grav */
  aether      = 32,  /* Room is on Aether */
  hbtc        = 33,  /* Room is extra special training area */
  past        = 34,  /* Inside the pendulum room */
  cbank       = 35,  /* This room is a clan bank */
  ship        = 36,  /* This room is a private ship room */
  yardrat     = 37,  /* This room is on planet Yardrat   */
  kanassa     = 38,  /* This room is on planet Kanassa   */
  arlia       = 39,  /* This room is on planet Arlia     */
  aura        = 40,  /* This room has an aura around it  */
  eorbit      = 41,  /* Earth Orbit                      */
  forbit      = 42,  /* Frigid Orbit                     */
  korbit      = 43,  /* Konack Orbit                     */
  norbit      = 44,  /* Namek  Orbit                     */
  vorbit      = 45,  /* Vegeta Orbit                     */
  aorbit      = 46,  /* Aether Orbit                     */
  yorbit      = 47,  /* Yardrat Orbit                    */
  kanorb      = 48,  /* Kanassa Orbit                    */
  arlorb      = 49,  /* Arlia Orbit                      */
  nebula      = 50,  /* Nebulae                          */
  astero      = 51,  /* Asteroid                         */
  wormho      = 52,  /* Wormhole                         */
  station     = 53,  /* Space Station                    */
  star        = 54,  /* Is a star                        */
  cerria      = 55,  /* This room is on planet Cerria    */
  corbit      = 56,  /* This room is in Cerria's Orbit   */
  bedroom     = 57,  /* +25% regen                       */
  workout     = 58,  /* Workout Room                     */
  garden1     = 59,  /* 8 plant garden                   */
  garden2     = 60,  /* 20 plant garden                  */
  fertile1    = 61,
  fertile2    = 62,
  fishing     = 63,
  fishfresh   = 64,
  canremodel  = 65,
  count_      = 66,
};
template <> struct flag_traits<room_flag> {
  static constexpr const name_table &names = room_bits;
};



