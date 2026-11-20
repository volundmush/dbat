#pragma once
#include <string_view>

#include "dbat/consts/types.hpp"

/* The OLC commands: do_oasis() starts most of them, by subcommand */
ACMD(do_oasis);
ACMD(do_oasis_list);
ACMD(do_oasis_links);
ACMD(do_oasis_oedit);
ACMD(do_oasis_redit);
ACMD(do_oasis_gedit);
ACMD(do_oasis_zedit);
ACMD(do_oasis_cedit);
ACMD(do_oasis_hsedit);
ACMD(do_iedit);
ACMD(do_show_save_list);
ACMD(do_rcopy);

/* All of a zone's settings and how much is in it, as "zlist <zone>" shows them */
void print_zone(GameWorld *g, struct char_data *ch, zone_vnum vnum);
