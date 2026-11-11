#pragma once
/* std::format for the game's text, as the printf-style functions it replaces formatted it */
#include <cstdio>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace dbat_format {
/* A C string argument as a view; printf printed a null one as "(null)", where std::format would crash */
template <class T> decltype(auto) arg(T &&v)
{
  using D = std::decay_t<T>;
  if constexpr (std::is_array_v<std::remove_reference_t<T>> && (std::is_same_v<D, const char *> || std::is_same_v<D, char *>))
    return std::string_view(static_cast<const char *>(v)); /* a buffer or a literal, up to its '\0' as %s read it */
  else if constexpr (std::is_same_v<D, const char *> || std::is_same_v<D, char *>)
    return std::string_view(v ? v : "(null)");
  else
    return std::forward<T>(v);
}
} // namespace dbat_format

/* text formatted as std::format does, the format checked when compiled */
template <class... Args> std::string str_format(std::format_string<Args...> fmt, Args &&...args)
{
  return [&](auto &&...v) { return std::vformat(fmt.get(), std::make_format_args(v...)); }(
      dbat_format::arg(std::forward<Args>(args))...);
}

/* writes text to fl as it is: its length, or -1 if the write failed, as fprintf() returned */
inline int file_print(FILE *fl, std::string_view text)
{
  return std::fwrite(text.data(), 1, text.size(), fl) == text.size() ? static_cast<int>(text.size()) : -1;
}
/* writes text formatted as std::format does to fl (a failed write is reported, not thrown as std::print does) */
template <class... Args>
  requires(sizeof...(Args) > 0)
int file_print(FILE *fl, std::format_string<Args...> fmt, Args &&...args)
{
  return file_print(fl, str_format(fmt, std::forward<Args>(args)...));
}

/* v read as unsigned, as printf's %u, %x and %o read it */
template <class T> constexpr auto as_unsigned(T v)
  requires std::is_integral_v<T>
{
  return static_cast<std::make_unsigned_t<T>>(v);
}

/* v padded as printf's %*s, %-*s and %*d pad it to a width given at run time (a negative width pads on the right).
 * std::format's own run-time widths ({:{}}) can't be checked when compiled by clang with libstdc++. */
template <class T>
std::string printf_width(T &&v, int width, bool left)
{
  if (width < 0) {
    left  = true;
    width = -width;
  }
  auto &&value = dbat_format::arg(std::forward<T>(v));
  return left ? std::vformat("{:<{}}", std::make_format_args(value, width))
              : std::vformat("{:>{}}", std::make_format_args(value, width));
}

/* the start of a string, as printf's %.*s keeps it (a negative precision keeps it all) */
template <class T>
std::string_view printf_precision(T &&v, int precision)
{
  std::string_view text = dbat_format::arg(std::forward<T>(v));
  return precision < 0 ? text : text.substr(0, precision);
}

/* v as printf's %.Nd prints it: at least digits digits after the sign (none at all for 0 with no digits) */
template <class T>
std::string printf_digits(T v, int digits)
  requires std::is_integral_v<T>
{
  if (digits == 0 && v == 0)
    return {};
  std::string text = std::format("{}", v);
  size_t sign      = text[0] == '-' ? 1 : 0;
  if (text.size() - sign < static_cast<size_t>(digits))
    text.insert(sign, static_cast<size_t>(digits) - (text.size() - sign), '0');
  return text;
}

/* v as the char printf's %c prints, the byte it ends in; std::format's {:c} throws on an int out of char's range */
template <class T> constexpr char as_char(T v)
  requires std::is_integral_v<T>
{
  return static_cast<char>(v);
}

/* p as printf's %p prints it: "0x" and lowercase hex, or "(nil)" for a null pointer */
inline std::string as_pointer(const void *p) { return p ? std::format("{}", p) : std::string("(nil)"); }
