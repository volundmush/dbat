#pragma once
/* Modifier constants used with obj affects ('A' fields) */
constexpr int APPLY_NONE              = 0; /* No effect			*/
constexpr int APPLY_STR               = 1; /* Apply to strength		*/
constexpr int APPLY_DEX               = 2; /* Apply to dexterity		*/
constexpr int APPLY_INT               = 3; /* Apply to intelligence	*/
constexpr int APPLY_WIS               = 4; /* Apply to wisdom		*/
constexpr int APPLY_CON               = 5; /* Apply to constitution	*/
constexpr int APPLY_CHA		= 6; /* Apply to charisma		*/
constexpr int APPLY_CLASS             = 7; /* Reserved			*/
constexpr int APPLY_LEVEL             = 8; /* Reserved			*/
constexpr int APPLY_AGE               = 9; /* Apply to age			*/
constexpr int APPLY_CHAR_WEIGHT      = 10; /* Apply to weight		*/
constexpr int APPLY_CHAR_HEIGHT      = 11; /* Apply to height		*/
constexpr int APPLY_MANA             = 12; /* Apply to max mana		*/
constexpr int APPLY_HIT              = 13; /* Apply to max hit points	*/
constexpr int APPLY_MOVE             = 14; /* Apply to max move points	*/
constexpr int APPLY_GOLD             = 15; /* Reserved			*/
constexpr int APPLY_EXP              = 16; /* Reserved			*/
constexpr int APPLY_AC               = 17; /* Apply to Armor Class		*/
constexpr int APPLY_ACCURACY         = 18; /* Apply to accuracy		*/
constexpr int APPLY_DAMAGE           = 19; /* Apply to damage 		*/
constexpr int APPLY_REGEN	       = 20; /* Regen Rate Buffed            */
constexpr int APPLY_TRAIN	       = 21; /* Skill training rate buffed   */
constexpr int APPLY_LIFEMAX	       = 22; /* Life Force max buffed        */
constexpr int APPLY_UNUSED3	       = 23; /* Unused			*/
constexpr int APPLY_UNUSED4	       = 24; /* Unused			*/
constexpr int APPLY_RACE             = 25; /* Apply to race                */
constexpr int APPLY_TURN_LEVEL       = 26; /* Apply to turn undead         */
constexpr int APPLY_SPELL_LVL_0      = 27; /* Apply to spell cast per day  */
constexpr int APPLY_SPELL_LVL_1      = 28; /* Apply to spell cast per day  */
constexpr int APPLY_SPELL_LVL_2      = 29; /* Apply to spell cast per day  */
constexpr int APPLY_SPELL_LVL_3      = 30; /* Apply to spell cast per day  */
constexpr int APPLY_SPELL_LVL_4      = 31; /* Apply to spell cast per day  */
constexpr int APPLY_SPELL_LVL_5      = 32; /* Apply to spell cast per day  */
constexpr int APPLY_SPELL_LVL_6      = 33; /* Apply to spell cast per day  */
constexpr int APPLY_SPELL_LVL_7      = 34; /* Apply to spell cast per day  */
constexpr int APPLY_SPELL_LVL_8      = 35; /* Apply to spell cast per day  */
constexpr int APPLY_SPELL_LVL_9      = 36; /* Apply to spell cast per day  */
constexpr int APPLY_KI               = 37; /* Apply to max ki		*/
constexpr int APPLY_FORTITUDE        = 38; /* Apply to fortitue save	*/
constexpr int APPLY_REFLEX           = 39; /* Apply to reflex save		*/
constexpr int APPLY_WILL             = 40; /* Apply to will save		*/
constexpr int APPLY_SKILL            = 41; /* Apply to a specific skill    */
constexpr int APPLY_FEAT             = 42; /* Apply to a specific feat     */
constexpr int APPLY_ALLSAVES         = 43; /* Apply to all 3 save types 	*/
constexpr int APPLY_RESISTANCE       = 44; /* Apply to resistance	 	*/
constexpr int APPLY_ALL_STATS        = 45; /* Apply to all attributes	*/

constexpr int NUM_APPLIES = 46;

extern const char *apply_types[NUM_APPLIES+1];