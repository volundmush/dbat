local dbat   = require("dbat")
local act    = dbat.lib.act

local function execute(ctx)
    local ch   = ctx.ch
    local arg  = ctx.argparams.tokens[1] or ""
    local skill = ch:skill_get("healglow")

    if skill == 0 then
        ch:send_line("You do not know how to perform that technique.")
        return
    end

    local vict
    if arg == "" then
        vict = ch
    elseif skill < 100 then
        ch:send_line("You can not target anyone except yourself unless you are a master of this technique.\nSyntax: healingglow")
        return
    else
        vict = ch:acquire_room_target(arg)
        if not vict then
            ch:send_line("Nobody around by that name.")
            return
        end
    end

    if vict:condition_has("healing_glow") then
        if vict:id_get() == ch:id_get() then
            ch:send_line("You already have a healing glow surrounding your body.")
        else
            ch:send_line("They already have a healing glow surrounding their body.")
        end
        return
    end

    if vict:condition_has("fighting") then
        if vict:id_get() == ch:id_get() then
            ch:send_line("You are too busy fighting!")
        else
            ch:send_line("They are too busy fighting!")
        end
        return
    end

    local cost = math.floor(ch:meter_max("ki") * 0.5)
    if ch:meter_current("ki") < cost then
        ch:send_line(string.format("You do not have enough ki. It requires at least 50%% of your ki in cost."))
        return
    end

    local duration = math.max(1, math.floor(skill * 0.1))
    local actx = { actor = ch, target = vict }
    if vict:id_get() == ch:id_get() then
        ch:send_line("@CPlacing your hands on your body you begin to focus your energies. Slowly a strong blue glow glistens and shines across your skin!@n")
        ch:act_around("@c$n@C places $s hands on $s body. Slowly a strong blue glow glistens and shines across $s skin!@n")
    else
        duration = duration + math.random(-2, 1)
        if duration < 1 then duration = 1 end
        act.to_char(ch, "@CPlacing your hands on @c$N's@C body you begin to focus your energies. Slowly a strong blue glow glistens and shines across $S skin!@n", actx)
        act.to_target("@c$n@C places $s hands on YOUR body. Slowly a strong blue glow glistens and shines across your skin!@n", actx)
        act.to_room("@c$n@C places $s hands on @c$N's@C body. Slowly a strong blue glow glistens and shines across $S skin!@n", { actor = ch, target = vict, exclude = { ch, vict } })
    end

    vict:condition_apply_with_duration("healing_glow", "skill", "healing glow", duration * dbat.consts.secs_per_mud_hour)
    ch:meter_mod("ki", -cost)
end

return { id = "healglow", aliases = { { "healingglow", 7 }, { "healglow", 7 } }, execute = execute }
