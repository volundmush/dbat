#pragma once
#include <array>
#include <cstddef>
#include <initializer_list>
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
 * and a flag_traits specialisation next to it gives the family's names. Every enumerator has its value written out:
 * the value is the bit the files save, so it must never move.
 *
 *   enum class room_flag : uint8_t { dark = 0, death = 1, ..., count_ = 66 };
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
  static_assert(std::is_unsigned_v<bitvector_t> && sizeof(bitvector_t) * 8 == 32);

public:
  constexpr flag_set() = default;
  /* this one flag set: a lone flag is a set of one wherever a set is wanted */
  constexpr flag_set(E flag) { set(flag); }
  /* these flags set */
  constexpr flag_set(std::initializer_list<E> flags)
  {
    for (E flag : flags)
      set(flag);
  }

  /* the family's flags */
  constexpr bool test(E flag) const { return test_bit(index(flag)); }
  constexpr void set(E flag, bool on = true) { set_bit(index(flag), on); }
  constexpr void reset(E flag) { set_bit(index(flag), false); }
  constexpr void toggle(E flag) { words_[index(flag) / 32] ^= mask(index(flag)); }

  /* whether any bit is set, the family's or not */
  constexpr bool any() const { return !none(); }
  constexpr bool none() const { return words_ == std::array<bitvector_t, FLAG_WORDS>{}; }
  constexpr bool operator==(const flag_set &) const = default;
  /* whether any bit is set in both: "has any of these", as IS_SET() with a mask asked */
  constexpr bool intersects(const flag_set &other) const
  {
    for (std::size_t i = 0; i < FLAG_WORDS; i++)
      if (words_[i] & other.words_[i])
        return true;
    return false;
  }

  /* other's bits set here as well, or taken away */
  constexpr flag_set &operator|=(const flag_set &other)
  {
    for (std::size_t i = 0; i < FLAG_WORDS; i++)
      words_[i] |= other.words_[i];
    return *this;
  }
  constexpr flag_set &operator-=(const flag_set &other)
  {
    for (std::size_t i = 0; i < FLAG_WORDS; i++)
      words_[i] &= ~other.words_[i];
    return *this;
  }

  /* The family's flags that are set, in order */
  auto flags() const
  {
    return std::views::iota(std::size_t{0}, flag_count<E>) | std::views::filter([this](std::size_t i) { return test_bit(i); }) |
           std::views::transform([](std::size_t i) { return static_cast<E>(i); });
  }

  /*
   * The words, for the files: a bit past the family's flags, as a file may set, is kept, so it is saved as it was
   * read. word() and set_word() are the single-word families' one word.
   */
  constexpr std::array<bitvector_t, FLAG_WORDS> words() const { return words_; }
  static constexpr flag_set from_words(std::span<const bitvector_t> words)
  {
    flag_set set;
    for (std::size_t i = 0; i < words.size() && i < FLAG_WORDS; i++)
      set.set_word(i, words[i]);
    return set;
  }
  constexpr bitvector_t word(std::size_t i) const { return i < FLAG_WORDS ? words_[i] : 0; }
  constexpr void set_word(std::size_t i, bitvector_t w)
  {
    if (i < FLAG_WORDS)
      words_[i] = w;
  }

  /* Bit n, whether or not the family names it: for numbers from a menu or a script past its flags (quirk) */
  constexpr bool test_bit(std::size_t n) const { return n < FLAG_BITS && (words_[n / 32] & mask(n)) != 0; }
  constexpr void toggle_bit(std::size_t n) { set_bit(n, !test_bit(n)); }
  constexpr void set_bit(std::size_t n, bool on = true)
  {
    if (n >= FLAG_BITS)
      return;
    if (on)
      words_[n / 32] |= mask(n);
    else
      words_[n / 32] &= ~mask(n);
  }

private:
  static constexpr std::size_t index(E flag) { return static_cast<std::size_t>(std::to_underlying(flag)); }
  static constexpr bitvector_t mask(std::size_t n) { return bitvector_t{1} << (n % 32); }
  /* the four words as the files hold them, so a flag_set is the size and alignment of the array it replaces */
  std::array<bitvector_t, FLAG_WORDS> words_{};
};

/* Flags together, as the masks they replace were or'ed: EX_ISDOOR | EX_CLOSED. The result is a set, never a flag,
   so it can't stand in for one (two flags or'ed as ints made a third) */
template <flag_family E> constexpr flag_set<E> operator|(flag_set<E> set, E flag)
{
  set.set(flag);
  return set;
}
template <flag_family E> constexpr flag_set<E> operator|(E a, E b) { return flag_set<E>{a, b}; }

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

/* What sprintbit() gave, for the single-word families: the names of word 0's bits, each followed by a space but the
   last ("None " if none are set), a bit past the names as UNDEFINED and one with an empty name as its empty name */
template <flag_family E> std::string flag_mask_names(const flag_set<E> &set)
{
  const name_table &names = flag_traits<E>::names;
  std::string out;
  std::size_t nr = 0;
  for (bitvector_t w = set.word(0); w; w >>= 1, nr++)
    if (w & 1) {
      out += nr < names.size() ? names[nr] : std::string_view("UNDEFINED");
      out += ' ';
    }
  if (out.empty())
    return "None ";
  out.pop_back();
  return out;
}
