/*
 * The help files, text/help/<file>.hlp, listed in text/help/index. An entry is
 *
 *   <keywords>                    words, or "words in quotes" as one
 *   <its text, any number of lines>
 *   #<the level it needs>
 *
 * and the file ends with a line starting with '$'. The lines are read as fgets() into a READ_SIZE buffer read them:
 * a longer line comes in pieces, each read as a line of its own, and each loses its last character, as though it
 * were the line end (quirk: a line in pieces loses one in each piece).
 *
 * The help table holds an entry once for each of its keywords; it is saved whole to text/help/help.hlp.
 */
#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

#include "dbat/text_files.hpp"
#include "dbat/args.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/genolc.hpp"
#include "dbat/help.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/stringutils.hpp"

namespace {

/* What an entry's text may hold, as the old reader's buffer held it */
constexpr size_t entry_room = 32384 - 1;
constexpr std::string_view truncated_mark = "\r\n*TRUNCATED*\r\n";

/* A help file's lines, as fgets() read them */
class help_lines {
public:
  explicit help_lines(io::text_reader &in) : in_(in) {}

  /* The next line, or nothing at the end of the file */
  std::optional<std::string_view> next()
  {
    if (rest_.empty()) {
      std::optional<std::string_view> raw = in_.raw_line();
      if (!raw)
        return std::nullopt;
      rest_ = *raw;
    }
    std::string_view piece = io::fgets_piece(rest_);
    ended_ = piece.back() != '\n' && piece.size() < static_cast<size_t>(READ_SIZE - 1);
    return io::chopped_piece(piece);
  }
  /* Whether the line read last was the file's last, with no line end, as feof() then said */
  bool ended() const { return ended_; }
  io::load_error fail(std::string message) const { return in_.fail(std::move(message)); }

private:
  io::text_reader &in_;
  std::string_view rest_{};
  bool ended_{};
};

struct help_entry {
  std::string key; /* its keywords line, with "\r\n" after it */
  std::string text;
  int min_level{};
};

/* The next entry, or nothing at the '$' that ends the file. Lines are passed over once the text is full (quirk: as
   the old reader's buffer was, where a line that filled it to within 2 bytes is joined to the next without a line
   end). With loading set, what is cut short or missing is logged. */
io::loaded<std::optional<help_entry>> read_entry(help_lines &lines, bool first, bool loading)
{
  /* quirk: a file whose last line has no line end is an error, unless it is the first, as the old count found */
  auto next = [&](std::string_view what) -> io::loaded<std::string_view> {
    std::optional<std::string_view> line = lines.next();
    if (!line)
      return std::unexpected(lines.fail(str_format("the file ends {}, before the '$' that should end it", what)));
    if (lines.ended() && !first)
      return std::unexpected(lines.fail(str_format("the file ends {}, without a line end", what)));
    first = false;
    return *line;
  };

  io::loaded<std::string_view> key = next("where an entry's keywords were expected");
  if (!key)
    return std::unexpected(key.error());
  if (key->starts_with('$'))
    return std::nullopt;

  help_entry entry{.key = str_format("{}\r\n", *key)};
  entry.text = entry.key;
  io::loaded<std::string_view> line;
  while ((line = next("in an entry's text")) && !line->starts_with('#')) {
    if (entry.text.size() >= entry_room)
      continue;
    entry.text += line->substr(0, entry_room - entry.text.size());
    if (entry.text.size() + 2 < entry_room)
      entry.text += "\r\n";
  }
  if (!line)
    return std::unexpected(line.error());

  if (entry.text.size() >= entry_room) {
    entry.text.resize(entry_room - truncated_mark.size());
    entry.text += truncated_mark;
    if (loading)
      mud_log("SYSERR: Help entry exceeded buffer space: {}", *key);
  }
  if (io::scan_ints(line->substr(1), std::span(&entry.min_level, 1)) != 1) {
    entry.min_level = 0;
    if (loading)
      mud_log("SYSERR: Help entry does not have a min level. {}", entry.key);
  }
  return entry;
}

/* Each of an entry's keywords, as fn(keyword, n) with n from 0 (quirk: an empty "" ends them) */
template <class Fn> void each_keyword(std::string_view key, Fn fn)
{
  for (int n = 0;; n++) {
    std::string word = take_quoted(key);
    if (word.empty())
      return;
    fn(std::move(word), n);
  }
}

/* Every entry of the file, as fn(entry) */
template <class Fn> io::loaded<void> read_entries(io::text_reader &in, bool loading, Fn fn)
{
  help_lines lines(in);
  for (bool first = true;; first = false) {
    io::loaded<std::optional<help_entry>> entry = read_entry(lines, first, loading);
    if (!entry)
      return std::unexpected(entry.error());
    if (!*entry)
      return {};
    fn(**entry);
  }
}

} // namespace

int count_help_keywords(const io::text_reader &file)
{
  io::text_reader in = file;
  int count          = 0;
  (void)read_entries(in, false, [&](const help_entry &entry) {
    each_keyword(entry.key, [&](std::string, int) { count++; });
  });
  return count;
}

io::loaded<void> read_help_file(GameWorld *g, io::text_reader &in)
{
  return read_entries(in, true, [&](const help_entry &entry) {
    each_keyword(entry.key, [&](std::string keyword, int n) {
      g->help_table.push_back(
          {.keywords = std::move(keyword), .entry = entry.text, .duplicate = n, .min_level = entry.min_level});
    });
  });
}

bool help_keyword_order(const help_index_element &a, const help_index_element &b)
{
  auto folded = [](char c) { return std::tolower(static_cast<unsigned char>(c)); };
  return std::ranges::lexicographical_compare(a.keywords, b.keywords, {}, folded, folded); /* as strcasecmp() */
}

void free_help_table(GameWorld *g)
{
  g->help_table.clear();
}

bool save_help(GameWorld *g)
{
  io::text_writer out(str_format("{}{}", HLP_PREFIX, HELP_FILE));
  for (const help_index_element &el : g->help_table) {
    if (el.duplicate)
      continue;
    std::string text = el.entry.empty() ? std::string("Empty\r\n") : el.entry;
    strip_cr(text);
    out.print("{}#{:d}\n", text, el.min_level);
  }
  out.write("$~\n");
  return out.commit();
}
