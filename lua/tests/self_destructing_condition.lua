local test = require("lua.test").new()
local dbat = require("dbat")

local function mob()
  return assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
end

test:case("self_destructing progresses to ready by scheduled event", function(t)
  local ch = mob()

  ch:skill_base_set("self destruct", 200)
  t:eq(ch:condition_apply("self_destructing", "test", "selfdestruct"), true)
  t:eq(ch:condition_number_get("self_destructing", "phase"), 1)

  dbat.get("conditions", "self_destructing"):dispatch_event(ch, ch:condition("self_destructing"), "tick")

  t:eq(ch:condition_has("self_destructing"), true)
  t:eq(ch:condition_number_get("self_destructing", "phase"), 2)

  ch:extract()
end)

test:case("self_destructing timeout removes condition", function(t)
  local ch = mob()

  ch:skill_base_set("self destruct", 0)
  t:eq(ch:condition_apply("self_destructing", "test", "selfdestruct"), true)
  local cond = assert(ch:condition("self_destructing"))
  cond:number_set("ticks", 59)

  dbat.get("conditions", "self_destructing"):dispatch_event(ch, cond, "tick")

  t:eq(ch:condition_has("self_destructing"), false)

  ch:extract()
end)

return test:run()
