#pragma once
#include <cstdint>
#include <string>

#include "dbat/consts/types.hpp"

/*
 * A player's order at the equipment menus (CON_POBJ): custom equipment built for RPP, or one of their things
 * restrung. It lives on the connection, and isn't an OLC session: the player isn't building, and isn't shown or
 * logged as using OLC.
 */
struct pobj_order {
  enum class kind { none, custom, restring };
  enum class step { none, main, name, short_desc, long_desc, type, weapon, quit };

  kind what{};
  step asking{};
  std::string name;       /* keywords */
  std::string short_desc;
  std::string long_desc;
  std::string was;        /* a restrung thing's short description before */
  int type{};             /* custom: the equipment slot, 1 to 15, or 16 for a weapon */
  int weapon{};           /* custom: the kind of weapon, 1 to 6, or 0 for none */
  int32_t restrung_id{};  /* the thing being restrung, by id */
};

/* Starts the custom equipment menu, on a generic armour vest */
void start_custom_order(GameWorld *g, struct descriptor_data *d);

/* Starts the restring menu for obj */
void start_restring(GameWorld *g, struct descriptor_data *d, struct obj_data *obj);

/* nanny()'s handler for CON_POBJ */
void pobj_edit_parse(GameWorld *g, struct descriptor_data *d, char *arg);
