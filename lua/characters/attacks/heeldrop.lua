local function act() return require("dbat").lib.act end

return {
    id   = "heeldrop",
    name = "Heeldrop",
    skill = "heeldrop",
    family = "melee",
    tier  = 4,
    elements = { blunt = 1.0 },
    limbs_required = { "legs" },
    damages_limbs = true,
    can_trigger_multihit = true,
    in_multihit          = true,
    can_block  = false,
    can_parry  = false,
    can_dodge  = false,
    consumes_charge = false,
    base_accuracy = 1.0,
    base_power    = 1.0,
    spar_safe = true,

    on_calculate_cost = function(inst)
        inst.cost.stamina = math.floor(inst.attacker:meter_max("powerlevel") / 90)
    end,

    on_check_combo = function(ch)
        if ch:limbcond_get(3) <= 0 and ch:limbcond_get(4) <= 0 then return false end
        return ch:skill_known("heeldrop")
    end,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        if loc == "head" then
            a.message({
                actor  = "@WYou leap and bring your heel crashing down on @C$N's@W skull!@n",
                target = "@C$n@W leaps and brings $s heel crashing down on your skull!@n",
                room   = "@c$n@W leaps and brings $s heel crashing down on @C$N's@W skull!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@WYou drop your heel down onto @C$N's@W arm!@n",
                target = "@C$n@W drops $s heel down onto your arm!@n",
                room   = "@c$n@W drops $s heel down onto @C$N's@W arm!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@WYou drive your heel down into @C$N's@W knee!@n",
                target = "@C$n@W drives $s heel down into your knee!@n",
                room   = "@c$n@W drives $s heel down into @C$N's@W knee!@n",
            }, ctx)
        else
            a.message({
                actor  = "@WYou leap and drive your heel straight down onto @C$N!@n",
                target = "@C$n@W leaps and drives $s heel straight down onto you!@n",
                room   = "@c$n@W leaps and drives $s heel straight down onto @C$N!@n",
            }, ctx)
        end
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou drop your heel but @C$N@W rolls out of the way!@n",
            target = "@C$n@W drops $s heel but you roll out of the way!@n",
            room   = "@c$n@W drops $s heel at @C$N@W but $E rolls away!@n",
        }, ctx)
    end,
}
