#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include "dbat/io/load_error.hpp"
#include "dbat/consts/types.hpp"
#include "dbat/flag_set.hpp"
#include "dbat/name_table.hpp"

constexpr int MAX_HOUSES = 1000;
constexpr int MAX_GUESTS = 10;

constexpr int HOUSE_PRIVATE = 0;
constexpr int HOUSE_GOD     = 1; /* Imm owned house             */
constexpr int HOUSE_CLAN    = 2; /* Clan crash-save room        */
constexpr int HOUSE_UNOWNED = 3;

constexpr int NUM_HOUSE_TYPES = 4;

/* A house's flags (house_control_rec::bitvector) */
extern const name_table house_bits; /* HOUSE_NUM_FLAGS of them */
enum class house_flag : uint8_t {
  noguests   = 0, /* Owner cannot add guests     */
  free       = 1, /* House does not require payments */
  noimms     = 2, /* Imms below level 999 cannot enter */
  imponly    = 3, /* Imms below level 1000 cannot enter */
  rentfree   = 4, /* No rent is charged on items left here */
  savenorent = 5, /* NORENT items are crashsaved too */
  nosave     = 6, /* Do not crash save this room - private only */
  count_     = 7,
};
template <> struct flag_traits<house_flag> {
  static constexpr const name_table &names = house_bits;
};

constexpr int HOUSE_NUM_FLAGS = static_cast<int>(flag_count<house_flag>);

/* The flags as the files save them, a long (all its bits, in words 0 and 1: a long is 32 bits on Windows), and back */
constexpr long house_flags_number(const flag_set<house_flag> &flags)
{
  return static_cast<long>(static_cast<uint64_t>(flags.word(0)) | static_cast<uint64_t>(flags.word(1)) << 32);
}
constexpr flag_set<house_flag> house_flags_from_number(long n)
{
  const auto bits = static_cast<uint64_t>(static_cast<int64_t>(n));
  flag_set<house_flag> flags;
  flags.set_word(0, static_cast<bitvector_t>(bits & 0xffffffffu));
  flags.set_word(1, static_cast<bitvector_t>(bits >> 32));
  return flags;
}

struct house_control_rec {
  room_vnum vnum{};          /* vnum of this house		*/
  room_vnum atrium{};        /* vnum of atrium		*/
  int16_t exit_num{};        /* direction of house's exit	*/
  time_t built_on{};         /* date this house was built	*/
  int mode{};                /* mode of ownership		*/
  long owner{};              /* idnum of house's owner	*/
  int num_of_guests{};       /* how many guests for house	*/
  std::array<long, MAX_GUESTS> guests{}; /* idnums of house's guests	*/
  time_t last_payment{};     /* date of last house payment   */
  flag_set<house_flag> bitvector;
  long builtby{};
  long spare2{};
  long spare3{};
  long spare4{};
  long spare5{};
  long spare6{};
  long spare7{};
};

/* Loads the houses at boot; an error if etc/hcontrol can't be read */
io::loaded<void> House_boot(GameWorld *g);
void House_save_all(GameWorld *g);
bool House_can_enter(GameWorld *g, struct char_data *ch, room_vnum house);
void hcontrol_list_houses(GameWorld *g, struct char_data *ch);
int find_house(GameWorld *g, room_vnum vnum);         /* house.c */
void House_save_control(GameWorld *g);

/* What etc/hcontrol holds: every house in it, before any is checked */
struct house_control_file {
  std::vector<house_control_rec> houses;
  bool missing{};   /* there is no file */
  bool converted{}; /* it was the old binary file */
};
io::loaded<house_control_file> read_house_control();
void House_delete_file(room_vnum vnum); /* house.c */

ACMD(do_hcontrol);
ACMD(do_house);

