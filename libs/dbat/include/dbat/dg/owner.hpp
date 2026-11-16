#pragma once
/*
 * What a DG script runs on: a mob, an object or a room.
 *
 * While a script runs, it has the owner itself, as an owner_ptr. Whatever must outlast the run, such as a script
 * waiting on a 'wait', keeps a script_owner instead: the kind and the DG UID, which find the owner again, or nothing
 * if it has gone, and never a pointer left dangling.
 */
#include <optional>
#include <variant>

#include "dbat/consts/triggers.hpp"
#include "dbat/consts/types.hpp"

/* In the order of MOB_TRIGGER, OBJ_TRIGGER and WLD_TRIGGER, so that index() is the old trigger type. */
using owner_ptr = std::variant<struct char_data *, struct obj_data *, struct room_data *>;
static_assert(std::variant_size_v<owner_ptr> == 3 && MOB_TRIGGER == 0 && OBJ_TRIGGER == 1 && WLD_TRIGGER == 2);

enum class owner_kind { mob = MOB_TRIGGER, obj = OBJ_TRIGGER, room = WLD_TRIGGER };

/* For std::visit: one lambda for each kind. */
template <class... F>
struct overloaded : F... {
  using F::operator()...;
};

/* The trigger type number (MOB_TRIGGER, OBJ_TRIGGER, WLD_TRIGGER) of the owner's kind */
inline int owner_type(const owner_ptr &go) { return static_cast<int>(go.index()); }

/* A script's owner held by its kind and DG UID */
struct script_owner {
  owner_kind kind{};
  long id{};

  /* The owner go is now */
  static script_owner of(const owner_ptr &go);

  /* The owner, if it's still in the game */
  std::optional<owner_ptr> resolve(GameWorld *g) const;
};

/* The entity's DG UID: a character's or object's id, or ROOM_ID_BASE plus a room's vnum */
long owner_uid(const owner_ptr &go);
