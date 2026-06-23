local dbat = require("dbat")
local act  = require("lua.libs.act")
local movement = require("lua.libs.movement")

local SECT = dbat.consts.sector_types
local EX   = dbat.consts.exit_flags

local SYNTAX = table.concat({
    "Syntax 1: hydromancy flood (direction)",
    "Example: hydromancy flood nw",
    "",
    "Syntax 2: hydromancy spike",
}, "\r\n")

local DIR_NAME_TO_IDX = {
    north = 0,  n  = 0,
    east  = 1,  e  = 1,
    south = 2,  s  = 2,
    west  = 3,  w  = 3,
    up    = 4,  u  = 4,
    down  = 5,  d  = 5,
    northwest = 6,  nw = 6,
    northeast = 7,  ne = 7,
    southeast = 8,  se = 8,
    southwest = 9,  sw = 9,
    inside    = 10, ["in"] = 10,
    outside   = 11, out   = 11,
}

local function execute(ctx)
    local ch = ctx.ch

    if ch:race_get() ~= "tsuna" or ch:skill_base_get("style") <= 0 then
        ch:send_line("You know nothing about hydromancy!")
        return
    end

    local skill  = ch:skill_base_get("style")
    local chance = dbat.axion_dice(0)
    local room   = ch:room_get()
    local geffect = room:geffect_get()
    local sector  = room:sector_type_get()

    local cost = math.floor(ch:meter_max("ki") / 12) - ch:stat_get("intelligence") * ch:stat_get("level")

    if geffect >= 0 and sector ~= SECT.WATER_SWIM and sector ~= SECT.WATER_NOSWIM then
        if sector ~= SECT.UNDERWATER then
            ch:send_line("There is not sufficient water here.")
        else
            ch:send_line("There is too much water here to control!")
        end
        return
    end

    if cost <= 0 then cost = 100 end

    if ch:meter_current("ki") < cost then
        ch:send_line("You do not have enough ki to manipulate any water around you.")
        return
    end

    if ch:cooldown_get() > 0 then
        ch:send_line("You must wait a short period before concentrating again.")
        return
    end

    local tokens = ctx.argparams and ctx.argparams.tokens or {}
    local arg  = (tokens[1] or ""):lower()
    local arg2 = (tokens[2] or ""):lower()

    if arg == "" then
        ch:send_line(SYNTAX)
        return
    end

    if arg == "spike" then
        local spike_cost = math.floor(100 + ch:skill_get("style") / (1 + ch:meter_max("ki") * 0.5))

        if ch:meter_current("ki") < spike_cost then
            ch:send_line("You do not have enough ki to form an ice spike.")
            return
        end

        if skill < chance then
            ch:meter_mod_int("ki", -spike_cost)
            act.to_char(ch,
                "@CYou press your palms together in front of your body but you fail to produce the proper control to form the spike!@n",
                {actor = ch})
            act.around(ch,
                "@c$n@C presses $s palms together and then slowly pulls them apart. Nothing important appears to have happened.",
                {actor = ch})
            ch:improve_skill("style", 2)
            return
        end

        local vnum
        if     skill >= 100 then vnum = 19058
        elseif skill >= 50  then vnum = 19057
        else                     vnum = 19056
        end

        local obj = dbat.read_object(vnum)
        ch:meter_mod_int("ki", -spike_cost)
        act.to_char(ch,
            "@CYou press your palms together in front of your body and focusing ki you force water up along your body. That water pools between your palms and as pull your palms apart a @c$p@C forms!@n",
            {actor = ch, item = obj})
        act.around(ch,
            "@c$n@C presses $s palms together in front of $s body and water begins to flow up $s body and pools between $s palms. Slowly pulling them apart reveals a @c$p@C as it forms between them!@n",
            {actor = ch, item = obj})

        if obj:weight_get() + ch:carry_weight_get() <= ch:carry_weight_max() then
            obj:to_char(ch)
        else
            ch:send_line("You are unable to hold it and so let it go at your feet.")
            act.around(ch, "@C$n@w drops an ice spike.@n", {actor = ch})
            obj:to_room(room)
        end

        ch:improve_skill("style", 1)
        ch:cooldown_set(10)

    elseif arg == "flood" then
        if arg2 == "" then
            ch:send_line(SYNTAX)
            return
        end

        local attempt = DIR_NAME_TO_IDX[arg2]
        if attempt == nil then
            ch:send_line("That is not a valid direction.")
            return
        end

        local exit = room:exit_get(attempt)
        if exit and not exit:flagged(EX.CLOSED) and exit:destination() then
            local saved_lastatk = ch:lastatk_get()
            ch:lastatk_set(500)

            if skill < chance then
                act.to_char(ch,
                    "@BUsing your ki you attempt to create a rush of water! @RYou fail!@n",
                    {actor = ch})
                act.around(ch,
                    "@b$n@B seems to attempt to create water with $s ki! @RHowever, $e fails!@n",
                    {actor = ch})
                ch:meter_mod_int("ki", -cost)
                ch:wait_set(20)
            else
                ch:meter_mod_int("ki", -cost)
                act.to_char(ch,
                    string.format("@BUsing your ki you create a rush of water flooding away toward the @C%s@B!@n",
                        movement.DIR_NAMES[attempt + 1]),
                    {actor = ch})
                act.around(ch,
                    string.format("@B$n@B uses $s ki to create a rush of water flooding away toward the @C%s@B!@n",
                        movement.DIR_NAMES[attempt + 1]),
                    {actor = ch})

                for vict in room:people_get() do
                    if vict ~= ch then
                        if not ch:can_kill(vict) then
                            act.to_char(vict, "@CYou are protected from the water!@n", {actor = vict})
                            act.around(vict, "@C$n@C is protected from the water!@n", {actor = vict})
                        elseif vict:race_get() == "kanassan" then
                            act.to_char(vict, "@CYou effortlessly swim against the current.@n", {actor = vict})
                            act.around(vict, "@C$n@C effortlessly swims against the current.@n", {actor = vict})
                        elseif vict:skill_base_get("balance") >= dbat.axion_dice(-10) then
                            act.to_char(vict, "@CYou manage to keep your balance and are not swept away!@n", {actor = vict})
                            act.around(vict, "@C$n@C manages to keep $s balance and is not swept away!@n", {actor = vict})
                        elseif ch:condition_has("flying") then
                            -- Note: intentional faithful port of C++ which checks caster's flight, not victim's
                            act.to_char(vict, "@CYou fly above the rushing waters and are untouched.@n", {actor = vict})
                            act.around(vict, "@C$n@C flies above the rushing waters and is untouched.@n", {actor = vict})
                        else
                            act.to_char(vict, "@cYou are caught by the rushing waters and sent tumbling away!@n", {actor = vict})
                            act.around(vict, "@c$n@c is caught by the rushing waters and sent tumbling away!@n", {actor = vict})
                            vict:try_move(movement.DIR_NAMES[attempt + 1])
                            ch:hurt_target(vict, cost * 4, 1)
                        end
                    end
                end

                local dest = exit:destination()
                dest:geffect_set(-3)
                ch:lastatk_set(saved_lastatk)
                ch:wait_set(20)
                ch:cooldown_set(15)
            end

            ch:lastatk_set(saved_lastatk)
        else
            ch:send_line("You can not flood the water that direction!")
        end
    else
        ch:send_line(SYNTAX)
    end
end

return {
    id      = "hydromancy",
    aliases = { { "hydromancy", 9 } },
    execute = execute,
}
