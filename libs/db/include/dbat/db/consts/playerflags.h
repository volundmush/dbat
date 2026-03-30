#pragma once

/* Player flags: used by char_data.act */
#define PLR_KILLER	0   /* Player is a player-killer        */
#define PLR_THIEF	1   /* Player is a player-thief         */
#define PLR_FROZEN	2   /* Player is frozen                 */
#define PLR_DONTSET	3   /* Don't EVER set (ISNPC bit) 	*/
#define PLR_WRITING	4   /* Player writing (board/mail/olc)  */
#define PLR_MAILING	5   /* Player is writing mail           */
#define PLR_CRASH	6   /* Player needs to be crash-saved   */
#define PLR_SITEOK	7   /* Player has been site-cleared     */
#define PLR_NOSHOUT	8   /* Player not allowed to shout/goss */
#define PLR_NOTITLE	9   /* Player not allowed to set title  */
#define PLR_DELETED	10  /* Player deleted - space reusable  */
#define PLR_LOADROOM	11  /* Player uses nonstandard loadroom */
#define PLR_NOWIZLIST	12  /* Player shouldn't be on wizlist  	*/
#define PLR_NODELETE	13  /* Player shouldn't be deleted     	*/
#define PLR_INVSTART	14  /* Player should enter game wizinvis*/
#define PLR_CRYO	15  /* Player is cryo-saved (purge prog)*/
#define PLR_NOTDEADYET	16  /* (R) Player being extracted.     	*/
#define PLR_AGEMID_G	17  /* Player has had pos of middle age	*/
#define PLR_AGEMID_B	18  /* Player has had neg of middle age	*/
#define PLR_AGEOLD_G	19  /* Player has had pos of old age	*/
#define PLR_AGEOLD_B	20  /* Player has had neg of old age	*/
#define PLR_AGEVEN_G	21  /* Player has had pos of venerable age	*/
#define PLR_AGEVEN_B	22  /* Player has had neg of venerable age	*/
#define PLR_OLDAGE	23  /* Player is dead of old age	*/
#define PLR_RARM        24  /* Player has a right arm           */
#define PLR_LARM        25  /* Player has a left arm            */
#define PLR_RLEG        26  /* Player has a right leg           */
#define PLR_LLEG        27  /* Player has a left leg            */
#define PLR_HEAD        28  /* Player has a head                */
#define PLR_STAIL       29  /* Player has a saiyan tail         */
#define PLR_TAIL        30  /* Player has a non-saiyan tail     */
#define PLR_PILOTING    31  /* Player is sitting in the pilots chair */
#define PLR_SKILLP      32  /* Player made a good choice in CC  */
#define PLR_SPAR        33  /* Player is in a spar stance       */
#define PLR_CHARGE      34  /* Player is charging               */
#define PLR_TRANS1      35  /* Transformation 1                 */
#define PLR_TRANS2      36  /* Transformation 2                 */
#define PLR_TRANS3      37  /* Transformation 3                 */
#define PLR_TRANS4      38  /* Transformation 4                 */
#define PLR_TRANS5      39  /* Transformation 5                 */
#define PLR_TRANS6      40  /* Transformation 6                 */
#define PLR_ABSORB      41  /* Absorb model                     */
#define PLR_REPAIR      42  /* Repair model                     */
#define PLR_SENSEM      43  /* Sense-Powersense model           */
#define PLR_POWERUP     44  /* Powering Up                      */
#define PLR_KNOCKED     45  /* Knocked OUT                      */
#define PLR_CRARM       46  /* Cybernetic Right Arm             */
#define PLR_CLARM       47  /* Cybernetic Left Arm              */
#define PLR_CRLEG       48  /* Cybernetic Right Leg             */
#define PLR_CLLEG       49  /* Cybernetic Left Leg              */
#define PLR_FPSSJ       50  /* Full Power Super Saiyan          */
#define PLR_IMMORTAL    51  /* The player is immortal           */
#define PLR_EYEC        52  /* The player has their eyes closed */
#define PLR_DISGUISED   53  /* The player is disguised          */
#define PLR_BANDAGED    54  /* THe player has been bandaged     */
#define PLR_PR          55  /* Has had their potential released */
#define PLR_HEALT       56  /* Is inside a healing tank         */
#define PLR_FURY        57  /* Is in fury mode                  */
#define PLR_POSE        58  /* Ginyu Pose Effect                */
#define PLR_OOZARU      59
#define PLR_ABSORBED    60
#define PLR_MULTP       61
#define PLR_PDEATH      62
#define PLR_THANDW      63
#define PLR_SELFD       64
#define PLR_SELFD2      65
#define PLR_SPIRAL      66
#define PLR_BIOGR       67
#define PLR_LSSJ        68
#define PLR_REPLEARN    69
#define PLR_FORGET      70
#define PLR_TRANSMISSION 71
#define PLR_FISHING     72
#define PLR_GOOP        73
#define PLR_MULTIHIT    74
#define PLR_AURALIGHT   75
#define PLR_RDISPLAY    76
#define PLR_STOLEN      77
#define PLR_TAILHIDE    78  /* Hides tail for S & HB            */
#define PLR_NOGROW      79  /* Halt Growth for S & HB           */

#define NUM_PLR_FLAGS 80