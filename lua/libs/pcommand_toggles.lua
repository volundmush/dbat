local dbat = require("dbat")

local M = {}

local PRF = dbat.consts.prf_flags
local AFF = dbat.consts.aff_flags
local ADMLVL = dbat.consts.adm_levels

local function can_player(ch)
    if ch:is_npc() then return false end
    return true
end

local function can_admin(level)
    return function(ch)
        if ch:is_npc() then return false end
        return ch:admin_level_get() >= level
    end
end

local function toggle_pref(ch, flag)
    return ch:pref_flag_toggle(flag)
end

local function toggle_aff(ch, flag)
    local value = not ch:aff_flagged(flag)
    ch:aff_flag_set(flag, value)
    return value
end

function M.pref(spec)
    return {
        id = spec.id,
        aliases = spec.aliases,
        can_execute = spec.admin_level and can_admin(spec.admin_level) or can_player,
        execute = function(ctx)
            local on = toggle_pref(ctx.ch, spec.flag)
            ctx.ch:send(on and spec.on or spec.off)
        end,
    }
end

function M.aff(spec)
    return {
        id = spec.id,
        aliases = spec.aliases,
        can_execute = can_player,
        execute = function(ctx)
            local on = toggle_aff(ctx.ch, spec.flag)
            ctx.ch:send(on and spec.on or spec.off)
        end,
    }
end

function M.admin_can(level)
    return can_admin(level)
end

M.PRF = PRF
M.AFF = AFF
M.ADMLVL = ADMLVL

return M
