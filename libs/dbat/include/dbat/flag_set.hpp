#pragma once
#include <array>
#include <bitset>
#include <cstddef>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "dbat/consts/types.hpp"
#include "dbat/name_table.hpp"

/*
 * Typed flags. Each family (rooms', affects', players' and so on) is an enum class whose last enumerator is count_,
 * and a flag_traits specialisation next to it gives the family's names:
 *
 *   enum class room_flag : uint8_t { dark, death, ..., count_ };
 *   template <> struct flag_traits<room_flag> { static constexpr const name_table &names = room_bits; };
 *
 * A flag_set<room_flag> holds a room's flags. It can't be asked about another family's flag: that is a compile
 * error, where the int constants it replaces let any family's number test any family's array.
 */
template <class E> struct flag_traits;

template <class E>
concept flag_family = std::is_enum_v<E> && requires { E::count_; flag_traits<E>::names; };

/* How many flags the family has */
template <flag_family E> constexpr std::size_t flag_count = static_cast<std::size_t>(std::to_underlying(E::count_));

/* The words a flag set is saved as, four of 32 bits, as the files have always held them */
constexpr std::size_t FLAG_WORDS = 4;
constexpr std::size_t FLAG_BITS  = FLAG_WORDS * 32;

template <flag_family E> class flag_set {
  static_assert(flag_count<E> <= FLAG_BITS, "a flag family must fit in the four words it is saved as");

public:
  constexpr flag_set() = default;

  /* the family's flags */
  constexpr bool test(E flag) const { return bits_.test(index(flag)); }
  constexpr void set(E flag, bool on = true) { bits_.set(index(flag), on); }
  constexpr void reset(E flag) { bits_.reset(index(flag)); }
  constexpr void toggle(E flag) { bits_.flip(index(flag)); }

  /* whether any bit is set, the family's or not */
  constexpr bool any() const { return bits_.any(); }
  constexpr bool none() const { return bits_.none(); }
  constexpr bool operator==(const flag_set &) const = default;

  /* The family's flags that are set, in order */
  auto flags() const
  {
    return std::views::iota(std::size_t{0}, flag_count<E>) | std::views::filter([this](std::size_t i) { return bits_.test(i); }) |
           std::views::transform([](std::size_t i) { return static_cast<E>(i); });
  }

  /*
   * The words, for the files: a bit past the family's flags, as a file may set, is kept, so it is saved as it was
   * read. word() and set_word() are the single-word families' one word.
   */
  constexpr std::array<bitvector_t, FLAG_WORDS> words() const
  {
    std::array<bitvector_t, FLAG_WORDS> out{};
    for (std::size_t i = 0; i < FLAG_WORDS; i++)
      out[i] = word(i);
    return out;
  }
  static constexpr flag_set from_words(std::span<const bitvector_t> words)
  {
    flag_set set;
    for (std::size_t i = 0; i < words.size() && i < FLAG_WORDS; i++)
      set.set_word(i, words[i]);
    return set;
  }
  constexpr bitvector_t word(std::size_t i) const
  {
    bitvector_t w = 0;
    for (std::size_t b = 0; b < 32; b++)
      if (bits_.test(i * 32 + b))
        w |= bitvector_t{1} << b;
    return w;
  }
  constexpr void set_word(std::size_t i, bitvector_t w)
  {
    for (std::size_t b = 0; b < 32; b++)
      bits_.set(i * 32 + b, (w >> b) & 1);
  }

  /* Bit n, whether or not the family names it: for numbers from a menu or a script past its flags (quirk) */
  constexpr bool test_bit(std::size_t n) const { return n < FLAG_BITS && bits_.test(n); }
  constexpr void set_bit(std::size_t n, bool on = true)
  {
    if (n < FLAG_BITS)
      bits_.set(n, on);
  }

private:
  static constexpr std::size_t index(E flag) { return static_cast<std::size_t>(std::to_underlying(flag)); }
  std::bitset<FLAG_BITS> bits_;
};

/* Flag i of the family, if there is one */
template <flag_family E> constexpr std::optional<E> flag_from_index(long long i)
{
  if (i < 0 || static_cast<unsigned long long>(i) >= flag_count<E>)
    return std::nullopt;
  return static_cast<E>(i);
}

/* A flag's name */
template <flag_family E> constexpr std::string_view flag_name(E flag) { return flag_traits<E>::names[static_cast<std::size_t>(std::to_underlying(flag))]; }

/* The flag of this name, exactly as the table has it, reading the table up to an empty name (as get_flag_by_name()
   did) */
template <flag_family E> constexpr std::optional<E> flag_by_name(std::string_view name)
{
  const name_table &names = flag_traits<E>::names;
  for (std::size_t i = 0; i < names.size() && !names[i].empty(); i++)
    if (names[i] == name)
      return static_cast<E>(i);
  return std::nullopt;
}

/* The names of the flags set, each followed by a space but the last ("None " if there are none), those with no
   name left out: what sprintbitarray() gave */
template <flag_family E> std::string flag_names(const flag_set<E> &set)
{
  const name_table &names = flag_traits<E>::names;
  std::string out;
  for (std::size_t i = 0; i < names.size() && i < FLAG_BITS; i++)
    if (set.test_bit(i) && !names[i].empty()) {
      out += names[i];
      out += ' ';
    }
  if (out.empty())
    return "None ";
  out.pop_back();
  return out;
}
