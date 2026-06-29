local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("wimpy is registered as a Lua command", function(t)
  local def = dbat.characters.registry.commands.wimpy
  t:assert(def ~= nil, "wimpy command should be registered")
  t:eq(def.id, "wimpy")
end)

test:case("wimpy alias dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("wimpy", commands), true)
  ch:extract()
end)

test:case("wimpy level binding stores threshold", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  ch:wimp_level_set(42)
  t:eq(ch:wimp_level_get(), 42)
  ch:wimp_level_set(0)
  t:eq(ch:wimp_level_get(), 0)
  ch:extract()
end)

return test:run()
