/* Verification helper (not part of the server build).
 * Called from gdb after boot by verify.sh, before roundtrip_dump(): writes statedump.txt, a canonical text dump of
 * what the boot loaded that no save file shows, so that two builds can be compared on it. The save files themselves
 * are compared by roundtrip_dump(); this covers what is only read (help, socials, levels, bans, the mail file and the
 * rest), what the loaders work out or fix up (room gravity, mob stats, portal locks) and the state the zone resets
 * leave. Long texts are given by their length and a hash. */
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>

#include "dbat/utils.hpp"
#include "dbat/db.hpp"
#include "dbat/ban.hpp"
#include "dbat/boards.hpp"
#include "dbat/disabled.hpp"
#include "dbat/house.hpp"
#include "dbat/mail.hpp"
#include "dbat/social.hpp"
#include "dbat/assemblies.hpp"
#include "dbat/interpreter.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/game_world.hpp"

namespace {

FILE *out;

/* a text, short, or as its length and FNV-1a hash */
std::string text(std::string_view s)
{
  if (s.size() <= 60 && s.find_first_of("\r\n") == std::string_view::npos)
    return str_format("'{}'", s);
  unsigned long long h = 14695981039346656037ull;
  for (unsigned char c : s)
    h = (h ^ c) * 1099511628211ull;
  return str_format("<{:d} bytes {:016x}>", s.size(), h);
}
std::string text(const char *s) { return s ? text(std::string_view(s)) : "(null)"; }
std::string text(const std::string &s) { return text(std::string_view(s)); }
std::string text(const std::optional<std::string> &s) { return s ? text(*s) : "(null)"; }

template <size_t N>
std::string bits(const bitvector_t (&flags)[N])
{
  std::string s;
  for (size_t i = 0; i < N; i++)
    s += str_format("{}{:d}", i ? "," : "", (long long)flags[i]);
  return s;
}
template <flag_family E>
std::string bits(const flag_set<E> &flags)
{
  std::string s;
  for (size_t i = 0; i < FLAG_WORDS; i++)
    s += str_format("{}{:d}", i ? "," : "", static_cast<long long>(flags.word(i)));
  return s;
}
std::string bits(const bitvector_t *flags, size_t n)
{
  std::string s;
  for (size_t i = 0; i < n; i++)
    s += str_format("{}{:d}", i ? "," : "", (long long)flags[i]);
  return s;
}

void ex_descs(const ex_desc_list &list)
{
  for (const extra_descr_data &e : list)
    file_print(out, "  extra {} {}\n", text(e.keyword), text(e.description));
}

void scripts(const std::vector<trig_vnum> &protos)
{
  if (protos.empty())
    return;
  std::string s;
  for (trig_vnum t : protos)
    s += str_format(" {:d}", t);
  file_print(out, "  triggers{}\n", s);
}

void zones(GameWorld *g)
{
  for (struct zone_data *z : g->zones.all()) {
    file_print(out, "zone {:d} {} builders {} life {:d} age {:d} rooms {:d}-{:d} mode {:d} levels {:d}-{:d} flags {}\n",
               z->number, text(z->name), text(z->builders), z->lifespan, z->age, z->bot, z->top, z->reset_mode,
               z->min_level, z->max_level, bits(z->zone_flags));
    for (const reset_com &c : z->cmd) /* a command without a global name or value has none: "(null)", as the char * had it */
      file_print(out, "  {:c} if {:d} {:d} {:d} {:d} {:d} {:d} line {:d} {} {}\n", as_char(c.command), (int)c.if_flag,
                 c.arg1, c.arg2, c.arg3, c.arg4, c.arg5, c.line, c.sarg1.empty() ? "(null)" : text(c.sarg1),
                 c.sarg2.empty() ? "(null)" : text(c.sarg2));
  }
}

void rooms(GameWorld *g)
{
  g->rooms.visit([&](room_vnum vnum, const struct room_data *r) {
    file_print(out, "room {:d} zone {:d} sector {:d} flags {} light {:d} timed {:d} dmg {:d} gravity {:d} geffect {:d} func {:d}\n",
               vnum, r->zone ? r->zone->number : -1, r->sector_type, bits(r->room_flags), r->light, r->timed, r->dmg,
               r->gravity, r->geffect, r->func != nullptr);
    file_print(out, "  {} {}\n", text(r->name), text(r->description));
    for (int d = 0; d < NUM_OF_DIRS; d++)
      if (const room_direction_data *e = r->dir_option[d].get())
        file_print(out, "  exit {:d} to {:d} key {:d} info {:d} dc {:d} {:d} {:d} {:d} save {:d} {:d} fail {:d} {:d} {} {}\n",
                   d, e->to_room, e->key, e->exit_info, e->dclock, e->dchide, e->dcskill, e->dcmove, e->failsavetype,
                   e->dcfailsave, e->failroom, e->totalfailroom, text(e->keyword), text(e->general_description));
    ex_descs(r->ex_description);
    scripts(r->proto_script);
    std::string here;
    for (long id : r->contents.ids())
      if (obj_data *o = obj_by_id(g, id))
        here += str_format(" o{:d}", obj_get_vnum(g, o));
    for (long id : r->people.ids())
      if (char_data *c = char_by_id(g, id))
        here += str_format(" m{:d}", char_is_npc(c) ? char_get_mob_vnum(g, c) : -1);
    if (!here.empty())
      file_print(out, "  holds{}\n", here);
  });
}

void mobiles(GameWorld *g)
{
  g->mob_proto.visit([&](mob_vnum vnum, struct char_data *m) {
    const index_data *ix = g->mob_index.find(vnum);
    file_print(out, "mob {:d} func {:d} {} level {:d} race {:d} class {:d} sex {:d} size {:d} align {:d} pos {:d}/{:d}\n",
               vnum, ix && ix->func != nullptr, text(char_get_sdesc(m)), char_get_level(m), char_get_race(m),
               char_get_class(m), char_get_sex(m), char_get_size(m), char_get_alignment(m), char_get_position(m),
               char_get_mob_specials_default_pos(m));
    file_print(out, "  hit {:d} ki {:d} move {:d} gold {:d} exp {:d} armor {:d} race_level {:d} class_level {:d} adj {:d} speaking {:d}\n",
               char_get_max_hit(m), char_get_max_ki(m), char_get_max_move(m), char_get_gold(m), char_get_exp(m),
               char_get_armor(m), char_get_race_level(m), char_get_class_level(m), char_get_level_adj(m),
               char_get_speaking(m));
    file_print(out, "  abils {:d} {:d} {:d} {:d} {:d} {:d} aff {:d} {:d} {:d} {:d} {:d} {:d} dice {:d}d{:d} attack {:d}\n",
               char_get_real_abils_str(m), char_get_real_abils_intel(m), char_get_real_abils_wis(m),
               char_get_real_abils_dex(m), char_get_real_abils_con(m), char_get_real_abils_cha(m),
               char_get_aff_abils_str(m), char_get_aff_abils_intel(m), char_get_aff_abils_wis(m),
               char_get_aff_abils_dex(m), char_get_aff_abils_con(m), char_get_aff_abils_cha(m),
               char_get_mob_specials_damnodice(m), char_get_mob_specials_damsizedice(m),
               char_get_mob_specials_attack_type(m));
    file_print(out, "  act {} affected {}\n", bits(char_get_mob_flags(m)),
               bits(char_get_affected_by(m)));
    std::string skills;
    for (int i = 0; i < SKILL_TABLE_SIZE; i++)
      if (char_get_skills_base(m, i))
        skills += str_format(" {:d}={:d}", i, char_get_skills_base(m, i));
    if (!skills.empty())
      file_print(out, "  skills{}\n", skills);
    for (const affected_type &a : char_affects(m))
      file_print(out, "  affect {:d} {:d} {:d} {:d}\n", a.type, a.duration, a.modifier, a.location);
    scripts(char_get_proto_script(m));
  });
}

void objects(GameWorld *g)
{
  g->obj_proto.visit([&](obj_vnum vnum, struct obj_data *o) {
    const index_data *ix = g->obj_index.find(vnum);
    std::string values;
    for (int i = 0; i < NUM_OBJ_VAL_POSITIONS; i++)
      values += str_format(" {:d}", obj_get_value(o, i));
    file_print(out, "obj {:d} func {:d} {} type {:d} weight {:d} cost {:d}/{:d} level {:d} size {:d} timer {:d} values{}\n",
               vnum, ix && ix->func != nullptr, text(obj_get_short_description(o)), obj_get_type_flag(o),
               (long long)obj_get_weight(o), obj_get_cost(o), obj_get_cost_per_day(o), obj_get_level(o),
               obj_get_size(o), obj_get_timer(o), values);
    file_print(out, "  extra {} wear {} affects {}\n", bits(obj_get_extra_flags(o)),
               bits(obj_get_wear_flags(o)), bits(obj_get_bitvector(o)));
    for (int i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj_get_affected(o)[i].location || obj_get_affected(o)[i].modifier)
        file_print(out, "  apply {:d} {:d} {:d}\n", obj_get_affected(o)[i].location, obj_get_affected(o)[i].modifier,
                   obj_get_affected(o)[i].specific);
    if (obj_get_sbinfo(o))
      for (int i = 0; i < SPELLBOOK_SIZE; i++)
        if (obj_get_sbinfo(o)[i].spellname)
          file_print(out, "  spell {:d} {:d} {:d}\n", i, obj_get_sbinfo(o)[i].spellname, obj_get_sbinfo(o)[i].pages);
    ex_descs(obj_get_ex_description(o));
    scripts(obj_get_proto_script(o));
  });
}

void triggers(GameWorld *g)
{
  g->trig_index.visit([&](trig_vnum vnum, const index_data *ix) {
    const trig_data *t = ix->proto;
    file_print(out, "trig {:d} {} attach {:d} types {:d} narg {:d} arg {} runs {:d} lines {:d}\n", vnum, text(t->name),
               t->attach_type, (long long)t->trigger_type, t->narg, text(t->arglist), t->program && t->program->runs(),
               t->program ? t->program->lines.size() : 0);
  });
}

void help(GameWorld *g)
{
  for (const help_index_element &h : g->help_table)
    file_print(out, "help {} dup {:d} level {:d} {}\n", text(h.keywords), h.duplicate, h.min_level, text(h.entry));
  for (int i = 0; i < NUM_CONTEXTS; i++)
    if (!g->context_help_list[i].empty())
      file_print(out, "context {:d} {}\n", i, text(g->context_help_list[i]));
}

void socials(GameWorld *g)
{
  for (int i = 0; i <= g->top_of_socialt; i++) {
    const social_messg &s = g->soc_mess_list[i];
    file_print(out, "social {:d} {} as {} hide {:d} pos {:d}/{:d} level {:d}\n", s.act_nr, text(s.command),
               text(s.sort_as), s.hide, s.min_victim_position, s.min_char_position, s.min_level_char);
    for (const social_message *m : {&s.char_no_arg, &s.others_no_arg, &s.char_found, &s.others_found, &s.vict_found,
                                    &s.char_body_found, &s.others_body_found, &s.vict_body_found, &s.not_found,
                                    &s.char_auto, &s.others_auto, &s.char_obj_found, &s.others_obj_found})
      file_print(out, "  {}\n", text(*m));
  }
}

void tables(GameWorld *g)
{
  file_print(out, "levels {} {:d}\n", text(g->level_version), g->level_vernum);
  for (int s = 0; s <= SAVING_WILL; s++) {
    std::string row;
    for (int c = 0; c < NUM_CLASSES; c++)
      row += str_format(" {:d}", g->save_classes[s][c]);
    file_print(out, "  saves {:d}{}\n", s, row);
  }
  std::string hits;
  for (int c = 0; c < NUM_CLASSES; c++)
    hits += str_format(" {:d}", g->basehit_classes[c]);
  file_print(out, "  basehit{}\n", hits);

  for (const ban_list_element *b = g->ban_list; b; b = b->next)
    file_print(out, "ban {} type {:d} date {:d} by {}\n", text(b->site), b->type, (long long)b->date, text(b->name));
  for (const std::string &name : g->invalid_list)
    file_print(out, "invalid {}\n", text(name));
  for (const disabled_data *d = g->disabled_first; d; d = d->next)
    file_print(out, "disabled {} sub {:d} by {} level {:d}\n", d->command ? text(d->command->command) : "(null)",
               d->subcmd, text(d->disabled_by), d->level);
  for (long i = 0; i < g->g_lNumAssemblies; i++) {
    const assembly_data &a = g->g_pAssemblyTable[i];
    file_print(out, "assembly {:d} type {:d} components {:d}\n", a.lVnum, a.uchAssemblyType, a.lNumComponents);
  }
  for (int i = 0; i < 25; i++)
    if (!g->topname[i].empty()) /* none before the toplist loads, which is after the dump */
      file_print(out, "top {:d} {} {:d}\n", i, text(g->topname[i]), (long long)g->toppoint[i]);
  file_print(out, "time {:d} {:d} {:d} {:d} era {:d} selfish {:d} dballs {:d} dragons {:d} {:d} {:d} {:d} {:d} {:d} {:d}\n",
             g->time_info.hours, g->time_info.day, g->time_info.month, g->time_info.year, g->ERAPLAYERS,
             g->SELFISHMETER, g->dballtime, g->SHADOW_DRAGON1, g->SHADOW_DRAGON2, g->SHADOW_DRAGON3,
             g->SHADOW_DRAGON4, g->SHADOW_DRAGON5, g->SHADOW_DRAGON6, g->SHADOW_DRAGON7);
  for (const auto &[name, s] : {std::pair<const char *, const std::string &>{"background", g->background},
                                {"credits", g->credits}, {"handbook", g->handbook}, {"help", g->help},
                                {"ihelp", g->ihelp}, {"immlist", g->immlist}, {"imotd", g->imotd}, {"info", g->info},
                                {"motd", g->motd}, {"news", g->news}, {"policies", g->policies},
                                {"wizlist", g->wizlist}, {"greetansi", g->GREETANSI}, {"greetings", g->GREETINGS}})
    file_print(out, "text {} {}\n", name, text(s));
}

void houses(GameWorld *g)
{
  for (int i = 0; i < g->num_of_houses; i++) {
    const house_control_rec &h = g->house_control[i];
    std::string guests;
    for (int j = 0; j < h.num_of_guests && j < MAX_GUESTS; j++)
      guests += str_format(" {:d}", h.guests[j]);
    file_print(out, "house {:d} atrium {:d} exit {:d} built {:d} mode {:d} owner {:d} paid {:d} bits {:d} by {:d} guests{}\n",
               h.vnum, h.atrium, h.exit_num, (long long)h.built_on, h.mode, h.owner, (long long)h.last_payment,
               h.bitvector, h.builtby, guests);
  }
}

/* The letters waiting, in the order they will be received */
void mail(GameWorld *g)
{
  for (const mail_letter &m : g->mail)
    file_print(out, "letter to {:d} from {:d} at {:d} {}\n", m.to, m.from, static_cast<long long>(m.time), text(m.text));
}

void boards(GameWorld *g)
{
  for (const board_info *b = g->bboards; b; b = b->next) {
    int n = 0;
    for (const board_msg *m = b->messages; m; m = m->next)
      file_print(out, "board {:d} message {:d} at {:d} {} {}\n", b->vnum, n++, (long long)m->timestamp,
                 text(m->subject), text(m->data));
  }
}

} // namespace

extern "C" long state_dump(GameWorld *g)
{
  if (!(out = fopen("statedump.txt", "w")))
    return -1;
  zones(g);
  rooms(g);
  mobiles(g);
  objects(g);
  triggers(g);
  help(g);
  socials(g);
  tables(g);
  houses(g);
  mail(g);
  boards(g);
  fclose(out);
  fprintf(stderr, "STATEDUMP written\n");
  return 0;
}
