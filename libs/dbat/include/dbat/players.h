#pragma once
#include <memory>
#include <map>
#include <string>

#include "Result.h"
struct Character;
struct PlayerData;

// global variables

Character *findPlayer(const std::string &name);

Result<std::string> validate_pc_name(const std::string &name);

extern bool canDeleteCharacter(std::weak_ptr<Character> ref);
extern bool deleteUserAccount(int id);
extern void deletePlayerCharacter(std::weak_ptr<Character> ref);

extern std::map<int64_t, std::shared_ptr<PlayerData>> players;

extern long get_id_by_name(const char *name);

extern char *get_name_by_id(long id);