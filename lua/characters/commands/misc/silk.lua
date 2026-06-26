local dbat = require("dbat")
local act  = dbat.lib.act

local PULSE_3SEC = 30
local PULSE_4SEC = 40

local APPLY_AC  = 17
local APPLY_STR =  1
local APPLY_INT =  3

-- Valid silk bundles usable as weave input
local VALID_BUNDLES = {
    [16700] = true,
    [16701] = true,
    [16702] = true,
    [16703] = true,
    [16704] = true,
    [16708] = true,
}

-- Bundle quality → stat bonuses for the woven output
local BUNDLE_STATS = {
    [16708] = { armor = 500 * 20, str = 4, intel = 4, price = 4, olevel = 80 },
    [16700] = { armor = 500 * 15, str = 3, intel = 3, price = 4, olevel = 75 },
    [16701] = { armor = 500 * 10, str = 3, intel = 3, price = 3, olevel = 50 },
    [16702] = { armor = 500 *  6, str = 2, intel = 2, price = 2, olevel = 25 },
    [16703] = { armor = 500 *  4, str = 1, intel = 1, price = 1.5, olevel = 5 },
}

local WEAVE_OUTPUT = { head = 16705, wrist = 16706, belt = 16707 }
local WEAVE_LABELS = { head = "headsash", wrist = "wristband", belt = "belt" }

local function find_silk_bundle(ch)
    for obj in ch:inventory() do
        if VALID_BUNDLES[obj:vnum_get()] and not obj:extra_flagged(dbat.consts.extra_flags.FORGED) then
            return obj
        end
    end
end

local function apply_weave_stats(weaved, bundle_vnum)
    local stats = BUNDLE_STATS[bundle_vnum] or { armor = 500, str = 0, intel = 0, price = 1, olevel = 0 }
    weaved:affect_set(0, APPLY_AC, 0, stats.armor)
    weaved:cost_set(math.floor(weaved:cost_get() * stats.price))
    weaved:value_set(0, stats.olevel)
    weaved:level_set(stats.olevel)
    if stats.str > 0 then weaved:affect_set(1, APPLY_STR, 0, stats.str) end
    if stats.intel > 0 then weaved:affect_set(2, APPLY_INT, 0, stats.intel) end
end

local function do_weave(ch, arg2)
    if arg2 == "" then
        ch:send_line("Syntax: silk weave (head | wrist | belt)")
        return
    end

    local output_vnum = WEAVE_OUTPUT[arg2]
    if not output_vnum then
        ch:send_line("Syntax: silk weave (head | wrist | belt)")
        return
    end

    local silk = find_silk_bundle(ch)
    if not silk then
        ch:send_line("You do not have an acceptable bundle of silk in your inventory!")
        return
    end

    local prob = ch:skill_get("silk")
    local perc = math.random(1, 120)

    if prob <= perc then
        act.to_char(ch, "@WYou attempt to weave $p@W into the desired piece but end up ruining the entire bundle instead.@n", { object = silk })
        act.around(ch, "@C$n@W attempts to weave $p@W into some type of clothing but ends up ruining the entire bundle instead.@n", { object = silk })
        silk:extract()
        ch:wait_set(PULSE_4SEC)
        return
    end

    local weaved_proto = dbat.obj_protos.by_id(output_vnum)
    if not weaved_proto then return end
    local weaved = weaved_proto:spawn()
    weaved:to_room(ch:room_get())
    apply_weave_stats(weaved, silk:vnum_get())

    act.to_char(ch, "@WYou attempt to weave the bundle and manage to create $p@W!@n", { object = weaved })
    act.around(ch, "@C$n@W attempts to weave a bundle into something and manages to create $p@W!@n", { object = weaved })

    weaved:from_room()
    weaved:to_char(ch)
    silk:extract()
    ch:wait_set(PULSE_4SEC)
end

local function do_bundle(ch)
    local prob = ch:skill_get("silk")
    local perc = math.random(1, 120)
    local cost  = math.floor((ch:meter_max_get("ki") * 0.01) * (prob * 0.20)) +
                  ch:stat_get("intelligence") * ch:stat_get("level")

    if ch:meter_get("ki") < cost then
        ch:send_line("You do not have enough ki to weave any bundles of silk.")
        return
    end

    ch:wait_set(PULSE_3SEC)

    local superoll = math.random(1, 100)
    local super = false
    if ch:race_get() == "kurzak" then
        if   prob >= 100 and 8 > superoll then super = true
        elseif prob >= 60 and 6 > superoll then super = true
        elseif prob >= 40 and 3 > superoll then super = true
        end
    end

    local spawn_vnum
    local success_msg_char = "@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@W piled at your feet!@n"
    local success_msg_room = "@C$n@W seems to concentrate for a moment before spitting out a golden colored silk from $s mouth. Gently $e weaves the silk and in no time at all $e has a $p@W piled at $s feet!@n"

    if super then
        spawn_vnum = 16708
    elseif prob > perc and prob >= 100 then
        spawn_vnum = 16700
    elseif prob > perc and prob >= 90 then
        spawn_vnum = 16701
    elseif prob > perc and prob >= 80 then
        spawn_vnum = 16702
    elseif prob > perc and prob >= 50 then
        spawn_vnum = 16703
    elseif prob > perc then
        spawn_vnum = 16704
    end

    if spawn_vnum then
        local proto = dbat.obj_protos.by_id(spawn_vnum)
        if not proto then return end
        local obj = proto:spawn()
        obj:to_room(ch:room_get())
        if super then
            act.to_char(ch, "@YYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You gently weave the silk and in no time at all you have a $p@Y piled at your feet!@n", { object = obj })
            ch:send_line("@YIt's SUPER grand!@n")
            act.around(ch, success_msg_room, { object = obj })
        else
            act.to_char(ch, success_msg_char, { object = obj })
            act.around(ch, success_msg_room, { object = obj })
        end
        ch:meter_mod_int("ki", -cost)
    else
        act.to_char(ch, "@WYou concentrate your ki into your silk sacs and begin to spit silk out of your mouth. You end up making a poorly formed puddle of goo...@n")
        act.around(ch, "@C$n@W seems to concentrate for a moment before spitting out a poorly formed puddle of goo...@n", {})
        ch:meter_mod_int("ki", -cost)
        ch:improve_skill("silk", 1)
    end
end

local function execute(ctx)
    local ch   = ctx.ch
    local arg  = ctx.argparams.tokens[1] or ""
    local arg2 = (ctx.argparams.tokens[2] or ""):lower()

    if not ch:know_skill("silk") then return end

    if arg == "" then
        ch:send_line("Syntax: silk (weave | bundle)")
        return
    end

    if arg:lower() == "weave" then
        do_weave(ch, arg2)
    elseif arg:lower() == "bundle" then
        do_bundle(ch)
    else
        ch:send_line("Syntax: silk (weave | bundle)")
    end
end

return {
    id      = "silk",
    aliases = { {"silk", 4} },
    execute = execute,
}
