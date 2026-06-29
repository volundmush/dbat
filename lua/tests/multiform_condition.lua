local test = require("lua.test").new()
local dbat = require("dbat")

local function mob(vnum, room)
  return assert(dbat.mob_protos.by_id(vnum):spawn(room or dbat.rooms.by_id(1)))
end

test:case("multiform schedules npc assist event", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local original = mob(1, room)
  local clone = mob(1, room)

  t:eq(clone:condition_apply_number("multiform", "original_id", original:id_get()), true)

  local cond = clone:condition("multiform")
  t:assert(cond:event_pending("assist"), "clone should schedule multiform assist")

  clone:extract()
  original:extract()
end)

test:case("multiform assist event joins original's fight", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local original = mob(1, room)
  local clone = mob(1, room)
  local target = mob(2, room)
  local old_random = math.random

  t:eq(clone:condition_apply_number("multiform", "original_id", original:id_get()), true)
  clone:skill_base_set("kick", 100)
  original:start_fighting(target)

  math.random = function()
    return 5
  end

  local ok, err = pcall(function()
    dbat.get("conditions", "multiform"):dispatch_event(clone, clone:condition("multiform"), "assist")
  end)
  math.random = old_random
  if not ok then error(err, 0) end

  t:assert(clone:fighting_get() and clone:fighting_get():is_same(target), "clone should attack original's target")

  clone:extract()
  original:extract()
  target:extract()
end)

return test:run()
