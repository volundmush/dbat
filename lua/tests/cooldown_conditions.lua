local test = require("lua.test").new()
local dbat = require("dbat")

local function mob()
  return assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
end

test:case("concentrate cooldown uses a Lua condition", function(t)
  local ch = mob()

  ch:cooldown_set(5)
  t:eq(ch:condition_has("cooldown_concentrate"), true)
  t:assert(ch:cooldown_get() >= 4, "cooldown_get should report remaining seconds")

  ch:cooldown_set(0)
  t:eq(ch:condition_has("cooldown_concentrate"), false)

  ch:extract()
end)

test:case("backstab cooldown reads condition duration", function(t)
  local ch = mob()

  t:eq(ch:condition_apply("cooldown_backstab"), true)
  local cond = assert(ch:condition("cooldown_backstab"))
  cond:schedule_expire(6)

  t:assert(ch:backstab_cooldown() >= 5, "backstab cooldown should report condition duration")

  ch:extract()
end)

test:case("selfdestruct cooldown uses a Lua condition", function(t)
  local ch = mob()

  ch:selfdestruct_cooldown_set(7)
  t:eq(ch:condition_has("cooldown_selfdestruct"), true)
  t:assert(ch:selfdestruct_cooldown_get() >= 6, "selfdestruct cooldown should report remaining seconds")

  ch:selfdestruct_cooldown_set(0)
  t:eq(ch:condition_has("cooldown_selfdestruct"), false)

  ch:extract()
end)

test:case("radar ping cooldown is a Lua condition", function(t)
  local ch = mob()

  t:eq(ch:condition_apply("cooldown_radar_ping"), true)
  local cond = assert(ch:condition("cooldown_radar_ping"))
  cond:schedule_expire(5)

  t:eq(ch:condition_has("cooldown_radar_ping"), true)
  t:assert(cond:remaining_ms() > 0, "radar ping cooldown should report remaining time")

  ch:extract()
end)

return test:run()
