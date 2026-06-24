local test = require("lua.test").new()
local dbat = require("dbat")

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

return test:run()
