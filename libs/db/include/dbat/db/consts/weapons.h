#pragma once
/* Character Creation Styles */
#define WIELD_NONE        0
#define WIELD_LIGHT       1
#define WIELD_ONEHAND     2
#define WIELD_TWOHAND     3

/* Number of weapon types */
#define MAX_WEAPON_TYPES            26

/* Critical hit types */
#define CRIT_X2		0
#define CRIT_X3		1
#define CRIT_X4		2

#define MAX_CRIT_TYPE	CRIT_X4
#define NUM_CRIT_TYPES 3

/* Combat feats that apply to a specific weapon type */
#define CFEAT_IMPROVED_CRITICAL			0
#define CFEAT_WEAPON_FINESSE			1
#define CFEAT_WEAPON_FOCUS			2
#define CFEAT_WEAPON_SPECIALIZATION		3
#define CFEAT_GREATER_WEAPON_FOCUS		4
#define CFEAT_GREATER_WEAPON_SPECIALIZATION	5

#define CFEAT_MAX				5