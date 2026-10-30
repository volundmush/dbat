#pragma once
/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE              0	/* No effect			*/
#define APPLY_STR               1	/* Apply to strength		*/
#define APPLY_DEX               2	/* Apply to dexterity		*/
#define APPLY_INT               3	/* Apply to intelligence	*/
#define APPLY_WIS               4	/* Apply to wisdom		*/
#define APPLY_CON               5	/* Apply to constitution	*/
#define APPLY_CHA		6	/* Apply to charisma		*/
#define APPLY_CLASS             7	/* Reserved			*/
#define APPLY_LEVEL             8	/* Reserved			*/
#define APPLY_AGE               9	/* Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/* Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/* Apply to height		*/
#define APPLY_MANA             12	/* Apply to max mana		*/
#define APPLY_HIT              13	/* Apply to max hit points	*/
#define APPLY_MOVE             14	/* Apply to max move points	*/
#define APPLY_GOLD             15	/* Reserved			*/
#define APPLY_EXP              16	/* Reserved			*/
#define APPLY_AC               17	/* Apply to Armor Class		*/
#define APPLY_ACCURACY         18	/* Apply to accuracy		*/
#define APPLY_DAMAGE           19	/* Apply to damage 		*/
#define APPLY_REGEN	       20	/* Regen Rate Buffed            */
#define APPLY_TRAIN	       21	/* Skill training rate buffed   */
#define APPLY_LIFEMAX	       22	/* Life Force max buffed        */
#define APPLY_UNUSED3	       23	/* Unused			*/
#define APPLY_UNUSED4	       24	/* Unused			*/
#define APPLY_RACE             25       /* Apply to race                */
#define APPLY_TURN_LEVEL       26       /* Apply to turn undead         */
#define APPLY_SPELL_LVL_0      27       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_1      28       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_2      29       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_3      30       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_4      31       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_5      32       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_6      33       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_7      34       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_8      35       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_9      36       /* Apply to spell cast per day  */
#define APPLY_KI               37	/* Apply to max ki		*/
#define APPLY_FORTITUDE        38	/* Apply to fortitue save	*/
#define APPLY_REFLEX           39	/* Apply to reflex save		*/
#define APPLY_WILL             40	/* Apply to will save		*/
#define APPLY_SKILL            41       /* Apply to a specific skill    */
#define APPLY_FEAT             42       /* Apply to a specific feat     */
#define APPLY_ALLSAVES         43       /* Apply to all 3 save types 	*/
#define APPLY_RESISTANCE       44       /* Apply to resistance	 	*/
#define APPLY_ALL_STATS        45       /* Apply to all attributes	*/

#define NUM_APPLIES 46

extern const char *apply_types[NUM_APPLIES+1];