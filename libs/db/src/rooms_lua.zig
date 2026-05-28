const std = @import("std");
const zlua = @import("zlua");
const cdb = @import("cdb");
const characters_lua = @import("characters_lua.zig");
const objects_lua = @import("objects_lua.zig");

const Lua = zlua.Lua;
const room_metatable = "dbat.Room";

const RoomHandle = extern struct {
    vnum: cdb.room_vnum,
};

pub fn register(lua: *Lua) void {
    registerRoomMetatable(lua);

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

    lua.pushFunction(zlua.wrap(luaRoomValid));
    lua.setField(-2, "valid");
    lua.pushFunction(zlua.wrap(luaRoomIsSame));
    lua.setField(-2, "is_same");
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
    lua.pushFunction(zlua.wrap(luaRoomDamageGet));
    lua.setField(-2, "damage_get");
    lua.pushFunction(zlua.wrap(luaRoomDamageSet));
    lua.setField(-2, "damage_set");
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

    lua.pop(1);
}

pub fn pushRoom(lua: *Lua, vnum: cdb.room_vnum) void {
    const handle = lua.newUserdata(RoomHandle, 0);
    handle.* = .{ .vnum = vnum };
    _ = lua.getMetatableRegistry(room_metatable);
    lua.setMetatable(-2);
}

fn checkRoomHandle(lua: *Lua) *RoomHandle {
    return lua.testUserdata(RoomHandle, 1, room_metatable) catch {
        lua.raiseErrorStr("expected dbat.Room", .{});
    };
}

fn checkRoom(lua: *Lua) *cdb.room_data {
    const handle = checkRoomHandle(lua);
    return cdb.room_by_id(handle.vnum) orelse {
        lua.raiseErrorStr("stale dbat.Room handle for room %d", .{handle.vnum});
    };
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

fn luaRoomToString(lua: *Lua) i32 {
    const handle = checkRoomHandle(lua);
    if (cdb.room_by_id(handle.vnum)) |room| {
        _ = lua.pushFString("dbat.Room(%d, %s)", .{ handle.vnum, cdb.room_name_get(room) });
    } else {
        _ = lua.pushFString("dbat.Room(%d, stale)", .{handle.vnum});
    }
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
    lua.newTable();

    var current = cdb.room_contents_get(room);
    var index: usize = 1;
    while (current != null) : (current = current.*.next_content) {
        objects_lua.pushObject(lua, cdb.obj_id_get(current));
        lua.setIndex(-2, @intCast(index));
        index += 1;
    }

    return valueIterator(lua);
}

fn luaRoomPeopleGet(lua: *Lua) i32 {
    const room = checkRoom(lua);
    lua.newTable();

    var current = cdb.room_people_get(room);
    var index: usize = 1;
    while (current != null) : (current = current.*.next_in_room) {
        characters_lua.pushCharacter(lua, cdb.char_id_get(current));
        lua.setIndex(-2, @intCast(index));
        index += 1;
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
