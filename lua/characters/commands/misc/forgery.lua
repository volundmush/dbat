local dbat   = require("dbat")
local Search = dbat.lib.search.new

local EF = dbat.consts.item_extra_flags
local POS = dbat.consts.positions

local FORGERY_KIT_VNUM = 19
local PULSE_2SEC = 20

local function blocked_by_position(ch)
    local pos = ch:position_get()
    if pos >= POS.RESTING or pos == POS.FIGHTING then return false end

    if pos == POS.DEAD then
        ch:send_line("Lie still; you are DEAD!!! :-(")
    elseif pos == POS.INCAP or pos == POS.MORTALLYW then
        ch:send_line("You are in a pretty bad shape, unable to do anything!")
    elseif pos == POS.STUNNED then
        ch:send_line("All you can do right now is think about the stars!")
    elseif pos == POS.SLEEPING then
        ch:send_line("In your dreams, or what?")
    end

    return true
end

local function object_name(obj)
    return obj:short_description_get() or "something"
end

local function find_kit(ch)
    for obj in ch:inventory() do
        if obj:vnum_get() == FORGERY_KIT_VNUM
            and not obj:extra_flagged(EF.BROKEN)
            and not obj:extra_flagged(EF.FORGED) then
            return obj
        end
    end
    return nil
end

local function forbidden_vnum(vnum)
    return vnum >= 60000
        or vnum == 0
        or (vnum >= 18800 and vnum <= 18999)
        or (vnum >= 19080 and vnum <= 19199)
        or (vnum >= 4 and vnum <= 6)
end

local function execute(ctx)
    local ch = ctx.ch
    local arg = ctx.argparams.tokens[1] or ""

    if blocked_by_position(ch) then return end
    if ch:is_npc() then return end
    if not ch:know_skill("forgery") then return end

    if arg == "" then
        ch:send_line("Okay, make a forgery of what?")
        return
    end

    local original = Search(ch):add_character_inventory(ch):find_one(arg)
    if not original then
        ch:send_line("You want to make a fake copy of what?")
        return
    end

    local kit = find_kit(ch)
    if not kit then
        ch:send_line("You need a forgery kit.")
        return
    end

    local vnum = original:vnum_get()
    local name = object_name(original)

    if vnum == FORGERY_KIT_VNUM then
        ch:send_line("You can't duplicate a forgery kit.")
        return
    end

    if original:extra_flagged(EF.FORGED) then
        ch:send_line("%s is forgery, there is no reason to make a fake of a fake!", name)
        ch:wait_set(PULSE_2SEC)
        return
    end

    if original:extra_flagged(EF.BROKEN) then
        ch:send_line("%s is broken, there is no reason to make a fake of this mess!", name)
        ch:wait_set(PULSE_2SEC)
        return
    end

    if forbidden_vnum(vnum) then
        if vnum >= 60000 or vnum == 0 then
            ch:send("You can not make a forgery of that! It's far too squishy....")
        else
            ch:send_line("You can not make a forgery of that!")
        end
        return
    end

    if original:extra_flagged(EF.PROTECTED) then
        ch:send_line("You don't know where to begin with this work of ART.")
        return
    end

    ch:reveal_hiding(0)
    ch:act("@c$n@w looks at $p, begins to work on forging a fake copy of it.@n", true, original, nil, "room")
    ch:improve_skill("forgery", 1)

    if ch:skill_get("forgery") < dbat.axion_dice(0) then
        if math.random(1, 10) >= 9 then
            ch:send_line("In the middle of creating a forgery of %s you screw up. The fabrication unit built into the forgery kit melts and bonds with the original. You clumsy mistake with the Estex Titanium drill has broken both.", name)
            kit:extract()
            original:extract()
            return
        end

        ch:send_line("You start to make a forgery of %s but screw up and waste your forgery kit..", name)
        ch:act("@c$n@w tried to duplicate $p but screws up somehow.@n", true, original, nil, "room")
        kit:extract()
        ch:wait_set(PULSE_2SEC)
        return
    end

    local proto = dbat.obj_protos.by_id(vnum)
    local copy = proto and proto:spawn()
    if not copy then return end

    copy:extra_flag_set(EF.FORGED, true)
    copy:weight_set(math.random(math.floor(copy:weight_get() / 2), copy:weight_get()))
    copy:to_char(ch)
    kit:extract()

    ch:send_line("You make an excellent forgery of %s@n!", name)
    ch:act("@c$n@w makes a perfect forgery of $p.@n", true, original, nil, "room")
    ch:wait_set(PULSE_2SEC)
end

return {
    id      = "forgery",
    aliases = { {"forgery", 4} },
    execute = execute,
}
