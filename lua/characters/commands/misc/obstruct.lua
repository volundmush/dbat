local dbat = require("dbat")
local act  = dbat.lib.act
local ST   = dbat.consts.sector_types
local RF   = dbat.consts.room_flags

local DIRS_LUA = dbat.consts.direction_names  -- 1-indexed: DIRS_LUA[dir+1] = "north" etc.

-- direction string → { from_dir, to_dir (opposite) }
-- from_dir: the exit blocked from THIS room; to_dir: the exit blocked from the DEST room
local DIR_MAP = {
    n  = { 0,  2 }, e  = { 1,  3 }, s  = { 2,  0 }, w  = { 3,  1 },
    u  = { 4,  5 }, d  = { 5,  4 }, i  = { 10, 11 }, o  = { 11, 10 },
    nw = { 6,  8 }, ne = { 7,  9 }, se = { 8,  6 }, sw = { 9,  7 },
}

local WALL_VNUM = 79

local function clamp_strength(strength, max_pl)
    if     strength > max_pl * 20 then return max_pl + math.floor(strength / 20)
    elseif strength > max_pl * 15 then return max_pl + math.floor(strength / 15)
    elseif strength > max_pl * 10 then return max_pl + math.floor(strength / 10)
    elseif strength > max_pl * 5  then return max_pl + math.floor(strength / 5)
    elseif strength > max_pl * 2  then return max_pl + math.floor(strength / 2)
    else return strength
    end
end

local function execute(ctx)
    local ch  = ctx.ch
    local arg = (ctx.argparams.tokens[1] or ""):lower()

    if ch:is_npc() then return end
    if not ch:know_skill("hyoga kabe") then return end

    local room = ch:room_get()
    local sect = room:sector_type_get()

    if room:flagged(RF.PEACEFUL) then
        ch:send_line("You can not use this in such a peaceful area.")
        return
    end
    if sect == ST.SPACE or room:flagged(RF.SPACE) then
        ch:send_line("You can not wall off the vastness of space.")
        return
    end
    if sect == ST.FLYING then
        ch:send_line("You can not create gravity defying glacial walls.")
        return
    end

    local skill = ch:skill_get("hyoga kabe")
    local prob  = dbat.axion_dice(0)
    local cost  = math.floor((ch:meter_max_get("ki") / skill) * 2.5)

    if arg == "" then
        ch:send_line("What direction are you wanting to block off?\n[ N | E | S | W | NE | NW | SE | SW | U | D | I | O ]")
        return
    end

    if ch:meter_get("ki") < cost then
        ch:send_line("You do not have enough ki to perform the technique.")
        return
    end

    local dirs = DIR_MAP[arg]
    if not dirs then
        ch:send_line("That is not an acceptable direction.\n[ N | E | S | W | NE | NW | SE | SW | U | D | I | O ]")
        return
    end

    local dir, dir2 = dirs[1], dirs[2]

    local exit = room:exit_get(dir)
    if not exit then
        ch:send_line("That direction does not exist here.")
        return
    end

    if skill < prob then
        act.to_char(ch, "@CYou channel your ki and start to create a wall of water, but lose your concentration and the water promptly disappears.@n")
        act.around(ch, "@c$n@C channels $s ki and starts to create a wall of water, but loses $s concentration and the water promptly disappears.@n", {})
        ch:meter_mod_int("ki", -cost)
        ch:improve_skill("hyoga kabe", 0)
        return
    end

    local dest = exit:destination()
    if not dest then
        ch:send_line("That direction does not exist here.")
        return
    end

    if dest:flagged(RF.PEACEFUL) then
        ch:send_line("You can not block off a peaceful area.")
        return
    end

    -- Check for an existing wall in the destination room (skill >= prob, so removal always succeeds)
    for obj in dest:contents() do
        if obj:vnum_get() == WALL_VNUM and obj:cost_get() == dir2 then
            act.to_char(ch, "@CYou place your hands on the glacial wall and concentrate. You unfreeze the wall and evaporate the water effortlessly.@n")
            act.around(ch, "@c$n@C places $s hands on the glacial wall and concentrates. Suddenly the wall melts and then evaporates!@n", {})
            ch:meter_mod_int("ki", -math.floor(cost / 2))
            obj:extract()
            return
        end
    end

    -- Create paired walls: one in dest room blocking dir2, one in current room blocking dir
    local proto = dbat.obj_protos.by_id(WALL_VNUM)
    if not proto then return end
    local wall_dest = proto:spawn()
    local wall_here = proto:spawn()
    wall_dest:to_room(dest)
    wall_here:to_room(room)

    local strength = math.floor((ch:stat_get("intelligence") * skill * ch:stat_get("wisdom")) * 20
                                + ch:meter_max_get("ki") * 0.001)
    strength = clamp_strength(strength, ch:meter_max_get("powerlevel"))

    wall_dest:cost_set(dir2)
    wall_dest:weight_set(strength)
    wall_here:cost_set(dir)
    wall_here:weight_set(strength)
    wall_dest:fellow_wall_set(wall_here)
    wall_here:fellow_wall_set(wall_dest)

    act.to_char(ch, "@CYou concentrate and channel your ki. A wall of water starts to form in such a way to block off the direction of your choice. As the wall becomes complete it freezes solid by your will!@n")
    act.around(ch, "@c$n@C concentrates and channels $s ki. A wall of water starts to form in such a way to block off one of the directions of this area. As the wall becomes complete it freezes solid by @c$n's@C will!@n", {})
    dest:send_line(string.format("@cA wall of water forms slowly upward blocking off the %s direction. This wall of water then freezes instantly once it stops growing.@n", DIRS_LUA[dir2 + 1] or "?"))

    ch:improve_skill("hyoga kabe", 0)
    ch:meter_mod_int("ki", -cost)
end

return {
    id      = "obstruct",
    aliases = { {"hyoga", 5} },
    execute = execute,
}
