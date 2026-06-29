local test = require("lua.test").new()
local dbat = require("dbat")

local function mobs()
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  local vict = assert(dbat.mob_protos.by_id(2):spawn(dbat.rooms.by_id(1)))
  return ch, vict
end

test:case("fighting removal clears combat-only conditions", function(t)
  local ch, vict = mobs()

  ch:start_fighting(vict)
  t:eq(ch:condition_has("fighting"), true)

  t:eq(ch:condition_apply("advantageous_position"), true)
  t:eq(ch:condition_apply("combo"), true)
  t:eq(ch:condition_has("advantageous_position"), true)
  t:eq(ch:condition_has("combo"), true)

  ch:condition_remove("fighting", "test_cleanup")

  t:eq(ch:condition_has("fighting"), false)
  t:eq(ch:condition_has("advantageous_position"), false)
  t:eq(ch:condition_has("combo"), false)

  ch:extract()
  vict:extract()
end)

test:case("npc combat tick can taunt from Lua", function(t)
  local ch, vict = mobs()
  local old_random = math.random
  local calls = 0

  ch:start_fighting(vict)

  math.random = function(...)
    calls = calls + 1
    if calls == 1 then return 30 end
    return old_random(...)
  end

  local ok, err = pcall(function()
    dbat.get("conditions", "fighting"):dispatch_event(ch, ch:condition("fighting"), "combat_tick")
  end)
  math.random = old_random
  if not ok then error(err, 0) end

  t:assert(ch:fighting_get():is_same(vict), "combat tick should leave npc fighting its target")

  ch:extract()
  vict:extract()
end)

return test:run()
