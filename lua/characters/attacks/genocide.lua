local function ke() return require("lua.libs.ki_effects") end
local function act() return require("dbat").lib.act end
local ki = require("lua.libs.ki")

return {
    id    = "genocide",
    family = "ki",
    name  = "Genocide",
    skill = "genocide",
    tier  = 3,
    elements = { ki = 1.0 },
    limbs_required = { "arm" },
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = false,
    can_parry = false,
    can_dodge = false,
    consumes_charge = true,
    spar_safe = false,

    on_check = function(inst)
        local ch = inst.attacker
        if not ki.can_grav(ch) then return false end

        local sk    = ch:skill_get("genocide") or 0
        local intel = ch:stat_get("intelligence") or 0
        local dista = 15 - math.floor(intel * 0.1)
        if sk >= 100 then dista = dista - 3
        elseif sk >= 60 then dista = dista - 2
        elseif sk >= 40 then dista = dista - 1 end
        inst.bomb_distance = math.max(1, dista)
    end,

    on_hit = function(inst)
        local ch   = inst.attacker
        local vict = inst.target
        act().message({
            actor  = "@WYou raise one arm above your head and pour your charged ki there. A large swirling pink ball of energy begins to form above your raised hand. You grin viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is complete and you toss it at @c$N@W!@n",
            target = "@C$n@W raises one arm above $s head and pours $s charged ki there. A large swirling pink ball of energy begins to form above $s raised hand. @C$n@W grins viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is complete and $e tosses it at YOU!@n",
            room   = "@C$n@W raises one arm above $s head and pours $s charged ki there. A large swirling pink ball of energy begins to form above $s raised hand. @C$n@W grins viciously as the @mG@Me@wn@mo@Mc@wi@md@Me@W attack is complete and $e tosses it at @c$N@W!@n",
        }, { actor = ch, target = vict })
        ch:send_line("@MThe Genocide ball will arrive in @W%d@M seconds!@n", inst.bomb_distance)

        ke().spawn_genocide(inst, inst.bomb_distance)
        inst.damage_to = {}
    end,

    on_miss = function(inst)
        local ch   = inst.attacker
        local vict = inst.target
        act().message({
            actor  = "@WYou raise one arm above your head and pour your charged ki there. A large swirling pink ball of energy begins to form above your raised hand. You lose concentration and the ball of energy dissipates!@n",
            target = "@C$n@W raises one arm above $s head and pours $s charged ki there. A large swirling pink ball of energy begins to form above $s raised hand. @C$n@W loses concentration and the ball of energy dissipates!@n",
            room   = "@C$n@W raises one arm above $s head and pours $s charged ki there. A large swirling pink ball of energy begins to form above $s raised hand. @C$n@W loses concentration and the ball of energy dissipates!@n",
        }, { actor = ch, target = vict })
    end,
}
