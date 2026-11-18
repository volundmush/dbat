/*
 * Journeys through the OLC editors, to compare two builds:
 *
 *   dbat_test_olc [--transcript FILE] [--shard K/N] [PREFIX]
 *
 * A journey is a builder, made fresh in the fixture world, typing lines: commands that open the editors, then what
 * each menu asks for. Lines starting with @ are the journey's own doings:
 *   @2 LINE    the second builder (made on first use, an implementor) types LINE
 *   @pulse N   runs N pulses of the game
 * Each journey runs in its own process, on its own copy of the world, from the same fresh boot, with its own seed and
 * the clock stopped. After every line the transcript has what the builder was sent and the state its connection is
 * in, and what a watcher sees: an implementor standing by, who sees the game's log at its fullest; at the end, every file the journey changed, in full, and who and what is in the fixture's rooms.
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "dbat/comm.hpp"
#include "dbat/consts/admlevel.hpp"
#include "dbat/consts/constates.hpp"
#include "dbat/consts/itemdata.hpp"
#include "dbat/consts/prefflags.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/db.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/handler.hpp"
#include "dbat/iterate.hpp"
#include "dbat/relocate.hpp"
#include "dbat/random.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/utils.hpp"
#include "dbat/world_audit.hpp"
#include "harness.hpp"

namespace {
  constexpr time_t TRANSCRIPT_TIME = 1700000000; /* the clock, stopped */
  constexpr int CASE_SECONDS       = 120;
  constexpr size_t SHOWN_IN_FULL   = 65536; /* longer changed files are shown by their hash */

  FILE *transcript;
  int cases, failures;

  using setup_fn = void (*)(GameWorld *g, struct char_data *ch);

  struct journey {
    std::string name;
    std::vector<std::string> lines;
    int admlevel   = ADMLVL_IMPL;
    setup_fn setup = nullptr; /* after the builder is made */
  };

  /* An implementor's journey */
  journey impl(std::string name, std::vector<std::string> lines, setup_fn setup = nullptr)
  {
    return {name, std::move(lines), ADMLVL_IMPL, setup};
  }

  /* Another admin level's */
  journey as(int admlevel, std::string name, std::vector<std::string> lines, setup_fn setup = nullptr)
  {
    return {name, std::move(lines), admlevel, setup};
  }

  void zone_3(GameWorld *g, struct char_data *ch) { char_set_olc_zone(g, ch, 3); }
  void zone_4(GameWorld *g, struct char_data *ch) { char_set_olc_zone(g, ch, 4); }

  /* With RPP and zenni to spend */
  void rich(GameWorld *g, struct char_data *ch)
  {
    char_set_rp(ch, 400);
    char_set_gold(ch, 100000);
  }

  /* A mortal holding the remodeling droid, in a room that can be remodeled, or not */
  void droid_only(GameWorld *g, struct char_data *ch) { obj_to_char(g, read_object(g, 19094), ch); }
  void with_droid(GameWorld *g, struct char_data *ch)
  {
    droid_only(g, ch);
    room_set_flag(g, char_get_in_room(ch), ROOM_CANREMODEL, true);
  }
  void remodelable(GameWorld *g, struct char_data *ch) { room_set_flag(g, char_get_in_room(ch), ROOM_CANREMODEL, true); }

  std::vector<std::string> then(std::vector<std::string> first, const std::vector<std::string> &rest)
  {
    first.insert(first.end(), rest.begin(), rest.end());
    return first;
  }

  /* Each item type's values, from a new object, answered one way and then another */
  void every_item_type(std::vector<journey> &out)
  {
    const std::vector<std::string> mild = {"3", "1", "0", "2", "5", "4", "3", "2"};
    const std::vector<std::string> wild = {"-1", "99", "-7", "99999", "0", "-200", "50", "40"};
    for (int type = 1; type < NUM_ITEM_TYPES; type++) {
      out.push_back(impl(str_format("oedit, values of type {:d}", type), then({"oedit 303", "5", std::to_string(type), "c"}, then(mild, {"/s", "q", "y"}))));
      out.push_back(impl(str_format("oedit, wild values of type {:d}", type), then({"oedit 303", "5", std::to_string(type), "c"}, then(wild, {"/s", "q", "y"}))));
    }
  }

  std::vector<journey> journeys()
  {
    std::vector<journey> out = {
      /* ---- redit ---- */
      impl("redit, opening and quitting", {"redit", "q", "redit 300", "q"}),
      impl("redit, refusals", {"redit save", "redit nonsense", "redit 99999", "redit save 99999"}),
      as(ADMLVL_BUILDER, "redit, a builder of no zone", {"redit 300", "redit save 3"}),
      as(ADMLVL_BUILDER, "redit, a builder of the zone", {"redit 300", "1", "Builder's room", "q", "y"}, zone_3),
      as(ADMLVL_BUILDER, "redit, a builder of another zone", {"redit 300"}, zone_4),
      as(ADMLVL_NONE, "redit, a mortal without the droid", {"program", "redit"}, remodelable),
      as(ADMLVL_NONE, "redit, a mortal remodeling", {"program", "1", "A remodeled room", "2", "Remodeled.", "/s", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f", "g", "w", "x", "s", "z hello", "h", "1", "thing", "2", "A thing.", "/s", "0", "q", "y"}, with_droid),
      as(ADMLVL_NONE, "redit, a mortal remodeling, not saving", {"program", "1", "A remodeled room", "q", "n", "i"}, with_droid),
      as(ADMLVL_NONE, "redit, a mortal remodeling a room that can't be", {"program"}, droid_only),
      impl("redit, saving a zone", {"redit save 3", "redit save", "zedit 3", "q", "redit save"}),
      impl("redit, the name", {"redit 300", "1", "", "1", "A new name~", "1", "A name long enough to be cut off at the hundredth character: 1234567890123456789012345678901234567890123456789", "q", "y"}),
      impl("redit, the description", {"redit 300", "2", "/c", "A new description.", "/l", "/n", "Another line.", "/s", "q", "y"}),
      impl("redit, the description aborted", {"redit 300", "2", "Not this.", "/a", "q"}),
      impl("redit, the flags", {"redit 300", "3", "1", "2", "5", "99", "-1", "abc", "1", "0", "q", "y"}),
      impl("redit, the sector", {"redit 300", "4", "99", "-1", "3", "q", "y"}),
      impl("redit, a new exit", {"redit 300", "5", "1", "99999", "301", "3", "door gate", "4", "-5", "4", "300", "5", "1", "2", "9", "0", "7", "15", "8", "-3", "9", "nonsense", "9", "kick", "a", "12", "b", "2", "c", "-1", "d", "301", "e", "-4", "z", "0", "q", "y"}),
      impl("redit, every exit", {"redit 300", "5", "0", "6", "0", "7", "0", "8", "0", "9", "0", "a", "0", "b", "0", "c", "0", "d", "0", "e", "0", "f", "0", "g", "0", "A", "0", "G", "0", "q"}),
      impl("redit, an exit's description", {"redit 300", "5", "2", "A gate.", "/s", "0", "q", "y"}),
      impl("redit, purging an exit", {"redit 300", "5", "1", "-1", "6", "q", "y"}),
      impl("redit, the extra descriptions", {"redit 300", "h", "3", "1", "", "1", "sign", "3", "2", "A sign.", "/s", "3", "1", "plaque", "2", "A plaque.", "/s", "3", "0", "h", "3", "3", "0", "q", "y"}),
      impl("redit, an extra description left unfinished", {"redit 300", "h", "1", "half", "0", "h", "9", "0", "q", "y"}),
      impl("redit, copy and wiznet", {"redit 300", "w", "W", "z  hello builders", "Z", "q"}),
      impl("redit, nonsense choices", {"redit 300", "?", "", "0", "q"}),
      impl("redit, deleting a room, not", {"redit 305", "x", "maybe", "n", "q"}),
      impl("redit, deleting a room", {"redit 305", "X", "y", "redit 305"}),
      impl("redit, a new room", {"redit 306", "1", "A brand new room", "q", "y", "redit 306", "q", "goto 306", "look"}),
      impl("redit, a new room not saved", {"redit 307", "1", "Never", "q", "n", "goto 307"}),
      impl("redit, invalid save answer", {"redit 300", "1", "Changed", "q", "maybe", "", "Y"}),
      impl("redit, the script editor", {"redit 300", "s", "?", "n", "1, 99999", "1, 301", "n", "300", "n", "3, 300", "n", "0, 300", "n", "1, 0", "n", "abc", "d", "9", "d", "0", "d", "abc", "d", "1", "x", "S", "X", "q", "y"}),
      impl("redit, a mismatched trigger", {"redit 300", "s", "n", "1, 310", "x", "q", "y"}),
      impl("redit, already being edited", {"redit 300", "@2 redit 300", "q", "@2 redit 300", "@2 q"}),
      impl("redit, the room you are in", {"goto 301", "redit", "q"}),
      /* ---- trigedit ---- */
      impl("trigedit, refusals", {"trigedit", "trigedit abc", "trigedit 99999"}),
      as(ADMLVL_BUILDER, "trigedit, a builder of no zone", {"trigedit 300"}),
      impl("trigedit, opening and quitting", {"trigedit 300", "?", "q"}),
      impl("trigedit, every field", {"trigedit 300", "1", "Renamed~trigger", "1", "", "2", "1", "2", "7", "2", "-3", "3", "1", "3", "99", "-1", "abc", "0", "4", "50", "4", "500", "4", "-5", "5", "an argument~", "6", "/c", "say hello", "if %actor.is_pc%", "  wait 1", "end", "/s", "q", "y"}),
      impl("trigedit, the script's text editor", {"trigedit 301", "6", "/h", "/l", "/n", "/l 1-2", "/n 2", "/d 1", "/i 1 inserted", "/e 1 edited", "/r 'a' 'b'", "/ra 'e' 'E'", "/f", "/fi", "/x", "/", "/s", "q", "n"}),
      impl("trigedit, a new trigger", {"trigedit 302", "1", "Fresh", "3", "2", "0", "5", "hi", "6", "say hi", "/s", "q", "y", "trigedit 302", "q", "tlist 3"}),
      impl("trigedit, a new trigger without a type", {"trigedit 303", "1", "Typeless", "q", "a", "q", "y"}),
      impl("trigedit, copying", {"trigedit 304", "w", "99999", "w", "300", "q", "y"}),
      impl("trigedit, wiznet and the save prompt", {"trigedit 300", "z hello", "1", "Changed", "q", "maybe", "n"}),
      impl("trigedit, already being edited", {"trigedit 300", "@2 trigedit 300", "q"}),
      impl("trigedit, saving and attaching", {"trigedit 300", "1", "Saved name", "q", "y", "redit 300", "s", "n", "1, 300", "x", "q", "y", "@pulse 5"}),
      /* ---- oedit ---- */
      impl("oedit, refusals", {"oedit", "oedit nonsense", "oedit 99999", "oedit save 99999", "oedit save"}),
      as(ADMLVL_BUILDER, "oedit, a builder of no zone", {"oedit 300", "oedit save 3"}),
      as(ADMLVL_BUILDER, "oedit, a builder of the zone", {"oedit 300", "2", "a builder's thing", "q", "y", "oedit save"}, zone_3),
      impl("oedit, saving a zone", {"oedit save 3"}),
      impl("oedit, opening and quitting", {"oedit 300", "?", "q"}),
      impl("oedit, the strings", {"oedit 300", "1", "", "1", "thing new", "2", "a new thing", "3", "A new thing lies here.", "4", "You use it.", "/s", "4", "/a", "q", "y"}),
      impl("oedit, the flags", {"oedit 300", "6", "1", "27", "999", "-1", "0", "7", "1", "14", "99", "-1", "0", "p", "1", "11", "999", "0", "q", "y"}),
      impl("oedit, the numbers", {"oedit 300", "8", "-5", "9", "99999999", "a", "250", "b", "60", "m", "-3", "m", "20", "n", "3", "n", "999", "y", "3", "y", "0", "y", "99", "q", "y"}),
      impl("oedit, a portal's timer", {"oedit 300", "5", "28", "b", "-1", "q", "y"}),
      impl("oedit, the type refused", {"oedit 300", "5", "0", "99", "abc", "12", "q", "y"}),
      impl("oedit, the applies", {"oedit 300", "d", "9", "-1", "1", "999", "-1", "1", "5", "1", "2", "5", "3", "1", "0", "2", "0", "q", "y"}),
      impl("oedit, a skill and a feat apply", {"oedit 300", "d", "1", "41", "5", "kick", "2", "42", "1", "alertness", "3", "41", "2", "12", "4", "42", "3", "7", "0", "q", "y"}),
      as(ADMLVL_GOD, "oedit, an apply twice, below a greater god", {"oedit 300", "d", "1", "1", "5", "2", "1", "0", "0", "q", "y"}, zone_3),
      impl("oedit, the spellbook", {"oedit 300", "t", "99", "-1", "1", "999", "-1", "1", "2", "1", "0", "1", "0", "0", "q", "y"}),
      impl("oedit, the extra descriptions", {"oedit 300", "e", "3", "9", "1", "", "1", "mark", "2", "A mark.", "/s", "3", "1", "rune", "2", "A rune.", "/s", "0", "e", "3", "0", "q", "y"}),
      impl("oedit, an extra description left unfinished", {"oedit 300", "e", "1", "half", "0", "q", "y"}),
      impl("oedit, the script editor", {"oedit 300", "s", "n", "1, 340", "x", "q", "y"}),
      impl("oedit, copy", {"oedit 303", "w", "99999", "w", "300", "q", "y", "oedit 303", "q"}),
      impl("oedit, wiznet and the save prompt", {"oedit 300", "z hello", "Z", "1", "renamed", "q", "maybe", "a", "q", "N"}),
      impl("oedit, deleting an object, not", {"oedit 302", "x", "maybe", "n", "q"}),
      impl("oedit, deleting an object", {"oedit 302", "X", "Y", "oedit 302"}),
      impl("oedit, a new object", {"oedit 304", "1", "gizmo", "2", "a gizmo", "q", "y", "load obj 304", "i"}),
      impl("oedit, a new board", {"oedit 305", "5", "30", "q", "y", "oedit 305", "1", "board", "q", "y"}),
      impl("oedit, already being edited", {"oedit 300", "@2 oedit 300", "q"}),
      /* ---- zedit ---- */
      impl("zedit, refusals", {"zedit nonsense", "zedit new", "zedit new 5", "zedit 99999", "zedit save 99999"}),
      as(ADMLVL_BUILDER, "zedit, a builder of no zone", {"zedit", "zedit save 3", "zedit new 5 500 599"}),
      as(ADMLVL_BUILDER, "zedit, a builder of the zone", {"zedit", "1", "a", "Builder zone", "b", "t", "q", "y"}, zone_3),
      impl("zedit, saving a zone", {"zedit save 3", "zedit save"}),
      impl("zedit, opening and quitting", {"zedit", "?", "q", "zedit 301", "q"}),
      impl("zedit, the zone's fields", {"zedit", "1", "builder someone", "a", "", "a", "Renamed Zone", "l", "abc", "l", "999", "l", "45", "r", "7", "r", "1", "m", "-1", "m", "5", "x", "999", "x", "abc", "x", "90", "f", "1", "2", "999", "-1", "0", "z hello", "q", "maybe", "y"}),
      impl("zedit, the zone's range", {"zedit", "b", "350", "b", "290", "b", "300", "t", "320", "t", "5000", "t", "450", "q", "y"}),
      impl("zedit, loading a mob and its things", {"zedit", "n", "abc", "n", "1", "x", "m", "maybe", "n", "abc", "99999", "300", "abc", "3", "", "150", "50", "n", "2", "g", "y", "301", "1", "10", "n", "3", "e", "y", "301", "1", "99", "16", "5", "q", "y"}),
      impl("zedit, objects and doors", {"zedit", "n", "1", "o", "n", "300", "2", "2", "0", "n", "2", "p", "y", "301", "1", "99999", "300", "0", "n", "1", "d", "n", "x", "99", "0", "5", "1", "n", "1", "r", "n", "99999", "300", "q", "y"}),
      impl("zedit, triggers and globals", {"zedit", "n", "1", "t", "9", "2", "99999", "301", "n", "1", "v", "0", "0", "", "power", "", "9000", "n", "0", "t", "1", "340", "q", "y"}),
      impl("zedit, changing and deleting commands", {"zedit", "e", "abc", "e", "99", "e", "1", "o", "n", "300", "1", "1", "1", "e", "a", "d", "abc", "d", "99", "d", "1", "d", "0", "q", "n"}),
      impl("zedit, a new command at the top", {"zedit", "n", "0", "m", "300", "1", "1", "1", "q", "y"}),
      impl("zedit, no changes", {"zedit", "q"}),
      impl("zedit, a new zone", {"zedit new 5 500 599", "zedit new 5 500 599", "zedit new 6 600 550", "zedit new 1000 100000 100099", "redit 500", "q", "zlist"}),
      impl("zedit, already being edited", {"zedit", "@2 zedit 300", "q"}),
      /* ---- sedit ---- */
      impl("sedit, refusals", {"sedit", "sedit nonsense", "sedit 99999", "sedit save 99999", "sedit save"}),
      as(ADMLVL_BUILDER, "sedit, a builder of no zone", {"sedit 300", "sedit save 3"}),
      as(ADMLVL_BUILDER, "sedit, a builder of the zone", {"sedit 300", "1", "3", "q", "y", "sedit save"}, zone_3),
      impl("sedit, saving a zone", {"sedit save 3"}),
      impl("sedit, opening and quitting", {"sedit 300", "?", "q"}),
      impl("sedit, the keeper and hours", {"sedit 300", "0", "abc", "0", "99999", "-1", "0", "300", "1", "6", "2", "40", "3", "-2", "4", "20", "5", "1.5", "5", "9", "6", "0.25", "6", "-1", "q", "y"}),
      impl("sedit, the messages", {"sedit 300", "7", "Not here", "8", "%s %s two names", "8", "%s You have none.", "9", "%d first %s", "a", "%s Too poor %q", "b", "%s 100%", "c", "%s That is %d zenni.", "d", "%s %d zenni for it, %d twice", "7", "", "q", "y"}),
      impl("sedit, the flags", {"sedit 300", "e", "1", "3", "999", "0", "f", "1", "2", "999", "-1", "q", "y"}),
      impl("sedit, the rooms", {"sedit 300", "r", "a", "99999", "301", "a", "302", "c", "l", "d", "abc", "d", "9", "d", "0", "x", "q", "q", "y"}),
      impl("sedit, the products", {"sedit 300", "p", "a", "99999", "301", "a", "-1", "a", "abc", "d", "9", "d", "0", "d", "0", "x", "q", "q", "y"}),
      impl("sedit, the types", {"sedit 300", "t", "a", "99", "sword blade", "a", "5", "", "d", "abc", "d", "9", "d", "0", "x", "q", "y"}),
      impl("sedit, copy", {"sedit 303", "w", "99999", "w", "300", "q", "y", "sedit 303", "q"}),
      impl("sedit, the save prompt", {"sedit 300", "1", "4", "q", "maybe", "n"}),
      impl("sedit, a new shop", {"sedit 304", "0", "300", "q", "y", "slist 3"}),
      impl("sedit, already being edited", {"sedit 300", "@2 sedit 300", "q"}),
      /* ---- gedit ---- */
      impl("gedit, refusals", {"gedit", "gedit nonsense", "gedit 99999", "gedit save 99999", "gedit save"}),
      as(ADMLVL_BUILDER, "gedit, a builder of no zone", {"gedit 300", "gedit save 3"}),
      as(ADMLVL_BUILDER, "gedit, a builder of the zone", {"gedit 300", "3", "2", "q", "y", "gedit save"}, zone_3),
      impl("gedit, saving a zone", {"gedit save 3"}),
      impl("gedit, opening and quitting", {"gedit 300", "?", "q"}),
      impl("gedit, the fields", {"gedit 300", "0", "abc", "0", "99999", "-1", "0", "300", "1", "%s I can't teach that.", "1", "%s %s twice", "2", "%d first %s", "2", "%s Bring zenni.", "3", "-1", "3", "4", "4", "30", "5", "2.5", "6", "abc", "6", "10", "7", "1", "5", "999", "-1", "0", "q", "maybe", "y"}),
      impl("gedit, the skills and feats", {"gedit 300", "9", "1", "407", "abc", "-5", "99999", "0", "8", "1", "3", "99999", "0", "b", "1", "0", "a", "A", "q", "y"}),
      impl("gedit, a new guild", {"gedit 304", "0", "300", "q", "y", "glist 3"}),
      impl("gedit, already being edited", {"gedit 300", "@2 gedit 300", "q"}),
      /* ---- hsedit ---- */
      impl("hsedit, refusals", {"hsedit", "q", "hsedit nonsense", "hsedit 99999", "hsedit save 99999", "hsedit save"}),
      as(ADMLVL_BUILDER, "hsedit, a builder of no zone", {"hsedit 300", "hsedit save 3"}),
      impl("hsedit, saving a zone", {"hsedit save 3"}),
      impl("hsedit, a new house", {"hsedit 300", "?", "1", "1", "nobody", "1", "1", "watcher", "1", "2", "99999", "1", "2", "1", "1", "Q", "2", "99999", "0", "2", "302", "2", "301", "3", "99", "1", "3", "q", "3", "4", "4", "99", "4", "2", "5", "maybe", "y", "5", "n", "6", "maybe", "y", "6", "n", "8", "1", "999", "0", "q", "maybe", "y", "hcontrol"}),
      impl("hsedit, the guests", {"hsedit 300", "1", "1", "watcher", "7", "a", "nobody", "a", "builder", "a", "second", "@2 look", "a", "second", "a", "second", "d", "nobody", "d", "builder", "d", "watcher", "d", "second", "c", "maybe", "n", "c", "y", "x", "q", "q", "y"}),
      impl("hsedit, deleting a house", {"hsedit 300", "1", "1", "watcher", "q", "y", "hsedit 300", "x", "maybe", "n", "x", "y", "hsedit 300", "q", "n"}),
      impl("hsedit, the type", {"hsedit 300", "4", "99", "4", "abc", "4", "2", "q", "y"}),
      impl("hsedit, quitting unchanged", {"hsedit 300", "q"}),
      impl("hsedit, already being edited", {"hsedit 300", "@2 hsedit 300", "q"}),
      /* ---- aedit ---- */
      impl("aedit, refusals", {"aedit", "aedit north", "aedit save"}),
      as(ADMLVL_IMMORT, "aedit, without access", {"aedit nod"}),
      impl("aedit, editing a social", {"aedit nod", "maybe", "y", "?", "1", "", "1", "nods", "2", "", "2", "99", "2", "5", "3", "4", "4", "", "4", "-1", "4", "3", "5", "a", "You nod.", "b", "$n nods.", "c", "Nod at whom?", "d", "You nod to yourself.", "e", "$n nods to $mself.", "f", "You nod at $N.", "g", "$n nods at $N.", "h", "$n nods at you.", "i", "You nod at $N's #.", "j", "$n nods at $N's #.", "k", "$n nods at your #.", "l", "You nod at $p.", "m", "$n nods at $p.", "n", "", "n", "two words", "n", "nodnod", "q", "maybe", "y"}),
      impl("aedit, emptying the messages", {"aedit nod", "y", "a", "", "b", "", "c", "", "d", "", "e", "", "f", "", "g", "", "h", "", "i", "", "j", "", "k", "", "l", "", "m", "", "a", "b", "q", "y", "astat", "astat nonsense", "astat nod", "nod", "nod watcher"}),
      impl("astat", {"astat nod", "astat no", "astat"}),
      impl("aedit, not editing it", {"aedit nod", "n", "aedit no", "q"}),
      impl("aedit, a new social", {"aedit newsocial", "maybe", "y", "q", "y", "newsocial", "aedit newsoc", "y", "q"}),
      impl("aedit, a new social not added", {"aedit brandnew", "n", "aedit brandnew", "q"}),
      impl("aedit, already being edited", {"aedit nod", "@2 aedit nod", "y", "q", "n"}),
      /* ---- hedit ---- */
      impl("hedit, refusals", {"hedit", "hedit save"}),
      impl("hedit, editing an entry", {"hedit fixture", "maybe", "y", "?", "1", "", "1", "<<X<< no", "1", "FIXTURE TESTING", "2", "More help.", "/s", "3", "abc", "3", "999", "3", "5", "q", "maybe", "y", "help fixture"}),
      impl("hedit, a new entry", {"hedit newhelp", "maybe", "y", "q", "y", "1", "NEWHELP", "2", "Brand new.", "/s", "q", "y", "help newhelp"}),
      impl("hedit, a new entry not added", {"hedit another", "n", "hedit another", "q"}),
      impl("hedit, not editing it", {"hedit fix", "n", "hedit fix", "q"}),
      impl("hedit, no changes", {"hedit fixture", "y", "q"}),
      impl("hedit, already being edited", {"hedit fixture", "@2 hedit fixture", "y", "q"}),
      impl("hedit, help index and check", {"hindex f", "hindex", "helpcheck"}),
      /* ---- cedit ---- */
      impl("cedit, opening and quitting", {"cedit", "?", "x", "q"}),
      as(ADMLVL_GRGOD, "cedit, below an implementor", {"cedit"}),
      impl("cedit, not saving", {"cedit", "g", "1", "q", "q", "maybe", "n"}),
      impl("cedit, already being edited", {"cedit", "@2 cedit", "q"}),
      impl("cedit, menu G", {"cedit", "G", "A", "7", "/s", "B", "7", "/s", "C", "7", "/s", "D", "7", "/s", "E", "7", "/s", "F", "7", "/s", "G", "7", "/s", "H", "7", "/s", "I", "7", "/s", "J", "7", "/s", "K", "7", "/s", "L", "7", "/s", "M", "7", "/s", "N", "7", "/s", "O", "7", "/s", "R", "7", "/s", "1", "7", "/s", "2", "7", "/s", "3", "7", "/s", "P", "7", "/s", "8", "7", "/s", "9", "7", "/s", "S", "7", "/s", "T", "7", "/s", "4", "7", "/s", "5", "7", "/s", "6", "7", "/s", "q", "q", "y"}),
      impl("cedit, menu G out of range", {"cedit", "G", "A", "-1", "/s", "A", "abc", "/s", "A", "999999", "/s", "B", "-1", "/s", "B", "abc", "/s", "B", "999999", "/s", "C", "-1", "/s", "C", "abc", "/s", "C", "999999", "/s", "D", "-1", "/s", "D", "abc", "/s", "D", "999999", "/s", "E", "-1", "/s", "E", "abc", "/s", "E", "999999", "/s", "F", "-1", "/s", "F", "abc", "/s", "F", "999999", "/s", "G", "-1", "/s", "G", "abc", "/s", "G", "999999", "/s", "H", "-1", "/s", "H", "abc", "/s", "H", "999999", "/s", "I", "-1", "/s", "I", "abc", "/s", "I", "999999", "/s", "J", "-1", "/s", "J", "abc", "/s", "J", "999999", "/s", "K", "-1", "/s", "K", "abc", "/s", "K", "999999", "/s", "L", "-1", "/s", "L", "abc", "/s", "L", "999999", "/s", "M", "-1", "/s", "M", "abc", "/s", "M", "999999", "/s", "N", "-1", "/s", "N", "abc", "/s", "N", "999999", "/s", "O", "-1", "/s", "O", "abc", "/s", "O", "999999", "/s", "R", "-1", "/s", "R", "abc", "/s", "R", "999999", "/s", "1", "-1", "/s", "1", "abc", "/s", "1", "999999", "/s", "2", "-1", "/s", "2", "abc", "/s", "2", "999999", "/s", "3", "-1", "/s", "3", "abc", "/s", "3", "999999", "/s", "P", "-1", "/s", "P", "abc", "/s", "P", "999999", "/s", "8", "-1", "/s", "8", "abc", "/s", "8", "999999", "/s", "9", "-1", "/s", "9", "abc", "/s", "9", "999999", "/s", "S", "-1", "/s", "S", "abc", "/s", "S", "999999", "/s", "T", "-1", "/s", "T", "abc", "/s", "T", "999999", "/s", "4", "-1", "/s", "4", "abc", "/s", "4", "999999", "/s", "5", "-1", "/s", "5", "abc", "/s", "5", "999999", "/s", "6", "-1", "/s", "6", "abc", "/s", "6", "999999", "/s", "q", "q", "y"}),
      impl("cedit, menu N", {"cedit", "N", "B", "7", "/s", "C", "7", "/s", "A", "7", "/s", "q", "q", "y"}),
      impl("cedit, menu N out of range", {"cedit", "N", "B", "-1", "/s", "B", "abc", "/s", "B", "999999", "/s", "C", "-1", "/s", "C", "abc", "/s", "C", "999999", "/s", "A", "-1", "/s", "A", "abc", "/s", "A", "999999", "/s", "q", "q", "y"}),
      impl("cedit, menu C", {"cedit", "C", "A", "7", "/s", "B", "7", "/s", "C", "7", "/s", "D", "7", "/s", "E", "7", "/s", "F", "7", "/s", "G", "7", "/s", "q", "q", "y"}),
      impl("cedit, menu C out of range", {"cedit", "C", "A", "-1", "/s", "A", "abc", "/s", "A", "999999", "/s", "B", "-1", "/s", "B", "abc", "/s", "B", "999999", "/s", "C", "-1", "/s", "C", "abc", "/s", "C", "999999", "/s", "D", "-1", "/s", "D", "abc", "/s", "D", "999999", "/s", "E", "-1", "/s", "E", "abc", "/s", "E", "999999", "/s", "F", "-1", "/s", "F", "abc", "/s", "F", "999999", "/s", "G", "-1", "/s", "G", "abc", "/s", "G", "999999", "/s", "q", "q", "y"}),
      impl("cedit, menu R", {"cedit", "R", "A", "7", "/s", "B", "7", "/s", "C", "7", "/s", "1", "7", "/s", "2", "7", "/s", "3", "7", "/s", "q", "q", "y"}),
      impl("cedit, menu R out of range", {"cedit", "R", "A", "-1", "/s", "A", "abc", "/s", "A", "999999", "/s", "B", "-1", "/s", "B", "abc", "/s", "B", "999999", "/s", "C", "-1", "/s", "C", "abc", "/s", "C", "999999", "/s", "1", "-1", "/s", "1", "abc", "/s", "1", "999999", "/s", "2", "-1", "/s", "2", "abc", "/s", "2", "999999", "/s", "3", "-1", "/s", "3", "abc", "/s", "3", "999999", "/s", "q", "q", "y"}),
      impl("cedit, menu O", {"cedit", "O", "A", "7", "/s", "B", "7", "/s", "C", "7", "/s", "D", "7", "/s", "E", "7", "/s", "F", "7", "/s", "G", "7", "/s", "H", "7", "/s", "J", "7", "/s", "I", "7", "/s", "K", "7", "/s", "L", "7", "/s", "M", "7", "/s", "N", "7", "/s", "q", "q", "y"}),
      impl("cedit, menu O out of range", {"cedit", "O", "A", "-1", "/s", "A", "abc", "/s", "A", "999999", "/s", "B", "-1", "/s", "B", "abc", "/s", "B", "999999", "/s", "C", "-1", "/s", "C", "abc", "/s", "C", "999999", "/s", "D", "-1", "/s", "D", "abc", "/s", "D", "999999", "/s", "E", "-1", "/s", "E", "abc", "/s", "E", "999999", "/s", "F", "-1", "/s", "F", "abc", "/s", "F", "999999", "/s", "G", "-1", "/s", "G", "abc", "/s", "G", "999999", "/s", "H", "-1", "/s", "H", "abc", "/s", "H", "999999", "/s", "J", "-1", "/s", "J", "abc", "/s", "J", "999999", "/s", "I", "-1", "/s", "I", "abc", "/s", "I", "999999", "/s", "K", "-1", "/s", "K", "abc", "/s", "K", "999999", "/s", "L", "-1", "/s", "L", "abc", "/s", "L", "999999", "/s", "M", "-1", "/s", "M", "abc", "/s", "M", "999999", "/s", "N", "-1", "/s", "N", "abc", "/s", "N", "999999", "/s", "q", "q", "y"}),
      impl("cedit, menu A", {"cedit", "A", "A", "7", "/s", "B", "7", "/s", "q", "q", "y"}),
      impl("cedit, menu A out of range", {"cedit", "A", "A", "-1", "/s", "A", "abc", "/s", "A", "999999", "/s", "B", "-1", "/s", "B", "abc", "/s", "B", "999999", "/s", "q", "q", "y"}),
      impl("cedit, menu D", {"cedit", "D", "A", "7", "/s", "B", "7", "/s", "q", "q", "y"}),
      impl("cedit, menu D out of range", {"cedit", "D", "A", "-1", "/s", "A", "abc", "/s", "A", "999999", "/s", "B", "-1", "/s", "B", "abc", "/s", "B", "999999", "/s", "q", "q", "y"}),
      impl("cedit, menu T", {"cedit", "T", "A", "7", "/s", "B", "7", "/s", "C", "7", "/s", "D", "7", "/s", "E", "7", "/s", "F", "7", "/s", "G", "7", "/s", "H", "7", "/s", "I", "7", "/s", "q", "q", "y"}),
      impl("cedit, menu T out of range", {"cedit", "T", "A", "-1", "/s", "A", "abc", "/s", "A", "999999", "/s", "B", "-1", "/s", "B", "abc", "/s", "B", "999999", "/s", "C", "-1", "/s", "C", "abc", "/s", "C", "999999", "/s", "D", "-1", "/s", "D", "abc", "/s", "D", "999999", "/s", "E", "-1", "/s", "E", "abc", "/s", "E", "999999", "/s", "F", "-1", "/s", "F", "abc", "/s", "F", "999999", "/s", "G", "-1", "/s", "G", "abc", "/s", "G", "999999", "/s", "H", "-1", "/s", "H", "abc", "/s", "H", "999999", "/s", "I", "-1", "/s", "I", "abc", "/s", "I", "999999", "/s", "q", "q", "y"}),
      /* ---- assedit ---- */
      impl("assedit, refusals", {"assedit", "assedit nonsense", "assedit new", "assedit new 99999", "assedit delete", "assedit 99999"}),
      impl("assedit, editing an assembly", {"assedit 300", "?", "t", "abc", "t", "99", "t", "2", "a", "abc", "a", "-5", "a", "303", "a", "301", "e", "abc", "e", "9", "e", "1", "maybe", "n", "maybe", "y", "e", "3", "y", "n", "d", "abc", "d", "9", "d", "2", "d", "1", "q", "assedit 300", "q"}),
      impl("assedit, a new assembly and deleting it", {"assedit new 303", "assedit 303", "a", "300", "q", "assedit delete 303", "assedit 303"}),
      impl("assedit, already being edited", {"assedit 300", "@2 assedit 300", "q"}),
      /* ---- tedit ---- */
      impl("tedit, the files", {"tedit", "tedit nonsense", "tedit motd", "A new message of the day.", "/s", "motd", "tedit credits", "/a", "tedit news", "/c", "/s", "tedit help", "Help screen.", "/s", "tedit info", "/s", "tedit background", "/s", "tedit handbook", "/s", "tedit update", "/s", "tedit ihelp", "/s", "tedit imotd", "@", "tedit donottouch", "Fresh news text.", "/s", "news"}),
      as(ADMLVL_GRGOD, "tedit, below an implementor", {"tedit", "tedit motd", "tedit help", "/s"}),
      /* ---- the text editor ---- */
      impl("text editor, every command", {"redit 300", "2", "/h", "/l", "/c", "/c", "/l", "/n", "/f", "/i 1 x", "/d 1", "/e 1 x", "/r 'a' 'b'", "first line", "second line with words to format and wrap about the place, long enough to wrap when formatted to the width", "third", "/l", "/l 2", "/l 2-3", "/l 3-2", "/l 9", "/l x", "/n", "/n 1-2", "/n 2-9", "/d", "/d 9", "/d 2", "/d 1-2", "alpha line", "beta line with several words in it to be formatted and wrapped by the formatter as it goes along", "gamma", "/i", "/i 9 x", "/i 1 inserted", "/i 1", "/e", "/e 9 x", "/e 1 edited", "/e 1", "/r", "/r 'x'", "/r 'third' 'THIRD'", "/ra 'e' 'E'", "/r 'nothing' 'here'", "/f", "/fi", "/f 1-2", "/fi 9", "/z", "/", "@", "q", "y"}),
      impl("text editor, too long", {"oedit 300", "e", "1", "x", "2", std::string(250, 'a'), std::string(250, 'b'), std::string(250, 'c'), std::string(250, 'd'), std::string(250, 'e'), std::string(250, 'f'), "/s", "0", "q", "y"}),
      /* ---- the lists, links, copying, digging and the save list ---- */
      impl("lists", {"rlist", "rlist 3", "rlist 300 305", "rlist 305 300", "rlist 99", "mlist 3", "mlist 300 399", "olist 3", "olist 300 310", "slist 3", "zlist", "zlist 3", "zlist 0 4", "tlist 3", "tlist 300 320", "glist 3", "links", "links 3", "links 99"}),
      impl("rcopy and tunnel", {"rcopy", "rcopy 384", "rcopy 99999 300", "tunnel", "tunnel north", "tunnel sideways 386", "tunnel west 386", "tunnel west 386", "tunnel up 99999", "tunnel down 384", "tunnel east -1", "tunnel east -1", "tunnel south 0", "look", "rcopy 300 384", "rcopy 384 300", "goto 386", "rclone 301", "rcopy 99999", "look", "redit 386", "q"}),
      impl("newsedit", {"newsedit", std::string("newsedit ") + std::string(60, 'n'), "newsedit a # title", "newsedit Fresh news", "Something happened.", "/s", "news", "newsedit Aborted news", "Not this.", "/a", "newsedit Empty news", "/s"}),
      as(ADMLVL_IMMORT, "buildwalk", {"buildwalk", "north", "south", "east", "redit 300", "q", "buildwalk", "north"}, zone_3),
      impl("buildwalk, an implementor", {"buildwalk", "west", "look", "west", "down", "buildwalk"}),
      as(ADMLVL_BUILDER, "buildwalk, a builder of no zone", {"buildwalk", "west", "buildwalk"}),
      impl("restring, away from the shop", {"load obj 301", "restring sword"}, rich),
      impl("the save list", {"olc", "redit 300", "1", "Changed", "q", "y", "olc"}),
      impl("skillset", {"skillset", "skillset watcher", "skillset watcher kick", "skillset watcher 'kick' 50", "skillset watcher 'kick' 500", "skillset nobody 'kick' 5", "skillset watcher 'nonsense' 5", "stat watcher"}),
      /* ---- medit ---- */
      impl("medit, refusals", {"medit", "medit nonsense", "medit 99999", "medit save 99999", "medit save"}),
      as(ADMLVL_BUILDER, "medit, a builder of no zone", {"medit 300", "medit save 3"}),
      as(ADMLVL_BUILDER, "medit, a builder of the zone", {"medit 300", "3", "a builder's mob", "q", "y", "medit save"}, zone_3),
      impl("medit, saving a zone", {"medit save 3"}),
      impl("medit, opening and quitting", {"medit 300", "?", "q"}),
      impl("medit, the strings", {"medit 300", "2", "", "2", "guard new", "3", "a new guard", "4", "", "4", "A new guard stands here.", "5", "A tall guard.", "/s", "q", "y"}),
      impl("medit, the sex and positions", {"medit 300", "1", "x", "1", "-4", "1", "9", "1", "2", "i", "abc", "i", "99", "i", "5", "j", "-1", "j", "8", "q", "y"}),
      impl("medit, the numbers", {"medit 300", "6", "0", "6", "999", "6", "50", "7", "-5000", "7", "350", "8", "70", "9", "5", "a", "40", "b", "200", "c", "3", "d", "4", "e", "5", "f", "1", "f", "500", "g", "9999999", "h", "-3", "h", "500", "q", "y"}),
      impl("medit, autobalance turned off", {"medit 300", "l", "17", "0", "8", "10", "l", "17", "0", "9", "1", "l", "17", "0", "a", "1", "l", "17", "0", "b", "1", "l", "17", "0", "c", "1", "l", "17", "0", "d", "1", "l", "17", "0", "e", "1", "l", "17", "0", "f", "1", "l", "17", "0", "g", "1", "q", "y"}),
      impl("medit, the flags", {"medit 300", "l", "1", "4", "999", "-1", "0", "m", "1", "4", "999", "0", "k", "2", "999", "0", "q", "y"}),
      impl("medit, the class, race and size", {"medit 300", "n", "abc", "n", "3", "o", "999", "o", "2", "y", "-5", "y", "3", "y", "99", "q", "y"}),
      impl("medit, the script editor", {"medit 300", "s", "n", "1, 310", "n", "2, 340", "x", "q", "y"}),
      impl("medit, copy", {"medit 303", "w", "99999", "w", "300", "q", "y", "medit 303", "q"}),
      impl("medit, wiznet and the save prompt", {"medit 300", "z hello", "Z", "3", "renamed", "q", "maybe", "N"}),
      impl("medit, deleting a mob, not", {"medit 302", "x", "maybe", "n", "q"}),
      impl("medit, deleting a mob", {"medit 302", "X", "y", "medit 302"}),
      impl("medit, a new mob", {"medit 304", "2", "fresh mob", "3", "a fresh mob", "q", "y", "load mob 304", "look"}),
      impl("medit, already being edited", {"medit 300", "@2 medit 300", "q"}),
      /* ---- iedit ---- */
      impl("iedit, refusals", {"iedit", "iedit nothing"}),
      impl("iedit, a carried sword", {"load obj 301", "iedit sword", "1", "sword blade unique", "2", "a unique sword", "8", "12", "s", "w", "q", "i", "save"}),
      impl("iedit, a worn sword", {"load obj 301", "get sword", "wield sword", "iedit sword", "c", "2", "3", "4", "5", "1", "2", "/s", "q", "eq"}),
      impl("iedit, a chest on the floor", {"load obj 300", "drop chest", "iedit chest", "6", "1", "0", "q", "look"}),
      impl("iedit, the object purged meanwhile", {"load obj 301", "iedit sword", "1", "doomed", "@2 purge", "@2 force builder q", "q", "i"}),
      /* ---- pobj (rpp restring and custom equipment) ---- */
      impl("pobj, restring", {"load obj 301", "get sword", "rpp 14 sword", "1", "", "1", "@Rred", "1", "longsword", "2", "@ra long sword@n", "3", "A long sword lies here.", "9", "q", "maybe", "y", "i"}, rich),
      impl("pobj, restring cancelled", {"load obj 301", "get sword", "rpp 14 sword", "q", "", "n", "i"}, rich),
      impl("pobj, restring too long", {"load obj 301", "get sword", "rpp 14 sword", "1", std::string(120, 'x'), "abc", "2", std::string(220, 'y'), "abc", "3", std::string(220, 'z'), "abc", "q", "y"}, rich),
      impl("pobj, custom armor", {"rpp 3", "1", "", "1", "@Gvest", "1", "shiny vest", "2", "@ga shiny vest@n", "3", "A shiny vest", "4", "", "4", "99", "4", "2", "5", "q", "", "maybe", "y", "i"}, rich),
      impl("pobj, custom weapon of each kind", {"rpp 3", "4", "16", "5", "", "5", "9", "5", "1", "q", "y", "rpp 3", "4", "16", "5", "2", "q", "y", "rpp 3", "4", "16", "5", "3", "q", "y", "rpp 3", "4", "16", "5", "4", "q", "y", "rpp 3", "4", "16", "5", "5", "q", "y", "rpp 3", "4", "16", "5", "6", "q", "y", "i"}, rich),
      impl("pobj, custom of each slot", {"rpp 3", "4", "3", "q", "y", "rpp 3", "4", "4", "q", "y", "rpp 3", "4", "5", "q", "y", "rpp 3", "4", "6", "q", "y", "rpp 3", "4", "7", "q", "y", "rpp 3", "4", "8", "q", "y", "rpp 3", "4", "9", "q", "y", "rpp 3", "4", "10", "q", "y", "rpp 3", "4", "11", "q", "y", "rpp 3", "4", "12", "q", "y", "rpp 3", "4", "13", "q", "y", "rpp 3", "4", "14", "q", "y", "rpp 3", "4", "15", "q", "y", "i"}, rich),
      impl("pobj, custom cancelled", {"rpp 3", "x", "q", "n"}, rich),
  };
  every_item_type(out);
  return out;
  }

  unsigned long seed_for(const std::string &what)
  {
    unsigned long h = 5381;
    for (char c : what)
      h = h * 33 + (unsigned char)c;
    return h;
  }

  /* A hash of a file's text, to show what was saved without the whole of it */
  unsigned long hash_of(const std::string &text)
  {
    unsigned long h = 14695981039346656037UL;
    for (char c : text)
      h = (h ^ (unsigned char)c) * 1099511628211UL;
    return h;
  }

  const char *const MADE_BY_LOGIN[] = {"plrfiles/", "intro/", "sense/", "user/", "test.log"};

  /* Every file under here that differs from the same file under original, or that it lacks, in name order; then
     every file the original has that is gone */
  void changed_files(const std::filesystem::path &original)
  {
    namespace fs = std::filesystem;
    std::vector<std::string> here, gone;
    for (const fs::directory_entry &e : fs::recursive_directory_iterator("."))
      if (e.is_regular_file())
        here.push_back(e.path().lexically_relative(".").string());
    for (const fs::directory_entry &e : fs::recursive_directory_iterator(original))
      if (e.is_regular_file() && !fs::exists(e.path().lexically_relative(original)))
        gone.push_back(e.path().lexically_relative(original).string());
    std::sort(here.begin(), here.end());
    std::sort(gone.begin(), gone.end());
    for (const std::string &name : here) {
      /* the report, and the files making the builders writes, the same in every journey */
      if (name.starts_with("case-") || std::ranges::any_of(MADE_BY_LOGIN, [&](const char *dir) { return name.starts_with(dir); }))
        continue;
      std::string text = th_file_text(name.c_str());
      fs::path before  = original / name;
      if (fs::exists(before) && th_file_text(before.c_str()) == text)
        continue;
      if (text.size() > SHOWN_IN_FULL)
        file_print(stdout, "== {} {}: {} bytes, hash {:x}\n", fs::exists(before) ? "changed" : "new", name, text.size(), hash_of(text));
      else
        file_print(stdout, "== {} {}: {} bytes\n{}\n== end\n", fs::exists(before) ? "changed" : "new", name, text.size(), text);
    }
    for (const std::string &name : gone)
      file_print(stdout, "== gone {}\n", name);
  }

  void audit_problem(void *, const char *problem) { file_print(stdout, "AUDIT: {}\n", problem); }

  bool still_connected(GameWorld *g, struct descriptor_data *d)
  {
    for (descriptor_data *other : each_desc(g))
      if (other == d)
        return true;
    return false;
  }

  /* What ch has been sent since last time, and the state its connection is in */
  void show_output(GameWorld *g, const char *who, struct char_data *ch)
  {
    struct descriptor_data *d = char_get_desc(ch);
    if (!d || !still_connected(g, d)) {
      file_print(stdout, "{}(no connection)\n", who);
      return;
    }
    if (*who && !*th_output(ch)) /* the others are shown only when they're sent something */
      return;
    file_print(stdout, "{}{}\n[state {:d}]\n", who, th_output(ch), desc_get_connected(d));
    th_take_output(g, ch);
  }

  /* A builder, who sees the whole of the game's log */
  struct char_data *builder(GameWorld *g, const char *name, int admlevel)
  {
    struct char_data *ch = th_player(g, name, admlevel);
    char_prf_set(g, ch, PRF_LOG1, true);
    char_prf_set(g, ch, PRF_LOG2, true);
    th_take_output(g, ch);
    return ch;
  }

  void play(GameWorld *g, const journey &j, unsigned long seed)
  {
    circle_srandom(g, seed);
    struct char_data *watcher = builder(g, "Watcher", ADMLVL_IMPL);
    struct char_data *ch      = builder(g, "Builder", j.admlevel);
    struct char_data *second  = nullptr;
    if (j.setup)
      j.setup(g, ch);
    th_take_output(g, ch);
    th_take_output(g, watcher);
    for (const std::string &text : j.lines) {
      const char *line = text.c_str();
      if (!strncmp(line, "@pulse ", 7)) {
        th_pulses(g, atoi(line + 7));
        file_print(stdout, "{}\n", line);
        show_output(g, "", ch);
        show_output(g, "w: ", watcher);
        continue;
      }
      if (!strncmp(line, "@2 ", 3)) {
        if (!second) {
          second = builder(g, "Second", ADMLVL_IMPL);
        }
        file_print(stdout, "2> {}\n", line + 3);
        th_type_at(g, char_get_desc(second), line + 3);
        show_output(g, "2: ", second);
        show_output(g, "", ch);
        show_output(g, "w: ", watcher);
        continue;
      }
      file_print(stdout, "> {}\n", line);
      th_type_at(g, char_get_desc(ch), line);
      show_output(g, "", ch);
      if (second)
        show_output(g, "2: ", second);
      show_output(g, "w: ", watcher);
    }
    file_print(stdout, "rng {:d}\n", g->rng_seed);
    if (world_audit(g, audit_problem, nullptr))
      th_exit(3);
  }

  void run(GameWorld *g, const journey &j)
  {
    char report[64];
    int status;
    snprintf(report, sizeof(report), "case-%ld.txt", (long)getpid());
    cases++;
    fflush(nullptr);
    std::filesystem::path original = std::filesystem::current_path();
    std::string own = str_format("{}/dbat-olc-{:d}-{:d}", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp", getpid(), cases);
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      exit(2);
    }
    if (pid == 0) {
      /* each journey plays in its own copy of the world, so the files one saves aren't there for the next */
      std::string copy = str_format("mkdir -p '{}/data' && cp -r . '{}/data'", own, own);
      if (system(copy.c_str()) != 0 || chdir((own + "/data").c_str()) < 0)
        th_exit(2);
      int fd = open(report, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      dup2(fd, STDOUT_FILENO);
      dup2(fd, STDERR_FILENO);
      alarm(CASE_SECONDS);
      play(g, j, seed_for(j.name));
      fflush(nullptr);
      changed_files(original);
      th_exit(0);
    }
    waitpid(pid, &status, 0);
    std::string report_path = own + "/data/" + report;
    std::string text        = th_file_text(report_path.c_str());
    std::string cleanup     = str_format("rm -rf '{}'", own);
    if (system(cleanup.c_str()) != 0)
      perror("rm");
    int outcome = WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
    if (transcript) {
      file_print(transcript, "######## {} | {:d}\n{}", j.name, outcome, text);
      fflush(transcript);
    }
    if (outcome) {
      failures++;
      file_print(stdout, "FAIL ({:d}): {}\n{}", outcome, j.name, text);
    }
  }
} // namespace

int main(int argc, char **argv)
{
  int shard = 0, shards = 1, arg = 1;

  while (arg < argc && !strncmp(argv[arg], "--", 2)) {
    if (!strcmp(argv[arg], "--transcript") && arg + 1 < argc) {
      if (!(transcript = fopen(argv[arg + 1], "w"))) {
        perror("--transcript");
        return 2;
      }
      arg += 2;
    } else if (!strcmp(argv[arg], "--shard") && arg + 1 < argc && sscanf(argv[arg + 1], "%d/%d", &shard, &shards) == 2 && shards > 0 && shard >= 0 &&
               shard < shards) {
      arg += 2;
    } else {
      return 2;
    }
  }
  const char *only = argc > arg ? argv[arg] : nullptr;

  GameWorld *g = th_boot_at(TRANSCRIPT_TIME);
  int i        = 0;
  for (const journey &j : journeys()) {
    if (i++ % shards != shard || (only && !j.name.starts_with(only)))
      continue;
    run(g, j);
  }
  if (transcript)
    fclose(transcript);
  file_print(stdout, "{:d} cases, {:d} failed\n", cases, failures);
  th_shutdown(g);
  return failures != 0;
}
