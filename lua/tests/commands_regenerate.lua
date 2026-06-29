local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("regenerate is registered as a Lua command", function(t)
  local def = dbat.characters.registry.commands.regenerate
  t:assert(def ~= nil, "regenerate command should be registered")
  t:eq(def.id, "regenerate")
end)

test:case("regenerate abbreviation dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("regen", commands), true)
  ch:extract()
end)

test:case("init_skill binding is callable", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(type(ch:init_skill("regenerate")), "number")
  ch:extract()
end)

return test:run()
