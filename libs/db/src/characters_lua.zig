const std = @import("std");
const zlua = @import("zlua");
const cdb = @import("cdb");
const objects_lua = @import("objects_lua.zig");

const Lua = zlua.Lua;
const character_metatable = "dbat.Character";

const CharacterHandle = extern struct {
    id: i64,
};

pub fn register(lua: *Lua) void {
    registerCharacterMetatable(lua);

    lua.newTable();
    lua.pushFunction(zlua.wrap(luaCharacterById));
    lua.setField(-2, "by_id");
    lua.setField(-2, "characters");
}

fn luaCharacterById(lua: *Lua) i32 {
    const id = lua.toInteger(1) catch {
        lua.pushNil();
        return 1;
    };

    if (cdb.char_by_id(id) == null) {
        lua.pushNil();
        return 1;
    }

    pushCharacter(lua, id);
    return 1;
}

fn registerCharacterMetatable(lua: *Lua) void {
    lua.newMetatable(character_metatable) catch {
        lua.pop(1);
        return;
    };

    lua.pushValue(-1);
    lua.setField(-2, "__index");

    addMethod(lua, "__tostring", luaCharacterToString);
    addMethod(lua, "valid", luaCharacterValid);
    addMethod(lua, "is_same", luaCharacterIsSame);
    addMethod(lua, "id_get", luaCharacterIdGet);
    addMethod(lua, "proto_id_get", luaCharacterProtoIdGet);
    addMethod(lua, "proto_id_set", luaCharacterProtoIdSet);
    addMethod(lua, "vnum_get", luaCharacterVnumGet);
    addMethod(lua, "vnum_set", luaCharacterVnumSet);
    addMethod(lua, "room_vnum_get", luaCharacterRoomVnumGet);
    addMethod(lua, "room_vnum_set", luaCharacterRoomVnumSet);
    addMethod(lua, "zone_vnum_get", luaCharacterZoneVnumGet);
    addMethod(lua, "name_get", luaCharacterNameGet);
    addMethod(lua, "name_set", luaCharacterNameSet);
    addMethod(lua, "description_get", luaCharacterDescriptionGet);
    addMethod(lua, "description_set", luaCharacterDescriptionSet);
    addMethod(lua, "short_description_get", luaCharacterShortDescriptionGet);
    addMethod(lua, "short_description_set", luaCharacterShortDescriptionSet);
    addMethod(lua, "long_description_get", luaCharacterLongDescriptionGet);
    addMethod(lua, "long_description_set", luaCharacterLongDescriptionSet);
    addMethod(lua, "title_get", luaCharacterTitleGet);
    addMethod(lua, "title_set", luaCharacterTitleSet);
    addMethod(lua, "class_get", luaCharacterClassGet);
    addMethod(lua, "class_set", luaCharacterClassSet);
    addMethod(lua, "class_mod", luaCharacterClassMod);
    addMethod(lua, "race_get", luaCharacterRaceGet);
    addMethod(lua, "race_set", luaCharacterRaceSet);
    addMethod(lua, "race_mod", luaCharacterRaceMod);
    addMethod(lua, "size_get", luaCharacterSizeGet);
    addMethod(lua, "size_set", luaCharacterSizeSet);
    addMethod(lua, "size_mod", luaCharacterSizeMod);
    addMethod(lua, "sex_get", luaCharacterSexGet);
    addMethod(lua, "sex_set", luaCharacterSexSet);
    addMethod(lua, "admin_level_get", luaCharacterAdminLevelGet);
    addMethod(lua, "admin_level_set", luaCharacterAdminLevelSet);
    addMethod(lua, "admin_level_mod", luaCharacterAdminLevelMod);
    addMethod(lua, "admin_flagged", luaCharacterAdminFlagged);
    addMethod(lua, "admin_flag_set", luaCharacterAdminFlagSet);
    addMethod(lua, "admin_flag_toggle", luaCharacterAdminFlagToggle);
    addMethod(lua, "stat_get", luaCharacterStatGet);
    addMethod(lua, "stat_set", luaCharacterStatSet);
    addMethod(lua, "stat_mod", luaCharacterStatMod);
    addMethod(lua, "der_base", luaCharacterDerivedBase);
    addMethod(lua, "der_total", luaCharacterDerivedTotal);
    addMethod(lua, "der_invalidate", luaCharacterDerivedInvalidate);
    addMethod(lua, "inventory_count", luaCharacterInventoryCount);
    addMethod(lua, "equipment_count", luaCharacterEquipmentCount);
    addMethod(lua, "inventory_get", luaCharacterInventoryGet);
    addMethod(lua, "equipment_get", luaCharacterEquipmentGet);
    addMethod(lua, "inventory", luaCharacterInventoryGet);
    addMethod(lua, "equipment", luaCharacterEquipmentGet);

    lua.pop(1);
}

fn addMethod(lua: *Lua, comptime name: [:0]const u8, comptime function: anytype) void {
    lua.pushFunction(zlua.wrap(function));
    lua.setField(-2, name);
}

pub fn pushCharacter(lua: *Lua, id: i64) void {
    const handle = lua.newUserdata(CharacterHandle, 0);
    handle.* = .{ .id = id };
    _ = lua.getMetatableRegistry(character_metatable);
    lua.setMetatable(-2);
}

fn checkCharacterHandle(lua: *Lua) *CharacterHandle {
    return lua.testUserdata(CharacterHandle, 1, character_metatable) catch {
        lua.raiseErrorStr("expected dbat.Character", .{});
    };
}

fn checkCharacter(lua: *Lua) *cdb.char_data {
    const handle = checkCharacterHandle(lua);
    return cdb.char_by_id(handle.id) orelse {
        lua.raiseErrorStr("stale dbat.Character handle for character %d", .{handle.id});
    };
}

fn integer(lua: *Lua, index: i32) zlua.Integer {
    return lua.toInteger(index) catch lua.typeError(index, "integer");
}

fn boolean(lua: *Lua, index: i32) bool {
    if (!lua.isBoolean(index)) lua.typeError(index, "boolean");
    return lua.toBoolean(index);
}

fn string(lua: *Lua, index: i32) [:0]const u8 {
    return lua.toString(index) catch lua.typeError(index, "string");
}

fn pushCString(lua: *Lua, value: [*c]const u8) void {
    if (value == null) {
        lua.pushNil();
        return;
    }
    _ = lua.pushString(std.mem.span(value));
}

fn intCastOrError(lua: *Lua, comptime T: type, value: zlua.Integer, label: [:0]const u8) T {
    return std.math.cast(T, value) orelse lua.raiseErrorStr("%s out of range", .{label.ptr});
}

fn luaCharacterValid(lua: *Lua) i32 {
    const handle = checkCharacterHandle(lua);
    lua.pushBoolean(cdb.char_by_id(handle.id) != null);
    return 1;
}

fn luaCharacterIsSame(lua: *Lua) i32 {
    const left = checkCharacterHandle(lua);
    const right = lua.testUserdata(CharacterHandle, 2, character_metatable) catch {
        lua.pushBoolean(false);
        return 1;
    };
    lua.pushBoolean(left.id == right.id);
    return 1;
}

fn luaCharacterToString(lua: *Lua) i32 {
    const handle = checkCharacterHandle(lua);
    if (cdb.char_by_id(handle.id)) |ch| {
        _ = lua.pushFString("dbat.Character(%d, %s)", .{ handle.id, cdb.char_name_get(ch) });
    } else {
        _ = lua.pushFString("dbat.Character(%d, stale)", .{handle.id});
    }
    return 1;
}

fn luaCharacterIdGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_id_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterProtoIdGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_proto_id_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterProtoIdSet(lua: *Lua) i32 {
    cdb.char_proto_id_set(checkCharacter(lua), intCastOrError(lua, cdb.mob_vnum, integer(lua, 2), "mobile proto id"));
    return 0;
}

fn luaCharacterVnumGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_vnum_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterVnumSet(lua: *Lua) i32 {
    cdb.char_vnum_set(checkCharacter(lua), intCastOrError(lua, cdb.mob_vnum, integer(lua, 2), "mobile vnum"));
    return 0;
}

fn luaCharacterRoomVnumGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_room_vnum_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterRoomVnumSet(lua: *Lua) i32 {
    cdb.char_room_vnum_set(checkCharacter(lua), intCastOrError(lua, cdb.room_vnum, integer(lua, 2), "room vnum"));
    return 0;
}

fn luaCharacterZoneVnumGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_zone_vnum_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterNameGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_name_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterNameSet(lua: *Lua) i32 {
    cdb.char_name_set(checkCharacter(lua), string(lua, 2));
    return 0;
}

fn luaCharacterDescriptionGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_description_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterDescriptionSet(lua: *Lua) i32 {
    cdb.char_description_set(checkCharacter(lua), string(lua, 2));
    return 0;
}

fn luaCharacterShortDescriptionGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_short_description_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterShortDescriptionSet(lua: *Lua) i32 {
    cdb.char_short_description_set(checkCharacter(lua), string(lua, 2));
    return 0;
}

fn luaCharacterLongDescriptionGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_long_description_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterLongDescriptionSet(lua: *Lua) i32 {
    cdb.char_long_description_set(checkCharacter(lua), string(lua, 2));
    return 0;
}

fn luaCharacterTitleGet(lua: *Lua) i32 {
    pushCString(lua, cdb.char_title_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterTitleSet(lua: *Lua) i32 {
    cdb.char_title_set(checkCharacter(lua), string(lua, 2));
    return 0;
}

fn luaCharacterClassGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_class_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterClassSet(lua: *Lua) i32 {
    cdb.char_class_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "class"));
    return 0;
}

fn luaCharacterClassMod(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const value = cdb.char_class_get(ch) + intCastOrError(lua, c_int, integer(lua, 2), "class delta");
    cdb.char_class_set(ch, value);
    lua.pushInteger(value);
    return 1;
}

fn luaCharacterRaceGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_race_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterRaceSet(lua: *Lua) i32 {
    cdb.char_race_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "race"));
    return 0;
}

fn luaCharacterRaceMod(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const value = cdb.char_race_get(ch) + intCastOrError(lua, c_int, integer(lua, 2), "race delta");
    cdb.char_race_set(ch, value);
    lua.pushInteger(value);
    return 1;
}

fn luaCharacterSizeGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_size_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterSizeSet(lua: *Lua) i32 {
    cdb.char_size_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "size"));
    return 0;
}

fn luaCharacterSizeMod(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const value = cdb.char_size_get(ch) + intCastOrError(lua, c_int, integer(lua, 2), "size delta");
    cdb.char_size_set(ch, value);
    lua.pushInteger(value);
    return 1;
}

fn luaCharacterSexGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_sex_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterSexSet(lua: *Lua) i32 {
    cdb.char_sex_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "sex"));
    return 0;
}

fn luaCharacterAdminLevelGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_admlevel_get(checkCharacter(lua)));
    return 1;
}

fn luaCharacterAdminLevelSet(lua: *Lua) i32 {
    cdb.char_admlevel_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "admin level"));
    return 0;
}

fn luaCharacterAdminLevelMod(lua: *Lua) i32 {
    const ch = checkCharacter(lua);
    const value = cdb.char_admlevel_get(ch) + intCastOrError(lua, c_int, integer(lua, 2), "admin level delta");
    cdb.char_admlevel_set(ch, value);
    lua.pushInteger(value);
    return 1;
}

fn luaCharacterAdminFlagged(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_admflagged(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "admin flag")));
    return 1;
}

fn luaCharacterAdminFlagSet(lua: *Lua) i32 {
    cdb.char_admflag_set(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "admin flag"), boolean(lua, 3));
    return 0;
}

fn luaCharacterAdminFlagToggle(lua: *Lua) i32 {
    lua.pushBoolean(cdb.char_admflag_toggle(checkCharacter(lua), intCastOrError(lua, c_int, integer(lua, 2), "admin flag")));
    return 1;
}

fn luaCharacterStatGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_stat_get(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterStatSet(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_stat_set(checkCharacter(lua), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "stat value")));
    return 1;
}

fn luaCharacterStatMod(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_stat_mod(checkCharacter(lua), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "stat delta")));
    return 1;
}

fn luaCharacterDerivedBase(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_der_get_base(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterDerivedTotal(lua: *Lua) i32 {
    lua.pushInteger(cdb.char_der_get_total(checkCharacter(lua), string(lua, 2)));
    return 1;
}

fn luaCharacterDerivedInvalidate(lua: *Lua) i32 {
    cdb.char_der_invalidate(checkCharacter(lua));
    return 0;
}

fn luaCharacterInventoryCount(lua: *Lua) i32 {
    const recursive = if (lua.isNoneOrNil(2)) false else boolean(lua, 2);
    lua.pushInteger(@intCast(cdb.char_inventory_count(checkCharacter(lua), recursive)));
    return 1;
}

fn luaCharacterEquipmentCount(lua: *Lua) i32 {
    const recursive = if (lua.isNoneOrNil(2)) false else boolean(lua, 2);
    lua.pushInteger(@intCast(cdb.char_equipment_count(checkCharacter(lua), recursive)));
    return 1;
}

fn luaCharacterInventoryGet(lua: *Lua) i32 {
    if (lua.isNoneOrNil(2)) return pushInventoryIterator(lua, checkCharacter(lua));

    const obj = cdb.char_inventory_get(checkCharacter(lua), intCastOrError(lua, usize, integer(lua, 2), "inventory index"));
    if (obj == null) {
        lua.pushNil();
        return 1;
    }
    objects_lua.pushObject(lua, cdb.obj_id_get(obj));
    return 1;
}

fn luaCharacterEquipmentGet(lua: *Lua) i32 {
    if (lua.isNoneOrNil(2)) return pushEquipmentIterator(lua, checkCharacter(lua));

    const obj = cdb.char_equipment_get(checkCharacter(lua), intCastOrError(lua, usize, integer(lua, 2), "equipment index"));
    if (obj == null) {
        lua.pushNil();
        return 1;
    }
    objects_lua.pushObject(lua, cdb.obj_id_get(obj));
    return 1;
}

fn pushInventoryIterator(lua: *Lua, ch: *cdb.char_data) i32 {
    lua.newTable();
    var pos: usize = 0;
    while (true) : (pos += 1) {
        const obj = cdb.char_inventory_get(ch, pos);
        if (obj == null) break;
        objects_lua.pushObject(lua, cdb.obj_id_get(obj));
        lua.setIndex(-2, @intCast(pos + 1));
    }
    return valueIterator(lua);
}

fn pushEquipmentIterator(lua: *Lua, ch: *cdb.char_data) i32 {
    lua.newTable();
    var pos: usize = 0;
    while (true) : (pos += 1) {
        const obj = cdb.char_equipment_get(ch, pos);
        if (pos >= cdb.NUM_WEARS) break;
        if (obj == null) continue;
        objects_lua.pushObject(lua, cdb.obj_id_get(obj));
        lua.setIndex(-2, @intCast(pos));
    }
    return pairsIterator(lua);
}

fn valueIterator(lua: *Lua) i32 {
    _ = lua.getGlobal("dbat");
    _ = lua.getField(-1, "_values");
    lua.remove(-2);
    lua.insert(-2);
    lua.protectedCall(.{ .args = 1, .results = 1 }) catch lua.raiseErrorStr("failed to create value iterator", .{});
    return 1;
}

fn pairsIterator(lua: *Lua) i32 {
    _ = lua.getGlobal("pairs");
    lua.insert(-2);
    lua.protectedCall(.{ .args = 1, .results = 3 }) catch lua.raiseErrorStr("failed to create pairs iterator", .{});
    return 3;
}
