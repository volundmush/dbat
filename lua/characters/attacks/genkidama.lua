local function ke() return require("lua.libs.ki_effects") end
local function act() return require("dbat").lib.act end

local function is_same_group(person, ch)
    if not (person:has_group() and person:aff_flagged(require("dbat").consts.aff_flags.GROUP)) then
        return false
    end
    local leader    = person:following_get()
    local my_leader = ch:following_get()
    return (leader    and leader:is_same(ch))        or
           (my_leader and my_leader:is_same(person)) or
           (leader and my_leader and leader:is_same(my_leader))
end

return {
    id    = "genkidama",
    family = "ki",
    name  = "Genkidama",
    skill = "genkidama",
    tier  = 5,
    elements = { ki = 1.0 },
    limbs_required = {},
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = false,
    can_parry = false,
    can_dodge = false,
    consumes_charge = true,
    base_accuracy = 999,
    spar_safe = false,

    on_check = function(inst)
        local ch = inst.attacker

        -- Genkidama uses its own skill roll, not the normal accuracy phase
        local prob = ch:skill_get("genkidama")
        if prob < math.random(1, 115) - 20 then
            ch:send_line("@RYou power up the genkidama but the energy slips away from your control!@n")
            return false
        end

        -- INT-based descent timer (matches C++ tick_huge_ki distance calc)
        local dista = 15 - math.floor((ch:stat_get("intelligence") or 0) * 0.1)
        if prob >= 100 then dista = dista - 3
        elseif prob >= 60 then dista = dista - 2
        elseif prob >= 40 then dista = dista - 1 end
        inst.bomb_distance = math.max(1, dista)

        -- Gather ki from same-group room members
        local gathered = 0
        for person in ch:room_get():people() do
            if not person:is_same(ch) and is_same_group(person, ch) then
                local ki_now  = person:meter_current("ki")
                local contrib = math.floor(ki_now / 10)
                person:meter_mod_int("ki", -math.floor(ki_now / 20))
                gathered = gathered + contrib
                person:send_line("@CYou donate your energy to the @BSpirit Bomb@C!@n")
                ch:send_line("@B%s@B contributes energy to the Spirit Bomb!@n", person:name_get())
            end
        end
        inst.gathered_ki = gathered

        if gathered > 0 then
            ch:send_line("@BYou gather @W%d@B energy from your allies!@n", gathered)
        end
    end,

    on_calculate_damage = function(inst)
        local ch     = inst.attacker
        local charge = inst.charge_used > 0
            and (inst.charge_used / ch:meter_max("ki"))
            or 0.5
        local base = math.floor(ch:meter_max("lifeforce") * charge * 0.002)
        return base + (inst.gathered_ki or 0)
    end,

    on_hit = function(inst)
        local ch   = inst.attacker
        local vict = inst.target
        act().message({
            actor  = "@BYou form a massive @bSpirit Bomb@B high above @C$N@B!@n",
            target = "@c$n@B forms a massive @bSpirit Bomb@B high above you!@n",
            room   = "@c$n@B forms a massive @bSpirit Bomb@B high above @C$N@B!@n",
        }, { actor = ch, target = vict })
        ch:send_line("@BThe Spirit Bomb will descend in @W%d@B seconds!@n", inst.bomb_distance)

        ke().spawn_huge_ki(inst, inst.bomb_distance)
        inst.damage_to = {}  -- bomb handles detonation via script tick; no immediate damage
    end,

    on_miss = function(inst) end,
}
