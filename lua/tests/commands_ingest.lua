local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("ingest is registered as a Lua command", function(t)
  local def = dbat.characters.registry.commands.ingest
  t:assert(def ~= nil, "ingest command should be registered")
  t:eq(def.id, "ingest")
end)

test:case("ingest abbreviation dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("inges", commands), true)
  ch:extract()
end)

test:case("ingest support helpers are callable", function(t)
  local room = dbat.rooms.by_id(1)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))
  local vict = assert(dbat.mob_protos.by_id(2):spawn(room))

  ch:race_set("majin")
  t:eq(ch:race_get(), "majin")
  t:eq(ch:absorbs_mod(0), ch:absorbs_get())
  ch:handle_ingest_learn(vict)

  vict:extract()
  ch:extract()
end)

return test:run()
