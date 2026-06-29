local test = require("lua.test").new()
local dbat = require("dbat")
local IWEAR = dbat.consts.item_wear_flags
local POS = dbat.consts.positions

local function mob()
  return dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1))
end

local function item()
  return dbat.obj_protos.by_id(1):spawn()
end

test:case("character script add/has/remove", function(t)
  local ch = mob()
  t:eq(ch:script_has("test_script"), false)
  t:eq(ch:script_add("test_script"), true)
  t:eq(ch:script_has("test_script"), true)
  t:eq(ch:script_remove("test_script", "test cleanup"), true)
  t:eq(ch:script_has("test_script"), false)
end)

test:case("character script number get/set/mod", function(t)
  local ch = mob()
  ch:script_add("test_script")
  t:eq(ch:script_number_get("test_script", "counter"), 0)
  ch:script_number_set("test_script", "counter", 42)
  t:eq(ch:script_number_get("test_script", "counter"), 42)

  local sc = ch:script("test_script")
  t:assert(sc ~= nil, "script handle should be non-nil")
  t:eq(sc:id(), "test_script")
  t:eq(sc:number_get("counter"), 42)
  t:eq(sc:number_mod("counter", 8), 50)
  t:eq(ch:script_number_get("test_script", "counter"), 50)
end)

test:case("character script text get/set", function(t)
  local ch = mob()
  ch:script_add("test_script")
  t:eq(ch:script_text_get("test_script", "label"), nil)
  ch:script_text_set("test_script", "label", "hello")
  t:eq(ch:script_text_get("test_script", "label"), "hello")

  local sc = ch:script("test_script")
  t:eq(sc:text_get("label"), "hello")
  sc:text_set("label", "world")
  t:eq(ch:script_text_get("test_script", "label"), "world")
end)

test:case("character script() returns nil when not present", function(t)
  local ch = mob()
  t:eq(ch:script("test_script"), nil)
  ch:script_add("test_script")
  t:assert(ch:script("test_script") ~= nil, "should be non-nil after add")
  ch:script_remove("test_script", "cleanup")
  t:eq(ch:script("test_script"), nil)
end)

test:case("character scripts() iteration", function(t)
  local ch = mob()
  ch:script_add("test_script")
  local found = false
  for sc in ch:scripts() do
    if sc:id() == "test_script" then found = true end
  end
  t:eq(found, true)
end)

test:case("object script add/has/remove", function(t)
  local obj = item()
  t:eq(obj:script_has("test_script"), false)
  t:eq(obj:script_add("test_script"), true)
  t:eq(obj:script_has("test_script"), true)
  t:eq(obj:script_remove("test_script", "test cleanup"), true)
  t:eq(obj:script_has("test_script"), false)
end)

test:case("object script number/text variables", function(t)
  local obj = item()
  obj:script_add("test_script")
  t:eq(obj:script_number_get("test_script", "val"), 0)
  obj:script_number_set("test_script", "val", 99)
  t:eq(obj:script_number_get("test_script", "val"), 99)

  obj:script_text_set("test_script", "tag", "marked")
  t:eq(obj:script_text_get("test_script", "tag"), "marked")

  local sc = obj:script("test_script")
  t:assert(sc ~= nil)
  t:eq(sc:number_get("val"), 99)
  t:eq(sc:text_get("tag"), "marked")
end)

test:case("huge attack object scripts provoke idle mobs", function(t)
  local room = assert(dbat.rooms.by_id(0))
  local user = assert(dbat.mob_protos.by_id(1):spawn(room))
  local target = assert(dbat.mob_protos.by_id(2):spawn(room))
  local idle = assert(dbat.mob_protos.by_id(3):spawn(room))
  local obj = assert(dbat.obj_protos.by_id(1):spawn(room))

  user:start_fighting(target)
  obj:script_add("ki_genocide")
  obj:script_number_set("ki_genocide", "user_id", user:id_get())
  obj:script_number_set("ki_genocide", "target_id", target:id_get())
  obj:script_number_set("ki_genocide", "kidist", 2)
  obj:script_number_set("ki_genocide", "damage", 1)

  dbat.get("object_scripts", "ki_genocide").on_event(obj, obj:script("ki_genocide"), "tick")

  t:assert(idle:fighting_get() and idle:fighting_get():is_same(user),
           "idle mob should attack the huge attack user")

  obj:extract()
  idle:extract()
  target:extract()
  user:extract()
end)

test:case("mob scavenger script picks up best available item", function(t)
  local room = assert(dbat.rooms.by_id(5))
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))
  local cheap = assert(dbat.obj_protos.by_id(1):spawn(room))
  local best = assert(dbat.obj_protos.by_id(1):spawn(room))
  local old_random = math.random

  cheap:cost_set(10)
  best:cost_set(50)
  cheap:wear_flag_set(IWEAR.TAKE, true)
  best:wear_flag_set(IWEAR.TAKE, true)
  ch:race_set("human")
  ch:position_set(POS.STANDING)
  ch:script_add("mob_scavenger")
  t:assert(ch:script("mob_scavenger"):event_pending("scavenge"))

  math.random = function(a, b)
    if a == 1 and b == 100 then return 100 end
    return a
  end
  dbat.get("character_scripts", "mob_scavenger").on_event(ch, ch:script("mob_scavenger"), "scavenge")
  math.random = old_random

  t:eq(best:carried_by_get(), ch:id_get(),
           "scavenger should pick up the highest value available object")
  t:eq(cheap:carried_by_get(), 0,
           "lower value object should remain in the room")

  cheap:extract()
  ch:extract()
end)

test:case("mob aggressive script tracks aggression countdown", function(t)
  local room = assert(dbat.rooms.by_id(0))
  local ch = assert(dbat.mob_protos.by_id(1):spawn(room))

  ch:mob_flag_set(dbat.consts.mob_flags.AGGRESSIVE, true)
  ch:script_add("mob_aggressive")
  t:assert(ch:script("mob_aggressive"):event_pending("aggress"))

  dbat.get("character_scripts", "mob_aggressive").on_event(ch, ch:script("mob_aggressive"), "aggress")
  t:eq(ch:script_number_get("mob_aggressive", "aggtimer"), 0)

  ch:extract()
end)

test:case("mob helper script schedules and ignores npc opponents", function(t)
  local room = assert(dbat.rooms.by_id(5))
  local helper = assert(dbat.mob_protos.by_id(1):spawn(room))
  local ally = assert(dbat.mob_protos.by_id(2):spawn(room))
  local opponent = assert(dbat.mob_protos.by_id(3):spawn(room))

  helper:mob_flag_set(dbat.consts.mob_flags.HELPER, true)
  helper:position_set(POS.STANDING)
  ally:race_set("human")
  ally:start_fighting(opponent)
  helper:script_add("mob_helper")
  t:assert(helper:script("mob_helper"):event_pending("help"))

  dbat.get("character_scripts", "mob_helper").on_event(helper, helper:script("mob_helper"), "help")
  t:eq(helper:fighting_get(), nil)

  opponent:extract()
  ally:extract()
  helper:extract()
end)

test:case("room geo_effect script follows positive geffect", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local fresh = assert(dbat.rooms.by_id(2))

  fresh:geffect_set(0)
  t:eq(fresh:script_has("geo_effect"), false)
  fresh:geffect_set(5)
  t:eq(fresh:script_has("geo_effect"), true)
  fresh:geffect_set(0)
  t:eq(fresh:script_has("geo_effect"), false)

  room:geffect_set(0)
  t:eq(room:script_has("geo_effect"), false)
  t:eq(room:script_add("geo_effect"), true)
  t:eq(room:script_has("geo_effect"), true)
  t:eq(room:script_remove("geo_effect", "test cleanup"), true)

  room:geffect_set(5)
  t:eq(room:script_has("geo_effect"), true)

  room:geffect_set(0)
  t:eq(room:script_has("geo_effect"), false)
end)

test:case("room geo_effect progresses lava by room event", function(t)
  local room = assert(dbat.rooms.by_id(1))
  local old_random = math.random

  room:geffect_set(5)
  t:eq(room:script_has("geo_effect"), true)

  math.random = function(a, b) return b or a end
  dbat.get("room_scripts", "geo_effect").on_event(room, room:script("geo_effect"), "tick")
  math.random = old_random

  t:eq(room:geffect_get(), 6)
  t:eq(room:script_has("geo_effect"), false)

  room:geffect_set(0)
end)

return test:run()
