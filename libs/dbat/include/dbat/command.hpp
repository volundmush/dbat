#pragma once
#include <list>
#include <string>
#include <string_view>
#include "dbat/consts/types.hpp"

struct command_info {
  const char *command{};
  const char *sort_as{};
  int8_t minimum_position{};
  void (*command_pointer)(GameWorld *g, struct char_data *ch, char *argument, int cmd, int subcmd){};
  int16_t minimum_level{};
  int16_t minimum_admlevel{};
  int subcmd{};
};

struct alias_data {
  std::string alias;
  std::string replacement; /* starts with a space */
  int type{};
};

constexpr int ALIAS_SIMPLE  = 0;
constexpr int ALIAS_COMPLEX = 1;

constexpr int ALIAS_SEP_CHAR  = ';';
constexpr int ALIAS_VAR_CHAR  = '$';
constexpr int ALIAS_GLOB_CHAR = '*';

struct alias_data *find_alias(std::list<alias_data> &aliases, std::string_view str);

int find_command(GameWorld *g, const char *command);

