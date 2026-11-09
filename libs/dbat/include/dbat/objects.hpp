#pragma once
#include <array>
#include <string>
#include "dbat/registry.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/consts/itemdata.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/affflags.hpp"
#include "dbat/consts/sizes.hpp"
#include "dbat/consts/weapons.hpp"
#include "dbat/consts/applies.hpp"
#include "dbat/affected.hpp"
#include "dbat/index.hpp"
#include "dbat/extradesc.hpp"

struct obj_spellbook_spell {
  int spellname{}; /* Which spell is written */
  int pages{};     /* How many pages does it take up */
};

/* ================== Memory Structure for Objects ================== */
struct obj_data {
  obj_vnum item_number{};  /* Where in data-base			*/
  room_vnum in_room{};     /* In what room -1 when conta/carr	*/
  room_vnum room_loaded{}; /* Room loaded in, for room_max checks	*/

  int value[NUM_OBJ_VAL_POSITIONS]{};      /* Values of the item (see list)    */
  int8_t type_flag{};                      /* Type of item                        */
  int level{};                             /* Minimum level of object.            */
  bitvector_t wear_flags[TW_ARRAY_MAX]{};  /* Where you can wear it     */
  bitvector_t extra_flags[EF_ARRAY_MAX]{}; /* If it hums, glows, etc.  */
  int64_t weight{};                        /* Weigt what else                     */
  int cost{};                              /* Value when sold (gp.)               */
  int timer{};                             /* Timer for object                    */
  bitvector_t bitvector[AF_ARRAY_MAX]{};   /* To set chars bits          */
  int size{};                              /* Size class of object                */

  struct obj_affected_type affected[MAX_OBJ_AFFECT]{}; /* affects */

  std::string name;                          /* Title of object :get etc.; empty for none */
  std::string description;                   /* When in room; empty for none */
  std::string short_description;             /* when worn/carry/in cont.; empty for none */
  std::string action_description;            /* What to write when used; empty for none */
  ex_desc_list ex_description;               /* extra descriptions     */
  struct char_data *carried_by{};            /* Carried by :NULL in room/conta   */
  struct char_data *worn_by{};               /* Worn by?			      */
  int16_t worn_on{};                         /* Worn where?		      */

  struct obj_data *in_obj{};   /* In what object NULL when none    */
  id_list contains;            /* the objects inside it, newest first */

  int32_t id{};        /* used by DG triggers              */
  time_t generation{}; /* creation time for dupe check     */
  int64_t unique_id{}; /* random bits for dupe check       */

  struct trig_proto_list *proto_script{}; /* list of default triggers  */
  struct script_data *script{};           /* script info for the object       */


  int32_t sitting_uid{}; /* Who is sitting on me? */

  // scouter frequency
  int scoutfreq{};

  time_t lload{};

  // healing tank charge
  int healcharge{};

  // ki attacks
  int64_t kicharge{};
  int kitype{};
  int32_t user_uid{};
  int32_t target_uid{};
  int distance{};

  // something about food
  int foob{};

  // auction data
  int32_t aucter{};
  int32_t curBidder{};
  time_t aucTime{};
  int bid{};
  int startbid{};
  char *auctname{};

  // notes and boards
  int posttype{};
  int32_t posted_to_uid{};

  // icewall stuff
  int32_t fellow_wall_uid{};

  // UNUSED FIELDS below here
  int cost_per_day{};                   /* Cost to keep pr. real day           */
  owned<std::array<obj_spellbook_spell, SPELLBOOK_SIZE>> sbinfo{}; /* For spellbook info: its own, or none */
};

