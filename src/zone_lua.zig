const std = @import("std");
const zlua = @import("zlua");
const cdb = @import("cdb");
const lua_meta = @import("lua_meta.zig");
const zone_api = @import("zone_api.zig");

const Lua = zlua.Lua;
const zone_metatable = "dbat.Zone";
const zone_script_metatable = "dbat.ZoneScript";

extern fn event_schedule_lua_zone_update(fire_at: i64, interval: i64, kind: ?[*:0]const u8, zone_id: cdb.zone_vnum) u64;
extern fn eq_cancel_owner(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn eq_owner_count(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn eq_owner_next_ms(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn event_queue_now_ms() i64;

const ZoneHandle = extern struct {
    vnum: cdb.zone_vnum,
};

const ZoneScriptHandle = extern struct {
    zone_vnum: cdb.zone_vnum,
    script: [64:0]u8,
};

pub fn register(lua: *Lua) void {
    registerZoneMetatable(lua);
    registerZoneScriptMetatable(lua);

    lua.newTable();
    lua.pushFunction(zlua.wrap(luaZoneById));
    lua.setField(-2, "by_id");
    lua.setField(-2, "zones");
}

fn registerZoneMetatable(lua: *Lua) void {
    lua.newMetatable(zone_metatable) catch {
        lua.pop(1);
        return;
    };

    lua.pushValue(-1);
    lua.setField(-2, "__index");

    addMethod(lua, "__tostring", luaZoneToString);
    addMethod(lua, "valid", luaZoneValid);
    addMethod(lua, "is_same", luaZoneIsSame);
    addMethod(lua, "__eq", luaZoneIsSame);
    addMethod(lua, "id_get", luaZoneIdGet);
    addMethod(lua, "vnum_get", luaZoneIdGet);
    addMethod(lua, "name_get", luaZoneNameGet);
    addMethod(lua, "send_text", luaZoneSendText);
    addMethod(lua, "event_schedule", luaZoneEventSchedule);
    addMethod(lua, "event_cancel", luaZoneEventCancel);
    addMethod(lua, "event_count", luaZoneEventCount);
    addMethod(lua, "event_remaining_ms", luaZoneEventRemainingMs);
    addMethod(lua, "script_add", luaZoneScriptAdd);
    addMethod(lua, "script_remove", luaZoneScriptRemove);
    addMethod(lua, "script_has", luaZoneScriptHas);
    addMethod(lua, "script", luaZoneScript);
    addMethod(lua, "scripts", luaZoneScripts);
    addMethod(lua, "script_number_get", luaZoneScriptNumberGet);
    addMethod(lua, "script_number_set", luaZoneScriptNumberSet);
    addMethod(lua, "script_text_get", luaZoneScriptTextGet);
    addMethod(lua, "script_text_set", luaZoneScriptTextSet);

    lua_meta.mergeMethods(lua, "lua.zones.zone");

    lua.pop(1);
}

fn registerZoneScriptMetatable(lua: *Lua) void {
    lua.newMetatable(zone_script_metatable) catch {
        lua.pop(1);
        return;
    };
    lua.pushValue(-1);
    lua.setField(-2, "__index");
    addMethod(lua, "id", luaZoneScriptId);
    addMethod(lua, "number_get", luaZoneScriptNumberGet_);
    addMethod(lua, "number_set", luaZoneScriptNumberSet_);
    addMethod(lua, "number_mod", luaZoneScriptNumberMod_);
    addMethod(lua, "text_get", luaZoneScriptTextGet_);
    addMethod(lua, "text_set", luaZoneScriptTextSet_);
    addMethod(lua, "schedule_event", luaZoneScriptScheduleEvent);
    addMethod(lua, "cancel_event", luaZoneScriptCancelEvent);
    addMethod(lua, "event_pending", luaZoneScriptEventPending);
    addMethod(lua, "event_next_ms", luaZoneScriptEventNextMs);
    lua.pop(1);
}

fn addMethod(lua: *Lua, comptime name: [:0]const u8, comptime function: anytype) void {
    lua.pushFunction(zlua.wrap(function));
    lua.setField(-2, name);
}

pub fn pushZone(lua: *Lua, vnum: cdb.zone_vnum) void {
    const handle = lua.newUserdata(ZoneHandle, 0);
    handle.* = .{ .vnum = vnum };
    _ = lua.getMetatableRegistry(zone_metatable);
    lua.setMetatable(-2);
}

fn checkZoneHandle(lua: *Lua) *ZoneHandle {
    return lua.testUserdata(ZoneHandle, 1, zone_metatable) catch {
        lua.raiseErrorStr("expected dbat.Zone", .{});
    };
}

fn checkZone(lua: *Lua) *cdb.zone_data {
    const handle = checkZoneHandle(lua);
    return cdb.zone_by_id(handle.vnum) orelse {
        lua.raiseErrorStr("stale dbat.Zone handle for zone %d", .{handle.vnum});
    };
}

fn luaZoneById(lua: *Lua) i32 {
    const id = lua.toInteger(1) catch {
        lua.pushNil();
        return 1;
    };
    const vnum = std.math.cast(cdb.zone_vnum, id) orelse {
        lua.pushNil();
        return 1;
    };
    if (cdb.zone_by_id(vnum) == null) {
        lua.pushNil();
        return 1;
    }
    pushZone(lua, vnum);
    return 1;
}

fn luaZoneValid(lua: *Lua) i32 {
    const handle = checkZoneHandle(lua);
    lua.pushBoolean(cdb.zone_by_id(handle.vnum) != null);
    return 1;
}

fn luaZoneIsSame(lua: *Lua) i32 {
    const left = checkZoneHandle(lua);
    const right = lua.testUserdata(ZoneHandle, 2, zone_metatable) catch {
        lua.pushBoolean(false);
        return 1;
    };
    lua.pushBoolean(left.vnum == right.vnum);
    return 1;
}

fn luaZoneToString(lua: *Lua) i32 {
    const handle = checkZoneHandle(lua);
    if (cdb.zone_by_id(handle.vnum)) |zone| {
        _ = lua.pushFString("dbat.Zone(%d, %s)", .{ handle.vnum, cdb.zone_name_get(zone) });
    } else {
        _ = lua.pushFString("dbat.Zone(%d, stale)", .{handle.vnum});
    }
    return 1;
}

fn luaZoneIdGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.zone_id_get(checkZone(lua)));
    return 1;
}

fn luaZoneNameGet(lua: *Lua) i32 {
    const name = cdb.zone_name_get(checkZone(lua));
    if (name == null) {
        lua.pushNil();
    } else {
        _ = lua.pushString(std.mem.span(name));
    }
    return 1;
}

fn luaZoneSendText(lua: *Lua) i32 {
    const zone = checkZone(lua);
    const text = lua.toString(2) catch lua.typeError(2, "string");
    cdb.send_to_zone(@constCast(text.ptr), zone);
    return 0;
}

fn luaZoneEventSchedule(lua: *Lua) i32 {
    const zone = checkZone(lua);
    const kind = lua.toString(2) catch lua.typeError(2, "string");
    const delay_ms = lua.toInteger(3) catch lua.typeError(3, "integer");
    const interval_ms: i64 = if (lua.typeOf(4) == .number) @intCast(lua.toInteger(4) catch 0) else 0;
    const vnum = cdb.zone_id_get(zone);
    const now = event_queue_now_ms();
    const id = event_schedule_lua_zone_update(now + delay_ms, interval_ms, kind.ptr, @intCast(vnum));
    lua.pushInteger(@intCast(id));
    return 1;
}

fn luaZoneEventCancel(lua: *Lua) i32 {
    const zone = checkZone(lua);
    const kind = lua.toString(2) catch lua.typeError(2, "string");
    const vnum = cdb.zone_id_get(zone);
    const n = eq_cancel_owner(@as(c_int, cdb.EQ_OWNER_ZONE), vnum, kind.ptr);
    lua.pushInteger(n);
    return 1;
}

fn luaZoneEventCount(lua: *Lua) i32 {
    const zone = checkZone(lua);
    const kind: ?[:0]const u8 = if (lua.typeOf(2) == .string) (lua.toString(2) catch null) else null;
    const ptr: ?[*:0]const u8 = if (kind) |k| k.ptr else null;
    const vnum = cdb.zone_id_get(zone);
    const n = eq_owner_count(@as(c_int, cdb.EQ_OWNER_ZONE), vnum, ptr);
    lua.pushInteger(n);
    return 1;
}

fn luaZoneEventRemainingMs(lua: *Lua) i32 {
    const zone = checkZone(lua);
    const kind: ?[:0]const u8 = if (lua.typeOf(2) == .string) (lua.toString(2) catch null) else null;
    const ptr: ?[*:0]const u8 = if (kind) |k| k.ptr else null;
    const vnum = cdb.zone_id_get(zone);
    const ms = eq_owner_next_ms(@as(c_int, cdb.EQ_OWNER_ZONE), vnum, ptr);
    lua.pushInteger(ms);
    return 1;
}

fn valueIterator(lua: *Lua) i32 {
    _ = lua.getGlobal("dbat");
    _ = lua.getField(-1, "_values");
    lua.remove(-2);
    lua.insert(-2);
    lua.protectedCall(.{ .args = 1, .results = 1 }) catch lua.raiseErrorStr("failed to create value iterator", .{});
    return 1;
}

// ---- ZoneScript handle ----

pub fn pushZoneScript(lua: *Lua, vnum: cdb.zone_vnum, script_id: []const u8) void {
    const handle = lua.newUserdata(ZoneScriptHandle, 0);
    handle.zone_vnum = vnum;
    handle.script = std.mem.zeroes([64:0]u8);
    const len = @min(script_id.len, handle.script.len - 1);
    @memcpy(handle.script[0..len], script_id[0..len]);
    _ = lua.getMetatableRegistry(zone_script_metatable);
    lua.setMetatable(-2);
}

fn checkZoneScriptHandle(lua: *Lua) *ZoneScriptHandle {
    return lua.testUserdata(ZoneScriptHandle, 1, zone_script_metatable) catch {
        lua.raiseErrorStr("expected dbat.ZoneScript", .{});
    };
}

fn zoneScriptZone(lua: *Lua, handle: *ZoneScriptHandle) *cdb.zone_data {
    return cdb.zone_by_id(handle.zone_vnum) orelse lua.raiseErrorStr("stale dbat.ZoneScript zone", .{});
}

fn zoneScriptName(handle: *ZoneScriptHandle) [*:0]const u8 {
    return @ptrCast(&handle.script);
}

fn zoneScriptEventKind(buf: *[192:0]u8, script: []const u8, event: []const u8) ?[:0]u8 {
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

fn luaZoneScriptId(lua: *Lua) i32 {
    _ = lua.pushString(std.mem.span(zoneScriptName(checkZoneScriptHandle(lua))));
    return 1;
}

fn luaZoneScriptNumberGet_(lua: *Lua) i32 {
    const handle = checkZoneScriptHandle(lua);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    lua.pushInteger(cdb.zone_script_number_get(zoneScriptZone(lua, handle), zoneScriptName(handle), key.ptr));
    return 1;
}

fn luaZoneScriptNumberSet_(lua: *Lua) i32 {
    const handle = checkZoneScriptHandle(lua);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    const val: i64 = @intCast(lua.toInteger(3) catch lua.typeError(3, "integer"));
    cdb.zone_script_number_set(zoneScriptZone(lua, handle), zoneScriptName(handle), key.ptr, val);
    return 0;
}

fn luaZoneScriptNumberMod_(lua: *Lua) i32 {
    const handle = checkZoneScriptHandle(lua);
    const zone = zoneScriptZone(lua, handle);
    const name = zoneScriptName(handle);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    const delta: i64 = @intCast(lua.toInteger(3) catch lua.typeError(3, "integer"));
    const old = cdb.zone_script_number_get(zone, name, key.ptr);
    cdb.zone_script_number_set(zone, name, key.ptr, old + delta);
    lua.pushInteger(old + delta);
    return 1;
}

fn luaZoneScriptTextGet_(lua: *Lua) i32 {
    const handle = checkZoneScriptHandle(lua);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    const val = cdb.zone_script_text_get(zoneScriptZone(lua, handle), zoneScriptName(handle), key.ptr);
    if (val == null) lua.pushNil() else _ = lua.pushString(std.mem.span(val));
    return 1;
}

fn luaZoneScriptTextSet_(lua: *Lua) i32 {
    const handle = checkZoneScriptHandle(lua);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    const val = lua.toString(3) catch lua.typeError(3, "string");
    cdb.zone_script_text_set(zoneScriptZone(lua, handle), zoneScriptName(handle), key.ptr, val.ptr);
    return 0;
}

fn luaZoneScriptScheduleEvent(lua: *Lua) i32 {
    const handle = checkZoneScriptHandle(lua);
    const zone = zoneScriptZone(lua, handle);
    const event_name = lua.toString(2) catch lua.typeError(2, "string");
    const delay_ms: i64 = @intCast(lua.toInteger(3) catch lua.typeError(3, "integer"));
    const interval_ms: i64 = if (lua.typeOf(4) == .number) @intCast(lua.toInteger(4) catch 0) else 0;
    const script = std.mem.span(zoneScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = zoneScriptEventKind(&buf, script, event_name) orelse {
        lua.pushInteger(0);
        return 1;
    };
    const vnum = cdb.zone_id_get(zone);
    _ = eq_cancel_owner(@as(c_int, cdb.EQ_OWNER_ZONE), vnum, kind.ptr);
    const id = event_schedule_lua_zone_update(event_queue_now_ms() + delay_ms, interval_ms, kind.ptr, @intCast(vnum));
    lua.pushInteger(@intCast(id));
    return 1;
}

fn luaZoneScriptCancelEvent(lua: *Lua) i32 {
    const handle = checkZoneScriptHandle(lua);
    const zone = zoneScriptZone(lua, handle);
    const event_name = lua.toString(2) catch lua.typeError(2, "string");
    const script = std.mem.span(zoneScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = zoneScriptEventKind(&buf, script, event_name) orelse return 0;
    _ = eq_cancel_owner(@as(c_int, cdb.EQ_OWNER_ZONE), cdb.zone_id_get(zone), kind.ptr);
    return 0;
}

fn luaZoneScriptEventPending(lua: *Lua) i32 {
    const handle = checkZoneScriptHandle(lua);
    const zone = zoneScriptZone(lua, handle);
    const event_name = lua.toString(2) catch lua.typeError(2, "string");
    const script = std.mem.span(zoneScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = zoneScriptEventKind(&buf, script, event_name) orelse {
        lua.pushBoolean(false);
        return 1;
    };
    lua.pushBoolean(eq_owner_count(@as(c_int, cdb.EQ_OWNER_ZONE), cdb.zone_id_get(zone), kind.ptr) > 0);
    return 1;
}

fn luaZoneScriptEventNextMs(lua: *Lua) i32 {
    const handle = checkZoneScriptHandle(lua);
    const zone = zoneScriptZone(lua, handle);
    const event_name = lua.toString(2) catch lua.typeError(2, "string");
    const script = std.mem.span(zoneScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = zoneScriptEventKind(&buf, script, event_name) orelse {
        lua.pushInteger(-1);
        return 1;
    };
    lua.pushInteger(eq_owner_next_ms(@as(c_int, cdb.EQ_OWNER_ZONE), cdb.zone_id_get(zone), kind.ptr));
    return 1;
}

// ---- Zone-level script entity methods ----

fn luaZoneScriptAdd(lua: *Lua) i32 {
    const key = lua.toString(2) catch lua.typeError(2, "string");
    lua.pushBoolean(cdb.zone_script_add(checkZone(lua), key.ptr));
    return 1;
}

fn luaZoneScriptRemove(lua: *Lua) i32 {
    const zone = checkZone(lua);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    const reason: [*:0]const u8 = if (lua.typeOf(3) == .string) blk: {
        const s = lua.toString(3) catch break :blk "removed";
        break :blk s.ptr;
    } else "removed";
    lua.pushBoolean(cdb.zone_script_remove(zone, key.ptr, reason));
    return 1;
}

fn luaZoneScriptHas(lua: *Lua) i32 {
    const key = lua.toString(2) catch lua.typeError(2, "string");
    lua.pushBoolean(cdb.zone_script_has(checkZone(lua), key.ptr));
    return 1;
}

fn luaZoneScript(lua: *Lua) i32 {
    const zone = checkZone(lua);
    const key = lua.toString(2) catch lua.typeError(2, "string");
    if (!cdb.zone_script_has(zone, key.ptr)) {
        lua.pushNil();
        return 1;
    }
    pushZoneScript(lua, cdb.zone_id_get(zone), key);
    return 1;
}

fn luaZoneScripts(lua: *Lua) i32 {
    const zone = checkZone(lua);
    lua.newTable();
    var maybe_iter = zone_api.zoneScriptIterator(zone);
    if (maybe_iter) |*iter| {
        var i: zlua.Integer = 1;
        while (iter.next()) |entry| {
            pushZoneScript(lua, cdb.zone_id_get(zone), entry.name);
            lua.setIndex(-2, i);
            i += 1;
        }
    }
    return valueIterator(lua);
}

fn luaZoneScriptNumberGet(lua: *Lua) i32 {
    const zone = checkZone(lua);
    const id = lua.toString(2) catch lua.typeError(2, "string");
    const key = lua.toString(3) catch lua.typeError(3, "string");
    lua.pushInteger(cdb.zone_script_number_get(zone, id.ptr, key.ptr));
    return 1;
}

fn luaZoneScriptNumberSet(lua: *Lua) i32 {
    const zone = checkZone(lua);
    const id = lua.toString(2) catch lua.typeError(2, "string");
    const key = lua.toString(3) catch lua.typeError(3, "string");
    const val: i64 = @intCast(lua.toInteger(4) catch lua.typeError(4, "integer"));
    cdb.zone_script_number_set(zone, id.ptr, key.ptr, val);
    return 0;
}

fn luaZoneScriptTextGet(lua: *Lua) i32 {
    const zone = checkZone(lua);
    const id = lua.toString(2) catch lua.typeError(2, "string");
    const key = lua.toString(3) catch lua.typeError(3, "string");
    const val = cdb.zone_script_text_get(zone, id.ptr, key.ptr);
    if (val == null) lua.pushNil() else _ = lua.pushString(std.mem.span(val));
    return 1;
}

fn luaZoneScriptTextSet(lua: *Lua) i32 {
    const zone = checkZone(lua);
    const id = lua.toString(2) catch lua.typeError(2, "string");
    const key = lua.toString(3) catch lua.typeError(3, "string");
    const val = lua.toString(4) catch lua.typeError(4, "string");
    cdb.zone_script_text_set(zone, id.ptr, key.ptr, val.ptr);
    return 0;
}
