local dbat  = require("dbat")
local PLR   = dbat.consts.player_flags
local POS   = dbat.consts.positions
local ITYPE = dbat.consts.item_types
local IWEAR = dbat.consts.item_wear_flags

local function pickup_chair(ch, chair)
    if chair:wear_flagged(IWEAR.TAKE)
       and chair:type_get() ~= ITYPE.CHAIR
       and ch:der_total("weight_carried") + chair:weight_get() <= ch:der_total("weight_carry_capacity")
    then
        chair:from_room()
        chair:to_char(ch)
        ch:send_line("You pick up " .. chair:short_description_get() .. ".")
        ch:act_around("$n picks up " .. chair:short_description_get() .. ".")
    end
    chair:sitting_set(nil)
    ch:sits_set(nil)
end

local function execute(ctx)
    local ch = ctx.ch

    if ch:condition_has("knocked_out") then
        ch:send_line("You are knocked out cold for right now!")
        return
    end
    if not ch:is_npc() and ch:limbcond_get(3) <= 0 and ch:limbcond_get(4) <= 0 then
        ch:send_line("With what legs will you be standing up on?")
        return
    end
    if ch:player_flagged(PLR.PILOTING) then
        ch:send_line("You are busy piloting a ship!")
        return
    end

    local pos = ch:position_get()
    if pos == POS.STANDING then
        ch:send_line("You are already standing.")
    elseif pos == POS.SITTING then
        ch:reveal_hiding(0)
        ch:send_line("You stand up.")
        ch:act_around("$n clambers to $s feet.")
        local chair = ch:sits_get()
        if chair then pickup_chair(ch, chair) end
        ch:position_set(ch:condition_has("fighting") and POS.FIGHTING or POS.STANDING)
    elseif pos == POS.RESTING then
        ch:send_line("You stop resting, and stand up.")
        ch:act_around("$n stops resting, and clambers to $s feet.")
        local chair = ch:sits_get()
        if chair then pickup_chair(ch, chair) end
        ch:position_set(POS.STANDING)
    elseif pos == POS.SLEEPING then
        ch:send_line("You have to wake up first!")
    else
        ch:send_line("You stop floating around, and put your feet on the ground.")
        ch:act_around("$n stops floating around, and puts $s feet on the ground.")
        ch:position_set(POS.STANDING)
    end
end

return { id = "stand", aliases = { { "stand", 2 } }, execute = execute }
