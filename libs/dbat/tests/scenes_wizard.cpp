/* Scenes for test_techniques: see scene_kit.hpp */
#include "scene_kit.hpp"

#include "dbat/act.wizard.hpp"
#include "dbat/consts/adminflags.hpp"
#include "dbat/consts/zoneflags.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/fileop.hpp"
#include "dbat/flags.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/players.hpp"
#include "dbat/zones.hpp"

namespace {
  /* ... an immortal of this level, with the powers it brings */
  void imm(GameWorld *g, struct char_data *att, int level = ADMLVL_IMPL)
  {
    admin_set(g, att, level);
  }

  /* ... with this as the news file */
  void news_file(const char *text)
  {
    if (FILE *fl = fopen("text/news", "w")) {
      fputs(text, fl);
      fclose(fl);
    }
  }

  constexpr const char *NEWS = "#1 First news\nThe first.\nThe first.\nTHE FIRST.\nAnd more.\n#2 Second news\nThe second.\n#x Broken\nNothing.\n#3 Third\nThe third.\n";

  /* ... with a lockout file of these names, or of count made up ones, or none */
  void lockout_file(std::initializer_list<const char *> names, int count = 0)
  {
    char fname[40];
    if (!get_filename(fname, sizeof(fname), INTRO_FILE, "lockout"))
      return;
    remove(fname);
    if (names.size() == 0 && count < 0)
      return;
    if (FILE *fl = fopen(fname, "w")) {
      for (const char *name : names)
        fprintf(fl, "%s\n", name);
      for (int i = 0; i < count; i++)
        fprintf(fl, "Locked%d\n", i);
      fclose(fl);
    }
  }

  /* ... with a user file for this account name, which ch plays unless asked */
  void account(GameWorld *g, struct char_data *ch, const char *user, bool playing = true)
  {
    struct descriptor_data *d = char_get_desc(ch);
    d->user                   = user;
    userWrite(g, d, 0, 0, 0, "index");
    if (!playing)
      d->user.clear();
  }

  /* ... carrying something one of a kind, with no prototype */
  struct obj_data *one_of_a_kind(GameWorld *g, struct char_data *att)
  {
    struct obj_data *obj = create_obj(g);
    obj_set_item_number(obj, NOTHING);
    obj_set_in_room(obj, NOWHERE);
    obj_set_name(obj, "head");
    obj_set_short_description(obj, "a bloody head");
    obj_set_type_flag(obj, ITEM_OTHER);
    obj_wear_set(obj, ITEM_WEAR_TAKE, true);
    add_unique_id(g, obj);
    obj_to_char(g, obj, att);
    return obj;
  }

  /* ... a thing here of this type, for stat to show */
  struct obj_data *typed(GameWorld *g, struct char_data *att, int type)
  {
    struct obj_data *obj = holding(g, att, "thing", type);
    for (int i = 0; i < 12; i++)
      obj_set_value(obj, i, i == 6 || i == 7 || i == 8 ? 0 : 1);
    return obj;
  }

  void seated_on(struct char_data *ch, struct obj_data *seat)
  {
    char_set_sits(ch, seat);
    obj_set_sitting(seat, ch);
  }

  struct char_data *the_dummy(GameWorld *g)
  {
    for (char_data *c : room_people(g, ARENA))
      if (char_is_npc(c) && isname("dummy", char_get_alias(c)))
        return c;
    return nullptr;
  }

  void zone_flag(GameWorld *g, room_vnum room, int flag)
  {
    SET_BIT_AR(room_get_zone(g, room)->zone_flags, flag);
  }

  void key_exit(GameWorld *g, obj_vnum key)
  {
    exit_to(g, NORTH, 300);
    room_get_dir_option(g, ARENA, NORTH)->key     = key;
    room_get_dir_option(g, ARENA, NORTH)->keyword = "door";
  }

  const scene list[] = {
    /* wizard/comm.cpp */
    {"a mob, reading", "news", false, SETUP(), true, 1},
    {"reading", "news", false, SETUP(news_file(NEWS);), false, 1},
    {"reading", "news list", false, SETUP(news_file(NEWS);), false, 1},
    {"reading", "news 1", false, SETUP(news_file(NEWS);), false, 1},
    {"reading", "news 2", false, SETUP(news_file(NEWS);), false, 1},
    {"reading", "news 3", false, SETUP(news_file(NEWS);), false, 1},
    {"reading", "news 9", false, SETUP(news_file(NEWS);), false, 1},
    {"reading", "news latest", false, SETUP(news_file(NEWS);), false, 1},
    {"reading empty news", "news list", false, SETUP(news_file("");), false, 1},
    {"reading news without numbers", "news list", false, SETUP(news_file("#x Nothing\nAt all.\n");), false, 1},
    {"reading no news", "news list", false, SETUP(remove("text/news");), false, 1},
    {"a mortal", "newsedit Hello", false, SETUP(news_file(NEWS);), false, 1},
    {"an immortal", "newsedit", false, SETUP(imm(g, att, ADMLVL_IMMORT); news_file(NEWS);), false, 1},
    {"an immortal", "newsedit A title that is far too long for the news file to take at all", false, SETUP(imm(g, att, ADMLVL_IMMORT); news_file(NEWS);), false, 1},
    {"an immortal", "newsedit A #title", false, SETUP(imm(g, att, ADMLVL_IMMORT); news_file(NEWS);), false, 1},
    {"an immortal", "newsedit Fresh news", false, SETUP(imm(g, att, ADMLVL_IMMORT); news_file(NEWS);), false, 1},
    {"an immortal, the news empty", "newsedit Fresh news", false, SETUP(imm(g, att, ADMLVL_IMMORT); news_file("");), false, 1},
    {"an immortal, the news unnumbered", "newsedit Fresh news", false, SETUP(imm(g, att, ADMLVL_IMMORT); news_file("#x Nothing\nAt all.\n");), false, 1},
    {"an immortal, no news file", "newsedit Fresh news", false, SETUP(imm(g, att, ADMLVL_IMMORT); remove("text/news");), false, 1},
    {"an immortal", "echo", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "echo The sky darkens.", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, not repeated", "echo The sky darkens.", false, SETUP(imm(g, att); char_prf_set(g, att, PRF_NOREPEAT, true);), false, 1},
    {"emoting", "emote waves.", false, SETUP(), false, 1},
    {"emoting", "emote waves at *Victim &1hi&2 and # grins.", false, SETUP(), false, 1},
    {"emoting", "emote waves at *Nobody.", false, SETUP(), false, 1},
    {"smoting", "smote # waves at *Victim.", false, SETUP(), false, 1},
    {"smoting", "smote The wind blows at *Victim.", false, SETUP(), false, 1},
    {"smoting", "smote The wind blows at *Nobody.", false, SETUP(), false, 1},
    {"smoting", "smote # says &1hello&2.", false, SETUP(), false, 1},
    {"a mob, smoting", "smote The wind blows.", false, SETUP(), true, 1},
    {"an immortal", "send", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "send nobody hi", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "send Victim You feel watched.", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, not repeated", "send Victim You feel watched.", false, SETUP(imm(g, att); char_prf_set(g, att, PRF_NOREPEAT, true);), false, 1},
    {"an immortal", "syslog", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, logging", "syslog", false, SETUP(imm(g, att); char_prf_set(g, att, PRF_LOG2, true);), false, 1},
    {"an immortal", "syslog brief", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "syslog normal", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "syslog complete", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "syslog off", false, SETUP(imm(g, att); char_prf_set(g, att, PRF_LOG1, true);), false, 1},
    {"an immortal", "syslog loud", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "gecho", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "gecho The ground shakes $$.", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, not repeated", "gecho The ground shakes.", false, SETUP(imm(g, att); char_prf_set(g, att, PRF_NOREPEAT, true);), false, 1},
    {"an immortal", "info", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "info Reboot soon.", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, not repeated", "info Reboot soon.", false, SETUP(imm(g, att); char_prf_set(g, att, PRF_NOREPEAT, true);), false, 1},
    {"an immortal", "poofin arrives in a flash.", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "poofout leaves in a flash.", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "wiznet", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "wiznet Hello gods.", false, SETUP(imm(g, att); imm(g, victim, ADMLVL_IMMORT);), false, 1},
    {"an immortal, unseen", "wiznet Hello gods.", false, SETUP(imm(g, att); imm(g, victim, ADMLVL_IMMORT); char_set_invis_level(g, att, ADMLVL_IMPL);), false, 1},
    {"an immortal, not repeated", "wiznet Hello gods.", false, SETUP(imm(g, att); char_prf_set(g, att, PRF_NOREPEAT, true);), false, 1},
    {"an immortal, offline", "wiznet Hello gods.", false, SETUP(imm(g, att); char_prf_set(g, att, PRF_NOWIZ, true);), false, 1},
    {"an immortal", "wiznet #4 For the high.", false, SETUP(imm(g, att); imm(g, victim, ADMLVL_VICE); imm(g, cast.helper, ADMLVL_IMMORT);), false, 1},
    {"a low immortal", "wiznet #4 For the high.", false, SETUP(imm(g, att, ADMLVL_IMMORT);), false, 1},
    {"an immortal", "wiznet #0 For all.", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "wiznet #", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "wiznet #nothing", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "wiznet *waves.", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "wiznet *4 waves.", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "wiznet \\#hashed", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, among others", "wiznet @", false, SETUP(imm(g, att); imm(g, victim, ADMLVL_IMMORT); char_plr_set(victim, PLR_WRITING, true); imm(g, cast.helper, ADMLVL_GOD); char_prf_set(g, cast.helper, PRF_NOWIZ, true); char_plr_set(cast.helper, PLR_MAILING, true);), false, 1},
    {"an immortal, a writer by", "wiznet Hello gods.", false, SETUP(imm(g, att); imm(g, victim, ADMLVL_IMMORT); char_plr_set(victim, PLR_WRITING, true);), false, 1},

    /* wizard/server.cpp */
    {"an immortal", "lag", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "lag Victim 9", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "lag Victim 0", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "lag Victim 1", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "lag Victim 2", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "lag Victim 3", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "lag Victim 4", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "lag Victim 5", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "lag Nobody 3", false, SETUP(imm(g, att);), false, 1},
    {"a low immortal", "lag Victim 3", false, SETUP(imm(g, att, ADMLVL_IMMORT); imm(g, victim, ADMLVL_GOD);), false, 1},
    {"an implementor", "shutdow", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "shutdown", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "shutdown reboot", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "shutdown die", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "shutdown now", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "shutdown pause", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "shutdown sideways", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "copyover help", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "copyover cancel", false, SETUP(imm(g, att);), false, 1},
    {"an implementor, a copyover coming", "copyover stop", false, SETUP(imm(g, att); g->copyover_timer = 500;), false, 1},
    {"an implementor", "copyover 0", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "copyover -5", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "copyover 45", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "copyover 60", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "copyover 120", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "copyover 90", false, SETUP(imm(g, att);), false, 1},
    {"an implementor, the clock at 121", "copyover help", false, SETUP(imm(g, att); g->copyover_timer = 121; copyover_check(g);), false, 1},
    {"an implementor, the clock at 61", "copyover help", false, SETUP(imm(g, att); g->copyover_timer = 61; copyover_check(g);), false, 1},
    {"an implementor, the clock at 75", "copyover help", false, SETUP(imm(g, att); g->copyover_timer = 75; copyover_check(g);), false, 1},
    {"an implementor, the clock at 41", "copyover help", false, SETUP(imm(g, att); g->copyover_timer = 41; copyover_check(g);), false, 1},
    {"an implementor, the clock at 36", "copyover help", false, SETUP(imm(g, att); g->copyover_timer = 36; copyover_check(g);), false, 1},
    {"an implementor, the clock at 26", "copyover help", false, SETUP(imm(g, att); g->copyover_timer = 26; copyover_check(g);), false, 1},
    {"an implementor, the clock at 24", "copyover help", false, SETUP(imm(g, att); g->copyover_timer = 24; copyover_check(g);), false, 1},
    {"an implementor", "dc", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "dc 999", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "dc 1", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "dc 2", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "dc 3", false, SETUP(imm(g, att);), false, 1},
    {"an implementor, the others immortal", "dc 2", false, SETUP(imm(g, att); imm(g, victim); imm(g, cast.helper); imm(g, cast.android);), false, 1},
    {"an implementor, the others closing", "dc 2", false, SETUP(imm(g, att); for (descriptor_data *d : each_desc(g)) if (d->character != att) desc_set_connected(d, CON_DISCONNECT);), false, 1},
    {"an implementor", "wizlock", false, SETUP(imm(g, att);), false, 1},
    {"an implementor, closed to new players", "wizlock", false, SETUP(imm(g, att); g->circle_restrict = 1;), false, 1},
    {"an implementor, closed below 50", "wizlock", false, SETUP(imm(g, att); g->circle_restrict = 50;), false, 1},
    {"an implementor", "wizlock 0", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "wizlock 1", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "wizlock 50", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "wizlock 101", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "wizlock 102", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "wizlock -1", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "saveall", false, SETUP(imm(g, att, ADMLVL_IMMORT);), false, 1},
    {"a builder", "saveall", false, SETUP(imm(g, att, ADMLVL_BUILDER);), false, 1},

    /* wizard/world.cpp */
    {"a mortal", "transo item70 Victim", false, SETUP(holding(g, att, "item70");), false, 1},
    {"an immortal", "transo", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "transo item70 Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "transo item70 Nobody", false, SETUP(imm(g, att); holding(g, att, "item70");), false, 1},
    {"an immortal", "transo item70 Victim", false, SETUP(imm(g, att); holding(g, att, "item70"); away(g, victim);), false, 1},
    {"an immortal", "transo item70 all", false, SETUP(imm(g, att); holding(g, att, "item70");), false, 1},
    {"an immortal, with something one of a kind", "transo head all", false, SETUP(imm(g, att); one_of_a_kind(g, att);), false, 1},
    {"a mortal", "recall", false, SETUP(), false, 1},
    {"an immortal", "recall", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "goto", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "goto 300", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "goto 99999", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "goto Victim", false, SETUP(imm(g, att); away(g, victim);), false, 1},
    {"an immortal", "goto 2.chest", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "goto chest", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "goto nothing", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, something carried elsewhere", "goto bauble", false, SETUP(imm(g, att); away(g, victim); holding(g, victim, "bauble");), false, 1},
    {"an immortal, something worn elsewhere", "goto bauble", false, SETUP(imm(g, att); away(g, victim); wearing(g, victim, "bauble", WEAR_FINGER_R);), false, 1},
    {"an immortal, something in a box", "goto bauble", false, SETUP(imm(g, att); struct obj_data *box = thing_here(g, "box", ITEM_CONTAINER); struct obj_data *b = read_object(g, 70); obj_set_name(b, "bauble"); obj_to_obj(g, b, box);), false, 1},
    {"an immortal in a tank", "goto 300", false, SETUP(imm(g, att); char_plr_set(att, PLR_HEALT, true);), false, 1},
    {"a builder, poofing", "goto 300", false, SETUP(imm(g, att, ADMLVL_BUILDER); char_get_player_specials(att)->poofin = "pops in."; char_get_player_specials(att)->poofout = "pops out.";), false, 1},
    {"a builder, to a godroom", "goto 300", false, SETUP(imm(g, att, ADMLVL_BUILDER); room_set_flag(g, 300, ROOM_GODROOM, true);), false, 1},
    {"a builder, to a quest zone", "goto 300", false, SETUP(imm(g, att, ADMLVL_BUILDER); zone_flag(g, 300, ZONE_QUEST);), false, 1},
    {"a builder, to a closed zone", "goto 300", false, SETUP(imm(g, att, ADMLVL_BUILDER); zone_flag(g, 300, ZONE_NOIMMORT);), false, 1},
    {"a vice, to a closed zone", "goto 300", false, SETUP(imm(g, att, ADMLVL_VICE); zone_flag(g, 300, ZONE_NOIMMORT);), false, 1},
    {"an immortal", "at", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "at 300", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "at 99999 look", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "at 300 look", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "at 300 goto 2", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "transfer", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "transfer Nobody", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "transfer self", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "transfer Victim", false, SETUP(imm(g, att); away(g, victim);), false, 1},
    {"an immortal", "transfer dummy", false, SETUP(imm(g, att, ADMLVL_IMMORT);), false, 1},
    {"an immortal, the victim higher", "transfer Victim", false, SETUP(imm(g, att, ADMLVL_IMMORT); imm(g, victim, ADMLVL_GOD); away(g, victim);), false, 1},
    {"an immortal, the victim in a tank", "transfer Victim", false, SETUP(imm(g, att); away(g, victim); char_plr_set(victim, PLR_HEALT, true);), false, 1},
    {"an immortal", "transfer all", false, SETUP(imm(g, att, ADMLVL_IMMORT);), false, 1},
    {"an implementor", "transfer all", false, SETUP(imm(g, att); away(g, victim); away(g, cast.helper); imm(g, cast.helper, ADMLVL_IMPL);), false, 1},
    {"an immortal", "teleport", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "teleport Nobody 300", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "teleport self 300", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, the victim higher", "teleport Victim 300", false, SETUP(imm(g, att, ADMLVL_IMMORT); imm(g, victim, ADMLVL_GOD);), false, 1},
    {"an immortal", "teleport Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "teleport Victim 99999", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "teleport Victim 300", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, the victim in a tank", "teleport Victim 300", false, SETUP(imm(g, att); char_plr_set(victim, PLR_HEALT, true);), false, 1},
    {"an immortal", "snoop", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, snooping", "snoop", false, SETUP(imm(g, att); char_get_desc(att)->snooping = char_get_desc(victim); char_get_desc(victim)->snoop_by = char_get_desc(att);), false, 1},
    {"an immortal", "snoop Nobody", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "snoop dummy", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "snoop self", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "snoop Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, snooping another", "snoop Victim", false, SETUP(imm(g, att); char_get_desc(att)->snooping = char_get_desc(cast.helper); char_get_desc(cast.helper)->snoop_by = char_get_desc(att);), false, 1},
    {"an immortal, the victim snooped", "snoop Victim", false, SETUP(imm(g, att); char_get_desc(cast.helper)->snooping = char_get_desc(victim); char_get_desc(victim)->snoop_by = char_get_desc(cast.helper);), false, 1},
    {"an immortal, snooped by the victim", "snoop Victim", false, SETUP(imm(g, att); char_get_desc(victim)->snooping = char_get_desc(att); char_get_desc(att)->snoop_by = char_get_desc(victim);), false, 1},
    {"an immortal, the victim higher", "snoop Victim", false, SETUP(imm(g, att, ADMLVL_IMMORT); imm(g, victim, ADMLVL_GOD);), false, 1},
    {"an immortal", "switch", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "switch Nobody", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "switch self", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "switch Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "switch dummy", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, the dummy in a godroom", "switch dummy", false, SETUP(imm(g, att, ADMLVL_GOD); room_set_flag(g, ARENA, ROOM_GODROOM, true);), false, 1},
    {"an immortal, by a body left behind", "switch Victim", false, SETUP(imm(g, att); char_get_desc(victim)->character = nullptr; desc_set_connected(char_get_desc(victim), CON_CLOSE); char_set_desc(victim, nullptr);), false, 1},
    {"a god, by a body left behind", "switch Victim", false, SETUP(imm(g, att, ADMLVL_GOD); char_get_desc(victim)->character = nullptr; desc_set_connected(char_get_desc(victim), CON_CLOSE); char_set_desc(victim, nullptr);), false, 1},
    {"an immortal", "return", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, having been in the dummy", "return", false, SETUP(imm(g, att); do_switch(g, att, "dummy", 0, 0); command_interpreter(g, the_dummy(g), "return");), false, 1},
    {"an immortal, having been in the dummy and switched again", "switch", false, SETUP(imm(g, att); do_switch(g, att, "dummy", 0, 0); command_interpreter(g, the_dummy(g), "switch Victim"); command_interpreter(g, the_dummy(g), "return");), false, 1},
    {"an immortal in the dummy, someone in their body", "return", false, SETUP(imm(g, att); do_switch(g, att, "dummy", 0, 0); struct descriptor_data *d = char_get_desc(victim); d->character = att; char_set_desc(att, d); char_set_desc(victim, nullptr);), false, 1},
    {"an immortal", "load", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "load obj x", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "load obj 7x", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "load obj 70 x", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "load thing 70", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "load obj 99999", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "load obj 70", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "load obj 70 3", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "load obj 70 0", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "load obj 70 500", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "load mob 99999", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "load mob 310", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "load mob 310 2", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "purge", false, SETUP(imm(g, att); holding(g, cast.android, "bauble"); char_from_room(g, cast.android); char_to_room(g, cast.android, ARENA); struct char_data *mob = read_mobile(g, 310); char_to_room(g, mob, ARENA); obj_to_char(g, read_object(g, 70), mob); equip_char(g, mob, read_object(g, 301), WEAR_WIELD1); obj_to_room(g, read_object(g, 70), ARENA);), false, 1},
    {"an immortal", "purge Nobody", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "purge dummy", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "purge chest", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "purge Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, the victim away", "purge Victim", false, SETUP(imm(g, att); away(g, victim);), false, 1},
    {"an immortal, the victim as high", "purge Victim", false, SETUP(imm(g, att, ADMLVL_GOD); imm(g, victim, ADMLVL_GOD);), false, 1},
    {"an immortal", "force", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "force Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "force Nobody say hi", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "force Victim say hi", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "force dummy say hi", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, the victim as high", "force Victim say hi", false, SETUP(imm(g, att, ADMLVL_GOD); imm(g, victim, ADMLVL_GOD);), false, 1},
    {"an immortal", "force room say hi", false, SETUP(imm(g, att, ADMLVL_IMMORT);), false, 1},
    {"an implementor", "force room say hi", false, SETUP(imm(g, att); imm(g, cast.helper);), false, 1},
    {"an implementor", "force all say hi", false, SETUP(imm(g, att); imm(g, cast.helper);), false, 1},
    {"an immortal", "zreset", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "zreset .", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "zreset 3", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "zreset 999", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "zreset 3", false, SETUP(imm(g, att, ADMLVL_IMMORT);), false, 1},
    {"an immortal", "zreset *", false, SETUP(imm(g, att, ADMLVL_GOD);), false, 1},
    {"an implementor", "zreset *", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, by a fight", "peace", true, SETUP(imm(g, att); set_fighting(g, victim, cast.helper); imm(g, cast.android); char_from_room(g, cast.android); char_to_room(g, cast.android, ARENA);), false, 1},
    {"a low immortal", "raise Victim", false, SETUP(imm(g, att, ADMLVL_IMMORT);), false, 1},
    {"a mob", "raise Victim", false, SETUP(), true, 1},
    {"a builder", "raise", false, SETUP(imm(g, att, ADMLVL_BUILDER);), false, 1},
    {"a builder", "raise Nobody", false, SETUP(imm(g, att, ADMLVL_BUILDER);), false, 1},
    {"a builder", "raise Victim", false, SETUP(imm(g, att, ADMLVL_BUILDER);), false, 1},
    {"a builder", "raise Victim", false, SETUP(imm(g, att, ADMLVL_BUILDER); the_dead(g, victim);), false, 1},
    {"an immortal", "zpurge", false, SETUP(imm(g, att); obj_to_room(g, read_object(g, 70), ARENA);), false, 1},
    {"an immortal", "zpurge 3", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "zpurge 999", false, SETUP(imm(g, att);), false, 1},
    {"a builder", "zpurge 3", false, SETUP(imm(g, att, ADMLVL_BUILDER);), false, 1},

    /* wizard/players.cpp */
    {"an immortal", "approve", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "approve Nobody", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "approve Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "approve Victim", false, SETUP(imm(g, att); char_plr_set(victim, PLR_BIOGR, true);), false, 1},
    {"an immortal", "lockout", false, SETUP(imm(g, att); lockout_file({});), false, 1},
    {"an immortal", "lockout iovan", false, SETUP(imm(g, att); lockout_file({});), false, 1},
    {"an immortal", "lockout Samael", false, SETUP(imm(g, att); lockout_file({});), false, 1},
    {"an immortal", "lockout list", false, SETUP(imm(g, att); lockout_file({}, -1);), false, 1},
    {"an immortal", "lockout list", false, SETUP(imm(g, att); lockout_file({"Bob"});), false, 1},
    {"an immortal", "lockout list", false, SETUP(imm(g, att); lockout_file({"Bob", "Carl", "Bob", "Dave", "Eve"});), false, 1},
    {"an immortal", "lockout list", false, SETUP(imm(g, att); lockout_file({"Bob", "Carl", "Dave", "Eve"});), false, 1},
    {"an immortal, no lockout file", "lockout bob", false, SETUP(imm(g, att); lockout_file({}, -1);), false, 1},
    {"an immortal", "lockout bob", false, SETUP(imm(g, att); lockout_file({"Carl", "Carl", "Dave"});), false, 1},
    {"an immortal", "lockout bob", false, SETUP(imm(g, att); lockout_file({"Bob", "Carl"});), false, 1},
    {"an immortal", "lockout Victim", false, SETUP(imm(g, att); lockout_file({"Carl"});), false, 1},
    {"an immortal, a long lockout list", "lockout bob", false, SETUP(imm(g, att); lockout_file({}, 600);), false, 1},
    {"an immortal", "reward", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "reward someone 5", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "reward vicuser 0", false, SETUP(imm(g, att); account(g, victim, "vicuser");), false, 1},
    {"an immortal", "reward vicuser 5", false, SETUP(imm(g, att); account(g, victim, "vicuser");), false, 1},
    {"an immortal", "reward vicuser 50", false, SETUP(imm(g, att); account(g, victim, "vicuser");), false, 1},
    {"an immortal", "reward vicuser -5", false, SETUP(imm(g, att); account(g, victim, "vicuser");), false, 1},
    {"an immortal, the user away", "reward vicuser 5", false, SETUP(imm(g, att); account(g, victim, "vicuser", false);), false, 1},
    {"an immortal, the user away", "reward vicuser -5", false, SETUP(imm(g, att); account(g, victim, "vicuser", false);), false, 1},
    {"an immortal", "permission", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "permission unrestrict", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "permission unrestrict saiyan", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "permission unrestrict majin", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "permission unrestrict kai", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "permission restrict", false, SETUP(imm(g, att); g->MAJIN_ALLOWED = true;), false, 1},
    {"an immortal", "permission sideways", false, SETUP(imm(g, att);), false, 1},
    {"a god", "interest", false, SETUP(imm(g, att, ADMLVL_GOD);), false, 1},
    {"a greater god", "interest", false, SETUP(imm(g, att, ADMLVL_GRGOD);), false, 1},
    {"a greater god, interest started", "interest", false, SETUP(imm(g, att, ADMLVL_GRGOD); g->INTERESTTIME = game_now(g) + 500;), false, 1},
    {"an implementor", "advance", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "advance Nobody 5", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "advance dummy 5", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "advance Victim", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "advance Victim x", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "advance Victim demote", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "advance Victim 101", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "advance Victim 20", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "advance Victim 10", false, SETUP(imm(g, att);), false, 1},
    {"an implementor", "advance Victim 25", false, SETUP(imm(g, att);), false, 1},
    {"a god", "handout", false, SETUP(imm(g, att, ADMLVL_BUILDER);), false, 1},
    {"a god", "handout", false, SETUP(imm(g, att, ADMLVL_GOD); imm(g, cast.helper, ADMLVL_IMMORT);), false, 1},
    {"an immortal", "restore", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "restore all", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "restore Nobody", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "restore Victim", false, SETUP(imm(g, att); decCurKI(g, victim, getMaxKI(g, victim) / 2, 0);), false, 1},
    {"an immortal", "restore self", false, SETUP(imm(g, att, ADMLVL_IMMORT);), false, 1},
    {"an immortal, the victim as high", "restore Victim", false, SETUP(imm(g, att, ADMLVL_GOD); imm(g, victim, ADMLVL_GOD);), false, 1},
    {"a mob", "invis", false, SETUP(), true, 1},
    {"an immortal", "invis", false, SETUP(imm(g, att, ADMLVL_GOD); imm(g, victim, ADMLVL_IMMORT); imm(g, cast.helper, ADMLVL_IMPL);), false, 1},
    {"an invisible immortal", "invis", false, SETUP(imm(g, att, ADMLVL_GOD); char_set_invis_level(g, att, 3);), false, 1},
    {"an immortal", "invis 2", false, SETUP(imm(g, att, ADMLVL_GOD); imm(g, victim, ADMLVL_IMMORT); imm(g, cast.helper, ADMLVL_BUILDER);), false, 1},
    {"an invisible immortal", "invis 1", false, SETUP(imm(g, att, ADMLVL_GOD); char_set_invis_level(g, att, 3); imm(g, victim, ADMLVL_IMMORT); imm(g, cast.helper, ADMLVL_BUILDER);), false, 1},
    {"an immortal", "invis 0", false, SETUP(imm(g, att, ADMLVL_GOD); char_set_invis_level(g, att, 3);), false, 1},
    {"an immortal", "invis 6", false, SETUP(imm(g, att, ADMLVL_GOD);), false, 1},
    {"an immortal", "reroll", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "reroll Nobody", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "reroll dummy", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, the victim higher", "reroll Victim", false, SETUP(imm(g, att, ADMLVL_IMMORT); imm(g, victim, ADMLVL_GOD);), false, 1},
    {"an immortal", "reroll Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "notitle Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "mute Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "freeze Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "freeze self", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, the victim as high", "freeze Victim", false, SETUP(imm(g, att, ADMLVL_GOD); imm(g, victim, ADMLVL_GOD);), false, 1},
    {"an immortal, the victim frozen", "freeze Victim", false, SETUP(imm(g, att); char_plr_set(victim, PLR_FROZEN, true);), false, 1},
    {"an immortal", "thaw Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, the victim frozen", "thaw Victim", false, SETUP(imm(g, att); char_plr_set(victim, PLR_FROZEN, true); char_set_freeze_level(g, victim, ADMLVL_GOD);), false, 1},
    {"a builder, the victim frozen by a god", "thaw Victim", false, SETUP(imm(g, att, ADMLVL_BUILDER); char_plr_set(victim, PLR_FROZEN, true); char_set_freeze_level(g, victim, ADMLVL_GOD);), false, 1},
    {"an immortal", "unaffect Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, the victim affected", "unaffect Victim", false, SETUP(imm(g, att); assign_affect(g, victim, AFF_HAYASA, SKILL_HAYASA, 5, 0, 0, 0, 0, 0, 0); char_aff_set(victim, AFF_FLYING, true);), false, 1},
    {"an immortal", "chown", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "chown bauble", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "chown bauble Nobody", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "chown bauble self", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, the victim higher", "chown bauble Victim", false, SETUP(imm(g, att); char_set_class_level(victim, 50);), false, 1},
    {"an immortal", "chown bauble Victim", false, SETUP(imm(g, att); char_set_class_level(att, 50);), false, 1},
    {"an immortal", "chown bauble Victim", false, SETUP(imm(g, att); char_set_class_level(att, 50); holding(g, victim, "bauble");), false, 1},
    {"an immortal", "chown bauble Victim", false, SETUP(imm(g, att); char_set_class_level(att, 50); wearing(g, victim, "bauble", WEAR_FINGER_R);), false, 1},
    {"an immortal, the victim blind", "chown bauble Victim", false, SETUP(imm(g, att); char_set_class_level(att, 50); wearing(g, victim, "bauble", WEAR_FINGER_R); char_aff_set(victim, AFF_BLIND, true);), false, 1},

    /* wizard/info.cpp */
    {"an immortal", "finddoor", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "finddoor 99999", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "finddoor 70", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "finddoor 70", false, SETUP(imm(g, att); key_exit(g, 70);), false, 1},
    {"an immortal", "finddoor nothing", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "finddoor key", false, SETUP(imm(g, att); key_exit(g, 70); holding(g, att, "key");), false, 1},
    {"an immortal", "vnum", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vnum mob", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vnum thing dummy", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vnum mob dummy", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vnum mob zzzz", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vnum m steel", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vnum obj item70", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vnum obj zzzz", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vnum mat zzzz", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vnum wtype sword", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vnum wtype zzzz", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vnum atype light", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vnum atype zzzz", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat room", false, SETUP(imm(g, att); key_exit(g, 70); described(&g->rooms.find(ARENA)->ex_description, "sign"); thing_here(g, "box", ITEM_CONTAINER);), false, 1},
    {"an immortal", "stat mob", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat mob Nobody", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat mob dummy", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat player", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat player Nobody", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat player Victim", false, SETUP(imm(g, att); group(g, victim, cast.helper); assign_affect(g, victim, AFF_HAYASA, SKILL_HAYASA, 5, 0, 0, 0, 0, 0, 0); char_set_bonuses(victim, 0, 1); char_set_bonuses(victim, 20, 1); seated_on(victim, cast.chest);), false, 1},
    {"an immortal", "stat player Victim", false, SETUP(imm(g, att); imm(g, victim, ADMLVL_BUILDER);), false, 1},
    {"an immortal", "stat file", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat file Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat file Zorbexb", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat file Andy", false, SETUP(imm(g, att); char_from_room(g, cast.android); char_to_room(g, cast.android, 300); char_set_invis_level(g, cast.android, 0);), false, 1},
    {"an immortal", "stat object", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat object nothing", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat object chest", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat zone", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat zone 3", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat dummy", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat chest", false, SETUP(imm(g, att); obj_to_obj(g, read_object(g, 70), cast.chest); seated_on(victim, cast.chest);), false, 1},
    {"an immortal", "stat Andy", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat nothing", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); wearing(g, att, "thing", WEAR_FINGER_R);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); enchanted(typed(g, att, ITEM_LIGHT)); obj_set_lload(holding(g, att, "other"), 5);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); obj_set_value(typed(g, att, ITEM_LIGHT), VAL_LIGHT_HOURS, -1);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); typed(g, att, ITEM_SCROLL);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); typed(g, att, ITEM_POTION);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); typed(g, att, ITEM_WAND);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); typed(g, att, ITEM_WEAPON);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); typed(g, att, ITEM_ARMOR);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); typed(g, att, ITEM_TRAP);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); typed(g, att, ITEM_DRINKCON);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); typed(g, att, ITEM_NOTE);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); typed(g, att, ITEM_KEY);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); typed(g, att, ITEM_FOOD);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); typed(g, att, ITEM_MONEY);), false, 1},
    {"an immortal", "stat thing", false, SETUP(imm(g, att); typed(g, att, ITEM_OTHER);), false, 1},
    {"an immortal", "stat tank", false, SETUP(imm(g, att); holding_as(g, att, "tank", 65);), false, 1},
    {"an immortal", "vstat", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vstat obj x", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vstat obj 7x", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vstat obj 99999", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vstat obj 70", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vstat mob 99999", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vstat mob 310", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "vstat thing 70", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "date", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "uptime", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, up a day", "uptime", false, SETUP(imm(g, att); g->boot_time = game_now(g) - 86400 - 3725;), false, 1},
    {"an immortal, up days", "uptime", false, SETUP(imm(g, att); g->boot_time = game_now(g) - 3 * 86400;), false, 1},
    {"an immortal", "last", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "last Zorbexb", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "last Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, the victim higher", "last Victim", false, SETUP(imm(g, att, ADMLVL_GOD); imm(g, victim, ADMLVL_VICE); save_char(g, victim);), false, 1},
    {"an immortal", "players", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "players 1-50", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "players 20", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "players -l 1-10", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "players -n Victim", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "players -i", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "players -m", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "players -d 0-5", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "players -d 5", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "players -h 0-5", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "players -h 5", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "players -x", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "players -", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "players abc", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, a player deleted", "players", false, SETUP(imm(g, att); SET_BIT(g->player_table[0].flags, PINDEX_DELETED);), false, 1},
    {"an immortal", "zcheck", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "zcheck .", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "zcheck 999", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "zcheck 0", false, SETUP(imm(g, att);), false, 1},
    {"an immortal, a room undescribed", "zcheck", false, SETUP(imm(g, att); room_set_description(g, ARENA, "");), false, 1},
    {"an immortal, a mob undescribed", "zcheck", false, SETUP(imm(g, att); char_set_ldesc(mob_proto_of(g, 310), ""); char_set_alias(mob_proto_of(g, 310), ""); char_set_sdesc(mob_proto_of(g, 310), "");), false, 1},
    {"an immortal, things amiss", "zcheck", false, SETUP(imm(g, att); struct char_data *m = mob_proto_of(g, 310); char_set_alias(m, "mob unfinished"); char_set_sdesc(m, "the unfinished mob"); char_set_ldesc(m, "An unfinished mob stands here.\r\n"); char_set_ddesc(m, "It looks unfinished."); char_set_gold(m, 99999999); char_set_exp(m, 999999999); char_mob_set(m, MOB_SPEC, true); char_mob_set(m, MOB_AGGRESSIVE, true); char_mob_set(m, MOB_AGGR_GOOD, true); char_aff_set(m, AFF_GROUP, true); struct obj_data *o = obj_proto_of(g, 301); obj_set_value(o, 3, 999); obj_set_value(o, 1, 50); obj_set_value(o, 2, 50); obj_set_cost(o, 0); obj_set_weight(o, 0); obj_set_level(o, 50); obj_set_action_description(o, "acts"); enchanted(o); described(&g->rooms.find(ARENA)->ex_description, "sign"); room_set_flag(g, ARENA, ROOM_ATRIUM, true); exit_to(g, NORTH, 2);), false, 1},
    {"an immortal", "checkload", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "checkload m x", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "checkload m 99999", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "checkload m 310", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "checkload o 99999", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "checkload o 70", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "checkload o 300", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "checkload t 99999", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "checkload t 300", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "checkload x 5", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "findkey", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "findkey sideways", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "findkey north", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "findkey north", false, SETUP(imm(g, att); key_exit(g, NOTHING);), false, 1},
    {"an immortal", "findkey n", false, SETUP(imm(g, att); key_exit(g, 70);), false, 1},
    {"an immortal", "spells", false, SETUP(imm(g, att);), false, 1},
    {"an immortal", "audit", false, SETUP(imm(g, att);), false, 1},
  };
} // namespace

extern const std::span<const scene> SCENES_WIZARD = list;
