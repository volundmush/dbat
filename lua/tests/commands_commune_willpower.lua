local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("commune and willpower are registered as Lua commands", function(t)
  local commune = dbat.characters.registry.commands.commune
  local willpower = dbat.characters.registry.commands.willpower

  t:assert(commune ~= nil, "commune command should be registered")
  t:eq(commune.id, "commune")
  t:assert(willpower ~= nil, "willpower command should be registered")
  t:eq(willpower.id, "willpower")
end)

test:case("commune and willpower aliases dispatch through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))

  t:eq(ch:execute_command("comm", commands), true)
  t:eq(ch:execute_command("wil", commands), true)

  ch:extract()
end)

test:case("willpower uses majinized condition number surface", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))

  t:eq(ch:condition_apply_number("majinized", "lord", 99), true)
  t:eq(ch:condition_number_get("majinized", "lord"), 99)
  t:eq(ch:condition_number_set("majinized", "lord", 3), 3)
  t:eq(ch:condition_number_get("majinized", "lord"), 3)

  ch:extract()
end)

return test:run()
