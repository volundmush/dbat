local test = require("lua.test").new()
local dbat = require("dbat")

local WEAR = dbat.consts.wear_positions
local EF = dbat.consts.item_extra_flags

local function mob()
  return assert(dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1)))
end

test:case("right arm meter update drops wielded item", function(t)
  local ch = mob()
  local obj = assert(dbat.obj_protos.by_id(3):spawn())

  obj:equip(ch, WEAR.WIELD1)
  t:eq(ch:equipment_get(WEAR.WIELD1):id_get(), obj:id_get())

  ch:meter_set_int("limb_right_arm", 0)

  t:eq(ch:equipment_get(WEAR.WIELD1), nil)
  t:eq(obj:carried_by_get(), ch:id_get())
  t:eq(ch:meter_current("limb_right_arm"), 0)

  obj:extract()
  ch:extract()
end)

test:case("broken_set removes worn item immediately", function(t)
  local ch = mob()
  local obj = assert(dbat.obj_protos.by_id(3):spawn())

  obj:broken_set(false)
  obj:equip(ch, WEAR.HEAD)
  t:eq(ch:equipment_get(WEAR.HEAD):id_get(), obj:id_get())

  obj:broken_set(true)

  t:eq(obj:extra_flagged(EF.BROKEN), true)
  t:eq(ch:equipment_get(WEAR.HEAD), nil)
  t:eq(obj:carried_by_get(), ch:id_get())

  obj:extract()
  ch:extract()
end)

return test:run()
