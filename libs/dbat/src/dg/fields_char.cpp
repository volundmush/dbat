/*
 * DG Scripts: the fields of a character, as in %actor.name% or %actor.gold(10)%. See fields.hpp.
 */
#include <algorithm>
#include <cstdlib>

#include "dbat/io/reader.hpp"
#include "dbat/utils.hpp"
#include "dbat/iterate.hpp"
#include "dbat/args.hpp"
#include "dbat/comm.hpp"
#include "dbat/db.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/flags.hpp"
#include "dbat/handler.hpp"
#include "dbat/spells.hpp"
#include "dbat/class.hpp"
#include "dbat/races.hpp"
#include "dbat/local_limits.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

#include "fields.hpp"

namespace dg {
namespace {

using result = std::optional<std::string>;

std::string number(long long n) { return str_format("{:d}", n); }
std::string yes_no(bool b) { return b ? "1" : "0"; }

/* One of the six abilities, raised or lowered by the argument and kept between 3 and 100 */
result ability(const field_query &q, char_data *c, int8_t (*get)(const char_data *), void (*set)(char_data *, int8_t))
{
  if (q.has_arg()) {
    set(c, get(c) + parse_int(q.subfield));
    if (get(c) > 100)
      set(c, 100);
    if (get(c) < 3)
      set(c, 3);
  }
  return number(get(c));
}

/* Hunger, thirst or drunkenness, which the argument sets, between -1 and 24 */
result condition(const field_query &q, char_data *c, int which)
{
  if (q.has_arg())
    char_set_conditions(q.g, c, which, std::max(-1, std::min(parse_int(q.subfield), 24)));
  return number(char_get_conditions(q.g, c, which));
}

/* Whether c has the flag the argument names, of the flags names lists; "0" without an argument */
result named_flag(const field_query &q, std::span<const std::string_view> names, auto &&flagged)
{
  if (!q.has_arg())
    return "0";
  int flag = get_flag_by_name(names, q.subfield);
  return yes_no(flag != NOFLAG && flagged(flag));
}

/* The same for a typed family, flagged() taking its flag */
template <flag_family E> result named_flag(const field_query &q, auto &&flagged)
{
  if (!q.has_arg())
    return "0";
  std::optional<E> flag = flag_by_name<E>(q.subfield);
  return yes_no(flag && flagged(*flag));
}

/* A player flag, which an argument of "on" or "off" sets */
result player_flag(const field_query &q, char_data *c, int flag)
{
  if (str_eq_nocase(q.subfield, "on"))
    char_plr_set(c, flag, true);
  else if (str_eq_nocase(q.subfield, "off"))
    char_plr_set(c, flag, false);
  return yes_no(char_plr_flagged(c, flag));
}

/* Raises or lowers health, ki or stamina by the argument */
void change_points(const field_query &q, char_data *c, int64_t (*inc)(GameWorld *, char_data *, int64_t, bool),
              int64_t (*dec)(GameWorld *, char_data *, int64_t, int64_t))
{
  if (q.has_arg()) {
    int64_t addition = io::c_atoll(q.subfield);
    if (addition > 0)
      inc(q.g, c, addition, true);
    else
      dec(q.g, c, addition, 0);
  }
}

/* The UID of an entity that may not be there, or "" */
std::string uid_or_none(const char_data *c) { return c ? uid_text(char_get_id(c)) : ""; }

/* What a skill is at, by name */
std::string skill_percent(GameWorld *g, char_data *ch, std::string_view skill)
{
  int skillnum = find_skill_num(g, skill, SKTYPE_SKILL);
  if (skillnum <= 0)
    return "unknown skill";
  return number(char_get_skill(ch, skillnum));
}

/* Whether ch has item, worn or carried, by UID, vnum or name, looking in containers too */
bool char_has_item(GameWorld *g, std::string_view item, char_data *ch)
{
  if (get_object_in_equip(g, ch, item) != nullptr)
    return true;
  return item_in_list(g, item, char_get_inventory(ch)) != 0;
}

/* The fields, by name */
const field<char_data> char_fields[] = {
    {"aaaaa", [](const field_query &, char_data *) -> result { return "0"; }},
    {"affect", [](const field_query &q, char_data *c) {
       return named_flag<aff_flag>(q, [c](aff_flag f) { return char_aff_flagged(c, f); });
     }},
    {"alias", [](const field_query &, char_data *c) -> result { return str_format("{}", char_get_pc_name(c)); }},
    {"align", [](const field_query &q, char_data *c) -> result {
       if (q.has_arg())
         char_set_alignment(c, std::max(-1000, std::min(parse_int(q.subfield), 1000)));
       return number(char_get_alignment(c));
     }},
    {"bank", [](const field_query &q, char_data *c) -> result {
       if (q.has_arg())
         char_set_bank_gold(c, char_get_bank_gold(c) + parse_int(q.subfield));
       return number(char_get_gold(c)); /* quirk: what's on hand, not in the bank */
     }},
    {"canbeseen", [](const field_query &q, char_data *c) -> result {
       char_data *const *self = std::get_if<char_data *>(&q.go);
       return yes_no(!(q.type == MOB_TRIGGER && self && !char_can_see(q.g, *self, c)));
     }},
    {"carry", [](const field_query &q, char_data *c) -> result { return yes_no(!char_is_npc(c) && char_get_carrying(q.g, c)); }},
    {"cha", [](const field_query &q, char_data *c) { return ability(q, c, char_get_aff_abils_cha, char_set_aff_abils_cha); }},
    {"clan", [](const field_query &, char_data *) -> result { return "0"; }},
    {"class", [](const field_query &, char_data *c) -> result {
       return char_is_npc(c) ? std::string("blank") : str_format("{}", char_sensei_name(c));
     }},
    {"con", [](const field_query &q, char_data *c) { return ability(q, c, char_get_aff_abils_con, char_set_aff_abils_con); }},
    {"dead", [](const field_query &, char_data *c) -> result { return yes_no(char_aff_flagged(c, AFF_SPIRIT)); }},
    {"death", [](const field_query &, char_data *c) -> result { return number((long long)char_get_deathtime(c)); }},
    {"dex", [](const field_query &q, char_data *c) { return ability(q, c, char_get_aff_abils_dex, char_set_aff_abils_dex); }},
    {"drag", [](const field_query &q, char_data *c) -> result { return yes_no(!char_is_npc(c) && char_get_drag(q.g, c)); }},
    {"drunk", [](const field_query &q, char_data *c) { return condition(q, c, DRUNK); }},
    {"eq", [](const field_query &q, char_data *c) -> result {
       if (!q.has_arg())
         return "";
       if (q.subfield[0] == '*') { /* whether anything is worn */
         for (int i = 0; i < NUM_WEARS; i++)
           if (char_get_equipment(c, i))
             return "1";
         return "";
       }
       int pos = find_eq_pos_script(q.subfield);
       if (pos < 0 || !char_get_equipment(c, pos))
         return "";
       return uid_text(obj_get_id(char_get_equipment(c, pos)));
     }},
    {"exp", [](const field_query &q, char_data *c) -> result {
       if (q.has_arg())
         gain_exp(q.g, c, std::min<int64_t>(io::c_atoll(q.subfield), 2100000000));
       return number(char_get_exp(c));
     }},
    {"fighting", [](const field_query &q, char_data *c) -> result { return uid_or_none(char_get_fighting(q.g, c)); }},
    {"flying", [](const field_query &, char_data *c) -> result { return yes_no(char_aff_flagged(c, AFF_FLYING)); }},
    {"follower", [](const field_query &, char_data *c) -> result {
       return char_get_followers(c) ? uid_text(char_get_followers(c).newest()) : "";
     }},
    {"global", [](const field_query &q, char_data *c) -> result { /* a global variable of a mob, by name */
       if (!char_is_npc(c) || !char_get_script(c))
         return "";
       /* quirk: the owner's command names, as %send%, are a mob's whatever the owner is */
       return plain_variable(q.g, q.go, char_get_script(c), nullptr, MOB_TRIGGER, q.subfield);
     }},
    {"gold", [](const field_query &q, char_data *c) -> result {
       if (q.has_arg())
         char_set_gold(c, char_get_gold(c) + parse_int(q.subfield));
       return number(char_get_gold(c));
     }},
    {"has_item", [](const field_query &q, char_data *c) -> result {
       return q.has_arg() ? number(char_has_item(q.g, q.subfield, c)) : "";
     }},
    {"heshe", [](const field_query &, char_data *c) -> result { return str_format("{}", char_hssh(c)); }},
    {"himher", [](const field_query &, char_data *c) -> result { return str_format("{}", char_hmhr(c)); }},
    {"hisher", [](const field_query &, char_data *c) -> result { return str_format("{}", char_hshr(c)); }},
    {"hitp", [](const field_query &q, char_data *c) -> result {
       change_points(q, c, incCurHealth, decCurHealth);
       if (q.has_arg())
         update_pos(q.g, c);
       return number(getCurPL(q.g, c));
     }},
    {"hunger", [](const field_query &q, char_data *c) { return condition(q, c, HUNGER); }},
    {"id", [](const field_query &, char_data *c) -> result { return number(char_get_id(c)); }},
    {"int", [](const field_query &q, char_data *c) { return ability(q, c, char_get_aff_abils_intel, char_set_aff_abils_intel); }},
    {"inventory", [](const field_query &q, char_data *c) -> result {
       if (!q.has_arg()) /* the newest thing carried */
         return char_get_inventory(c) ? uid_text(char_get_inventory(c).newest()) : "";
       for (obj_data *obj : char_inventory(q.g, c))
         if (obj_get_vnum(q.g, obj) == parse_int(q.subfield))
           return uid_text(obj_get_id(obj));
       return "";
     }},
    {"is_killer", [](const field_query &q, char_data *c) { return player_flag(q, c, PLR_KILLER); }},
    {"is_pc", [](const field_query &, char_data *c) -> result { return yes_no(!char_is_npc(c)); }},
    {"is_thief", [](const field_query &q, char_data *c) { return player_flag(q, c, PLR_THIEF); }},
    {"level", [](const field_query &, char_data *c) -> result { return number(char_get_level(c)); }},
    {"mana", [](const field_query &q, char_data *c) -> result {
       change_points(q, c, incCurKI, decCurKI);
       return number(getCurKI(q.g, c));
     }},
    {"master", [](const field_query &q, char_data *c) -> result { return uid_or_none(char_get_master(q.g, c)); }},
    /* the maxima can't be changed by a script: an argument does nothing */
    {"maxhitp", [](const field_query &q, char_data *c) -> result { return number(getEffMaxPL(q.g, c)); }},
    {"maxmana", [](const field_query &q, char_data *c) -> result { return number(getMaxKI(q.g, c)); }},
    {"maxmove", [](const field_query &q, char_data *c) -> result { return number(getMaxST(q.g, c)); }},
    {"move", [](const field_query &q, char_data *c) -> result {
       change_points(q, c, incCurST, decCurST);
       return number(getCurST(q.g, c));
     }},
    {"name", [](const field_query &, char_data *c) -> result { return str_format("{}", char_get_name(c)); }},
    {"next_in_room", [](const field_query &q, char_data *c) -> result { return uid_or_none(char_next_in_room(q.g, c)); }},
    {"plr", [](const field_query &q, char_data *c) {
       return named_flag(q, player_bits, [c](int f) { return char_plr_flagged(c, f); });
     }},
    {"pos", [](const field_query &q, char_data *c) -> result {
       if (q.has_arg()) /* sleeping, resting, sitting, fighting or standing, or the start of one */
         for (int i = POS_SLEEPING; i <= POS_STANDING; i++)
           if (!compare_nocase_n(q.subfield, position_types[i], q.subfield.size())) {
             char_set_position(c, i);
             break;
           }
       return str_format("{}", position_types[char_get_position(c)]);
     }},
    {"prac", [](const field_query &q, char_data *c) -> result {
       if (char_is_npc(c) && char_get_in_room(c) != NOWHERE)
         send_to_room(q.g, char_get_in_room(c), "Error!: Report this trigger error to the coding authorities!\r\n");
       if (q.has_arg()) {
         int points = char_get_practice_points(q.g, c) + parse_int(q.subfield);
         if (points >= 0) /* quirk: read again, as the MAX macro did, which an NPC logs a second time */
           points = char_get_practice_points(q.g, c) + parse_int(q.subfield);
         char_set_practice_points(q.g, c, std::max(0, points));
       }
       return number(char_get_practice_points(q.g, c));
     }},
    {"pref", [](const field_query &q, char_data *c) {
       return named_flag<pref_flag>(q, [&q, c](pref_flag f) { return char_prf_flagged(q.g, c, f); });
     }},
    {"race", [](const field_query &, char_data *c) -> result { return str_format("{}", char_true_race_name(c)); }},
    {"room", [](const field_query &, char_data *c) -> result { /* in NOWHERE, the void; see dg_scripts.hpp */
       return uid_text(char_get_in_room(c) != NOWHERE ? char_get_in_room(c) + ROOM_ID_BASE : ROOM_ID_BASE);
     }},
    {"rpp", [](const field_query &q, char_data *c) -> result {
       if (q.has_arg())
         char_set_rp(c, char_get_rp(c) + parse_int(q.subfield));
       return number(char_get_rp(c));
     }},
    {"sex", [](const field_query &, char_data *c) -> result { return str_format("{}", genders[(int)char_get_sex(c)]); }},
    {"size", [](const field_query &q, char_data *c) -> result {
       if (q.has_arg()) {
         int ns = search_block(q.subfield, size_names, false);
         if (ns > -1)
           char_set_size(c, ns);
       }
       return sprinttype(get_size(c), size_names);
     }},
    {"skill", [](const field_query &q, char_data *c) -> result { return skill_percent(q.g, c, q.subfield); }},
    {"skillset", [](const field_query &q, char_data *c) -> result { /* skillset("<skill name>" <amount>) */
       if (!char_is_npc(c) && q.has_arg()) {
         std::string_view rest = q.subfield;
         std::string skillname = take_quoted(rest);
         rest                  = skip_spaces(rest);
         if (!rest.empty() && is_number(rest)) {
           int skillnum = find_skill_num(q.g, skillname, SKTYPE_SKILL);
           if (skillnum > 0)
             char_set_skills_base(c, skillnum, std::max(0, std::min(100, parse_int(rest))));
         }
       }
       return ""; /* a field, which gives nothing */
     }},
    {"str", [](const field_query &q, char_data *c) { return ability(q, c, char_get_aff_abils_str, char_set_aff_abils_str); }},
    {"thirst", [](const field_query &q, char_data *c) { return condition(q, c, THIRST); }},
    {"tnl", [](const field_query &, char_data *c) -> result { return number((long long)level_exp(c, char_get_level(c) + 1)); }},
    {"varexists", [](const field_query &q, char_data *c) -> result {
       return yes_no(char_get_script(c) && char_get_script(c)->global_vars.find(q.subfield));
     }},
    {"vnum", [](const field_query &q, char_data *c) -> result {
       if (q.has_arg())
         return number(char_is_npc(c) ? (int)(char_get_mob_vnum(q.g, c) == parse_int(q.subfield)) : -1);
       /* -1 for a player, which is deprecated: %actor.is_pc% says so --Welcor 09/03 */
       return char_is_npc(c) ? number(char_get_mob_vnum(q.g, c)) : "-1";
     }},
    {"weight", [](const field_query &, char_data *c) -> result { return number(char_get_weight(c)); }},
    {"wis", [](const field_query &q, char_data *c) { return ability(q, c, char_get_aff_abils_wis, char_set_aff_abils_wis); }},
    {"zenni", [](const field_query &q, char_data *c) -> result {
       if (q.has_arg())
         char_set_gold(c, char_get_gold(c) + parse_int(q.subfield));
       return number(char_get_gold(c));
     }},
};

} // namespace

std::string char_field(const field_query &q, char_data *c)
{
  if (const field<char_data> *f = find_field<char_data>(char_fields, q.field))
    if (result value = f->get(q, c))
      return *value;

  /* not a field: one of c's global variables */
  if (char_get_script(c))
    if (const variable *vd = char_get_script(c)->global_vars.find(q.field))
      return vd->value;
  script_log(q.g, "Trigger: {}, VNum {:d}. unknown char field: '{}'", q.trig->name, trig_get_vnum(q.g, q.trig), q.field);
  return "";
}

} // namespace dg
