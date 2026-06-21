local function act() return require("dbat").lib.act end

local CHANT_ACTOR =
    "@WYou cup your hands at your side and begin to pool your charged ki there. " ..
    "As the ki pools there you begin to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. " ..
    "Suddenly you bring your hands forward facing @C$N@W and shout '@BHAAAA!!!@W' " ..
    "while releasing a bright blue kamehameha at $M! "

local CHANT_TARGET =
    "@C$n@W cups $s hands at $s side and begins to pool charged ki there. " ..
    "As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. " ..
    "Suddenly $e brings $s hands forward facing you and shouts '@BHAAAA!!!@W' " ..
    "while releasing a bright blue kamehameha! "

local CHANT_ROOM =
    "@C$n@W cups $s hands at $s side and begins to pool charged ki there. " ..
    "As the ki pools there $e begins to chant, '@BKaaaaa@bmeeee@Bhaaaaaaa@bmeeeee@W'. " ..
    "Suddenly $e brings $s hands forward facing @c$N@W and shouts '@BHAAAA!!!@W' " ..
    "while releasing a bright blue kamehameha! "

local CONCLUSIONS = {
    body = {
        actor  = "It slams into $S body and explodes!@n",
        target = "It slams into your body and explodes!@n",
        room   = "It slams into $S body and explodes!@n",
    },
    head = {
        actor  = "It slams into $S face and explodes!@n",
        target = "It slams into your face and explodes!@n",
        room   = "It slams into $S face and explodes!@n",
    },
    arm = {
        actor  = "It slams into $S arm and explodes!@n",
        target = "It slams into your arm and explodes!@n",
        room   = "It slams into $S arm and explodes!@n",
    },
    leg = {
        actor  = "It slams into $S leg and explodes!@n",
        target = "It slams into your leg and explodes!@n",
        room   = "It slams into $S leg and explodes!@n",
    },
}

return {
    id   = "kamehameha",
    family = "ki",
    name = "Kamehameha",
    skill = "kamehameha",
    tier  = 3,
    elements = { ki = 1.0 },
    limbs_required = {},
    damages_limbs = false,
    can_combo = false,
    in_combo  = false,
    can_trigger_multihit = false,
    in_multihit          = false,
    can_block = false,
    can_parry = false,
    can_dodge = true,
    consumes_charge = true,
    min_charge = 100,
    max_charge = 0,
    base_accuracy = 1.2,
    base_power    = 2.5,
    spar_safe = true,

    on_hit = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        local loc = inst.hit_location
        local con = CONCLUSIONS[loc] or CONCLUSIONS.body
        a.message({
            actor  = CHANT_ACTOR .. con.actor,
            target = CHANT_TARGET .. con.target,
            room   = CHANT_ROOM   .. con.room,
        }, ctx)
        -- TODO: dam_eq_loc
    end,

    on_miss = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@WYou can't believe it but your kamehameha misses, flying through the air harmlessly!@n",
            target = "@C$n@W fires a kamehameha at you, but misses!@n",
            room   = "@c$n@W fires a kamehameha at @C$N@W, but somehow misses!@n",
        }, ctx)
    end,

    on_dodged = function(inst)
        local a   = act()
        local ctx = { actor = inst.attacker, target = inst.target }
        a.message({
            actor  = "@C$N@W manages to dodge your kamehameha, letting it slam into the surroundings!@n",
            target = "@WYou dodge @C$n's@W kamehameha, letting it slam into the surroundings!@n",
            room   = "@C$N@W manages to dodge @c$n's@W kamehameha, letting it slam into the surroundings!@n",
        }, ctx)
        a.to_room("@wA bright explosion erupts from the impact!\r\n", { actor = inst.attacker })
        -- TODO: dodge_ki scatter damage to room
    end,

    on_blocked = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W moves quickly and blocks your kamehameha!@n",
            target = "@WYou move quickly and block @C$n's@W kamehameha!@n",
            room   = "@C$N@W moves quickly and blocks @c$n's@W kamehameha!@n",
        }, ctx)
        -- TODO: blocked kamehameha deals 1/4 damage back
    end,

    on_absorbed = function(inst)
        local ctx = { actor = inst.attacker, target = inst.target }
        act().message({
            actor  = "@C$N@W absorbs your kamehameha into $S android systems!@n",
            target = "@WYou absorb @C$n's@W kamehameha into your android systems!@n",
            room   = "@C$N@W absorbs @c$n's@W kamehameha into $S android systems!@n",
        }, ctx)
    end,

    on_after_cost = function(inst)
        local skill = inst.skill_level
        local amt   = inst.cost.ki or 0
        if amt <= 0 then return end
        local refund
        if     skill >= 100 then refund = math.floor(amt * 0.25)
        elseif skill >= 60  then refund = math.floor(amt * 0.10)
        elseif skill >= 40  then refund = math.floor(amt * 0.05)
        end
        if refund and refund > 0 then
            inst.attacker:meter_mod_int("ki", refund)
        end
    end,
}
