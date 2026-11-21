#pragma once
#include <cstdlib>
#include <array>
#include <cstring>
#include <deque>
#include <memory>
#include <string>

#include "dbat/account.hpp"
#include "dbat/chargen_draft.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/constates.hpp"
#include "dbat/net/platform.hpp"
#include "dbat/obj_edit.hpp"
#include "dbat/olc/session.hpp"

/* A line of input waiting to be run */
struct txt_line {
  std::string text;
  int aliased{}; /* it came from an alias, not from the player */
};

struct txt_q {
  std::deque<txt_line> lines; /* first in, first out */
  int typed{};                /* queued lines the player typed (alias expansions are not counted) */
};

/* What waits to be sent to a connection. It holds SMALL_BUFSIZE - 1 bytes until a text doesn't fit, then
   LARGE_BUFSIZE - 1 until everything has been sent; full to that, it is in the overflow state, and takes nothing
   more until it has been sent. */
struct output_buffer {
  std::string text;
  bool large{}; /* grown past the small size */

  /* Room left for more */
  size_t space() const { return (large ? LARGE_BUFSIZE : SMALL_BUFSIZE) - 1 - text.size(); }
  bool overflowed() const { return space() == 0; }
};

struct descriptor_data {
  net::socket sock;                    /* the connection */
  std::string host;                    /* the address it came from, at most HOST_LENGTH long */
  int8_t bad_pws{};                    /* number of bad pw attemps this login	*/
  int8_t idle_tics{};                  /* tics idle at password prompt		*/
  int connected{};                     /* mode of 'connectedness'		*/
  int desc_num{};                      /* unique num assigned to desc		*/
  time_t login_time{};                 /* when the person connected		*/
  std::string *str{};                  /* where the text being written goes once saved, or null */
  std::string edit_text;               /* the text being written, saved to str at the end */
  std::string draft;                   /* where a news entry or letter is saved, until it is filed or sent */
  std::string backstr;                 /* what was there when writing began, put back if an OLC edit is aborted */
  size_t max_str{};                    /* maximum size of string in modify-str	*/
  int32_t mail_to{};                   /* name for mail system			*/
  int has_prompt{};                    /* is the user at a prompt?             */
  net::readiness ready;                /* how the socket stood at this pulse's poll (game_loop) */
  std::string raw_input;               /* what has come since the last newline (MAX_RAW_INPUT_LENGTH - 1 at most) */
  std::string last_input;              /* the last command, for ! and ^old^new */
  output_buffer out;                   /* what waits to be sent */
  std::array<std::string, HISTORY_SIZE> history; /* the commands typed, for !text */
  int history_pos{};                   /* Circular array position.		*/
  struct txt_q input{};                /* q of unprocessed input		*/
  struct char_data *character{};       /* linked to char			*/
  struct char_data *original{};        /* original char if switched		*/
  struct descriptor_data *snooping{};  /* Who is this char snooping	*/
  struct descriptor_data *snoop_by{};  /* And who is snooping this char	*/
  std::unique_ptr<olc_session> olc{};  /* the editor in use, or character creation's stat points */
  struct account acct;                 /* the account logged in to            */
  std::string loadplay;                /* What character am I loading?        */
  int writenew{};                      /* What slot am I writing to?          */
  struct chargen_draft chargen;         /* a character being made: what waits for the end */
  std::string newsbuf;
  struct pobj_order pobj;              /* an order at the custom equipment or restring menu */
  /*---------------Ship Construction Editing Variables---------------------*/
  int shipmenu{};
  int shipsize{};
  int shipextra[4]{};
  int shields{};
  int armor{};
  int drive{};
  int shipweap{};
  /*-----------------------------------------------------------------------*/
  std::string title;
};

template <class Editor>
Editor &session_of(struct descriptor_data *d)
{
  return static_cast<Editor &>(*d->olc);
}
