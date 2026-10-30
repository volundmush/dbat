#pragma once
/*
 * ADM flags - define admin privs for chars
 */
#define ADM_TELLALL		0	/* Can use 'tell all' to broadcast GOD */
#define ADM_SEEINV		1	/* Sees other chars inventory IMM */
#define ADM_SEESECRET		2	/* Sees secret doors IMM */
#define ADM_KNOWWEATHER		3	/* Knows details of weather GOD */
#define ADM_FULLWHERE		4	/* Full output of 'where' command IMM */
#define ADM_MONEY 		5	/* Char has a bottomless wallet GOD */
#define ADM_EATANYTHING 	6	/* Char can eat anything GOD */
#define ADM_NOPOISON	 	7	/* Char can't be poisoned IMM */
#define ADM_WALKANYWHERE	8	/* Char has unrestricted walking IMM */
#define ADM_NOKEYS		9	/* Char needs no keys for locks GOD */
#define ADM_INSTANTKILL		10	/* "kill" command is instant IMPL */
#define ADM_NOSTEAL		11	/* Char cannot be stolen from IMM */
#define ADM_TRANSALL		12	/* Can use 'trans all' GRGOD */
#define ADM_SWITCHMORTAL	13	/* Can 'switch' to a mortal PC body IMPL */
#define ADM_FORCEMASS		14	/* Can force rooms or all GRGOD */
#define ADM_ALLHOUSES		15	/* Can enter any house GRGOD */
#define ADM_NODAMAGE		16	/* Cannot be damaged IMM */
#define ADM_ALLSHOPS		17	/* Can use all shops GOD */
#define ADM_CEDIT		18	/* Can use cedit IMPL */

#define NUM_ADMFLAGS            19
#define AD_ARRAY_MAX	4
extern const char *admin_flags[NUM_ADMFLAGS+1];