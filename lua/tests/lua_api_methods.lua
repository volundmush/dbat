local test = require("lua.test").new()
local dbat = require("dbat")

test:case("character send_text is safe without descriptor", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))

  ch:send_text("literal %s text should not be a format string\r\n")

  t:assert(ch:valid())
  t:assert(not ch:is_extracted())
  ch:extract()
  t:assert(ch:valid())       -- still in registry until end-of-loop cleanup
  t:assert(ch:is_extracted()) -- but marked for deferred extraction
end)

test:case("visibility and darkness helpers return booleans", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local viewer = assert(dbat.mob_protos.by_id(1):spawn(room))
  local target = assert(dbat.mob_protos.by_id(2):spawn(room))
  local obj = assert(dbat.obj_protos.by_id(1):spawn(room))

  t:eq(type(room:is_dark()), "boolean")
  t:eq(type(viewer:can_see_in_dark()), "boolean")
  t:eq(type(viewer:can_see_char(target)), "boolean")
  t:eq(type(viewer:can_see_obj(obj)), "boolean")

  obj:extract()
  target:extract()
  viewer:extract()
end)

test:case("object extract invalidates handle", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local obj = assert(dbat.obj_protos.by_id(2):spawn(room))

  t:assert(obj:valid())
  obj:extract()
  t:assert(not obj:valid())
end)

test:case("race and sensei APIs use string ids", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))

  ch:race_set("saiyan")
  t:eq(ch:race_get(), "saiyan")

  ch:sensei_set("roshi")
  t:eq(ch:sensei_get(), "roshi")

  ch:sensei_set("commoner")
  t:eq(ch:sensei_get(), "commoner")

  ch:extract()
end)

test:case("sex API uses string ids", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))

  ch:sex_set("male")
  t:eq(ch:sex_get(), "male")

  ch:sex_set("female")
  t:eq(ch:sex_get(), "female")

  ch:sex_set("neutral")
  t:eq(ch:sex_get(), "neutral")

  ch:extract()
end)

test:case("mind link helpers expose relationship and linker flag", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))
  local vict = assert(dbat.mob_protos.by_id(2):spawn(room))

  ch:mindlinked_set(vict)
  t:assert(ch:mindlinked_get():is_same(vict))

  vict:linker_set(1)
  t:eq(vict:linker_get(), 1)
  vict:linker_set(0)
  t:eq(vict:linker_get(), 0)

  ch:mindlinked_set(nil)
  t:eq(ch:mindlinked_get(), nil)

  vict:extract()
  ch:extract()
end)

test:case("wimp level setter updates auto-flee threshold", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))

  ch:wimp_level_set(123)
  t:eq(ch:wimp_level_get(), 123)
  ch:wimp_level_set(0)
  t:eq(ch:wimp_level_get(), 0)

  ch:extract()
end)

return test:run()
