/* Scenes for test_techniques: see scene_kit.hpp. These are act.comm.cpp's. */
#include "scene_kit.hpp"

#include "dbat/config.hpp"
#include "dbat/act.comm.hpp"
#include "dbat/interpreter.hpp"

namespace {
  void languages_off(GameWorld *g) { CONFIG_ENABLE_LANGUAGES = false; }

  /* ... speaking a language, known this well */
  void speaks(struct char_data *ch, int language, int skill = 50)
  {
    char_set_skills_base(ch, language, skill);
    char_set_speaking(ch, language);
  }

  /* flag is a pref_flag (an int on a base from before pref_flag) */
  void prefers(GameWorld *g, struct char_data *ch, auto flag) { char_prf_set(g, ch, flag, true); }

  /* ... known by this account name */
  void account(struct char_data *ch, const char *user)
  {
    if (char_get_desc(ch))
      th_account(char_get_desc(ch)).user = user;
  }

  /* ... the victim known as Namek, and the helper as Help */
  void accounts(struct char_data *att, struct char_data *victim)
  {
    account(att, "Me");
    account(victim, "Namek");
    account(cast.helper, "Helper");
  }

  /* ... leading a group of the victim and the helper, with one of them asleep if asked */
  void party(GameWorld *g, struct char_data *att, struct char_data *victim, bool asleep = false)
  {
    group(g, att, victim);
    group(g, att, cast.helper);
    if (asleep)
      char_set_position(cast.helper, POS_SLEEPING);
  }

  /* ... with the victim and helper listening in, this well */
  void listeners(struct char_data *victim, int skill)
  {
    char_set_skills_base(victim, SKILL_LISTEN, skill);
    char_set_skills_base(cast.helper, SKILL_LISTEN, skill);
  }

  struct obj_data *thing(GameWorld *g, struct char_data *ch, const char *name, int type)
  {
    struct obj_data *obj = read_object(g, 70);
    obj_set_name(obj, name);
    obj_set_type_flag(obj, type);
    obj_to_char(g, obj, ch);
    return obj;
  }

  struct obj_data *held(GameWorld *g, struct char_data *ch, const char *name, int type)
  {
    struct obj_data *obj = read_object(g, 70);
    obj_set_name(obj, name);
    obj_set_type_flag(obj, type);
    equip_char(g, ch, obj, WEAR_WIELD2);
    return obj;
  }

  void board_here(GameWorld *g)
  {
    struct obj_data *obj = read_object(g, 70);
    obj_set_name(obj, "board");
    obj_set_type_flag(obj, ITEM_BOARD);
    obj_to_room(g, obj, ARENA);
  }

  const scene list[] = {
    /* voice */
    {"mute", "voice gravelly", false, SETUP(char_set_bonuses(att, BONUS_MUTE, 1);), false, 1},
    {"", "voice", false, SETUP(), false, 1},
    {"", "voice a voice so long that it goes on and on and on past the seventy five characters allowed", false, SETUP(), false, 1},
    {"", "voice @Rred", false, SETUP(), false, 1},
    {"", "voice gravelly", false, SETUP(), false, 1},
    {"voiced already, without rpp", "voice gravelly", false, SETUP(char_set_voice(att, "squeaky"); char_set_rp(att, 0);), false, 1},
    {"voiced already", "voice gravelly", false, SETUP(char_set_voice(att, "squeaky"); char_set_rp(att, 3);), false, 1},

    /* languages and speak */
    {"where all understand", "languages", false, SETUP(languages_off(g);), false, 1},
    {"", "languages", false, SETUP(), false, 1},
    {"knowing elven and dwarven", "languages", false, SETUP(speaks(att, SKILL_LANG_ELVEN); char_set_skills_base(att, SKILL_LANG_DWARVEN, 10);), false, 1},
    {"knowing elven", "speak elven", false, SETUP(char_set_skills_base(att, SKILL_LANG_ELVEN, 50);), false, 1},
    {"knowing elven", "speak dwarven", false, SETUP(char_set_skills_base(att, SKILL_LANG_ELVEN, 50);), false, 1},
    {"knowing elven", "speak klingon", false, SETUP(char_set_skills_base(att, SKILL_LANG_ELVEN, 50);), false, 1},
    {"knowing draconic", "speak draconic", false, SETUP(char_set_skills_base(att, SKILL_LANG_DRACONIC, 50);), false, 1},

    /* osay */
    {"", "osay", false, SETUP(), false, 1},
    {"", "osay hello there", false, SETUP(), false, 1},
    {"hidden", "osay hello there", false, SETUP(prefers(g, att, PRF_HIDE);), false, 1},
    {"an immortal", "osay hello there", false, SETUP(admin_set(g, att, ADMLVL_IMMORT);), false, 1},

    /* gsay and gtell */
    {"", "gsay hello", false, SETUP(), false, 1},
    {"grouped, in the arena", "gsay hello", false, SETUP(party(g, att, victim); moved(g, att, 17800);), false, 1},
    {"leading a group", "gsay", false, SETUP(party(g, att, victim);), false, 1},
    {"leading a group", "gsay hello there friends", false, SETUP(party(g, att, victim);), false, 2},
    {"leading a group, one asleep", "gtell hello there friends", false, SETUP(party(g, att, victim, true);), false, 1},
    {"leading a group, without repeats", "gsay hello there friends", false, SETUP(party(g, att, victim); prefers(g, att, PRF_NOREPEAT);), false, 1},
    {"in a group", "gsay hello there friends", false, SETUP(group(g, victim, att); group(g, victim, cast.helper);), false, 2},
    {"in a group, the leader asleep", "gsay hello there friends", false, SETUP(group(g, victim, att); group(g, victim, cast.helper); char_set_position(victim, POS_SLEEPING);), false, 1},
    {"in a group, speaking elven", "gsay hello there friends", false, SETUP(group(g, victim, att); group(g, victim, cast.helper); speaks(att, SKILL_LANG_ELVEN); char_set_skills_base(victim, SKILL_LANG_ELVEN, 50);), false, 2},
    {"leading a group, speaking elven", "gsay hello there friends", false, SETUP(party(g, att, victim); speaks(att, SKILL_LANG_ELVEN); char_set_skills_base(victim, SKILL_LANG_ELVEN, 50);), false, 2},
    {"leading a group, where all understand", "gsay hello there friends", false, SETUP(party(g, att, victim); languages_off(g);), false, 1},
    {"leading a group, one asleep, where all understand", "gsay hello there friends", false, SETUP(party(g, att, victim, true); languages_off(g);), false, 1},
    {"in a group, where all understand", "gsay hello there friends", false, SETUP(group(g, victim, att); group(g, victim, cast.helper); languages_off(g);), false, 1},
    {"leading the dummy", "gsay hello there", false, SETUP(for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { group(g, att, c); break; }), false, 1},

    /* tell */
    {"", "tell", false, SETUP(), false, 1},
    {"", "tell namek", false, SETUP(), false, 1},
    {"", "tell nobody hello", false, SETUP(accounts(att, victim);), false, 1},
    {"", "tell namek hello", false, SETUP(accounts(att, victim);), false, 2},
    {"", "tell nam hello", false, SETUP(accounts(att, victim);), false, 1},
    {"", "tell me hello", false, SETUP(accounts(att, victim);), false, 1},
    {"", "tell victim hello", false, SETUP(accounts(att, victim);), false, 1},
    {"", "tell victim hello", false, SETUP(accounts(att, victim); admin_set(g, victim, ADMLVL_IMMORT);), false, 1},
    {"a beginner", "tell namek hello", false, SETUP(accounts(att, victim); char_set_class_level(att, 2);), false, 1},
    {"a beginner", "tell namek hello", false, SETUP(accounts(att, victim); char_set_class_level(att, 2); admin_set(g, victim, ADMLVL_IMMORT);), false, 1},
    {"away", "tell namek hello", false, SETUP(accounts(att, victim); prefers(g, att, PRF_AFK);), false, 1},
    {"", "tell namek hello, are you away?", false, SETUP(accounts(att, victim); prefers(g, victim, PRF_AFK);), false, 1},
    {"an immortal", "tell namek hello, are you away?", false, SETUP(accounts(att, victim); prefers(g, victim, PRF_AFK); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"", "tell namek hello, beginner", false, SETUP(accounts(att, victim); char_set_class_level(victim, 2);), false, 1},
    {"refusing tells", "tell namek hello", false, SETUP(accounts(att, victim); prefers(g, att, PRF_NOTELL);), false, 1},
    {"in a soundproof room", "tell namek hello", false, SETUP(accounts(att, victim); room_set_flag(g, ARENA, ROOM_SOUNDPROOF, true); away(g, victim);), false, 1},
    {"", "tell namek hello, writer", false, SETUP(accounts(att, victim); char_plr_set(victim, PLR_WRITING, true);), false, 1},
    {"", "tell namek hello, refuser", false, SETUP(accounts(att, victim); prefers(g, victim, PRF_NOTELL);), false, 1},
    {"", "tell namek hello, in there", false, SETUP(accounts(att, victim); away(g, victim); room_set_flag(g, 300, ROOM_SOUNDPROOF, true);), false, 1},
    {"without repeats", "tell namek hello", false, SETUP(accounts(att, victim); prefers(g, att, PRF_NOREPEAT);), false, 1},
    {"where all understand", "tell namek hello", false, SETUP(accounts(att, victim); languages_off(g);), false, 1},
    {"an immortal, where all understand", "tell namek hello", false, SETUP(accounts(att, victim); languages_off(g); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"where all understand", "tell victim hello", false, SETUP(accounts(att, victim); languages_off(g); admin_set(g, victim, ADMLVL_IMMORT);), false, 1},
    {"an implementor", "tell namek hello", false, SETUP(accounts(att, victim); admin_set(g, att, ADMLVL_IMPL);), false, 1},
    {"speaking elven", "tell namek hello", false, SETUP(accounts(att, victim); speaks(att, SKILL_LANG_ELVEN);), false, 1},
    {"speaking elven, unseen", "tell namek hello", false, SETUP(accounts(att, victim); speaks(att, SKILL_LANG_ELVEN); char_set_skills_base(victim, SKILL_LANG_ELVEN, 50); char_aff_set(att, AFF_INVISIBLE, true);), false, 1},
    {"watching the dummy tell", "tell", false, SETUP(accounts(att, victim); for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { command_interpreter(g, c, "tell victim hello"); command_interpreter(g, c, "tell nobody hello"); break; }), false, 1},
    {"watching the dummy tell, where all understand", "tell", false, SETUP(accounts(att, victim); languages_off(g); for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { command_interpreter(g, c, "tell victim hello"); break; }), false, 1},

    /* reply */
    {"in the time chamber", "reply hello", false, SETUP(room_set_flag(g, ARENA, ROOM_HBTC, true);), false, 1},
    {"in the past", "reply hello", false, SETUP(room_set_flag(g, ARENA, ROOM_PAST, true);), false, 1},
    {"told nothing", "reply hello", false, SETUP(char_set_last_tell(g, att, NOBODY);), false, 1},
    {"told something", "reply", false, SETUP(char_set_last_tell(g, att, char_get_idnum(victim));), false, 1},
    {"told something", "reply hello", false, SETUP(accounts(att, victim); char_set_last_tell(g, att, char_get_idnum(victim));), false, 1},
    {"told something by someone gone", "reply hello", false, SETUP(char_set_last_tell(g, att, 999999);), false, 1},
    {"told something, away", "reply hello", false, SETUP(char_set_last_tell(g, att, char_get_idnum(victim)); prefers(g, att, PRF_AFK);), false, 1},
    {"told something", "tell namek hi", false, SETUP(accounts(att, victim); command_interpreter(g, victim, "tell me hello"); command_interpreter(g, att, "reply hi yourself");), false, 1},

    /* whisper and ask */
    {"mute", "whisper namek hello", false, SETUP(char_set_bonuses(att, BONUS_MUTE, 1);), false, 1},
    {"", "whisper", false, SETUP(), false, 1},
    {"", "ask namek", false, SETUP(), false, 1},
    {"", "whisper nobody hello", false, SETUP(), false, 1},
    {"", "whisper self hello", false, SETUP(), false, 1},
    {"", "whisper namek hello there", false, SETUP(), false, 1},
    {"", "ask namek are you there?", false, SETUP(), false, 1},
    {"without repeats", "ask namek are you there?", false, SETUP(prefers(g, att, PRF_NOREPEAT);), false, 1},
    {"where all understand", "whisper namek hello there", false, SETUP(languages_off(g);), false, 1},
    {"speaking elven", "whisper namek hello there", false, SETUP(speaks(att, SKILL_LANG_ELVEN);), false, 1},
    {"overheard keenly", "whisper namek the password is swordfish, keep it quiet", false, SETUP(char_set_skills_base(cast.helper, SKILL_LISTEN, 100); char_set_skills_base(victim, SKILL_LISTEN, 100);), false, 4},
    {"overheard well", "whisper namek the password is swordfish, keep it quiet", false, SETUP(char_set_skills_base(cast.helper, SKILL_LISTEN, 85);), false, 8},
    {"overheard somewhat", "whisper namek the password is swordfish, keep it quiet", false, SETUP(char_set_skills_base(cast.helper, SKILL_LISTEN, 60);), false, 8},
    {"overheard poorly", "whisper namek the password is swordfish, keep it quiet", false, SETUP(char_set_skills_base(cast.helper, SKILL_LISTEN, 35);), false, 12},
    {"overheard barely", "whisper namek the password is swordfish, keep it quiet", false, SETUP(char_set_skills_base(cast.helper, SKILL_LISTEN, 10);), false, 12},
    {"overheard by all", "whisper namek ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz", false, SETUP(listeners(victim, 30); for (char_data *c : room_people(g, ARENA)) if (!char_is_npc(c)) char_set_skills_base(c, SKILL_LISTEN, 30);), false, 12},

    /* write */
    {"by a board", "write hello", false, SETUP(board_here(g);), false, 1},
    {"", "write", false, SETUP(), false, 1},
    {"", "write paper pen", false, SETUP(), false, 1},
    {"with paper", "write paper pen", false, SETUP(thing(g, att, "paper", ITEM_NOTE);), false, 1},
    {"with paper and a pen", "write paper pen", false, SETUP(thing(g, att, "paper", ITEM_NOTE); thing(g, att, "pen", ITEM_PEN);), false, 1},
    {"with paper and a stick", "write paper stick", false, SETUP(thing(g, att, "paper", ITEM_NOTE); thing(g, att, "stick", ITEM_OTHER);), false, 1},
    {"with a rock and a pen", "write rock pen", false, SETUP(thing(g, att, "rock", ITEM_OTHER); thing(g, att, "pen", ITEM_PEN);), false, 1},
    {"with written paper and a pen", "write paper pen", false, SETUP(obj_set_action_description(thing(g, att, "paper", ITEM_NOTE), "Old words.\r\n"); thing(g, att, "pen", ITEM_PEN);), false, 1},
    {"", "write paper", false, SETUP(), false, 1},
    {"with a rock", "write rock", false, SETUP(thing(g, att, "rock", ITEM_OTHER);), false, 1},
    {"with paper", "write paper", false, SETUP(thing(g, att, "paper", ITEM_NOTE);), false, 1},
    {"with paper, holding a pen", "write paper", false, SETUP(thing(g, att, "paper", ITEM_NOTE); held(g, att, "pen", ITEM_PEN);), false, 1},
    {"with paper, holding an unseen pen", "write paper", false, SETUP(thing(g, att, "paper", ITEM_NOTE); obj_extra_set(held(g, att, "pen", ITEM_PEN), ITEM_INVISIBLE, true);), false, 1},
    {"with paper, holding a stick", "write paper", false, SETUP(thing(g, att, "paper", ITEM_NOTE); held(g, att, "stick", ITEM_OTHER);), false, 1},
    {"with a pen, holding paper", "write pen", false, SETUP(thing(g, att, "pen", ITEM_PEN); held(g, att, "paper", ITEM_NOTE);), false, 1},
    {"with a pen, holding a rock", "write pen", false, SETUP(thing(g, att, "pen", ITEM_PEN); held(g, att, "rock", ITEM_OTHER);), false, 1},
    {"watching the dummy write", "write", false, SETUP(for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { do_write(g, c, "paper pen", 0, 0); break; }), false, 1},

    /* page */
    {"a builder", "page", false, SETUP(admin_set(g, att, ADMLVL_BUILDER);), false, 1},
    {"a builder", "page nobody hello", false, SETUP(admin_set(g, att, ADMLVL_BUILDER);), false, 1},
    {"a builder", "page namek hello", false, SETUP(admin_set(g, att, ADMLVL_BUILDER);), false, 1},
    {"a builder, without repeats", "page namek hello", false, SETUP(admin_set(g, att, ADMLVL_BUILDER); prefers(g, att, PRF_NOREPEAT);), false, 1},
    {"a builder", "page all hello", false, SETUP(admin_set(g, att, ADMLVL_BUILDER);), false, 1},
    {"an implementor", "page all hello", false, SETUP(admin_set(g, att, ADMLVL_IMPL);), false, 1},
    {"watching the dummy page", "page", false, SETUP(admin_set(g, att, ADMLVL_BUILDER); for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { do_page(g, c, "namek hello", 0, 0); break; }), false, 1},

    /* the channels: music, shout, ooc, newbie, grats and gemote */
    {"silenced", "ooc hello", false, SETUP(char_plr_set(att, PLR_NOSHOUT, true);), false, 1},
    {"silenced", "gemote waves", false, SETUP(char_plr_set(att, PLR_NOSHOUT, true);), false, 1},
    {"in a soundproof room", "shout hello", false, SETUP(room_set_flag(g, ARENA, ROOM_SOUNDPROOF, true);), false, 1},
    {"mute", "shout hello", false, SETUP(char_set_bonuses(att, BONUS_MUTE, 1);), false, 1},
    {"mute", "ooc hello", false, SETUP(char_set_bonuses(att, BONUS_MUTE, 1);), false, 1},
    {"", "ooc *waves", false, SETUP(), false, 1},
    {"", "gemote waves", false, SETUP(), false, 1},
    {"", "gemote :waves", false, SETUP(), false, 1},
    {"too young", "ooc hello", false, SETUP(CONFIG_LEVEL_CAN_SHOUT = 30;), false, 1},
    {"off the channel", "ooc hello", false, SETUP(prefers(g, att, PRF_NOGOSS);), false, 1},
    {"deaf", "shout hello", false, SETUP(prefers(g, att, PRF_DEAF);), false, 1},
    {"", "ooc", false, SETUP(), false, 1},
    {"", "ooc hello $$ everyone", false, SETUP(), false, 2},
    {"", "music la la la", false, SETUP(), false, 1},
    {"", "newbie how do I play?", false, SETUP(), false, 1},
    {"", "grats well done", false, SETUP(), false, 1},
    {"", "shout hello", false, SETUP(away(g, victim);), false, 2},
    {"with one asleep", "shout hello", false, SETUP(char_set_position(victim, POS_SLEEPING);), false, 1},
    {"with one far away", "shout hello", false, SETUP(moved(g, victim, 1960);), false, 1},
    {"without repeats", "ooc hello", false, SETUP(prefers(g, att, PRF_NOREPEAT);), false, 1},
    {"with some off the channel", "ooc hello", false, SETUP(prefers(g, victim, PRF_NOGOSS);), false, 1},
    {"with some writing", "ooc hello", false, SETUP(char_plr_set(victim, PLR_WRITING, true);), false, 1},
    {"with some in a soundproof room", "ooc hello", false, SETUP(away(g, victim); room_set_flag(g, 300, ROOM_SOUNDPROOF, true);), false, 1},
    {"speaking dwarven", "ooc hello there", false, SETUP(speaks(att, SKILL_LANG_DWARVEN);), false, 2},
    {"speaking elven", "ooc hello there", false, SETUP(speaks(att, SKILL_LANG_ELVEN); char_set_skills_base(victim, SKILL_LANG_ELVEN, 50);), false, 2},
    {"an immortal", "ooc hello", false, SETUP(admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"where all understand", "ooc hello", false, SETUP(languages_off(g);), false, 1},
    {"where all understand", "shout hello", false, SETUP(languages_off(g); away(g, victim);), false, 1},
    {"an immortal, where all understand", "ooc hello", false, SETUP(languages_off(g); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"hidden, where all understand", "ooc hello", false, SETUP(languages_off(g); prefers(g, att, PRF_HIDE); admin_set(g, victim, ADMLVL_IMMORT);), false, 1},
    {"where all understand, before an immortal", "ooc hello", false, SETUP(languages_off(g); admin_set(g, victim, ADMLVL_IMMORT);), false, 1},
    {"spamming", "ooc hello", false, SETUP(char_set_spam(att, 2);), false, 1},
    {"spamming on", "ooc hello", false, SETUP(char_set_spam(att, 3);), false, 1},
    {"an immortal, spamming on", "ooc hello", false, SETUP(char_set_spam(att, 3); admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"watching the dummy ooc", "ooc", false, SETUP(for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { do_gen_comm(g, c, "hello", 0, SCMD_GOSSIP); break; }), false, 1},

    /* respond */
    {"", "respond 1", false, SETUP(), false, 1},
    {"by a board", "respond", false, SETUP(board_here(g);), false, 1},
    {"by a board", "respond first", false, SETUP(board_here(g);), false, 1},
    {"by a board", "respond 0", false, SETUP(board_here(g);), false, 1},
    {"by a board", "respond 1", false, SETUP(board_here(g);), false, 1},
    {"watching the dummy respond", "respond", false, SETUP(for (char_data *c : room_people(g, ARENA)) if (char_is_npc(c) && isname("dummy", char_get_alias(c))) { do_respond(g, c, "1", 0, 0); break; }), false, 1},
  };
} // namespace

const std::span<const scene> SCENES_COMM = list;
