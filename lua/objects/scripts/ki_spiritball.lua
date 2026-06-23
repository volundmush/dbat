-- Homing projectile tick handler for spirit ball (vnum 81).
-- Stays in room; extracts itself if target leaves. Parry-checks on arrival.
-- State lives on C++ obj fields: user_get(), target_get(), kicharge_get().
return {
    id = "ki_spiritball",

    on_event = function(obj, script, event_name)
        if event_name ~= "tick" then return end
        local target = obj:target_get()
        local user   = obj:user_get()
        local dmg    = obj:kicharge_get()

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
            user:send_line("@wYou lose sight of @C%s@W and let the spirit ball fly away!@n",
                           target:name_get())
            target:send_line("@wYou manage to escape @C%s's@W spirit ball!@n", user:name_get())
            obj:extract()
            return
        end

        user:send_line("@RYou move your hand and direct the spirit ball after @r%s@R!@n",
                       target:name_get())
        target:send_line("@r%s@R moves $s hand and directs the spirit ball after YOU!@n",
                         user:name_get())

        local parry = target:skill_get("parry") or 0
        if parry >= math.random(1, 140) then
            require("lua.libs.ki_effects").ki_terrain_hit(user)
            target:send_line("@WYou deflect the spirit ball and it strikes the surroundings!@n")
            user:send_line("@WYour spirit ball was deflected!@n")
        else
            target:damage({ powerlevel = dmg }, user)
            target:send_line("@rThe spirit ball slams into your body, exploding in a flash of bright light!@n")
        end
        obj:extract()
    end,
}
