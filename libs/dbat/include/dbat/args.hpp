#pragma once
#include <string>
#include <string_view>

/*
 * Taking a command's argument apart a word at a time, the way the game always has: words are separated by
 * whitespace and come back lowercased. Each take_*() takes its word off the front of args, leaving the rest there.
 */

/* The next word, passing over filler words ("the", "in", "at" and the like), as one_argument() did. */
std::string take_arg(std::string_view &args);

/* The next word, filler words included, as any_one_arg() did. */
std::string take_word(std::string_view &args);

/* The next word, or the words between double quotes as one, as one_word() did. */
std::string take_quoted(std::string_view &args);

/* args without its leading whitespace. */
std::string_view skip_spaces(std::string_view args);

/* True for the filler words take_arg() passes over. */
bool is_fill_word(std::string_view word);
