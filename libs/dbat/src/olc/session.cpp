/*
 * The editors' sessions: nanny()'s way into them, and what opening and leaving an editor share.
 */
#include "dbat/consts/constates.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/consts/admlevel.hpp"
#include "dbat/consts/playerflags.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/handler.hpp"
#include "dbat/log.hpp"
#include "dbat/oasis.hpp"
#include "dbat/genolc.hpp"
#include "dbat/genzon.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/stringutils.hpp"
#include "dbat/utils.hpp"
#include "dbat/olc/session.hpp"

int olc_session::help_context(const struct descriptor_data *) const { return NOTHING; }

void open_session(GameWorld *g, struct descriptor_data *d, std::unique_ptr<olc_session> session, std::string_view who)
{
  if (d->olc && !who.empty())
    mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "SYSERR: {}: Player already had olc structure.", who);
  d->olc = std::move(session);
}

void begin_editing(GameWorld *g, struct descriptor_data *d, int state)
{
  desc_set_connected(d, state);
  act(g, "$n starts using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
  char_plr_set(d->character, PLR_WRITING, true);
}

void leave_olc(GameWorld *g, struct descriptor_data *d)
{
  if (!d->olc)
    return;
  if (d->character) {
    char_plr_set(d->character, PLR_WRITING, false);
    act(g, "$n stops using OLC.", true, d->character, nullptr, nullptr, TO_ROOM);
    d->olc->log_leaving(g, d);
    desc_set_connected(d, CON_PLAYING);
  }
  d->olc.reset();
}

void log_stops_editing(GameWorld *g, struct descriptor_data *d, zone_vnum zone)
{
  mudlog(g, LOG_BRF, ADMLVL_IMMORT, true, "OLC: {} stops editing zone {:d} allowed zone {:d}", char_get_name(d->character), zone,
         char_get_olc_zone(g, d->character));
}

IDXTYPE zone_to_save(GameWorld *g, struct char_data *ch, std::string_view named, zone_vnum lowest_own)
{
  IDXTYPE number = NOWHERE;
  if (is_number(named))
    number = parse_int(named);
  else if (char_get_olc_zone(g, ch) >= lowest_own)
    if (struct zone_data *zone = find_zone(g, char_get_olc_zone(g, ch)))
      number = genolc_zone_bottom(g, zone);
  if (number == NOWHERE)
    send_to_char(g, ch, "Save which zone?\r\n");
  return number;
}

struct zone_data *zone_to_edit(GameWorld *g, struct char_data *ch, IDXTYPE number, bool by_zone_number)
{
  struct zone_data *zone = by_zone_number ? find_zone(g, number) : find_zone_by_thing(g, number);
  if (zone == nullptr) {
    send_to_char(g, ch, "Sorry, there is no zone for that number!\r\n");
    return nullptr;
  }
  if (!can_edit_zone(g, ch, zone)) {
    send_cannot_edit(g, ch, zone->number);
    return nullptr;
  }
  return zone;
}

void save_zone_files(GameWorld *g, struct char_data *ch, struct zone_data *zone, std::string_view things, std::string_view info,
                     bool (*save)(GameWorld *, struct zone_data *))
{
  send_to_char(g, ch, "Saving all {} in zone {:d}.\r\n", things, zone->number);
  mudlog(g, LOG_CMP, MAX(ADMLVL_BUILDER, char_get_invis_level(g, ch)), true, "OLC: {} saves {} for zone {:d}.", char_get_name(ch), info,
         zone->number);
  save(g, zone);
}

void olc_parse(GameWorld *g, struct descriptor_data *d, char *arg)
{
  if (d->olc)
    d->olc->parse(g, d, arg);
}
