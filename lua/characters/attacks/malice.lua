local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    [1] = {
        actor  = "@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S chest, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's chest!@n",
        target = "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your chest, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your chest!@n",
        room   = "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slams an open palm into @c$N's@W chest, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's chest!@n",
    },
    [2] = {
        actor  = "@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S head, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's head!@n",
        target = "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your head, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your head!@n",
        room   = "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slams an open palm into @c$N's@W head, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's head!@n",
    },
    [4] = {
        actor  = "@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S gut, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's gut!@n",
        target = "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your gut, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your gut!@n",
        room   = "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slams an open palm into @c$N's@W gut, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's gut!@n",
    },
    [5] = {
        actor  = "@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S arm, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's arm!@n",
        target = "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your arm, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your arm!@n",
        room   = "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slams an open palm into @c$N's@W arm, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's arm!@n",
    },
    [6] = {
        actor  = "@WYou rush forward at @c$N@W, building ki into your arm. As you slam an open palm into $S leg, you send the charged energy into $S body. A few small explosions seem to hit across $S entire body, forcing $M to stumble back. Finally, you launch $M into the air, pointing a forefinger at $m like a pistol, and shout '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of your first strike on @c$N@W's leg!@n",
        target = "@C$n @Wrushes forward at you, building ki into $s arm. As $e slams an open palm into your leg, $e sends the charged energy into your body. A few small explosions seem to hit across your entire body, forcing you to stumble back. Finally, @C$n@W launches you into the air, pointing a forefinger at your body like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on your leg!@n",
        room   = "@C$n@W rushes forward at $N@W, building ki into $s arm. As $e slams an open palm into @c$N's@W leg, $e sends the charged energy into $S body. A few small explosions seem to hit across @c$N's@W entire body, forcing $M to stumble back. Finally, @C$n@W launches $M into the air, pointing a forefinger at $m like a pistol, and shouts '@MM@ma@Dl@wi@Wce Br@we@Dak@me@Mr@W!' as a dark, violet explosion erupts at the epicenter of $s first strike on @c$N@W's leg!@n",
    },
}

return {
    id     = "malice",
    family = "ki",
    name   = "Malice Breaker",
    skill  = "malice breaker",
    tier   = 3,
    elements = { ki = 1.0 },
    consumes_charge = true,
    limbs_required = { "arm" },
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = true,
    can_parry = false,
    can_dodge = true,
    spar_safe = true,
    base_accuracy = 1.0,
    base_power    = 3.0,

    on_check = function(inst)
        if inst.attacker:condition_has("mystic_melody") then
            return false, "Your mystic melody prevents you from using malice breaker."
        end
        return require("lua.libs.ki").can_grav(inst.attacker)
    end,

    on_modify_accuracy = function(inst)
        local hours = require("dbat").time().hours
        if hours <= 15 or hours > 22 then
            inst.accuracy_modifier = inst.accuracy_modifier + 5
        end
    end,

    on_calculate_damage = function(inst)
        local atk   = require("lua.libs.attack")
        local base  = atk._base_damage(inst.attacker, inst.def, inst)
        local hours = require("dbat").time().hours
        if hours <= 15 then
            return math.floor(base * 1.25)
        elseif hours <= 22 then
            return math.floor(base * 1.4)
        end
        return base
    end,

    on_hit = function(inst)
        local roll = math.random(1, 6)
        -- C++ uses random 1-6 directly as hit location (1=body, 2/3=head, 4=body, 5=arm, 6=leg)
        -- Map: 1→body, 2→head, 3→head, 4→body, 5→arm, 6→leg
        local loc_map = { [1]="body", [2]="head", [3]="head", [4]="body", [5]="arm", [6]="leg" }
        local loc = loc_map[roll] or "body"
        local msg_key = (roll == 3) and 2 or roll  -- cases 2 and 3 share the same head msg
        local msgs = HIT_MSGS[msg_key] or HIT_MSGS[1]
        act().message(msgs, { actor = inst.attacker, target = inst.target })
        -- Override the hit_location for stat tracking
        inst.hit_location = loc
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W manages to dodge your Malice Breaker, letting it slam into the surroundings!@n",
            target = "@WYou dodge @C$n's@W Malice Breaker, letting it slam into the surroundings!@n",
            room   = "@C$N@W manages to dodge @c$n's@W Malice Breaker, letting it slam into the surroundings!@n",
        }, { actor = inst.attacker, target = inst.target })
        local room = inst.attacker:room_get()
        if room then
            room:send_all("@wA bright explosion erupts from the impact!\r\n")
            if room:damage_get() <= 80 then room:damage_mod(20) end
        end
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou can't believe it but your Malice Breaker misses, flying through the air harmlessly!@n",
            target = "@C$n@W fires a Malice Breaker at you, but misses!@n",
            room   = "@c$n@W fires a Malice Breaker at @C$N@W, but somehow misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W moves quickly and blocks your Malice Breaker!@n",
            target = "@WYou move quickly and block @C$n's@W Malice Breaker!@n",
            room   = "@C$N@W moves quickly and blocks @c$n's@W Malice Breaker!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
