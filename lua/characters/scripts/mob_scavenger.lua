local dbat = require("dbat")
local act  = require("lua.libs.act")

local EF    = dbat.consts.item_extra_flags
local IWEAR = dbat.consts.item_wear_flags
local ITYPE = dbat.consts.item_types
local MF    = dbat.consts.mob_flags
local POS   = dbat.consts.positions

local INTERVAL_MS = 3000

local MESSAGES = {
    "$n@W says, '@CFinders keepers, losers weepers.@W'@n",
    "$n@W says, '@CPeople always leaving their garbage JUST LYING AROUND. The nerve....@W'@n",
    "$n@W says, '@CWho would leave this here? Oh well..@W'@n",
    "$n@W says, '@CI always wanted one of these.@W'@n",
    "$n@W looks around quickly to see if anyone is paying attention.@n",
}

local function player_present(room)
    if not room then return false end
    for person in room:people() do
        if not person:is_npc() then return true end
    end
    return false
end

local function can_get(ch, obj)
    return obj:wear_flagged(IWEAR.TAKE)
        and obj:sitting_get() == 0
        and obj:weight_get() + ch:carry_weight_get() <= ch:carry_weight_max()
        and ch:can_see_obj(obj)
end

local function valid_prize(ch, obj)
    return can_get(ch, obj)
        and obj:type_get() ~= ITYPE.BED
        and not obj:is_posted()
        and not obj:extra_flagged(EF.NOPICKUP)
end

return {
    id         = "mob_scavenger",
    persistent = false,

    on_apply = function(ch, script)
        script:schedule_event("scavenge", INTERVAL_MS, INTERVAL_MS)
    end,

    on_remove = function(ch, script, reason)
        script:cancel_event("scavenge")
    end,

    on_event = function(ch, script, event)
        if event ~= "scavenge" then return end
        if ch:position_get() <= POS.SLEEPING then return end
        if ch:fighting_get() then return end
        if not ch:is_humanoid() then return end
        if ch:mob_flagged(MF.NOSCAVENGER) or ch:mob_flagged(MF.NOKILL) then return end

        local room = ch:room_get()
        if not room then return end
        if player_present(room) and dbat.axion_dice(0) <= 118 then return end
        if math.random(1, 100) < 95 then return end

        local best_obj
        local best_cost = 1
        for obj in room:contents() do
            local cost = obj:cost_get()
            if cost > best_cost and valid_prize(ch, obj) then
                best_obj = obj
                best_cost = cost
            end
        end

        if not best_obj then return end

        act.around(ch, MESSAGES[math.random(1, #MESSAGES)], { actor = ch })
        ch:perform_get_from_room(best_obj)
    end,
}
