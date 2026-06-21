-- Homing projectile tick handler for tsuihidan.
-- Moves to target's room each second; parry-checks and detonates on arrival.
-- Script vars: damage (i64), target_id (i64), user_id (i64)
return {
    id = "ki_tsuihidan",

    on_event = function(obj, script, event_name)
        if event_name ~= "tick" then return end
        local db     = require("dbat")
        local target = db.characters.by_id(script:number_get("target_id"))
        local user   = db.characters.by_id(script:number_get("user_id"))
        local dmg    = script:number_get("damage")

        if not target or not user then
            obj:extract()
            return
        end

        local obj_room    = obj:room_get()
        local target_room = target:room_get()

        if not obj_room or not target_room then
            obj:extract()
            return
        end

        if not target_room:is_same(obj_room) then
            obj:from_room()
            obj:to_room(target_room)
            obj_room:send_text("@WA homing blast streaks away in pursuit!@n\r\n")
            target_room:send_text("@WA homing blast closes in on you!@n\r\n")
            return
        end

        -- Same room: parry check (mirrors C++ homing_hit_tsuihidan)
        local parry = target:skill_get("parry") or 0
        if parry >= math.random(1, 140) then
            require("lua.libs.ki_effects").ki_terrain_hit(user)
            target:send_line("@WYou parry the homing blast and it strikes the surroundings!@n")
            user:send_line("@WYour tsuihidan was deflected at the last moment!@n")
        else
            target:meter_mod_int("lifeforce", -dmg)
            target:send_line("@RThe homing blast finally catches you and SLAMS into you!@n")
            user:send_line("@WYour tsuihidan finally catches @C%s@W and detonates!@n", target:name_get())
            target_room:send_text("@WA homing blast detonates!@n\r\n")
        end
        obj:extract()
    end,
}
