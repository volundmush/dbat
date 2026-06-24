local dbat     = require("dbat")
local movement = require("lua.libs.movement")
local combat   = require("lua.libs.combat")

local AFF = dbat.consts.aff_flags
local MF  = dbat.consts.mob_flags
local RF  = dbat.consts.room_flags
local EX  = dbat.consts.exit_flags
local POS = dbat.consts.positions

local INTERVAL_MS = 3000
local DIR_COUNT   = 12

return {
    id         = "mob_wander",
    persistent = false,

    on_apply = function(ch, script)
        script:schedule_event("wander", INTERVAL_MS, INTERVAL_MS)
    end,

    on_remove = function(ch, script, reason)
        script:cancel_event("wander")
    end,

    on_event = function(ch, script, event)
        if event ~= "wander" then return end
        if ch:position_get() ~= POS.STANDING    then return end
        if ch:fighting_get()                     then return end
        if ch:aff_flagged(AFF.TAMED)             then return end
        if ch:aff_flagged(AFF.PARALYZE)          then return end
        if ch:absorbing_get() or ch:absorbed_by_get() then return end
        if math.random(1, 3) ~= 3               then return end

        local room = ch:room_get()
        if not room then return end

        local stay_zone = ch:mob_flagged(MF.STAY_ZONE)
        local my_zone   = stay_zone and room:zone_vnum_get() or nil

        local valid = {}
        for dir = 0, DIR_COUNT - 1 do
            local exit = room:exit_get(dir)
            if exit and not exit:flagged(EX.CLOSED) then
                local dest = exit:destination()
                if dest
                    and not dest:flagged(RF.NOMOB)
                    and not dest:flagged(RF.DEATH)
                    and (not stay_zone or dest:zone_vnum_get() == my_zone)
                then
                    valid[#valid + 1] = dir
                end
            end
        end

        if #valid == 0         then return end
        if not combat.block_calc(ch) then return end

        movement.perform_move(ch, valid[math.random(1, #valid)])
    end,
}
