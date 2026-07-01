local dbat = require("dbat")

local POS    = dbat.consts.positions
local PLR    = dbat.consts.player_flags
local ADMLVL = dbat.consts.adm_levels

local M = {}

local function blocked_by_position(ch, minimum)
  if not minimum then return false end

  local pos = ch:position_get()
  if pos >= minimum or pos == POS.FIGHTING then return false end

  if pos == POS.DEAD then
    ch:send_line("Lie still; you are DEAD!!! :-(")
  elseif pos == POS.INCAP or pos == POS.MORTALLYW then
    ch:send_line("You are in a pretty bad shape, unable to do anything!")
  elseif pos == POS.STUNNED then
    ch:send_line("All you can do right now is think about the stars!")
  elseif pos == POS.SLEEPING then
    ch:send_line("In your dreams, or what?")
  elseif pos == POS.RESTING then
    ch:send_line("Nah... You feel too relaxed to do that..")
  elseif pos == POS.SITTING then
    ch:send_line("Maybe you should get on your feet first?")
  elseif pos == POS.FIGHTING then
    ch:send_line("No way!  You're fighting for your life!")
  end

  return true
end

local function blocked_by_global_state(ch)
  if not ch:is_npc()
      and ch:condition_has_tag("goop")
      and ch:admin_level_get() < ADMLVL.IMPL then
    ch:send_line("You only have your internal thoughts until your body has finished regenerating!")
    return true
  end

  if not ch:is_npc()
      and ch:player_flagged(PLR.FROZEN)
      and ch:admin_level_get() < ADMLVL.IMPL then
    ch:send_line("You try, but the mind-numbing cold prevents you...")
    return true
  end

  if not ch:is_npc() and ch:condition_has("spiral") then
    ch:send_line("You are occupied with your Spiral Comet attack!")
    return true
  end

  return false
end

function M.legacy(spec)
  return {
    id = spec.id,
    aliases = spec.aliases,
    priority = spec.priority,
    execute = function(ctx)
      local ch = ctx.ch
      if blocked_by_global_state(ch) then return end
      if blocked_by_position(ch, spec.min_position) then return end
      ch:item_legacy_command(spec.legacy or spec.id, ctx.arguments or "")
    end,
  }
end

return M
