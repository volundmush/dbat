local function act() return require("dbat").lib.act end

return {
    id   = "ram",
    name = "Ram",
    skill = "punch",
    family = "melee",
    tier  = 1,
    elements = { blunt = 1.0 },
    damages_limbs        = true,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block  = true,
    can_parry  = false,
    can_dodge  = true,
    base_accuracy = 0.95,
    base_power    = 2.1,
    spar_safe = true,

    on_check = function(inst)
        if not inst.attacker:is_npc() then
            return false, "You cannot do that."
        end
        return true
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        if loc == "head" then
            a.message({
                actor  = "@CYou@W aim your body at @C$N@W and ram into $S face!@n",
                target = "@C$n@W aims $s body at you and rams into YOUR face!@n",
                room   = "@c$n@W aims $s body at @C$N@W and rams into $S face!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@CYou@W aim your body at @C$N@W and ram into $S arm!@n",
                target = "@C$n@W aims $s body at you and rams into YOUR arm!@n",
                room   = "@c$n@W aims $s body at @C$N@W and rams into $S arm!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@CYou@W aim your body at @C$N@W and ram into $S leg!@n",
                target = "@C$n@W aims $s body at you and rams into YOUR leg!@n",
                room   = "@c$n@W aims $s body at @C$N@W and rams into $S leg!@n",
            }, ctx)
        else
            a.message({
                actor  = "@CYou@W aim your body at @C$N@W and ram into $S body!@n",
                target = "@C$n@W aims $s body at you and rams into YOUR body!@n",
                room   = "@c$n@W aims $s body at @C$N@W and rams into $S body!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@CYou@W move to ram @C$N@W, but miss!@n",
            target = "@C$n@W moves to ram you, but misses!@n",
            room   = "@c$n@W moves to ram @C$N@W, but somehow misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W manages to dodge your ram!@n",
            target = "@WYou dodge @C$n's@W attempted ram!@n",
            room   = "@C$N@W manages to dodge @c$n's@W attempted ram!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W moves quickly and blocks your body as you try to ram $M!@n",
            target = "@WYou move quickly and block @C$n's@W body as $e tries to ram YOU!@n",
            room   = "@C$N@W moves quickly and blocks @c$n's@W body as $e tries to ram $M!@n",
        }, ctx)
    end,
}
