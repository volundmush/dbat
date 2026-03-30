#pragma once

/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING	 0	/* Playing - Nominal state		*/
#define CON_CLOSE	 1	/* User disconnect, remove character.	*/
#define CON_GET_NAME	 2	/* By what name ..?			*/
#define CON_NAME_CNFRM	 3	/* Did I get that right, x?		*/
#define CON_PASSWORD	 4	/* Password:				*/
#define CON_NEWPASSWD	 5	/* Give me a password for x		*/
#define CON_CNFPASSWD	 6	/* Please retype password:		*/
#define CON_QSEX	 7	/* Sex?					*/
#define CON_QCLASS	 8	/* Class?				*/
#define CON_RMOTD	 9	/* PRESS RETURN after MOTD		*/
#define CON_MENU	 10	/* Your choice: (main menu)		*/
#define CON_EXDESC	 11	/* Enter a new description:		*/
#define CON_CHPWD_GETOLD 12	/* Changing passwd: get old		*/
#define CON_CHPWD_GETNEW 13	/* Changing passwd: get new		*/
#define CON_CHPWD_VRFY   14	/* Verify new password			*/
#define CON_DELCNF1	 15	/* Delete confirmation 1		*/
#define CON_DELCNF2	 16	/* Delete confirmation 2		*/
#define CON_DISCONNECT	 17	/* In-game link loss (leave character)	*/
#define CON_OEDIT	 18	/* OLC mode - object editor		*/
#define CON_REDIT	 19	/* OLC mode - room editor		*/
#define CON_ZEDIT	 20	/* OLC mode - zone info editor		*/
#define CON_MEDIT	 21	/* OLC mode - mobile editor		*/
#define CON_SEDIT	 22	/* OLC mode - shop editor		*/
#define CON_TEDIT	 23	/* OLC mode - text editor		*/
#define CON_CEDIT	 24	/* OLC mode - config editor		*/
#define CON_QRACE        25     /* Race? 				*/
#define CON_ASSEDIT      26     /* OLC mode - Assemblies                */
#define CON_AEDIT        27	/* OLC mode - social (action) edit      */
#define CON_TRIGEDIT     28	/* OLC mode - trigger edit              */
#define CON_RACE_HELP    29	/* Race Help 				*/
#define CON_CLASS_HELP   30	/* Class Help 				*/
#define CON_QANSI	 31	/* Ask for ANSI support     */
#define CON_GEDIT	 32	/* OLC mode - guild editor 		*/
#define CON_QROLLSTATS	 33	/* Reroll stats 			*/
#define CON_IEDIT        34	/* OLC mode - individual edit		*/
#define CON_LEVELUP	 35	/* Level up menu			*/
#define CON_QSTATS 	 36	/* Assign starting stats        	*/
#define CON_HAIRL        37     /* Choose your hair length        */
#define CON_HAIRS        38     /* Choose your hair style         */
#define CON_HAIRC        39     /* Choose your hair color         */
#define CON_SKIN         40     /* Choose your skin color         */
#define CON_EYE          41     /* Choose your eye color          */
#define CON_Q1           42     /* Make a life choice!            */
#define CON_Q2           43     /* Make a second life choice!     */
#define CON_Q3           44     /* Make a third life choice!      */
#define CON_Q4           45     /* Make a fourth life choice!     */
#define CON_Q5           46     /* Make a fifth life choice!      */
#define CON_Q6           47     /* Make a sixth life choice!      */
#define CON_Q7           48     /* Make a seventh life choice!    */
#define CON_Q8           49     /* Make an eighth life choice!    */
#define CON_Q9           50     /* Make a ninth life choice!      */
#define CON_QX           51     /* Make a tenth life choice!      */
#define CON_HSEDIT       52     /* House Olc                      */
#define CON_ALPHA        53     /* Alpha Password                 */
#define CON_ALPHA2       54     /* Alpha Password For Newb        */
#define CON_ANDROID      55
#define CON_HEDIT        56     /* OLC mode - help edit           */
#define CON_GET_USER     57
#define CON_GET_EMAIL    58
#define CON_UMENU        59
#define CON_USER_CONF    60
#define CON_DISTFEA      61
#define CON_HW           62
#define CON_AURA         63
#define CON_BONUS        64
#define CON_NEGATIVE     65
#define CON_NEWSEDIT     66
#define CON_RACIAL       67
#define CON_POBJ         68
#define CON_ALIGN        69
#define CON_SKILLS       70
#define CON_USER_TITLE   71
#define CON_GENOME       72

#define NUM_CON_TYPES 73