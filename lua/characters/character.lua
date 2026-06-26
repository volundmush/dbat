-- Per-character derived stat cache, keyed by character id.
-- Each entry stores {gen=N, ...derived_name=value} so stale entries
-- are detected via the modifier_gen counter from Zig.
local der_caches = {}

-- Lazy-loaded constants and libraries for room display functions.
local _C
local function C()
  if _C then return _C end
  local d = require("dbat")
  local mov = require("lua.libs.movement")
  _C = {
    AF     = d.consts.aff_flags,
    PLR    = d.consts.player_flags,
    PRF    = d.consts.prf_flags,
    dirs   = d.consts.direction_names,
    aura   = d.consts.aura_color_names,
    skin   = d.consts.skin_color_names,
    has_o2 = mov.has_o2,
  }
  return _C
end

local function get_act()
  return require("dbat").lib.act
end

-- Position suffix strings indexed by position integer (0-based → +1 for Lua).
local ROOM_POSITIONS = {
  " is dead",
  " is mortally wounded",
  " is lying here, incapacitated",
  " is lying here, stunned",
  " is sleeping here",
  " is resting here",
  " is sitting here",
  "!FIGHTING!",
  " is standing here",
}

local scale = 10000

local function der_total(ch, name)
  local id = ch:id_get()
  local gen = ch:modifier_gen()
  local cache = der_caches[id]
  if cache and cache.gen == gen then
    local v = cache[name]
    if v ~= nil then return v end
  else
    cache = {gen = gen}
    der_caches[id] = cache
  end

  local def = dbat.characters.registry.derived[name]
  if not def then return 0 end

  -- Base value
  local base
  if def.calculate_base then
    base = def.calculate_base(ch) or 0
  else
    base = ch:stat_get(def.base_stat or name)
  end

  if def.no_modifiers then
    local value = base
    if def.min_value ~= nil then value = math.max(value, def.min_value) end
    if def.max_value ~= nil then value = math.min(value, def.max_value) end
    cache[name] = value
    return value
  end

  -- Modifier accumulation: direct target
  local mods = ch:modifiers_for("derived", name)
  local flat = mods.flat
  local percent = mods.percent
  local multipliers = mods.multipliers
  local min_ov = mods.min
  local max_ov = mods.max
  local set_ov = mods.set

  -- Additional modifier targets
  if def.modifier_targets then
    for _, target in ipairs(def.modifier_targets) do
      local more = ch:modifiers_for(target[1], target[2])
      flat = flat + more.flat
      percent = percent + more.percent
      for _, m in ipairs(more.multipliers) do
        multipliers[#multipliers + 1] = m
      end
      if more.min ~= nil then
        min_ov = min_ov and math.max(min_ov, more.min) or more.min
      end
      if more.max ~= nil then
        max_ov = max_ov and math.min(max_ov, more.max) or more.max
      end
      if more.set ~= nil then set_ov = more.set end
    end
  end

  -- Legacy modifiers (from old affect system)
  if def.legacy_modifiers then
    for _, lm in ipairs(def.legacy_modifiers) do
      flat = flat + ch:legacy_modifier(lm[1], lm[2])
    end
  end

  -- Apply modifiers to base
  local value = base + flat
  if percent ~= 0 then
    value = value + math.floor(value * percent / scale)
  end
  for _, m in ipairs(multipliers) do
    value = math.floor(value * m / scale)
  end

  -- Clamp with definition bounds first, then overrides
  if def.min_value ~= nil then value = math.max(value, def.min_value) end
  if def.max_value ~= nil then value = math.min(value, def.max_value) end
  if min_ov ~= nil then value = math.max(value, min_ov) end
  if max_ov ~= nil then value = math.min(value, max_ov) end
  if set_ov ~= nil then value = set_ov end

  cache[name] = value
  return value
end

local function can_see(ch, entref)
  local kind = entref and entref:reftype()

  if kind == "character" then
    return ch:can_see_char(entref)
  end
  if kind == "object" then
    return ch:can_see_obj(entref)
  end

  error("Character:can_see expected character or object", 2)
end

local function apparent_sex(ch, viewer)
  viewer = viewer or ch

  return ch:sex_get()
end

local function apparent_race(ch, viewer)
  viewer = viewer or ch

  return ch:race_get()
end

-- Returns the name viewer uses for ch: real name for NPCs/admins, intro
-- nickname if known, or a race/sex placeholder ("a male saiyan") if not.
local function display_name_for(ch, viewer)
  viewer = viewer or ch

  if ch:is_npc() then
    return ch:short_description_get()
  end

  -- Admins and NPCs always see the real name.
  if viewer:is_npc() or viewer:admin_level_get() > 0 then
    return ch:name_get()
  end

  -- Admin PCs are always known by real name regardless.
  if ch:admin_level_get() > 0 then
    return ch:name_get()
  end

  -- Player-to-player: use intro system.
  -- intro_known returns 1 if viewer knows ch, 0 if not, 2 if no file.
  local known = viewer:intro_known(ch)
  if known == 1 then
    return viewer:get_intro_name(ch) or ch:name_get()
  end

  -- Viewer hasn't been introduced: show descriptive placeholder.
  return ch:introd_calc() or ch:race_get()
end

local function keywords_for(ch, viewer)
  viewer = viewer or ch
  local keywords = {}

  local function add_words(str)
    for word in string.gmatch(str or "", "%S+") do
      keywords[#keywords + 1] = word
    end
  end

  if ch:is_npc() then
    add_words(ch:name_get())
    return keywords
  end

  if viewer:is_npc() or viewer:admin_level_get() > 0 or ch:admin_level_get() > 0 then
    add_words(ch:name_get())
    return keywords
  end

  local known = viewer:intro_known(ch)
  if known == 1 then
    add_words(viewer:get_intro_name(ch))
  else
    -- Unknown player: keywords come from the descriptive calc.
    add_words(ch:introd_calc())
  end

  return keywords
end

local function find_in_registry(category, legacy_id)
  local registry = dbat.characters.registry[category]
  if not registry then return nil end
  for _, entry in pairs(registry) do
    if entry.legacy_id == legacy_id then
      return entry
    end
  end
  return nil
end

local function append_mods(out, mods)
  for _, m in ipairs(mods or {}) do
    out[#out + 1] = m
  end
end

local function modifiers(ch)
  local all = {}

  -- Race
  local race_id = ch:race_get()
  if race_id then
    local entry = find_in_registry("races", race_id)
    if entry and entry.modifiers then
      append_mods(all, entry.modifiers(ch))
    end
  end

  -- Sensei
  local sensei_id = ch:sensei_get()
  if sensei_id then
    local entry = find_in_registry("senseis", sensei_id)
    if entry and entry.modifiers then
      append_mods(all, entry.modifiers(ch))
    end
  end

  -- Conditions
  for _, cond_id in ipairs(ch:conditions()) do
    local cond = ch:condition(cond_id)
    local cond_def = dbat.characters.registry.conditions[cond_id]
    if cond_def and cond_def.modifiers then
      append_mods(all, cond_def.modifiers(ch, cond))
    end
  end

  -- Room
  local room = ch:room_get()
  if room then
    append_mods(all, room:modifiers())
  end

  -- Sitting object
  local obj = ch:sits_get()
  if obj then
    append_mods(all, obj:modifiers())
  end

  return all
end

local NEEDS_INTERVAL_MS    = 150000
local SURVIVAL_INTERVAL_MS = 100000  -- SECS_PER_MUD_HOUR(300) / 3 * 1000

local function decrement_needs(ch)
  local PC = dbat.consts.player_conds
  for _, cond in ipairs({ PC.DRUNK, PC.HUNGER, PC.THIRST }) do
    if math.random(1, 2) == 2 then
      ch:gain_condition(cond, -1)
    end
  end
end

-- Port of tick_char_survival() from local_limits.cpp (PC path only; NPCs handled in C++).
-- Returns true if the character died and should not be further processed.
local function survival_check(ch)
  local room = ch:room_get()
  if not room then return false end

  local ST  = dbat.consts.sector_types
  local RF  = dbat.consts.room_flags
  local MF  = dbat.consts.mob_flags
  local act = get_act()

  -- 1. WATER_NOSWIM: swimming stamina drain
  if room:sector_type_get() == ST.WATER_NOSWIM
      and not ch:carried_by_char_get()
      and ch:race_get() ~= "kanassan" then
    local cur_st   = ch:meter_current("stamina")
    local carry_wt = ch:der_total("weight_carried")
    ch:meter_mod_int("stamina", -carry_wt)
    if cur_st >= carry_wt then
      act.to_char(ch, "@bYou swim in place.@n")
      act.around(ch, "@C$n@b swims in place.@n")
    else
      act.to_char(ch, "@RYou are drowning!@n")
      act.around(ch, "@C$n@b gulps water as $e struggles to stay above the water line.@n")
      local max_pl = ch:meter_max("powerlevel")
      if ch:meter_current("powerlevel") - (max_pl // 3) <= 0 then
        act.to_char(ch, "@rYou drown!@n")
        act.around(ch, "@R$n@r drowns!@n")
        ch:die()
        return true
      else
        ch:meter_mod_int("powerlevel", -(max_pl // 3))
      end
    end
  end

  local ki_regen = ch:der_total("ki_regen")
  local pl_regen = ch:der_total("powerlevel_regen")

  -- 2. Sunken room (not space), no O2: ki drain then health drain
  if not C().has_o2(ch) and room:is_sunken() and not room:flagged(RF.SPACE) then
    local max_ki = ch:meter_max("ki")
    if ch:meter_current("ki") - ki_regen > max_ki // 200 then
      ch:send_line("Your ki holds an atmosphere around you.")
      ch:meter_mod_int("ki", -(ki_regen + math.floor(max_ki * 0.005)))
    else
      local max_pl = ch:meter_max("powerlevel")
      if ch:meter_current("powerlevel") - pl_regen > math.floor(max_pl * 0.05) then
        ch:send_line("You struggle trying to hold your breath!")
        ch:meter_mod_int("powerlevel", -(pl_regen + math.floor(max_pl * 0.05)))
      elseif ch:meter_current("powerlevel") <= max_pl // 20 then
        ch:send_line("You have drowned!")
        act.around(ch, "@W$n@W drowns right in front of you.@n")
        ch:die()
        return true
      end
    end
  end

  -- 3. Space, no O2: same pattern with slightly different ki threshold
  if not C().has_o2(ch) and room:flagged(RF.SPACE) then
    local max_ki = ch:meter_max("ki")
    if ch:meter_current("ki") - ki_regen > math.floor(max_ki * 0.005) then
      ch:send_line("Your ki holds an atmosphere around you.")
      ch:meter_mod_int("ki", -(ki_regen + math.floor(max_ki * 0.005)))
    else
      local max_pl = ch:meter_max("powerlevel")
      if ch:meter_current("powerlevel") - pl_regen > math.floor(max_pl * 0.05) then
        ch:send_line("You struggle trying to hold your breath!")
        ch:meter_mod_int("powerlevel", -(pl_regen + math.floor(max_pl * 0.05)))
      elseif ch:meter_current("powerlevel") <= max_pl // 20 then
        ch:send_line("You have drowned!")
        ch:meter_mod("powerlevel", -math.floor(1000000 * 0.01))
        act.around(ch, "@W$n@W drowns right in front of you.@n")
        ch:die()
        return true
      end
    end
  end

  -- 4. Lava (geffect==6): burn 5% health
  if not ch:condition_has("flying")
      and room:geffect_get() == 6
      and not ch:mob_flagged(MF.NOKILL)
      and ch:race_get() ~= "demon" then
    act.to_char(ch, "@rYour legs are burned by the lava!@n")
    act.around(ch, "@R$n@r's legs are burned by the lava!@n")
    ch:meter_mod("powerlevel", -math.floor(1000000 * 0.05))
    if ch:meter_current("powerlevel") < 0 then
      act.to_char(ch, "@rYou have burned to death!@n")
      act.around(ch, "@R$n@r has burned to death!@n")
      ch:die()
      return true
    end
  end

  return false
end

local function on_event(ch, kind)
  if kind == "activate" then
    ch:event_cancel("decrement_needs")
    ch:event_schedule("decrement_needs", NEEDS_INTERVAL_MS, NEEDS_INTERVAL_MS)
    ch:event_cancel("survival_check")
    ch:event_schedule("survival_check", SURVIVAL_INTERVAL_MS, SURVIVAL_INTERVAL_MS)
    return
  end

  if kind == "decrement_needs" then
    decrement_needs(ch)
    return
  end

  if kind == "survival_check" then
    survival_check(ch)
    return
  end

  local subsystem, id, event_name = kind:match("^([^:]+):([^:]+):?(.*)$")
  event_name = (event_name and event_name ~= "") and event_name or "tick"

  if subsystem == "condition" then
    if not ch:condition_has(id) then return end
    local def = dbat.get("conditions", id)
    if def then def:dispatch_event(ch, ch:condition(id), event_name) end
  elseif subsystem == "script" then
    if not ch:script_has(id) then return end
    local def = dbat.get("character_scripts", id)
    if def and def.on_event then def.on_event(ch, ch:script(id), event_name) end
  elseif subsystem == "transformation" then
    -- future: route to transformation registry
  end
end

local function act_self(ch, msg, ctx)
  context = ctx or {}
  context.actor = ch
  local dbat = require("dbat")
  dbat.lib.act.to_char(ch, msg, context)
end

local function act_around(ch, msg, ctx)
  context = ctx or {}
  local dbat = require("dbat")
  dbat.lib.act.around(ch, msg, context)
end

-- Three-way message send (actor/target/room) with ch as implicit actor.
-- ctx may contain { target = vict, tool = obj, ... }; actor is always ch.
local function act_message(ch, msgs, ctx)
  local context = ctx or {}
  context.actor = ch
  require("dbat").lib.act.message(msgs, context)
end

-- Convenience wrapper: ch:act(msg, hide_inv, obj, vict, dest)
-- dest: "char" | "room" | "vict" | "notvict"
-- Mirrors C++ act() call signature used in ported commands.
local function act(ch, msg, hide_inv, obj, vict, dest)
  local act_lib = get_act()
  local ctx = { actor = ch, tool = obj, target = vict, hide_invisible = hide_inv }
  if dest == "char" then
    act_lib.to_char(ch, msg, ctx)
  elseif dest == "room" then
    act_lib.around(ch, msg, ctx)
  elseif dest == "vict" then
    if vict ~= nil then act_lib.to_char(vict, msg, ctx) end
  elseif dest == "notvict" then
    local room = ch:room_get()
    if not room then return end
    ctx.exclude = { ch }
    if vict ~= nil then ctx.exclude[2] = vict end
    ctx.room = room
    act_lib.to_room(msg, ctx)
  end
end

-- Build the argparams table — mirrors lua_api.zig:pushArgParams/pushTokens.
local function build_argparams(arguments)
    local function tokenize(s)
        local t = {}
        for tok in string.gmatch(s, "%S+") do t[#t + 1] = tok end
        return t
    end
    local params = { raw = arguments }
    local eq = string.find(arguments, "=", 1, true)
    if eq then
        params.equals      = true
        params.lsargs      = string.sub(arguments, 1, eq - 1)
        params.rsargs      = string.sub(arguments, eq + 1)
        params.left_tokens  = tokenize(params.lsargs)
        params.right_tokens = tokenize(params.rsargs)
    else
        params.equals      = false
        params.lsargs      = arguments
        params.rsargs      = ""
        params.left_tokens  = tokenize(arguments)
        params.right_tokens = {}
    end
    params.tokens = tokenize(arguments)
    return params
end

-- Alias matching — mirrors lua_api.zig:aliasMatches().
-- alias format: {pattern, min_len, sensitive?} or {name=..., min=..., sensitive=...}
local function alias_matches_word(aliases, word)
    for _, alias in ipairs(aliases or {}) do
        local pattern   = alias[1] or alias.name
        local min_len   = alias[2] or alias.min or (pattern and #pattern)
        local sensitive = alias[3] or alias.sensitive or false
        if not pattern then goto continue end
        if #word >= min_len and #word <= #pattern then
            local prefix = string.sub(pattern, 1, #word)
            local matched = sensitive and (word == prefix)
                                       or (string.lower(word) == string.lower(prefix))
            if matched then return true end
        end
        ::continue::
    end
    return false
end

-- Try to dispatch `input` against the commands in cmd_class (must have sorted_list()).
-- Returns true if a command was matched (even if can_execute blocked it).
-- Used by: pcommand_try (bypass wait), command_try (before C++ table).
local function execute_command(ch, input, cmd_class)
    local word = (input or ""):match("^(%S+)") or ""
    local rest = (input or ""):match("^%S+%s*(.-)$") or ""
    for _, def in ipairs(cmd_class.sorted_list()) do
        if alias_matches_word(def.aliases, word) then
            if def.can_see and not def.can_see(ch) then goto next end
            if def.can_execute then
                local ok, reason = def.can_execute(ch)
                if not ok then
                    ch:send((reason or "You cannot do that.") .. "\r\n")
                    return true
                end
            end
            def.execute({ ch = ch, actor = ch,
                          command = def.id or word, alias = word,
                          arguments = rest, argparams = build_argparams(rest) })
            return true
        end
        ::next::
    end
    return false
end

-- Return sorted list of commands from cmd_class visible to ch.
-- Useful for help displays, score sheets, etc.
local function visible_commands(ch, cmd_class)
    local visible = {}
    for _, def in ipairs(cmd_class.sorted_list()) do
        if not def.can_see or def.can_see(ch) then
            visible[#visible + 1] = def
        end
    end
    return visible
end

-- Port of C++ init_skill(): returns the effective skill level for a roll.
-- For PCs: their actual skill (+ TRANSMISSION bonus, capped 118).
-- For NPCs: a level-scaled random range matching old mob AI behaviour.
local function init_skill(ch, skill_name)
    if not ch:is_npc() then
        local sk = ch:skill_get(skill_name)
        if ch:player_flagged(dbat.consts.player_flags.TRANSMISSION) then
            sk = sk + 4
        end
        return math.min(sk, 118)
    end
    local level = ch:stat_get("level")
    if     level <= 10  then return math.random(30,  50)
    elseif level <= 20  then return math.random(45,  65)
    elseif level <= 30  then return math.random(55,  70)
    elseif level <= 50  then return math.random(65,  80)
    elseif level <= 70  then return math.random(75,  90)
    elseif level <= 80  then return math.random(85, 100)
    elseif level <= 90  then return math.random(90, 100)
    elseif level <= 100 then return math.random(95, 100)
    elseif level <= 110 then return math.random(95, 105)
    else                     return math.random(100, 110)
    end
end

-- Search the character's room for a visible target by name.
-- With no name (or empty string) returns the current fighting target.
-- With allow_objects=true, room objects are also eligible.
local function acquire_room_target(ch, name, allow_objects)
    if not name or name == "" then
        return ch:fighting_get()
    end
    local room = ch:room_get()
    if not room then return nil end
    local Search = require("lua.libs.search").new
    local s = Search(ch)
        :add_room_people(room)
        :add_filter(function(searcher, e) return searcher:can_see(e) end)
    if allow_objects then
        s:add_room_objects(room)
    end
    return s:find_one(name)
end

-- Called by the attack pipeline so victim conditions/scripts can respond.
-- Defense conditions write inst.blocked/parried/dodged/absorbed/partial_negation/backlash.
local function launch_attack(ch, attack_id, target, opts)
    return require("lua.libs.attack").launch(ch, attack_id, target, opts)
end

local function check_attack_offense(ch, instance)
    for _, cond_id in ipairs(ch:conditions()) do
        local def = dbat.get("conditions", cond_id)
        if def and def.on_check_attack_offense then
            def.on_check_attack_offense(ch, ch:condition(cond_id), instance)
        end
    end
    local race_id = ch:race_get()
    if race_id then
        local race_def = dbat.get("races", race_id)
        if race_def and race_def.on_check_attack_offense then
            race_def.on_check_attack_offense(ch, instance)
        end
    end
    local sensei_id = ch:sensei_get()
    if sensei_id then
        local sensei_def = dbat.get("senseis", sensei_id)
        if sensei_def and sensei_def.on_check_attack_offense then
            sensei_def.on_check_attack_offense(ch, instance)
        end
    end
end

local function check_attack_defense(ch, instance)
    for _, cond_id in ipairs(ch:conditions()) do
        local def = dbat.get("conditions", cond_id)
        if def and def.on_check_attack_defense then
            def.on_check_attack_defense(ch, ch:condition(cond_id), instance)
        end
    end
    local race_id = ch:race_get()
    if race_id then
        local race_def = dbat.get("races", race_id)
        if race_def and race_def.on_check_attack_defense then
            race_def.on_check_attack_defense(ch, instance)
        end
    end
    local sensei_id = ch:sensei_get()
    if sensei_id then
        local sensei_def = dbat.get("senseis", sensei_id)
        if sensei_def and sensei_def.on_check_attack_defense then
            sensei_def.on_check_attack_defense(ch, instance)
        end
    end
end

-- General-purpose damage entry point.
-- dmg_table: { powerlevel = N, ki = M, ..., spar = bool }
-- Powerlevel damage routes through the damage pipeline (lua/libs/damage.lua).
-- Other meter entries (ki drain, stamina drain, etc.) are applied directly.
-- Callers that need element-specific pipeline hooks should use damage.inflict() directly.
local function damage(ch, dmg_table, source)
    local spar = dmg_table.spar or false

    -- Non-powerlevel entries (ki drain, stamina drain, etc.) bypass the pipeline.
    for meter, amount in pairs(dmg_table) do
        if meter ~= "spar" and meter ~= "powerlevel" and type(amount) == "number" and amount > 0 then
            ch:meter_mod_int(meter, -amount)
        end
    end

    local pl_dmg = dmg_table.powerlevel
    if pl_dmg and type(pl_dmg) == "number" and pl_dmg > 0 then
        require("lua.libs.damage").inflict({
            source            = source,
            target            = ch,
            damage            = pl_dmg,
            damage_by_element = {},
            spar              = spar,
            hooks_applied     = true,
        })
    end
end

-- Called after on_hit; applies damage_to meters, handles death, fires victim-side hooks.
local function on_attacked(ch, instance)
    -- Non-powerlevel damage_to entries (ki drain, etc.) applied directly with spar clamp.
    for meter, amount in pairs(instance.damage_to) do
        if meter ~= "powerlevel" and amount > 0 then
            local dmg = amount
            if instance.spar then
                local cur = ch:meter_current(meter)
                if cur > 1 then dmg = math.min(dmg, cur - 1) end
            end
            ch:meter_mod_int(meter, -dmg)
        end
    end

    -- Powerlevel routes through the damage pipeline.
    -- Offense/defense hooks already ran in the attack pipeline, so hooks_applied = true.
    if (instance.damage_to.powerlevel or 0) > 0 then
        instance.hooks_applied = true
        require("lua.libs.damage").inflict(instance)
    end

    -- Condition hooks only for survivors (on_kill hooks fire inside damage.inflict).
    if not instance.killed then
        for _, cond_id in ipairs(ch:conditions()) do
            local def = dbat.get("conditions", cond_id)
            if def and def.on_attacked then
                def.on_attacked(ch, ch:condition(cond_id), instance)
            end
        end
    end
    if instance.xp_credit > 0 then
        instance.attacker:gain_exp(instance.xp_credit)
    end
end

local function meter_is_full(ch, name)
    return ch:meter_current(name) >= ch:meter_max(name)
end

local function send_text(ch, msg, ...)
  local text = select('#', ...) > 0 and string.format(msg, ...) or msg
  ch:send_raw(text)
end

local function send_around(ch, msg)
  local room = ch:room_get()
  if not room then return end
  for other in room:people() do
    if other:id_get() ~= ch:id_get() then
      other:send(msg)
    end
  end
end

local function send_line(ch, msg, ...)
  local text = select('#', ...) > 0 and string.format(msg, ...) or msg
  if not text:match("\r\n$") then text = text .. "\r\n" end
  ch:send_raw(text)
end

local function send_line_around(ch, msg, ...)
  local text = select('#', ...) > 0 and string.format(msg, ...) or msg
  if not text:match("\r\n$") then text = text .. "\r\n" end
  local room = ch:room_get()
  if not room then return end
  for other in room:people() do
    if other:id_get() ~= ch:id_get() then
      other:send_raw(text)
    end
  end
end

-- Returns true if ch has any active player transformation.
local function is_transformed(ch)
  local PLR = C().PLR
  return ch:player_flagged(PLR.TRANS1) or ch:player_flagged(PLR.TRANS2)
      or ch:player_flagged(PLR.TRANS3) or ch:player_flagged(PLR.TRANS4)
      or ch:player_flagged(PLR.TRANS5) or ch:player_flagged(PLR.TRANS6)
      or ch:player_flagged(PLR.OOZARU)
end

-- Name viewer uses for a combat target (intro-aware).
local function combat_name_for(viewer, target)
  if viewer:intro_known(target) == 1 then
    return viewer:get_intro_name(target) or target:introd_calc() or ""
  end
  return target:introd_calc() or ""
end

-- Append charge/transform aura lines (mirrors lines 2979-3011 of list_one_char).
local function append_charge_aura(t, ch, viewer, ctx, c)
  local act = get_act()
  local PLR = c.PLR
  local race     = ch:race_get()
  local is_saiyan = (race == "saiyan" or race == "halfbreed")
  local in_oozaru = ch:player_flagged(PLR.OOZARU)
  local charged   = ch:charge_get() > 0
  local xformed   = is_transformed(ch)
  local aura_name = c.aura[ch:aura_get() + 1] or "white"

  if is_saiyan then
    if not in_oozaru and charged and xformed then
      t[#t+1] = act.render_for(viewer, "@w...$e has a @Ybright @Yg@yo@Yl@yd@Ye@yn@w aura around $s body!\r\n", ctx)
    elseif not in_oozaru and charged and not xformed then
      t[#t+1] = act.render_for(viewer, "@w...$e has a @Ybright@w " .. aura_name .. " aura around $s body!\r\n", ctx)
    elseif not in_oozaru and not charged and xformed then
      t[#t+1] = act.render_for(viewer, "@w...$e has energy crackling around $s body!\r\n", ctx)
    elseif in_oozaru and charged then
      t[#t+1] = act.render_for(viewer, "@w...$e is in the form of a @rgreat ape@w!\r\n", ctx)
    elseif in_oozaru and not charged then
      t[#t+1] = act.render_for(viewer, "@w...$e has energy crackling around $s @rgreat ape@w body!\r\n", ctx)
    end
  else
    if xformed and race ~= "android" then
      t[#t+1] = act.render_for(viewer, "@w...$e has energy crackling around $s body!\r\n", ctx)
    end
    if charged then
      t[#t+1] = act.render_for(viewer, "@w...$e has a @Ybright@w " .. aura_name .. " aura around $s body!\r\n", ctx)
    end
  end
end

-- Append the effect/aura lines that appear after the main character line.
local function append_effects(t, ch, viewer, ctx, c)
  local act = get_act()
  local AF, PLR = c.AF, c.PLR

  if ch:condition_has("fishing") and not ch:is_npc() then
    t[#t+1] = act.render_for(viewer, "@w...$e is @Cfishing@w.@n\r\n", ctx)
  end
  if not viewer:is_npc() and ch:player_flagged(PLR.AURALIGHT) then
    local aura_name = c.aura[ch:aura_get() + 1] or "white"
    t[#t+1] = act.render_for(viewer, "...is surrounded by a bright " .. aura_name .. " aura.@n\r\n", ctx)
  end
  if ch:condition_has("barrier") and not ch:know_skill("aqua_barrier") then
    t[#t+1] = act.render_for(viewer, "@w...$e has a @bbarrier@w around $s body!\r\n", ctx)
  end
  if ch:condition_has("fireshield") then
    t[#t+1] = act.render_for(viewer, "@w...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!\r\n", ctx)
  end
  if ch:condition_has("healing_glow") then
    t[#t+1] = act.render_for(viewer, "@w...$e has a serene @Cblue@Y glow@w around $s body.\r\n", ctx)
  end
  if ch:condition_has("ethereal_armor") then
    t[#t+1] = act.render_for(viewer, "@w...$e has ghostly @Ggreen@w ethereal armor around $s body.\r\n", ctx)
  end
  if ch:condition_has("barrier") and ch:know_skill("aqua_barrier") then
    t[#t+1] = act.render_for(viewer, "@w...$e has a @bbarrier@w of @cwater@w and @CKi@w around $s body!\r\n", ctx)
  end
  if ch:condition_has("flying") then
    local alt = ch:condition_number_get("flying", "altitude")
    if alt == 1 then
      t[#t+1] = act.render_for(viewer, "@w...$e is in the air!\r\n", ctx)
    elseif alt == 2 then
      t[#t+1] = act.render_for(viewer, "@w...$e is high in the air!\r\n", ctx)
    end
  end
  if ch:stat_get("kaioken") > 0 then
    t[#t+1] = act.render_for(viewer, "@w...@r$e has a red aura around $s body!\r\n", ctx)
  end
  if not ch:is_npc() and ch:player_flagged(PLR.SPIRAL) then
    t[#t+1] = act.render_for(viewer, "@w...$e is spinning in a vortex!\r\n", ctx)
  end
  append_charge_aura(t, ch, viewer, ctx, c)
  if ch:condition_has("kyodaika") then
    t[#t+1] = act.render_for(viewer, "@w...$e has expanded $s body size@w!\r\n", ctx)
  end
  if ch:condition_has("hayasa") then
    t[#t+1] = act.render_for(viewer, "@w...$e has a soft @cblue@w glow around $s body!\r\n", ctx)
  end
  local feat = ch:feature_get()
  if feat then
    t[#t+1] = act.render_for(viewer, "@C" .. feat .. "@n\r\n", ctx)
  end
  local rdis = ch:rdisplay_get()
  if rdis and rdis ~= "Empty" then
    t[#t+1] = act.render_for(viewer, "..." .. rdis .. "\r\n", ctx)
  end
end

-- Subset of effect lines used on NPC path-1 (long_descr, mirrors C++ lines 2570-2600).
local function append_npc_path1_effects(t, ch, viewer, ctx, c)
  local act = get_act()
  local AF, PLR = c.AF, c.PLR

  if ch:condition_has("flying") then
    local alt = ch:condition_number_get("flying", "altitude")
    if alt == 1 then t[#t+1] = act.render_for(viewer, "...$e is in the air!\r\n", ctx)
    elseif alt == 2 then t[#t+1] = act.render_for(viewer, "...$e is high in the air!\r\n", ctx)
    end
  end
  if ch:condition_has("barrier") and not ch:know_skill("aqua_barrier") then
    t[#t+1] = act.render_for(viewer, "...$e has a barrier around $s body!\r\n", ctx)
  end
  if ch:condition_has("fireshield") then
    t[#t+1] = act.render_for(viewer, "...$e has @rf@Rl@Ya@rm@Re@Ys@w around $s body!\r\n", ctx)
  end
  if ch:condition_has("barrier") and ch:know_skill("aqua_barrier") then
    t[#t+1] = act.render_for(viewer, "...$e has a @Gbarrier@w of @cwater@w and @Cki@w around $s body!\r\n", ctx)
  end
  if not ch:is_npc() and ch:player_flagged(PLR.SPIRAL) then
    t[#t+1] = act.render_for(viewer, "...$e is spinning in a vortex!\r\n", ctx)
  end
  if ch:charge_get() > 0 then
    local aura_name = c.aura[ch:aura_get() + 1] or "white"
    t[#t+1] = act.render_for(viewer, "...$e has a bright " .. aura_name .. " aura around $s body!\r\n", ctx)
  end
  if ch:condition_has("dark_metamorphosis") then
    t[#t+1] = act.render_for(viewer, "@w...$e has a dark, @rred@w aura and menacing presence.\r\n", ctx)
  end
  if ch:condition_has("hayasa") then
    t[#t+1] = act.render_for(viewer, "...$e has a soft @cblue@w glow around $s body!\r\n", ctx)
  end
  if ch:aff_flagged(AF.BLIND) then
    t[#t+1] = act.render_for(viewer, "...$e is groping around blindly!\r\n", ctx)
  end
  local feat = ch:feature_get()
  if feat then
    t[#t+1] = act.render_for(viewer, "@C" .. feat .. "@n\r\n", ctx)
  end
end

-- Stacking key for identical idle NPCs in a room listing.
-- Returns nil for players, fighting NPCs, or NPCs not at full health.
local function stack_key(ch)
  if not ch:is_npc() then return nil end
  if ch:is_fighting() then return nil end
  if ch:meter_current("powerlevel") ~= ch:meter_max("powerlevel") then return nil end
  local AF = C().AF
  local flags = (ch:aff_flagged(AF.INVISIBLE) and 1  or 0)
              + (ch:aff_flagged(AF.HIDE)       and 2  or 0)
              + (ch:aff_flagged(AF.SANCTUARY)  and 4  or 0)
              + (ch:aff_flagged(AF.FIRESHIELD) and 8  or 0)
              + (ch:aff_flagged(AF.BLIND)      and 16 or 0)
              + (ch:aff_flagged(AF.HAYASA)     and 32 or 0)
              + (ch:aff_flagged(AF.ETHEREAL)   and 64 or 0)
  return table.concat({
    tostring(ch:proto_id_get()),
    tostring(ch:position_get()),
    tostring(flags),
    (ch:name_get() or ""):lower(),
  }, "\0")
end

-- Render one character's room-listing line as seen by viewer.
-- Returns a string (may be multi-line; trailing \r\n always present).
-- Mirrors list_one_char() in act.informative.cpp.
local function render_room_line(ch, viewer)
  local c   = C()
  local act = get_act()
  local AF, PLR, PRF = c.AF, c.PLR, c.PRF
  local ctx = {actor = ch}
  local t   = {}

  -- ROOMFLAGS vnum prefix (viewer pref, NPC targets only)
  if not viewer:is_npc() and viewer:pref_flagged(PRF.ROOMFLAGS) and ch:is_npc() then
    t[#t+1] = string.format("@D[@G%d@D]@w ", ch:proto_id_get())
  end

  -- PATH 1: NPC with long_descr at default position, not fighting
  if ch:is_npc() and ch:long_description_get()
      and ch:position_get() == ch:default_position_get()
      and not ch:is_fighting() then
    t[#t+1] = ch:long_description_get()
    local health = ch:meter_current("powerlevel") / 1000000.0
    if     health <= 0.1 then t[#t+1] = act.render_for(viewer, "@R...Should be DEAD soon.@w\r\n", ctx)
    elseif health <= 0.2 then t[#t+1] = act.render_for(viewer, "@R...Is on $s last leg.@w\r\n", ctx)
    elseif health <= 0.3 then t[#t+1] = act.render_for(viewer, "@R...Is absolutely covered in wounds.@w\r\n", ctx)
    elseif health <= 0.4 then t[#t+1] = act.render_for(viewer, "@R...$s body is in terrible shape.@w\r\n", ctx)
    elseif health <= 0.5 then t[#t+1] = act.render_for(viewer, "@R...Blood is seeping from the wounds on $s body.@w\r\n", ctx)
    elseif health <= 0.6 then t[#t+1] = act.render_for(viewer, "@R...Horrible wounds on $s body.@w\r\n", ctx)
    elseif health <= 0.7 then t[#t+1] = act.render_for(viewer, "@R...Quite a few wounds on $s body.@w\r\n", ctx)
    elseif health <= 0.8 then t[#t+1] = act.render_for(viewer, "@R...Many wounds on $s body.@w\r\n", ctx)
    elseif health <= 0.9 then t[#t+1] = act.render_for(viewer, "@R...A few wounds on $s body.@w\r\n", ctx)
    elseif health  < 1.0 then t[#t+1] = act.render_for(viewer, "@R...Some slight wounds on $s body.@w\r\n", ctx)
    end
    if ch:eavesdrop_get() > 0 then
      local dir = c.dirs[ch:eavesdrop_dir_get() + 1] or "?"
      t[#t+1] = act.render_for(viewer, "@w...$e is spying on everything to the @c" .. dir .. "@w.\r\n", ctx)
    end
    append_npc_path1_effects(t, ch, viewer, ctx, c)
    return table.concat(t)
  end

  -- PATH 2: NPC (not on path 1)
  if ch:is_npc() then
    local short = ch:short_description_get() or ""
    local head  = "@w" .. short:sub(1,1):upper() .. short:sub(2)
    local grappled  = ch:grappled_get()
    local absorbby  = ch:absorbed_by_get()
    local fighting  = ch:fighting_get()
    local pos       = ch:position_get()

    if grappled and grappled:id_get() == viewer:id_get() then
      t[#t+1] = head .. " is being grappled with by YOU!"
    elseif grappled then
      -- C++ copy-paste: says "absorbed from" but it's actually the grappled case
      t[#t+1] = head .. " is being absorbed from by " .. combat_name_for(viewer, grappled) .. "!"
    elseif absorbby and absorbby:id_get() == viewer:id_get() then
      t[#t+1] = head .. " is being absorbed from by YOU!"
    elseif absorbby then
      t[#t+1] = head .. " is being absorbed from by " .. combat_name_for(viewer, absorbby) .. "!"
    elseif fighting and fighting:id_get() ~= viewer:id_get() and pos ~= 6 and pos ~= 4 then
      t[#t+1] = head .. " is fighting " .. combat_name_for(viewer, fighting) .. "!"
    elseif fighting and fighting:id_get() == viewer:id_get() and pos ~= 6 and pos ~= 4 then
      t[#t+1] = head .. " is fighting YOU!"
    elseif fighting and pos == 6 then
      t[#t+1] = head .. " is sitting here."
    elseif fighting and pos == 4 then
      t[#t+1] = head .. " is sleeping here."
    else
      t[#t+1] = head
    end

    -- Invisible/hide/position for non-fighting NPCs
    if not fighting then
      if ch:aff_flagged(AF.INVISIBLE) then t[#t+1] = ", is invisible" end
      if ch:aff_flagged(AF.ETHEREAL)  then t[#t+1] = ", has a halo" end
      if ch:aff_flagged(AF.HIDE) and ch:id_get() ~= viewer:id_get() then
        t[#t+1] = ", is hiding"
      end
      t[#t+1] = "@w" .. (ROOM_POSITIONS[pos + 1] or "")
    end
    t[#t+1] = "@n\r\n"

    if ch:eavesdrop_get() > 0 then
      local dir = c.dirs[ch:eavesdrop_dir_get() + 1] or "?"
      t[#t+1] = act.render_for(viewer, "@w...$e is spying on everything to the @c" .. dir .. "@w.\r\n", ctx)
    end
    append_effects(t, ch, viewer, ctx, c)
    return table.concat(t)
  end

  -- PATH 3: Player
  -- Liquefied majin: special blob display
  if ch:race_get() == "majin" and ch:aff_flagged(AF.LIQUEFIED) then
    local skin_name = c.skin[ch:skin_get() + 1] or "colorless"
    return "@wSeveral blobs of " .. skin_name .. " colored goo spread out here.@n\r\n"
  end

  -- Display name (intro-aware, handles admin/disguise)
  t[#t+1] = "@w" .. display_name_for(ch, viewer)

  -- State indicators (comma-separated, tracks whether any were added)
  local had_state = false
  local function state(str)
    t[#t+1] = str
    had_state = true
  end

  local fighting = ch:fighting_get()
  -- Invisible / hide / etc. (only when not fighting, same as C++)
  if not fighting then
    if ch:aff_flagged(AF.INVISIBLE) then state(", is invisible") end
    if ch:aff_flagged(AF.ETHEREAL)  then state(", has a halo") end
    if ch:aff_flagged(AF.HIDE) and ch:id_get() ~= viewer:id_get() then
      state(", is hiding")
    end
    if not ch:has_connection()         then state(", has a blank stare") end
    if ch:player_flagged(PLR.WRITING)  then state(", is writing") end
    if ch:pref_flagged(PRF.BUILDWALK)  then state(", is buildwalking") end

    local absorbing = ch:absorbing_get()
    local grappling = ch:grappling_get()
    local carrying  = ch:carrying_char_get()
    local carried   = ch:carried_by_char_get()

    if absorbing and absorbing:id_get() ~= viewer:id_get() then
      state(", is absorbing from " .. absorbing:name_get())
    end
    if grappling and grappling:id_get() ~= viewer:id_get() then
      state(", is grappling with " .. combat_name_for(viewer, grappling))
    end
    if carrying and carrying:id_get() ~= viewer:id_get() then
      state(", is carrying " .. combat_name_for(viewer, carrying))
    end
    if carried and carried:id_get() ~= viewer:id_get() then
      state(", is being carried by " .. combat_name_for(viewer, carried))
    end
    if grappling and grappling:id_get() == viewer:id_get() then
      state(", is grappling with YOU")
    end
    if absorbing and absorbing:id_get() == viewer:id_get() then
      state(", is absorbing from YOU")
    end
    -- viewer's pointers targeting ch
    local v_absorbing = viewer:absorbing_get()
    if v_absorbing and v_absorbing:id_get() == ch:id_get() then
      state(", is being absorbed from by YOU")
    end
    local v_grappling = viewer:grappling_get()
    if v_grappling and v_grappling:id_get() == ch:id_get() then
      state(", is being grappled with by YOU")
    end
    local v_carrying = viewer:carrying_char_get()
    if v_carrying and v_carrying:id_get() == ch:id_get() then
      state(", is being carried by you")
    end
  end

  -- Fighting state (player vs player, including viewer as target)
  if not viewer:is_npc() and not ch:is_npc() and fighting then
    local is_spar = ch:player_flagged(PLR.SPAR)
        and not viewer:is_npc() and fighting:player_flagged(PLR.SPAR)
    if is_spar then
      t[#t+1] = ", is here sparring "
    else
      t[#t+1] = ", is here fighting "
    end
    if fighting:id_get() == viewer:id_get() then
      t[#t+1] = "@rYOU@w"
    else
      local f_room = fighting:room_get()
      local c_room = ch:room_get()
      if f_room and c_room and f_room:id_get() == c_room:id_get() then
        if viewer:admin_level_get() > 0 then
          t[#t+1] = fighting:name_get()
        else
          t[#t+1] = combat_name_for(viewer, fighting)
        end
      else
        t[#t+1] = "someone who has already left!"
      end
    end
    had_state = true
  end

  -- Position / chair / piloting suffix
  local pos   = ch:position_get()
  local chair = ch:sits_get()
  if chair then
    local cname = chair:short_description_get() or "something"
    if ch:player_flagged(PLR.HEALT) then
      t[#t+1] = "@w is floating inside a healing tank."
    elseif had_state then
      t[#t+1] = ",@w and" .. (ROOM_POSITIONS[pos + 1] or "") .. " on " .. cname .. "."
    else
      t[#t+1] = "@w" .. (ROOM_POSITIONS[pos + 1] or "") .. " on " .. cname .. "."
    end
  elseif ch:player_flagged(PLR.PILOTING) then
    t[#t+1] = "@w, is sitting in the pilot's chair.\r\n"
  elseif not fighting then
    if had_state then
      t[#t+1] = "@w, and" .. (ROOM_POSITIONS[pos + 1] or "") .. "."
    else
      t[#t+1] = "@w" .. (ROOM_POSITIONS[pos + 1] or "") .. "."
    end
  end

  -- detect_align aura badge
  if viewer:aff_flagged(AF.DETECT_ALIGN) then
    local align = ch:stat_get("alignment")
    if align <= -50  then t[#t+1] = " (@rRed@[3] Aura)"
    elseif align >= 50 then t[#t+1] = " (@bBlue@[3] Aura)"
    end
  end

  -- AFK / IDLE badge
  if not ch:is_npc() and ch:pref_flagged(PRF.AFK) then
    t[#t+1] = " @D(@RAFK@D)"
  elseif not ch:is_npc() and ch:timer_get() > 3 then
    t[#t+1] = " @D(@RIDLE@D)"
  end

  t[#t+1] = "@n\r\n"

  -- Post-line effect rows
  if ch:eavesdrop_get() > 0 then
    local dir = c.dirs[ch:eavesdrop_dir_get() + 1] or "?"
    t[#t+1] = act.render_for(viewer, "@w...$e is spying on everything to the @c" .. dir .. "@w.\r\n", ctx)
  end
  append_effects(t, ch, viewer, ctx, c)

  return table.concat(t)
end

local function is_humanoid(ch)
  local race = dbat.characters.registry.races and dbat.characters.registry.races[ch:race_get()]
  return not race or race.is_humanoid ~= false
end

return {
  is_humanoid = is_humanoid,
  meter_is_full = meter_is_full,
  can_see = can_see,
  keywords_for = keywords_for,
  modifiers = modifiers,
  apparent_sex = apparent_sex,
  apparent_race = apparent_race,
  display_name_for = display_name_for,
  on_event = on_event,
  act = act,
  act_self = act_self,
  act_around = act_around,
  act_message = act_message,
  der_total = der_total,
  execute_command = execute_command,
  visible_commands = visible_commands,
  send_text = send_text,
  send_around = send_around,
  send_line = send_line,
  send_line_around = send_line_around,
  is_transformed = is_transformed,
  stack_key = stack_key,
  render_room_line = render_room_line,
  check_attack_offense = check_attack_offense,
  check_attack_defense = check_attack_defense,
  damage = damage,
  on_attacked = on_attacked,
  launch_attack = launch_attack,
  init_skill          = init_skill,
  acquire_room_target = acquire_room_target,
}
