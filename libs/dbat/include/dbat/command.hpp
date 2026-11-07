#pragma once
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
  char *alias{};
  char *replacement{};
  int type{};
  struct alias_data *next{};
};

constexpr int ALIAS_SIMPLE  = 0;
constexpr int ALIAS_COMPLEX = 1;

constexpr int ALIAS_SEP_CHAR  = ';';
constexpr int ALIAS_VAR_CHAR  = '$';
constexpr int ALIAS_GLOB_CHAR = '*';

void free_alias(struct alias_data *a);
struct alias_data *find_alias(struct alias_data *alias_list, char *str);

int find_command(GameWorld *g, const char *command);

