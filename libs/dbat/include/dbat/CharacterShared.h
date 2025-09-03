#pragma once
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include "const/Appearance.h"
#include "const/Position.h"

#include "const/Race.h"
#include "const/Sensei.h"
#include "const/Sex.h"
#include "const/CharacterFlag.h"
#include "const/MobFlag.h"
#include "const/AdminFlag.h"
#include "const/Mutation.h"
#include "const/Size.h"
#include "Flags.h"

struct CharacterPrototype;
struct Character;

/* These data contain information about a players time data */
struct time_data {
    time_data() = default;
    int64_t birth{};    /* NO LONGER USED This represents the characters current IC age        */
    time_t created{};    /* This does not change                              */
    int64_t maxage{};    /* This represents death by natural causes (UNUSED) */
    time_t logon{};    /* Time of the last logon (used to calculate played) */
    double played{};    /* This is the total accumulated time played in secs */
    double seconds_aged{}; // The player's current IC age, in seconds.
    int currentAge();
};


/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs. This structure
 * can be changed freely.
 */
struct alias_data {
    std::string name;
    std::string replacement;
    int type{};
};

/* Specials used by NPCs, not PCs */
struct mob_special_data {
    std::list<std::weak_ptr<Character>> memory{};        /* List of attackers to remember	       */
    int attack_type{};        /* The Attack Type Bitvector for NPC's     */
    int default_pos{POS_STANDING};        /* Default position for NPC                */
    int damnodice{};          /* The number of damage dice's	       */
    int damsizedice{};        /* The size of the damage dice's           */
    bool newitem{};             /* Check if mob has new inv item       */
};

/* Queued spell entry */
struct queued_act {
    int level;
    int spellnum;
};


enum ResurrectionMode : uint8_t {
    Costless = 0,
    Basic = 1,
    RPP = 2
};

struct skill_data {
    int16_t level{0};
    int16_t perfs{0};
};

struct trans_data {

    std::string description{};
    double time_spent_in_form{0.0};
    int grade = 1;
    bool visible = true;
    bool limit_broken = false;
    bool unlocked = false;

    std::unordered_map<Appearance, std::string> appearances;
    std::unordered_map<std::string, double> vars;

    double blutz{0.0}; // The number of seconds you can spend in Oozaru.
};

enum Task 
{
    nothing = 0,
    meditate = 1,
    situps = 2,
    pushups = 3,
    crafting = 4,

    trainStr = 10,
    trainAgl = 11,
    trainCon = 12,
    trainSpd = 13,
    trainInt = 14,
    trainWis = 15,
};

const std::string DoingTaskName[] {
    "nothing",
    "meditating",
    "situps",
    "pushups",
    "crafting",
    "RES",
    "RES",
    "RES",
    "RES",
    "RES",
    "str training",
    "agl training",
    "con training",
    "spd training",
    "int training",
    "wis training",
};

struct card {
    std::string name = "Default";
    std::function<bool(struct Character *ch)> effect;
    std::string playerAnnounce = "You focus hard on your work.\r\n";
    std::string roomAnnounce = "$n focuses hard on $s work.\r\n";
    bool discard = false;

};

struct deck {
    std::vector<struct card> deck;
    std::vector<struct card> discard;

    void shuffleDeck();
    void discardCard(std::string);
    void discardCard(card card);
    bool playTopCard(Character* ch);
    card findCard(std::string);
    void addCardToDeck(std::string, int num = 1);
    void removeCard(std::string);
    void addCardToDeck(card, int num = 1);
    void removeCard(card);
    void initDeck(Character* ch);
};

struct craftTask {
    struct Object *pObject = nullptr;
    int improvementRounds = 0;
};

template<typename T>
bool MOB_FLAGGED(T* ch, int flag) {
    return ch->mob_flags.get(flag);
};

extern bool AFF_FLAGGED(Character *ch, int flag);
extern bool AFF_FLAGGED(CharacterPrototype *ch, int flag);

extern bool PLR_FLAGGED(Character *ch, int flag);
extern bool PRF_FLAGGED(Character *ch, int flag);
extern bool ADM_FLAGGED(Character *ch, int flag);

struct CharacterBase {
    Race race{Race::human};
    std::optional<SubRace> subrace{};
    Sensei sensei{Sensei::commoner};
    Sex sex{Sex::male};
    struct mob_special_data mob_specials{};
    Size size{Size::medium};
    FlagHandler<CharacterFlag> character_flags{};
    FlagHandler<MobFlag> mob_flags{};
    FlagHandler<Race> bio_genomes{};
    FlagHandler<Mutation> mutations{};
};