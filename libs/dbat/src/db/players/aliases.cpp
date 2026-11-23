/*
 * A player's aliases, plralias/<bucket>/<name>.alias: each alias is five lines,
 *
 *   <length of the alias>
 *   <the alias>
 *   <length of what it stands for>
 *   <what it stands for>
 *   <its type: simple, or complex with ';' and $ arguments>
 *
 * read as fscanf("%d\n") and fgets() read them: a length passes over all the whitespace after it, the next line's
 * leading spaces too (quirk), and the text is that many characters, or up to and including a line end before them.
 */
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <list>
#include <optional>
#include <string>
#include <string_view>

#include "dbat/alias.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/command.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/fileop.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/paths.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"

namespace {

class alias_reader {
public:
  explicit alias_reader(std::string_view text) : text_(text) {}

  /* fscanf("%d\n"): a number and all the whitespace after it */
  bool number(int &n)
  {
    io::fields f(text_.substr(at_));
    if (!f.number(n))
      return false;
    at_ = text_.size() - f.rest().size();
    while (at_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[at_])))
      at_++;
    return true;
  }
  /* fgets(buf, length + 1): up to length characters, stopping after a line end; as a C string, up to a '\0' */
  std::string text(int length)
  {
    std::string_view piece = text_.substr(at_, static_cast<size_t>(length));
    if (size_t end = piece.find('\n'); end != std::string_view::npos)
      piece = piece.substr(0, end + 1);
    at_ += piece.size();
    return std::string(piece.substr(0, piece.find('\0')));
  }
  bool at_end() const { return at_ >= text_.size(); }

private:
  std::string_view text_;
  size_t at_{};
};

/* One alias; nothing if the file ends or is malformed partway through it */
std::optional<alias_data> read_alias(alias_reader &in)
{
  alias_data a;
  int length = 0;
  if (!in.number(length) || length < 0 || length >= MAX_STRING_LENGTH - 1)
    return std::nullopt;
  a.alias = in.text(length);
  if (!in.number(length) || length < 0 || length >= MAX_STRING_LENGTH - 1)
    return std::nullopt;
  a.replacement = " " + in.text(length);
  if (!in.number(a.type))
    return std::nullopt;
  return a;
}

} // namespace

void write_aliases(GameWorld *g, struct char_data *ch)
{
  std::optional<std::string> fn = player_path(ALIAS_FILE, char_get_name(ch));
  if (!fn)
    return;
  std::list<alias_data> *aliases = char_aliases(g, ch);

  if (!aliases || aliases->empty()) {
    std::error_code ec;
    std::filesystem::remove(*fn, ec);
    return;
  }

  io::text_writer out(*fn, true);
  for (const alias_data &a : *aliases) {
    std::string_view replacement = a.replacement;
    replacement.remove_prefix(std::min<size_t>(1, replacement.size())); /* its leading space */
    out.print("{:d}\n{}\n{:d}\n{}\n{:d}\n", static_cast<int>(a.alias.size()), a.alias,
              static_cast<int>(a.replacement.size()) - 1, replacement, a.type);
  }
  out.commit();
}

void read_aliases(GameWorld *g, struct char_data *ch)
{
  std::optional<std::string> fn = player_path(ALIAS_FILE, char_get_name(ch));
  if (!fn)
    return;
  io::loaded<io::text_reader> file = io::text_reader::open(*fn);
  if (!file) {
    std::error_code ec;
    if (std::filesystem::exists(*fn, ec))
      mud_log("SYSERR: Couldn't open alias file '{}' for {}: {}", *fn, char_get_name(ch), io::why_unreadable(*fn));
    return;
  }

  /* A malformed record is dropped along with everything after it; the records before it are kept. */
  std::list<alias_data> loaded;
  alias_reader in(file->text());
  while (!in.at_end())
    if (std::optional<alias_data> a = read_alias(in))
      loaded.push_back(std::move(*a));
    else
      break;

  if (std::list<alias_data> *aliases = char_aliases(g, ch))
    *aliases = std::move(loaded);
}

void delete_aliases(std::string_view charname)
{
  std::optional<std::string> fn = player_path(ALIAS_FILE, charname);
  if (!fn)
    return;
  std::error_code ec;
  if (!std::filesystem::remove(*fn, ec) && ec)
    mud_log("SYSERR: deleting alias file {}: {}", *fn, ec.message());
}
