local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("trip is registered as a Lua command", function(t)
  local def = dbat.characters.registry.commands.trip
  t:assert(def ~= nil, "trip command should be registered")
  t:eq(def.id, "trip")
end)

test:case("trip alias dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("trip", commands), true)
  ch:extract()
end)

test:case("trip support APIs can set position and combat", function(t)
  local room = dbat.rooms.by_id(1)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))
  local vict = assert(dbat.mob_protos.by_id(2):spawn(room))

  vict:position_set(dbat.consts.positions.SITTING)
  t:eq(vict:position_get(), dbat.consts.positions.SITTING)

  ch:start_fighting(vict)
  vict:start_fighting(ch)
  t:assert(ch:fighting_get():is_same(vict), "actor should fight victim")
  t:assert(vict:fighting_get():is_same(ch), "victim should fight actor")

  ch:extract()
  vict:extract()
end)

return test:run()
