#pragma once
#include <memory>
#include <optional>
#include <string_view>

#include "dbat/consts/types.hpp"

/*
 * An editor's session: what a connection in one of the OLC states is editing, and how that editor answers a line.
 * The connection owns it (d->olc), and freeing it frees whatever the editor was still holding.
 *
 * Each editor is a session of its own kind; the ones not yet rewritten share oasis_olc_data (oasis.hpp), which finds
 * the editor's code by the connection's state.
 */
struct olc_session {
  olc_session()                               = default;
  olc_session(const olc_session &)            = delete;
  olc_session &operator=(const olc_session &) = delete;
  virtual ~olc_session()                      = default;

  /* A line typed at the editor, its leading spaces gone. The editor may end the session, freeing itself, so
     nothing of it may be touched after that. */
  virtual void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) = 0;

  /* The text editor this editor opened has been closed: action is STRINGADD_SAVE or STRINGADD_ABORT. */
  virtual void text_done(GameWorld *g, struct descriptor_data *d, int action) {}

  /* The context help (a CONTEXT_* number) for what the editor is asking now, or NOTHING. */
  virtual int help_context(const struct descriptor_data *d) const;

  /* Logs the builder leaving the editor ("OLC: ... stops editing ..."). */
  virtual void log_leaving(GameWorld *g, struct descriptor_data *d) const = 0;
};

/* The connection's session, as the editor it is */
template <class Editor>
Editor &session_of(struct descriptor_data *d);

/* Gives d its session for an editor about to open; one it still has is freed, and logged as `who` noticing it unless
   who is empty. */
void open_session(GameWorld *g, struct descriptor_data *d, std::unique_ptr<olc_session> session, std::string_view who);

/* The editor is open: d goes into its state, the room sees the builder start, and the builder is writing. */
void begin_editing(GameWorld *g, struct descriptor_data *d, int state);

/* The builder leaves the editor, back to playing, and the session goes. */
void leave_olc(GameWorld *g, struct descriptor_data *d);

/* nanny()'s handler for every editor's state: hands the line to the connection's session. */
void olc_parse(GameWorld *g, struct descriptor_data *d, char *arg);

/* The first character of an answer, or '\0' for none: what the menus switch on */
constexpr char first_char(std::string_view line) { return line.empty() ? '\0' : line.front(); }

/* An answer to a yes-or-no question ("Do you wish to save your changes?"): y or n in either case, or neither */
constexpr std::optional<bool> yes_or_no(std::string_view line)
{
  switch (first_char(line)) {
    case 'y':
    case 'Y': return true;
    case 'n':
    case 'N': return false;
    default: return std::nullopt;
  }
}

/* What sedit's and gedit's number fields refuse ("Field must be numerical"): a minus sign with no digit after it.
   Anything else is read as atoi() reads it. */
constexpr bool minus_without_digit(std::string_view line)
{
  return line.starts_with('-') && !(line.size() > 1 && line[1] >= '0' && line[1] <= '9');
}

/* The "OLC: ... stops editing zone ..." line most editors log when the builder leaves them */
void log_stops_editing(GameWorld *g, struct descriptor_data *d, zone_vnum zone);

/* The number "<editor> save [zone]" saves: the zone named, else the bottom of the builder's own zone if it is at least
   lowest_own; NOWHERE, the builder asked "Save which zone?", for neither */
IDXTYPE zone_to_save(GameWorld *g, struct char_data *ch, std::string_view named, zone_vnum lowest_own = 1);

/* The zone the builder is opening an editor in, found by zone number or by the vnum of something in it; null, the
   builder told why, if there is none or it isn't theirs to edit */
struct zone_data *zone_to_edit(GameWorld *g, struct char_data *ch, IDXTYPE number, bool by_zone_number);

/* "<editor> save": tells the builder ("Saving all <things> in zone N."), logs it ("... saves <info> for zone N."), and
   writes the zone's files of that kind */
void save_zone_files(GameWorld *g, struct char_data *ch, struct zone_data *zone, std::string_view things, std::string_view info,
                     bool (*save)(GameWorld *, struct zone_data *));
