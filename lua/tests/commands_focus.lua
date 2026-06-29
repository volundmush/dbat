local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("focus is registered as a Lua command", function(t)
  local def = dbat.characters.registry.commands.focus
  t:assert(def ~= nil, "focus command should be registered")
  t:eq(def.id, "focus")
end)

test:case("focus alias dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("focus", commands), true)
  ch:extract()
end)

test:case("can_kill accepts legacy focus mode argument", function(t)
  local room = dbat.rooms.by_id(1)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))
  local vict = assert(dbat.mob_protos.by_id(1):spawn(room))

  t:eq(type(ch:can_kill(vict, 2)), "boolean")

  vict:extract()
  ch:extract()
end)

return test:run()
