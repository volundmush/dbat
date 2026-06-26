local dbat = require("dbat")
local act  = dbat.lib.act
local ST   = dbat.consts.sector_types

local function is_sunken(room)
    return room:geffect_get() < 0 or room:sector_type_get() == ST.UNDERWATER
end

local function execute(ctx)
    local ch = ctx.ch

    if ch:condition_has("fireshield") then
        ch:send_line("You are already covered in a fireshield!")
        return
    end
    if ch:condition_has("sanctuary") then
        ch:send_line("You are covered in a barrier!")
        return
    end
    if is_sunken(ch:room_get()) then
        ch:send_line("There is way too much water here!")
        return
    end

    local cost = math.floor(ch:meter_max_get("ki") * 0.03)
    if ch:meter_get("ki") < cost then
        ch:send_line("You do not have enough ki!")
        return
    end

    local skill = ch:skill_get("fireshield")
    local prob  = dbat.axion_dice(0)

    if skill <= prob then
        act.to_char(ch, "@WYou hold your hands up in front of you on either side and try to summon defensive @rf@Rl@Ya@rm@Re@Ys@W to cover your body. Yet you screw up and the technique fails!@n")
        act.around(ch, "@c$n@W holds $s hands up in front of $m on either side and tries to summon defensive @rf@Rl@Ya@rm@Re@Ys@W to cover $s body. Yet $e seems to screw up and the technique fails!@n", {})
    else
        act.to_char(ch, "@WYou hold your hands up in front of you on either side and try to summon defensive @rf@Rl@Ya@rm@Re@ys@W to cover your body. The ki you have gathered pours out of your body and creates intense black @rf@Rl@Ya@rm@Re@Ys@W that cover your entire body in a protective layer!")
        act.around(ch, "@c$n@W holds $s hands up in front of $m on either side and tries to summon defensive @rf@Rl@Ya@rm@Re@ys@W to cover $s body. The ki $e has gathered pours out of $s body and creates intense black @rf@Rl@Ya@rm@Re@Ys@W that cover $s entire body in a protective layer!", {})
        ch:condition_apply("fireshield", "skill", "fireshield")
    end

    ch:improve_skill("fireshield", 0)
    ch:meter_mod_int("ki", -cost)
end

local function can_execute(ch)
    return ch:know_skill("fireshield")
end

return {
    id          = "fireshield",
    aliases     = { {"fireshield", 5} },
    execute     = execute,
    can_execute = can_execute,
}
