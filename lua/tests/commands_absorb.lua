local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("absorb is registered as a Lua command", function(t)
  local def = dbat.characters.registry.commands.absorb
  t:assert(def ~= nil, "absorb command should be registered")
  t:eq(def.id, "absorb")
end)

test:case("absorb abbreviation dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("absor", commands), true)
  ch:extract()
end)

test:case("absorb helper bindings are callable", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))

  t:eq(type(ch:limb_ok(0)), "boolean")
  local before = ch:absorbs_get()
  t:eq(ch:absorbs_set(before), before)
  t:eq(ch:absorbs_mod(0), before)

  ch:extract()
end)

return test:run()
