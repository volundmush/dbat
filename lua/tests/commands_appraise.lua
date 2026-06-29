local test = require("lua.test").new()
local dbat = require("dbat")
local commands = require("lua.characters.commands")

test:case("appraise is registered as a Lua command", function(t)
  local def = dbat.characters.registry.commands.appraise
  t:assert(def ~= nil, "appraise command should be registered")
  t:eq(def.id, "appraise")
end)

test:case("appraise alias dispatches through Lua command matcher", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  t:eq(ch:execute_command("appraise thing", commands), true)
  ch:extract()
end)

test:case("object affect specific getters expose skill details", function(t)
  local obj = assert(dbat.obj_protos.by_id(1):spawn())
  obj:affect_set(0, dbat.consts.applies.SKILL, dbat.consts.skills.APPRAISE, 7)

  t:eq(obj:affect_location_get(0), dbat.consts.applies.SKILL)
  t:eq(obj:affect_specific_get(0), dbat.consts.skills.APPRAISE)
  t:eq(obj:affect_modifier_get(0), 7)
  t:eq(obj:affect_specific_name_get(0), "appraise")

  obj:extract()
end)

return test:run()
