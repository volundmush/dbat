local dbat = require("dbat")
local act  = require("lua.libs.act")

local AFF = dbat.consts.aff_flags
local MF  = dbat.consts.mob_flags
local POS = dbat.consts.positions

local INTERVAL_MS = 3000

local function should_help(ch, vict)
    local opponent = vict:fighting_get()
    if vict:is_same(ch) or not vict:is_npc() or not opponent then return false end
    if opponent:is_npc() or opponent:is_same(ch) then return false end
    if not vict:is_humanoid() then return false end
    return true
end

return {
    id         = "mob_helper",
    persistent = false,

    on_apply = function(ch, script)
        script:schedule_event("help", INTERVAL_MS, INTERVAL_MS)
    end,

    on_remove = function(ch, script, reason)
        script:cancel_event("help")
    end,

    on_event = function(ch, script, event)
        if event ~= "help" then return end
        if ch:position_get() <= POS.SLEEPING then return end
        if not ch:mob_flagged(MF.HELPER) then return end
        if ch:aff_flagged(AFF.BLIND) or ch:aff_flagged(AFF.CHARM) then return end

        local room = ch:room_get()
        if not room then return end

        for vict in room:people() do
            if should_help(ch, vict) then
                local opponent = vict:fighting_get()
                act.around(ch, "$n jumps to the aid of $N!", { actor = ch, target = vict })
                ch:launch_attack("punch", opponent)
                return
            end
        end
    end,
}
