local dbat     = require("dbat")
local act      = require("lua.libs.act")
local commands = require("lua.characters.commands")

local AFF = dbat.consts.aff_flags
local MF  = dbat.consts.mob_flags
local POS = dbat.consts.positions
local PRF = dbat.consts.prf_flags

local INTERVAL_MS = 3000

local function hidden_from_attention(vict)
    return vict:aff_flagged(AFF.HIDE) or vict:aff_flagged(AFF.SNEAK)
end

local function should_skip(ch, vict, spot_roll)
    if vict:is_same(ch) or ch:fighting_get() then return true end
    if not ch:can_see_char(vict) or vict:is_npc() then return true end
    if vict:pref_flagged(PRF.NOHASSLE) then return true end
    if ch:mob_flagged(MF.AGGR_EVIL) and vict:stat_get("alignment") < 50 then return true end
    if ch:mob_flagged(MF.AGGR_GOOD) and vict:stat_get("alignment") > -50 then return true end
    if vict:stat_get("level") < 5 then return true end
    if vict:aff_flagged(AFF.HIDE) and vict:skill_get("hide") > spot_roll then return true end
    if vict:aff_flagged(AFF.SNEAK) and vict:skill_get("move_silently") > spot_roll then return true end
    return false
end

local function announce_humanoid(ch, vict)
    if hidden_from_attention(vict) then
        act.message({
            target = "@C$n@w notices YOU.\n@w'I am going to get you!' @C$n@w shouts at you!@n",
            room   = "@C$n@w notices @c$N@w.\n@w'I am going to get you!' @C$n@w shouts at @c$N@w!@n",
        }, { actor = ch, target = vict })
    else
        act.message({
            target = "@w'I am going to get you!' @C$n@w shouts at you!@n",
            room   = "@w'I am going to get you!' @C$n@w shouts at @c$N@w!@n",
        }, { actor = ch, target = vict })
    end
end

local function announce_beast(ch, vict)
    if hidden_from_attention(vict) then
        act.message({
            target = "@C$n@w notices YOU.\n@C$n @wgrowls viciously at you!@n",
            room   = "@C$n@w notices @c$N@w.\n@C$n @wgrowls viciously at @c$N@w!@n",
        }, { actor = ch, target = vict })
    else
        act.message({
            target = "@C$n @wgrowls viciously at you!@n",
            room   = "@C$n @wgrowls viciously at @c$N@w!@n",
        }, { actor = ch, target = vict })
    end
end

local function adjust_flying(ch, vict)
    local vict_flying = vict:condition_has("flying")
    local ch_flying = ch:condition_has("flying")

    if vict_flying and not ch_flying and ch:is_humanoid() and ch:stat_get("level") > 10 then
        ch:execute_command("fly", commands)
        return true
    end

    if not vict_flying and ch_flying then
        ch:execute_command("fly", commands)
        return true
    end

    return false
end

return {
    id         = "mob_aggressive",
    persistent = false,

    on_apply = function(ch, script)
        script:schedule_event("aggress", INTERVAL_MS, INTERVAL_MS)
    end,

    on_remove = function(ch, script, reason)
        script:cancel_event("aggress")
    end,

    on_event = function(ch, script, event)
        if event ~= "aggress" then return end
        if ch:position_get() <= POS.SLEEPING then return end
        if not ch:mob_flagged(MF.AGGRESSIVE) then return end
        if ch:aff_flagged(AFF.PARALYZE) then return end

        local room = ch:room_get()
        if not room then return end

        local spot_roll = math.random(1, ch:stat_get("level") + 10)
        local found = false

        for vict in room:people() do
            if not found and not should_skip(ch, vict, spot_roll) then
                local timer = script:number_get("aggtimer")
                if timer < 8 then
                    script:number_set("aggtimer", timer + 1)
                else
                    script:number_set("aggtimer", 0)
                    if ch:is_humanoid() then
                        announce_humanoid(ch, vict)
                        if adjust_flying(ch, vict) then return end
                        ch:launch_attack("punch", vict)
                    else
                        if adjust_flying(ch, vict) then return end
                        announce_beast(ch, vict)
                        ch:launch_attack("bite", vict)
                    end
                    found = true
                end
            end
        end
    end,
}
