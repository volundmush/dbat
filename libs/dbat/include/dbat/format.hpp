#pragma once
/* std::format for the game's text, as the printf-style functions it replaces formatted it */
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

/* v read as unsigned, as printf's %u, %x and %o read it */
template <class T> constexpr auto as_unsigned(T v)
  requires std::is_integral_v<T>
{
  return static_cast<std::make_unsigned_t<T>>(v);
}

/* v as the char printf's %c prints, the byte it ends in; std::format's {:c} throws on an int out of char's range */
template <class T> constexpr char as_char(T v)
  requires std::is_integral_v<T>
{
  return static_cast<char>(v);
}

/* p as printf's %p prints it: "0x" and lowercase hex, or "(nil)" for a null pointer */
inline std::string as_pointer(const void *p) { return p ? std::format("{}", p) : std::string("(nil)"); }
