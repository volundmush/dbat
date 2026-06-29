# Object Userdata API

Access: `dbat.objects.by_id(id)` → Object or nil  
Access: `dbat.objects.all()` → iterator of all objects

---

## Meta / Lifecycle

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `valid` | `obj:valid()` | bool | False if handle is stale |
| `is_same` | `obj:is_same(other)` | bool | Identity comparison |
| `reftype` | `obj:reftype()` | `"object"` | Useful for generic entity code |
| `extract` | `obj:extract()` | — | Remove from world |

---

## Identity / Descriptors

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `id_get` | `obj:id_get()` | integer | Unique runtime id |
| `vnum_get` | `obj:vnum_get()` | integer | |
| `vnum_set` | `obj:vnum_set(vnum)` | — | |
| `proto_id_get` | `obj:proto_id_get()` | integer | Prototype vnum |
| `proto_id_set` | `obj:proto_id_set(vnum)` | — | |
| `name_get` | `obj:name_get()` | string\|nil | Keyword list (e.g. `"sword long"`) |
| `name_set` | `obj:name_set(str)` | — | |
| `short_description_get` | `obj:short_description_get()` | string\|nil | Shown in room / inventory |
| `short_description_set` | `obj:short_description_set(str)` | — | |
| `description_get` | `obj:description_get()` | string\|nil | Shown when examined |
| `description_set` | `obj:description_set(str)` | — | |
| `action_description_get` | `obj:action_description_get()` | string\|nil | Used for certain item type messages |
| `action_description_set` | `obj:action_description_set(str)` | — | |

---

## Properties

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `type_get` | `obj:type_get()` | integer | See `dbat.consts.item_types` |
| `type_set` | `obj:type_set(n)` | — | |
| `level_get` | `obj:level_get()` | integer | |
| `level_set` | `obj:level_set(n)` | — | |
| `level_mod` | `obj:level_mod(delta)` | integer | |
| `value_get` | `obj:value_get(index)` | integer | `value[0..N]` — meaning varies by item type |
| `value_set` | `obj:value_set(index, value)` | — | |
| `value_mod` | `obj:value_mod(index, delta)` | integer | |
| `size_get` | `obj:size_get()` | integer | |
| `size_set` | `obj:size_set(n)` | — | |
| `size_mod` | `obj:size_mod(delta)` | integer | |
| `cost_get` | `obj:cost_get()` | integer | Gold value |
| `cost_set` | `obj:cost_set(n)` | — | |
| `cost_mod` | `obj:cost_mod(delta)` | integer | |
| `affect_location_get` | `obj:affect_location_get(index)` | integer | Object affect apply location |
| `affect_modifier_get` | `obj:affect_modifier_get(index)` | integer | Object affect modifier |
| `affect_specific_get` | `obj:affect_specific_get(index)` | integer | Object affect specific id |
| `affect_specific_name_get` | `obj:affect_specific_name_get(index)` | string\|nil | Skill name for skill-specific affects |
| `weight_get` | `obj:weight_get()` | integer | Own weight |
| `weight_contained_get` | `obj:weight_contained_get()` | integer | Weight of contents only |
| `weight_total_get` | `obj:weight_total_get()` | integer | Own + contents weight |
| `weight_set` | `obj:weight_set(n)` | — | |
| `weight_mod` | `obj:weight_mod(delta)` | integer | |
| `timer_get` | `obj:timer_get()` | integer | Decay timer (ticks until extracted) |
| `timer_set` | `obj:timer_set(n)` | — | |
| `timer_mod` | `obj:timer_mod(delta)` | integer | |

---

## Flags

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `wear_flagged` | `obj:wear_flagged(flag)` | bool | See `dbat.consts.item_wear_flags` |
| `wear_flag_set` | `obj:wear_flag_set(flag, bool)` | — | |
| `wear_flag_toggle` | `obj:wear_flag_toggle(flag)` | bool | |
| `extra_flagged` | `obj:extra_flagged(flag)` | bool | See `dbat.consts.item_extra_flags` |
| `extra_flag_set` | `obj:extra_flag_set(flag, bool)` | — | |
| `extra_flag_toggle` | `obj:extra_flag_toggle(flag)` | bool | |
| `broken_set` | `obj:broken_set(bool)` | — | Sets `ITEM_BROKEN`; setting true removes the item if worn |
| `aff_flagged` | `obj:aff_flagged(flag)` | bool | See `dbat.consts.aff_flags` |
| `aff_flag_set` | `obj:aff_flag_set(flag, bool)` | — | |
| `aff_flag_toggle` | `obj:aff_flag_toggle(flag)` | bool | |

---

## Location

`to_*` methods automatically remove the object from its current location first.

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `room_vnum_get` | `obj:room_vnum_get()` | integer | Vnum of room it's in |
| `room_vnum_set` | `obj:room_vnum_set(vnum)` | — | |
| `room_loaded_get` | `obj:room_loaded_get()` | integer | Room it was loaded in |
| `room_loaded_set` | `obj:room_loaded_set(vnum)` | — | |
| `from_room` | `obj:from_room()` | — | Remove from room |
| `to_room` | `obj:to_room(room)` | — | Place in room |
| `from_char` | `obj:from_char()` | — | Remove from character inventory |
| `to_char` | `obj:to_char(ch)` | — | Place in character inventory |
| `equip` | `obj:equip(ch, position)` | — | Equip on character at wear position |
| `carried_by_get` | `obj:carried_by_get()` | integer | Character id carrying it (0 if none) |
| `worn_by_get` | `obj:worn_by_get()` | integer | Character id wearing it (0 if none) |
| `worn_on_get` | `obj:worn_on_get()` | integer | Wear position (-1 if not worn) |
| `worn_on_set` | `obj:worn_on_set(position)` | — | |
| `in_obj_get` | `obj:in_obj_get()` | integer | Container object id (0 if none) |
| `sitting_get` | `obj:sitting_get()` | integer | Character id sitting on it (0 if none) |
| `room_get` | `obj:room_get()` | Room\|nil | Room the object is in |
| `post_type_get` | `obj:post_type_get()` | integer | 0 = normal, >0 = posted note |
| `is_posted` | `obj:is_posted()` | bool | True if posted_to target is set |
| `fellow_wall_has` | `obj:fellow_wall_has()` | bool | True if part of a Glacian Wall set |
| `foob_get` | `obj:foob_get()` | integer | Original food value (for eaten-check) |

---

## Inventory (as container)

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `inventory_count` | `obj:inventory_count([recursive])` | integer | |
| `inventory` | `obj:inventory()` | iterator | Iterator over contained objects |
| `inventory_get` | `obj:inventory_get(index)` | Object\|nil | Get item at index |

---

## Lua-side Methods (from `lua/objects/object.lua`)

These are merged into the Object metatable at load time:

| Method | Signature | Notes |
|--------|-----------|-------|
| `keywords_for` | `obj:keywords_for([viewer])` | Returns keyword list for search |
| `display_name_for` | `obj:display_name_for([viewer [, prefix]])` | Rendered display name |
| `stack_key` | `obj:stack_key()` | string\|nil — key for grouping identical objects in inventory display |
| `room_stack_key` | `obj:room_stack_key()` | string\|nil — like stack_key but also excludes occupied furniture and posted notes |
| `render_inventory_line` | `obj:render_inventory_line(viewer)` | SHOW_OBJ_SHORT line with trailing newline |
| `render_room_line` | `obj:render_room_line(viewer)` | SHOW_OBJ_LONG line with trailing \\r\\n, or nil to skip |
| `modifiers` | `obj:modifiers()` | Returns modifier list (used for furniture stat bonuses) |
| `on_mud_hour` | `obj:on_mud_hour()` | Hourly hook |
| `on_second` | `obj:on_second()` | Per-second hook |

---

## ObjectPrototype Userdata

Access: `dbat.obj_protos.by_id(vnum)` → ObjectPrototype or nil

| Method | Signature | Returns |
|--------|-----------|---------|
| `valid` | `proto:valid()` | bool |
| `reftype` | `proto:reftype()` | `"object_prototype"` |
| `vnum_get` | `proto:vnum_get()` | integer |
| `spawn` | `proto:spawn([room])` | Object\|nil |
