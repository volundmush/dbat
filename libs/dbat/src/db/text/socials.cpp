/*
 * The socials, misc/socials.new (or misc/socials, the old form, if use_new_socials is off in the config):
 *
 *   ~<command> <sort as> <hide> <min position> <victim's min position> <min level>
 *   <the message to the actor, with no argument>
 *   <to the others, with no argument>
 *   ...                                     13 messages in all, each a line, or "#" for none
 *                                           a blank line between socials
 *   $
 *
 * The old form has "<command> <hide> <victim's min position>" and 8 messages, stopping after 3 if the third is "#".
 *
 * The file is read as fscanf() and fgets() read it: the words and numbers pass over any whitespace before and after
 * them, line ends and blank lines included (quirk: the first message loses the spaces it starts with, and if it is
 * blank, the line after it is the first), and a message is the rest of its line, as fgets() into a
 * MAX_STRING_LENGTH buffer reads it, losing its last character as though it were the line end.
 */
#include <algorithm>
#include <cctype>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "dbat/text_files.hpp"
#include "dbat/config.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/consts/positions.hpp"
#include "dbat/db.hpp"
#include "dbat/game_world.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/log.hpp"
#include "dbat/social.hpp"

namespace {

/* A social's messages in the order the file has them; the old form has the first 8 */
constexpr char *social_messg::*messages[] = {
    &social_messg::char_no_arg,     &social_messg::others_no_arg,     &social_messg::char_found,
    &social_messg::others_found,    &social_messg::vict_found,        &social_messg::not_found,
    &social_messg::char_auto,       &social_messg::others_auto,       &social_messg::char_body_found,
    &social_messg::others_body_found, &social_messg::vict_body_found, &social_messg::char_obj_found,
    &social_messg::others_obj_found,
};
constexpr int old_form_messages = 8;

/* fgets(buf, MAX_STRING_LENGTH), as the socials were read */
constexpr size_t fgets_buffer = MAX_STRING_LENGTH;

/* The number of socials, as the old reader counted them before reading any (quirk: as while (!feof()) counted them,
   the last line counts twice if it ends with a line end): lines starting with '~', or blank lines in the old form.
   last is the last line it read, which is what the first word is if the file holds none. */
int count_socials(std::string_view text, bool new_form, std::string &last)
{
  int count = 0;
  std::string_view piece;
  for (std::string_view rest = text; !rest.empty();) {
    piece = io::fgets_piece(rest, fgets_buffer);
    count += new_form ? piece.starts_with('~') : piece.starts_with('\n') || piece.starts_with('\r');
  }
  if (!piece.empty() && (piece.back() == '\n' || piece.size() == fgets_buffer - 1))
    count += new_form ? piece.starts_with('~') : piece.starts_with('\n') || piece.starts_with('\r');
  last = piece;
  return count;
}

/* The socials file, read a character at a time */
class social_reader {
public:
  explicit social_reader(const io::text_reader &in) : in_(in), text_(in.text()) {}

  /* fscanf(" %s "): a word and the whitespace either side of it; false, leaving w as it was, at the end */
  bool word(std::string &w)
  {
    io::fields f(rest());
    std::string read;
    if (!f.word(read, fgets_buffer - 1))
      return false;
    w = std::move(read);
    take(f);
    skip_spaces();
    return true;
  }
  /* fscanf(" %s %d %d ...") into sort_as and numbers, then the whitespace after them: how many it read */
  int fields(std::string *sort_as, std::span<int *> numbers)
  {
    io::fields f(rest());
    if (sort_as && !f.word(*sort_as, MAX_INPUT_LENGTH - 1))
      return 0;
    for (int *n : numbers)
      if (!f.number(*n))
        break;
    take(f);
    skip_spaces();
    return f.count();
  }
  /* A message by fread_action()'s rules: the rest of the line, or nothing for "#"; an error at the end of the
     file, or on its last line if that has no line end */
  io::loaded<char *> message(int social)
  {
    std::string_view rest = this->rest();
    std::string_view piece = io::fgets_piece(rest, fgets_buffer);
    at_ += piece.size();
    if (piece.empty() || (piece.back() != '\n' && piece.size() < fgets_buffer - 1))
      return std::unexpected(fail(str_format("the file ends in social #{:d}'s messages", social)));
    if (piece.starts_with('#'))
      return nullptr;
    return strdup(std::string(io::chopped_piece(piece)).c_str());
  }

  io::load_error fail(std::string message) const
  {
    int line = 1 + static_cast<int>(std::count(text_.begin(), text_.begin() + std::min(at_, text_.size()), '\n'));
    return {in_.name(), line, std::move(message)};
  }

private:
  std::string_view rest() const { return text_.substr(at_); }
  void take(const io::fields &f) { at_ = text_.size() - f.rest().size(); }
  void skip_spaces()
  {
    while (at_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[at_])))
      at_++;
  }

  const io::text_reader &in_;
  std::string_view text_;
  size_t at_{};
};

} // namespace

io::loaded<void> read_socials(GameWorld *g)
{
  bool new_form                       = CONFIG_NEW_SOCIALS;
  io::loaded<io::text_reader> file = io::text_reader::open(new_form ? SOCMESS_FILE_NEW : SOCMESS_FILE);
  if (!file)
    return std::unexpected(file.error());

  std::string word;
  g->top_of_socialt += count_socials(file->text(), new_form, word);
  mud_log("Social table contains {:d} socials.", g->top_of_socialt);
  if (g->top_of_socialt < 0) /* a game with no socials can still run */
    return {};
  g->soc_mess_list = new social_messg[g->top_of_socialt + 1]();

  social_reader in(*file);
  int curr_soc = -1;
  for (;;) {
    in.word(word); /* quirk: at the end of the file, the word is the one before */
    if (word.starts_with('$'))
      break;
    if (curr_soc >= g->top_of_socialt)
      return std::unexpected(in.fail(str_format("more socials than were counted in the file, near '{}'", word)));

    social_messg social{};
    std::string sort_as;
    int hide = 0, min_char_pos = POS_RESTING, min_pos = 0, min_lvl = 0;
    if (new_form) {
      int *numbers[] = {&hide, &min_char_pos, &min_pos, &min_lvl};
      if (in.fields(&sort_as, numbers) != 5)
        return std::unexpected(in.fail(str_format("format error in the social near '{}'", word)));
      social.command = strdup(word.c_str() + 1); /* quirk: whatever its first character is */
      social.sort_as = strdup(sort_as.c_str());
    } else {
      int *numbers[] = {&hide, &min_pos};
      if (in.fields(nullptr, numbers) != 2)
        return std::unexpected(in.fail(str_format("format error in the social near '{}'", word)));
      social.command = strdup(word.c_str());
      social.sort_as = strdup(word.c_str());
    }
    social.hide                = hide;
    social.min_char_position   = min_char_pos;
    social.min_victim_position = min_pos;
    social.min_level_char      = min_lvl;

    curr_soc++;
    g->soc_mess_list[curr_soc] = social;
    social_messg &stored       = g->soc_mess_list[curr_soc];
    int count                  = new_form ? static_cast<int>(std::size(messages)) : old_form_messages;
    for (int i = 0; i < count; i++) {
      io::loaded<char *> message = in.message(curr_soc);
      if (!message)
        return std::unexpected(message.error());
      stored.*messages[i] = *message;
      /* the old form: with no message for a victim found, the rest are left out */
      if (!new_form && messages[i] == &social_messg::char_found && !*message)
        break;
    }
  }
  g->top_of_socialt = curr_soc;
  return {};
}

bool save_socials(GameWorld *g)
{
  io::text_writer out(SOCMESS_FILE_NEW);
  for (int i = 0; i <= g->top_of_socialt; i++) {
    const social_messg &social = g->soc_mess_list[i];
    out.print("~{} {} {:d} {:d} {:d} {:d}\n", social.command, social.sort_as, social.hide, social.min_char_position,
              social.min_victim_position, social.min_level_char);
    for (char *social_messg::*message : messages)
      out.print("{}\n", social.*message ? social.*message : "#");
    out.write("\n");
  }
  out.write("$\n");
  return out.commit();
}
