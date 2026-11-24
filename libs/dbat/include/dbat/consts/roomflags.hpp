#pragma once
#include <cstdint>
#include <string_view>

#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

extern const name_table room_bits; /* NUM_ROOM_FLAGS of them */

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

/* The old names, until every use says room_flag:: */
constexpr room_flag ROOM_DARK        = room_flag::dark;
constexpr room_flag ROOM_DEATH       = room_flag::death;
constexpr room_flag ROOM_NOMOB       = room_flag::nomob;
constexpr room_flag ROOM_INDOORS     = room_flag::indoors;
constexpr room_flag ROOM_PEACEFUL    = room_flag::peaceful;
constexpr room_flag ROOM_SOUNDPROOF  = room_flag::soundproof;
constexpr room_flag ROOM_NOTRACK     = room_flag::notrack;
constexpr room_flag ROOM_NOINSTANT   = room_flag::noinstant;
constexpr room_flag ROOM_TUNNEL      = room_flag::tunnel;
constexpr room_flag ROOM_PRIVATE     = room_flag::private_;
constexpr room_flag ROOM_GODROOM     = room_flag::godroom;
constexpr room_flag ROOM_HOUSE       = room_flag::house;
constexpr room_flag ROOM_HOUSE_CRASH = room_flag::house_crash;
constexpr room_flag ROOM_ATRIUM      = room_flag::atrium;
constexpr room_flag ROOM_OLC         = room_flag::olc;
constexpr room_flag ROOM_BFS_MARK    = room_flag::bfs_mark;
constexpr room_flag ROOM_VEHICLE     = room_flag::vehicle;
constexpr room_flag ROOM_UNDERGROUND = room_flag::underground;
constexpr room_flag ROOM_CURRENT     = room_flag::current;
constexpr room_flag ROOM_TIMED_DT    = room_flag::timed_dt;
constexpr room_flag ROOM_EARTH       = room_flag::earth;
constexpr room_flag ROOM_VEGETA      = room_flag::vegeta;
constexpr room_flag ROOM_FRIGID      = room_flag::frigid;
constexpr room_flag ROOM_KONACK      = room_flag::konack;
constexpr room_flag ROOM_NAMEK       = room_flag::namek;
constexpr room_flag ROOM_NEO         = room_flag::neo;
constexpr room_flag ROOM_AL          = room_flag::al;
constexpr room_flag ROOM_SPACE       = room_flag::space;
constexpr room_flag ROOM_HELL        = room_flag::hell;
constexpr room_flag ROOM_REGEN       = room_flag::regen;
constexpr room_flag ROOM_RHELL       = room_flag::rhell;
constexpr room_flag ROOM_GRAVITYX10  = room_flag::gravityx10;
constexpr room_flag ROOM_AETHER      = room_flag::aether;
constexpr room_flag ROOM_HBTC        = room_flag::hbtc;
constexpr room_flag ROOM_PAST        = room_flag::past;
constexpr room_flag ROOM_CBANK       = room_flag::cbank;
constexpr room_flag ROOM_SHIP        = room_flag::ship;
constexpr room_flag ROOM_YARDRAT     = room_flag::yardrat;
constexpr room_flag ROOM_KANASSA     = room_flag::kanassa;
constexpr room_flag ROOM_ARLIA       = room_flag::arlia;
constexpr room_flag ROOM_AURA        = room_flag::aura;
constexpr room_flag ROOM_EORBIT      = room_flag::eorbit;
constexpr room_flag ROOM_FORBIT      = room_flag::forbit;
constexpr room_flag ROOM_KORBIT      = room_flag::korbit;
constexpr room_flag ROOM_NORBIT      = room_flag::norbit;
constexpr room_flag ROOM_VORBIT      = room_flag::vorbit;
constexpr room_flag ROOM_AORBIT      = room_flag::aorbit;
constexpr room_flag ROOM_YORBIT      = room_flag::yorbit;
constexpr room_flag ROOM_KANORB      = room_flag::kanorb;
constexpr room_flag ROOM_ARLORB      = room_flag::arlorb;
constexpr room_flag ROOM_NEBULA      = room_flag::nebula;
constexpr room_flag ROOM_ASTERO      = room_flag::astero;
constexpr room_flag ROOM_WORMHO      = room_flag::wormho;
constexpr room_flag ROOM_STATION     = room_flag::station;
constexpr room_flag ROOM_STAR        = room_flag::star;
constexpr room_flag ROOM_CERRIA      = room_flag::cerria;
constexpr room_flag ROOM_CORBIT      = room_flag::corbit;
constexpr room_flag ROOM_BEDROOM     = room_flag::bedroom;
constexpr room_flag ROOM_WORKOUT     = room_flag::workout;
constexpr room_flag ROOM_GARDEN1     = room_flag::garden1;
constexpr room_flag ROOM_GARDEN2     = room_flag::garden2;
constexpr room_flag ROOM_FERTILE1    = room_flag::fertile1;
constexpr room_flag ROOM_FERTILE2    = room_flag::fertile2;
constexpr room_flag ROOM_FISHING     = room_flag::fishing;
constexpr room_flag ROOM_FISHFRESH   = room_flag::fishfresh;
constexpr room_flag ROOM_CANREMODEL  = room_flag::canremodel;

constexpr int NUM_ROOM_FLAGS = static_cast<int>(flag_count<room_flag>);

constexpr int RF_ARRAY_MAX = 4;

