/*
 * Reading the game's text files: see dbat/io/reader.hpp.
 */
#include <cctype>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "dbat/io/reader.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/log.hpp"

namespace io {

namespace {

/* As isspace() in the "C" locale */
bool is_space(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

bool is_digit(char c) { return c >= '0' && c <= '9'; }

/* The digits at the start of text, after a sign, as strtol() reads them: the value, held to a long's range */
long read_long(std::string_view text, size_t &at)
{
  bool negative = false;
  if (at < text.size() && (text[at] == '+' || text[at] == '-'))
    negative = text[at++] == '-';
  unsigned long long magnitude = 0;
  bool over                    = false;
  for (; at < text.size() && is_digit(text[at]); at++) {
    if (magnitude > (ULLONG_MAX - 9) / 10)
      over = true;
    else
      magnitude = magnitude * 10 + static_cast<unsigned>(text[at] - '0');
  }
  if (negative)
    return over || magnitude > static_cast<unsigned long long>(LONG_MAX) + 1 ? LONG_MIN
                                                                            : static_cast<long>(0 - magnitude);
  return over || magnitude > static_cast<unsigned long long>(LONG_MAX) ? LONG_MAX : static_cast<long>(magnitude);
}

} // namespace

bool is_skipped_line(std::string_view raw)
{
  return raw.empty() || raw[0] == '*' || raw[0] == '\n' || raw[0] == '\r';
}

std::string_view line_text(std::string_view raw)
{
  size_t end = raw.find_first_of(std::string_view("\r\n\0", 3));
  if (end == std::string_view::npos)
    end = raw.size();
  return raw.substr(0, std::min<size_t>(end, READ_SIZE - 1));
}

bool add_string_line(std::string &s, std::string_view raw)
{
  size_t n = raw.find_first_of(std::string_view("\r\n\0", 3));
  if (n == std::string_view::npos)
    n = raw.size();
  if (n > 0 && raw[n - 1] == '~') {
    s.append(raw.substr(0, n - 1));
    return true;
  }
  s.append(raw.substr(0, n));
  s += "\r\n";
  return false;
}

bool read_raw_line(std::FILE *fl, std::string &raw)
{
  raw.clear();
  for (int c; (c = std::getc(fl)) != EOF;) {
    raw += static_cast<char>(c);
    if (c == '\n')
      break;
  }
  return !raw.empty();
}

long c_atol(std::string_view text)
{
  size_t at = 0;
  while (at < text.size() && is_space(text[at]))
    at++;
  if (at < text.size() && (text[at] == '+' || text[at] == '-') && (at + 1 >= text.size() || !is_digit(text[at + 1])))
    return 0;
  return read_long(text, at);
}

int scan_ints(std::string_view text, std::span<int> out)
{
  size_t at = 0;
  int read  = 0;
  for (int &n : out) {
    while (at < text.size() && is_space(text[at]))
      at++;
    if (at >= text.size())
      return read ? read : -1; /* the text ran out: sscanf() says EOF if nothing was read */
    size_t start = at;
    if (text[at] == '+' || text[at] == '-')
      at++;
    if (at >= text.size() || !is_digit(text[at]))
      return read; /* not a number: a sign alone, even at the end, is a mismatch, not the text running out */
    at = start;
    n  = static_cast<int>(read_long(text, at)); /* quirk: past an int, cut to one */
    read++;
  }
  return read;
}

int scan_float(std::string_view text, float &x)
{
  size_t at = 0;
  while (at < text.size() && is_space(text[at]))
    at++;
  if (at >= text.size())
    return -1;
  std::string rest(text.substr(at)); /* strtof() reads %f's numbers, "inf" and "nan" with them, and needs an end */
  char *end = nullptr;
  float read = std::strtof(rest.c_str(), &end);
  if (end == rest.c_str())
    return 0;
  /* sscanf() takes in what could still become a number, then has read nothing if it doesn't: an exponent's 'e' (or a
     hex number's 'p') and sign with no digits after them, or a "0x" with no hex digits */
  std::string_view parsed(rest.c_str(), static_cast<size_t>(end - rest.c_str()));
  if (parsed.find_first_of("iInN") == std::string_view::npos) { /* not inf or nan */
    bool hex = parsed.find_first_of("xX") != std::string_view::npos;
    std::string_view digits = parsed.substr(parsed[0] == '+' || parsed[0] == '-');
    if (!hex && digits == "0" && (*end == 'x' || *end == 'X'))
      return 0;
    const char *exponent = hex ? "pP" : "eE";
    if (parsed.find_first_of(exponent) == std::string_view::npos && (*end == exponent[0] || *end == exponent[1])) {
      const char *after = end + 1;
      if (*after == '+' || *after == '-')
        after++;
      if (!isdigit(static_cast<unsigned char>(*after)))
        return 0;
    }
  }
  x = read;
  return 1;
}

bool fields::start()
{
  if (failed_)
    return false;
  while (at_ < text_.size() && is_space(text_[at_]))
    at_++;
  if (at_ >= text_.size())
    failed_ = true;
  return !failed_;
}

bool fields::number(int &n)
{
  if (!start())
    return false;
  size_t digits = at_ + (text_[at_] == '+' || text_[at_] == '-');
  if (digits >= text_.size() || !is_digit(text_[digits])) {
    failed_ = true;
    return false;
  }
  n = static_cast<int>(read_long(text_, at_)); /* quirk: past an int, cut to one */
  count_++;
  return true;
}

bool fields::word(std::string &w, size_t width)
{
  if (!start())
    return false;
  size_t from = at_;
  while (at_ < text_.size() && at_ - from < width && !is_space(text_[at_]))
    at_++;
  w.assign(text_.substr(from, at_ - from));
  count_++;
  return true;
}

bool fields::until(std::string &w, std::string_view stops, size_t width)
{
  if (!start())
    return false;
  size_t from = at_;
  while (at_ < text_.size() && at_ - from < width && stops.find(text_[at_]) == std::string_view::npos)
    at_++;
  if (at_ == from) {
    failed_ = true;
    return false;
  }
  w.assign(text_.substr(from, at_ - from));
  count_++;
  return true;
}

bool fields::literal(char c)
{
  if (failed_ || at_ >= text_.size() || text_[at_] != c) {
    failed_ = true;
    return false;
  }
  at_++;
  return true;
}

bitvector_t ascii_flags(std::string_view word, int first_letter_bit, const char *who)
{
  bitvector_t flags = 0;
  bool is_num = true, warned = false;

  for (char ch : word) {
    unsigned char c = static_cast<unsigned char>(ch);
    if (c >= 'a' && c <= 'z')
      flags |= 1U << (first_letter_bit + (c - 'a'));
    else if (c >= 'A' && c <= 'F')
      flags |= 1U << (26 + (c - 'A'));
    else if (c >= 'A' && c <= 'Z' && !warned) {
      /* Only a-z and A-F fit in a 32-bit bitvector_t; see sprintascii(). */
      mud_log("SYSERR: {}: ignoring out-of-range flag letter '{:c}' in \"{}\".", who, as_char(c), word);
      warned = true;
    }
    if (!(is_digit(ch) || ch == '-'))
      is_num = false;
  }
  if (is_num)
    flags = static_cast<bitvector_t>(c_atol(word));
  return flags;
}

loaded<std::vector<std::string>> read_index(const std::filesystem::path &path)
{
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return std::unexpected(load_error{path.string(), 0, "can't be opened"});
  std::vector<std::string> entries;
  std::string entry;
  for (;;) {
    if (!(in >> entry)) {
      mud_log("SYSERR: index file ends without a '$'");
      break;
    }
    if (entry[0] == '$')
      break;
    entries.push_back(entry);
  }
  return entries;
}

loaded<text_reader> text_reader::open(const std::filesystem::path &path)
{
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return std::unexpected(load_error{path.string(), 0, "can't be opened"});
  std::ostringstream text;
  text << in.rdbuf();
  return text_reader(path.string(), std::move(text).str());
}

text_reader::text_reader(std::string name, std::string text) : name_(std::move(name)), text_(std::move(text)) {}

std::optional<std::string_view> text_reader::raw_line()
{
  if (at_end())
    return std::nullopt;
  size_t end = text_.find('\n', pos_);
  end        = end == std::string::npos ? text_.size() : end + 1;
  std::string_view raw(text_.data() + pos_, end - pos_);
  line_ = newlines_ + 1;
  if (raw.back() == '\n')
    newlines_++;
  pos_ = end;
  return raw;
}

std::optional<std::string_view> text_reader::line()
{
  for (;;) {
    std::optional<std::string_view> raw = raw_line();
    if (!raw)
      return std::nullopt;
    if (!is_skipped_line(*raw))
      return line_text(*raw);
  }
}

loaded<std::string> text_reader::tilde_string(std::string_view what)
{
  std::string s;
  for (;;) {
    std::optional<std::string_view> raw = raw_line();
    if (!raw)
      return std::unexpected(fail(str_format("the file ends inside a string, at or near {}", what)));
    if (add_string_line(s, *raw))
      return s;
  }
}

int text_reader::peek_letter()
{
  while (pos_ < text_.size() && is_space(text_[pos_]))
    if (text_[pos_++] == '\n')
      newlines_++;
  return pos_ < text_.size() ? static_cast<unsigned char>(text_[pos_]) : EOF;
}

} // namespace io
