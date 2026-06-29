local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("grapple is registered as a Lua command", function(t)
  local def = dbat.characters.registry.commands.grapple
  t:assert(def ~= nil, "grapple command should be registered")
  t:eq(def.id, "grapple")
end)

test:case("grapple alias dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("grapp", commands), true)
  ch:extract()
end)

test:case("grapple state setters pair characters with grapple types", function(t)
  local room = dbat.rooms.by_id(1)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))
  local vict = assert(dbat.mob_protos.by_id(2):spawn(room))

  ch:grappling_set(vict, 2)
  vict:grappled_set(ch, 2)

  t:assert(ch:grappling_get():is_same(vict), "attacker should be grappling victim")
  t:assert(vict:grappled_get():is_same(ch), "victim should be grappled by attacker")
  t:eq(ch:graptype_get(), 2)
  t:eq(vict:graptype_get(), 2)

  ch:grappling_set(nil, 0)
  vict:grappled_set(nil, 0)
  ch:extract()
  vict:extract()
end)

return test:run()
