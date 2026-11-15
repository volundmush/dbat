/* Scenes for test_techniques: see scene_kit.hpp. These are act.social.cpp's: the socials (the fixture has only nod),
   gemote, and insult, which no command reaches. */
#include "scene_kit.hpp"

#include "dbat/act.comm.hpp"
#include "dbat/act.social.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/social.hpp"

namespace {
  struct social_messg *nod(GameWorld *g) { return &g->soc_mess_list[0]; }

  /* ... nod made into a social of body parts too */
  void bodily(GameWorld *g)
  {
    nod(g)->char_body_found   = strdup("You nod at $N's $t.");
    nod(g)->others_body_found = strdup("$n nods at $N's $t.");
    nod(g)->vict_body_found   = strdup("$n nods at your $t.");
  }

  /* ... nod with this message gone */
  void without(GameWorld *g, char *social_messg::*text)
  {
    free(nod(g)->*text);
    nod(g)->*text = nullptr;
  }

  void sexes(struct char_data *att, struct char_data *victim, int a, int v)
  {
    char_set_sex(att, a);
    char_set_sex(victim, v);
  }

  const scene list[] = {
    /* nod */
    {"", "nod", false, SETUP(), false, 1},
    {"", "nod namek", false, SETUP(), false, 1},
    {"", "nod namek head", false, SETUP(), false, 1},
    {"with nods of body parts", "nod namek head", false, SETUP(bodily(g);), false, 1},
    {"with nods of body parts", "nod namek", false, SETUP(bodily(g);), false, 1},
    {"", "nod self", false, SETUP(), false, 1},
    {"without a nod to oneself", "nod self", false, SETUP(without(g, &social_messg::char_auto);), false, 1},
    {"", "nod nobody", false, SETUP(), false, 1},
    {"without a nod to no one", "nod nobody", false, SETUP(without(g, &social_messg::not_found);), false, 1},
    {"", "nod chest", false, SETUP(), false, 1},
    {"with a pebble", "nod pebble", false, SETUP(holding(g, att, "pebble");), false, 1},
    {"without nods at things", "nod chest", false, SETUP(without(g, &social_messg::char_obj_found);), false, 1},
    {"without nods at anyone", "nod namek", false, SETUP(without(g, &social_messg::char_found);), false, 1},
    {"by a sleeper", "nod namek", false, SETUP(char_set_position(victim, POS_SLEEPING); nod(g)->min_victim_position = POS_STANDING;), false, 1},
    {"by a sleeper, with nods of body parts", "nod namek head", false, SETUP(bodily(g); char_set_position(victim, POS_SLEEPING); nod(g)->min_victim_position = POS_STANDING;), false, 1},

    /* gemote, and ooc's * */
    {"", "gemote nod", false, SETUP(), false, 1},
    {"", "gemote nod namek", false, SETUP(), false, 1},
    {"", "gemote nod self", false, SETUP(), false, 1},
    {"", "gemote nod nobody", false, SETUP(), false, 1},
    {"by a sleeper", "gemote nod namek", false, SETUP(char_set_position(victim, POS_SLEEPING); nod(g)->min_victim_position = POS_STANDING;), false, 1},
    {"without a nod to the room", "gemote nod", false, SETUP(free(nod(g)->others_no_arg); nod(g)->others_no_arg = strdup("");), false, 1},
    {"without a nod to oneself seen", "gemote nod self", false, SETUP(without(g, &social_messg::others_auto);), false, 1},
    {"without nods at anyone", "gemote nod namek", false, SETUP(without(g, &social_messg::char_found);), false, 1},
    {"in a soundproof room", "gemote nod", false, SETUP(room_set_flag(g, ARENA, ROOM_SOUNDPROOF, true); do_gmote(g, att, "nod", 0, 1);), false, 1},
    {"", "gemote waves happily", false, SETUP(), false, 1},
    {"an immortal", "gemote waves happily", false, SETUP(admin_set(g, att, ADMLVL_IMMORT);), false, 1},
    {"", "ooc *nod namek", false, SETUP(), false, 1},
    {"", "gemote no", false, SETUP(), false, 1},

    /* insult, called by the setup */
    {"insulting", "look", false, SETUP(do_insult(g, att, "", 0, 0); do_insult(g, att, "nobody", 0, 0); do_insult(g, att, "self", 0, 0);), false, 1},
    {"a man insulting a man", "look", false, SETUP(sexes(att, victim, SEX_MALE, SEX_MALE); do_insult(g, att, "namek", 0, 0);), false, 6},
    {"a man insulting a woman", "look", false, SETUP(sexes(att, victim, SEX_MALE, SEX_FEMALE); do_insult(g, att, "namek", 0, 0);), false, 6},
    {"a woman insulting a man", "look", false, SETUP(sexes(att, victim, SEX_FEMALE, SEX_MALE); do_insult(g, att, "namek", 0, 0);), false, 6},
    {"a woman insulting a woman", "look", false, SETUP(sexes(att, victim, SEX_FEMALE, SEX_FEMALE); do_insult(g, att, "namek", 0, 0);), false, 6},

    /* command_interpreter's checks of the one typing (command.cpp) */
    {"liquefied", "south", false, SETUP(char_aff_set(att, AFF_LIQUEFIED, true);), false, 1},
    {"liquefied", "look", false, SETUP(char_aff_set(att, AFF_LIQUEFIED, true);), false, 1},
    {"liquefied", "ingest", false, SETUP(char_aff_set(att, AFF_LIQUEFIED, true);), false, 1},
    {"petrified", "south", false, SETUP(assign_affect(g, att, AFF_PARALYZE, 0, 5, 0, 0, 0, 0, 0, 0);), false, 1},
    {"petrified", "score", false, SETUP(assign_affect(g, att, AFF_PARALYZE, 0, 5, 0, 0, 0, 0, 0, 0);), false, 1},
    {"petrified", "ingest", false, SETUP(assign_affect(g, att, AFF_PARALYZE, 0, 5, 0, 0, 0, 0, 0, 0);), false, 1},
    {"frozen in ice", "south", false, SETUP(assign_affect(g, att, AFF_FROZEN, 0, 5, 0, 0, 0, 0, 0, 0);), false, 1},
    {"frozen in ice", "status", false, SETUP(assign_affect(g, att, AFF_FROZEN, 0, 5, 0, 0, 0, 0, 0, 0);), false, 1},
    {"paralysed", "south", false, SETUP(assign_affect(g, att, AFF_PARA, 0, 5, 0, 0, 0, 0, 0, 0);), false, 6},
    {"paralysed", "osay hi", false, SETUP(assign_affect(g, att, AFF_PARA, 0, 5, 0, 0, 0, 0, 0, 0);), false, 4},
    {"an immortal, petrified", "south", false, SETUP(admin_set(g, att, ADMLVL_IMMORT); assign_affect(g, att, AFF_PARALYZE, 0, 5, 0, 0, 0, 0, 0, 0);), false, 1},
    {"asleep", "carry namek", false, SETUP(char_set_position(att, POS_SLEEPING);), false, 1},
    {"resting", "carry namek", false, SETUP(char_set_position(att, POS_RESTING);), false, 1},
    {"sitting", "carry namek", false, SETUP(char_set_position(att, POS_SITTING);), false, 1},
    {"stunned", "carry namek", false, SETUP(char_set_position(att, POS_STUNNED);), false, 1},
  };
} // namespace

const std::span<const scene> SCENES_SOCIAL = list;
