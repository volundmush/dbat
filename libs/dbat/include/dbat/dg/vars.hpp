#pragma once
/*
 * Script variables: a trigger's locals, a script's globals, a player's saved variables.
 *
 * A store lists its variables newest first, as the old linked lists did, so that tstat, vstat and the saved
 * variables files show them in the same order. Names are matched without regard to case.
 */
#include <string>
#include <string_view>
#include <vector>

namespace dg {

struct variable {
  std::string name{};
  std::string value{};
  long context{}; /* 0: seen from any context */
};

class var_store {
public:
  /* The newest variable called name, or null. Good until the store next changes. */
  variable *find(std::string_view name);
  const variable *find(std::string_view name) const;
  /* The newest called name that is seen from context: its own, or 0's */
  variable *find_in(std::string_view name, long context);

  /* Sets name to value. The newest variable of that name takes it, unless it belongs to another context than
     context; then a new variable does, as the newest. */
  void set(std::string_view name, std::string_view value, long context);

  /* Removes the newest variable called name; false if there is none */
  bool erase(std::string_view name);
  /* Removes this one, which find gave */
  void erase(const variable *v);

  bool empty() const { return vars_.empty(); }
  void clear() { vars_.clear(); }

  /* Newest first */
  auto begin() const { return vars_.rbegin(); }
  auto end() const { return vars_.rend(); }

private:
  std::vector<variable> vars_; /* oldest first, so that adding one is cheap */
};

} // namespace dg
