local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("situp is registered as a Lua command", function(t)
  local def = dbat.characters.registry.commands.situp
  t:assert(def ~= nil, "situp command should be registered")
  t:eq(def.id, "situp")
end)

test:case("situp alias dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("situp", commands), true)
  ch:extract()
end)

return test:run()
