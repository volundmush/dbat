-- Homing projectile tick handler for kienzan (vnum 84).
-- Like spirit ball but with instant-kill logic: if damage > 1/5 of target max PL,
-- Majin/Bio characters get two regenerate roll attempts before dying outright.
-- State lives on C++ obj fields: user_get(), target_get(), kicharge_get().

local function try_regen(target, user, dmg)
    local race = target:race_get()
    if race ~= "majin" and race ~= "bio" then return false end
    if target:skill_get("regenerate") <= math.random(1, 101) then return false end
    local ki_cost = math.floor(target:meter_max("ki") / 40)
    if target:meter_current("ki") < ki_cost then return false end
    target:send_line("@rYou are cut in half by the attack but regenerate a moment later!@n")
    user:send_line("@R%s@r is cut in half by the attack but regenerates a moment later!@n",
                   target:name_get())
    target:meter_mod_int("ki", -ki_cost)
    target:damage({ powerlevel = dmg }, user)
    return true
end

return {
    id = "ki_kienzan",

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
            user:send_line("@wYou lose sight of @C%s@W and let the kienzan fly away!@n",
                           target:name_get())
            target:send_line("@wYou manage to escape @C%s's@W kienzan!@n", user:name_get())
            obj:extract()
            return
        end

        user:send_line("@RYou move your hand and direct the kienzan after @r%s@R!@n",
                       target:name_get())
        target:send_line("@r%s@R moves $s hand and directs the kienzan after YOU!@n",
                         user:name_get())

        local parry = target:skill_get("parry") or 0
        if parry >= math.random(1, 140) then
            require("lua.libs.ki_effects").ki_terrain_hit(user)
            target:send_line("@WYou deflect the kienzan and it strikes the surroundings!@n")
            user:send_line("@WYour kienzan was deflected!@n")
            obj:extract()
            return
        end

        if dmg > target:meter_max("powerlevel") / 5 then
            if not try_regen(target, user, dmg) and not try_regen(target, user, dmg) then
                target:send_line("@rYou are cut in half by the attack!@n")
                user:send_line("@R%s@r is cut in half by the attack!@n", target:name_get())
                target:die(user)
            end
        else
            target:damage({ powerlevel = dmg }, user)
            target:send_line("@rThe kienzan slams into your body, exploding in a flash of bright light!@n")
        end
        obj:extract()
    end,
}
