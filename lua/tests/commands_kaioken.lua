local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("kaioken is registered as a Lua command", function(t)
  local def = dbat.characters.registry.commands.kaioken
  t:assert(def ~= nil, "kaioken command should be registered")
  t:eq(def.id, "kaioken")
end)

test:case("kaioken alias dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("kaioken 0", commands), true)
  ch:extract()
end)

test:case("starphase getter is callable", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(type(ch:starphase_get()), "number")
  ch:extract()
end)

return test:run()
