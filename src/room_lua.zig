const std = @import("std");
const zlua = @import("zlua");
const cdb = @import("cdb");
const characters_lua = @import("character_lua.zig");
const objects_lua = @import("object_lua.zig");
const lua_meta = @import("lua_meta.zig");
const room_api = @import("room_api.zig");

const Lua = zlua.Lua;
const room_metatable = "dbat.Room";
const room_script_metatable = "dbat.RoomScript";
const exit_metatable = "dbat.Exit";

const ExitHandle = extern struct {
    room_vnum: cdb.room_vnum,
    dir: i32,
};

extern fn event_schedule_lua_room_update(fire_at: i64, interval: i64, kind: ?[*:0]const u8, room_id: cdb.room_vnum) u64;
extern fn eq_cancel_owner(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn eq_owner_count(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn eq_owner_next_ms(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn event_queue_now_ms() i64;

const RoomHandle = extern struct {
    vnum: cdb.room_vnum,
};

const RoomScriptHandle = extern struct {
    room_vnum: cdb.room_vnum,
    script: [64:0]u8,
};

pub fn register(lua: *Lua) void {
    registerRoomMetatable(lua);
    registerRoomScriptMetatable(lua);
    registerExitMetatable(lua);

    lua.newTable();
    lua.pushFunction(zlua.wrap(luaRoomById));
    lua.setField(-2, "by_id");
    lua.setField(-2, "rooms");
}

fn luaRoomById(lua: *Lua) i32 {
    const vnum = lua.toInteger(1) catch {
        lua.pushNil();
        return 1;
    };
    const room_vnum = std.math.cast(cdb.room_vnum, vnum) orelse {
        lua.pushNil();
        return 1;
    };

    if (cdb.room_by_id(room_vnum) == null) {
        lua.pushNil();
        return 1;
    }

    pushRoom(lua, room_vnum);
    return 1;
}

fn registerRoomMetatable(lua: *Lua) void {
    lua.newMetatable(room_metatable) catch {
        lua.pop(1);
        return;
    };

    lua.pushValue(-1);
    lua.setField(-2, "__index");

    lua.pushFunction(zlua.wrap(luaRoomToString));
    lua.setField(-2, "__tostring");

    lua.pushFunction(zlua.wrap(luaRoomRefType));
    lua.setField(-2, "reftype");

    lua.pushFunction(zlua.wrap(luaRoomValid));
    lua.setField(-2, "valid");
    lua.pushFunction(zlua.wrap(luaRoomIsSame));
    lua.setField(-2, "is_same");
    lua.pushFunction(zlua.wrap(luaRoomIsSame));
    lua.setField(-2, "__eq");
    lua.pushFunction(zlua.wrap(luaRoomIsDark));
    lua.setField(-2, "is_dark");
    lua.pushFunction(zlua.wrap(luaRoomIsSunken));
    lua.setField(-2, "is_sunken");
    lua.pushFunction(zlua.wrap(luaRoomFlagged));
    lua.setField(-2, "flagged");
    lua.pushFunction(zlua.wrap(luaRoomFlagSet));
    lua.setField(-2, "flag_set");
    lua.pushFunction(zlua.wrap(luaRoomFlagToggle));
    lua.setField(-2, "flag_toggle");
    lua.pushFunction(zlua.wrap(luaRoomCookElement));
    lua.setField(-2, "cook_element");
    lua.pushFunction(zlua.wrap(luaRoomIdGet));
    lua.setField(-2, "id_get");
    lua.pushFunction(zlua.wrap(luaRoomVnumGet));
    lua.setField(-2, "vnum_get");
    lua.pushFunction(zlua.wrap(luaRoomNameGet));
    lua.setField(-2, "name_get");
    lua.pushFunction(zlua.wrap(luaRoomNameSet));
    lua.setField(-2, "name_set");
    lua.pushFunction(zlua.wrap(luaRoomDescriptionGet));
    lua.setField(-2, "description_get");
    lua.pushFunction(zlua.wrap(luaRoomDescriptionSet));
    lua.setField(-2, "description_set");
    lua.pushFunction(zlua.wrap(luaRoomSectorTypeGet));
    lua.setField(-2, "sector_type_get");
    lua.pushFunction(zlua.wrap(luaRoomSectorTypeSet));
    lua.setField(-2, "sector_type_set");
    lua.pushFunction(zlua.wrap(luaRoomZoneVnumGet));
    lua.setField(-2, "zone_vnum_get");
    lua.pushFunction(zlua.wrap(luaRoomLightGet));
    lua.setField(-2, "light_get");
    lua.pushFunction(zlua.wrap(luaRoomLightSet));
    lua.setField(-2, "light_set");
    lua.pushFunction(zlua.wrap(luaRoomLightMod));
    lua.setField(-2, "light_mod");
    lua.pushFunction(zlua.wrap(luaRoomDamageGet));
    lua.setField(-2, "damage_get");
    lua.pushFunction(zlua.wrap(luaRoomDamageSet));
    lua.setField(-2, "damage_set");
    lua.pushFunction(zlua.wrap(luaRoomDamageMod));
    lua.setField(-2, "damage_mod");
    lua.pushFunction(zlua.wrap(luaRoomGravityGet));
    lua.setField(-2, "gravity_get");
    lua.pushFunction(zlua.wrap(luaRoomGravitySet));
    lua.setField(-2, "gravity_set");
    lua.pushFunction(zlua.wrap(luaRoomGeffectGet));
    lua.setField(-2, "geffect_get");
    lua.pushFunction(zlua.wrap(luaRoomGeffectSet));
    lua.setField(-2, "geffect_set");
    lua.pushFunction(zlua.wrap(luaRoomContentsGet));
    lua.setField(-2, "contents_get");
    lua.pushFunction(zlua.wrap(luaRoomContentsGet));
    lua.setField(-2, "contents");
    lua.pushFunction(zlua.wrap(luaRoomPeopleGet));
    lua.setField(-2, "people_get");
    lua.pushFunction(zlua.wrap(luaRoomPeopleGet));
    lua.setField(-2, "people");
    lua.pushFunction(zlua.wrap(luaRoomEventSchedule));
    lua.setField(-2, "event_schedule");
    lua.pushFunction(zlua.wrap(luaRoomEventCancel));
    lua.setField(-2, "event_cancel");
    lua.pushFunction(zlua.wrap(luaRoomEventCount));
    lua.setField(-2, "event_count");
    lua.pushFunction(zlua.wrap(luaRoomEventRemainingMs));
    lua.setField(-2, "event_remaining_ms");
    lua.pushFunction(zlua.wrap(luaRoomExitGet));
    lua.setField(-2, "exit_get");

    lua_meta.mergeMethods(lua, "lua.rooms.room");
    lua.pushFunction(zlua.wrap(luaRoomScriptAdd)); lua.setField(-2, "script_add");
    lua.pushFunction(zlua.wrap(luaRoomScriptRemove)); lua.setField(-2, "script_remove");
    lua.pushFunction(zlua.wrap(luaRoomScriptHas)); lua.setField(-2, "script_has");
    lua.pushFunction(zlua.wrap(luaRoomScript)); lua.setField(-2, "script");
    lua.pushFunction(zlua.wrap(luaRoomScripts)); lua.setField(-2, "scripts");
    lua.pushFunction(zlua.wrap(luaRoomScriptNumberGet)); lua.setField(-2, "script_number_get");
    lua.pushFunction(zlua.wrap(luaRoomScriptNumberSet)); lua.setField(-2, "script_number_set");
    lua.pushFunction(zlua.wrap(luaRoomScriptTextGet)); lua.setField(-2, "script_text_get");
    lua.pushFunction(zlua.wrap(luaRoomScriptTextSet)); lua.setField(-2, "script_text_set");

    lua.pop(1);
}

fn registerRoomScriptMetatable(lua: *Lua) void {
    lua.newMetatable(room_script_metatable) catch {
        lua.pop(1);
        return;
    };
    lua.pushValue(-1);
    lua.setField(-2, "__index");
    lua.pushFunction(zlua.wrap(luaRoomScriptIdFn)); lua.setField(-2, "id");
    lua.pushFunction(zlua.wrap(luaRoomScriptNumberGetFn)); lua.setField(-2, "number_get");
    lua.pushFunction(zlua.wrap(luaRoomScriptNumberSetFn)); lua.setField(-2, "number_set");
    lua.pushFunction(zlua.wrap(luaRoomScriptNumberModFn)); lua.setField(-2, "number_mod");
    lua.pushFunction(zlua.wrap(luaRoomScriptTextGetFn)); lua.setField(-2, "text_get");
    lua.pushFunction(zlua.wrap(luaRoomScriptTextSetFn)); lua.setField(-2, "text_set");
    lua.pushFunction(zlua.wrap(luaRoomScriptScheduleEventFn)); lua.setField(-2, "schedule_event");
    lua.pushFunction(zlua.wrap(luaRoomScriptCancelEventFn)); lua.setField(-2, "cancel_event");
    lua.pushFunction(zlua.wrap(luaRoomScriptEventPendingFn)); lua.setField(-2, "event_pending");
    lua.pushFunction(zlua.wrap(luaRoomScriptEventNextMsFn)); lua.setField(-2, "event_next_ms");
    lua.pop(1);
}

pub fn pushRoom(lua: *Lua, vnum: cdb.room_vnum) void {
    const handle = lua.newUserdata(RoomHandle, 0);
    handle.* = .{ .vnum = vnum };
    _ = lua.getMetatableRegistry(room_metatable);
    lua.setMetatable(-2);
}

fn checkRoomHandle(lua: *Lua) *RoomHandle {
    return checkRoomHandleAt(lua, 1);
}

pub fn checkRoomHandleAt(lua: *Lua, index: i32) *RoomHandle {
    return lua.testUserdata(RoomHandle, index, room_metatable) catch {
        lua.raiseErrorStr("expected dbat.Room", .{});
    };
}

pub fn checkRoomAt(lua: *Lua, index: i32) *cdb.room_data {
    const handle = checkRoomHandleAt(lua, index);
    return cdb.room_by_id(handle.vnum) orelse {
        lua.raiseErrorStr("stale dbat.Room handle for room %d", .{handle.vnum});
    };
}

pub fn checkRoom(lua: *Lua) *cdb.room_data {
    return checkRoomAt(lua, 1);
}

fn luaRoomValid(lua: *Lua) i32 {
    const handle = checkRoomHandle(lua);
    lua.pushBoolean(cdb.room_by_id(handle.vnum) != null);
    return 1;
}

fn luaRoomIsSame(lua: *Lua) i32 {
    const left = checkRoomHandle(lua);
    const right = lua.testUserdata(RoomHandle, 2, room_metatable) catch {
        lua.pushBoolean(false);
        return 1;
    };
    lua.pushBoolean(left.vnum == right.vnum);
    return 1;
}

fn luaRoomIsDark(lua: *Lua) i32 {
    lua.pushBoolean(cdb.room_is_dark(checkRoom(lua)));
    return 1;
}

fn luaRoomIsSunken(lua: *Lua) i32 {
    lua.pushBoolean(cdb.room_is_sunken(checkRoom(lua)));
    return 1;
}

fn luaRoomFlagged(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const pos = lua.toInteger(2) catch lua.typeError(2, "integer");
    lua.pushBoolean(cdb.room_flagged(room, @intCast(pos)) != 0);
    return 1;
}

fn luaRoomFlagSet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const pos = lua.toInteger(2) catch lua.typeError(2, "integer");
    const value = lua.toBoolean(3);
    cdb.room_flag_set(room, @intCast(pos), value);
    return 0;
}

fn luaRoomFlagToggle(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const pos = lua.toInteger(2) catch lua.typeError(2, "integer");
    lua.pushBoolean(cdb.room_flag_toggle(room, @intCast(pos)));
    return 1;
}

fn luaRoomCookElement(lua: *Lua) i32 {
    lua.pushInteger(@intFromBool(cdb.cook_element(checkRoom(lua))));
    return 1;
}

fn luaRoomToString(lua: *Lua) i32 {
    const handle = checkRoomHandle(lua);
    if (cdb.room_by_id(handle.vnum)) |room| {
        _ = lua.pushFString("dbat.Room(%d, %s)", .{ handle.vnum, cdb.room_name_get(room) });
    } else {
        _ = lua.pushFString("dbat.Room(%d, stale)", .{handle.vnum});
    }
    return 1;
}

fn luaRoomRefType(lua: *Lua) i32 {
    _ = checkRoom(lua);
    _ = lua.pushString("room");
    return 1;
}

fn luaRoomIdGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.room_id_get(checkRoom(lua)));
    return 1;
}

fn luaRoomVnumGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.room_vnum_get(checkRoom(lua)));
    return 1;
}

fn luaRoomNameGet(lua: *Lua) i32 {
    _ = lua.pushString(std.mem.span(cdb.room_name_get(checkRoom(lua))));
    return 1;
}

fn luaRoomNameSet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const name = lua.toString(2) catch lua.typeError(2, "string");
    cdb.room_name_set(room, name);
    return 0;
}

fn luaRoomDescriptionGet(lua: *Lua) i32 {
    _ = lua.pushString(std.mem.span(cdb.room_description_get(checkRoom(lua))));
    return 1;
}

fn luaRoomDescriptionSet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const description = lua.toString(2) catch lua.typeError(2, "string");
    cdb.room_description_set(room, description);
    return 0;
}

fn luaRoomSectorTypeGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.room_sector_type_get(checkRoom(lua)));
    return 1;
}

fn luaRoomSectorTypeSet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const sector = lua.toInteger(2) catch lua.typeError(2, "integer");
    cdb.room_sector_type_set(room, @intCast(sector));
    return 0;
}

fn luaRoomZoneVnumGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.room_zone_vnum_get(checkRoom(lua)));
    return 1;
}

fn luaRoomLightGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.room_light_get(checkRoom(lua)));
    return 1;
}

fn luaRoomLightSet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const light = lua.toInteger(2) catch lua.typeError(2, "integer");
    const value = std.math.cast(u16, light) orelse lua.raiseErrorStr("room light out of range", .{});
    cdb.room_light_set(room, value);
    return 0;
}

fn luaRoomLightMod(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const delta = lua.toInteger(2) catch lua.typeError(2, "integer");
    cdb.room_light_mod(room, @intCast(delta));
    return 0;
}

fn luaRoomDamageGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.room_dmg_get(checkRoom(lua)));
    return 1;
}

fn luaRoomDamageSet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const damage = lua.toInteger(2) catch lua.typeError(2, "integer");
    cdb.room_dmg_set(room, @intCast(damage));
    return 0;
}

fn luaRoomDamageMod(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const delta = lua.toInteger(2) catch lua.typeError(2, "integer");
    cdb.room_dmg_mod(room, @intCast(delta));
    return 0;
}

fn luaRoomGravityGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.room_gravity_get(checkRoom(lua)));
    return 1;
}

fn luaRoomGravitySet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const gravity = lua.toInteger(2) catch lua.typeError(2, "integer");
    cdb.room_gravity_set(room, @intCast(gravity));
    return 0;
}

fn luaRoomGeffectGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.room_geffect_get(checkRoom(lua)));
    return 1;
}

fn luaRoomGeffectSet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const geffect = lua.toInteger(2) catch lua.typeError(2, "integer");
    cdb.room_geffect_set(room, @intCast(geffect));
    return 0;
}

fn luaRoomContentsGet(lua: *Lua) i32 {
    const room = checkRoom(lua);

    var count: usize = 0;
    const ids = cdb.room_objects_get(room, &count);
    defer if (ids) |_| std.c.free(@as(?*anyopaque, @ptrCast(ids)));

    lua.newTable();
    for (0..count) |i| {
        if (ids) |ptr| {
            objects_lua.pushObject(lua, ptr[i]);
            lua.setIndex(-2, @intCast(i + 1));
        }
    }
    return valueIterator(lua);
}

fn luaRoomPeopleGet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    lua.newTable();

    var count: usize = 0;
    const ids = cdb.room_person_ids(room, &count);
    var index: usize = 1;
    if (ids) |id_list| {
        defer cdb.room_person_ids_free(id_list);
        for (id_list[0..count]) |id| {
            const ch = cdb.char_by_id(id) orelse continue;
            characters_lua.pushCharacter(lua, cdb.char_id_get(ch));
            lua.setIndex(-2, @intCast(index));
            index += 1;
        }
    }

    return valueIterator(lua);
}

fn valueIterator(lua: *Lua) i32 {
    _ = lua.getGlobal("dbat");
    _ = lua.getField(-1, "_values");
    lua.remove(-2);
    lua.insert(-2);
    lua.protectedCall(.{ .args = 1, .results = 1 }) catch lua.raiseErrorStr("failed to create value iterator", .{});
    return 1;
}

fn luaRoomEventSchedule(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const kind = lua.toString(2) catch lua.typeError(2, "string");
    const delay_ms = lua.toInteger(3) catch lua.typeError(3, "integer");
    const interval_ms: i64 = if (lua.typeOf(4) == .number) @intCast(lua.toInteger(4) catch 0) else 0;
    const vnum = cdb.room_vnum_get(room);
    const now = event_queue_now_ms();
    const id = event_schedule_lua_room_update(now + delay_ms, interval_ms, kind.ptr, vnum);
    lua.pushInteger(@intCast(id));
    return 1;
}

fn luaRoomEventCancel(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const kind = lua.toString(2) catch lua.typeError(2, "string");
    const vnum = cdb.room_vnum_get(room);
    const n = eq_cancel_owner(@as(c_int, cdb.EQ_OWNER_ROOM), @as(i64, vnum), kind.ptr);
    lua.pushInteger(n);
    return 1;
}

fn luaRoomEventCount(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const kind: ?[:0]const u8 = if (lua.typeOf(2) == .string) (lua.toString(2) catch null) else null;
    const ptr: ?[*:0]const u8 = if (kind) |k| k.ptr else null;
    const vnum = cdb.room_vnum_get(room);
    const n = eq_owner_count(@as(c_int, cdb.EQ_OWNER_ROOM), @as(i64, vnum), ptr);
    lua.pushInteger(n);
    return 1;
}

fn luaRoomEventRemainingMs(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const kind: ?[:0]const u8 = if (lua.typeOf(2) == .string) (lua.toString(2) catch null) else null;
    const ptr: ?[*:0]const u8 = if (kind) |k| k.ptr else null;
    const vnum = cdb.room_vnum_get(room);
    const ms = eq_owner_next_ms(@as(c_int, cdb.EQ_OWNER_ROOM), @as(i64, vnum), ptr);
    lua.pushInteger(ms);
    return 1;
}

// ---- RoomScript handle ----

pub fn pushRoomScript(lua: *Lua, vnum: cdb.room_vnum, script_id: []const u8) void {
    const handle = lua.newUserdata(RoomScriptHandle, 0);
    handle.room_vnum = vnum;
    handle.script = std.mem.zeroes([64:0]u8);
    const len = @min(script_id.len, handle.script.len - 1);
    @memcpy(handle.script[0..len], script_id[0..len]);
    _ = lua.getMetatableRegistry(room_script_metatable);
    lua.setMetatable(-2);
}

fn checkRoomScriptHandle(lua: *Lua) *RoomScriptHandle {
    return lua.testUserdata(RoomScriptHandle, 1, room_script_metatable) catch {
        lua.raiseErrorStr("expected dbat.RoomScript", .{});
    };
}

fn roomScriptRoom(lua: *Lua, handle: *RoomScriptHandle) *cdb.room_data {
    return cdb.room_by_id(handle.room_vnum) orelse lua.raiseErrorStr("stale dbat.RoomScript room", .{});
}

fn roomScriptName(handle: *RoomScriptHandle) [*:0]const u8 {
    return @ptrCast(&handle.script);
}

fn roomScriptEventKind(buf: *[192:0]u8, script: []const u8, event: []const u8) ?[:0]u8 {
    const prefix = "script:";
    const total = prefix.len + script.len + 1 + event.len;
    if (total >= buf.len) return null;
    @memcpy(buf[0..prefix.len], prefix);
    @memcpy(buf[prefix.len..][0..script.len], script);
    buf[prefix.len + script.len] = ':';
    @memcpy(buf[prefix.len + script.len + 1 ..][0..event.len], event);
    buf[total] = 0;
    return buf[0..total :0];
}

fn luaRoomScriptIdFn(lua: *Lua) i32 {
    _ = lua.pushString(std.mem.span(roomScriptName(checkRoomScriptHandle(lua))));
    return 1;
}

fn luaRoomScriptNumberGetFn(lua: *Lua) i32 {
    const handle = checkRoomScriptHandle(lua);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    lua.pushInteger(cdb.room_script_number_get(roomScriptRoom(lua, handle), roomScriptName(handle), key.ptr));
    return 1;
}

fn luaRoomScriptNumberSetFn(lua: *Lua) i32 {
    const handle = checkRoomScriptHandle(lua);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    const val = lua.toInteger(3) catch lua.typeError(3, "integer");
    cdb.room_script_number_set(roomScriptRoom(lua, handle), roomScriptName(handle), key.ptr, @intCast(val));
    return 0;
}

fn luaRoomScriptNumberModFn(lua: *Lua) i32 {
    const handle = checkRoomScriptHandle(lua);
    const room = roomScriptRoom(lua, handle);
    const name = roomScriptName(handle);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    const delta: i64 = @intCast(lua.toInteger(3) catch lua.typeError(3, "integer"));
    const old = cdb.room_script_number_get(room, name, key.ptr);
    cdb.room_script_number_set(room, name, key.ptr, old + delta);
    lua.pushInteger(old + delta);
    return 1;
}

fn luaRoomScriptTextGetFn(lua: *Lua) i32 {
    const handle = checkRoomScriptHandle(lua);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    const val = cdb.room_script_text_get(roomScriptRoom(lua, handle), roomScriptName(handle), key.ptr);
    if (val == null) lua.pushNil() else _ = lua.pushString(std.mem.span(val));
    return 1;
}

fn luaRoomScriptTextSetFn(lua: *Lua) i32 {
    const handle = checkRoomScriptHandle(lua);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    const val = lua.toString(3) catch lua.typeError(3, "string");
    cdb.room_script_text_set(roomScriptRoom(lua, handle), roomScriptName(handle), key.ptr, val.ptr);
    return 0;
}

fn luaRoomScriptScheduleEventFn(lua: *Lua) i32 {
    const handle = checkRoomScriptHandle(lua);
    const room = roomScriptRoom(lua, handle);
    const event_name = lua.toString(2) catch lua.typeError(2, "string");
    const delay_ms: i64 = @intCast(lua.toInteger(3) catch lua.typeError(3, "integer"));
    const interval_ms: i64 = if (lua.typeOf(4) == .number) @intCast(lua.toInteger(4) catch 0) else 0;
    const script = std.mem.span(roomScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = roomScriptEventKind(&buf, script, event_name) orelse {
        lua.pushInteger(0);
        return 1;
    };
    const vnum = cdb.room_vnum_get(room);
    _ = eq_cancel_owner(@as(c_int, cdb.EQ_OWNER_ROOM), @as(i64, vnum), kind.ptr);
    const id = event_schedule_lua_room_update(event_queue_now_ms() + delay_ms, interval_ms, kind.ptr, vnum);
    lua.pushInteger(@intCast(id));
    return 1;
}

fn luaRoomScriptCancelEventFn(lua: *Lua) i32 {
    const handle = checkRoomScriptHandle(lua);
    const room = roomScriptRoom(lua, handle);
    const event_name = lua.toString(2) catch lua.typeError(2, "string");
    const script = std.mem.span(roomScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = roomScriptEventKind(&buf, script, event_name) orelse return 0;
    _ = eq_cancel_owner(@as(c_int, cdb.EQ_OWNER_ROOM), @as(i64, cdb.room_vnum_get(room)), kind.ptr);
    return 0;
}

fn luaRoomScriptEventPendingFn(lua: *Lua) i32 {
    const handle = checkRoomScriptHandle(lua);
    const room = roomScriptRoom(lua, handle);
    const event_name = lua.toString(2) catch lua.typeError(2, "string");
    const script = std.mem.span(roomScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = roomScriptEventKind(&buf, script, event_name) orelse {
        lua.pushBoolean(false);
        return 1;
    };
    lua.pushBoolean(eq_owner_count(@as(c_int, cdb.EQ_OWNER_ROOM), @as(i64, cdb.room_vnum_get(room)), kind.ptr) > 0);
    return 1;
}

fn luaRoomScriptEventNextMsFn(lua: *Lua) i32 {
    const handle = checkRoomScriptHandle(lua);
    const room = roomScriptRoom(lua, handle);
    const event_name = lua.toString(2) catch lua.typeError(2, "string");
    const script = std.mem.span(roomScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = roomScriptEventKind(&buf, script, event_name) orelse {
        lua.pushInteger(-1);
        return 1;
    };
    lua.pushInteger(eq_owner_next_ms(@as(c_int, cdb.EQ_OWNER_ROOM), @as(i64, cdb.room_vnum_get(room)), kind.ptr));
    return 1;
}

// ---- Room-level script entity methods ----

fn luaRoomScriptAdd(lua: *Lua) i32 {
    const key = lua.toString(2) catch lua.typeError(2, "string");
    lua.pushBoolean(cdb.room_script_add(checkRoom(lua), key.ptr));
    return 1;
}

fn luaRoomScriptRemove(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    const reason: [*:0]const u8 = if (lua.typeOf(3) == .string) blk: {
        const s = lua.toString(3) catch break :blk "removed";
        break :blk s.ptr;
    } else "removed";
    lua.pushBoolean(cdb.room_script_remove(room, key.ptr, reason));
    return 1;
}

fn luaRoomScriptHas(lua: *Lua) i32 {
    const key = lua.toString(2) catch lua.typeError(2, "string");
    lua.pushBoolean(cdb.room_script_has(checkRoom(lua), key.ptr));
    return 1;
}

fn luaRoomScript(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    if (!cdb.room_script_has(room, key.ptr)) {
        lua.pushNil();
        return 1;
    }
    pushRoomScript(lua, cdb.room_vnum_get(room), key);
    return 1;
}

fn luaRoomScripts(lua: *Lua) i32 {
    const room = checkRoom(lua);
    lua.newTable();
    var maybe_iter = room_api.roomScriptIterator(room);
    if (maybe_iter) |*iter| {
        var i: zlua.Integer = 1;
        while (iter.next()) |entry| {
            pushRoomScript(lua, cdb.room_vnum_get(room), entry.name);
            lua.setIndex(-2, i);
            i += 1;
        }
    }
    return valueIterator(lua);
}

fn luaRoomScriptNumberGet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const id = lua.toString(2) catch lua.typeError(2, "string");
    const key = lua.toString(3) catch lua.typeError(3, "string");
    lua.pushInteger(cdb.room_script_number_get(room, id.ptr, key.ptr));
    return 1;
}

fn luaRoomScriptNumberSet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const id = lua.toString(2) catch lua.typeError(2, "string");
    const key = lua.toString(3) catch lua.typeError(3, "string");
    const val: i64 = @intCast(lua.toInteger(4) catch lua.typeError(4, "integer"));
    cdb.room_script_number_set(room, id.ptr, key.ptr, val);
    return 0;
}

fn luaRoomScriptTextGet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const id = lua.toString(2) catch lua.typeError(2, "string");
    const key = lua.toString(3) catch lua.typeError(3, "string");
    const val = cdb.room_script_text_get(room, id.ptr, key.ptr);
    if (val == null) lua.pushNil() else _ = lua.pushString(std.mem.span(val));
    return 1;
}

fn luaRoomScriptTextSet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const id = lua.toString(2) catch lua.typeError(2, "string");
    const key = lua.toString(3) catch lua.typeError(3, "string");
    const val = lua.toString(4) catch lua.typeError(4, "string");
    cdb.room_script_text_set(room, id.ptr, key.ptr, val.ptr);
    return 0;
}

// --- Exit userdata ---

fn registerExitMetatable(lua: *Lua) void {
    lua.newMetatable(exit_metatable) catch {
        lua.pop(1);
        return;
    };
    lua.pushValue(-1);
    lua.setField(-2, "__index");
    lua.pushFunction(zlua.wrap(luaExitValid));       lua.setField(-2, "valid");
    lua.pushFunction(zlua.wrap(luaExitDestination)); lua.setField(-2, "destination");
    lua.pushFunction(zlua.wrap(luaExitFlagged));     lua.setField(-2, "flagged");
    lua.pushFunction(zlua.wrap(luaExitKeyword));     lua.setField(-2, "keyword");
    lua.pushFunction(zlua.wrap(luaExitKey));         lua.setField(-2, "key");
    lua.pop(1);
}

fn pushExit(lua: *Lua, room_vnum: cdb.room_vnum, dir: i32) void {
    const handle = lua.newUserdata(ExitHandle, 0);
    handle.* = .{ .room_vnum = room_vnum, .dir = dir };
    _ = lua.getMetatableRegistry(exit_metatable);
    lua.setMetatable(-2);
}

fn checkExitHandle(lua: *Lua) *ExitHandle {
    return lua.checkUserdata(ExitHandle, 1, exit_metatable);
}

fn getExitPtr(handle: *ExitHandle) ?*cdb.room_direction_data {
    const room = cdb.room_by_id(handle.room_vnum) orelse return null;
    return cdb.room_dir_option_get(room, handle.dir);
}

fn luaRoomExitGet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    const dir = lua.toInteger(2) catch lua.typeError(2, "integer");
    const exit = cdb.room_dir_option_get(room, @intCast(dir));
    if (exit == null) {
        lua.pushNil();
    } else {
        pushExit(lua, cdb.room_vnum_get(room), @intCast(dir));
    }
    return 1;
}

fn luaExitValid(lua: *Lua) i32 {
    const handle = checkExitHandle(lua);
    lua.pushBoolean(getExitPtr(handle) != null);
    return 1;
}

fn luaExitDestination(lua: *Lua) i32 {
    const handle = checkExitHandle(lua);
    const exit = getExitPtr(handle) orelse { lua.pushNil(); return 1; };
    const dest = cdb.exit_dest_get(exit);
    if (dest == null) {
        lua.pushNil();
    } else {
        pushRoom(lua, cdb.room_vnum_get(dest));
    }
    return 1;
}

fn luaExitFlagged(lua: *Lua) i32 {
    const handle = checkExitHandle(lua);
    const flag: c_int = @intCast(lua.toInteger(2) catch lua.typeError(2, "integer"));
    const exit = getExitPtr(handle) orelse { lua.pushBoolean(false); return 1; };
    lua.pushBoolean(cdb.exit_flagged(exit, flag));
    return 1;
}

fn luaExitKeyword(lua: *Lua) i32 {
    const handle = checkExitHandle(lua);
    const exit = getExitPtr(handle) orelse { _ = lua.pushString(""); return 1; };
    const kw = cdb.exit_keyword_get(exit);
    if (kw == null) _ = lua.pushString("") else _ = lua.pushString(std.mem.span(kw));
    return 1;
}

fn luaExitKey(lua: *Lua) i32 {
    const handle = checkExitHandle(lua);
    const exit = getExitPtr(handle) orelse { lua.pushInteger(-1); return 1; };
    lua.pushInteger(cdb.exit_key_get(exit));
    return 1;
}
