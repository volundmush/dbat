#pragma once
#include <string>
#include "dbat/registry.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/consts/roomflags.hpp"
#include "dbat/consts/directions.hpp"
#include "dbat/consts/sectortypes.hpp"
#include "dbat/consts/exitflags.hpp"
#include "dbat/extradesc.hpp"

struct room_direction_data {
  std::string general_description; /* When look DIR; empty for none */
  std::string keyword;             /* for open/close; empty for none */

  int16_t exit_info{};       /* Exit info			*/
  obj_vnum key{};            /* Key's number (-1 for no key)		*/
  room_vnum to_room{};       /* Where direction leads (NOWHERE)	*/
  int dclock{};              /* DC to pick the lock			*/
  int dchide{};              /* DC to find hidden			*/
  int dcskill{};             /* Skill req. to move through exit	*/
  int dcmove{};              /* DC for skill to move through exit	*/
  int failsavetype{};        /* Saving Throw type on skill fail	*/
  int dcfailsave{};          /* DC to save against on fail		*/
  room_vnum failroom{};      /* Room # to put char in when fail > 5  */
  room_vnum totalfailroom{}; /* Room # if char fails save < 5	*/
};

/* ================== Memory Structure for room ======================= */
struct room_data {
  room_vnum number{};                                    /* Rooms number	(vnum)		      */
  struct zone_data *zone{};                              /* Room zone (for resetting)          */
  int sector_type{};                                     /* sector type (move/hide)            */
  std::string name;                                      /* Rooms name 'You are ...'; empty for none */
  std::string description;                               /* Shown when entered; empty for none */
  ex_desc_list ex_description;                           /* for examine/look */
  owned<room_direction_data> dir_option[NUM_OF_DIRS];    /* Directions: the room owns its exits */
  bitvector_t room_flags[RF_ARRAY_MAX]{};                /* DEATH,DARK ... etc */

  struct trig_proto_list *proto_script{}; /* list of default triggers  */
  struct script_data *script{};           /* script info for the object         */

  int8_t light{}; /* Number of lightsources in room     */
  SPECIAL(*func){};

  id_list contents; /* the objects lying here, newest first */
  id_list people;   /* the characters here, newest first */

  int timed{};   /* For timed Dt's                     */
  int dmg{};     /* How damaged the room is            */
  int gravity{}; /* What is the level of gravity?      */
  int geffect{}; /* Effect of ground destruction       */
};
/* ====================================================================== */

