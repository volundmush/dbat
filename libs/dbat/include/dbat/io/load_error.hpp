#pragma once
/*
 * Why a file couldn't be loaded: where, and what was wrong. Loaders give one back rather than stopping the game, and
 * whoever called them decides what happens: the boot stops, a player's file is refused, and so on.
 */
#include <expected>
#include <string>

#include "dbat/format.hpp"

namespace io {

struct load_error {
  std::string path{};    /* the file, as it was opened */
  int line{};            /* from 1; 0 when the error isn't on a line, such as a file that can't be opened */
  std::string message{};

  /* "world/obj/3.obj:142: object #312: expected 13 flag fields, got 4" */
  std::string text() const { return line ? str_format("{}:{:d}: {}", path, line, message) : str_format("{}: {}", path, message); }
};

/* What a loader gives: what it read, or why it couldn't */
template <class T> using loaded = std::expected<T, load_error>;

} // namespace io
