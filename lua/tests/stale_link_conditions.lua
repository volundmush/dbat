local test = require("lua.test").new()
local dbat = require("dbat")

local function mobs()
  local a = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  local b = assert(dbat.mob_protos.by_id(2):spawn(dbat.rooms.by_id(1)))
  return a, b
end

test:case("defending relationship clears when rooms differ", function(t)
  local ch, vict = mobs()
  ch:defending_for_set(vict)
  vict:defended_by_set(ch)
  vict:to_room(dbat.rooms.by_id(2))

  dbat.get("conditions", "defending_for"):dispatch_event(ch, ch:condition("defending_for"), "stale_check")
  t:eq(ch:defending_for_get(), nil)
  t:eq(vict:defended_by_get(), nil)
  ch:extract()
  vict:extract()
end)

test:case("blocking relationship clears when rooms differ", function(t)
  local ch, vict = mobs()
  ch:blocking_set(vict)
  vict:blocked_by_set(ch)
  vict:to_room(dbat.rooms.by_id(2))

  dbat.get("conditions", "blocking"):dispatch_event(ch, ch:condition("blocking"), "stale_check")
  t:eq(ch:blocking_get(), nil)
  t:eq(vict:blocked_by_get(), nil)
  ch:extract()
  vict:extract()
end)

test:case("carrying relationship drops when rooms differ", function(t)
  local ch, vict = mobs()
  ch:carrying_char_set(vict)
  vict:carried_by_char_set(ch)
  vict:to_room(dbat.rooms.by_id(2))

  dbat.get("conditions", "carrying_char"):dispatch_event(ch, ch:condition("carrying_char"), "stale_check")
  t:eq(ch:carrying_char_get(), nil)
  t:eq(vict:carried_by_char_get(), nil)
  ch:extract()
  vict:extract()
end)

test:case("using_furniture backs sits_get and clears stale furniture", function(t)
  local ch = assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
  local chair = assert(dbat.obj_protos.by_id(1):spawn(dbat.rooms.by_id(1)))

  ch:sits_set(chair)
  t:eq(ch:condition_has("using_furniture"), true)
  t:assert(ch:sits_get():is_same(chair), "sits_get should return furniture from condition")
  t:eq(chair:sitting_get(), ch:id_get())

  chair:to_room(dbat.rooms.by_id(2))
  dbat.get("conditions", "using_furniture"):dispatch_event(ch, ch:condition("using_furniture"), "stale_check")
  t:eq(ch:sits_get(), nil)
  t:eq(chair:sitting_get(), 0)

  chair:extract()
  ch:extract()
end)

return test:run()
