/*
 * Reading the old binary hcontrol and plrmail files: see legacy_binary.hpp.
 */
#include <cstdint>
#include <cstring>
#include <string>

#include "legacy_binary.hpp"
#include "dbat/log.hpp"

namespace {

/* a little-endian signed number of n bytes at at */
int64_t le(std::string_view bytes, size_t at, size_t n)
{
  uint64_t v = 0;
  for (size_t i = 0; i < n; i++)
    v |= static_cast<uint64_t>(static_cast<unsigned char>(bytes[at + i])) << (8 * i);
  if (n < 8 && (v >> (8 * n - 1)) & 1) /* sign-extend */
    v |= ~uint64_t{0} << (8 * n);
  return static_cast<int64_t>(v);
}

/* a C string of at most n bytes at at */
std::string c_string(std::string_view bytes, size_t at, size_t n)
{
  std::string_view s = bytes.substr(at, n);
  return std::string(s.substr(0, s.find('\0')));
}

/* The old mail blocks: a header block holds the letter's start; a data block, more of it */
constexpr int64_t header_block = -1, last_block = -2;
constexpr size_t header_text   = 256 - 8 - 32 - 1; /* HEADER_BLOCK_DATASIZE */
constexpr size_t data_text     = 256 - 8 - 1;      /* DATA_BLOCK_DATASIZE */

} // namespace

namespace legacy {

bool is_binary(std::string_view text, std::string_view header, size_t record_size)
{
  return !text.starts_with(header) && text.size() % record_size == 0;
}

/*
 * A house_control_rec, as the x86-64 build laid it out:
 *   0 vnum (4)  4 atrium (4)  8 exit_num (2)  16 built_on (8)  24 mode (4)  32 owner (8)  40 num_of_guests (4)
 *   48 guests (10 x 8)  128 last_payment (8)  136 bitvector  144 builtby  152-192 spare2 to spare7 (8 each)
 */
std::vector<house_control_rec> houses(std::string_view bytes)
{
  std::vector<house_control_rec> out;
  for (size_t at = 0; at + house_record_size <= bytes.size(); at += house_record_size) {
    house_control_rec h{};
    h.vnum          = static_cast<room_vnum>(le(bytes, at + 0, 4));
    h.atrium        = static_cast<room_vnum>(le(bytes, at + 4, 4));
    h.exit_num      = static_cast<int16_t>(le(bytes, at + 8, 2));
    h.built_on      = le(bytes, at + 16, 8);
    h.mode          = static_cast<int>(le(bytes, at + 24, 4));
    h.owner         = le(bytes, at + 32, 8);
    h.num_of_guests = static_cast<int>(le(bytes, at + 40, 4));
    for (int i = 0; i < MAX_GUESTS; i++)
      h.guests[i] = le(bytes, at + 48 + 8 * static_cast<size_t>(i), 8);
    h.last_payment = le(bytes, at + 128, 8);
    long *rest[]   = {&h.bitvector, &h.builtby, &h.spare2, &h.spare3, &h.spare4, &h.spare5, &h.spare6, &h.spare7};
    for (size_t i = 0; i < std::size(rest); i++)
      *rest[i] = le(bytes, at + 136 + 8 * i, 8);
    out.push_back(h);
  }
  return out;
}

/*
 * A header block: 0 block_type (-1)  8 next_block  16 from  24 to  32 mail_time  40 text (216, NUL-ended).
 * A data block: 0 block_type (the next block's place, or -2 for the last)  8 text (248, NUL-ended).
 */
std::vector<mail_letter> letters(std::string_view bytes)
{
  std::vector<mail_letter> out;
  for (size_t at = 0; at + mail_block_size <= bytes.size(); at += mail_block_size) {
    if (le(bytes, at, 8) != header_block)
      continue;
    mail_letter m;
    m.to   = le(bytes, at + 24, 8); /* long and time_t, whatever their width */
    m.from = le(bytes, at + 16, 8);
    m.time = le(bytes, at + 32, 8);
    m.text = c_string(bytes, at + 40, header_text);
    int64_t next = le(bytes, at + 8, 8);
    for (int guard = 0; next != last_block; guard++) {
      if (next < 0 || next % static_cast<int64_t>(mail_block_size) ||
          static_cast<size_t>(next) + mail_block_size > bytes.size() || guard >= 1000) {
        mud_log("SYSERR: a letter to {:d} in the old mail file links to a block it hasn't ({:d}); the rest of it is lost",
                m.to, next);
        break;
      }
      size_t block = static_cast<size_t>(next);
      m.text += c_string(bytes, block + 8, data_text);
      next = le(bytes, block, 8);
    }
    out.push_back(std::move(m));
  }
  return out;
}

} // namespace legacy
