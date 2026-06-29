local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("think and telepathy are registered as Lua commands", function(t)
  local think = dbat.characters.registry.commands.think
  local telepathy = dbat.characters.registry.commands.telepathy

  t:assert(think ~= nil, "think command should be registered")
  t:eq(think.id, "think")
  t:assert(telepathy ~= nil, "telepathy command should be registered")
  t:eq(telepathy.id, "telepathy")
end)

test:case("think and telepathy aliases dispatch through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))

  t:eq(ch:execute_command("thin hello", commands), true)
  t:eq(ch:execute_command("telepa", commands), true)

  ch:extract()
end)

test:case("think and telepathy use mind link helper surface", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))
  local vict = assert(dbat.mob_protos.by_id(2):spawn(room))

  ch:mindlinked_set(vict)
  vict:mindlinked_set(ch)
  ch:linker_set(1)

  t:assert(ch:mindlinked_get():is_same(vict))
  t:assert(vict:mindlinked_get():is_same(ch))
  t:eq(ch:linker_get(), 1)

  ch:mindlinked_set(nil)
  vict:mindlinked_set(nil)
  ch:linker_set(0)

  vict:extract()
  ch:extract()
end)

return test:run()
