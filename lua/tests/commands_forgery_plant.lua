local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("forgery and plant are registered as Lua commands", function(t)
  local forgery = dbat.characters.registry.commands.forgery
  local plant = dbat.characters.registry.commands.plant

  t:assert(forgery ~= nil, "forgery command should be registered")
  t:eq(forgery.id, "forgery")
  t:assert(plant ~= nil, "plant command should be registered")
  t:eq(plant.id, "plant")
end)

test:case("forgery and plant aliases dispatch through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))

  t:eq(ch:execute_command("forg thing", commands), true)
  t:eq(ch:execute_command("plan thing target", commands), true)

  ch:extract()
end)

test:case("is_shopkeeper binding is callable", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(type(ch:is_shopkeeper()), "boolean")
  ch:extract()
end)

return test:run()
