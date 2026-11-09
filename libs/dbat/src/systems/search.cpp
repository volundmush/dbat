#include <cassert>
#include <cstdlib>

#include "dbat/args.hpp"

#include "dbat/iterate.hpp"
#include "dbat/utils.hpp"
#include "dbat/act.informative.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

static unsigned char char_at(std::string_view s, size_t i) { return i < s.size() ? s[i] : '\0'; }

static bool contains(std::string_view text, std::string_view part) { return text.find(part) != std::string_view::npos; }

bool is_name(std::string_view str, std::string_view namelist)
{
  if (str.empty() || namelist.empty())
    return false;

  size_t n = 0; /* where in namelist */
  for (;;) {
    for (size_t s = 0;; s++, n++) {
      if (!char_at(str, s) && !isalpha(char_at(namelist, n)))
        return true;

      if (!char_at(namelist, n))
        return false;

      if (!char_at(str, s) || char_at(namelist, n) == ' ')
        break;

      if (tolower(char_at(str, s)) != tolower(char_at(namelist, n)))
        break;
    }

    /* skip to next name */
    while (isalpha(char_at(namelist, n)))
      n++;
    if (!char_at(namelist, n))
      return false;
    n++; /* first char of new name */
  }
}

bool is_name(const char *str, const char *namelist) { return is_name(std::string_view(str ? str : ""), namelist ? namelist : ""); }

/* allow abbreviations */
bool isname(std::string_view str, std::string_view namelist)
{
  if (str.empty() || namelist.empty())
    return false;

  if (str_eq_nocase(str, namelist)) /* the easy way */
    return true;

  for (size_t start = 0;;) {
    size_t end         = std::min(namelist.find_first_of(" \t", start), namelist.size());
    std::string_view w = namelist.substr(start, end - start);
    if (is_abbrev(str, w)) {
      /* Don't allow abbreviated numbers, only alpha names need abbreviation */
      /* This, I just consider a bug fix, because abbreviating numbers is just*/
      /* asking for trouble. IE: 100 would return true on 1000 --Sryth*/
      return !(isdigit((unsigned char)str[0]) && parse_int(str) != parse_int(w));
    }
    if (end == namelist.size())
      return false;
    start = end + 1;
  }
}

bool isname(std::string_view str, const char *namelist) { return isname(str, std::string_view(namelist ? namelist : "")); }

bool isname(const char *str, const char *namelist) { return isname(std::string_view(str ? str : ""), namelist); }

int get_number(std::string &name)
{
  size_t dot = name.find('.');
  if (dot == std::string::npos)
    return 1;

  std::string number = name.substr(0, dot);
  name.erase(0, dot + 1);
  if (!is_number(number))
    return 0;
  return parse_int(number);
}

/*
 * Runs find on name as a std::string, then writes back what it made of it, as the char * forms have always
 * changed the caller's name in place. It is never longer: an "N." gone, a letter capitalised.
 */
template <class F>
static auto in_place(char *name, F find)
{
  std::string text = name;
  auto found       = find(text);
  if (text != name) {
    assert(text.size() <= strlen(name));
    memcpy(name, text.c_str(), text.size() + 1);
  }
  return found;
}

int get_number(char **name)
{
  return in_place(*name, [](std::string &s) { return get_number(s); });
}

/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *get_obj_in_list_num(GameWorld *g, obj_vnum num, const id_list &list)
{

  for (obj_data *i : obj_list(g, list))
    if (obj_get_item_number(i) == num)
      return (i);

  return (nullptr);
}

/* search the entire world for an object number, and return a pointer  */
struct obj_data *get_obj_num(GameWorld *g, obj_vnum nr)
{

  for (obj_data *i : each_obj(g))
    if (obj_get_item_number(i) == nr)
      return (i);

  return (nullptr);
}

/* search a room for a char, and return a pointer if found..  */
struct char_data *get_char_room(GameWorld *g, std::string &name, int *number, room_vnum room)
{
  int num;

  if (!number) {
    number = &num;
    num    = get_number(name);
  }

  if (*number == 0)
    return (nullptr);

  for (char_data *i : room_people(g, room)) {
    if (!(*number))
      break;
    if (isname(name, char_get_alias(i)))
      if (--(*number) == 0)
        return (i);
  }
  return (nullptr);
}

/* search all over the world for a char num, and return a pointer if found */
struct char_data *get_char_num(GameWorld *g, mob_vnum nr)
{

  for (char_data *i : each_char(g))
    if (char_get_nr(i) == nr)
      return (i);

  return (nullptr);
}

/* Whether name is, or is part of, what ch knows i as: by race, by introduction, or (to or for an immortal or
 * NPC) by name */
static bool goes_by(struct char_data *ch, struct char_data *i, std::string_view name)
{
  auto named = [&](std::string_view known) { return str_eq_nocase(known, name) || contains(known, name); };

  if (char_get_admlevel(ch) < 1 && char_get_admlevel(i) < 1 && !char_is_npc(ch) && !char_is_npc(i)) {
    if (!named(juggleRaceName(i, true))) {
      if (readIntro(ch, i) == 1) {
        if (!named(get_i_name(ch, i)))
          return false;
      } else {
        return false;
      }
    }
  }
  if ((char_get_admlevel(ch) >= 1 || char_get_admlevel(i) >= 1 || char_is_npc(ch) || char_is_npc(i))) {
    const char *alias = char_get_alias(i);
    if (!named(alias ? alias : "")) {
      if (!named(juggleRaceName(i, true))) {
        if (!char_is_npc(ch) && !char_is_npc(i) && readIntro(ch, i) == 1) {
          if (!named(get_i_name(ch, i)))
            return false;
        } else {
          return false;
        }
      }
    }
  }
  return true;
}

struct char_data *get_player_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, int inroom)
{
  int num;

  if (!number) {
    number = &num;
    num    = get_number(name);
  }

  for (char_data *i : each_char(g)) {
    if (char_is_npc(i))
      continue;
    if (inroom == FIND_CHAR_ROOM && char_get_in_room(i) != char_get_in_room(ch))
      continue;
    if (!goes_by(ch, i, name))
      continue;
    if (!char_can_see(g, ch, i))
      continue;
    if (--(*number) != 0)
      continue;
    return (i);
  }

  return (nullptr);
}

struct char_data *get_char_room_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number)
{
  int num;

  if (!number) {
    number = &num;
    num    = get_number(name);
  }

  /* JE 7/18/94 :-) :-) */
  if (str_eq_nocase(name, "self") || str_eq_nocase(name, "me"))
    return (ch);

  /* 0.<name> means PC with name */
  if (*number == 0)
    return (get_player_vis(g, ch, name, nullptr, FIND_CHAR_ROOM));

  /* CAP(name) capitalises the caller's name, as the scan reaches it; the comparisons after it see the capital */
  for (char_data *i : room_people(g, char_get_in_room(ch))) {
    if (!(*number))
      break;
    if (str_eq_nocase(name, "last") && char_get_lasthit(i) != 0 && char_get_lasthit(i) == char_get_idnum(ch)) {
      if (char_can_see(g, ch, i))
        if (--(*number) == 0)
          return (i);
    } else if (isname(name, char_get_alias(i)) && (char_is_npc(i) || char_is_npc(ch) || char_get_admlevel(i) > 0 || char_get_admlevel(ch) > 0) && i != ch) {
      if (char_can_see(g, ch, i))
        if (--(*number) == 0)
          return (i);
    } else if (isname(name, char_get_alias(i)) && i == ch) {
      if (char_can_see(g, ch, i))
        if (--(*number) == 0)
          return (i);
    } else if (!char_is_npc(i) && !char_is_npc(ch) && str_eq_nocase(get_i_name(ch, i), CAP(name)) && i != ch) {
      if (char_can_see(g, ch, i))
        if (--(*number) == 0)
          return (i);
    } else if (!char_is_npc(i) && !char_is_npc(ch) && contains(get_i_name(ch, i), CAP(name)) && i != ch) {
      if (char_can_see(g, ch, i))
        if (--(*number) == 0)
          return (i);
    } else if (!char_is_npc(i) && std::string_view(juggleRaceName(i, true)) == CAP(name) && i != ch) {
      if (char_can_see(g, ch, i))
        if (--(*number) == 0)
          return (i);
    } else if (!char_is_npc(i) && contains(juggleRaceName(i, true), CAP(name)) && i != ch) {
      if (char_can_see(g, ch, i))
        if (--(*number) == 0)
          return (i);
    } else if (!char_is_npc(i) && std::string_view(juggleRaceName(i, true)) == name && i != ch) {
      if (char_can_see(g, ch, i))
        if (--(*number) == 0)
          return (i);
    } else if (!char_is_npc(i) && contains(juggleRaceName(i, true), name) && i != ch) {
      if (char_can_see(g, ch, i))
        if (--(*number) == 0)
          return (i);
    }
  }
  return (nullptr);
}

struct char_data *get_char_world_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number)
{
  struct char_data *i;
  int num;

  if (!number) {
    number = &num;
    num    = get_number(name);
  }

  if ((i = get_char_room_vis(g, ch, name, number)) != nullptr)
    return (i);

  if (*number == 0)
    return get_player_vis(g, ch, name, nullptr, 0);

  for (char_data *tch : each_char(g)) {
    if (!(*number))
      break;
    if (char_get_in_room(ch) == char_get_in_room(tch))
      continue;
    if (!goes_by(ch, tch, name))
      continue;
    /*if (!CAN_SEE(ch, i))
      continue;*/
    if (--(*number) != 0)
      continue;

    return (tch);
  }
  return (nullptr);
}

struct char_data *get_char_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, int where)
{
  if (where == FIND_CHAR_ROOM)
    return get_char_room_vis(g, ch, name, number);
  else if (where == FIND_CHAR_WORLD)
    return get_char_world_vis(g, ch, name, number);
  else
    return (nullptr);
}

bool obj_named_and_visible(GameWorld *g, struct char_data *ch, std::string_view name, struct obj_data *obj)
{
  return isname(name, obj_get_name(obj)) && (char_can_see_obj(g, ch, obj) || obj_get_type_flag(obj) == ITEM_LIGHT);
}

struct obj_data *get_obj_in_list_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, const id_list &list)
{
  int num;

  if (!number) {
    number = &num;
    num    = get_number(name);
  }

  if (*number == 0)
    return (nullptr);

  for (obj_data *i : obj_list(g, list)) {
    if (!(*number))
      break;
    if (obj_named_and_visible(g, ch, name, i))
      if (--(*number) == 0)
        return (i);
  }
  return (nullptr);
}

/* search the entire world for an object, and return a pointer  */
struct obj_data *get_obj_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number)
{
  struct obj_data *i;
  int num;

  if (!number) {
    number = &num;
    num    = get_number(name);
  }

  if (*number == 0)
    return (nullptr);

  /* scan items carried */
  if ((i = get_obj_in_list_vis(g, ch, name, number, char_get_inventory(ch))) != nullptr)
    return (i);

  /* scan room */
  if ((i = get_obj_in_list_vis(g, ch, name, number, room_get_contents(g, char_get_in_room(ch)))) != nullptr)
    return (i);

  /* ok.. no luck yet. scan the entire obj list   */
  for (obj_data *item : each_obj(g)) {
    if (!(*number))
      break;
    if (isname(name, obj_get_name(item)))
      if (char_can_see_obj(g, ch, item))
        if (--(*number) == 0)
          return (item);
  }
  return (nullptr);
}

struct obj_data *get_obj_in_equip_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, struct obj_data *equipment[])
{
  int pos = get_obj_pos_in_equip_vis(g, ch, name, number, equipment);
  return pos < 0 ? nullptr : equipment[pos];
}

int get_obj_pos_in_equip_vis(GameWorld *g, struct char_data *ch, std::string &name, int *number, struct obj_data *equipment[])
{
  int j, num;

  if (!number) {
    number = &num;
    num    = get_number(name);
  }

  if (*number == 0)
    return (-1);

  for (j = 0; j < NUM_WEARS; j++)
    if (equipment[j] && char_can_see_obj(g, ch, equipment[j]) && isname(name, obj_get_name(equipment[j])))
      if (--(*number) == 0)
        return (j);

  return (-1);
}

struct char_data *get_char_room(GameWorld *g, char *name, int *number, room_vnum room)
{
  return in_place(name, [&](std::string &s) { return get_char_room(g, s, number, room); });
}

struct char_data *get_player_vis(GameWorld *g, struct char_data *ch, char *name, int *number, int inroom)
{
  return in_place(name, [&](std::string &s) { return get_player_vis(g, ch, s, number, inroom); });
}

struct char_data *get_char_vis(GameWorld *g, struct char_data *ch, char *name, int *number, int where)
{
  return in_place(name, [&](std::string &s) { return get_char_vis(g, ch, s, number, where); });
}

struct char_data *get_char_room_vis(GameWorld *g, struct char_data *ch, char *name, int *number)
{
  return in_place(name, [&](std::string &s) { return get_char_room_vis(g, ch, s, number); });
}

struct char_data *get_char_world_vis(GameWorld *g, struct char_data *ch, char *name, int *number)
{
  return in_place(name, [&](std::string &s) { return get_char_world_vis(g, ch, s, number); });
}

struct obj_data *get_obj_in_list_vis(GameWorld *g, struct char_data *ch, char *name, int *number, const id_list &list)
{
  return in_place(name, [&](std::string &s) { return get_obj_in_list_vis(g, ch, s, number, list); });
}

struct obj_data *get_obj_vis(GameWorld *g, struct char_data *ch, char *name, int *number)
{
  return in_place(name, [&](std::string &s) { return get_obj_vis(g, ch, s, number); });
}

struct obj_data *get_obj_in_equip_vis(GameWorld *g, struct char_data *ch, char *arg, int *number, struct obj_data *equipment[])
{
  return in_place(arg, [&](std::string &s) { return get_obj_in_equip_vis(g, ch, s, number, equipment); });
}

int get_obj_pos_in_equip_vis(GameWorld *g, struct char_data *ch, char *arg, int *number, struct obj_data *equipment[])
{
  return in_place(arg, [&](std::string &s) { return get_obj_pos_in_equip_vis(g, ch, s, number, equipment); });
}

/* a function to scan for "all" or "all.x" */
int find_all_dots(std::string &arg)
{
  if (arg == "all")
    return (FIND_ALL);
  if (arg.starts_with("all.")) {
    arg.erase(0, 4);
    return (FIND_ALLDOT);
  }
  return (FIND_INDIV);
}

int find_all_dots(char *arg)
{
  return in_place(arg, [](std::string &s) { return find_all_dots(s); });
}

/* Generic Find, designed to find any object/character
 *
 * Calling:
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be NULL if no character was found, otherwise points
 * **tar_obj Will be NULL if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in.
 */

int generic_find(GameWorld *g, std::string_view arg, bitvector_t bitvector, struct char_data *ch,
                 struct char_data **tar_ch, struct obj_data **tar_obj)
{
  int i, found, number;

  *tar_ch  = nullptr;
  *tar_obj = nullptr;

  std::string name = take_arg(arg);

  if (name.empty())
    return (0);
  if (!(number = get_number(name)))
    return (0);

  if (IS_SET(bitvector, FIND_CHAR_ROOM)) { /* Find person in room */
    if ((*tar_ch = get_char_room_vis(g, ch, name, &number)) != nullptr)
      return (FIND_CHAR_ROOM);
  }

  if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
    if ((*tar_ch = get_char_world_vis(g, ch, name, &number)) != nullptr)
      return (FIND_CHAR_WORLD);
  }

  if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
    for (found = false, i = 0; i < NUM_WEARS && !found; i++)
      if (char_get_equipment(ch, i) && isname(name, obj_get_name(char_get_equipment(ch, i))) && --number == 0) {
        *tar_obj = char_get_equipment(ch, i);
        found    = true;
      }
    if (found)
      return (FIND_OBJ_EQUIP);
  }

  if (IS_SET(bitvector, FIND_OBJ_INV)) {
    if ((*tar_obj = get_obj_in_list_vis(g, ch, name, &number, char_get_inventory(ch))) != nullptr)
      return (FIND_OBJ_INV);
  }

  if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
    if ((*tar_obj = get_obj_in_list_vis(g, ch, name, &number, room_get_contents(g, char_get_in_room(ch)))) != nullptr)
      return (FIND_OBJ_ROOM);
  }

  if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
    if ((*tar_obj = get_obj_vis(g, ch, name, &number)))
      return (FIND_OBJ_WORLD);
  }

  return (0);
}