local test = require("lua.test").new()
local dbat = require("dbat")

test:case("entity references expose reftype", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))
  local obj = assert(dbat.obj_protos.by_id(1):spawn(room))

  t:eq(room:reftype(), "room")
  t:eq(ch:reftype(), "character")
  t:eq(obj:reftype(), "object")
  t:eq(dbat.mob_protos.by_id(1):reftype(), "mob_prototype")
  t:eq(dbat.obj_protos.by_id(1):reftype(), "object_prototype")

  obj:extract()
  ch:extract()
end)

test:case("pure Lua character can_see dispatches by reftype", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local viewer = assert(dbat.mob_protos.by_id(1):spawn(room))
  local target = assert(dbat.mob_protos.by_id(2):spawn(room))
  local obj = assert(dbat.obj_protos.by_id(1):spawn(room))

  t:eq(viewer:can_see(target), viewer:can_see_char(target))
  t:eq(viewer:can_see(obj), viewer:can_see_obj(obj))

  obj:extract()
  target:extract()
  viewer:extract()
end)

test:case("__eq works for entity userdata", function(t)
  local room1 = assert(dbat.rooms.by_id(1))
  local room2 = assert(dbat.rooms.by_id(1))
  local room3 = assert(dbat.rooms.by_id(2))
  t:assert(room1 == room2, "same room: two handles should be equal")
  t:assert(room1 ~= room3, "different rooms should not be equal")

  local ch1 = assert(dbat.mob_protos.by_id(1):spawn(room1))
  local ch2 = ch1  -- same reference
  local ch3 = assert(dbat.mob_protos.by_id(2):spawn(room1))
  t:assert(ch1 == ch2, "same character reference should be equal")
  t:assert(ch1 ~= ch3, "different characters should not be equal")

  local obj1 = assert(dbat.obj_protos.by_id(1):spawn(room1))
  local obj2 = assert(dbat.obj_protos.by_id(1):spawn(room1))
  t:assert(obj1 ~= obj2, "different object instances should not be equal")

  obj2:extract()
  obj1:extract()
  ch3:extract()
  ch1:extract()
end)

test:case("pure Lua meta methods are merged into userdata metatables", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local viewer = assert(dbat.mob_protos.by_id(1):spawn(room))
  local target = assert(dbat.mob_protos.by_id(2):spawn(room))
  local obj = assert(dbat.obj_protos.by_id(1):spawn(room))

  t:eq(type(target:keywords_for(viewer)), "table")
  t:eq(type(obj:keywords_for(viewer)), "table")
  t:eq(type(room:refs()), "table")

  obj:extract()
  target:extract()
  viewer:extract()
end)

return test:run()
