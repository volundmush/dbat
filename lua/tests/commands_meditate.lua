local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("meditate is registered as a Lua command", function(t)
  local def = dbat.characters.registry.commands.meditate
  t:assert(def ~= nil, "meditate command should be registered")
  t:eq(def.id, "meditate")
end)

test:case("meditate alias dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("medita", commands), true)
  ch:extract()
end)

test:case("meditate support bindings are callable", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))
  local vict = assert(dbat.mob_protos.by_id(2):spawn(room))
  local obj = assert(dbat.obj_protos.by_id(1):spawn(room))

  ch:mindlinked_set(vict)
  t:assert(ch:mindlinked_get():is_same(vict))
  ch:mindlinked_set(nil)
  t:eq(ch:mindlinked_get(), nil)

  vict:linker_set(1)
  t:eq(vict:linker_get(), 1)
  vict:linker_set(0)
  t:eq(vict:linker_get(), 0)

  local sitter_id = obj:sitting_get()
  t:eq(type(sitter_id), "number")
  if sitter_id ~= 0 then
    t:assert(dbat.characters.by_id(sitter_id) ~= nil)
  end

  obj:extract()
  vict:extract()
  ch:extract()
end)

return test:run()
