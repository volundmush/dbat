#pragma once
#include <optional>
#include <string_view>
#include "dbat/consts/playerflags.hpp"
#include "dbat/consts/types.hpp"

struct transform_bonus {
  int64_t bonus{};
  long double mult{}, drain{};
  std::optional<player_flag> flag{}; /* the form's player flag; none for the base form */
  int rpp_cost{};
  int64_t requires_pl{};
  std::string_view name{};
  std::string_view msg_transform_self{};
  std::string_view msg_transform_others{};
};

struct transforms_available {
  int number{};
  struct transform_bonus *bonuses{};
};

extern const transform_bonus base_form;
extern const transform_bonus oozaru_form;

