local function act() return require("dbat").lib.act end

local function apply_burn(attacker, target, spar)
    if spar then return end
    if target:condition_has("burned") then return end
    if target:condition_has("fireproof") then
        attacker:send_line("@RThey appear to be fireproof!@n")
        return
    end
    local chance = target:condition_has("fireprone") and 1 or math.random(1, 4)
    if chance <= 1 then
        if target:condition_has("fireprone") then
            target:send_line("@RYou are extremely flammable and are burned by the attack!@n")
            attacker:send_line("@RThey are easily burned!@n")
        else
            target:send_line("@RYou are burned by the attack!@n")
            attacker:send_line("@RThey are burned by the attack!@n")
        end
        target:condition_apply("burned")
    end
end

local BREATH_HIT_LONG = "@C$n@W aims $s mouth at $N and opens it wide slowly. A high pitched sound can be heard as the mouth opens, and as the throat is exposed a bright white flame can be seen burning there. Suddenly @C$n@W breathes a jet of @rf@Ri@Ye@Rr@ry@W flames onto"

return {
    id   = "breath",
    name = "Breath",
    skill = "punch",
    family = "melee",
    tier  = 1,
    elements = { fire = 1.0 },
    damages_limbs        = true,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block  = true,
    can_parry  = false,
    can_dodge  = true,
    base_accuracy = 1.15,
    base_power    = 3.0,
    spar_safe = false,

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
                actor  = "@CYou@W aim your mouth at @C$N@W and open it wide slowly. The bright white flame within roars as you breathe a jet of @rf@Ri@Ye@Rr@ry@W flames onto $S face!@n",
                target = BREATH_HIT_LONG .. " YOUR face!@n",
                room   = BREATH_HIT_LONG .. " @C$N's@W face!@n",
            }, ctx)
        elseif loc == "arm" then
            a.message({
                actor  = "@CYou@W aim your mouth at @C$N@W and open it wide slowly. The bright white flame within roars as you breathe a jet of @rf@Ri@Ye@Rr@ry@W flames onto $S arm!@n",
                target = BREATH_HIT_LONG .. " YOUR arm!@n",
                room   = BREATH_HIT_LONG .. " @C$N's@W arm!@n",
            }, ctx)
        elseif loc == "leg" then
            a.message({
                actor  = "@CYou@W aim your mouth at @C$N@W and open it wide slowly. The bright white flame within roars as you breathe a jet of @rf@Ri@Ye@Rr@ry@W flames onto $S leg!@n",
                target = BREATH_HIT_LONG .. " YOUR leg!@n",
                room   = BREATH_HIT_LONG .. " @C$N's@W leg!@n",
            }, ctx)
        else
            a.message({
                actor  = "@CYou@W aim your mouth at @C$N@W and open it wide slowly. The bright white flame within roars as you breathe a jet of @rf@Ri@Ye@Rr@ry@W flames onto $S body!@n",
                target = BREATH_HIT_LONG .. " YOUR body!@n",
                room   = BREATH_HIT_LONG .. " @C$N's@W body!@n",
            }, ctx)
        end
        apply_burn(inst.attacker, inst.target, inst.spar)
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@CYou@W move to breathe flames on @C$N@W, but miss!@n",
            target = "@C$n@W moves to breathe flames on you, but misses!@n",
            room   = "@c$n@W moves to breathe flames on @C$N@W, but somehow misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W manages to dodge the fiery jets of flames from your mouth!@n",
            target = "@WYou dodge the fiery jets of flames coming from @C$n's@W mouth!@n",
            room   = "@C$N@W manages to dodge the fiery jets of flames coming from @c$n's@W mouth!@n",
        }, ctx)
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W moves quickly and blocks your fiery breath!@n",
            target = "@WYou move quickly and block @C$n's@W fiery breath!@n",
            room   = "@C$N@W moves quickly and blocks @c$n's@W fiery breath!@n",
        }, ctx)
        apply_burn(inst.attacker, inst.target, inst.spar)
    end,
}
