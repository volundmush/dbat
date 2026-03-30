#pragma once

/* Affect bits: used in char_data.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_DONTUSE           0    /* DON'T USE! 		*/
#define AFF_BLIND             1    /* (R) Char is blind         */
#define AFF_INVISIBLE         2    /* Char is invisible         */
#define AFF_DETECT_ALIGN      3    /* Char is sensitive to align*/
#define AFF_DETECT_INVIS      4    /* Char can see invis chars  */
#define AFF_DETECT_MAGIC      5    /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE        6    /* Char can sense hidden life*/
#define AFF_WATERWALK         7    /* Char can walk on water    */
#define AFF_SANCTUARY         8    /* Char protected by sanct.  */
#define AFF_GROUP             9    /* (R) Char is grouped       */
#define AFF_CURSE             10   /* Char is cursed            */
#define AFF_INFRAVISION       11   /* Char can see in dark      */
#define AFF_POISON            12   /* (R) Char is poisoned      */
#define AFF_WEAKENED_STATE    13   /* Char protected from evil  */
#define AFF_PROTECT_GOOD      14   /* Char protected from good  */
#define AFF_SLEEP             15   /* (R) Char magically asleep */
#define AFF_NOTRACK           16   /* Char can't be tracked     */
#define AFF_UNDEAD            17   /* Char is undead 		*/
#define AFF_PARALYZE          18   /* Char is paralized		*/
#define AFF_SNEAK             19   /* Char can move quietly     */
#define AFF_HIDE              20   /* Char is hidden            */
#define AFF_UNUSED20          21   /* Room for future expansion */
#define AFF_CHARM             22   /* Char is charmed         	*/
#define AFF_FLYING            23   /* Char is flying         	*/
#define AFF_WATERBREATH       24   /* Char can breath non O2    */
#define AFF_ANGELIC           25   /* Char is an angelic being  */
#define AFF_ETHEREAL          26   /* Char is ethereal          */
#define AFF_MAGICONLY         27   /* Char only hurt by magic   */
#define AFF_NEXTPARTIAL       28   /* Next action cannot be full*/
#define AFF_NEXTNOACTION      29   /* Next action cannot attack (took full action between rounds) */
#define AFF_STUNNED           30   /* Char is stunned		*/
#define AFF_TAMED             31   /* Char has been tamed	*/
#define AFF_CDEATH            32   /* Char is undergoing creeping death */
#define AFF_SPIRIT            33   /* Char has no body          */
#define AFF_STONESKIN         34   /* Char has temporary DR     */
#define AFF_SUMMONED          35   /* Char is summoned (i.e. transient */
#define AFF_CELESTIAL         36   /* Char is celestial         */
#define AFF_FIENDISH          37   /* Char is fiendish          */
#define AFF_FIRE_SHIELD       38   /* Char has fire shield      */
#define AFF_LOW_LIGHT         39   /* Char has low light eyes   */
#define AFF_ZANZOKEN          40   /* Char is ready to zanzoken */
#define AFF_KNOCKED           41   /* Char is knocked OUT!      */
#define AFF_MIGHT             42   /* Strength +3               */
#define AFF_FLEX              43   /* Agility +3                */
#define AFF_GENIUS            44   /* Intelligence +3           */
#define AFF_BLESS             45   /* Bless for better regen    */
#define AFF_BURNT             46   /* Disintergrated corpse     */
#define AFF_BURNED            47   /* Burned by honoo or similar skill */
#define AFF_MBREAK            48   /* Can't charge while flagged */
#define AFF_HASS              49   /* Does double punch damage  */
#define AFF_FUTURE            50   /* Future Sight */
#define AFF_PARA              51   /* Real Paralyze */
#define AFF_INFUSE            52   /* Ki infused attacks */
#define AFF_ENLIGHTEN         53   /* Enlighten */
#define AFF_FROZEN            54   /* They got frozededed */
#define AFF_FIRESHIELD        55   /* They have a blazing personality */
#define AFF_ENSNARED          56   /* They have silk ensnaring their arms! */
#define AFF_HAYASA            57   /* They are speedy!                */
#define AFF_PURSUIT           58   /* Being followed */
#define AFF_WITHER            59   /* Their body is withered */
#define AFF_HYDROZAP          60   /* Custom Skill Kanso Suru */
#define AFF_POSITION          61   /* Better combat position */
#define AFF_SHOCKED           62   /* Psychic Shock          */
#define AFF_METAMORPH         63   /* Metamorphisis, Demon's Ripoff Custom Skill */
#define AFF_HEALGLOW          64   /* Healing Glow */
#define AFF_EARMOR            65   /* Ethereal Armor */
#define AFF_ECHAINS           66   /* Ethereal Chains */
#define AFF_WUNJO             67   /* Wunjo rune */
#define AFF_POTENT            68   /* Purisaz rune */
#define AFF_ASHED             69   /* Leaves ash */
#define AFF_PUKED             70
#define AFF_LIQUEFIED         71
#define AFF_SHELL             72
#define AFF_IMMUNITY          73
#define AFF_SPIRITCONTROL     74

#define NUM_AFF_FLAGS 75