local dbat   = require("dbat")
local Search = dbat.lib.search.new
local act    = dbat.lib.act

local IT   = dbat.consts.item_types
local PLR  = dbat.consts.player_flags
local ADM  = dbat.consts.admin_flags
local SMH  = dbat.consts.secs_per_mud_hour
local OCMD = dbat.consts.obj_cmd_types
local COND = dbat.consts.player_conds

local function an(word)
  return (word or ""):match("^[aeiouAEIOU]") and "an" or "a"
end

-- Port of majin_gain() from act.item.cpp
local MAJIN_CAPS = { [-1] = 300000, [0] = 500000, [1] = 1200000, [2] = 1500000 }
local MAJIN_DIVS = { [-1] = 1200,   [0] = 1200,   [1] = 1000,    [2] = 900 }
local fmt = dbat.lib.text.add_commas

local function majin_gain(ch, gain_type)
  if ch:race_get() ~= "majin" or ch:is_npc() then return end
  if ch:is_soft_capped() then
    ch:send_line("You can not gain anymore from candy consumption at your current level.")
    return
  end
  local div = MAJIN_DIVS[gain_type]
  local cap = MAJIN_CAPS[gain_type]
  local level = ch:stat_get("level")
  local pl = math.min(cap, math.floor(ch:stat_get("powerlevel") / div) + math.random(level, level * 2))
  local ki = math.min(cap, math.floor(ch:stat_get("ki") / div) + math.random(level, level * 2))
  local st = math.min(cap, math.floor(ch:stat_get("stamina") / div) + math.random(level, level * 2))
  ch:stat_mod("powerlevel", pl)
  ch:stat_mod("ki", ki)
  ch:stat_mod("stamina", st)
  ch:send_line(string.format(
    "@mYou feel stronger after consuming the candy @D[@RPL@W: @r%s @CKi@D: @c%s @GSt@D: @g%s@D]@m!@n",
    fmt(pl), fmt(ki), fmt(st)))
end

-- Majin candy vnums → { gain_mode, st_recovery_fraction }
local MAJIN_CANDY = {
  [53] = { -1, 0.033 },
  [93] = {  0, 0.05  },
  [94] = {  1, 0.1   },
  [95] = {  2, 0.1   },
}

local function execute(ctx)
  local ch  = ctx.ch
  local arg = ctx.argparams.tokens[1] or ""
  local is_taste = ctx.alias:sub(1, 1):lower() == "t"

  if ch:is_npc() then return end

  local race = ch:race_get()
  if race == "android" or ch:stat_get("hunger") < 0 then
    ch:send_line("You need not eat!")
    return
  end
  if ch:player_flagged(PLR.HEALT) then
    ch:send_line("You are inside a healing tank!")
    return
  end
  if ch:condition_has("poison") then
    ch:send_line("You feel too sick from the poison to eat!")
    return
  end
  if arg == "" then
    ch:send_line("Eat what?")
    return
  end

  local food = Search(ch):add_character_inventory(ch):add_filter(function(s, e)
    return s:can_see(e)
  end):find_one(arg)
  if not food then
    ch:send_line(string.format("You don't seem to have %s %s.", an(arg), arg))
    return
  end

  -- thirst before hunger cross-check
  if ch:stat_get("thirst") <= 1 and ch:stat_get("hunger") >= 2 then
    ch:send_line("You need to drink first!")
    return
  end

  -- tasting a drinkcon redirects to drink/sip
  local ftype = food:type_get()
  if is_taste and (ftype == IT.DRINKCON or ftype == IT.FOUNTAIN) then
    -- rebuild command context for drink/sip
    local drink_cmd = dbat.characters.registry.commands["drink"]
    if drink_cmd then
      local fake_ctx = {
        ch = ch, actor = ch,
        command = "drink", alias = "sip",
        arguments = arg,
        argparams = { raw = arg, tokens = { arg }, equals = false, lsargs = arg, rsargs = "", left_tokens = { arg }, right_tokens = {} },
      }
      drink_cmd.execute(fake_ctx)
    end
    return
  end

  if ftype ~= IT.FOOD and ftype ~= IT.YUM then
    ch:send_line("You can't eat THAT!")
    return
  end

  if not dbat.dgscripts.consume_otrigger(food, ch, OCMD.EAT) then return end

  -- Majin candy special case
  local candy = MAJIN_CANDY[food:vnum_get()]
  if candy then
    local gain_mode, st_frac = candy[1], candy[2]
    ch:meter_mod("lifeforce", math.floor(1000000 * 0.01))
    ch:meter_mod("stamina", math.floor(1000000 * st_frac))
    if food:extra_flagged(dbat.consts.item_extra_flags.FORGED) then
      ch:send_line("This is a forgery. You gain nothing!")
    else
      ch:send_line("You feel more energetic!")
      majin_gain(ch, gain_mode)
    end
    food:extract()
    return
  end

  if ch:stat_get("hunger") >= 48 then
    if race == "majin" then
      ch:send_line("You are full, but there's always room for candy!")
    else
      ch:send_line("You are full.")
    end
    return
  end

  if not is_taste then
    act.to_char(ch, "You eat some of $p.", { object = food })
    if food:action_description_get() then
      act.to_char(ch, food:action_description_get(), { object = food })
    end
    act.around(ch, "$n eats some of $p.", { object = food })
  else
    act.to_char(ch, "You nibble a little bit of $p.", { object = food })
    act.around(ch, "$n tastes a little bit of $p.", { object = food })
    if food:value_get(3) ~= 0 then  -- VAL_FOOD_POISON
      ch:send_line("It has a sickening taste! Better not eat it...")
      return
    end
  end

  -- VAL_FOOD_FOODVAL=0, VAL_FOOD_MAXFOODVAL=1
  local foob = 48 - ch:stat_get("hunger")
  local maxfval = math.max(1, food:value_get(1))
  local amount
  if not is_taste then
    amount = math.max(1, math.min(foob, food:value_get(0)))
  else
    amount = 1
  end
  local percent_eaten = amount / maxfval

  ch:gain_condition(COND.HUNGER, amount)

  if not ch:condition_has("food_restored") and not is_taste then
    local stamina_pct = amount
    if ftype == IT.YUM then stamina_pct = stamina_pct + 25 end
    ch:condition_apply_with_number("food_restored", "eat", "food restoration", "amount", stamina_pct)
  end

  if not is_taste then
    -- VAL_FOOD_PSBONUS=2, VAL_FOOD_EXPBONUS=6
    local psbonus  = math.floor((maxfval + food:value_get(2)) * percent_eaten)
    local expbonus = math.floor(food:value_get(6) * (ch:stat_get("level") * 0.4 + 1) * percent_eaten)
    ch:gain_exp(expbonus)
    ch:stat_mod("practices", psbonus)
    ch:send_line(string.format(
      "That was exceptionally delicious! @D[@mPS@D: @C+%d@D] [@gEXP@D: @G+%s@D]@n",
      psbonus, fmt(expbonus)))

    if food:value_get(3) == 0 and ch:meter_current("powerlevel") < ch:meter_max("powerlevel") then
      if food:weight_get() < 6 then
        ch:meter_mod("powerlevel", math.floor(1000000 * 0.05))
      else
        ch:meter_mod("powerlevel", math.floor(1000000 * 0.1))
      end
      if ftype == IT.YUM then
        ch:meter_mod("powerlevel", math.floor(1000000 * 0.2))
      end
      ch:send_line("@MYou feel some of your strength return!@n")
    end
  end

  if ch:stat_get("hunger") >= 48 then
    if race == "majin" then
      ch:send_line("You are full, but there's always room for candy!")
    else
      ch:send_line("You are full.")
    end
  end

  if food:value_get(3) ~= 0 and not ch:adm_flagged(ADM.NOPOISON) then
    ch:send_line("Oops, that tasted rather strange!")
    act.around(ch, "$n coughs and utters some strange sounds.")
    ch:condition_apply_with_duration("poison", "eat", "poison", amount * 2 * SMH)
  end

  -- Consume food
  local new_foodval = food:value_get(0) - amount
  food:value_set(0, new_foodval)
  if new_foodval <= 0 then
    ch:send_line("You finish the last bite.")
    food:extract()
  end
end

return {
  id      = "eat",
  aliases = {
    { "eat",   2 },
    { "taste", 3 },
  },
  execute = execute,
}
