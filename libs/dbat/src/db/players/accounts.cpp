/*
 * Account files, user/<bucket>/<name>.user: a field a line, in the order each_account_field() gives them (see
 * dbat/account.hpp). And the intro files beside them: the lockout list, and each character's "<name> <what they
 * know them as>" lines.
 *
 * And the customs files, a line for each custom item the account has had made.
 *
 * The account and intro files are read as std::getline() read them, with get_line()'s rules for blank lines and comments: reading goes
 * on while the end of the file hasn't been met (quirk), so a file ending in a line end gives its last line to the
 * field after it too, and an empty one gives the first field an empty line.
 */
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

#include "dbat/account.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/comm.hpp"
#include "dbat/descriptor.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/fileop.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/stringutils.hpp"

namespace {

/* A file read as an istream with std::getline() */
class getline_reader {
public:
  explicit getline_reader(std::string_view text) : text_(text) {}

  /* as istream::good(): false once the end of the file has been met */
  bool good() const { return !eof_; }

  /* The next line that isn't blank or a '*' comment, without its line end, cut to READ_SIZE - 1 bytes; false,
     leaving line as it was, at the end of the file */
  bool next_line(std::string &line)
  {
    std::string_view raw;
    do {
      if (at_ >= text_.size()) {
        eof_ = true;
        return false;
      }
      size_t end = text_.find('\n', at_);
      if (end == std::string_view::npos) {
        raw  = text_.substr(at_);
        at_  = text_.size();
        eof_ = true;
      } else {
        raw = text_.substr(at_, end - at_);
        at_ = end + 1;
      }
    } while (raw.empty() || raw[0] == '*' || raw[0] == '\r');
    raw = raw.substr(0, std::min(raw.find_first_of(std::string_view("\r\n\0", 3)), static_cast<size_t>(READ_SIZE - 1)));
    line = raw;
    return true;
  }

private:
  std::string_view text_;
  size_t at_{};
  bool eof_{};
};

void read_field(const std::string &line, std::string &text)
{
  std::string word;
  io::fields(line).word(word, 99);
  text = word;
}

void read_field(const std::string &line, int &number)
{
  io::fields(line).number(number); /* a line that isn't a number leaves it as it was */
}

} // namespace

bool read_account_file(const std::string &path, account &a)
{
  io::loaded<io::text_reader> file = io::text_reader::open(path);
  if (!file)
    return false;
  getline_reader in(file->text());
  std::string line;
  each_account_field(a, [&](auto &field) {
    if (!in.good())
      return;
    in.next_line(line);
    read_field(line, field);
  });
  return true;
}

bool write_account(const char *name, const account &a)
{
  std::optional<std::string> path = player_path(USER_FILE, name);
  if (!path)
    return false;
  io::text_writer out(*path, true);
  each_account_field(a, [&](const auto &field) { out.print("{}\n", field); });
  if (!out.commit()) {
    mud_log("ERROR: could not save user, {}, to filename, {}.", name, *path);
    return false;
  }
  return true;
}

bool readUserIndex(const std::string &name)
{
  std::optional<std::string> path = player_path(USER_FILE, name.c_str());
  return path && io::text_reader::open(*path).has_value();
}

/* True if name is on the lockout list (the "lockout" intro file). */
bool lockRead(std::string_view name)
{
  std::optional<std::string> path = player_path(INTRO_FILE, "lockout");
  if (!path)
    return false;
  io::loaded<io::text_reader> file = io::text_reader::open(*path);
  if (!file)
    return false;
  getline_reader in(file->text());
  bool known = false;
  std::string line;
  while (in.next_line(line)) {
    std::string word;
    if (io::fields(line).word(word, 49) && str_eq_nocase(name, word))
      known = true;
  }
  return known;
}

/* For transfering money or doing things with an offline player: whose name ch knows by arg, or "NOTHING". */
std::string rIntro(struct char_data *ch, std::string_view arg)
{
  std::string name = "NOTHING";
  if (char_is_npc(ch))
    return name;
  std::optional<std::string> path = player_path(INTRO_FILE, char_get_name(ch));
  if (!path)
    return name;
  io::loaded<io::text_reader> file = io::text_reader::open(*path);
  if (!file)
    return name;

  /* quirk: as read_account_file(), the last line is read twice; and a line of one word keeps the last line's second */
  getline_reader in(file->text());
  std::string line, filler, scrap;
  while (in.good()) {
    in.next_line(line);
    io::fields f(line);
    f.word(filler, 49) && f.word(scrap, 99);
    if (str_eq_nocase(arg, scrap))
      name = filler;
  }
  return name;
}

/* Adds obj to the customs file of ch's account, dropping lines that repeat the one before (and its blank lines) */
void customWrite(struct char_data *ch, struct obj_data *obj)
{
  if (char_is_npc(ch))
    return;
  std::optional<std::string> path = player_path(CUSTOME_FILE, char_get_desc(ch)->acct.user.c_str());
  if (!path) {
    mud_log("ERROR: Custom unable to be saved to user file!");
    return;
  }

  io::text_writer out(*path, true);
  if (io::loaded<io::text_reader> file = io::text_reader::open(*path)) {
    std::string prev;
    while (std::optional<std::string_view> line = file->line()) {
      if (!str_eq_nocase(prev, *line))
        out.print("{}\n", *line);
      prev = *line;
    }
  }
  out.print("{}\n\n", obj_get_short_description(obj));
  out.commit();
}

/* Shows a customs file: name's, to d's character, for type 1, or d's own account's, without lines that repeat the
   one before */
void customRead(GameWorld *g, struct descriptor_data *d, int type, const char *name)
{
  std::optional<std::string> path = player_path(CUSTOME_FILE, type == 1 ? name : d->acct.user.c_str());
  if (!path) {
    mud_log("ERROR: Custom unable to be read from user file!");
    return;
  }
  io::loaded<io::text_reader> file = io::text_reader::open(*path);
  if (!file) {
    mud_log("ERROR: Custom file unable to be read!");
    return;
  }

  std::string buf, prev;
  while (std::optional<std::string_view> line = file->line()) {
    if (!str_eq_nocase(prev, *line))
      buf += str_format("{}\n", *line);
    if (type != 1) /* quirk: someone else's shows its repeats */
      prev = *line;
  }
  if (type == 1)
    send_to_char(g, d->character, buf);
  else
    write_to_output(g, d, buf);
}

/* Makes the customs file of d's account, if it hasn't one */
void customCreate(struct descriptor_data *d)
{
  if (!d || d->acct.customfile == 1)
    return;
  std::optional<std::string> path = player_path(CUSTOME_FILE, d->acct.user.c_str());
  if (!path)
    return;
  io::text_writer out(*path, true);
  out.write("@D--@RUser @GC@gustom@Gs@D--@n\n");
  d->acct.customfile = 1;
  if (!out.commit())
    mud_log("ERROR: could not create custom file.");
}
