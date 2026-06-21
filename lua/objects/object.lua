-- Constants from dbat are loaded lazily so this module can be
-- required at engine startup before dbat's registry is initialized.
local _C  -- lazy cache for dbat.consts tables

local function C()
    if _C then return _C end
    local d = require("dbat")
    _C = {
        EF  = d.consts.item_extra_flags,
        IT  = d.consts.item_types,
        PRF = d.consts.prf_flags,
        AF  = d.consts.aff_flags,
        MAT = d.consts.materials,
        CF  = d.consts.container_flags,
        RF  = d.consts.room_flags,
        ST  = d.consts.sector_types,
        dirs = d.consts.direction_names,
    }
    return _C
end

local function keywords_for(obj, viewer)
  local keywords = {}

  local name = obj:name_get()
  for word in string.gmatch(name or "", "%S+") do
    keywords[#keywords + 1] = word
  end

  return keywords
end

local function display_name_for(obj, viewer, prefix)
  viewer = viewer or obj

  return obj:short_description_get()
end

-- Used for furniture-based modifiers.
local function modifiers(obj)
  local mods = {}

  local vital_regen = 10000

  if obj:proto_id_get() == 19090 then
    vital_regen = vital_regen + 10000
  end
  if obj:proto_id_get() == 19091 then
    vital_regen = vital_regen + 30000
  end

  -- healing tanks
  if obj:proto_id_get() == 65 then
    vital_regen = 200000
  end

  if vital_regen ~= 0 then
    local label = obj:short_description_get()
    mods[#mods + 1] = { target = { "regen", "vitals" }, kind = "percent", value = vital_regen, label = label }
  end

  return mods
end

local function check_attack(obj, instance)
end

local function on_attacked(obj, instance)
    if instance.xp_credit > 0 then
        instance.attacker:gain_exp(instance.xp_credit)
    end
end

local function on_mud_hour(obj)
end

local function on_second(obj)
end

local function on_heartbeat(obj, hb)
end

local function on_event(obj, kind)
  local subsystem, id, event_name = kind:match("^([^:]+):([^:]+):?(.*)$")
  event_name = (event_name and event_name ~= "") and event_name or "tick"
  if subsystem == "script" then
    if not obj:script_has(id) then return end
    local def = require("dbat").get("object_scripts", id)
    if def and def.on_event then def.on_event(obj, obj:script(id), event_name) end
  end
end

-- ---------------------------------------------------------------------------
-- Inventory rendering
-- ---------------------------------------------------------------------------

local QUALITY_LABELS = {
  [0] = " @D[@wQuality @RC@D]@n",
  [1] = " @D[@wQuality @RC@D]@n",
  [2] = " @D[@wQuality @RC+@D]@n",
  [3] = " @D[@wQuality @yC++@D]@n",
  [4] = " @D[@wQuality @yB@D]@n",
  [5] = " @D[@wQuality @CB+@D]@n",
  [6] = " @D[@wQuality @CB++@D]@n",
  [7] = " @D[@wQuality @CA@D]@n",
  [8] = " @D[@wQuality @GA+@D]@n",
}

local PLANT_STAGES = {
  [0] = " @D[@ySeed@D]@n",
  [1] = " @D[@GSprout@D]@n",
  [2] = " @D[@GYoung@D]@n",
  [3] = " @D[@GMature@D]@n",
  [4] = " @D[@GBudding@D]@n",
  [5] = "@D[@GClose Harvest@D]@n",
  [6] = "@D[@gHarvest@D]@n",
}

-- Key used to group identical objects for stacking in inventory display.
-- Matches the multi-condition comparison in list_obj_to_char.
local function stack_key(obj)
  local c = C()
  local EF, IT = c.EF, c.IT
  local proto = obj:proto_id_get()
  local parts = {
    tostring(proto),
    (obj:short_description_get() or ""):lower(),
    (obj:description_get() or ""):lower(),
    obj:extra_flagged(EF.BROKEN) and "1" or "0",
    obj:extra_flagged(EF.DUPLICATE) and "1" or "0",
    tostring(obj:value_get(6)),
  }
  if obj:type_get() == IT.PLANT then
    parts[#parts+1] = tostring(obj:value_get(2))  -- VAL_MATURITY
  end
  if proto == 255 then
    parts[#parts+1] = tostring(obj:value_get(0))
  end
  return table.concat(parts, "\0")
end

-- Port of show_obj_to_char(SHOW_OBJ_SHORT) + show_obj_modifiers.
-- Returns a fully formatted string with trailing \n.
-- Count prefix (for stacking) is the caller's responsibility.
local function render_inventory_line(obj, viewer)
  local c = C()
  local EF, IT, PRF, AF, MAT, CF = c.EF, c.IT, c.PRF, c.AF, c.MAT, c.CF

  local t = {}

  -- PRF_ROOMFLAGS: vnum prefix
  if viewer:pref_flagged(PRF.ROOMFLAGS) then
    t[#t+1] = string.format("[%d] ", obj:proto_id_get())
  end

  -- PRF_IHEALTH: health + short description; otherwise just short description
  local sdesc = obj:short_description_get() or ""
  if viewer:pref_flagged(PRF.IHEALTH) then
    t[#t+1] = string.format("@D<@gH@D: @C%d@D>@w %s", obj:value_get(4), sdesc)
  else
    t[#t+1] = sdesc
  end

  -- Special VNUM suffixes
  local vid = obj:proto_id_get()
  if vid == 255 then
    t[#t+1] = QUALITY_LABELS[obj:value_get(0)] or ""
  elseif vid == 3424 then
    t[#t+1] = string.format(" @D[@bInk Remaining@D: @w%d@D]@n", obj:value_get(6))
  elseif vid == 3423 then
    t[#t+1] = string.format(" @D[@B%d@D/@B24 Inks@D]@n", obj:value_get(6))
  end

  -- THROW flag
  if obj:extra_flagged(EF.THROW) then
    t[#t+1] = " @D[@RThrow Only@D]@n"
  end

  -- Plant growth stages
  local otype = obj:type_get()
  if otype == IT.PLANT and not obj:extra_flagged(EF.MATURE) then
    if obj:value_get(6) < -9 then   -- VAL_WATERLEVEL = 6
      t[#t+1] = "@D[@RDead@D]@n"
    else
      t[#t+1] = PLANT_STAGES[obj:value_get(2)] or ""  -- VAL_MATURITY = 2
    end
  end

  -- Container open/closed (skip sheaths and corpses)
  if otype == IT.CONTAINER and obj:value_get(3) ~= 1 and not obj:extra_flagged(EF.SHEATH) then
    if (obj:value_get(1) & CF.CLOSED) == 0 then
      t[#t+1] = " @D[@G-open-@D]@n"
    else
      t[#t+1] = " @D[@rclosed@D]@n"
    end
  end

  -- Duplicate
  if obj:extra_flagged(EF.DUPLICATE) then
    t[#t+1] = " @D[@YDuplicate@D]@n"
  end

  -- show_obj_modifiers: visibility and aura flags
  if obj:extra_flagged(EF.INVISIBLE) then t[#t+1] = " (invisible)" end
  if obj:extra_flagged(EF.BLESS) and viewer:aff_flagged(AF.DETECT_ALIGN) then
    t[#t+1] = " ..It glows blue!"
  end
  if obj:extra_flagged(EF.MAGIC) and viewer:aff_flagged(AF.DETECT_MAGIC) then
    t[#t+1] = " ..It glows yellow!"
  end
  if obj:extra_flagged(EF.GLOW) then t[#t+1] = " @D(@GGlowing@D)@n" end
  if obj:extra_flagged(EF.HOT)  then t[#t+1] = " @D(@RHOT@D)@n"     end
  if obj:extra_flagged(EF.HUM)  then t[#t+1] = " @D(@RHumming@D)@n" end

  -- Token slots
  if obj:extra_flagged(EF.SLOT2) then
    if obj:extra_flagged(EF.SLOT_ONE) and not obj:extra_flagged(EF.SLOTS_FILLED) then
      t[#t+1] = " @D[@m1/2 Tokens@D]@n"
    elseif obj:extra_flagged(EF.SLOTS_FILLED) then
      t[#t+1] = " @D[@m2/2 Tokens@D]@n"
    else
      t[#t+1] = " @D[@m0/2 Tokens@D]@n"
    end
  end
  if obj:extra_flagged(EF.SLOT1) then
    if obj:extra_flagged(EF.SLOTS_FILLED) then
      t[#t+1] = " @D[@m1/1 Tokens@D]@n"
    else
      t[#t+1] = " @D[@m0/1 Tokens@D]@n"
    end
  end

  -- Ki charge distance
  if obj:kicharge_get() > 0 then
    t[#t+1] = string.format(" %d meters away", obj:distance_get() * 20 + math.random(1, 5))
  end

  -- Custom / restring labels
  if obj:extra_flagged(EF.CUSTOM)   then t[#t+1] = " @D(@YCUSTOM@D)@n" end
  if obj:extra_flagged(EF.RESTRING) then t[#t+1] = " @D(@R*@D)@n"      end

  -- Broken condition or trailing period
  if obj:extra_flagged(EF.BROKEN) then
    local mat = obj:value_get(7)  -- VAL_ALL_MATERIAL = 7
    if mat == MAT.STEEL or mat == MAT.MITHRIL or mat == MAT.METAL then
      t[#t+1] = ", and appears to be twisted and broken."
    elseif mat == MAT.WOOD then
      t[#t+1] = ", and is broken into hundreds of splinters."
    elseif mat == MAT.GLASS then
      t[#t+1] = ", and is shattered on the ground."
    elseif mat == MAT.STONE then
      t[#t+1] = ", and is a pile of rubble."
    else
      t[#t+1] = ", and is broken."
    end
  else
    if otype ~= IT.BOARD and otype ~= IT.CONTAINER then
      t[#t+1] = "."
    end
  end

  t[#t+1] = "\n"
  return table.concat(t)
end

-- ---------------------------------------------------------------------------
-- Room rendering
-- ---------------------------------------------------------------------------

local SEE_PLANT_STAGES = {
  [0] = "@wA @G%s@y seed@w has been planted here. @D(@C%d Water Hours@D)@n\r\n",
  [1] = "@wA very young @G%s@w has sprouted from a planter here. @D(@C%d Water Hours@D)@n\r\n",
  [2] = "@wA half grown @G%s@w is in a planter here. @D(@C%d Water Hours@D)@n\r\n",
  [3] = "@wA mature @G%s@w is growing in a planter here. @D(@C%d Water Hours@D)@n\r\n",
  [4] = "@wA mature @G%s@w is flowering in a planter here. @D(@C%d Water Hours@D)@n\r\n",
  [5] = "@wA mature @G%s@w that is close to harvestable is here. @D(@C%d Water Hours@D)@n\r\n",
  [6] = "@wA @Rharvestable @G%s@w is in the planter here. @D(@C%d Water Hours@D)@n\r\n",
}

local function see_plant(obj)
  local water = obj:value_get(6)  -- VAL_WATERLEVEL
  local sdesc = obj:short_description_get() or ""
  if water >= 0 then
    local tmpl = SEE_PLANT_STAGES[obj:value_get(2)]  -- VAL_MATURITY
    if tmpl then return string.format(tmpl, sdesc, water) end
    return ""
  elseif water > -4 then
    return string.format("@yA @G%s@y that is looking a bit @rdry@y, is here.@n\r\n", sdesc)
  elseif water > -10 then
    return string.format("@yA @G%s@y that is looking extremely @rdry@y, is here.@n\r\n", sdesc)
  else
    return string.format("@yA @G%s@y that is completely @rdead@y and @rwithered@y, is here.@n\r\n", sdesc)
  end
end

-- Key for grouping identical objects in a room listing.
-- Extends stack_key with room-specific exclusions from list_obj_to_char.
local function room_stack_key(obj)
  if obj:sitting_get() ~= 0 then return nil end   -- occupied furniture never stacks
  if obj:post_type_get() ~= 0 then return nil end  -- posted notes never stack
  if obj:fellow_wall_has() then return nil end      -- Glacian Walls never stack
  return stack_key(obj)
end

-- Port of show_obj_to_char(SHOW_OBJ_LONG) + show_obj_modifiers.
-- Returns a fully formatted string with trailing \r\n, or nil to skip.
local function render_room_line(obj, viewer)
  local c = C()
  local EF, IT, PRF, AF, CF, RF, ST =
        c.EF, c.IT, c.PRF, c.AF, c.CF, c.RF, c.ST

  local desc = obj:description_get() or ""

  -- Hidden objects: dot-prefix desc hidden unless viewer has HOLYLIGHT
  if desc:sub(1, 1) == "." and not viewer:pref_flagged(PRF.HOLYLIGHT) then
    return nil
  end

  -- Vehicle: skip if viewer is inside this vehicle
  if obj:type_get() == IT.VEHICLE then
    if viewer:room_vnum_get() == obj:value_get(0) then return nil end
  end

  -- Occupied furniture
  local t = {}
  if obj:sitting_get() ~= 0 then
    if viewer:admin_level_get() < 1 then return nil end
    t[#t+1] = "@D(@YBeing Used@D)@w"
  end

  -- Garden room plant: detailed water-level display
  local room = obj:room_get()
  if obj:type_get() == IT.PLANT and room then
    if room:flagged(RF.GARDEN1) or room:flagged(RF.GARDEN2) then
      return table.concat(t) .. see_plant(obj)
    end
  end

  -- Buried object
  if obj:extra_flagged(EF.BURIED) then
    local spotted = viewer:skill_get("spot") > math.random(20, 110)
    if not spotted then return nil end
    local w = obj:weight_get()
    local label
    if obj:value_get(3) == 1 then  -- IS_CORPSE: value[3] == 1
      label = "recent grave covered by"
    elseif w < 10 then
      label = "small mound of"
    elseif w < 50 then
      label = "medium sized mound of"
    elseif w < 1000 then
      label = "large mound of"
    else
      label = "gigantic mound of"
    end
    local medium = (room and room:sector_type_get() == ST.DESERT) and "sand" or "dirt"
    return string.format("@yA %s soft %s is here.@n\r\n", label, medium)
  end

  -- Special VNUMs
  local vid = obj:proto_id_get()
  local text = require("dbat").lib.text

  if vid == 11 then
    t[#t+1] = string.format(
      "@wA gravity generator, set to %sx gravity, is built here",
      text.add_commas(obj:weight_get()))
  elseif vid == 79 then
    local dir = c.dirs[obj:cost_get() + 1] or "?"
    t[#t+1] = string.format(
      "@wA @cG@Cl@wa@cc@Ci@wa@cl @wW@ca@Cl@wl @D[@C%s@D]@w is blocking access to the @G%s@w direction",
      text.add_commas(obj:weight_get()), dir)
  else
    t[#t+1] = "@w"
    -- ROOMFLAGS vnum prefix (only for non-posted objects)
    if viewer:pref_flagged(PRF.ROOMFLAGS) and obj:post_type_get() == 0 then
      t[#t+1] = string.format("@D[@G%d@D]@w ", vid)
    end

    local ptype = obj:post_type_get()
    if ptype > 0 then
      if obj:is_posted() then
        return nil  -- posted to a specific target, don't show
      else
        t[#t+1] = string.format("%s@w, has been posted here.@n", obj:short_description_get() or "")
      end
    else
      t[#t+1] = desc .. "@n"

      local otype = obj:type_get()

      -- Vehicle door/hatch open indicator
      if otype == IT.VEHICLE then
        if (obj:value_get(1) & CF.CLOSED) == 0 then
          if vid > 19199 then
            t[#t+1] = "\r\n@c...its outer hatch is open@n"
          else
            t[#t+1] = "\r\n@c...its door is open@n"
          end
        end
      end

      -- Container open/closed (skip sheaths and corpses)
      if otype == IT.CONTAINER and obj:value_get(3) ~= 1 and not obj:extra_flagged(EF.SHEATH) then
        if (obj:value_get(1) & CF.CLOSED) == 0 then
          t[#t+1] = ". @D[@G-open-@D]@n"
        else
          t[#t+1] = ". @D[@rclosed@D]@n"
        end
      end

      -- Hatch open/closed/locked
      if otype == IT.HATCH then
        if (obj:value_get(1) & CF.CLOSED) == 0 then
          t[#t+1] = ", it is open"
        else
          t[#t+1] = ", it is closed"
        end
        if (obj:value_get(1) & CF.LOCKED) ~= 0 then
          t[#t+1] = " and locked@n"
        else
          t[#t+1] = "@n"
        end
      end

      -- Food: partially eaten
      if otype == IT.FOOD then
        if obj:value_get(0) < obj:foob_get() then
          t[#t+1] = ", and it has been ate on@n"
        end
      end
    end
  end

  -- show_obj_modifiers
  if obj:extra_flagged(EF.INVISIBLE) then t[#t+1] = " (invisible)" end
  if obj:extra_flagged(EF.BLESS) and viewer:aff_flagged(AF.DETECT_ALIGN) then
    t[#t+1] = " ..It glows blue!"
  end
  if obj:extra_flagged(EF.MAGIC) and viewer:aff_flagged(AF.DETECT_MAGIC) then
    t[#t+1] = " ..It glows yellow!"
  end
  if obj:extra_flagged(EF.GLOW) then t[#t+1] = " @D(@GGlowing@D)@n" end
  if obj:extra_flagged(EF.HOT)  then t[#t+1] = " @D(@RHOT@D)@n"     end
  if obj:extra_flagged(EF.HUM)  then t[#t+1] = " @D(@RHumming@D)@n" end

  t[#t+1] = "\r\n"
  return table.concat(t)
end

return {
  keywords_for = keywords_for,
  display_name_for = display_name_for,
  stack_key = stack_key,
  room_stack_key = room_stack_key,
  render_inventory_line = render_inventory_line,
  render_room_line = render_room_line,
  modifiers = modifiers,
  check_attack = check_attack,
  on_attacked = on_attacked,
  on_mud_hour = on_mud_hour,
  on_second = on_second,
  on_heartbeat = on_heartbeat,
  on_event = on_event,
}
