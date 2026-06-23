local dbat     = require("dbat")
local act      = require("lua.libs.act")
local commands = require("lua.characters.commands")

local AFF = dbat.consts.aff_flags
local MF  = dbat.consts.mob_flags
local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions

local FIGHT_INTERVAL_MS = 5000

-- ── NPC combat helpers ────────────────────────────────────────────────────

local function flee_mob(ch, msg)
    act.around(ch, msg, {actor=ch})
    for obj in ch:inventory_get() do
        obj:extract()
    end
    ch:extract()
end

local function npc_combat_ai(ch, cond)
    local vict = ch:fighting_get()
    if not vict then return end
    if not ch:is_npc() or ch:mob_flagged(MF.DUMMY) then return end

    local foe_flying = vict:condition_has("flying")
    local ch_flying  = ch:condition_has("flying")

    -- Altitude matching
    if foe_flying and not ch_flying and ch:is_humanoid() and ch:stat_get("level") > 10 then
        ch:execute_command("fly", commands)
        return
    end
    if not foe_flying and ch_flying then
        ch:execute_command("fly", commands)
        return
    end
    if foe_flying and ch_flying and
       ch:condition_number_get("flying", "altitude") < vict:condition_number_get("flying", "altitude") then
        ch:execute_command("fly high", commands)
        return
    end

    -- Non-flyers facing airborne foe may flee
    if foe_flying and not ch_flying and not ch:is_humanoid() and
       ch:position_get() > POS.RESTING and math.random(1, 30) >= 22 and not ch:block_calc() then
        flee_mob(ch, "$n@G flees in terror and you lose sight of $m!")
        return
    end
    if foe_flying and ch:is_humanoid() and ch:stat_get("level") <= 10 and
       math.random(1, 30) >= 22 and not ch:block_calc() then
        flee_mob(ch, "$n@G turns and runs away. You lose sight of $m!")
        return
    end

    -- Position recovery
    local pos = ch:position_get()
    if (pos == POS.SITTING or pos == POS.RESTING) and ch:sec_roll() == 1 then
        ch:execute_command("stand", commands)
        return
    end
    if ch:aff_flagged(AFF.PARA) and
       ch:stat_get("intelligence") + 10 < math.random(1, 60) then
        act.to_char(ch, "@yYou fail to overcome your paralysis!@n", {actor=ch})
        act.around(ch, "@Y$n @ystruggles with $s paralysis!@n", {actor=ch})
        return
    end
    if pos == POS.SLEEPING and not ch:aff_flagged(AFF.KNOCKED) and ch:sec_roll() == 1 then
        ch:execute_command("wake", commands)
        ch:execute_command("stand", commands)
        return
    end

    -- Can't attack if out of range or incapacitated
    if ch:room_get() ~= vict:room_get() or ch:aff_flagged(AFF.KNOCKED) or
       pos == POS.SITTING or pos == POS.RESTING or pos == POS.SLEEPING then
        return
    end

    if math.random(1, 30) <= 12 then return end

    ch:mob_attack(vict:name_get() or "")
end

-- ── PC combat helpers ─────────────────────────────────────────────────────

local function tick_position_advantage(ch)
    local vict = ch:fighting_get()
    if not vict then return end

    if not ch:condition_has("advantageous_position") then
        if ch:roll_balance() > dbat.axion_dice(0) and math.random(1, 10) >= 7 then
            if not vict:condition_has("advantageous_position") then
                ch:send_line("@YYou manage to move into an advantageous position!@n")
                act.around(ch, "@y$n@Y manages to move into an advantageous position!@n", {actor=ch})
                ch:condition_add("advantageous_position")
            else
                if ch:roll_balance() > vict:roll_balance() then
                    act.to_char(ch,   "@YYou struggle to gain a better position than @y$N@Y and succeed!@n",    {actor=ch, target=vict})
                    act.to_char(vict, "@y$n@Y struggles to gain a better position than you and succeeds!@n",    {actor=ch, target=vict})
                    act.around(ch,    "@y$n@Y struggles to gain a better position than @y$N@Y and succeeds!@n", {actor=ch, target=vict})
                    vict:condition_remove("advantageous_position", "outmaneuvered")
                    ch:condition_add("advantageous_position")
                end
            end
        end
    else
        if ch:roll_balance() < dbat.axion_dice(-30) or ch:position_get() < POS.STANDING then
            ch:send_line("@YYou are moved out of your position!@n")
            act.around(ch, "@y$n@Y is moved out of $s position!@n", {actor=ch})
            ch:condition_remove("advantageous_position", "lost")
        end
    end
end

local function tick_wimp_flee(ch)
    local wimp = ch:wimp_level_get()
    if wimp > 0 and ch:meter_current("powerlevel") < wimp and ch:meter_current("powerlevel") > 0 then
        ch:send_line("You wimp out, and attempt to flee!")
        ch:flee("")
    end
end

local function tick_disguise_slip(ch)
    if not ch:player_flagged(PLR.DISGUISED) then return end
    if not ch:fighting_get() then return end
    if ch:skill_get("disguise") < math.random(1, 125) then
        ch:send_line("Your disguise comes off because of your swift movements!")
        ch:player_flag_set(PLR.DISGUISED, false)
        act.around(ch, "@W$n's@W disguise comes off because of $s swift movements!@n", {actor=ch})
    end
end

-- ── Condition definition ──────────────────────────────────────────────────

return {
    id         = "fighting",
    name       = "Fighting",
    tags       = { "fighting", "in_combat" },
    persistent = false,
    on_apply = function(ch, cond)
        cond:schedule_event("combat_tick", FIGHT_INTERVAL_MS, FIGHT_INTERVAL_MS)
    end,
    on_remove = function(ch, cond, reason)
        cond:cancel_event("combat_tick")
    end,
    on_event = function(ch, cond, event)
        if event ~= "combat_tick" then return end
        tick_position_advantage(ch)
        if not ch:is_npc() then
            tick_wimp_flee(ch)
            tick_disguise_slip(ch)
        else
            npc_combat_ai(ch, cond)
        end
    end,
}
