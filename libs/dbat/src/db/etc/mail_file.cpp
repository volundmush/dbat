/*
 * The mail file, etc/plrmail: every letter waiting to be received, in the order they were sent.
 *
 *   # dbat plrmail 2
 *   #<to> <from> <when> <length>
 *   <the letter: exactly length bytes, line ends and all>
 *   ...
 *
 * Older deployments hold it as the old block file: see legacy_binary.cpp, which converts it as it loads.
 */
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "dbat/mail.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"

#include "legacy_binary.hpp"

namespace {

constexpr std::string_view plrmail_header = "# dbat plrmail 2";

io::loaded<std::vector<mail_letter>> read_mail_text(io::text_reader &in)
{
  std::optional<std::string_view> line = in.raw_line();
  if (!line || !line->starts_with(plrmail_header))
    return std::unexpected(in.fail(str_format("expected '{}' to begin the file", plrmail_header)));

  std::vector<mail_letter> letters;
  while ((line = in.raw_line())) {
    if (io::is_skipped_line(*line))
      continue;
    mail_letter m;
    long long to = 0, from = 0, when = 0, length = 0;
    io::fields f(io::line_text(*line).substr(line->starts_with('#') ? 1 : 0));
    if (!line->starts_with('#') || !(f.number(to) && f.number(from) && f.number(when) && f.number(length)) || length < 0)
      return std::unexpected(in.fail(str_format("expected '#<to> <from> <when> <length>' to begin a letter, got '{}'",
                                                io::line_text(*line))));
    std::optional<std::string_view> text = in.bytes(static_cast<size_t>(length));
    if (!text)
      return std::unexpected(in.fail(str_format("the letter to {:d} is cut short: the file ends before its {:d} bytes",
                                                to, length)));
    std::optional<std::string_view> end = in.raw_line();
    if (!end || io::line_text(*end) != "")
      return std::unexpected(in.fail(str_format("the letter to {:d} runs past its {:d} bytes", to, length)));
    letters.push_back({.to = static_cast<long>(to), .from = static_cast<long>(from), .time = static_cast<time_t>(when),
                       .text = std::string(*text)});
  }
  return letters;
}

} // namespace

bool save_mail_file(GameWorld *g, bool backup)
{
  io::text_writer out(MAIL_FILE, backup);
  out.print("{}\n", plrmail_header);
  for (const mail_letter &m : g->mail)
    out.print("#{:d} {:d} {:d} {:d}\n{}\n", m.to, m.from, static_cast<long long>(m.time), m.text.size(), m.text);
  return out.commit();
}

/*
 * int scan_file(none)
 * Returns false if mail file is corrupted or true if everything correct.
 *
 * This is called once during boot-up: it reads every letter waiting.
 */
bool scan_file(GameWorld *g)
{
  io::loaded<io::text_reader> file = io::text_reader::open(MAIL_FILE);
  if (!file) {
    mud_log("   Mail file non-existant... creating new file.");
    save_mail_file(g);
    return true;
  }

  std::string_view text = file->text();
  bool converted        = false;
  if (legacy::is_binary(text, plrmail_header, legacy::mail_block_size)) {
    g->mail   = legacy::letters(text);
    converted = true;
  } else if (io::loaded<std::vector<mail_letter>> letters = read_mail_text(*file)) {
    g->mail = std::move(*letters);
  } else {
    mud_log("SYSERR: {}", letters.error().text());
    mud_log("SYSERR: Error booting mail system -- Mail file corrupt!");
    mud_log("SYSERR: Mail disabled!");
    return false;
  }

  mud_log("   {:d} bytes read.", text.size());
  if (converted) {
    if (!save_mail_file(g, true)) {
      mud_log("SYSERR: Mail disabled! Unable to write mail file '{}'.", MAIL_FILE);
      return false;
    }
    mud_log("Converted {} from the old binary format ({:d} letters); the old file is kept as {}.bak", MAIL_FILE,
            g->mail.size(), MAIL_FILE);
  }
  mud_log("   Mail file read -- {:d} messages.", g->mail.size());
  return true;
}
