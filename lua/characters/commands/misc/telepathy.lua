local dbat = require("dbat")

local Search = dbat.lib.search.new
local text   = dbat.lib.text

local AFF = dbat.consts.aff_flags
local PLR = dbat.consts.player_flags

local SYNTAX = "Syntax: telepathy [ read ] (target)\n        telepathy [ link ] (target)\n        telepathy [  far ] (target)\n        telepathy (target) (message)"

local function in_arena(ch)
    local rv = ch:room_vnum_get()
    return rv >= 17800 and rv <= 17874
end

local function trim(s)
    return (s or ""):match("^%s*(.-)%s*$")
end

local function split_first(raw)
    raw = trim(raw)
    local first, rest = raw:match("^(%S+)%s*(.*)$")
    return first or "", rest or ""
end

local function visible_room_target(ch, name)
    if name == "" then return nil end
    return Search(ch):add_room_people(ch:room_get()):add_filter(function(s, e)
        return s:can_see_char(e)
    end):find_one(name)
end

local function visible_world_target(ch, name)
    if name == "" then return nil end
    return Search(ch):add_global_characters():add_filter(function(s, e)
        return s:can_see_char(e)
    end):find_one(name)
end

local function ki_cost(ch)
    return math.floor(ch:meter_max("ki") / 40)
end

local function spend_ki(ch)
    ch:meter_mod_int("ki", -ki_cost(ch))
end

local function comm_name(ch)
    if ch:admin_level_get() > 0 then return ch:name_get() end
    return ch:user_get() or ch:name_get()
end

local function race_name(ch)
    local def = dbat.get("races", ch:race_get())
    return def and def.name or ch:race_get() or "Unknown"
end

local function sensei_name(ch)
    local def = dbat.get("senseis", ch:sensei_get())
    return def and def.name or ch:sensei_get() or "Unknown"
end

local function alignment_name(align)
    if align >= 1000 then return "Saint         " end
    if align > 750 then return "Extremely Good" end
    if align > 500 then return "Really Good   " end
    if align > 250 then return "Good          " end
    if align > 100 then return "Pretty Good   " end
    if align > 50 then return "Sorta Good    " end
    if align > -50 then return "Neutral       " end
    if align > -100 then return "Sorta Evil    " end
    if align > -500 then return "Pretty Evil   " end
    if align >= -750 then return "Evil          " end
    if align < -750 then return "Extremely Evil" end
    if align <= -1000 then return "Devil         " end
    return "Unknown       "
end

local function reject_self_or_android(ch, vict)
    if vict:is_same(ch) then
        ch:send_line("Oh that makes a lot of sense...")
        return true
    end
    if vict:race_get() == "android" then
        ch:send_line("You can't touch the mind of such an artificial being.")
        return true
    end
    return false
end

local function far(ch, name)
    if ch:is_npc() then return end

    local vict = visible_world_target(ch, name)
    if not vict then
        ch:send_line("Look through who's eyes?")
        return
    end
    if vict:is_same(ch) then
        ch:send_line("Oh that makes a lot of sense...")
        return
    end
    if vict:is_npc() then
        ch:send_line("You can't touch the mind of such a thing.")
        return
    end
    if vict:admin_level_get() > ch:admin_level_get() then
        ch:send_line("Their mental power oustrips your's by unfathomable measurements!")
        return
    end
    if ch:condition_has("shocked") then
        ch:send_line("Your mind has been shocked by telepathic feedback! You are not able to use telepathy right now.")
        return
    end
    if vict:race_get() == "android" then
        ch:send_line("You can't touch the mind of such an artificial being.")
        return
    end
    if vict:skill_get("telepathy") + vict:stat_get("intelligence") > ch:skill_get("telepathy") + ch:stat_get("intelligence") then
        ch:send_line("They throw off your attempt with their own telepathic abilities!")
        return
    end
    local ch_room = ch:room_get()
    local vict_room = vict:room_get()
    if ch_room and vict_room and ch_room:is_same(vict_room) then
        ch:send_line("They are in the same room as you!")
        return
    end
    if vict:aff_flagged(AFF.BLIND) then
        ch:send_line("They are blind!")
        return
    end
    if vict:player_flagged(PLR.EYEC) then
        ch:send_line("Their eyes are closed!")
        return
    end

    if vict_room then ch:look_at_specific_room(vict_room) end
    ch:send_line("You see all this through their eyes!")
    if vict:stat_get("intelligence") > ch:stat_get("intelligence") then
        ch:send_line("You feel like someone was using your mind for something...")
    end
    spend_ki(ch)
end

local function link(ch, name)
    if ch:is_npc() then return end

    local linked = ch:mindlinked_get()
    if linked then
        ch:act("@CYou remove the link your mind had with @w$N.@n", true, nil, linked, "char")
        ch:act("@w$n@C removes the link $s mind had with yours.@n", true, nil, linked, "vict")
        ch:mindlinked_set(nil)
        linked:mindlinked_set(nil)
        ch:linker_set(0)
        return
    end

    local vict = visible_world_target(ch, name)
    if not vict then
        ch:send_line("Link with the mind of who?")
        return
    end
    if vict:is_same(ch) then
        ch:send_line("Oh that makes a lot of sense...")
        return
    end
    if vict:is_npc() then
        ch:send_line("You can't touch the mind of such a thing.")
        return
    end
    if vict:race_get() == "android" then
        ch:send_line("You can't touch the mind of such an artificial being.")
        return
    end
    if vict:skill_get("telepathy") > 0 then
        ch:send_line("Kinda pointless when you are both telepathic huh?")
        return
    end
    if vict:mindlinked_get() then
        ch:send_line("Someone else is already telepathically linked with them.")
        return
    end
    if ch:skill_get("telepathy") < dbat.axion_dice(math.floor(vict:stat_get("intelligence") * 0.1)) then
        ch:act("@R$n@r tried to link $s mind with yours, but you manage to force a break in the link!@n", false, nil, vict, "vict")
        ch:act("@R$N@r manages to sense the intrusion and with $S intelligence push you out!@n", false, nil, vict, "char")
        return
    end

    ch:act("@CYou link your mind with @w$N.@n", true, nil, vict, "char")
    ch:act("@w$n@C links $s mind with yours. You can speak your thoughts to $m with 'think'.@n", true, nil, vict, "vict")
    vict:send_line("@wIf this is undesirable, Try: meditate break@n")
    vict:mindlinked_set(ch)
    ch:mindlinked_set(vict)
    ch:linker_set(1)
end

local function read(ch, name)
    local vict = visible_room_target(ch, name)
    if not vict then
        ch:send_line("Read the mind of who?")
        return
    end
    if reject_self_or_android(ch, vict) then return end

    if dbat.axion_dice(0) > ch:skill_get("telepathy") then
        spend_ki(ch)
        ch:act("@wYou attempt to read $N's@w mind, but fail to see it clearly.@n", true, nil, vict, "char")
        if math.random(1, 15) >= 14 and not ch:condition_has("shocked") then
            ch:act("@MYour mind has been shocked!@n", true, nil, nil, "char")
            ch:condition_apply("shocked", "combat", "telepathy_fail")
        else
            ch:improve_skill("telepathy", 0)
        end
        return
    end

    if vict:skill_get("telepathy") >= ch:skill_get("telepathy") and math.random(1, 2) == 2 then
        spend_ki(ch)
        ch:act("@wYou fail to read @c$N's@w mind and they seemed to have noticed the attempt!@n", true, nil, vict, "char")
        ch:act("@C$n@w attempts to read your mind, but you resist and force $m out!@n", true, nil, vict, "vict")
        ch:improve_skill("telepathy", 0)
        return
    end

    ch:send_line("@wYou peer into their mind:")
    spend_ki(ch)
    ch:send_line("@GName      @D: @W%s@n", vict:name_get())
    ch:send_line("@GRace      @D: @W%s@n", race_name(vict))
    ch:send_line("@GSensei    @D: @W%s@n", sensei_name(vict))
    ch:send_line("@GStr       @D: @W%d@n", vict:stat_get("strength"))
    ch:send_line("@GCon       @D: @W%d@n", vict:stat_get("constitution"))
    ch:send_line("@GInt       @D: @W%d@n", vict:stat_get("intelligence"))
    ch:send_line("@GWis       @D: @W%d@n", vict:stat_get("wisdom"))
    ch:send_line("@GSpd       @D: @W%d@n", vict:stat_get("speed"))
    ch:send_line("@GAgi       @D: @W%d@n", vict:stat_get("agility"))
    ch:send_line("@GZenni     @D: @W%s@n", text.add_commas(vict:stat_get("money")))
    ch:send_line("@GBank Zenni@D: @W%s@n", text.add_commas(vict:stat_get("money_bank")))
    ch:send_line("@GAlignment @D: @w%s@n", alignment_name(vict:stat_get("alignment")))
    ch:improve_skill("telepathy", 0)
end

local function send_thought(ch, raw, arg, arg2)
    local vict = ch:mindlinked_get()
    local message = raw
    if not vict then
        vict = visible_world_target(ch, arg)
        if not vict then
            ch:send_line("Send your thoughts to who?")
            return
        end
        message = arg2
    end

    if reject_self_or_android(ch, vict) then return end

    ch:send_line("@WYou tell @c%s@W telepathically, @w'@C%s@w'@n", vict:name_get(), message)
    vict:send_line("@c%s@W talks to you telepathically, @w'@C%s@w'@n", ch:name_get(), message)
    dbat.send_to_imm(string.format("@GTELEPATHY: @C%s@G telepaths @c%s, @W'@w%s@W'@n",
        comm_name(ch), comm_name(vict), message))
    spend_ki(ch)
end

local function execute(ctx)
    local ch = ctx.ch
    local raw = ctx.argparams.raw or ""
    local arg, arg2 = split_first(raw)
    local lower_arg = arg:lower()

    if not ch:know_skill("telepathy") then return end

    if in_arena(ch) then
        ch:send_line("Lol, no.")
        return
    end

    if arg == "" then
        ch:send_line(SYNTAX)
        return
    end

    if ch:meter_current("ki") < ki_cost(ch) then
        ch:send_line("You do not have enough ki to focus your mental abilities.")
        return
    end

    if lower_arg == "far" then
        far(ch, arg2)
    elseif lower_arg == "link" then
        link(ch, arg2)
    elseif lower_arg == "read" then
        read(ch, arg2)
    else
        send_thought(ch, trim(raw), arg, arg2)
    end
end

return {
    id = "telepathy",
    aliases = { {"telepathy", 6} },
    execute = execute,
}
