#pragma once
#include <string_view>

#include "dbat/consts/types.hpp"
#include "dbat/io/reader.hpp"

bool save_config(GameWorld *g, struct zone_data *unused);                     /* Exported from cedit.c */
/* For load_config(): one "tag = value" line of the config file; a text option reads its text from in */
io::loaded<void> config_load_option(GameWorld *g, std::string_view tag, std::string_view value, io::text_reader &in);

