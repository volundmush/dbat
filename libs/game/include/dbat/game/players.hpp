#pragma once
#include <memory>
#include <unordered_map>
#include <string>

#include "Result.hpp"
struct Character;
struct PlayerData;

// global variables

Character *findPlayer(const std::string &name);

Result<std::string> validate_pc_name(const std::string &name);

extern bool canDeleteCharacter(std::weak_ptr<Character> ref);
extern bool deleteUserAccount(std::string id);
extern void deletePlayerCharacter(std::weak_ptr<Character> ref);

extern std::unordered_map<std::string, std::shared_ptr<PlayerData>> players;
