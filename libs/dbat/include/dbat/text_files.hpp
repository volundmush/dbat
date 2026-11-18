#pragma once
/*
 * The game's text files: help (text/help/, see db/text/help.cpp) and the pages read whole, such as the motd and the
 * news (see db/text/textfiles.cpp).
 */
#include "dbat/io/reader.hpp"

struct GameWorld;
struct help_index_element;

/* How many keywords a help file holds, which is how many entries its table takes */
int count_help_keywords(const io::text_reader &in);
/* Adds a help file's entries to the help table */
io::loaded<void> read_help_file(GameWorld *g, io::text_reader &in);
/* The help table's order, by keyword whatever its case */
bool help_keyword_order(const help_index_element &a, const help_index_element &b);
/* Writes the whole help table to text/help/help.hlp; false, and logged, if it couldn't */
bool save_help(GameWorld *g);

/* Reads every page at boot */
void load_text_pages(GameWorld *g);

/* Reads the socials, misc/socials.new (or the old form, misc/socials) */
io::loaded<void> read_socials(GameWorld *g);
/* Writes the socials to misc/socials.new; false, and logged, if it couldn't */
bool save_socials(GameWorld *g);
