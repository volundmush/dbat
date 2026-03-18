#pragma once
#include <map>
#include <memory>
#include <nlohmann/json_fwd.hpp>

#include "HasOrganizationInfo.hpp"

#include "const/Skill.hpp"
#include "const/Max.hpp"

#include "Flags.hpp"

struct guild_info_type
{
    int pc_class;
    room_vnum guild_room;
    int direction;
};

struct Guild : public org_data
{
    void toggle_skill(uint16_t skill_id);
    void toggle_feat(uint16_t skill_id);
    FlagHandler<Skill> skills;         /* array to keep track of which feats things we'll train */
    float charge{1.0};                 /* charge * skill level = how much we'll charge */
    std::string no_such_skill{};       /* message when we don't teach that skill */
    std::string not_enough_gold{};     /* message when the student doesn't have enough gold */
    int minlvl{0};                     /* Minumum level guildmaster will train */
    int open{0}, close{28};            /* when we will train */
    std::unordered_set<uint8_t> feats; /* array to keep track of which feats things we'll train */
};

constexpr const char* MSG_TRAINER_NOT_OPEN = "I'm busy! Come back later!";
constexpr const char* MSG_TRAINER_NO_SEE_CH = "I don't train someone I can't see!";
constexpr const char* MSG_TRAINER_DISLIKE_ALIGN = "Get out of here before I get angry, you are not aligned with me!";
constexpr const char* MSG_TRAINER_DISLIKE_CLASS = "I won't train those of your discipline!";
constexpr const char* MSG_TRAINER_DISLIKE_RACE = "Get out of here, I don't help your kind!";
constexpr const char* MSG_TRAINER_MINLVL = "You are not of a skilled enough level to recieve my training.";

#define GM_NUM(i) guild_index.at(i)->vnum
#define GM_TRAINER(i) guild_index.at(i)->keeper
#define GM_OPEN(i) guild_index.at(i)->open
#define GM_CLOSE(i) guild_index.at(i)->close
#define GM_CHARGE(i) guild_index.at(i)->charge
#define GM_MINLVL(i) guild_index.at(i)->minlvl
#define GM_WITH_WHO(i) guild_index.at(i)->with_who
#define GM_NO_SKILL(i) guild_index.at(i)->no_such_skill
#define GM_NO_GOLD(i) guild_index.at(i)->not_enough_gold
#define GM_COST(i, j, k) (int)(GM_CHARGE(i) * spell_info[j].min_level[(int)GET_CLASS(k)])
#define GM_FUNC(i) guild_index.at(i)->func

extern guild_vnum top_guild;
extern int spell_sort_info[SKILL_TABLE_SIZE + 1];

/* Functions defined in guild.c */
extern int print_skills_by_type(Character *ch, char *buf, int maxsz, int sktype, char *argument);

extern void levelup_parse(struct descriptor_data *d, char *arg);

extern int rpp_to_level(Character *ch);

extern int slot_count(Character *ch);

extern void show_guild(Character *ch, char *arg);

extern void handle_ingest_learn(Character *ch, Character *vict);

extern void list_skills(Character *ch, char *arg);

extern void assign_the_guilds();

/*. External . */
extern SPECIAL(guild);

extern std::map<guild_vnum, std::shared_ptr<Guild>> guild_index;

void to_json(nlohmann::json& j, const Guild& g);
void from_json(const nlohmann::json& j, Guild& g);
