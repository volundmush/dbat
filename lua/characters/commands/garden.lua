local dbat          = require("dbat")
local act           = dbat.lib.act
local search        = dbat.lib.search
local text          = dbat.lib.text
local partial_match = require("lua.libs.utils").partial_match

local SUBCMDS = { "collect", "water", "harvest", "dig", "plant", "pick" }

local PULSE_3SEC = 30  -- 3 * PASSES_PER_SEC (10)
local PULSE_4SEC = 40  -- 4 * PASSES_PER_SEC (10)

-- SEARCH_GENUINE | SEARCH_WORKING: skip duplicate/broken items
local SEARCH_WORKING_GENUINE = 3

-- Plant vnums that use the harvest subcommand (clippers)
local HARVESTABLE_VNUMS = {
  [250]   = true,
  [1129]  = true,
  [17210] = true,
  [17211] = true,
  [17214] = true,
  [17216] = true,
  [17218] = true,
  [17220] = true,
  [17222] = true,
  [17224] = true,
  [17226] = true,
  [3702]  = true,
}

local PLANT_TABLE = {
  [250]   = { fruit = 1,     extract = false, bonus = 0,   cap = 2, floor = 0, mult_25 = false, rand_3 = false },
  [1129]  = { fruit = 1131,  extract = true,  bonus = 0,   cap = 0, floor = 0, mult_25 = false, rand_3 = false },
  [17210] = { fruit = 17212, extract = true,  bonus = 2,   cap = 0, floor = 0, mult_25 = false, rand_3 = false },
  [17211] = { fruit = 17213, extract = true,  bonus = 2,   cap = 0, floor = 0, mult_25 = false, rand_3 = false },
  [17214] = { fruit = 17215, extract = true,  bonus = 1,   cap = 0, floor = 0, mult_25 = false, rand_3 = false },
  [17216] = { fruit = 17217, extract = true,  bonus = 0,   cap = 0, floor = 0, mult_25 = false, rand_3 = false },
  [17218] = { fruit = 17219, extract = true,  bonus = 14,  cap = 0, floor = 0, mult_25 = false, rand_3 = false },
  [17220] = { fruit = 17221, extract = true,  bonus = 0,   cap = 0, floor = 0, mult_25 = true,  rand_3 = false },
  [17222] = { fruit = 17223, extract = true,  bonus = 0,   cap = 0, floor = 0, mult_25 = false, rand_3 = true  },
  [17224] = { fruit = 17225, extract = true,  bonus = 10,  cap = 0, floor = 0, mult_25 = false, rand_3 = false },
  [17226] = { fruit = 17227, extract = true,  bonus = -8,  cap = 0, floor = 1, mult_25 = false, rand_3 = false },
  [3702]  = { fruit = 3703,  extract = true,  bonus = -2,  cap = 0, floor = 0, mult_25 = false, rand_3 = false },
}

local SOIL_MATGOAL_REDUCTIONS = { [1]=10, [2]=15, [3]=20, [4]=25, [5]=50, [6]=60, [7]=70 }

local function harvest_plant(ch, plant, skill)
  local reward = math.random(5, 15)

  -- Soil quality bonus
  local soilq = plant:value_get(8)  -- VAL_SOILQ
  if soilq > 7 then
    reward = reward + 10
    ch:send_raw("@GThe soil seems to have made the plant exteremely bountiful")
  elseif soilq >= 5 then
    reward = reward + 6
    ch:send_raw("@GThe soil seems to have made the plant very bountiful")
  elseif soilq >= 3 then
    reward = reward + 4
    ch:send_raw("@GThe soil seems to have made the plant bountiful")
  elseif soilq > 0 then
    reward = reward + 2
    ch:send_raw("@GThe soil seems to have made the plant a bit more bountiful")
  end

  -- Skill bonus
  if skill >= 100 then
    reward = reward + 10
    ch:send_line(" and your outstanding skill has helped the plant be more bountiful yet!@n")
  elseif skill >= 90 then
    reward = reward + 8
    ch:send_line(" and your great skill has also helped the plant be more bountiful yet!@n")
  elseif skill >= 80 then
    reward = reward + 5
    ch:send_line(" and your good skill has also helped the plant be more bountiful yet!@n")
  elseif skill >= 50 then
    reward = reward + 3
    ch:send_line(" and your decent skill has also helped the plant be more bountiful yet!@n")
  elseif skill >= 40 then
    reward = reward + 2
    ch:send_line(" and your mastery of the basics of gardening has also helped the plant be more bountiful yet!@n")
  elseif skill >= 30 then
    reward = reward + 1
    ch:send_line(" and you somehow managed to make the plant slightly more bountiful with what little you know!@n")
  else
    ch:send_line(".@n")
  end

  local entry = PLANT_TABLE[plant:proto_id_get()]
  if not entry then
    dbat.log(string.format("SYSERR: harvest_plant called for illegitimate plant, VNUM %d.", plant:proto_id_get()))
    return
  end

  if entry.rand_3 then
    reward = reward + math.random(1, 3)
  elseif entry.mult_25 then
    reward = reward - math.floor(reward * 0.75)
  else
    reward = reward + entry.bonus
  end
  if entry.cap > 0 and reward > entry.cap then reward = entry.cap end
  if entry.floor > 0 and reward < entry.floor then reward = entry.floor end

  local proto = dbat.obj_protos.by_id(entry.fruit)
  local last_fruit
  for _ = 1, reward do
    last_fruit = proto:spawn()
    last_fruit:to_char(ch)
  end
  ch:send_line("@YYou harvest @D[@G%d@D]@Y @g%s@Y!@n",
    reward, (last_fruit and last_fruit:short_description_get()) or "fruit")

  if entry.extract then
    ch:send_line("@wThe harvesting process has killed the plant. Do not worry, this is normal for that type.@n")
    plant:extract()
  else
    plant:value_set(2, 3)  -- VAL_MATURITY = 3
    plant:value_set(0, 0)  -- VAL_GROWTH = 0
  end
end

local function execute(ctx)
  local ch = ctx.ch

  local tokens = ctx.argparams.tokens
  local arg    = string.lower(tokens[1] or "")
  local subcmd = partial_match(SUBCMDS, string.lower(tokens[2] or ""))

  -- Auto-learn gardening on first use
  if not ch:know_skill("gardening") then
    local slots_used  = ch:slot_count()
    local slots_total = ch:der_total("skill_slots")
    if slots_used + 1 <= slots_total then
      ch:skill_base_set("gardening", math.random(8, 16))
      ch:send_line("@GYou learn the very basics of gardening.")
    else
      ch:send_line("You need additional skill slots to pick up the skill linked with this.")
      return
    end
  end

  local room = ch:room_get()
  if not room then return end

  local RF = dbat.consts.room_flags
  local ST = dbat.consts.sector_types

  -- collect subcommand (works from any room with soil terrain)
  if partial_match(SUBCMDS, arg) == "collect" then
    local shovel = ch:inventory_find_vnum(254, SEARCH_WORKING_GENUINE)
    if not shovel then
      ch:send_line("You need a shovel in order to collect soil.")
      return
    end
    local sect = room:sector_type_get()
    if sect ~= ST.FOREST and sect ~= ST.FIELD and sect ~= ST.MOUNTAIN and sect ~= ST.HILLS then
      ch:send_line("You can not collect soil from this area.")
      return
    end
    local soil = dbat.obj_protos.by_id(255):spawn()
    if room:flagged(RF.FERTILE1) then
      act.to_actor("@yYou sink your shovel into the soft ground and manage to dig up a pile of fertile soil!@n", { actor = ch })
      act.around(ch, "@w$n@y sinks $s shovel into the soft ground and manages to dig up a pile of fertile soil!@n", { actor = ch })
      soil:value_set(0, 8)
    elseif room:flagged(RF.FERTILE2) then
      act.to_actor("@yYou sink your shovel into the soft ground and manage to dig up a pile of good soil!@n", { actor = ch })
      act.around(ch, "@w$n@y sinks $s shovel into the soft ground and manages to dig up a pile of good soil!@n", { actor = ch })
      soil:value_set(0, math.random(5, 7))
    else
      act.to_actor("@yYou sink your shovel into the soft ground and manage to dig up a pile of soil!@n", { actor = ch })
      act.around(ch, "@w$n@y sinks $s shovel into the soft ground and manages to dig up a pile of soil!@n", { actor = ch })
      soil:value_set(0, math.random(0, 4))
    end
    soil:to_char(ch)
    ch:wait_set(PULSE_4SEC)
    return
  end

  if arg == "" or not subcmd then
    ch:send_line("Syntax: garden (plant) ( water | harvest | dig | plant | pick )")
    ch:send_line("Syntax: garden collect [Will collect soil from a room with soil.")
    return
  end

  if not room:flagged(RF.GARDEN1) and not room:flagged(RF.GARDEN2) then
    ch:send_line("You are not even in a garden!")
    return
  end

  -- Find the target object
  local obj
  if subcmd == "plant" then
    obj = search.new(ch):add_character_inventory(ch):find_one(arg)
    if not obj then
      ch:send_line("What are you trying to plant?")
      ch:send_line("Syntax: garden (plant in inventory) plant")
      return
    end
  else
    obj = search.new(ch):add_room_objects(room):find_one(arg)
    if not obj then
      ch:send_line("That plant doesn't seem to be here.")
      return
    end
  end

  local cost = math.floor(ch:meter_max("stamina") * 0.005) + math.random(50, 150)
  local skill = ch:skill_get("gardening")
  local act_ctx = { actor = ch, tool = obj }

  if ch:meter_current("stamina") < cost then
    ch:send_line(string.format("@WYou need at least @G%s@W stamina to garden.", text.add_commas(cost)))
    return
  end

  if subcmd == "water" then
    local water = ch:inventory_find_vnum(251)
    if not water then
      ch:send_line("You need a bottle of grow water in order to water the plant.")
      return
    end
    local wlevel = obj:value_get(6)  -- VAL_WATERLEVEL
    if wlevel >= 500 then
      ch:send_line("You stop as you realize that the plant already has enough water.")
      return
    elseif wlevel <= -10 then
      ch:send_line("The plant is dead!")
      return
    end
    ch:meter_mod("stamina", -cost)
    if skill < dbat.axion_dice(0) then
      act.to_actor("@GAs you go to water @g$p@G you end up sloppily wasting about half of it on the ground.@n", act_ctx)
      act.around(ch, "@g$n@G takes a bottle of grow water and sloshes some of it on @g$p@G.@n", act_ctx)
      obj:value_set(6, math.min(wlevel + 40, 500))
    else
      act.to_actor("@GYou calmly and expertly pour the grow water on @g$p@G.@n", act_ctx)
      act.around(ch, "@g$n@G calmly and expertly pours some grow water on @g$p@G.@n", act_ctx)
      obj:value_set(6, math.min(wlevel + 225, 500))
    end
    if obj:value_get(6) >= 500 then
      ch:send_line("@YThe plant is now at full water level.@n")
    end
    water:extract()
    ch:wait_set(PULSE_3SEC)
    ch:improve_skill("gardening", 0)

  elseif subcmd == "harvest" then
    local clippers = ch:inventory_find_vnum(253, SEARCH_WORKING_GENUINE)
    if not clippers then
      ch:send_line("You need a pair of gardening clippers in order to harvest the plant.")
      return
    end
    if not HARVESTABLE_VNUMS[obj:proto_id_get()] then
      ch:send_line("You can not harvest that plant. Instead, Syntax: garden (plant) (pick)")
      return
    end
    if obj:value_get(6) <= -10 then
      ch:send_line("That plant is dead!")
      return
    end
    if obj:value_get(2) < obj:value_get(3) then  -- VAL_MATURITY < VAL_MAXMATURE
      ch:send_line("You stop as you realize that the plant isn't mature enough to harvest.")
      return
    end
    ch:meter_mod("stamina", -cost)
    if skill < dbat.axion_dice(-5) then
      act.to_actor("@GAs you go to harvest @g$p@G you end up cutting it in half instead!@n", act_ctx)
      act.around(ch, "@g$n@G attempts to harvest @g$p@G with $s clippers, but accidently cuts the plant in half!@n", act_ctx)
      obj:extract()
    else
      act.to_actor("@GYou calmly and expertly harvest @g$p@G.@n", act_ctx)
      act.around(ch, "@g$n@G calmly and expertly harvests @g$p@G.@n", act_ctx)
      -- VAL_ALL_HEALTH = 4
      local hp = clippers:value_get(4) - 1
      clippers:value_set(4, hp)
      if hp <= 0 then
        ch:send_line("The clippers are now too dull to use.")
        ch:wait_set(PULSE_3SEC)
        ch:improve_skill("gardening", 0)
        return
      end
      harvest_plant(ch, obj, skill)
    end
    ch:wait_set(PULSE_3SEC)
    ch:improve_skill("gardening", 0)

  elseif subcmd == "dig" then
    local shovel = ch:inventory_find_vnum(254, SEARCH_WORKING_GENUINE)
    if not shovel then
      ch:send_line("You need a shovel in order to dig up the plant.")
      return
    end
    act.to_actor("@GYou calmly dig up @g$p@G.@n", act_ctx)
    act.around(ch, "@g$n@G calmly digs up @g$p@G.@n", act_ctx)
    ch:meter_mod("stamina", -cost)
    obj:from_room()
    obj:to_char(ch)
    obj:value_set(8, 0)  -- VAL_SOILQ = 0
    ch:wait_set(PULSE_3SEC)
    ch:improve_skill("gardening", 0)

  elseif subcmd == "plant" then
    local shovel = ch:inventory_find_vnum(254, SEARCH_WORKING_GENUINE)
    if not shovel then
      ch:send_line("You need a shovel in order to plant.")
      return
    end
    local soil = ch:inventory_find_vnum(255)
    if not soil then
      ch:send_line("You need a pile of soil in order to plant.")
      return
    end
    local count = room:saveroom_count()
    if count > 7 and room:flagged(RF.GARDEN1) then
      ch:send_line("This room already has all its planters full. Try digging up some plants.")
      return
    elseif count > 19 and room:flagged(RF.GARDEN2) then
      ch:send_line("This room already has all its planters full. Try digging up some plants.")
      return
    end
    ch:meter_mod("stamina", -cost)
    if skill < dbat.axion_dice(-5) then
      act.to_actor("@GYou end up digging a hole too shallow to hold @g$p@G. Better try again.@n", act_ctx)
      act.around(ch, "@g$n@G digs a very shallow hole in one of the planters and then realizes @g$p@G won't fit in it.@n", act_ctx)
    else
      act.to_actor("@GYou dig a proper sized hole and plant @g$p@G in it.@n", act_ctx)
      act.around(ch, "@g$n@G digs a proper sized hole in a planter and plants @g$p@G in it.@n", act_ctx)
      obj:from_char()
      obj:to_room(room)
      obj:value_set(3, 6)   -- VAL_MAXMATURE = 6
      obj:value_set(1, 200) -- VAL_MATGOAL = 200
      local soilq = soil:value_get(0)
      obj:value_set(8, soilq)  -- VAL_SOILQ
      local reduction = SOIL_MATGOAL_REDUCTIONS[soilq] or 80
      obj:value_set(1, 200 - reduction)
      soil:extract()
    end
    ch:wait_set(PULSE_3SEC)
    ch:improve_skill("gardening", 0)

  elseif subcmd == "pick" then
    local EF = dbat.consts.item_extra_flags
    if not obj:extra_flagged(EF.MATURE) then
      ch:send_line("You can't pick that type of plant. Syntax: garden (plant) harvest")
      return
    end
    if obj:value_get(2) < obj:value_get(3) then  -- VAL_MATURITY < VAL_MAXMATURE
      ch:send_line("That plant is not mature enough yet.")
      return
    end
    if skill < dbat.axion_dice(-5) then
      act.to_actor("@GYou end up shredding @g$p@G with your clumsy unskilled hands.@n", act_ctx)
      act.around(ch, "@g$n@G grabs a hold of @g$p@G and shreds it in an attempt to pick it.@n", act_ctx)
      return
    end
    act.to_actor("@GYou grab a hold of @g$p@G and carefully pick it out of the soil.@n", act_ctx)
    act.around(ch, "@g$n@G grabs a hold of @g$p@G and carefully picks it out of the soil.@n", act_ctx)
    obj:from_room()
    obj:to_char(ch)
    ch:meter_mod("stamina", -cost)
    ch:wait_set(PULSE_3SEC)
    ch:improve_skill("gardening", 0)

  end
end

return {
  id      = "garden",
  aliases = { { "garden", 3 } },
  execute = execute,
}
