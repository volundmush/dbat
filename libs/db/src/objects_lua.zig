const std = @import("std");
const zlua = @import("zlua");
const cdb = @import("cdb");

const Lua = zlua.Lua;
const object_metatable = "dbat.Object";

const ObjectHandle = extern struct {
    id: i64,
};

pub fn register(lua: *Lua) void {
    registerObjectMetatable(lua);

    lua.newTable();
    lua.pushFunction(zlua.wrap(luaObjectById));
    lua.setField(-2, "by_id");
    lua.setField(-2, "objects");
}

fn luaObjectById(lua: *Lua) i32 {
    const id = lua.toInteger(1) catch {
        lua.pushNil();
        return 1;
    };

    if (cdb.obj_by_id(id) == null) {
        lua.pushNil();
        return 1;
    }

    pushObject(lua, id);
    return 1;
}

fn registerObjectMetatable(lua: *Lua) void {
    lua.newMetatable(object_metatable) catch {
        lua.pop(1);
        return;
    };

    lua.pushValue(-1);
    lua.setField(-2, "__index");

    addMethod(lua, "__tostring", luaObjectToString);
    addMethod(lua, "valid", luaObjectValid);
    addMethod(lua, "is_same", luaObjectIsSame);
    addMethod(lua, "id_get", luaObjectIdGet);
    addMethod(lua, "proto_id_get", luaObjectProtoIdGet);
    addMethod(lua, "proto_id_set", luaObjectProtoIdSet);
    addMethod(lua, "vnum_get", luaObjectVnumGet);
    addMethod(lua, "vnum_set", luaObjectVnumSet);
    addMethod(lua, "room_vnum_get", luaObjectRoomVnumGet);
    addMethod(lua, "room_vnum_set", luaObjectRoomVnumSet);
    addMethod(lua, "room_loaded_get", luaObjectRoomLoadedGet);
    addMethod(lua, "room_loaded_set", luaObjectRoomLoadedSet);
    addMethod(lua, "value_get", luaObjectValueGet);
    addMethod(lua, "value_set", luaObjectValueSet);
    addMethod(lua, "value_mod", luaObjectValueMod);
    addMethod(lua, "type_get", luaObjectTypeGet);
    addMethod(lua, "type_set", luaObjectTypeSet);
    addMethod(lua, "level_get", luaObjectLevelGet);
    addMethod(lua, "level_set", luaObjectLevelSet);
    addMethod(lua, "level_mod", luaObjectLevelMod);
    addMethod(lua, "wear_flagged", luaObjectWearFlagged);
    addMethod(lua, "wear_flag_set", luaObjectWearFlagSet);
    addMethod(lua, "wear_flag_toggle", luaObjectWearFlagToggle);
    addMethod(lua, "extra_flagged", luaObjectExtraFlagged);
    addMethod(lua, "extra_flag_set", luaObjectExtraFlagSet);
    addMethod(lua, "extra_flag_toggle", luaObjectExtraFlagToggle);
    addMethod(lua, "aff_flagged", luaObjectAffFlagged);
    addMethod(lua, "aff_flag_set", luaObjectAffFlagSet);
    addMethod(lua, "aff_flag_toggle", luaObjectAffFlagToggle);
    addMethod(lua, "weight_get", luaObjectWeightGet);
    addMethod(lua, "weight_contained_get", luaObjectWeightContainedGet);
    addMethod(lua, "weight_total_get", luaObjectWeightTotalGet);
    addMethod(lua, "weight_set", luaObjectWeightSet);
    addMethod(lua, "weight_mod", luaObjectWeightMod);
    addMethod(lua, "cost_get", luaObjectCostGet);
    addMethod(lua, "cost_set", luaObjectCostSet);
    addMethod(lua, "cost_mod", luaObjectCostMod);
    addMethod(lua, "timer_get", luaObjectTimerGet);
    addMethod(lua, "timer_set", luaObjectTimerSet);
    addMethod(lua, "timer_mod", luaObjectTimerMod);
    addMethod(lua, "size_get", luaObjectSizeGet);
    addMethod(lua, "size_set", luaObjectSizeSet);
    addMethod(lua, "size_mod", luaObjectSizeMod);
    addMethod(lua, "name_get", luaObjectNameGet);
    addMethod(lua, "name_set", luaObjectNameSet);
    addMethod(lua, "description_get", luaObjectDescriptionGet);
    addMethod(lua, "description_set", luaObjectDescriptionSet);
    addMethod(lua, "short_description_get", luaObjectShortDescriptionGet);
    addMethod(lua, "short_description_set", luaObjectShortDescriptionSet);
    addMethod(lua, "action_description_get", luaObjectActionDescriptionGet);
    addMethod(lua, "action_description_set", luaObjectActionDescriptionSet);
    addMethod(lua, "carried_by_get", luaObjectCarriedByGet);
    addMethod(lua, "worn_by_get", luaObjectWornByGet);
    addMethod(lua, "worn_on_get", luaObjectWornOnGet);
    addMethod(lua, "worn_on_set", luaObjectWornOnSet);
    addMethod(lua, "in_obj_get", luaObjectInObjGet);
    addMethod(lua, "sitting_get", luaObjectSittingGet);
    addMethod(lua, "inventory_count", luaObjectInventoryCount);
    addMethod(lua, "inventory_get", luaObjectInventoryGet);
    addMethod(lua, "inventory", luaObjectInventoryGet);

    lua.pop(1);
}

fn addMethod(lua: *Lua, comptime name: [:0]const u8, comptime function: anytype) void {
    lua.pushFunction(zlua.wrap(function));
    lua.setField(-2, name);
}

pub fn pushObject(lua: *Lua, id: i64) void {
    const handle = lua.newUserdata(ObjectHandle, 0);
    handle.* = .{ .id = id };
    _ = lua.getMetatableRegistry(object_metatable);
    lua.setMetatable(-2);
}

fn checkObjectHandle(lua: *Lua) *ObjectHandle {
    return lua.testUserdata(ObjectHandle, 1, object_metatable) catch {
        lua.raiseErrorStr("expected dbat.Object", .{});
    };
}

fn checkObject(lua: *Lua) *cdb.obj_data {
    const handle = checkObjectHandle(lua);
    return cdb.obj_by_id(handle.id) orelse {
        lua.raiseErrorStr("stale dbat.Object handle for object %d", .{handle.id});
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

fn luaObjectValid(lua: *Lua) i32 {
    const handle = checkObjectHandle(lua);
    lua.pushBoolean(cdb.obj_by_id(handle.id) != null);
    return 1;
}

fn luaObjectIsSame(lua: *Lua) i32 {
    const left = checkObjectHandle(lua);
    const right = lua.testUserdata(ObjectHandle, 2, object_metatable) catch {
        lua.pushBoolean(false);
        return 1;
    };
    lua.pushBoolean(left.id == right.id);
    return 1;
}

fn luaObjectToString(lua: *Lua) i32 {
    const handle = checkObjectHandle(lua);
    if (cdb.obj_by_id(handle.id)) |obj| {
        _ = lua.pushFString("dbat.Object(%d, %s)", .{ handle.id, cdb.obj_name_get(obj) });
    } else {
        _ = lua.pushFString("dbat.Object(%d, stale)", .{handle.id});
    }
    return 1;
}

fn luaObjectIdGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_id_get(checkObject(lua)));
    return 1;
}

fn luaObjectProtoIdGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_proto_id_get(checkObject(lua)));
    return 1;
}

fn luaObjectProtoIdSet(lua: *Lua) i32 {
    cdb.obj_proto_id_set(checkObject(lua), intCastOrError(lua, cdb.obj_vnum, integer(lua, 2), "object proto id"));
    return 0;
}

fn luaObjectVnumGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_vnum_get(checkObject(lua)));
    return 1;
}

fn luaObjectVnumSet(lua: *Lua) i32 {
    cdb.obj_vnum_set(checkObject(lua), intCastOrError(lua, cdb.obj_vnum, integer(lua, 2), "object vnum"));
    return 0;
}

fn luaObjectRoomVnumGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_room_vnum_get(checkObject(lua)));
    return 1;
}

fn luaObjectRoomVnumSet(lua: *Lua) i32 {
    cdb.obj_room_vnum_set(checkObject(lua), intCastOrError(lua, cdb.room_vnum, integer(lua, 2), "room vnum"));
    return 0;
}

fn luaObjectRoomLoadedGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_room_loaded_get(checkObject(lua)));
    return 1;
}

fn luaObjectRoomLoadedSet(lua: *Lua) i32 {
    cdb.obj_room_loaded_set(checkObject(lua), intCastOrError(lua, cdb.room_vnum, integer(lua, 2), "loaded room vnum"));
    return 0;
}

fn luaObjectValueGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_value_get(checkObject(lua), intCastOrError(lua, usize, integer(lua, 2), "object value index")));
    return 1;
}

fn luaObjectValueSet(lua: *Lua) i32 {
    cdb.obj_value_set(checkObject(lua), intCastOrError(lua, usize, integer(lua, 2), "object value index"), intCastOrError(lua, c_int, integer(lua, 3), "object value"));
    return 0;
}

fn luaObjectValueMod(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_value_mod(checkObject(lua), intCastOrError(lua, usize, integer(lua, 2), "object value index"), intCastOrError(lua, c_int, integer(lua, 3), "object value delta")));
    return 1;
}

fn luaObjectTypeGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_type_get(checkObject(lua)));
    return 1;
}

fn luaObjectTypeSet(lua: *Lua) i32 {
    cdb.obj_type_set(checkObject(lua), intCastOrError(lua, i8, integer(lua, 2), "object type"));
    return 0;
}

fn luaObjectLevelGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_level_get(checkObject(lua)));
    return 1;
}

fn luaObjectLevelSet(lua: *Lua) i32 {
    cdb.obj_level_set(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "object level"));
    return 0;
}

fn luaObjectLevelMod(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const value = cdb.obj_level_get(obj) + intCastOrError(lua, c_int, integer(lua, 2), "object level delta");
    cdb.obj_level_set(obj, value);
    lua.pushInteger(value);
    return 1;
}

fn luaObjectWearFlagged(lua: *Lua) i32 {
    lua.pushBoolean(cdb.obj_wear_flagged(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "wear flag")));
    return 1;
}

fn luaObjectWearFlagSet(lua: *Lua) i32 {
    cdb.obj_wear_flag_set(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "wear flag"), boolean(lua, 3));
    return 0;
}

fn luaObjectWearFlagToggle(lua: *Lua) i32 {
    lua.pushBoolean(cdb.obj_wear_flag_toggle(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "wear flag")));
    return 1;
}

fn luaObjectExtraFlagged(lua: *Lua) i32 {
    lua.pushBoolean(cdb.obj_extra_flagged(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "extra flag")));
    return 1;
}

fn luaObjectExtraFlagSet(lua: *Lua) i32 {
    cdb.obj_extra_flag_set(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "extra flag"), boolean(lua, 3));
    return 0;
}

fn luaObjectExtraFlagToggle(lua: *Lua) i32 {
    lua.pushBoolean(cdb.obj_extra_flag_toggle(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "extra flag")));
    return 1;
}

fn luaObjectAffFlagged(lua: *Lua) i32 {
    lua.pushBoolean(cdb.obj_aff_flagged(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "aff flag")));
    return 1;
}

fn luaObjectAffFlagSet(lua: *Lua) i32 {
    cdb.obj_aff_flag_set(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "aff flag"), boolean(lua, 3));
    return 0;
}

fn luaObjectAffFlagToggle(lua: *Lua) i32 {
    lua.pushBoolean(cdb.obj_aff_flag_toggle(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "aff flag")));
    return 1;
}

fn luaObjectWeightGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_weight_get(checkObject(lua)));
    return 1;
}

fn luaObjectWeightContainedGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_weight_get_contained(checkObject(lua)));
    return 1;
}

fn luaObjectWeightTotalGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_weight_get_total(checkObject(lua)));
    return 1;
}

fn luaObjectWeightSet(lua: *Lua) i32 {
    cdb.obj_weight_set(checkObject(lua), intCastOrError(lua, i64, integer(lua, 2), "object weight"));
    return 0;
}

fn luaObjectWeightMod(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_weight_mod(checkObject(lua), intCastOrError(lua, i64, integer(lua, 2), "object weight delta")));
    return 1;
}

fn luaObjectCostGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_cost_get(checkObject(lua)));
    return 1;
}

fn luaObjectCostSet(lua: *Lua) i32 {
    cdb.obj_cost_set(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "object cost"));
    return 0;
}

fn luaObjectCostMod(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_cost_mod(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "object cost delta")));
    return 1;
}

fn luaObjectTimerGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_timer_get(checkObject(lua)));
    return 1;
}

fn luaObjectTimerSet(lua: *Lua) i32 {
    cdb.obj_timer_set(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "object timer"));
    return 0;
}

fn luaObjectTimerMod(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_timer_mod(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "object timer delta")));
    return 1;
}

fn luaObjectSizeGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_size_get(checkObject(lua)));
    return 1;
}

fn luaObjectSizeSet(lua: *Lua) i32 {
    cdb.obj_size_set(checkObject(lua), intCastOrError(lua, c_int, integer(lua, 2), "object size"));
    return 0;
}

fn luaObjectSizeMod(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const value = cdb.obj_size_get(obj) + intCastOrError(lua, c_int, integer(lua, 2), "object size delta");
    cdb.obj_size_set(obj, value);
    lua.pushInteger(value);
    return 1;
}

fn luaObjectNameGet(lua: *Lua) i32 {
    pushCString(lua, cdb.obj_name_get(checkObject(lua)));
    return 1;
}

fn luaObjectNameSet(lua: *Lua) i32 {
    cdb.obj_name_set(checkObject(lua), string(lua, 2));
    return 0;
}

fn luaObjectDescriptionGet(lua: *Lua) i32 {
    pushCString(lua, cdb.obj_description_get(checkObject(lua)));
    return 1;
}

fn luaObjectDescriptionSet(lua: *Lua) i32 {
    cdb.obj_description_set(checkObject(lua), string(lua, 2));
    return 0;
}

fn luaObjectShortDescriptionGet(lua: *Lua) i32 {
    pushCString(lua, cdb.obj_short_description_get(checkObject(lua)));
    return 1;
}

fn luaObjectShortDescriptionSet(lua: *Lua) i32 {
    cdb.obj_short_description_set(checkObject(lua), string(lua, 2));
    return 0;
}

fn luaObjectActionDescriptionGet(lua: *Lua) i32 {
    pushCString(lua, cdb.obj_action_description_get(checkObject(lua)));
    return 1;
}

fn luaObjectActionDescriptionSet(lua: *Lua) i32 {
    cdb.obj_action_description_set(checkObject(lua), string(lua, 2));
    return 0;
}

fn luaObjectCarriedByGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_carried_by_get(checkObject(lua)));
    return 1;
}

fn luaObjectWornByGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_worn_by_get(checkObject(lua)));
    return 1;
}

fn luaObjectWornOnGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_worn_on_get(checkObject(lua)));
    return 1;
}

fn luaObjectWornOnSet(lua: *Lua) i32 {
    cdb.obj_worn_on_set(checkObject(lua), intCastOrError(lua, i16, integer(lua, 2), "worn position"));
    return 0;
}

fn luaObjectInObjGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_in_obj_get(checkObject(lua)));
    return 1;
}

fn luaObjectSittingGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_sitting_get(checkObject(lua)));
    return 1;
}

fn luaObjectInventoryCount(lua: *Lua) i32 {
    const recursive = if (lua.isNoneOrNil(2)) false else boolean(lua, 2);
    lua.pushInteger(@intCast(cdb.obj_inventory_count(checkObject(lua), recursive)));
    return 1;
}

fn luaObjectInventoryGet(lua: *Lua) i32 {
    const obj = checkObject(lua);
    if (!lua.isNoneOrNil(2)) {
        var current = obj.contains;
        var pos: usize = 0;
        const wanted = intCastOrError(lua, usize, integer(lua, 2), "inventory index");
        while (current != null) : (current = current.*.next_content) {
            if (pos == wanted) {
                pushObject(lua, cdb.obj_id_get(current));
                return 1;
            }
            pos += 1;
        }
        lua.pushNil();
        return 1;
    }

    lua.newTable();
    var current = obj.contains;
    var index: usize = 1;
    while (current != null) : (current = current.*.next_content) {
        pushObject(lua, cdb.obj_id_get(current));
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
