# Room and Zone Userdata API

---

## Room Userdata

Access: `dbat.rooms.by_id(vnum)` → Room or nil

### Meta

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `valid` | `room:valid()` | bool | |
| `is_same` | `room:is_same(other)` | bool | |
| `reftype` | `room:reftype()` | `"room"` | |

### Identity / Descriptors

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `id_get` | `room:id_get()` | integer | Same as vnum for rooms |
| `vnum_get` | `room:vnum_get()` | integer | |
| `name_get` | `room:name_get()` | string | |
| `name_set` | `room:name_set(str)` | — | |
| `description_get` | `room:description_get()` | string | |
| `description_set` | `room:description_set(str)` | — | |
| `zone_vnum_get` | `room:zone_vnum_get()` | integer | Parent zone vnum |

### Environment

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `sector_type_get` | `room:sector_type_get()` | integer | See `dbat.consts.sector_types` |
| `sector_type_set` | `room:sector_type_set(n)` | — | |
| `light_get` | `room:light_get()` | integer | |
| `light_set` | `room:light_set(n)` | — | |
| `damage_get` | `room:damage_get()` | integer | Environmental damage value |
| `damage_set` | `room:damage_set(n)` | — | |
| `gravity_get` | `room:gravity_get()` | integer | Gravity multiplier |
| `gravity_set` | `room:gravity_set(n)` | — | |
| `geffect_get` | `room:geffect_get()` | integer | Global effect id; positive lava states attach `geo_effect` |
| `geffect_set` | `room:geffect_set(n)` | — | Adds/removes the `geo_effect` room script as needed |
| `is_dark` | `room:is_dark()` | bool | |
| `is_sunken` | `room:is_sunken()` | bool | Underwater room |
| `cook_element` | `room:cook_element()` | integer | Cooking element type (0 if none) |

### Flags

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `flagged` | `room:flagged(flag)` | bool | See `dbat.consts.room_flags` |
| `flag_set` | `room:flag_set(flag, bool)` | — | |
| `flag_toggle` | `room:flag_toggle(flag)` | bool | |

### Contents

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `contents` | `room:contents()` | iterator | Iterates over objects in room |
| `contents_get` | `room:contents_get()` | iterator | Alias for `contents` |
| `people` | `room:people()` | iterator | Iterates over characters in room |
| `people_get` | `room:people_get()` | iterator | Alias for `people` |

### Lua-side Methods (from `lua/rooms/room.lua`)

These are merged into the Room metatable at load time:

| Method | Signature | Notes |
|--------|-----------|-------|
| `send_text` | `room:send_text(msg [, ...])` | Send to all people in room; optional `string.format` |
| `send_line` | `room:send_line(msg [, ...])` | Same, but ensures `\r\n` tail |
| `modifiers` | `room:modifiers()` | Returns modifier list (regen rooms, bedrooms, aura rooms, cook elements) |
| `on_event` | `room:on_event(kind)` | Entity event dispatch hook; routes by subsystem prefix in `kind` |
| `event_schedule` | `room:event_schedule(kind, delay_ms [, interval_ms])` | Schedule an entity-update event; returns event id |
| `event_cancel` | `room:event_cancel(kind)` | Cancel all events matching `kind`; returns count cancelled |
| `event_count` | `room:event_count([kind])` | Count pending events (all if `kind` omitted) |
| `event_remaining_ms` | `room:event_remaining_ms([kind])` | ms until soonest matching event, or -1 if none |

---

## Zone Userdata

Access: `dbat.zones.by_id(vnum)` → Zone or nil

| Method | Signature | Returns | Notes |
|--------|-----------|---------|-------|
| `valid` | `zone:valid()` | bool | |
| `is_same` | `zone:is_same(other)` | bool | |
| `id_get` | `zone:id_get()` | integer | |
| `vnum_get` | `zone:vnum_get()` | integer | Alias for `id_get` |
| `name_get` | `zone:name_get()` | string\|nil | |
| `send_text` | `zone:send_text(text)` | — | Send to all characters in zone |
| `event_schedule` | `zone:event_schedule(kind, delay_ms [, interval_ms])` | integer | Returns event id |
| `event_cancel` | `zone:event_cancel(kind)` | integer | Count cancelled |
| `event_count` | `zone:event_count([kind])` | integer | |
| `event_remaining_ms` | `zone:event_remaining_ms([kind])` | integer | ms to next, or -1 |
