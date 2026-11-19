#pragma once
#include <memory>
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
