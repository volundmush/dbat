#pragma once
#include <stdint.h>

struct command_info {
   const char *command;
   const char *sort_as;
   int8_t minimum_position;
   void	(*command_pointer)
	   (struct char_data *ch, char *argument, int cmd, int subcmd);
   int16_t minimum_level;
   int16_t minimum_admlevel;
   int	subcmd;
};

struct alias_data {
  char *alias;
  char *replacement;
  int type;
  struct alias_data *next;
};

#define ALIAS_SIMPLE	0
#define ALIAS_COMPLEX	1

#define ALIAS_SEP_CHAR	';'
#define ALIAS_VAR_CHAR	'$'
#define ALIAS_GLOB_CHAR	'*'