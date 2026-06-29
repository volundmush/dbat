local dbat   = require("dbat")
local Search = dbat.lib.search.new

local ADM = dbat.consts.admin_flags
local MOB = dbat.consts.mob_flags
local POS = dbat.consts.positions
local RF  = dbat.consts.room_flags

local PULSE_2SEC = 20

local function blocked_by_position(ch)
    local pos = ch:position_get()
    if pos >= POS.STANDING or pos == POS.FIGHTING then return false end

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
    end

    return true
end

local function execute(ctx)
    local ch = ctx.ch
    local obj_name = ctx.argparams.tokens[1] or ""
    local vict_name = ctx.argparams.tokens[2] or ""
    local room = ch:room_get()

    if blocked_by_position(ch) then return end

    if room and room:flagged(RF.PEACEFUL) then
        ch:send_line("This room just has such a peaceful, easy feeling...")
        return
    end

    local vict = nil
    if vict_name ~= "" and room then
        vict = Search(ch):add_room_people(room):find_one(vict_name)
    end
    if not vict then
        ch:send_line("Plant what on who?")
        return
    elseif vict:is_same(ch) then
        ch:send_line("Come on now, that's rather stupid!")
        return
    end

    if vict:is_npc() and vict:mob_flagged(MOB.NOKILL) and ch:admin_level_get() == 0 then
        ch:send_line("That isn't such a good idea...")
        return
    end

    local roll = ch:roll_skill("sleight_of_hand") + math.random(1, 3)
    local fail = math.random(1, 105)
    local detect = 0
    if vict:position_get() >= POS.SLEEPING then
        detect = vict:roll_skill("spot") + math.random(1, 3)
    end

    if (vict:admin_flagged(ADM.NOSTEAL) or vict:is_shopkeeper()) and ch:admin_level_get() < 5 then
        roll = -10
    end

    local obj = Search(ch):add_character_inventory(ch):find_one(obj_name)
    if not obj then
        ch:send_line("You don't have that to plant on them.")
        return
    end

    if roll <= detect and roll <= fail then
        ch:reveal_hiding(0)
        ch:act("@C$n@w tries to plant $p@w on you!@n", true, obj, vict, "vict")
        ch:act("@C$n@w tries to plant $p@w on @c$N@w!@n", true, obj, vict, "notvict")
        ch:act("@wYou try and fail to plant $p@w on @c$N@w, and $E notices!@n", true, obj, vict, "char")
        ch:wait_set(PULSE_2SEC)
        return
    elseif roll <= fail then
        ch:act("@wYou try and fail to plant $p@w on @c$N@w! However no one seemed to notice.@n", true, obj, vict, "char")
        ch:wait_set(PULSE_2SEC)
        return
    elseif obj:weight_get() + vict:carry_weight_get() > vict:carry_weight_max() then
        ch:reveal_hiding(0)
        ch:act("@C$n@w tries to plant $p@w on you!@n", true, obj, vict, "vict")
        ch:act("@C$n@w tries to plant $p@w on @c$N@w!@n", true, obj, vict, "notvict")
        ch:act("@wYou try and fail to plant $p@w on @c$N@w because $E can't carry the weight. It seems $E noticed the attempt!@n", true, obj, vict, "char")
        ch:wait_set(PULSE_2SEC)
        return
    elseif roll <= detect then
        ch:act("@cYou feel like the weight of your inventory has changed.@n", true, obj, vict, "vict")
        ch:act("@c$N@w looks around after feeling $S pockets.@n", true, obj, vict, "notvict")
        ch:act("@wYou plant $p@w on @c$N@w! @c$N @wseems to notice the change in weight in their inventory.@n", true, obj, vict, "char")
        obj:to_char(vict)
        ch:wait_set(PULSE_2SEC)
        return
    end

    ch:act("@wYou plant $p@w on @c$N@w! No one noticed, whew....@n", true, obj, vict, "char")
    obj:to_char(vict)
    ch:wait_set(PULSE_2SEC)
end

return {
    id      = "plant",
    aliases = { {"plant", 4} },
    execute = execute,
}
