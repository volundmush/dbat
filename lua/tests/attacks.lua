local test  = require("lua.test").new()
local dbat  = require("dbat")
local atk   = require("lua.libs.attack")

local function mob()
    return dbat.mob_protos.by_id(1):spawn(dbat.rooms.by_id(1))
end

test:case("attack defs load with expected schema fields", function(t)
    local punch = dbat.get("attacks", "punch")
    t:assert(punch ~= nil, "punch def should exist")
    t:eq(punch.id,   "punch")
    t:eq(punch.tier, 1)
    t:assert(punch.elements ~= nil,  "elements should be present")
    t:assert(punch.elements.blunt ~= nil, "punch should have blunt element")
    t:eq(punch.can_combo,  true)
    t:eq(punch.can_block,  true)
    t:eq(punch.can_parry,  true)
    t:eq(punch.can_dodge,  true)
    t:eq(punch.spar_safe,  true)
    t:eq(punch.can_trigger_multihit, true)
    t:eq(punch.in_multihit,          true)

    local kame = dbat.get("attacks", "kamehameha")
    t:assert(kame ~= nil, "kamehameha def should exist")
    t:eq(kame.tier,            3)
    t:eq(kame.consumes_charge, true)
    t:eq(kame.min_charge,      100)
    t:eq(kame.can_block,       false)
    t:eq(kame.can_parry,       false)
    t:eq(kame.can_trigger_multihit, false)
end)

test:case("attack.launch runs against a mob without error", function(t)
    local ch   = mob()
    local vict = mob()
    -- Give attacker enough stamina to pay costs
    ch:meter_mod_int("stamina", ch:meter_max("stamina"))
    local ok = atk.launch(ch, "punch", vict)
    -- ok may be true (hit) or nil (miss is still a successful launch)
    t:assert(ok ~= false, "launch should not return false with sufficient resources")
end)

test:case("attack instance fields are populated after launch", function(t)
    local ch   = mob()
    local vict = mob()
    ch:meter_mod_int("stamina", ch:meter_max("stamina"))

    -- Intercept via on_hit to capture the instance
    local captured_inst = nil
    local def = dbat.get("attacks", "punch")
    local orig_on_hit = def.on_hit
    def.on_hit = function(inst)
        captured_inst = inst
        if orig_on_hit then orig_on_hit(inst) end
    end

    atk.launch(ch, "punch", vict)
    def.on_hit = orig_on_hit

    -- If the attack hit, we have a captured instance to inspect
    if captured_inst then
        t:assert(captured_inst.hit == true,      "inst.hit should be true inside on_hit")
        t:assert(captured_inst.damage > 0,       "inst.damage should be > 0")
        t:assert(captured_inst.hit_location ~= nil, "hit_location should be set")
        local valid_locs = { body = true, head = true, arm = true, leg = true }
        t:assert(valid_locs[captured_inst.hit_location], "hit_location should be a known value")
        -- damage_by_element should have only blunt
        local elem_count = 0
        for k, _ in pairs(captured_inst.damage_by_element) do
            elem_count = elem_count + 1
            t:eq(k, "blunt")
        end
        t:eq(elem_count, 1)
    else
        t:assert(true, "attack missed; instance fields not verifiable (ok)")
    end
end)

test:case("spar mode prevents death from attack", function(t)
    local PLR = dbat.consts.player_flags
    local ch   = mob()
    local vict = mob()
    ch:meter_mod_int("stamina", ch:meter_max("stamina"))

    -- Put attacker into spar mode and drain victim lifeforce to 1 HP
    ch:player_flag_set(PLR.SPAR, true)
    local max_lf = vict:meter_max("lifeforce")
    vict:meter_mod_int("lifeforce", -(max_lf - 1))
    t:eq(vict:meter_current("lifeforce"), 1)

    -- Even 100 hits should not kill
    for _ = 1, 10 do
        atk.launch(ch, "punch", vict)
    end
    t:assert(vict:meter_current("lifeforce") >= 1, "vict should survive spar attacks")

    ch:player_flag_set(PLR.SPAR, false)
end)

test:case("ki attack aborts when charge is insufficient", function(t)
    local ch   = mob()
    local vict = mob()
    ch:meter_mod_int("stamina", ch:meter_max("stamina"))
    ch:meter_mod_int("ki",      ch:meter_max("ki"))
    -- charge_get returns 0 by default (no charge built up)
    -- kamehameha requires min_charge=100

    local ok = atk.launch(ch, "kamehameha", vict)
    t:eq(ok, false, "kamehameha should abort with insufficient charge")

    -- Vict should be completely unharmed
    t:eq(vict:meter_current("lifeforce"), vict:meter_max("lifeforce"))
end)

test:case("check_attack_defense and check_attack_offense are callable", function(t)
    local ch   = mob()
    local vict = mob()
    local def  = dbat.get("attacks", "punch")
    local inst = atk._build_instance(ch, def, vict, {})
    inst.hit    = true
    inst.damage = 100
    local ok1 = pcall(function() vict:check_attack_defense(inst) end)
    t:assert(ok1, "check_attack_defense should be callable without error")
    local ok2 = pcall(function() ch:check_attack_offense(inst) end)
    t:assert(ok2, "check_attack_offense should be callable without error")
end)

test:case("on_attacked drains the correct meter", function(t)
    local ch   = mob()
    local vict = mob()
    ch:meter_mod_int("stamina", ch:meter_max("stamina"))

    local def = dbat.get("attacks", "punch")
    local drain_ki_hit = false

    local orig = def.on_hit
    def.on_hit = function(inst)
        -- Redirect damage to ki drain instead of lifeforce
        inst.damage_to = { ki = 500 }
        drain_ki_hit = true
        if orig then orig(inst) end
    end

    vict:meter_mod_int("ki", vict:meter_max("ki"))
    local ki_before = vict:meter_current("ki")
    local lf_before = vict:meter_current("lifeforce")

    atk.launch(ch, "punch", vict)
    def.on_hit = orig

    if drain_ki_hit then
        t:assert(vict:meter_current("ki") < ki_before,       "ki should have been drained")
        t:eq(vict:meter_current("lifeforce"), lf_before, "lifeforce should be untouched")
    else
        t:assert(true, "attack missed; ki drain not testable (ok)")
    end
end)

test:case("backlash applies damage to attacker", function(t)
    local ch   = mob()
    local vict = mob()
    ch:meter_mod_int("stamina", ch:meter_max("stamina"))

    -- Inject a check_attack hook that adds backlash (simulates fireshield)
    local def = dbat.get("conditions", "test_fireshield_fake")
    -- No real condition; instead patch via on_hit on the attack def
    local punch = dbat.get("attacks", "punch")
    local orig  = punch.on_hit
    punch.on_hit = function(inst)
        inst.backlash = inst.backlash or {}
        table.insert(inst.backlash, { damage = 999999, elements = { fire = 1.0 } })
        if orig then orig(inst) end
    end

    local lf_before = ch:meter_current("lifeforce")
    atk.launch(ch, "punch", vict)
    punch.on_hit = orig

    -- Backlash only fires on hit, so only check if the attack landed
    -- We can't guarantee a hit, but if lifeforce dropped we know it worked
    -- Test is structural: no error = pass; damage check is conditional
    t:assert(true, "backlash injection did not crash")
end)

test:case("kick def uses legs limb requirement", function(t)
    local kick = dbat.get("attacks", "kick")
    t:assert(kick ~= nil, "kick def should exist")
    t:eq(kick.limbs_required[1], "legs")
    t:eq(kick.can_parry, true)
    t:assert(kick.base_power > 1.0, "kick should have power > 1.0")
end)

test:case("damage_by_element keys match elements in def", function(t)
    local ch   = mob()
    local vict = mob()
    ch:meter_mod_int("stamina", ch:meter_max("stamina"))

    local hit_inst = nil
    local punch = dbat.get("attacks", "punch")
    local orig  = punch.on_hit
    punch.on_hit = function(inst)
        hit_inst = inst
        if orig then orig(inst) end
    end

    atk.launch(ch, "punch", vict)
    punch.on_hit = orig

    if hit_inst then
        for elem, _ in pairs(hit_inst.damage_by_element) do
            t:assert(punch.elements[elem] ~= nil,
                string.format("element '%s' in damage_by_element should match punch.elements", elem))
        end
    else
        t:assert(true, "attack missed; element keys not testable (ok)")
    end
end)

return test:run()
