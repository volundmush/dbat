-- Spirit Bomb countdown and AoE detonation.
-- Counts down kidist ticks then hits all non-allies in room with 1.25x multiplier.
-- Script vars: damage (i64), target_id (i64), user_id (i64), kidist (i64)
return {
    id = "ki_genkidama",

    on_event = function(obj, script, event_name)
        if event_name ~= "tick" then return end
        local db   = require("dbat")
        local room = obj:room_get()
        if not room then obj:extract(); return end

        local kidist = script:number_get("kidist")
        local dmg    = script:number_get("damage")
        local user   = db.characters.by_id(script:number_get("user_id"))
        if kidist > 1 then
            script:number_set("kidist", kidist - 1)
            room:send_text(("@BA massive @bSpirit Bomb@B descends! @W(%d seconds remain)@n\r\n"):format(kidist - 1))
            if user then user:send_line("@BYour Spirit Bomb closes in!@n") end
            return
        end

        -- Detonation: AoE with genki multiplier 1.25x, allies are spared
        room:send_text("@BThe @bSpirit Bomb@B DETONATES in a blinding flash of pure energy!@n\r\n")
        local zone = db.zones.by_id(room:zone_vnum_get())
        if zone then zone:send_text("@BA massive explosion of light illuminates the horizon!@n\r\n") end

        local function is_ally(person)
            if not user then return false end
            if person:is_same(user) then return true end
            if not person:has_group() then return false end
            local leader    = person:following_get()
            local my_leader = user:following_get()
            return (leader    and leader:is_same(user))      or
                   (my_leader and my_leader:is_same(person)) or
                   (leader and my_leader and leader:is_same(my_leader))
        end

        local actual = math.floor(dmg * 1.25)
        for person in room:people() do
            if not is_ally(person) then
                person:damage({ powerlevel = actual }, user)
                person:send_line("@BYou are engulfed by the Spirit Bomb's explosion!@n")
            end
        end

        obj:extract()
    end,
}
