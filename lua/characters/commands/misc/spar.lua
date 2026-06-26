local dbat = require("dbat")
local PLR  = dbat.consts.plr_flags

local function execute(ctx)
    local ch = ctx.ch
    if ch:player_flagged(PLR.SPAR) then
        ch:send_line("@wYou cease your sparring stance.@n")
        ch:act_around("@C$n@w ceases $s sparring stance.@n", {hide_invisible = false})
    else
        ch:send_line("@wYou move into your sparring stance.@n")
        ch:act_around("@C$n@w moves into $s sparring stance.@n", {hide_invisible = false})
    end
    ch:player_flag_set(PLR.SPAR, not ch:player_flagged(PLR.SPAR))
end

local function can_execute(ctx)
    if ctx.ch:is_npc() then return false end
    return true
end

return {
    id        = "spar",
    aliases   = { {"spar", 3} },
    execute   = execute,
    can_execute = can_execute,
}
