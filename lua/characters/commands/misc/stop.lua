local dbat = require("dbat")
local P    = dbat.consts.pulses

local function execute(ctx)
    local ch = ctx.ch

    if ch:is_npc() then return end

    if not ch:is_fighting() then
        ch:send_line("You are not even fighting!")
        return
    end

    ch:act("@CYou move out of your fighting posture.@n", true, nil, nil, "char")
    ch:act("@c$n@C moves out of $s fighting posture.@n", true, nil, nil, "room")
    ch:stop_fighting()
    ch:wait_set(P.two_sec)
end

return {
    id      = "stop",
    aliases = { {"stop", 3} },
    execute = execute,
}
