local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act

local IT   = dbat.consts.item_types
local EF   = dbat.consts.item_extra_flags
local ST   = dbat.consts.sector_types
local PLR  = dbat.consts.player_flags
local OCMD = dbat.consts.obj_cmd_types
local SMH  = dbat.consts.secs_per_mud_hour
local COND = dbat.consts.player_conds

-- (Drunk, Hunger, Thirst) per liquid type — mirrors drink_aff[] in itemdata.cpp
local DRINK_AFF = {
  [0]  = {0, 0, 5}, [1]  = {3, 1, 1}, [2]  = {3, 1, 1}, [3] = {3, 0, 2},
  [4]  = {4, 0, 2}, [5]  = {6, 0, 0}, [6]  = {0, 0, 4}, [7] = {6, 0, 0},
  [8]  = {0, 0, 3}, [9]  = {0, 0, 4}, [10] = {0, 1, 3}, [11]= {0, 0, 3},
  [12] = {0, 0, 3}, [13] = {0, 0, 2}, [14] = {0, 0, 1}, [15]= {0, 0, 8},
}

local DRINK_NAMES = {
  [0]  = "water",     [1]  = "beer",            [2]  = "wine",
  [3]  = "ale",       [4]  = "dark ale",         [5]  = "whisky",
  [6]  = "lemonade",  [7]  = "firebreather",     [8]  = "local speciality",
  [9]  = "juice",     [10] = "milk",             [11] = "tea",
  [12] = "coffee",    [13] = "blood",            [14] = "salt water",
  [15] = "clear water",
}

-- VAL indices for drinkcons
local CAP   = 0  -- VAL_DRINKCON_CAPACITY
local FULL  = 1  -- VAL_DRINKCON_HOWFULL
local LIQ   = 2  -- VAL_DRINKCON_LIQUID
local POIS  = 3  -- VAL_DRINKCON_POISON

local function wellspring_ki(ch, was_thirsty, is_sip)
  if is_sip then return end
  local wskill = ch:skill_get("wellspring")
  if wskill == 0 then return end
  if ch:meter_current("ki") >= ch:meter_max("ki") then return end
  if was_thirsty > 30 then return end
  local ki_gain = math.floor(
    (ch:meter_max("ki") * 0.005 + ch:stat_get("wisdom") * math.random(80, 100)) * wskill
  )
  ch:meter_mod_int("ki", ki_gain)
  if ch:meter_current("ki") >= ch:meter_max("ki") then
    ch:send_line("You feel your ki return to full strength.")
  else
    ch:send_line("You feel your ki has rejuvenated.")
  end
end

local function execute(ctx)
  local ch  = ctx.ch
  local arg = ctx.argparams.tokens[1] or ""
  local is_sip = ctx.alias:sub(1, 1):lower() == "s"

  if ch:is_npc() then return end

  local race = ch:race_get()
  if race == "android" or ch:stat_get("thirst") < 0 then
    ch:send_line("You need not drink!")
    return
  end
  if ch:player_flagged(PLR.HEALT) then
    ch:send_line("You are inside a healing tank!")
    return
  end

  -- hunger before thirst cross-check (excluding namek and bio genome 3)
  if ch:stat_get("hunger") <= 1 and ch:stat_get("thirst") >= 2
      and race ~= "namek"
      and not ch:condition_has("genome_namek") then
    ch:send_line("You need to eat first!")
    return
  end

  local was_thirsty = ch:stat_get("thirst")

  -- No-arg: drink from water sector or sunken room
  if arg == "" then
    local room = ch:room_get()
    local sect = room and room:sector_type_get() or -1
    local is_water = sect == ST.WATER_SWIM or sect == ST.WATER_NOSWIM or sect == ST.UNDERWATER
    local is_sunken = room and room:is_sunken() or false

    if is_water or is_sunken then
      act.around(ch, "$n takes a refreshing drink from the surrounding water.")
      ch:send_line("You take a refreshing drink from the surrounding water.")
      ch:gain_condition(COND.THIRST, 1)
      if not is_sip then
        local wskill = ch:skill_get("wellspring")
        if wskill ~= 0 and ch:meter_current("ki") < ch:meter_max("ki") and was_thirsty <= 30 then
          local ki_gain = math.floor(
            (ch:meter_max("ki") * 0.005 + ch:stat_get("wisdom") * math.random(80, 100)) * wskill
          )
          ch:meter_mod_int("ki", ki_gain)
          ch:send_line("You feel your ki return to full strength.")
        end
      end
      if ch:stat_get("thirst") >= 48 then
        ch:send_line("You don't feel thirsty anymore.")
      end
      return
    else
      ch:send_line("Drink from what?")
      return
    end
  end

  -- Search inventory then room
  local on_ground = false
  local temp = Search(ch):add_character_inventory(ch):add_filter(function(s, e)
    return s:can_see(e)
  end):find_one(arg)
  if not temp then
    temp = Search(ch):add_room_objects(ch:room_get()):add_filter(function(s, e)
      return s:can_see(e)
    end):find_one(arg)
    if temp then on_ground = true end
  end

  if not temp then
    ch:send_line("You can't find it!")
    return
  end

  local ttype = temp:type_get()

  -- Special vnum 86: ki potion
  if ttype ~= IT.DRINKCON and ttype ~= IT.FOUNTAIN then
    if temp:vnum_get() == 86 then
      if on_ground then
        ch:send_line("You need to pick that up first.")
        return
      end
      act.to_char(ch,
        "@wYou uncork the $p and tip it to your lips. Drinking it down you feel a warmth flow through your body and your ki returns to full strength .@n",
        { object = temp })
      act.around(ch,
        "@C$n@w uncorks the $p and tips it to $s lips. Drinking it down and then smiling.@n",
        { object = temp })
      temp:from_char()
      temp:extract()
      ch:meter_set_int("ki", ch:meter_max("ki"))
      ch:gain_condition(COND.THIRST, 8)
      return
    else
      ch:send_line("You can't drink from that!")
      return
    end
  end

  if on_ground and ttype == IT.DRINKCON then
    ch:send_line("You have to be holding that to drink from it.")
    return
  end
  if temp:extra_flagged(EF.BROKEN) then
    ch:send_line("Seems like it's broken!")
    return
  end
  if temp:extra_flagged(EF.FORGED) then
    ch:send_line("Seems like it doesn't work, maybe it is fake...")
    return
  end

  if ch:stat_get("drunk") > 10 and ch:stat_get("thirst") > 0 then
    ch:send_line("You can't seem to get close enough to your mouth.")
    act.around(ch, "$n tries to drink but misses $s mouth!")
    return
  end

  if temp:value_get(FULL) <= 0 and temp:value_get(CAP) >= 1 then
    ch:send_line("It's empty.")
    return
  end

  if not dbat.dgscripts.consume_otrigger(temp, ch, OCMD.DRINK) then return end

  local liquid = temp:value_get(LIQ)
  local liq_name = DRINK_NAMES[liquid] or "something"
  local amount

  if not is_sip then
    act.around(ch, string.format("$n drinks %s from $p.", liq_name), { object = temp })
    ch:send_line(string.format("You drink the %s.", liq_name))
    if temp:action_description_get() then
      act.to_char(ch, temp:action_description_get(), { object = temp })
    end
    amount = 4
  else
    act.around(ch, "$n sips from $p.", { object = temp })
    ch:send_line(string.format("It tastes like %s.", liq_name))
    if temp:value_get(POIS) ~= 0 then
      ch:send_line("It has a sickening taste! Better not eat it...")
      return
    end
    amount = 1
  end

  amount = math.min(amount, temp:value_get(FULL))

  -- Weight reduction (only for non-eternal containers)
  if temp:value_get(CAP) > 0 then
    temp:drinkcon_weight_drain(amount)
  end

  local aff = DRINK_AFF[liquid] or {0, 0, 1}
  ch:gain_condition(COND.DRUNK,  aff[1] * amount)
  ch:gain_condition(COND.HUNGER, aff[2] * amount)
  ch:gain_condition(COND.THIRST, aff[3] * amount)

  if not ch:condition_has("food_restored") and not is_sip then
    ch:condition_apply_with_number("food_restored", "drink", "food restoration", "amount", amount)
  end

  -- Wellspring ki regen (water variants: liquid 0, 14, 15)
  if not is_sip and was_thirsty <= 30 then
    if liquid == 0 or liquid == 14 or liquid == 15 then
      wellspring_ki(ch, was_thirsty, false)
    end
  end

  if ch:stat_get("drunk") > 10 then
    ch:send_line("You feel drunk.")
  end
  if ch:stat_get("thirst") >= 48 then
    ch:send_line("You don't feel thirsty anymore.")
  end

  -- Poison (mutants with genome 7 are immune)
  if temp:value_get(POIS) ~= 0 then
    local is_immune = race == "mutant"
        and (ch:genome_get(0) == 7 or ch:genome_get(1) == 7)
    if not is_immune then
      ch:send_line("Oops, it tasted rather strange!")
      act.around(ch, "$n chokes and utters some strange sounds.")
      ch:condition_apply_with_duration("poison", "drink", "poison", amount * 3 * SMH)
    end
  end

  -- Reduce container contents
  if temp:value_get(CAP) > 0 then
    local new_full = temp:value_get(FULL) - amount
    temp:value_set(FULL, new_full)
    if new_full <= 0 then
      temp:drinkcon_name_update()
      temp:value_set(POIS, 0)
    end
  end
end

return {
  id      = "drink",
  aliases = {
    { "drink", 3 },
    { "sip",   3 },
  },
  execute = execute,
}
