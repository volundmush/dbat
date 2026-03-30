#pragma once


#define SPELL_LEVEL_0     0 
#define SPELL_LEVEL_1     1
#define SPELL_LEVEL_2     2
#define SPELL_LEVEL_3     3
#define SPELL_LEVEL_4     4
#define SPELL_LEVEL_5     5
#define SPELL_LEVEL_6     6
#define SPELL_LEVEL_7     7
#define SPELL_LEVEL_8     8
#define SPELL_LEVEL_9     9

#define MAX_SPELL_LEVEL   10                    /* how many spell levels */
#define MAX_MEM          (MAX_SPELL_LEVEL * 10) /* how many total spells */

#define DOMAIN_UNDEFINED	-1
#define DOMAIN_AIR		0
#define DOMAIN_ANIMAL		1
#define DOMAIN_CHAOS		2
#define DOMAIN_DEATH		3
#define DOMAIN_DESTRUCTION	4
#define DOMAIN_EARTH		5
#define DOMAIN_EVIL		6
#define DOMAIN_FIRE		7
#define DOMAIN_GOOD		8
#define DOMAIN_HEALING		9
#define DOMAIN_KNOWLEDGE	10
#define DOMAIN_LAW		11
#define DOMAIN_LUCK		12
#define DOMAIN_MAGIC		13
#define DOMAIN_PLANT		14
#define DOMAIN_PROTECTION	15
#define DOMAIN_STRENGTH		16
#define DOMAIN_SUN		17
#define DOMAIN_TRAVEL		18
#define DOMAIN_TRICKERY		19
#define DOMAIN_UNIVERSAL	20
#define DOMAIN_WAR		22
#define DOMAIN_WATER		23
#define DOMAIN_ARTIFACE         24
#define DOMAIN_CHARM            25
#define DOMAIN_COMMUNITY        26
#define DOMAIN_CREATION         27
#define DOMAIN_DARKNESS         28
#define DOMAIN_GLORY            29
#define DOMAIN_LIBERATION       30
#define DOMAIN_MADNESS          31
#define DOMAIN_NOBILITY         32
#define DOMAIN_REPOSE           33
#define DOMAIN_RUNE             34
#define DOMAIN_SCALYKIND        35
#define DOMAIN_WEATHER          36

#define NUM_DOMAINS		37

#define SCHOOL_UNDEFINED	-1
#define SCHOOL_ABJURATION	0
#define SCHOOL_CONJURATION	1
#define SCHOOL_DIVINATION	2
#define SCHOOL_ENCHANTMENT	3
#define SCHOOL_EVOCATION	4
#define SCHOOL_ILLUSION		5
#define SCHOOL_NECROMANCY	6
#define SCHOOL_TRANSMUTATION	7
#define SCHOOL_UNIVERSAL	8

#define NUM_SCHOOLS		10

#define DEITY_UNDEFINED			-1

#define NUM_DEITIES			0

/* Spell feats that apply to a specific school of spells */
#define CFEAT_SPELL_FOCUS			0
#define CFEAT_GREATER_SPELL_FOCUS		1

#define SFEAT_MAX				1