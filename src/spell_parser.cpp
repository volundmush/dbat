/* ************************************************************************
*   File: spell_parser.c                                Part of CircleMUD *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "dbat/spell_parser.h"
#include "dbat/utils.h"
#include "dbat/interpreter.h"
#include "dbat/spells.h"
#include "dbat/handler.h"
#include "dbat/comm.h"
#include "dbat/db.h"
#include "dbat/dg_scripts.h"
#include "dbat/fight.h"
#include "dbat/act.other.h"
#include "dbat/class.h"

/* extern globals */

/* local globals */
struct spell_info_type spell_info[SKILL_TABLE_SIZE];

/* local functions */
void unused_spell(int spl);

void mag_assign_spells();

/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, 200 should provide
 * ample slots for skills.
 */

struct syllable {
    const char *org;
    const char *news;
};


struct syllable syls[] = {
        {" ",       " "},
        {"ar",      "abra"},
        {"ate",     "i"},
        {"cau",     "kada"},
        {"blind",   "nose"},
        {"bur",     "mosa"},
        {"cu",      "judi"},
        {"de",      "oculo"},
        {"dis",     "mar"},
        {"ect",     "kamina"},
        {"en",      "uns"},
        {"gro",     "cra"},
        {"light",   "dies"},
        {"lo",      "hi"},
        {"magi",    "kari"},
        {"mon",     "bar"},
        {"mor",     "zak"},
        {"move",    "sido"},
        {"ness",    "lacri"},
        {"ning",    "illa"},
        {"per",     "duda"},
        {"ra",      "gru"},
        {"re",      "candus"},
        {"son",     "sabru"},
        {"tect",    "infra"},
        {"tri",     "cula"},
        {"ven",     "nofo"},
        {"word of", "inset"},
        {"a",       "i"},
        {"b",       "v"},
        {"c",       "q"},
        {"d",       "m"},
        {"e",       "o"},
        {"f",       "y"},
        {"g",       "t"},
        {"h",       "p"},
        {"i",       "u"},
        {"j",       "y"},
        {"k",       "t"},
        {"l",       "r"},
        {"m",       "w"},
        {"n",       "b"},
        {"o",       "a"},
        {"p",       "s"},
        {"q",       "d"},
        {"r",       "f"},
        {"s",       "g"},
        {"t",       "h"},
        {"u",       "e"},
        {"v",       "z"},
        {"w",       "x"},
        {"x",       "n"},
        {"y",       "l"},
        {"z",       "k"},
        {"",        ""}
};

const char *unused_spellname = "!UNUSED!"; /* So we can get &unused_spellname */


int mag_kicost(BaseCharacter *ch, int spellnum) {
    int i, min, tval;
    SenseiID whichclass;
    return MAX(SINFO.ki_max - (SINFO.ki_change *
                               (GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS(ch)])),
               SINFO.ki_min);
}



/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and < SKILL_TABLE_SIZE
 */
const char *skill_name(int num) {
    if (num > 0 && num < SKILL_TABLE_SIZE)
        return (spell_info[num].name);
    else if (num == -1)
        return ("UNUSED");
    else
        return ("UNDEFINED");
}

int find_skill_num(char *name, int sktype) {
    int skindex, ok;
    char *temp, *temp2;
    char first[256], first2[256], tempbuf[256];

    for (skindex = 1; skindex < SKILL_TABLE_SIZE; skindex++) {
        if (is_abbrev(name, spell_info[skindex].name) && (spell_info[skindex].skilltype & sktype)) {
            return (skindex);
        }

        ok = true;
        strlcpy(tempbuf, spell_info[skindex].name, sizeof(tempbuf));        /* strlcpy: OK */
        temp = any_one_arg(tempbuf, first);
        temp2 = any_one_arg(name, first2);
        while (*first && *first2 && ok) {
            if (!is_abbrev(first2, first))
                ok = false;
            temp = any_one_arg(temp, first);
            temp2 = any_one_arg(temp2, first2);
        }

        if (ok && !*first2 && (spell_info[skindex].skilltype & sktype)) {
            return (skindex);
        }
    }

    return (-1);
}

int skill_type(int snum) {
    return spell_info[snum].skilltype;
}

void set_skill_type(int snum, int sktype) {
    spell_info[snum].skilltype = sktype;
}


/* Assign the spells on boot up */
void
spello(int spl, const char *name, int max_mana, int min_mana, int mana_change, int minpos, int targets, int violent,
       int routines, int save_flags, int comp_flags, const char *wearoff, int cmspell_level, int school, int domain) {
    int i;

    for (i = 0; i < NUM_CLASSES; i++)
        spell_info[spl].min_level[i] = CONFIG_LEVEL_CAP;
    for (i = 0; i < NUM_RACES; i++)
        spell_info[spl].race_can_learn[i] = CONFIG_LEVEL_CAP;
    spell_info[spl].mana_max = max_mana;
    spell_info[spl].mana_min = min_mana;
    spell_info[spl].mana_change = mana_change;
    spell_info[spl].ki_max = 0;
    spell_info[spl].ki_min = 0;
    spell_info[spl].ki_change = 0;
    spell_info[spl].min_position = minpos;
    spell_info[spl].targets = targets;
    spell_info[spl].violent = violent;
    spell_info[spl].routines = routines;
    spell_info[spl].name = name;
    spell_info[spl].wear_off_msg = wearoff;
    spell_info[spl].skilltype = SKTYPE_SPELL;
    spell_info[spl].flags = 0;
    spell_info[spl].save_flags = save_flags;
    spell_info[spl].comp_flags = comp_flags;
    spell_info[spl].spell_level = cmspell_level;
    spell_info[spl].school = school;
    spell_info[spl].domain = domain;
}


void unused_spell(int spl) {
    int i;

    for (i = 0; i < NUM_CLASSES; i++) {
        spell_info[spl].min_level[i] = CONFIG_LEVEL_CAP;
        spell_info[spl].can_learn_skill[i] = SKLEARN_CROSSCLASS;
    }
    for (i = 0; i < NUM_RACES; i++)
        spell_info[spl].race_can_learn[i] = SKLEARN_CROSSCLASS;
    spell_info[spl].mana_max = 0;
    spell_info[spl].mana_min = 0;
    spell_info[spl].mana_change = 0;
    spell_info[spl].ki_max = 0;
    spell_info[spl].ki_min = 0;
    spell_info[spl].ki_change = 0;
    spell_info[spl].min_position = 0;
    spell_info[spl].targets = 0;
    spell_info[spl].violent = 0;
    spell_info[spl].routines = 0;
    spell_info[spl].name = unused_spellname;
    spell_info[spl].skilltype = SKTYPE_NONE;
    spell_info[spl].flags = 0;
    spell_info[spl].save_flags = 0;
    spell_info[spl].comp_flags = 0;
    spell_info[spl].spell_level = 0;
    spell_info[spl].school = 0;
    spell_info[spl].domain = 0;
}


void skillo(int skill, const char *name, int flags) {
    spello(skill, name, 0, 0, 0, 0, 0, 0, 0, 0, 0, nullptr, 0, 0, 0);
    spell_info[skill].skilltype = SKTYPE_SKILL;
    spell_info[skill].flags = flags;
}

/*
 * Arguments for spello calls:
 *
 * spellnum, maxmana, minmana, manachng, minpos, targets, violent?, routines.
 *
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 * spells.h (such as SPELL_HEAL).
 *
 * maxmana :  The maximum mana this spell will take (i.e., the mana it
 * will take when the player first gets the spell).
 *
 * minmana :  The minimum mana this spell will take, no matter how high
 * level the caster is.
 *
 * manachng:  The change in mana for the spell from level to level.  This
 * number should be positive, but represents the reduction in mana cost as
 * the caster's level increases.
 *
 * minpos  :  Minimum position the caster must be in for the spell to work
 * (usually fighting or standing). targets :  A "list" of the valid targets
 * for the spell, joined with bitwise OR ('|').
 *
 * violent :  TRUE or FALSE, depending on if this is considered a violent
 * spell and should not be cast in PEACEFUL rooms or on yourself.  Should be
 * set on any spell that inflicts damage, is considered aggressive (i.e.
 * charm, curse), or is otherwise nasty.
 *
 * routines:  A list of magic routines which are associated with this spell
 * if the spell uses spell templates.  Also joined with bitwise OR ('|').
 *
 * spellname: The name of the spell.
 *
 * school: The school of the spell.
 *
 * domain: The domain of the spell.
 *
 * See the CircleMUD documentation for a more detailed description of these
 * fields.
 */

/*
 * NOTE: SPELL LEVELS ARE NO LONGER ASSIGNED HERE AS OF Circle 3.0 bpl9.
 * In order to make this cleaner, as well as to make adding new classes
 * much easier, spell levels are now assigned in class.c.  You only need
 * a spello() call to define a new spell; to decide who gets to use a spell
 * or skill, look in class.c.  -JE 5 Feb 1996
 */

void mag_assign_spells() {
    int i;

    /* Do not change the loop below. */
    for (i = 0; i < SKILL_TABLE_SIZE; i++)
        unused_spell(i);
    /* Do not change the loop above. */


    /*
     * Declaration of skills - this actually doesn't do anything except
     * set it up so that immortals can use these skills by default.  The
     * min level to use the skill for other classes is set up in class.c.
     */

    /*
     * skillo does spello and then marks the skill as a new style skill with
     * the appropriate flags.
     */
    /* Buff/Neg Skills */
    skillo(SKILL_FLEX, "flex", SKFLAG_CHAMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_GENIUS, "genius", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_ENLIGHTEN, "enlighten", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_TSKIN, "tough skin", SKFLAG_STRMOD | SKFLAG_ARMORALL);
    skillo(SKILL_KAIOKEN, "kaioken", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_BLESS, "bless", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_CURSE, "curse", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_POISON, "poison", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_VIGOR, "vigor", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_POSE, "special pose", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_HASSHUKEN, "hasshuken", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);

    /* Effect Skills */
    skillo(SKILL_GARDENING, "gardening", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_EXTRACT, "extract", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_RUNIC, "runic", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_COMMUNE, "commune", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_SOLARF, "solar flare", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_MIGHT, "might", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_BALANCE, "balance", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
    skillo(SKILL_BUILD, "build", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_CONCENTRATION, "concentration", SKFLAG_CONMOD);
    skillo(SKILL_SPOT, "spot", SKFLAG_WISMOD);
    skillo(SKILL_FIRST_AID, "first aid", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_DISGUISE, "disguise", SKFLAG_CHAMOD);
    skillo(SKILL_ESCAPE_ARTIST, "escape", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
    skillo(SKILL_APPRAISE, "appraise", SKFLAG_INTMOD);
    skillo(SKILL_HEAL, "heal", SKFLAG_WISMOD | SKFLAG_ARMORBAD);
    skillo(SKILL_FORGERY, "forgery", SKFLAG_INTMOD);
    skillo(SKILL_HIDE, "hide", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
    skillo(SKILL_LISTEN, "listen", SKFLAG_WISMOD);
    skillo(SKILL_EAVESDROP, "eavesdrop", SKFLAG_INTMOD);
    skillo(SKILL_CURE, "cure poison", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_OPEN_LOCK, "open lock", SKFLAG_DEXMOD | SKFLAG_NEEDTRAIN | SKFLAG_ARMORBAD);
    skillo(SKILL_REGENERATE, "regenerate", SKFLAG_CONMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_KEEN, "keen sight", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_SEARCH, "search", SKFLAG_INTMOD);
    skillo(SKILL_MOVE_SILENTLY, "move silently", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
    skillo(SKILL_ABSORB, "absorb", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_SLEIGHT_OF_HAND, "sleight of hand", SKFLAG_DEXMOD | SKFLAG_ARMORALL);
    skillo(SKILL_INGEST, "ingest", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_REPAIR, "fix", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_SENSE, "sense", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_SURVIVAL, "survival", SKFLAG_WISMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_YOIK, "yoikominminken", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_CREATE, "create", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_SPIT, "stone spit", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_POTENTIAL, "potential release", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_TELEPATHY, "telepathy", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_FOCUS, "focus", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_INSTANTT, "instant transmission", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);

    /* Weapon Skills */
    skillo(SKILL_SWORD, "sword", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_DAGGER, "dagger", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_CLUB, "club", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_SPEAR, "spear", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_GUN, "gun", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_BRAWL, "brawl", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);

    /* Defensive Skills */
    skillo(SKILL_DODGE, "dodge", SKFLAG_CHAMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_PARRY, "parry", SKFLAG_DEXMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_BLOCK, "block", SKFLAG_DEXMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_ZANZOKEN, "zanzoken", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_BARRIER, "barrier", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);

    /* Offensive Skills */
    skillo(SKILL_THROW, "throw", SKFLAG_DEXMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_PUNCH, "punch", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_KICK, "kick", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_ELBOW, "elbow", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_KNEE, "knee", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_ROUNDHOUSE, "roundhouse", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_UPPERCUT, "uppercut", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_SLAM, "slam", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_HEELDROP, "heeldrop", SKFLAG_STRMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_KIBALL, "kiball", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_KIBLAST, "kiblast", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_BEAM, "beam", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_TSUIHIDAN, "tsuihidan", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_SHOGEKIHA, "shogekiha", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_RENZO, "renzokou energy dan", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_MASENKO, "masenko", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_DODONPA, "dodonpa", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_GALIKGUN, "galik gun", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_KAMEHAMEHA, "kamehameha", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_DEATHBEAM, "deathbeam", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_ERASER, "eraser cannon", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_TSLASH, "twin slash", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_PSYBLAST, "psychic blast", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_HONOO, "honoo", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_DUALBEAM, "dual beam", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_ROGAFUFUKEN, "rogafufuken", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_BAKUHATSUHA, "bakuhatsuha", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_KIENZAN, "kienzan", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_TRIBEAM, "tribeam", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_SBC, "special beam cannon", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_FINALFLASH, "final flash", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_CRUSHER, "crusher ball", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_DDSLASH, "darkness dragon slash", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_PBARRAGE, "psychic barrage", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_HELLFLASH, "hell flash", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_HELLSPEAR, "hell spear blast", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_KAKUSANHA, "kakusanha", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_SCATTER, "scatter shot", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_BIGBANG, "big bang", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_PSLASH, "phoenix slash", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_DEATHBALL, "deathball", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_SPIRITBALL, "spirit ball", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_GENKIDAMA, "genki dama", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER5);
    skillo(SKILL_GENOCIDE, "genocide", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER5);
    skillo(SKILL_DUALWIELD, "dual wield", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_TWOHAND, "twohand", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_STYLE, "fighting arts", SKFLAG_INTMOD);
    skillo(SKILL_KURA, "kuraiiro seiki", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_TAILWHIP, "tailwhip", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER1);
    skillo(SKILL_KOUSENGAN, "kousengan", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER1);
    skillo(SKILL_TAISHA, "taisha reiki", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_PARALYZE, "paralyze", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_INFUSE, "infuse", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_ROLL, "roll", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_TRIP, "trip", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_GRAPPLE, "grapple", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_WSPIKE, "water spikes", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_SELFD, "self destruct", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_SPIRAL, "spiral comet", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_BREAKER, "star breaker", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_MIMIC, "mimic", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_WRAZOR, "water razor", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_KOTEIRU, "koteiru bakuha", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_DIMIZU, "dimizu toride", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_HYOGA_KABE, "hyoga kabe", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_WELLSPRING, "wellspring", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_AQUA_BARRIER, "aqua barrier", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_WARP, "warp pool", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_HSPIRAL, "hell spiral", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_ARMOR, "nanite armor", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_FIRESHIELD, "fireshield", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_COOKING, "cooking", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_SEISHOU, "seishou enko", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER2);
    skillo(SKILL_SILK, "silk", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_BASH, "bash", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_HEADBUTT, "headbutt", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_ENSNARE, "ensnare", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_STARNOVA, "starnova", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_MALICE, "malice breaker", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_ZEN, "zen blade strike", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER3);
    skillo(SKILL_SUNDER, "sundering force", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_WITHER, "wither", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_METAMORPH, "dark metamorphosis", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_HAYASA, "hayasa", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_ENERGIZE, "energize throwing", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_PURSUIT, "pursuit", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_HEALGLOW, "healing glow", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_HANDLING, "handling", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_MYSTICMUSIC, "mystic music", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN);
    skillo(SKILL_LIGHTGRENADE, "light grenade", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_MULTIFORM, "multiform", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER1);
    skillo(SKILL_SPIRITCONTROL, "spirit control", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER1);
    skillo(SKILL_BALEFIRE, "balefire", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER4);
    skillo(SKILL_BLESSEDHAMMER, "blessed hammer", SKFLAG_INTMOD | SKFLAG_NEEDTRAIN | SKFLAG_TIER1);
}
