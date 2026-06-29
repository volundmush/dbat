local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("recharge is registered as a Lua command", function(t)
  local recharge = dbat.characters.registry.commands.recharge

  t:assert(recharge ~= nil, "recharge command should be registered")
  t:eq(recharge.id, "recharge")
end)

test:case("recharge alias dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("rechar", commands), true)
  ch:extract()
end)

return test:run()
