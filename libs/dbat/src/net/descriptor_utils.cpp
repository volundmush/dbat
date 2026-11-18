#include "dbat/descriptor.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/db.hpp"
#include "dbat/fileop.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/comm.hpp"
#include "dbat/game_world.hpp"

/* Formerly macros in descriptor_utils.hpp. */

bool desc_is_playing(const struct descriptor_data *d)
{
  return (desc_get_connected(d) == CON_TEDIT || desc_get_connected(d) == CON_REDIT || desc_get_connected(d) == CON_MEDIT || desc_get_connected(d) == CON_OEDIT || desc_get_connected(d) == CON_ZEDIT || desc_get_connected(d) == CON_SEDIT || desc_get_connected(d) == CON_CEDIT || desc_get_connected(d) == CON_PLAYING || desc_get_connected(d) == CON_TRIGEDIT || desc_get_connected(d) == CON_AEDIT || desc_get_connected(d) == CON_GEDIT || desc_get_connected(d) == CON_IEDIT || desc_get_connected(d) == CON_HEDIT || desc_get_connected(d) == CON_NEWSEDIT || desc_get_connected(d) == CON_POBJ || desc_get_connected(d) == CON_ASSEDIT || desc_get_connected(d) == CON_HSEDIT || desc_get_connected(d) == CON_LEVELUP);
}

/* Field accessors (formerly accessor macros). */

int desc_get_connected(const struct descriptor_data *d)
{
  return d->connected;
}

void desc_set_connected(struct descriptor_data *d, int v)
{
  d->connected = v;
}
