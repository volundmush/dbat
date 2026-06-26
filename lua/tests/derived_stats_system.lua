local test = require("lua.test").new()
local dbat = require("dbat")

local function mob()
  return dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1))
end

test:case("derived definitions are registered", function(t)
  local powerlevel = dbat.get("derived", "powerlevel")
  t:assert(powerlevel ~= nil, "powerlevel derived stat should exist")
  t:eq(powerlevel.name, "Power Level")
end)

test:case("derived base and total follow base stat", function(t)
  local ch = mob()
  ch:stat_set("strength", 25)
  t:eq(ch:der_base("strength"), 25)
  t:eq(ch:der_total("strength"), 25)

  ch:stat_set("powerlevel", 1000)
  t:eq(ch:der_base("powerlevel"), 1000)
  t:eq(ch:der_total("powerlevel"), 1000)
end)

test:case("condition modifiers affect derived totals", function(t)
  local ch = mob()
  ch:stat_set("powerlevel", 1000)
  t:eq(ch:der_total("powerlevel"), 1000)

  t:eq(ch:condition_apply_number("kaioken", "level", 2), true)
  t:eq(ch:condition_number_get("kaioken", "level"), 2)
  t:eq(ch:der_total("powerlevel"), 1200)

  t:eq(ch:condition_remove("kaioken"), true)
  t:eq(ch:der_total("powerlevel"), 1000)
end)

test:case("multiform splits original power stats across clones", function(t)
  local ch = mob()
  local clone1 = mob()
  local clone2 = mob()

  ch:stat_set("powerlevel", 1200)
  ch:stat_set("ki", 900)
  ch:stat_set("stamina", 600)

  -- Register 2 clones in the Zig map and apply multiform_original condition
  ch:clone_add(clone1)
  ch:clone_add(clone2)
  t:eq(ch:condition_apply("multiform_original", "skill", "multiform"), true)
  t:eq(ch:der_base("powerlevel"), 400)
  t:eq(ch:der_base("ki"), 300)
  t:eq(ch:der_base("stamina"), 200)
end)

test:case("multiform clones use original derived bases", function(t)
  local original = mob()
  local clone1 = mob()
  local clone2 = mob()
  local clone = mob()

  original:stat_set("powerlevel", 1200)
  original:stat_set("ki", 900)
  original:stat_set("stamina", 600)
  clone:stat_set("powerlevel", 1)
  clone:stat_set("ki", 1)
  clone:stat_set("stamina", 1)

  original:clone_add(clone1)
  original:clone_add(clone2)
  t:eq(original:condition_apply("multiform_original", "skill", "multiform"), true)
  t:eq(clone:condition_apply_number("multiform", "original_id", original:id_get()), true)

  t:eq(clone:der_base("powerlevel"), original:der_base("powerlevel"))
  t:eq(clone:der_base("ki"), original:der_base("ki"))
  t:eq(clone:der_base("stamina"), original:der_base("stamina"))
end)

test:case("equipment weight follows equipped objects", function(t)
  local ch = mob()
  local obj1 = dbat.obj_protos.by_id(1):spawn()
  local obj2 = dbat.obj_protos.by_id(2):spawn()

  t:eq(ch:der_base("weight_equipment"), 0)

  obj1:equip(ch, 0)
  obj2:equip(ch, 1)
  t:eq(ch:der_base("weight_equipment"), obj1:weight_total_get() + obj2:weight_total_get())

  ch:unequip(0)
  t:eq(ch:der_base("weight_equipment"), obj2:weight_total_get())
end)

test:case("inventory carried and total weight follow carried objects", function(t)
  local ch = mob()
  local carried = dbat.obj_protos.by_id(3):spawn()
  local equipped = dbat.obj_protos.by_id(4):spawn()

  ch:stat_set("weight", 150)
  carried:to_char(ch)
  equipped:equip(ch, 0)

  t:eq(ch:der_base("weight_inventory"), carried:weight_total_get())
  t:eq(ch:der_base("weight_equipment"), equipped:weight_total_get())
  t:eq(ch:der_base("weight_carried"), carried:weight_total_get() + equipped:weight_total_get())
  t:eq(ch:der_base("weight_total"), 150 + ch:der_base("weight_carried"))
end)

test:case("carry capacity follows strength and powerlevel", function(t)
  local ch = mob()

  ch:stat_set("strength", 12)
  ch:stat_set("powerlevel", 800)
  t:eq(ch:der_base("weight_carry_capacity"), 604)

  ch:stat_set("strength", 1)
  ch:stat_set("powerlevel", 0)
  t:eq(ch:der_base("weight_carry_capacity"), 50)
end)

test:case("burden is fixed point weight over gravity capacity", function(t)
  local ch = mob()
  local room = ch:room_get()

  room:gravity_set(2)
  ch:stat_set("weight", 10)
  ch:stat_set("strength", 2)
  ch:stat_set("powerlevel", 0)

  t:eq(ch:der_base("weight_carry_capacity"), 100)
  t:eq(ch:der_base("weight_total"), 10)
  t:eq(ch:der_base("burden"), 200000)

  room:gravity_set(1)
end)

test:case("burden assumes normal gravity without a room and clamps full", function(t)
  local ch = mob()

  ch:from_room()
  ch:stat_set("weight", 200)
  ch:stat_set("strength", 2)
  ch:stat_set("powerlevel", 0)

  t:eq(ch:room_get(), nil)
  t:eq(ch:der_base("burden"), 1000000)
end)

test:case("effective powerlevel uses lower suppression or burden result", function(t)
  local ch = mob()

  ch:stat_set("powerlevel", 1000)
  ch:stat_set("strength", 1)
  ch:stat_set("weight", 50)
  ch:stat_set("suppression", 20)

  t:eq(ch:der_base("burden"), 909090)
  t:eq(ch:der_base("powerlevel_effective"), 90)

  ch:stat_set("suppression", 5)
  t:eq(ch:der_base("powerlevel_effective"), 50)
end)

return test:run()
