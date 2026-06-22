local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

local HIT_MSGS = {
    body = {
        actor  = "@WYou place your index and middle fingers against your forehead and pool your charged ki there. Sparks and light surround your fingertips as the technique becomes ready. You point your fingers at @c$N@W and yell '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from your fingers and slams into $S body!@n",
        target = "@c$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @c$n@W points $s fingers at YOU and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into YOUR body!@n",
        room   = "@c$n@W places $s index and middle fingers against $s forehead and pools $s charged ki there. Sparks and light surround the fingertips as the technique becomes ready. @c$n@W points $s fingers at @c$N@W and yells '@YS@yp@De@Yc@yi@Da@Yl @yB@De@Ya@ym @DC@Ya@yn@Dn@Yo@yn@W!' A spiraling beam of energy fires from the fingers and slams into @c$N@W's body!@n",
    },
    head = {
        actor  = "@WYou place your index and middle fingers against your forehead and pool your charged ki there. You point your fingers at @c$N@W and yell '@YSpecial Beam Cannon@W!' A spiraling beam slams into $S head!@n",
        target = "@c$n@W places $s index and middle fingers against $s forehead and yells '@YSpecial Beam Cannon@W!' A spiraling beam slams into YOUR head!@n",
        room   = "@c$n@W places $s index and middle fingers against $s forehead and yells '@YSpecial Beam Cannon@W!' A spiraling beam slams into @c$N@W's head!@n",
    },
    arm = {
        actor  = "@WYou point your fingers at @c$N@W and yell '@YSpecial Beam Cannon@W!' A spiraling beam slams into $S arm!@n",
        target = "@c$n@W points $s fingers at YOU and yells '@YSpecial Beam Cannon@W!' A spiraling beam slams into YOUR arm!@n",
        room   = "@c$n@W points $s fingers at @c$N@W and yells '@YSpecial Beam Cannon@W!' A spiraling beam slams into @c$N@W's arm!@n",
    },
    leg = {
        actor  = "@WYou point your fingers at @c$N@W and yell '@YSpecial Beam Cannon@W!' A spiraling beam slams into $S leg!@n",
        target = "@c$n@W points $s fingers at YOU and yells '@YSpecial Beam Cannon@W!' A spiraling beam slams into YOUR leg!@n",
        room   = "@c$n@W points $s fingers at @c$N@W and yells '@YSpecial Beam Cannon@W!' A spiraling beam slams into @c$N@W's leg!@n",
    },
}

return {
    id     = "sbc",
    family = "ki",
    name   = "Special Beam Cannon",
    skill  = "special beam cannon",
    tier   = 2,
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
    base_power    = 1.5,

    on_check = function(inst)
        return ki.can_grav(inst.attacker)
    end,

    on_hit = function(inst)
        act().message(HIT_MSGS[inst.hit_location] or HIT_MSGS.body,
            { actor = inst.attacker, target = inst.target })
    end,

    on_miss = function(inst)
        act().message({
            actor  = "@WYou can't believe it but your special beam cannon misses, flying through the air harmlessly!@n",
            target = "@C$n@W fires a special beam cannon at you, but misses!@n",
            room   = "@c$n@W fires a special beam cannon at @C$N@W, but somehow misses!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,

    on_dodged = function(inst)
        act().message({
            actor  = "@C$N@W manages to dodge your special beam cannon, letting it slam into the surroundings!@n",
            target = "@WYou dodge @C$n's@W special beam cannon, letting it slam into the surroundings!@n",
            room   = "@C$N@W manages to dodge @c$n's@W special beam cannon, letting it slam into the surroundings!@n",
        }, { actor = inst.attacker, target = inst.target })
        local room = inst.attacker:room_get()
        if room then
            room:send_all("@wA bright explosion erupts from the impact!\r\n")
            if room:damage_get() <= 90 then room:damage_mod(10) end
        end
    end,

    on_blocked = function(inst)
        act().message({
            actor  = "@C$N@W blocks your special beam cannon!@n",
            target = "@WYou block @C$n's@W special beam cannon!@n",
            room   = "@C$N@W blocks @c$n's@W special beam cannon!@n",
        }, { actor = inst.attacker, target = inst.target })
    end,
}
