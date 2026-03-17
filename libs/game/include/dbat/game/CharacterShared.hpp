#pragma once
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <nlohmann/json_fwd.hpp>

#include "HasMudStrings.hpp"
#include "HasExtraDescriptions.hpp"
#include "HasMisc.hpp"

#include "const/Appearance.hpp"
#include "const/Position.hpp"

#include "const/Race.hpp"
#include "const/Sensei.hpp"
#include "const/Sex.hpp"
#include "const/CharacterFlag.hpp"
#include "const/MobFlag.hpp"
#include "const/AdminFlag.hpp"
#include "const/AffectFlag.hpp"
#include "const/Mutation.hpp"
#include "const/Size.hpp"
#include "const/Position.hpp"
#include "const/PlayerFlag.hpp"
#include "Flags.hpp"

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

void to_json(nlohmann::json& j, const time_data& unit);
void from_json(const nlohmann::json& j, time_data& unit);
void to_json(nlohmann::json& j, const alias_data& unit);
void from_json(const nlohmann::json& j, alias_data& unit);
void to_json(nlohmann::json& j, const skill_data& unit);
void from_json(const nlohmann::json& j, skill_data& unit);

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
extern bool AFF_FLAGGED(Character *ch, AffectFlag flag);
extern bool AFF_FLAGGED(Character *ch, int flag);
extern bool AFF_FLAGGED(CharacterPrototype *ch, int flag);

extern bool PLR_FLAGGED(Character *ch, PlayerFlag flag);
extern bool PRF_FLAGGED(Character *ch, int flag);
extern bool ADM_FLAGGED(Character *ch, AdminFlag flag);

struct CharacterBase : public HasVnum, public HasMudStrings, public HasExtraDescriptions, public HasStats {
    Race race{Race::human};
    std::optional<AndroidModel> model{};
    Sensei sensei{Sensei::commoner};
    Sex sex{Sex::male};
    Size size{Size::medium};
    Position position{Position::Standing};
    FlagHandler<CharacterFlag> character_flags{};
    FlagHandler<MobFlag> mob_flags{};
    FlagHandler<Race> bio_genomes{};
    FlagHandler<Mutation> mutations{};
    FlagHandler<AffectFlag> affect_flags{};
};

inline std::string format_as(const CharacterBase& cb) {
    std::vector<std::string> parts;
    parts.emplace_back(format_as(static_cast<const HasVnum&>(cb)));
    parts.emplace_back(format_as(static_cast<const HasMudStrings&>(cb)));
    parts.emplace_back(format_as(static_cast<const HasExtraDescriptions&>(cb)));
    parts.emplace_back(fmt::format("Race: {}", enchantum::to_string(cb.race)));
    if(cb.model) parts.emplace_back(fmt::format("Model: {}", enchantum::to_string(cb.model.value())));
    parts.emplace_back(fmt::format("Sensei: {}", enchantum::to_string(cb.sensei)));
    parts.emplace_back(fmt::format("Sex: {}", enchantum::to_string(cb.sex)));
    parts.emplace_back(fmt::format("Size: {}", enchantum::to_string(cb.size)));
    parts.emplace_back(fmt::format("Position: {}", enchantum::to_string(cb.position)));
    if(cb.character_flags) parts.emplace_back(fmt::format("Character Flags: {}", format_as(cb.character_flags)));
    if(cb.mob_flags) parts.emplace_back(fmt::format("Mob Flags: {}", format_as(cb.mob_flags)));
    if(cb.bio_genomes) parts.emplace_back(fmt::format("BioGenomes: {}", format_as(cb.bio_genomes)));
    if(cb.mutations) parts.emplace_back(fmt::format("Mutations: {}", format_as(cb.mutations)));
    if(cb.affect_flags) parts.emplace_back(fmt::format("Affect Flags: {}", format_as(cb.affect_flags)));
    return fmt::format("Base Character Data:\r\n{}", fmt::join(parts, "\r\n"));
}
