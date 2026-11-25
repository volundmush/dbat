#pragma once
#include <cstddef>
#include <span>
#include <string_view>

/*
 * A table of names, such as dirs or item_types, read by number. The tables were arrays of C strings ended by a "\n",
 * and the game indexes them with numbers read from objects, characters and files, which it doesn't always check: one
 * at the end read the "\n" (and showed it as a line break), one past it whatever memory followed. So a name past the
 * end is "\n", as the end once was (quirk), and never a read past the table. Readers that go through every name take
 * it as a std::span.
 */
class name_table {
public:
  constexpr name_table(std::span<const std::string_view> names) : names_(names) {}

  /* name i, or "\n" past the end (a negative number, made a size_t, is past it too) */
  constexpr std::string_view operator[](std::size_t i) const { return i < names_.size() ? names_[i] : "\n"; }
  constexpr std::size_t size() const { return names_.size(); }
  constexpr auto begin() const { return names_.begin(); }
  constexpr auto end() const { return names_.end(); }
  constexpr std::span<const std::string_view> span() const { return names_; }
  constexpr operator std::span<const std::string_view>() const { return names_; }

private:
  std::span<const std::string_view> names_;
};

/* names[type] as a description shows a type (a sector, an item type, an apply): UNDEFINED past the end of names, and
 * for a negative type the first name (quirk) */
constexpr std::string_view type_name(std::span<const std::string_view> names, int type)
{
  std::size_t nr = type < 0 ? 0 : static_cast<std::size_t>(type);
  return nr < names.size() ? names[nr] : "UNDEFINED";
}
