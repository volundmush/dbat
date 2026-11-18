/*
 * The file layer (dbat/io) against what it replaces: the old get_line(), fread_string(), fread_letter(),
 * asciiflag_conv() and get_filename(), kept here as they were (on POSIX's getline() and open_memstream()) to compare
 * with, and sscanf() itself, on awkward input and on random input.
 */
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

#include <unistd.h> /* getpid(), for a directory of its own; the test is POSIX, as the old readers it compares with are */

#include "dbat/io/paths.hpp"
#include "dbat/io/reader.hpp"
#include "dbat/io/writer.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/db.hpp"
#include "dbat/fileop.hpp"
#include "dbat/format.hpp"

static int failures;
#define CHECK(cond)                                                                                         \
  do {                                                                                                      \
    if (!(cond)) {                                                                                          \
      file_print(stdout, "FAIL {}:{:d}: {}\n", __FILE__, __LINE__, #cond);                                  \
      failures++;                                                                                           \
    }                                                                                                       \
  } while (0)

/* ---- the old readers, as they were ---- */

static int old_get_line(FILE *fl, char *buf)
{
  char *line = nullptr;
  size_t cap = 0;
  int lines  = 0;

  do {
    if (getline(&line, &cap, fl) < 0) {
      free(line);
      return (0);
    }
    lines++;
  } while (*line == '*' || *line == '\n' || *line == '\r');

  line[strcspn(line, "\r\n")] = '\0';
  strlcpy(buf, line, READ_SIZE);
  free(line);
  return (lines);
}

/* fread_string() without its exit(): false at the end of the file */
static bool old_fread_string(FILE *fl, std::string &out, bool &null)
{
  char *line = nullptr, *text = nullptr;
  size_t cap = 0, len = 0, n;
  FILE *mem  = open_memstream(&text, &len);
  bool done  = false;

  while (!done) {
    if (getline(&line, &cap, fl) < 0) {
      free(line);
      fclose(mem);
      free(text);
      return false;
    }
    n = strcspn(line, "\r\n");
    if (n > 0 && line[n - 1] == '~') {
      fwrite(line, 1, n - 1, mem);
      done = true;
    } else {
      fwrite(line, 1, n, mem);
      fputs("\r\n", mem);
    }
  }
  free(line);
  fclose(mem);
  null = !len;
  out.assign(text, len);
  free(text);
  return true;
}

static int old_fread_letter(FILE *fp)
{
  int c;
  do {
    c = getc(fp);
  } while (isspace(c));
  ungetc(c, fp);
  return c;
}

static bitvector_t old_asciiflag_conv(const char *flag, int first)
{
  bitvector_t flags = 0;
  int is_num = true;
  for (const char *p = flag; *p; p++) {
    unsigned char c = (unsigned char)*p;
    if (islower(c))
      flags |= 1U << (first + (c - 'a'));
    else if (c >= 'A' && c <= 'F')
      flags |= 1U << (26 + (c - 'A'));
    if (!(isdigit(c) || (c == '-')))
      is_num = false;
  }
  if (is_num)
    flags = atol(flag);
  return flags;
}

/* ---- helpers ---- */

static FILE *file_of(const std::string &text)
{
  FILE *fl = tmpfile();
  fwrite(text.data(), 1, text.size(), fl);
  rewind(fl);
  return fl;
}

static std::string random_text(std::mt19937 &rng, size_t length)
{
  static const char alphabet[] = "ab~ *#\r\n\n\n~0123-+\t\0x";
  std::string s;
  for (size_t i = 0; i < length; i++)
    s += alphabet[rng() % (sizeof(alphabet) - 1)];
  return s;
}

using namespace std::string_literals;

static const std::vector<std::string> awkward = {
    "",
    "\n",
    "one line",
    "one line\n",
    "* comment\n\nreal\n",
    "\r\nafter a blank CR line\r\n",
    "text~\nnext\n",
    "a\nb\n~\n",
    "~\n",
    "no tilde at all\nstill none",
    "inner ~ tilde\nends~",
    "nul\0inside~\nafter\n"s,
    "\0\nstarts with a nul\n"s,
    std::string(400, 'x') + "\nshort\n",
    std::string(300, 'y') + "~\n",
    "   spaced\n\t\tT 300\n",
    "\r",
    "~~\n",
    "last line without a newline~",
};

/* ---- the checks ---- */

/* line() reads what get_line() read, line for line, and counts the lines it passes over */
static void check_lines(const std::string &text)
{
  FILE *fl = file_of(text);
  io::text_reader r("test", text);
  char buf[READ_SIZE];
  for (;;) {
    int advanced = old_get_line(fl, buf);
    int before   = r.line_number();
    std::optional<std::string_view> line = r.line();
    CHECK((advanced > 0) == line.has_value());
    if (!advanced || !line)
      break;
    CHECK(*line == std::string_view(buf));
    CHECK(r.line_number() - before == advanced);
  }
  fclose(fl);

  /* and so does the wrapper on FILE */
  fl  = file_of(text);
  FILE *old = file_of(text);
  char a[READ_SIZE], b[READ_SIZE];
  for (;;) {
    int x = get_line(fl, a), y = old_get_line(old, b);
    CHECK(x == y);
    if (!x || !y)
      break;
    CHECK(!strcmp(a, b));
  }
  fclose(fl);
  fclose(old);
}

/* tilde_string() reads what fread_string() read, string for string, and fails where it ran out */
static void check_strings(const std::string &text)
{
  FILE *fl = file_of(text);
  io::text_reader r("test", text);
  for (;;) {
    std::string want;
    bool null = false;
    bool ok   = old_fread_string(fl, want, null);
    io::loaded<std::string> got = r.tilde_string("the test");
    CHECK(ok == got.has_value());
    if (!ok || !got)
      break;
    CHECK(*got == want);
    CHECK(got->empty() == null);
  }
  fclose(fl);
}

/* peek_letter() finds what fread_letter() found, and leaves the reading where it left it */
static void check_letters(const std::string &text)
{
  FILE *fl = file_of(text);
  io::text_reader r("test", text);
  for (int i = 0; i < 50; i++) {
    int want = old_fread_letter(fl);
    CHECK(r.peek_letter() == want);
    char buf[READ_SIZE];
    int advanced = old_get_line(fl, buf);
    std::optional<std::string_view> line = r.line();
    CHECK((advanced > 0) == line.has_value());
    if (!advanced || !line)
      break;
    CHECK(*line == std::string_view(buf));
  }
  fclose(fl);
}

static void check_reading_awkward_files()
{
  for (const std::string &text : awkward) {
    check_lines(text);
    check_strings(text);
    check_letters(text);
  }
}

static void check_reading_random_files()
{
  std::mt19937 rng(12345);
  for (int i = 0; i < 3000; i++) {
    std::string text = random_text(rng, rng() % 200);
    check_lines(text);
    check_strings(text);
    check_letters(text);
  }
}

static void check_line_numbers()
{
  io::text_reader r("test", "* comment\n\nfirst\nstring\nend~\n\n  T 1\nlast");
  CHECK(r.line() == "first" && r.line_number() == 3);
  CHECK(r.tilde_string("x") == "string\r\nend" && r.line_number() == 5);
  CHECK(r.peek_letter() == 'T' && r.line_number() == 5);
  CHECK(r.line() == "T 1" && r.line_number() == 7);
  CHECK(r.line() == "last" && r.line_number() == 8);
  CHECK(!r.line() && r.at_end());
  io::loaded<std::string> missing = r.tilde_string("object #3");
  CHECK(!missing && missing.error().text() == "test:8: the file ends inside a string, at or near object #3");
}

static void compare_scan(const std::string &text)
{
  int want[6] = {7, 7, 7, 7, 7, 7}, got[6] = {7, 7, 7, 7, 7, 7};
  int n = sscanf(text.c_str(), "%d %d %d %d %d %d", want, want + 1, want + 2, want + 3, want + 4, want + 5);
  int m = io::scan_ints(text, got);
  if (n != m || memcmp(want, got, sizeof(want))) {
    file_print(stdout, "scan_ints('{}'): {:d} [{:d} {:d} {:d}], sscanf {:d} [{:d} {:d} {:d}]\n", text, m, got[0], got[1],
               got[2], n, want[0], want[1], want[2]);
    failures++;
  }
}

static void check_numbers_read_as_sscanf_reads_them()
{
  for (const char *text : {"", " ", "1", "1 2 3", "  -4 +5 6 7 8 9 10", "12abc 3", "a 1", "-", "- 1", "+", "1 -", "1 - 2",
                           "3000000000", "-3000000000", "99999999999999999999", "-99999999999999999999",
                           "2147483647 -2147483648", "\t1\n2", "1,2", "0x10", "007 -0"})
    compare_scan(text);
  std::mt19937 rng(54321);
  static const char alphabet[] = "0123456789 -+x\t";
  for (int i = 0; i < 20000; i++) {
    std::string s;
    for (size_t n = rng() % 30; n; n--)
      s += alphabet[rng() % (sizeof(alphabet) - 1)];
    compare_scan(s);
  }
}

/* io::fields against sscanf() with the formats the loaders read by: a trigger's type line, a zone's 'V' command
   and a zone's header */
static void compare_fields(const std::string &text)
{
  int a = 0, b = 0, c = 0, d = 0, e = 0, f6 = 0, g = 0, h = 0;
  char w1[1024], w2[1024], w3[1024], w4[1024];

  int want = sscanf(text.c_str(), "%d %255s %d", &a, w1, &b);
  io::fields f(text);
  int x = 0, y = 0;
  std::string s1, s2, s3, s4;
  f.number(x) && f.word(s1, 255) && f.number(y);
  if (f.count() != std::max(want, 0) || (want >= 1 && x != a) || (want >= 2 && s1 != w1) || (want >= 3 && y != b)) {
    file_print(stdout, "fields (trigger) differ for '{}'\n", text);
    CHECK(false);
  }

  int n[6];
  want = sscanf(text.c_str(), " %d %d %d %d %d %d %79s %79[^\f\n\r\t\v]", &a, &b, &c, &d, &e, &f6, w1, w2);
  io::fields v(text);
  for (int &i : n)
    v.number(i);
  v.word(s1, 79) && v.until(s2, "\f\n\r\t\v", 79);
  int got[6] = {a, b, c, d, e, f6};
  bool same  = v.count() == std::max(want, 0);
  for (int i = 0; i < std::min(want, 6); i++)
    same = same && n[i] == got[i];
  if (!same || (want >= 7 && s1 != w1) || (want >= 8 && s2 != w2)) {
    file_print(stdout, "fields (V command) differ for '{}'\n", text);
    CHECK(false);
  }

  want = sscanf(text.c_str(), " %d %d %d %d %s %s %s %s %d %d", &a, &b, &c, &d, w1, w2, w3, w4, &g, &h);
  io::fields z(text);
  int m[6];
  z.number(m[0]) && z.number(m[1]) && z.number(m[2]) && z.number(m[3]) && z.word(s1) && z.word(s2) && z.word(s3) &&
      z.word(s4) && z.number(m[4]) && z.number(m[5]);
  int zgot[10]    = {a, b, c, d, 0, 0, 0, 0, g, h};
  const char *ws[] = {w1, w2, w3, w4};
  std::string *ss[] = {&s1, &s2, &s3, &s4};
  same              = z.count() == std::max(want, 0);
  for (int i = 0; i < want; i++)
    if (i < 4)
      same = same && m[i] == zgot[i];
    else if (i < 8)
      same = same && *ss[i - 4] == ws[i - 4];
    else
      same = same && m[i - 4] == zgot[i];
  if (!same) {
    file_print(stdout, "fields (zone header) differ for '{}'\n", text);
    CHECK(false);
  }
  /* a mob's points: the 'd' and '+' are matched as they stand, with no spaces passed over before them */
  int mob[9], want_mob[9];
  want = sscanf(text.c_str(), " %d %d %d %dd%d+%d %dd%d+%d ", want_mob, want_mob + 1, want_mob + 2, want_mob + 3,
                want_mob + 4, want_mob + 5, want_mob + 6, want_mob + 7, want_mob + 8);
  io::fields p(text);
  p.number(mob[0]) && p.number(mob[1]) && p.number(mob[2]) && p.number(mob[3]) && p.literal('d') &&
      p.number(mob[4]) && p.literal('+') && p.number(mob[5]) && p.number(mob[6]) && p.literal('d') &&
      p.number(mob[7]) && p.literal('+') && p.number(mob[8]);
  same = p.count() == std::max(want, 0);
  for (int i = 0; i < want; i++)
    same = same && mob[i] == want_mob[i];
  if (!same) {
    file_print(stdout, "fields (mob points) differ for '{}'\n", text);
    CHECK(false);
  }
}

static void check_fields_read_as_sscanf_reads_them()
{
  for (const char *text : {"", " ", "2 g 100", "2", "2 g", "x g 1", "1 2 3 4 5 6 name value with spaces",
                           "1 2 3 4 5 6 name", "1 2 3 4 5 6", "300 399 5 2 a b c d 0 100", "300 399 5",
                           "1 0 10 0d0+0 0d0+0", "1 0 10 5d6+-7 1 d2+3", "1 0 10 5 d6+7 1d2+3", "1 2 3 4d+5"})
    compare_fields(text);
  std::mt19937 rng(98765);
  static const char alphabet[] = "0123456789 -+abdZ~\t\v";
  for (int i = 0; i < 20000; i++) {
    std::string s;
    for (size_t n = rng() % 200; n; n--)
      s += alphabet[rng() % (sizeof(alphabet) - 1)];
    compare_fields(s);
  }
}

/* scan_float() reads what sscanf("%f") reads */
static void check_floats_read_as_sscanf_reads_them()
{
  std::mt19937 rng(4242);
  static const char alphabet[] = "0123456789 -+.eExinfa\t";
  std::vector<std::string> texts = {"", " ", "1.00", "0.75", " -2.5", "1e3", "1e", "1e+", ".", "-.5", "inf", "nan",
                                    "0x1p3", "x", "3.25 junk"};
  for (int i = 0; i < 20000; i++) {
    std::string s;
    for (size_t n = rng() % 12; n; n--)
      s += alphabet[rng() % (sizeof(alphabet) - 1)];
    texts.push_back(s);
  }
  for (const std::string &text : texts) {
    float want = 7.0f, got = 7.0f;
    int want_n = sscanf(text.c_str(), "%f", &want);
    int got_n  = io::scan_float(text, got);
    if (want_n != got_n || (want_n == 1 && memcmp(&want, &got, sizeof(float)) != 0 && !(want != want && got != got))) {
      file_print(stdout, "scan_float differs for '{}': {:d} {} against {:d} {}\n", text, want_n, want, got_n, got);
      CHECK(false);
    }
  }
}

static void check_flags_read_as_asciiflag_conv_reads_them()
{
  for (const char *word : {"0", "abc", "aD", "zF", "12", "-1", "4294967296", "99999999999999999999", "a1", "1-2", "",
                           "abcdefghijklmnopqrstuvwxyzABCDEF", "-"})
    for (int first : {0, 1}) {
      bitvector_t want = old_asciiflag_conv(word, first);
      CHECK(io::ascii_flags(word, first, "test") == want);
    }
}

static void check_player_files()
{
  CHECK(io::player_file(PLR_FILE, "Kelvarin") == "plrfiles/K-O/kelvarin.plr");
  CHECK(io::player_file(CRASH_FILE, "Bob") == "plrobjs/A-E/bob.new");
  CHECK(io::player_file(USER_FILE, "zed") == "user/U-Z/zed.usr");
  CHECK(io::player_file(ALIAS_FILE, "1abc") == "plralias/ZZZ/1abc.alias");
  CHECK(!io::player_file(PLR_FILE, ""));
  CHECK(!io::player_file(7, "Bob")); /* no such kind */
  char cut[16];
  CHECK(get_filename(cut, sizeof(cut), PLR_FILE, "Kelvarin") && !strcmp(cut, "plrfiles/K-O/ke")); /* quirk: cut */
}

static void check_replacing_files()
{
  namespace fs = std::filesystem;
  fs::path dir = fs::temp_directory_path() / str_format("dbat-test-io-{:d}", getpid());
  fs::create_directories(dir);
  std::string path = (dir / "file").string();

  CHECK(io::replace_file(path, "first\n", true));
  CHECK(!fs::exists(path + ".bak") && !fs::exists(path + ".tmp"));
  io::text_writer out(path, true);
  out.print("second {:d}\n", 2);
  CHECK(out.commit());
  CHECK(io::text_reader::open(path)->line() == "second 2");
  CHECK(io::text_reader::open(path + ".bak")->line() == "first");
  CHECK(!io::replace_file((dir / "no-such-dir" / "file").string(), "x", false)); /* logged, nothing made */

  CHECK(io::ensure_directory(dir / "a" / "b") && fs::is_directory(dir / "a" / "b"));
  io::touch(dir / "c");
  auto names = io::list_directory(dir);
  CHECK(names && *names == (std::vector<std::string>{"a", "c", "file", "file.bak"}));
  CHECK(!io::list_directory(dir / "none"));

  io::replace_file((dir / "index").string(), "3.wld\n4.wld\n$\nafter\n", false);
  auto index = io::read_index(dir / "index");
  CHECK(index && *index == (std::vector<std::string>{"3.wld", "4.wld"}));
  io::replace_file((dir / "index").string(), "  5.wld   6.wld", false); /* no "$": logged, read to the end */
  index = io::read_index(dir / "index");
  CHECK(index && *index == (std::vector<std::string>{"5.wld", "6.wld"}));
  CHECK(!io::read_index(dir / "none") && io::read_index(dir / "none").error().text().ends_with(": can't be opened"));
  fs::remove_all(dir);
}

int main()
{
  check_reading_awkward_files();
  check_reading_random_files();
  check_line_numbers();
  check_numbers_read_as_sscanf_reads_them();
  check_fields_read_as_sscanf_reads_them();
  check_floats_read_as_sscanf_reads_them();
  check_flags_read_as_asciiflag_conv_reads_them();
  check_player_files();
  check_replacing_files();
  if (failures)
    file_print(stdout, "{:d} check(s) failed\n", failures);
  else
    file_print(stdout, "all checks passed\n");
  return failures != 0;
}
