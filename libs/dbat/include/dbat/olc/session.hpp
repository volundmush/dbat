#pragma once
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
  int mode{};                  /* what the editor is asking for: the editor's own numbering */
  struct zone_data *in_zone{}; /* the zone of what is being edited */
  room_vnum number{};          /* the vnum of what is being edited */
  int value{};                 /* mostly whether anything has changed; some editors keep a number being asked for here */

  olc_session()                               = default;
  olc_session(const olc_session &)            = delete;
  olc_session &operator=(const olc_session &) = delete;
  virtual ~olc_session()                      = default;

  /* A line typed at the editor, its leading spaces gone. The editor may end the session, freeing itself. */
  virtual void parse(GameWorld *g, struct descriptor_data *d, std::string_view line) = 0;

  /* The text editor this editor opened has been closed: action is STRINGADD_SAVE or STRINGADD_ABORT. */
  virtual void text_done(GameWorld *g, struct descriptor_data *d, int action) {}

  /* The context help (a CONTEXT_* number) for what the editor is asking now, or NOTHING. */
  virtual int help_context(const struct descriptor_data *d) const;
};

/* nanny()'s handler for every editor's state: hands the line to the connection's session. */
void olc_parse(GameWorld *g, struct descriptor_data *d, char *arg);
