/*
 * The interpreter: runs a trigger's program by walking its tree, with the blocks it is inside on the frame stack in
 * trig->run (see dbat/dg/run.hpp). A wait keeps the frames, and the run carries on from them when it is over.
 */
#include "dbat/utils.hpp"
#include "dbat/room_utils.hpp"
#include "dbat/object_utils.hpp"
#include "dbat/character_utils.hpp"
#include "dbat/game_world.hpp"

#include "dg_internal.hpp"

namespace {

using dg::frame;

constexpr int PASSES_BEFORE_A_REST = 40;   /* loop passes in one go, before the run waits a pulse */
constexpr int MOST_PASSES          = 5000; /* loop passes in a whole run, before it is ended */

/* The line a node starts at */
size_t first_line(const dg::node &n)
{
  return std::visit(overloaded{[](const dg::if_node &i) { return i.branches.front().line; },
                               [](const auto &other) { return other.line; }},
                    n.what);
}

/* The line that closes the block a node opens */
size_t closing_line(const dg::node &n)
{
  return std::visit(overloaded{[](const dg::if_node &i) { return i.end; },
                               [](const dg::while_node &w) { return w.done; },
                               [](const dg::switch_node &s) { return s.done; },
                               [](const auto &other) { return other.line; }},
                    n.what);
}

class runner {
public:
  enum class outcome {
    end,   /* the run is over */
    pause, /* it keeps its place: it waits, or its owner was purged */
  };

  runner(GameWorld *g, owner_ptr go, trig_data *trig, bool &owner_gone)
      : g_(g), go_(go), sc_(*script_slot(go)), trig_(trig), prog_(trig->run.prog), owner_gone_(owner_gone)
  {
  }

  int value() const { return ret_val_; }

  outcome run()
  {
    std::vector<frame> &frames = trig_->run.frames;

    while (!trig_->extracted && !frames.empty()) {
      frame &f = frames.back();
      if (f.next == f.body->size()) {
        if (std::optional<outcome> o = close_block())
          return *o;
        continue;
      }
      const dg::node &n = (*f.body)[f.next++];
      std::optional<outcome> o = std::visit([&](const auto &what) { return step(n, what); }, n.what);
      if (o)
        return *o;
    }
    return outcome::end;
  }

private:
  GameWorld *g_;
  owner_ptr go_;
  script_data *sc_;
  trig_data *trig_;
  std::shared_ptr<const dg::program> prog_; /* held, so that it outlasts the run whatever happens */
  bool &owner_gone_;
  int ret_val_ = 1;
  int passes_  = 0; /* loop passes since this call began, or since a while last found its condition false */

  const dg::line &line(size_t at) const { return prog_->lines[at]; }
  void at(size_t l) { dg_ctx_set_line(g_, line(l)); }
  bool test(const dg::expression &condition) { return is_true(evaluate(g_, condition, go_, sc_, trig_)); }
  void enter(dg::block_role role, const dg::block &body, size_t next, const dg::node &of)
  {
    trig_->run.frames.push_back({role, &body, next, &of});
  }

  /* The innermost block has run to its end */
  std::optional<outcome> close_block()
  {
    std::vector<frame> &frames = trig_->run.frames;
    frame &f                   = frames.back();

    switch (f.role) {
      case dg::block_role::script:
        return outcome::end;
      case dg::block_role::branch:
      case dg::block_role::cases:
        at(closing_line(*f.of));
        frames.pop_back();
        return {};
      case dg::block_role::loop:
        break;
    }

    const dg::while_node &w = std::get<dg::while_node>(f.of->what);
    size_t depth            = frames.size() - 1; /* f may move while the condition runs */
    at(w.done);
    if (!test(w.condition)) {
      frames.pop_back();
      return {};
    }
    frames[depth].next = 0;
    passes_++;
    trig_->run.passes++;
    if (passes_ == PASSES_BEFORE_A_REST) {
      schedule_wait(g_, go_, trig_, 1);
      return outcome::pause;
    }
    if (trig_->run.passes >= MOST_PASSES) {
      script_log(g_, "Trigger VNum {:d} has looped 5,000 times!!!", trig_get_vnum(g_, trig_));
      return outcome::end;
    }
    return {};
  }

  std::optional<outcome> step(const dg::node &, const dg::command_node &c)
  {
    at(c.line);
    std::string cmd = render(g_, c.command, go_, sc_, trig_);
    command_call call{g_, go_, sc_, trig_, cmd, c.line, ret_val_, owner_gone_};

    switch (run_command(call)) {
      case command_result::next:
        return {};
      case command_result::halt:
        return outcome::end;
      case command_result::stop:
        return outcome::pause;
    }
    return {};
  }

  std::optional<outcome> step(const dg::node &, const dg::break_node &b)
  {
    std::vector<frame> &frames = trig_->run.frames;

    at(b.line);
    for (;;) {
      dg::block_role role = frames.back().role;
      frames.pop_back();
      if (role == dg::block_role::loop || role == dg::block_role::cases)
        return {};
    }
  }

  std::optional<outcome> step(const dg::node &n, const dg::if_node &i)
  {
    for (const dg::if_node::branch &b : i.branches) {
      at(b.line);
      if (!b.tests || test(b.condition)) {
        enter(dg::block_role::branch, b.body, 0, n);
        break;
      }
    }
    return {};
  }

  std::optional<outcome> step(const dg::node &n, const dg::while_node &w)
  {
    at(w.line);
    if (test(w.condition))
      enter(dg::block_role::loop, w.body, 0, n);
    else
      passes_ = 0; /* quirk: only here, and not when a loop ends at its done */
    return {};
  }

  std::optional<outcome> step(const dg::node &n, const dg::switch_node &s)
  {
    at(s.line);
    std::string subject = evaluate(g_, s.subject, go_, sc_, trig_);
    for (const dg::switch_node::label &l : s.labels)
      if (l.is_default || case_matches(subject, l.value)) {
        enter(dg::block_role::cases, s.body, l.at, n);
        break;
      }
    return {};
  }
};

/* Runs trig on go: from its first line (TRIG_NEW, as dg_triggers.cpp starts it) or from where it waited
   (TRIG_RESTART). Gives the script's return value. */
int run_trigger(GameWorld *g, owner_ptr go, trig_data *trig, int mode, bool &owner_gone)
{
  struct script_data *sc = *script_slot(go);

  if (g->script_depth > MAX_SCRIPT_DEPTH) {
    script_log(g, "Trigger {:d} recursed beyond maximum allowed depth.", trig_get_vnum(g, trig));
    std::visit(overloaded{[&](char_data *ch) {
                            script_log(g, "It was attached to {} [{:d}]", char_get_name(ch), char_get_mob_vnum(g, ch));
                          },
                          [&](obj_data *obj) {
                            script_log(g, "It was attached to {} [{:d}]", obj_get_short_description(obj),
                                       obj_get_vnum(g, obj));
                          },
                          [&](room_data *room) {
                            script_log(g, "It was attached to {} [{:d}]", room_get_name(room), room_get_number(room));
                          }},
               go);

    /*
       extract_script() works on rooms, but on mobiles and objects, it will be called again if the caller is
       load_mtrigger or load_otrigger; if it is one of these, we must make sure the script is not just reloaded on
       the next mob. We make the calling code decide how to handle it, so it doesn't get totally removed unless it's
       a load_xtrigger().
     */
    extract_script(g, go);
    return SCRIPT_ERROR_CODE;
  }

  g->script_depth++;

  if (mode == TRIG_NEW) {
    trig->run.prog = trig->program;
    trig->run.frames  = {{dg::block_role::script, &trig->program->body, 0, nullptr}};
    trig->run.passes  = 0;
    sc->context       = 0;
  }

  g->dg_owner_purged = 0;

  runner r(g, go, trig, owner_gone);
  /* Once extracted, what the trigger belonged to may be gone, and free_trigger() has the rest. */
  if (r.run() == runner::outcome::end && !trig->extracted) {
    trig->var_list.clear();
    trig->run.stop();
  }

  g->script_depth--;
  return r.value();
}

} // namespace

/* Runs a trigger with dg_ctx describing it, restoring the caller's context after. The result says whether go was
   purged meanwhile, as callers can't otherwise tell. */
run_result script_driver(GameWorld *g, owner_ptr go, trig_data *trig, int mode)
{
  struct dg_run_ctx saved = g->dg_ctx;
  run_result result;

  if (trig->extracted) /* an earlier trigger on the same thing removed this one */
    return result;

  dg_ctx_begin(g, go, trig);
  result.value = run_trigger(g, go, trig, mode, result.owner_gone);
  g->dg_ctx    = saved;
  if (trig->owner_gone)
    result.owner_gone = true;
  return result;
}

bool trigger_ready(GameWorld *g, trig_data *t, bitvector_t type)
{
  if (!(t->trigger_type & type) || t->run.running())
    return false;
  if (t->program && t->program->runs())
    return true;
  trig_data *proto = t->index ? t->index->proto : t; /* once for all its instances */
  if (!proto->refusal_logged) {
    proto->refusal_logged = true;
    mud_log("SCRIPT ERR: trigger #{:d} '{}' does not run: its script has errors, logged when it was loaded or saved",
            trig_get_vnum(g, t), t->name);
  }
  return false;
}

const dg::line *dg::run_state::next_line() const
{
  for (auto f = frames.rbegin(); f != frames.rend(); ++f) {
    if (f->next < f->body->size())
      return &prog->lines[first_line((*f->body)[f->next])];
    if (f->of)
      return &prog->lines[closing_line(*f->of)];
  }
  return nullptr;
}

const dg::line *dg::run_state::opened_at(const frame &f) const
{
  if (!f.of)
    return nullptr;
  if (const auto *i = std::get_if<dg::if_node>(&f.of->what))
    for (const dg::if_node::branch &b : i->branches)
      if (&b.body == f.body)
        return &prog->lines[b.line];
  return &prog->lines[first_line(*f.of)];
}
