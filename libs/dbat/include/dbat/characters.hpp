#pragma once
#include <deque>
#include <list>
#include <map>
#include <string>
#include "dbat/command.hpp"
#include "dbat/affected.hpp"
#include "dbat/registry.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/consts/prefflags.hpp"
#include "dbat/consts/magic.hpp"
#include "dbat/consts/colors.hpp"
#include "dbat/consts/conditions.hpp"
#include "dbat/consts/history.hpp"
#include "dbat/consts/senseis.hpp"
#include "dbat/consts/itemdata.hpp"
#include "dbat/consts/skills.hpp"
#include "dbat/consts/bonus.hpp"
#include "dbat/consts/feats.hpp"
#include "dbat/consts/affflags.hpp"
#include "dbat/consts/adminflags.hpp"
#include "dbat/consts/mobflags.hpp"
#include "dbat/consts/playerflags.hpp"
#include "dbat/consts/positions.hpp"
#include "dbat/consts/fightprefs.hpp"
#include "dbat/consts/appearance.hpp"
#include "dbat/consts/admlevel.hpp"
#include "dbat/consts/races.hpp"
#include "dbat/consts/sizes.hpp"
#include "dbat/index.hpp"

constexpr int PM_ARRAY_MAX = 4;

/* These data contain information about a players time data */
struct time_data {
  time_t birth{};   /* This represents the characters current age        */
  time_t created{}; /* This does not change                              */
  time_t maxage{};  /* This represents death by natural causes           */
  time_t logon{};   /* Time of the last logon (used to calculate played) */
  time_t played{};  /* This is the total accumulated time played in secs */
};

/* The pclean_criteria_data is set up in config.c and used in db.c to
   determine the conditions which will cause a player character to be
   deleted from disk if the automagic pwipe system is enabled (see config.c).
*/
struct pclean_criteria_data {
  int level{}; /* max level for this time limit	*/
  int days{};  /* time limit in days			*/
};

/* Char's abilities. */
struct abil_data {
  int8_t str{}; /* New stats can go over 18 freely, no more /xx */
  int8_t intel{};
  int8_t wis{};
  int8_t dex{};
  int8_t con{};
  int8_t cha{};
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs. This structure
 * can be changed freely.
 */
struct skill_data {
  int8_t base{};
  int8_t mod{};
  int8_t perf{};
};

struct player_special_data {
  char *poofin{};                          /* Description on arrival of a god.     */
  char *poofout{};                         /* Description upon a god's exit.       */
  std::list<alias_data> aliases;           /* Character's aliases, newest first   */
  int32_t last_tell{};                     /* idnum of last tell from              */
  void *last_olc_targ{};                   /* olc control                          */
  int last_olc_mode{};                     /* olc control                          */
  char *host{};                            /* host of last logon                   */
  int wimp_level{};                        /* Below this # of hit points, flee!	*/
  int8_t freeze_level{};                   /* Level of god who froze char, if any	*/
  int16_t invis_level{};                   /* level of invisibility		*/
  room_vnum load_room{};                   /* Which room to place char in		*/
  std::map<uint16_t, skill_data> skills;   /* by skill id, only those with something set; NPCs have none */
  bitvector_t pref[PR_ARRAY_MAX]{};        /* preference flags for PC's.		*/
  uint8_t bad_pws{};                       /* number of bad password attemps	*/
  int8_t conditions[NUM_CONDITIONS]{};     /* Drunk, full, thirsty			*/

  // Only the character's own class in this array is used, to store PS/Practices.
  int practice_points{};                   /* Practice points available to the character */
  std::deque<std::string> comm_hist[NUM_HIST]; /* each channel's history, oldest first */
  zone_vnum olc_zone{};                    /* Zone where OLC is permitted		*/
  int speaking{};                          /* Language currently speaking		*/

  char *color_choices[NUM_COLOR]{}; /* Choices for custom colors		*/
  int murder{};                     /* Murder of PC's count                 */

  // Attribute training
  int trainstr{};
  int trainint{};
  int traincon{};
  int trainwis{};
  int trainagl{};
  int trainspd{};

  // player characters can carry others.
  int32_t carrying_uid{};
  int32_t carried_by_uid{};

  int racial_pref{};

  // UNUSED
  int gauntlet{};
  int tlevel{};
  int ability_trains{};
  int feat_points{};
  // END UNUSED
};

/* Specials used by NPCs, not PCs */
struct mob_special_data {
  id_list memory;       /* the ids of attackers to remember, newest first */
  int newitem{};        /* Check if mob has new inv item       */

  // BELOW FIELDS ARE UNUSED
  int8_t attack_type{}; /* The Attack Type Bitvector for NPC's     */
  int8_t default_pos{}; /* Default position for NPC                */
  int8_t damnodice{};   /* The number of damage dice's	       */
  int8_t damsizedice{}; /* The size of the damage dice's           */
};

struct char_data {
  int32_t id{};    /* used by DG triggers			*/
  int32_t idnum{}; /* player's idnum; -1 for mobiles	*/

  int pfilepos{};          /* playerfile pos			*/
  mob_vnum nr{};           /* Mob's prototype vnum			*/
  room_vnum in_room{};     /* Location (real room number)		*/
  room_vnum was_in_room{}; /* location for linkdead people		*/
  int wait{};              /* wait for how many loops		*/

  char *name{};        /* PC / NPC s name (kill ...  )		*/
  char *short_descr{}; /* for NPC 'actions'			*/
  char *long_descr{};  /* for 'look'				*/
  char *description{}; /* Extra descriptions                   */
  char *title{};       /* PC / NPC's title                     */

  int size{};   /* Size class of char                   */
  int8_t sex{}; /* PC / NPC's sex                       */
  int race{};

  int chclass{}; /* Last class taken                     */

  // admin stuff
  int admlevel{};                       /* PC / NPC's admin level               */
  bitvector_t admflags[AD_ARRAY_MAX]{}; /* Bitvector for admin privs		*/

  room_vnum hometown{};    /* PC Hometown / NPC spawn room         */
  struct time_data time{}; /* PC's AGE in days			*/
  uint8_t weight{};        /* PC / NPC's weight                    */
  uint8_t height{};        /* PC / NPC's height                    */

  int alignment{}; /* +-1000 for alignment good vs. evil	*/

  // appearance fields
  int8_t hairl{};   /* PC hair length                       */
  int8_t hairs{};   /* PC hair style                        */
  int8_t hairc{};   /* PC hair color                        */
  int8_t skin{};    /* PC skin color                        */
  int8_t eye{};     /* PC eye color                         */
  int8_t distfea{}; /* PC's Distinguishing Feature          */
  int aura{};
  const char *feature{};
  const char *rdisplay{};
  char *voice{}; /* PC's snet voice */

  struct abil_data real_abils{}; /* Abilities without modifiers   */
  struct abil_data aff_abils{};  /* Abils with spells/stones/etc  */
  owned<player_special_data> player_specials; /* players only: null for NPCs */
  /* PC specials				*/
  struct mob_special_data mob_specials{};
  /* NPC specials				*/

  std::list<affected_type> affected; /* affected by what spells, newest first */

  // inventory and equipment
  struct obj_data *equipment[NUM_WEARS]{};
  id_list carrying;     /* the objects it carries, newest first */
  int carry_weight{};   // carried weight
  int carry_items{}; // number of carried items

  struct descriptor_data *desc{}; /* NULL for mobiles			*/
  char *loguser{};                /* What user was I last saved as?      */

  // dgscripts data
  struct trig_proto_list *proto_script{};
  struct script_data *script{};   /* script info for the object		*/
  struct script_memory *memory{}; /* for mob memory triggers		*/

  /* For fighting list			*/

  id_list followers;               /* the characters following it, newest first */
  int32_t master_uid{};            /* Who is char following?		*/
  int32_t master_id{};

  int32_t fighting_uid{}; /* Opponent				*/

  int8_t position{}; /* Standing, fighting, sleeping, etc.	*/

  int timer{}; /* Timer for update			*/

  int32_t sits_uid{}; /* What am I sitting on? */

  // combat blocking
  int32_t blocks_uid{};  /* Who am I blocking?    */
  int32_t blocked_uid{}; /* Who is blocking me?    */

  // android absorb
  int32_t absorbing_uid{}; /* Who am I absorbing */
  int32_t absorbby_uid{};  /* Who is absorbing me */

  // defend stuff
  int32_t defender_uid{};
  int32_t defending_uid{};

  int32_t poisonby_uid{};

  // grappling stuff
  int grap{};
  int32_t grappling_uid{};
  int32_t grappled_uid{};

  // Skill info
  int skill_slots{};
  int forgeting{};
  int forgetcount{};

  bitvector_t act[PM_ARRAY_MAX]{}; /* act flag for NPC's; player flag for PC's */

  bitvector_t affected_by[AF_ARRAY_MAX]{}; /* Bitvector for current affects	*/

  // magic music
  short song{};
  int powerattack{}; /* Setting for power attack level	*/

  // this is actually group kills
  int combatexpertise{}; /* Setting for Combat expertise level   */

  // Money stuff
  int gold{};       /* Money carried			*/
  int bank_gold{};  /* Gold the char has in a bank account	*/
  time_t lastint{}; // last interest time

  // advancement stuff
  int level{};      /* PC / NPC's level                     */
  int64_t exp{};    /* The experience of the player		*/
  int upgrade{};    // android upgrade points
  int race_level{}; /* a mob's entire level (see GET_HITDICE) */
  int level_adj{};

  // Base stats
  int64_t basepl{};
  int64_t baseki{};
  int64_t basest{};

  // used for temporaryt storage of bonuses
  int64_t max_mana{}; /* Max mana for PC/NPC			*/
  int64_t max_hit{};  /* Max hit for PC/NPC			*/
  int64_t max_move{}; /* Max move for PC/NPC			*/
  int64_t max_ki{};   /* Max ki for PC/NPC			*/

  // resource meters, ranges from 0 to 1.0
  double health{};
  double energy{};
  double stamina{};
  double life{};

  // how much you're suppressed.
  int64_t suppression{};

  // charge systemm
  int64_t charge{};
  int64_t chargeto{};

  // current barrier strength
  int64_t barrier{};

  int boosts{};

  int altitude{}; // used for fly/fly higher

  int spam{}; // channel spam

  time_t lastpl{};
  time_t lboard[5]{};

  // absorbtion data for bio/majin
  int absorbs{};
  int ingestLearned{};

  // food, drink, sleep
  int sleeptime{};
  int foodr{};
  int overf{};

  // Saiyan and halfy stuff
  int tail_growth{};
  int rage_meter{};
  short fury{};

  // distance attention stuff
  room_vnum listenroom{};
  int eavesdir{};
  int arenawatch{};

  // dragging
  int32_t drag_uid{};
  int32_t dragged_uid{};

  int32_t mindlink_uid{};
  int lasthit{};

  // limb information... why do we have three of them?
  int limbs[4]{}; /* 0 Right Arm, 1 Left Arm, 2 Right Leg, 3 Left Leg */
  int limb_condition[4]{};
  bitvector_t bodyparts[AF_ARRAY_MAX]{}; /* Bitvector for current bodyparts      */

  time_t rewtime{};

  int genome[2]{}; /* Bio racial bonus, Genome */

  // roleplay points stuff
  int rp{};
  int trp{};
  int rbank{};

  // combo system data
  int combo{};
  int lastattack{};
  int combhits{};

  // spaceship piloting
  int ping{};
  room_vnum radar1{};
  room_vnum radar2{};
  room_vnum radar3{};

  // hoshijin stuff
  int starphase{};
  int mimic{};

  // Chargen bonus/flaw points
  int ccpoints{};
  int negcount{};
  int bonuses[MAX_BONUSES]{};
  int choice{};

  // cooldowns here
  int cooldown{};
  int backstabcool{};
  int con_cooldown{};
  int con_sdcooldown{};
  int gooptime{};

  // Death stuff
  int death_type{};
  int dcount{};
  room_vnum droom{};
  time_t deathtime{};

  // Arlian molt advancement
  int64_t moltexp{};
  int moltlevel{};

  // majinize
  int64_t majinizer{};
  int majinize{};

  // misc combat stuff
  int speedboost{};
  int armor_last{};
  int accuracy{}; /* Base hit accuracy			*/
  int armor{};    /* Internally stored *10		*/

  // transformation data
  int transclass{};
  int transcost[6]{};
  int kaioken{}; // current level of kaioken 0-20

  // Fishing stuff - accuracy_mod is fish_pole_bonus
  int accuracy_mod{}; /* Any bonus or penalty to the accuracy	*/
  int fishstate{};

  char *temp_prompt{};

  int personality{};
  int combine{};
  int linker{};

  int throws{};

  int lifeperc{};

  int mobcharge{};
  int preference{};
  int aggtimer{};

  // miscellaneous bonuses
  int blesslvl{};
  int lifebonus{};
  int asb{}; // auto-skill bonus
  int regen{};

  // multiform stuff
  int32_t original_uid{};
  short clones{};

  int relax_count{};
};

