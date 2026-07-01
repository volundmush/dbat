# Character Userdata API

Access: `dbat.characters.by_id(id)` → Character or nil  
Access: `dbat.characters.all()` → iterator of all non-extracted Characters

---

## Identity / Descriptors

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `id_get` | `ch:id_get()` | integer | Unique runtime id |
| `vnum_get` | `ch:vnum_get()` | integer | Mob vnum (-1 for PCs) |
| `vnum_set` | `ch:vnum_set(vnum)` | — | |
| `proto_id_get` | `ch:proto_id_get()` | integer | Prototype vnum |
| `proto_id_set` | `ch:proto_id_set(vnum)` | — | |
| `name_get` | `ch:name_get()` | string\|nil | |
| `name_set` | `ch:name_set(name)` | — | |
| `short_description_get` | `ch:short_description_get()` | string\|nil | NPC one-liner |
| `short_description_set` | `ch:short_description_set(str)` | — | |
| `long_description_get` | `ch:long_description_get()` | string\|nil | NPC long desc |
| `long_description_set` | `ch:long_description_set(str)` | — | |
| `description_get` | `ch:description_get()` | string\|nil | Player look desc |
| `description_set` | `ch:description_set(str)` | — | |
| `title_get` | `ch:title_get()` | string\|nil | Player title |
| `title_set` | `ch:title_set(str)` | — | |
| `race_get` | `ch:race_get()` | string\|nil | Race slug id |
| `race_set` | `ch:race_set(id)` | — | Must be a valid race slug |
| `sensei_get` | `ch:sensei_get()` | string\|nil | Sensei slug id |
| `sensei_set` | `ch:sensei_set(id)` | — | Must be a valid sensei slug |
| `sex_get` | `ch:sex_get()` | `"neutral"\|"male"\|"female"` | |
| `sex_set` | `ch:sex_set(str)` | — | |
| `size_get` | `ch:size_get()` | integer | |
| `size_set` | `ch:size_set(n)` | — | |
| `size_mod` | `ch:size_mod(delta)` | integer (new value) | |
| `position_get` | `ch:position_get()` | integer | See `dbat.consts.positions` |
| `position_set` | `ch:position_set(n)` | — | |
| `starphase_get` | `ch:starphase_get()` | integer | Legacy Hoshijin phase field |
| `user_get` | `ch:user_get()` | string\|nil | Player account username |

---

## Meta / Lifecycle

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `valid` | `ch:valid()` | bool | False if handle is stale |
| `is_extracted` | `ch:is_extracted()` | bool | |
| `is_npc` | `ch:is_npc()` | bool | |
| `is_same` | `ch:is_same(other)` | bool | Identity comparison |
| `reftype` | `ch:reftype()` | `"character"` | Useful for generic entity code |
| `extract` | `ch:extract()` | — | Remove from world |
| `update` | `ch:update([kind], [seconds])` | — | Trigger condition updates |
| `absorbs_get` | `ch:absorbs_get()` | integer | Bio absorb counter |
| `absorbs_set` | `ch:absorbs_set(value)` | integer | |
| `absorbs_mod` | `ch:absorbs_mod(delta)` | integer | |
| `handle_ingest_learn` | `ch:handle_ingest_learn(victim)` | — | Legacy Majin ingest skill-learning routine |
| `limb_ok` | `ch:limb_ok(type)` | bool | Legacy limb availability check |
| `backstab_cooldown` | `ch:backstab_cooldown()` | integer | Seconds remaining on `cooldown_backstab` |
| `cooldown_get` | `ch:cooldown_get()` | integer | Seconds remaining on `cooldown_concentrate` |
| `cooldown_set` | `ch:cooldown_set(seconds)` | — | Set or clear `cooldown_concentrate` |
| `selfdestruct_cooldown_get` | `ch:selfdestruct_cooldown_get()` | integer | Seconds remaining on `cooldown_selfdestruct` |
| `selfdestruct_cooldown_set` | `ch:selfdestruct_cooldown_set(seconds)` | — | Set or clear `cooldown_selfdestruct` |

---

## Output

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `send` | `ch:send(text)` | — | Raw send to descriptor buffer (no color processing) |
| `send_raw` | `ch:send_raw(text)` | — | Send via `send_to_char`; color codes processed; no format support |
| `send_text` | `ch:send_text(msg [, ...])` | — | `send_raw(msg)` with optional `string.format(msg, ...)` (Lua-side) |
| `send_line` | `ch:send_line(msg [, ...])` | — | `send_text` + ensures `\r\n` tail |
| `send_around` | `ch:send_around(msg)` | — | Raw send to everyone in room except `ch` |
| `send_line_around` | `ch:send_line_around(msg [, ...])` | — | Formatted + `\r\n` to room except `ch` |
| `send_to_sense` | `ch:send_to_sense(sense_type, text)` | — | Send to characters with matching sense ability |
| `send_to_scouter` | `ch:send_to_scouter(text [, num [, type]])` | — | Broadcast to scouter users |

---

## Admin / Flags

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `admin_level_get` | `ch:admin_level_get()` | integer | |
| `admin_level_set` | `ch:admin_level_set(n)` | — | |
| `admin_level_mod` | `ch:admin_level_mod(delta)` | integer | |
| `admin_flagged` | `ch:admin_flagged(flag)` | bool | |
| `admin_flag_set` | `ch:admin_flag_set(flag, bool)` | — | |
| `admin_flag_toggle` | `ch:admin_flag_toggle(flag)` | bool (new state) | |
| `player_flagged` | `ch:player_flagged(flag)` | bool | |
| `player_flag_set` | `ch:player_flag_set(flag, bool)` | — | |
| `player_flag_toggle` | `ch:player_flag_toggle(flag)` | bool | |
| `is_shopkeeper` | `ch:is_shopkeeper()` | bool | True when NPC prototype uses the shopkeeper special proc |
| `pref_flagged` | `ch:pref_flagged(flag)` | bool | |
| `pref_flag_set` | `ch:pref_flag_set(flag, bool)` | — | |
| `pref_flag_toggle` | `ch:pref_flag_toggle(flag)` | bool | |

Flag constants: `dbat.consts.admin_flags`, `dbat.consts.player_flags`, `dbat.consts.prf_flags`

---

## Stats

Base stats are persistent integers stored by name. Derived stats are computed values (see `der_*`).

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `stat_get` | `ch:stat_get(name)` | integer | |
| `stat_set` | `ch:stat_set(name, value)` | integer (new) | |
| `stat_mod` | `ch:stat_mod(name, delta)` | integer (new) | |
| `perform_get_from_room` | `ch:perform_get_from_room(obj)` | bool | Runs normal get-from-room behavior for a specific room object |
| `item_legacy_command` | `ch:item_legacy_command(command, argument)` | — | Bridge for Lua item command ports that still use legacy item-side effects |
| `der_base` | `ch:der_base(name)` | integer | Base value before modifiers |
| `der_total` | `ch:der_total(name)` | integer | Final value after all modifiers (cached via Lua) |
| `der_invalidate` | `ch:der_invalidate()` | — | Force modifier cache rebuild |
| `modifiers_for` | `ch:modifiers_for(category, id)` | table | See below |
| `legacy_modifier` | `ch:legacy_modifier(location, specific)` | integer | Old affect-system modifier lookup |
| `modifier_gen` | `ch:modifier_gen()` | integer | Modifier generation counter (cache key) |

**`modifiers_for` return table:**
```lua
{
    flat        = integer,       -- additive bonus
    percent     = integer,       -- percent bonus (scaled ×10000)
    multipliers = {integer, …},  -- per-multiplier values (scaled ×10000)
    min         = integer|nil,   -- override_min value
    max         = integer|nil,   -- override_max value
    set         = integer|nil,   -- set-override value
}
```

---

## Meters

Meters are capped resource pools (powerlevel, ki, lifeforce, stamina). Each meter has a current value and a max derived from a derived stat.

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `meter_get` | `ch:meter_get(name)` | integer | Current value as percentage (×10000) |
| `meter_set` | `ch:meter_set(name, value)` | integer | |
| `meter_mod` | `ch:meter_mod(name, delta)` | integer | |
| `meter_set_int` | `ch:meter_set_int(name, current)` | integer | Set raw integer current |
| `meter_mod_int` | `ch:meter_mod_int(name, delta)` | integer | Modify raw integer current |
| `meter_current` | `ch:meter_current(name)` | integer | Raw current value |
| `meter_max` | `ch:meter_max(name)` | integer | Max (from derived stat) |

Meter definitions may provide `on_update(ch, old_value, new_value)`, called after `meter_set` changes the fixed-point current value.

---

## Skills

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `skill_base_get` | `ch:skill_base_get(name)` | integer | |
| `skill_base_set` | `ch:skill_base_set(name, value)` | integer | |
| `skill_base_mod` | `ch:skill_base_mod(name, delta)` | integer | |
| `skill_modifier_get` | `ch:skill_modifier_get(name)` | integer | Modifier bonus only |
| `skill_total_get` | `ch:skill_total_get(name)` | integer | base + modifier |
| `skill_get` | `ch:skill_get(name)` | integer | Alias for `skill_total_get` |
| `init_skill` | `ch:init_skill(name)` | integer | Legacy initialized skill roll/value |
| `skill_perf_get` | `ch:skill_perf_get(name)` | integer | Performance metric |
| `skill_perf_set` | `ch:skill_perf_set(name, value)` | integer | |
| `skill_perf_mod` | `ch:skill_perf_mod(name, delta)` | integer | |

---

## Conditions

Conditions are named status effects. They can store per-character number and string variables.

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `condition_has` | `ch:condition_has(id)` | bool | |
| `condition_has_tag` | `ch:condition_has_tag(tag)` | bool | True if any active condition has tag |
| `condition_active_with_tag` | `ch:condition_active_with_tag(tag)` | Condition\|nil | First condition with tag |
| `condition_add` | `ch:condition_add(id [, category [, source_id]])` | bool | Add without triggering on_apply |
| `condition_apply` | `ch:condition_apply(id [, category [, source_id]])` | bool | Add and trigger on_apply |
| `condition_apply_variables` | `ch:condition_apply_variables(id, nums, strs [, cat [, src]])` | bool | Apply with initial variables |
| `condition_apply_number` | `ch:condition_apply_number(id, key, value [, cat [, src]])` | bool | Apply with one initial number |
| `condition_apply_with_duration` | `ch:condition_apply_with_duration(id, duration [, cat [, src]])` | bool | Apply with duration |
| `condition_remove` | `ch:condition_remove(id [, reason])` | bool | |
| `condition_remove_tag` | `ch:condition_remove_tag(tag [, reason])` | integer | Count removed |
| `condition` | `ch:condition(id)` | Condition\|nil | Get Condition userdata |
| `conditions` | `ch:conditions()` | {string, …} | List of active condition ids |
| `condition_number_get` | `ch:condition_number_get(id, key)` | integer | |
| `condition_number_set` | `ch:condition_number_set(id, key, value)` | integer | |
| `condition_number_mod` | `ch:condition_number_mod(id, key, delta)` | integer | |
| `condition_string_get` | `ch:condition_string_get(id, key)` | string\|nil | |
| `condition_string_set` | `ch:condition_string_set(id, key, value)` | bool | |

### Condition Userdata

Obtained from `ch:condition(id)` or `ch:condition_active_with_tag(tag)`.

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `id` | `cond:id()` | string | |
| `stacks` | `cond:stacks()` | integer | |
| `stacks_set` | `cond:stacks_set(n)` | integer | |
| `duration` | `cond:duration()` | integer | Remaining seconds via event queue; -1 = permanent |
| `duration_set` | `cond:duration_set(n)` | integer | |
| `number_get` | `cond:number_get(key)` | integer | |
| `number_set` | `cond:number_set(key, value)` | integer | |
| `number_mod` | `cond:number_mod(key, delta)` | integer | |
| `string_get` | `cond:string_get(key)` | string\|nil | |
| `string_set` | `cond:string_set(key, value)` | bool | |
| `schedule_event` | `cond:schedule_event(name, delay_ms [, interval_ms])` | integer | Schedule a named event; 0 interval = one-shot |
| `cancel_event` | `cond:cancel_event(name)` | — | Cancel all events with this name |
| `event_pending` | `cond:event_pending(name)` | bool | True if event is scheduled |
| `event_next_ms` | `cond:event_next_ms(name)` | integer | Relative ms until next fire; -1 if not pending |
| `schedule_expire` | `cond:schedule_expire(secs)` | — | Schedule one-shot "expire" event; replaces existing |
| `remaining_ms` | `cond:remaining_ms()` | integer | ms until "expire" event; -1 if permanent |
| `remaining_secs` | `cond:remaining_secs()` | integer | seconds until "expire" event (floored); -1 if permanent |

---

## Transformations

Transformation state is tracked separately from conditions. A transformation may or may not have an associated condition.

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `transform_has` | `ch:transform_has(id)` | bool | True if currently in this form |
| `transform_add` | `ch:transform_add(id)` | bool | |
| `transform_remove` | `ch:transform_remove(id)` | bool | |
| `transform_unlocked` | `ch:transform_unlocked(id)` | bool | |
| `transform_unlock` | `ch:transform_unlock(id [, source])` | bool | |
| `transform_number_get` | `ch:transform_number_get(id, key)` | integer | |
| `transform_number_set` | `ch:transform_number_set(id, key, value)` | integer | |
| `transform_number_mod` | `ch:transform_number_mod(id, key, delta)` | integer | |
| `transform_string_get` | `ch:transform_string_get(id, key)` | string\|nil | |
| `transform_string_set` | `ch:transform_string_set(id, key, value)` | bool | |

---

## Location / Movement

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `room_vnum_get` | `ch:room_vnum_get()` | integer | |
| `room_vnum_set` | `ch:room_vnum_set(vnum)` | — | |
| `room_get` | `ch:room_get()` | Room\|nil | |
| `from_room` | `ch:from_room()` | — | Remove from current room |
| `to_room` | `ch:to_room(room)` | — | Move to room (calls from_room first) |
| `zone_vnum_get` | `ch:zone_vnum_get()` | integer | |
| `zone_get` | `ch:zone_get()` | Zone\|nil | |
| `is_outside` | `ch:is_outside()` | bool | |
| `sits_get` | `ch:sits_get()` | Object\|nil | Furniture ch is sitting on |
| `sits_set` | `ch:sits_set(obj\|nil)` | — | |

---

## Inventory / Equipment

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `inventory_count` | `ch:inventory_count([recursive])` | integer | |
| `equipment_count` | `ch:equipment_count([recursive])` | integer | |
| `inventory` | `ch:inventory()` | iterator | No arg = iterator over all carried items |
| `inventory_get` | `ch:inventory_get(index)` | Object\|nil | Get item at index |
| `equipment` | `ch:equipment()` | pairs iterator | No arg = `{[position]=Object}` pairs |
| `equipment_get` | `ch:equipment_get(index)` | Object\|nil | Get item at wear position |
| `unequip` | `ch:unequip(position)` | Object\|nil | Remove and return item |

---

## Visibility / Combat

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `can_see_in_dark` | `ch:can_see_in_dark()` | bool | |
| `can_see_char` | `ch:can_see_char(other)` | bool | |
| `can_see_obj` | `ch:can_see_obj(obj)` | bool | |
| `can_see` | `ch:can_see(entref)` | bool | Generic; accepts character or object (Lua-side) |
| `can_kill` | `ch:can_kill(other [, mode])` | bool | Legacy permission check; mode defaults to 1 |
| `reveal_hiding` | `ch:reveal_hiding([reveal_type])` | — | |
| `release_charge` | `ch:release_charge()` | bool | |
| `command_queue_clear` | `ch:command_queue_clear()` | — | |
| `fighting_get` | `ch:fighting_get()` | Character\|nil | Current fight target |
| `grappling_get` | `ch:grappling_get()` | Character\|nil | Who ch is grappling |
| `grappled_get` | `ch:grappled_get()` | Character\|nil | Who is grappling ch |
| `absorbing_get` | `ch:absorbing_get()` | Character\|nil | Who ch is absorbing from |
| `absorbed_by_get` | `ch:absorbed_by_get()` | Character\|nil | Who is absorbing from ch |
| `mindlinked_get` | `ch:mindlinked_get()` | Character\|nil | Who ch is mind linked with |
| `mindlinked_set` | `ch:mindlinked_set(vict_or_nil)` | — | Set or clear mind link condition |
| `linker_get` | `ch:linker_get()` | integer | Legacy mind-link owner flag |
| `linker_set` | `ch:linker_set(value)` | — | |
| `wimp_level_get` | `ch:wimp_level_get()` | integer | Auto-flee powerlevel threshold |
| `wimp_level_set` | `ch:wimp_level_set(value)` | — | |
| `timer_get` | `ch:timer_get()` | integer | Idle timer (>3 = idle) |
| `has_connection` | `ch:has_connection()` | bool | Has live descriptor (false = blank stare) |
| `default_position_get` | `ch:default_position_get()` | integer | NPC default position |
| `eavesdrop_get` | `ch:eavesdrop_get()` | integer | Eavesdrop target room (0 = none) |
| `eavesdrop_set` | `ch:eavesdrop_set(vnum)` | — | Set eavesdrop target room |
| `eavesdrop_dir_get` | `ch:eavesdrop_dir_get()` | integer | Eavesdrop direction index |
| `eavesdrop_dir_set` | `ch:eavesdrop_dir_set(dir)` | — | Set eavesdrop direction index |

---

## Lua-side Methods (from `lua/characters/character.lua`)

These are merged into the Character metatable at load time:

| Method | Signature | Notes |
|--------|-----------|-------|
| `can_see` | `ch:can_see(entref)` | Dispatches to `can_see_char` or `can_see_obj` by reftype |
| `keywords_for` | `ch:keywords_for([viewer])` | Returns keyword list for search |
| `modifiers` | `ch:modifiers()` | Collects all active modifiers from race/sensei/conditions/room; furniture is included through `using_furniture` |
| `apparent_sex` | `ch:apparent_sex([viewer])` | |
| `apparent_race` | `ch:apparent_race([viewer])` | |
| `display_name_for` | `ch:display_name_for([viewer])` | |
| `der_total` | `ch:der_total(name)` | Cached derived stat calculation with full modifier accumulation |
| `execute_command` | `ch:execute_command(input, cmd_class)` | Dispatch command against a command class |
| `visible_commands` | `ch:visible_commands(cmd_class)` | Filtered list of commands visible to ch |
| `send_around` | `ch:send_around(msg)` | Send to all in room except ch |
| `act_self` | `ch:act_self(msg [, ctx])` | Send rendered message to self |
| `act_around` | `ch:act_around(msg [, ctx])` | Send rendered message to room excluding self |
| `on_event` | `ch:on_event(kind)` | Entity event dispatch hook; routes by subsystem prefix in `kind` |
| `is_transformed` | `ch:is_transformed()` | Returns true if any TRANS1-6 or OOZARU player flag is set |
| `stack_key` | `ch:stack_key()` | string\|nil — key for grouping identical idle NPCs in room display |
| `render_room_line` | `ch:render_room_line(viewer)` | Render this character's room-listing line as seen by viewer |
| `event_schedule` | `ch:event_schedule(kind, delay_ms [, interval_ms])` | Schedule an entity-update event; returns event id |
| `event_cancel` | `ch:event_cancel(kind)` | Cancel all events matching `kind`; returns count cancelled |
| `event_count` | `ch:event_count([kind])` | Count pending events (all if `kind` omitted) |
| `event_remaining_ms` | `ch:event_remaining_ms([kind])` | ms until soonest matching event, or -1 if none |

---

## MobPrototype Userdata

Access: `dbat.mob_protos.by_id(vnum)` → MobPrototype or nil

| Method | Signature | Returns |
|--------|-----------|---------|
| `valid` | `proto:valid()` | bool |
| `reftype` | `proto:reftype()` | `"mob_prototype"` |
| `vnum_get` | `proto:vnum_get()` | integer |
| `spawn` | `proto:spawn([room])` | Character\|nil |
