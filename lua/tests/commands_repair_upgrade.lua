local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("repair and upgrade are registered as Lua commands", function(t)
  local repair = dbat.characters.registry.commands.repair
  local upgrade = dbat.characters.registry.commands.upgrade

  t:assert(repair ~= nil, "repair command should be registered")
  t:eq(repair.id, "repair")
  t:assert(upgrade ~= nil, "upgrade command should be registered")
  t:eq(upgrade.id, "upgrade")
end)

test:case("repair and upgrade aliases dispatch through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("repair", commands), true)
  t:eq(ch:execute_command("upgrad", commands), true)
  ch:extract()
end)

return test:run()
