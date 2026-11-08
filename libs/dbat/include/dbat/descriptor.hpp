#pragma once
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>

#include <sys/socket.h>

#include "dbat/consts/types.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/constates.hpp"

struct txt_block {
  char *text{};
  int aliased{};
  struct txt_block *next{};
};

/* A line of input waiting to be run */
struct txt_line {
  std::string text;
  int aliased{}; /* it came from an alias, not from the player */
};

struct txt_q {
  std::deque<txt_line> lines; /* first in, first out */
  int typed{};                /* queued lines the player typed (alias expansions are not counted) */
};

/*
 * Where text written in the editor goes once it is saved: a malloc'd char * string, which is freed and replaced, or a
 * std::string. Fields move from the first to the second as they become std::string.
 */
class edit_target {
  char **chars_{};
  std::string *text_{};

public:
  edit_target() = default;
  edit_target(char **chars) : chars_(chars) {}
  edit_target(std::string *text) : text_(text) {}

  explicit operator bool() const { return chars_ || text_; }
  /* the text there now, "" if none */
  std::string get() const
  {
    if (chars_)
      return *chars_ ? *chars_ : "";
    return text_ ? *text_ : std::string();
  }
  void set(const std::string &text) const
  {
    if (chars_) {
      free(*chars_);
      *chars_ = strdup(text.c_str());
    } else if (text_)
      *text_ = text;
  }
  /* the text there before editing began, kept as a char * (null for none) */
  void restore(char *old) const
  {
    if (chars_) {
      free(*chars_);
      *chars_ = old;
    } else if (text_) {
      *text_ = old ? old : "";
      free(old);
    }
  }
  bool is(char *const *chars) const { return chars_ && chars_ == chars; }
  char **chars() const { return chars_; }
  std::string *text() const { return text_; }
};

struct descriptor_data {
  int descriptor{};                    /* file descriptor for socket		*/
  char host[HOST_LENGTH + 1]{};        /* hostname				*/
  int8_t bad_pws{};                    /* number of bad pw attemps this login	*/
  int8_t idle_tics{};                  /* tics idle at password prompt		*/
  int connected{};                     /* mode of 'connectedness'		*/
  int desc_num{};                      /* unique num assigned to desc		*/
  time_t login_time{};                 /* when the person connected		*/
  edit_target str;                     /* where the text being written goes, or none */
  std::string edit_text;               /* the text being written, saved to str at the end */
  std::string draft;                   /* where a news entry is saved, until it is filed */
  char *backstr{};                     /* backup string for modify-str system	*/
  size_t max_str{};                    /* maximum size of string in modify-str	*/
  int32_t mail_to{};                   /* name for mail system			*/
  int has_prompt{};                    /* is the user at a prompt?             */
  short poll_revents{};                /* this pulse's poll() result (game_loop) */
  char inbuf[MAX_RAW_INPUT_LENGTH]{};  /* buffer for raw input		*/
  char last_input[MAX_INPUT_LENGTH]{}; /* the last input			*/
  char small_outbuf[SMALL_BUFSIZE]{};  /* standard output buffer		*/
  char *output{};                      /* ptr to the current output buffer	*/
  std::string history[HISTORY_SIZE];   /* History of commands, for ! mostly.	*/
  int history_pos{};                   /* Circular array position.		*/
  int bufptr{};                        /* ptr to end of current output		*/
  int bufspace{};                      /* space left in the output buffer	*/
  struct txt_block *large_outbuf{};    /* ptr to large buffer, if we need it */
  struct txt_q input{};                /* q of unprocessed input		*/
  struct char_data *character{};       /* linked to char			*/
  struct char_data *original{};        /* original char if switched		*/
  struct descriptor_data *snooping{};  /* Who is this char snooping	*/
  struct descriptor_data *snoop_by{};  /* And who is snooping this char	*/
  struct oasis_olc_data *olc{};        /* OLC info                            */
  std::string user;                    /* What user am I?                     */
  std::string email;                   /* User Account Email.                 */
  std::string pass;                    /* User Account Password.              */
  std::string loadplay;                /* What character am I loading?        */
  int writenew{};                      /* What slot am I writing to?          */
  int total{};                         /* What Is My Total Character Limit?   */
  int rpp{};                           /* What is my total RPP?               */
  std::string tmp1;
  std::string tmp2;
  std::string tmp3;
  std::string tmp4;
  std::string tmp5;
  int level{};
  std::string newsbuf;
  /*---------------Player Level Object Editing Variables-------------------*/
  int obj_editval{};
  int obj_editflag{};
  std::string obj_was;
  std::string obj_name;
  std::string obj_short;
  std::string obj_long;
  int obj_type{};
  int obj_weapon{};
  int32_t obj_point_uid{}; /* the object being restrung, by id */
  /*---------------Ship Construction Editing Variables---------------------*/
  int shipmenu{};
  int shipsize{};
  int shipextra[4]{};
  int shields{};
  int armor{};
  int drive{};
  int shipweap{};
  /*-----------------------------------------------------------------------*/
  int user_freed{};
  int customfile{};
  std::string title;
  int rbank{};
};

