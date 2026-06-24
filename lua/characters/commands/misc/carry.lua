local dbat   = require("dbat")
local act    = require("lua.libs.act")
local Search = dbat.lib.search.new

local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions

return {
    id = "carry",
    aliases = {
        {"carry", 5},
    },
    execute = function(ctx)
        local ch = ctx.ch
        if ch:is_npc() then return end

        if ch:dragging_get() then
            ch:send_line("You are busy dragging someone at the moment.")
            return
        end

        if ch:player_flagged(PLR.PILOTING) then
            ch:send_line("You are busy piloting a ship!")
            return
        end

        if ch:carrying_char_get() then
            ch:carry_drop(ch:stat_get("alignment") > 50 and 0 or 1)
            return
        end

        local arg = ""
        if ctx.argparams and ctx.argparams.tokens then
            arg = table.concat(ctx.argparams.tokens, " ")
        end
        if arg == "" then
            ch:send_line("You want to carry who?")
            return
        end

        local room = ch:room_get()
        local vict = Search(ch):add_room_people(room):find_one(arg)
        if not vict then
            ch:send_line("That person isn't here.")
            return
        end

        if vict:is_npc() then
            ch:send_line("There's no point in carrying them.")
            return
        end

        if vict:carried_by_char_get() then
            ch:send_line("Someone is already carrying them!")
            return
        end

        if vict:position_get() > POS.SLEEPING then
            ch:send_line("They are not unconcious.")
            return
        end

        if vict:der_total("weight") + vict:carry_weight_get() > ch:carry_weight_max() then
            act.to_char(ch, "@WYou try to pick up @C$N@W but have to put them down. They are too heavy for you at the moment.@n", {actor=ch, target=vict})
            act.around(ch, "@C$n@W tries to pick up @c$N@W. After struggling for a moment $e has to put $M down.@n", {actor=ch, target=vict})
            ch:wait_set(6)
            return
        end

        act.to_char(ch, "@WYou pick up @C$N@W and put $M over your shoulder.@n", {actor=ch, target=vict})
        act.around(ch, "@C$n@W picks up @c$N@W and puts $M over $s shoulder.@n", {actor=ch, target=vict})
        local chair = vict:sits_get()
        if chair then
            chair:sitting_set(nil)
            vict:sits_set(nil)
        end
        ch:carrying_char_set(vict)
        vict:carried_by_char_set(ch)
        ch:wait_set(6)
    end,
}
