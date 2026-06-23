local dbat   = require("dbat")
local act    = require("lua.libs.act")
local search = dbat.lib.search

local PLR = dbat.consts.player_flags
local AFF = dbat.consts.aff_flags

return {
    id = "follow",
    aliases = {
        {"follow", "follo", 5},
    },
    execute = function(ctx)
        local ch  = ctx.ch
        local arg = ctx.argparams and ctx.argparams.tokens and ctx.argparams.tokens[1] or ""

        if ch:plr_flagged(PLR.HEALT) then
            ch:send_line("You are inside a healing tank!")
            return
        end

        if arg == "" then
            ch:send_line("Whom do you wish to follow?")
            return
        end

        local leader = search.new(ch):add_room_people(ch:room_get()):find_one(arg)
        if not leader then
            ch:send_line("%s", dbat.consts.noperson)
            return
        end

        if ch:master_get() == leader then
            act.to_char(ch, "You are already following $M.", {actor=ch, target=leader})
            return
        end

        if ch:aff_flagged(AFF.CHARM) and ch:master_get() then
            local master = ch:master_get()
            act.to_char(ch, "But you only feel like following $N!", {actor=ch, target=master})
            return
        end

        if leader == ch then
            if not ch:master_get() then
                ch:send_line("You are already following yourself.")
                return
            end
            ch:stop_follower()
            return
        end

        if ch:circle_follow(leader) then
            ch:send_line("Sorry, but following in loops is not allowed.")
            return
        end

        if ch:master_get() then
            ch:stop_follower()
        end
        ch:condition_remove("group", "leave_group")
        ch:reveal_hiding(0)
        ch:add_follower(leader)
    end,
}
