/* Script variables: see dbat/dg/vars.hpp. */
#include "dbat/dg/vars.hpp"

#include <strings.h>

namespace dg {

namespace {

bool same_name(std::string_view a, const std::string &b)
{
  return a.size() == b.size() && strncasecmp(a.data(), b.data(), a.size()) == 0;
}

} // namespace

variable *var_store::find(std::string_view name)
{
  for (auto v = vars_.rbegin(); v != vars_.rend(); ++v)
    if (same_name(name, v->name))
      return &*v;
  return nullptr;
}

const variable *var_store::find(std::string_view name) const
{
  return const_cast<var_store *>(this)->find(name);
}

variable *var_store::find_in(std::string_view name, long context)
{
  for (auto v = vars_.rbegin(); v != vars_.rend(); ++v)
    if (same_name(name, v->name) && (v->context == 0 || v->context == context))
      return &*v;
  return nullptr;
}

void var_store::set(std::string_view name, std::string_view value, long context)
{
  /* either may be a variable's own, here */
  std::string new_value(value);
  variable *v = find(name);

  if (!v || (v->context && v->context != context)) {
    vars_.push_back({std::string(name), {}, context});
    v = &vars_.back();
  }
  v->value = std::move(new_value);
}

bool var_store::erase(std::string_view name)
{
  const variable *v = find(name);
  if (!v)
    return false;
  erase(v);
  return true;
}

void var_store::erase(const variable *v)
{
  vars_.erase(vars_.begin() + (v - vars_.data()));
}

} // namespace dg
