const std = @import("std");
const zlua = @import("zlua");
const cdb = @import("cdb");
const characters_lua = @import("character_lua.zig");
const rooms_lua = @import("room_lua.zig");
const lua_meta = @import("lua_meta.zig");
const object_api = @import("object_api.zig");

const Lua = zlua.Lua;
const object_metatable = "dbat.Object";
const obj_proto_metatable = "dbat.ObjectPrototype";
const obj_script_metatable = "dbat.ObjectScript";

extern fn event_schedule_lua_obj_update(fire_at: i64, interval: i64, kind: ?[*:0]const u8, obj_id: i64) u64;
extern fn hatch_get_vehicle(hatch: *cdb.obj_data) ?*cdb.obj_data;
extern fn find_vehicle_by_vnum(vnum: c_int) ?*cdb.obj_data;
extern fn create_obj() ?*cdb.obj_data;
extern fn eq_cancel_owner(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn eq_owner_count(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn eq_owner_next_ms(owner_kind: c_int, owner_id: i64, tag: ?[*:0]const u8) i64;
extern fn event_queue_now_ms() i64;

const ObjectHandle = extern struct {
    id: i64,
};

const ObjProtoHandle = extern struct {
    vnum: cdb.obj_vnum,
};

const ObjScriptHandle = extern struct {
    obj_id: i64,
    script: [64:0]u8,
};

pub fn register(lua: *Lua) void {
    registerObjectMetatable(lua);
    registerObjProtoMetatable(lua);
    registerObjScriptMetatable(lua);

    lua.newTable();
    lua.pushFunction(zlua.wrap(luaObjectById));
    lua.setField(-2, "by_id");
    lua.pushFunction(zlua.wrap(luaObjectsAll));
    lua.setField(-2, "all");
    lua.pushFunction(zlua.wrap(luaFindVehicle));
    lua.setField(-2, "find_vehicle");
    lua.setField(-2, "objects");

    lua.pushFunction(zlua.wrap(luaObjCreate));
    lua.setField(-2, "obj_create");

    lua.newTable();
    lua.pushFunction(zlua.wrap(luaObjProtoById));
    lua.setField(-2, "by_id");
    lua.pushFunction(zlua.wrap(luaObjProtosAll));
    lua.setField(-2, "all");
    lua.setField(-2, "obj_protos");
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

fn luaFindVehicle(lua: *Lua) i32 {
    const vnum = lua.toInteger(1) catch { lua.pushNil(); return 1; };
    const vehicle = find_vehicle_by_vnum(@intCast(vnum));
    if (vehicle == null) { lua.pushNil(); return 1; }
    pushObject(lua, cdb.obj_id_get(vehicle));
    return 1;
}

fn luaObjectsAll(lua: *Lua) i32 {
    lua.newTable();
    const iterator = cdb.obj_iterator_create();
    defer cdb.obj_iterator_free(iterator);

    var index: usize = 1;
    while (cdb.obj_next(iterator)) |obj| {
        pushObject(lua, cdb.obj_id_get(obj));
        lua.setIndex(-2, @intCast(index));
        index += 1;
    }

    return valueIterator(lua);
}

fn registerObjectMetatable(lua: *Lua) void {
    lua.newMetatable(object_metatable) catch {
        lua.pop(1);
        return;
    };

    lua.pushValue(-1);
    lua.setField(-2, "__index");

    addMethod(lua, "__tostring", luaObjectToString);
    addMethod(lua, "reftype", luaObjectRefType);
    addMethod(lua, "valid", luaObjectValid);
    addMethod(lua, "is_same", luaObjectIsSame);
    addMethod(lua, "__eq", luaObjectIsSame);
    addMethod(lua, "extract", luaObjectExtract);
    addMethod(lua, "id_get", luaObjectIdGet);
    addMethod(lua, "proto_id_get", luaObjectProtoIdGet);
    addMethod(lua, "proto_id_set", luaObjectProtoIdSet);
    addMethod(lua, "vnum_get", luaObjectVnumGet);
    addMethod(lua, "vnum_set", luaObjectVnumSet);
    addMethod(lua, "room_vnum_get", luaObjectRoomVnumGet);
    addMethod(lua, "room_vnum_set", luaObjectRoomVnumSet);
    addMethod(lua, "room_loaded_get", luaObjectRoomLoadedGet);
    addMethod(lua, "room_loaded_set", luaObjectRoomLoadedSet);
    addMethod(lua, "from_room", luaObjectFromRoom);
    addMethod(lua, "to_room", luaObjectToRoom);
    addMethod(lua, "from_char", luaObjectFromChar);
    addMethod(lua, "to_char", luaObjectToChar);
    addMethod(lua, "from_container", luaObjectFromContainer);
    addMethod(lua, "to_container", luaObjectToContainer);
    addMethod(lua, "show_to", luaObjectShowTo);
    addMethod(lua, "equip", luaObjectEquip);
    addMethod(lua, "value_get", luaObjectValueGet);
    addMethod(lua, "value_set", luaObjectValueSet);
    addMethod(lua, "value_mod", luaObjectValueMod);
    addMethod(lua, "type_get", luaObjectTypeGet);
    addMethod(lua, "type_set", luaObjectTypeSet);
    addMethod(lua, "level_get", luaObjectLevelGet);
    addMethod(lua, "level_set", luaObjectLevelSet);
    addMethod(lua, "level_mod", luaObjectLevelMod);
    addMethod(lua, "affect_set", luaObjectAffectSet);
    addMethod(lua, "affect_location_get", luaObjectAffectLocationGet);
    addMethod(lua, "affect_modifier_get", luaObjectAffectModifierGet);
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
    addMethod(lua, "sitting_set", luaObjectSittingSet);
    addMethod(lua, "inventory_count", luaObjectInventoryCount);
    addMethod(lua, "inventory_get", luaObjectInventoryGet);
    addMethod(lua, "inventory", luaObjectInventoryGet);
    addMethod(lua, "event_schedule", luaObjectEventSchedule);
    addMethod(lua, "event_cancel", luaObjectEventCancel);
    addMethod(lua, "event_count", luaObjectEventCount);
    addMethod(lua, "event_remaining_ms", luaObjectEventRemainingMs);
    addMethod(lua, "kicharge_get", luaObjectKichargeGet);
    addMethod(lua, "user_get", luaObjectUserGet);
    addMethod(lua, "target_get", luaObjectTargetGet);
    addMethod(lua, "distance_get", luaObjectDistanceGet);
    addMethod(lua, "scoutfreq_get", luaObjectScoutfreqGet);
    addMethod(lua, "room_get", luaObjectRoomGet);
    addMethod(lua, "hatch_vehicle_get", luaObjectHatchVehicleGet);
    addMethod(lua, "post_type_get", luaObjectPostTypeGet);
    addMethod(lua, "is_posted", luaObjectIsPosted);
    addMethod(lua, "fellow_wall_has", luaObjectFellowWallHas);
    addMethod(lua, "fellow_wall_set", luaObjectFellowWallSet);
    addMethod(lua, "foob_get", luaObjectFoobGet);
    addMethod(lua, "drinkcon_weight_drain", luaObjectDrinkconWeightDrain);
    addMethod(lua, "drinkcon_name_update", luaObjectDrinkconNameUpdate);
    addMethod(lua, "script_add", luaObjectScriptAdd);
    addMethod(lua, "script_remove", luaObjectScriptRemove);
    addMethod(lua, "script_has", luaObjectScriptHas);
    addMethod(lua, "script", luaObjectScript);
    addMethod(lua, "scripts", luaObjectScripts);
    addMethod(lua, "script_number_get", luaObjectScriptNumberGet);
    addMethod(lua, "script_number_set", luaObjectScriptNumberSet);
    addMethod(lua, "script_text_get", luaObjectScriptTextGet);
    addMethod(lua, "script_text_set", luaObjectScriptTextSet);

    lua_meta.mergeMethods(lua, "lua.objects.object");

    lua.pop(1);
}

fn registerObjProtoMetatable(lua: *Lua) void {
    lua.newMetatable(obj_proto_metatable) catch {
        lua.pop(1);
        return;
    };

    lua.pushValue(-1);
    lua.setField(-2, "__index");

    addMethod(lua, "__tostring", luaObjProtoToString);
    addMethod(lua, "reftype", luaObjProtoRefType);
    addMethod(lua, "valid", luaObjProtoValid);
    addMethod(lua, "vnum_get", luaObjProtoVnumGet);
    addMethod(lua, "name_get", luaObjProtoNameGet);
    addMethod(lua, "short_description_get", luaObjProtoShortDescrGet);
    addMethod(lua, "type_get", luaObjProtoTypeGet);
    addMethod(lua, "value_get", luaObjProtoValueGet);
    addMethod(lua, "has_trig", luaObjProtoHasTrig);
    addMethod(lua, "spawn", luaObjProtoSpawn);

    lua.pop(1);
}

fn registerObjScriptMetatable(lua: *Lua) void {
    lua.newMetatable(obj_script_metatable) catch {
        lua.pop(1);
        return;
    };
    lua.pushValue(-1);
    lua.setField(-2, "__index");
    addMethod(lua, "id", luaObjScriptId);
    addMethod(lua, "number_get", luaObjScriptNumberGet);
    addMethod(lua, "number_set", luaObjScriptNumberSet);
    addMethod(lua, "number_mod", luaObjScriptNumberMod);
    addMethod(lua, "text_get", luaObjScriptTextGet);
    addMethod(lua, "text_set", luaObjScriptTextSet);
    addMethod(lua, "schedule_event", luaObjScriptScheduleEvent);
    addMethod(lua, "cancel_event", luaObjScriptCancelEvent);
    addMethod(lua, "event_pending", luaObjScriptEventPending);
    addMethod(lua, "event_next_ms", luaObjScriptEventNextMs);
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

pub fn pushObjProto(lua: *Lua, vnum: cdb.obj_vnum) void {
    const handle = lua.newUserdata(ObjProtoHandle, 0);
    handle.* = .{ .vnum = vnum };
    _ = lua.getMetatableRegistry(obj_proto_metatable);
    lua.setMetatable(-2);
}

fn checkObjectHandle(lua: *Lua) *ObjectHandle {
    return lua.testUserdata(ObjectHandle, 1, object_metatable) catch {
        lua.raiseErrorStr("expected dbat.Object", .{});
    };
}

fn checkObject(lua: *Lua) *cdb.obj_data {
    return checkObjectAt(lua, 1);
}

pub fn checkObjectAt(lua: *Lua, index: i32) *cdb.obj_data {
    const handle = lua.testUserdata(ObjectHandle, index, object_metatable) catch {
        lua.raiseErrorStr("expected dbat.Object", .{});
    };
    return cdb.obj_by_id(handle.id) orelse {
        lua.raiseErrorStr("stale dbat.Object handle for object %d", .{handle.id});
    };
}

fn checkObjectSelf(lua: *Lua) *cdb.obj_data {
    const handle = checkObjectHandle(lua);
    return cdb.obj_by_id(handle.id) orelse {
        lua.raiseErrorStr("stale dbat.Object handle for object %d", .{handle.id});
    };
}

fn checkObjProtoHandle(lua: *Lua) *ObjProtoHandle {
    return lua.testUserdata(ObjProtoHandle, 1, obj_proto_metatable) catch {
        lua.raiseErrorStr("expected dbat.ObjectPrototype", .{});
    };
}

fn checkObjProto(lua: *Lua) *cdb.obj_proto_data {
    const handle = checkObjProtoHandle(lua);
    return cdb.obj_proto_by_id(handle.vnum) orelse {
        lua.raiseErrorStr("stale dbat.ObjectPrototype handle for object prototype %d", .{handle.vnum});
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

fn luaObjectExtract(lua: *Lua) i32 {
    cdb.extract_obj(checkObject(lua));
    return 0;
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

fn luaObjectRefType(lua: *Lua) i32 {
    _ = checkObject(lua);
    _ = lua.pushString("object");
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

fn luaObjectFromRoom(lua: *Lua) i32 {
    const obj = checkObject(lua);
    if (cdb.obj_room_get(obj) != null) cdb.obj_from_room(obj);
    return 0;
}

fn luaObjectToRoom(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const room = rooms_lua.checkRoomAt(lua, 2);
    removeObjectFromLocation(obj);
    cdb.obj_to_room(obj, room);
    return 0;
}

fn luaObjectFromChar(lua: *Lua) i32 {
    const obj = checkObject(lua);
    if (cdb.obj_carried_by_get(obj) != 0) cdb.obj_from_char(obj);
    return 0;
}

fn luaObjectToChar(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const ch = characters_lua.checkCharacterAt(lua, 2);
    removeObjectFromLocation(obj);
    cdb.obj_to_char(obj, ch);
    return 0;
}

fn luaObjectFromContainer(lua: *Lua) i32 {
    cdb.obj_from_container(checkObject(lua));
    return 0;
}

fn luaObjectToContainer(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const container = checkObjectAt(lua, 2);
    cdb.obj_to_container(obj, container);
    return 0;
}

fn luaObjectShowTo(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const ch = characters_lua.checkCharacterAt(lua, 2);
    cdb.obj_show_action_to_char(obj, ch);
    return 0;
}

fn luaObjectEquip(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const ch = characters_lua.checkCharacterAt(lua, 2);
    const pos = intCastOrError(lua, c_int, integer(lua, 3), "equipment position");
    removeObjectFromLocation(obj);
    cdb.equip_char(ch, obj, pos);
    return 0;
}

fn luaObjProtoById(lua: *Lua) i32 {
    const vnum = lua.toInteger(1) catch {
        lua.pushNil();
        return 1;
    };
    const obj_vnum = std.math.cast(cdb.obj_vnum, vnum) orelse {
        lua.pushNil();
        return 1;
    };
    if (cdb.obj_proto_by_id(obj_vnum) == null) {
        lua.pushNil();
        return 1;
    }
    pushObjProto(lua, obj_vnum);
    return 1;
}

fn luaObjProtoToString(lua: *Lua) i32 {
    const handle = checkObjProtoHandle(lua);
    _ = checkObjProto(lua);
    _ = lua.pushFString("dbat.ObjectPrototype(%d)", .{handle.vnum});
    return 1;
}

fn luaObjProtoRefType(lua: *Lua) i32 {
    _ = checkObjProto(lua);
    _ = lua.pushString("object_prototype");
    return 1;
}

fn luaObjProtoValid(lua: *Lua) i32 {
    const handle = checkObjProtoHandle(lua);
    lua.pushBoolean(cdb.obj_proto_by_id(handle.vnum) != null);
    return 1;
}

fn luaObjProtoVnumGet(lua: *Lua) i32 {
    lua.pushInteger(checkObjProtoHandle(lua).vnum);
    return 1;
}

fn luaObjCreate(lua: *Lua) i32 {
    const obj = create_obj() orelse { lua.pushNil(); return 1; };
    pushObject(lua, cdb.obj_id_get(obj));
    return 1;
}

fn luaObjProtoSpawn(lua: *Lua) i32 {
    const handle = checkObjProtoHandle(lua);
    _ = checkObjProto(lua);
    const obj = cdb.obj_spawn(handle.vnum);
    if (obj == null) {
        lua.pushNil();
        return 1;
    }

    if (!lua.isNoneOrNil(2)) {
        const room = rooms_lua.checkRoomAt(lua, 2);
        cdb.obj_to_room(obj, room);
    }

    pushObject(lua, cdb.obj_id_get(obj));
    return 1;
}

fn luaObjProtoNameGet(lua: *Lua) i32 {
    const proto = checkObjProto(lua);
    if (proto.name) |n| _ = lua.pushString(std.mem.span(n)) else lua.pushNil();
    return 1;
}
fn luaObjProtoShortDescrGet(lua: *Lua) i32 {
    const proto = checkObjProto(lua);
    if (proto.short_description) |s| _ = lua.pushString(std.mem.span(s)) else lua.pushNil();
    return 1;
}
fn luaObjProtoTypeGet(lua: *Lua) i32 {
    const proto = checkObjProto(lua);
    lua.pushInteger(proto.type_flag);
    return 1;
}
fn luaObjProtoValueGet(lua: *Lua) i32 {
    const proto = checkObjProto(lua);
    const pos = integer(lua, 2);
    if (pos < 0 or pos >= cdb.NUM_OBJ_VAL_POSITIONS) {
        lua.pushInteger(0);
        return 1;
    }
    lua.pushInteger(proto.value[@intCast(pos)]);
    return 1;
}
fn luaObjProtoHasTrig(lua: *Lua) i32 {
    const proto = checkObjProto(lua);
    lua.pushBoolean(proto.proto_script != null);
    return 1;
}
fn luaObjProtosAll(lua: *Lua) i32 {
    lua.newTable();
    const iterator = cdb.obj_proto_iterator_create();
    defer cdb.obj_proto_iterator_free(iterator);
    var index: zlua.Integer = 1;
    while (true) {
        const proto_c = cdb.obj_proto_next(iterator);
        if (proto_c == null) break;
        const proto: *cdb.obj_proto_data = @ptrCast(@alignCast(proto_c));
        pushObjProto(lua, proto.id);
        lua.setIndex(-2, index);
        index += 1;
    }
    return valueIterator(lua);
}

fn removeObjectFromLocation(obj: *cdb.obj_data) void {
    if (cdb.obj_carried_by_get(obj) != 0) {
        cdb.obj_from_char(obj);
    } else if (cdb.obj_room_get(obj) != null) {
        cdb.obj_from_room(obj);
    } else if (cdb.obj_in_obj_get(obj) != 0) {
        cdb.obj_from_obj(obj);
    }
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

fn luaObjectAffectSet(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const index = @as(usize, @intCast(integer(lua, 2)));
    const location = @as(c_int, @intCast(integer(lua, 3)));
    const specific = @as(c_int, @intCast(integer(lua, 4)));
    const modifier = @as(c_int, @intCast(integer(lua, 5)));
    if (index < obj.affected.len) {
        obj.affected[index].location = location;
        obj.affected[index].specific = specific;
        obj.affected[index].modifier = modifier;
    }
    return 0;
}
fn luaObjectAffectLocationGet(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const index = @as(usize, @intCast(integer(lua, 2)));
    lua.pushInteger(if (index < obj.affected.len) obj.affected[index].location else 0);
    return 1;
}
fn luaObjectAffectModifierGet(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const index = @as(usize, @intCast(integer(lua, 2)));
    lua.pushInteger(if (index < obj.affected.len) obj.affected[index].modifier else 0);
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
fn luaObjectSittingSet(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const ch: ?*cdb.char_data = if (lua.isNoneOrNil(2)) null else characters_lua.checkCharacterAt(lua, 2);
    cdb.obj_sitting_set(obj, ch);
    return 0;
}

fn luaObjectInventoryCount(lua: *Lua) i32 {
    const recursive = if (lua.isNoneOrNil(2)) false else boolean(lua, 2);
    lua.pushInteger(@intCast(cdb.obj_inventory_count(checkObject(lua), recursive)));
    return 1;
}

fn luaObjectInventoryGet(lua: *Lua) i32 {
    const obj = checkObject(lua);
    if (!lua.isNoneOrNil(2)) {
        var count: usize = 0;
        const ids = cdb.obj_inventory_get(obj, &count);
        defer if (ids) |_| std.c.free(@as(?*anyopaque, @ptrCast(ids)));

        const pos = intCastOrError(lua, usize, integer(lua, 2), "inventory index");
        if (ids == null or pos >= count) {
            lua.pushNil();
            return 1;
        }
        pushObject(lua, ids[pos]);
        return 1;
    }

    var count: usize = 0;
    const ids = cdb.obj_inventory_get(obj, &count);
    defer if (ids) |_| std.c.free(@as(?*anyopaque, @ptrCast(ids)));

    lua.newTable();
    for (0..count) |i| {
        if (ids) |ptr| {
            pushObject(lua, ptr[i]);
            lua.setIndex(-2, @intCast(i + 1));
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

fn luaObjectEventSchedule(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const kind = string(lua, 2);
    const delay_ms = integer(lua, 3);
    const interval_ms: i64 = if (lua.typeOf(4) == .number) @intCast(integer(lua, 4)) else 0;
    const now = event_queue_now_ms();
    const id = event_schedule_lua_obj_update(now + delay_ms, interval_ms, kind.ptr, cdb.obj_id_get(obj));
    lua.pushInteger(@intCast(id));
    return 1;
}

fn luaObjectEventCancel(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const kind = string(lua, 2);
    const n = eq_cancel_owner(@as(c_int, cdb.EQ_OWNER_OBJ), cdb.obj_id_get(obj), kind.ptr);
    lua.pushInteger(n);
    return 1;
}

fn luaObjectEventCount(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const kind: ?[*:0]const u8 = if (lua.typeOf(2) == .string) string(lua, 2).ptr else null;
    const n = eq_owner_count(@as(c_int, cdb.EQ_OWNER_OBJ), cdb.obj_id_get(obj), kind);
    lua.pushInteger(n);
    return 1;
}

fn luaObjectEventRemainingMs(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const kind: ?[*:0]const u8 = if (lua.typeOf(2) == .string) string(lua, 2).ptr else null;
    const ms = eq_owner_next_ms(@as(c_int, cdb.EQ_OWNER_OBJ), cdb.obj_id_get(obj), kind);
    lua.pushInteger(ms);
    return 1;
}

fn luaObjectKichargeGet(lua: *Lua) i32 {
    lua.pushInteger(checkObject(lua).kicharge);
    return 1;
}

fn luaObjectUserGet(lua: *Lua) i32 {
    const obj = checkObject(lua);
    if (obj.user) |user|
        characters_lua.pushCharacter(lua, cdb.char_id_get(user))
    else
        lua.pushNil();
    return 1;
}

fn luaObjectTargetGet(lua: *Lua) i32 {
    const obj = checkObject(lua);
    if (obj.target) |target|
        characters_lua.pushCharacter(lua, cdb.char_id_get(target))
    else
        lua.pushNil();
    return 1;
}

fn luaObjectDistanceGet(lua: *Lua) i32 {
    lua.pushInteger(checkObject(lua).distance);
    return 1;
}

fn luaObjectScoutfreqGet(lua: *Lua) i32 {
    lua.pushInteger(checkObject(lua).scoutfreq);
    return 1;
}

fn luaObjectRoomGet(lua: *Lua) i32 {
    const room = cdb.obj_room_get(checkObject(lua));
    if (room == null) {
        lua.pushNil();
        return 1;
    }
    rooms_lua.pushRoom(lua, room.*.id);
    return 1;
}

fn luaObjectHatchVehicleGet(lua: *Lua) i32 {
    const vehicle = hatch_get_vehicle(checkObject(lua));
    if (vehicle == null) {
        lua.pushNil();
        return 1;
    }
    pushObject(lua, cdb.obj_id_get(vehicle.?));
    return 1;
}

fn luaObjectPostTypeGet(lua: *Lua) i32 {
    lua.pushInteger(checkObject(lua).posttype);
    return 1;
}

fn luaObjectIsPosted(lua: *Lua) i32 {
    lua.pushBoolean(checkObject(lua).posted_to != null);
    return 1;
}

fn luaObjectFellowWallHas(lua: *Lua) i32 {
    lua.pushBoolean(checkObject(lua).fellow_wall != null);
    return 1;
}

fn luaObjectFellowWallSet(lua: *Lua) i32 {
    const obj = checkObject(lua);
    if (lua.isNoneOrNil(2)) {
        obj.fellow_wall = null;
    } else {
        obj.fellow_wall = checkObjectAt(lua, 2);
    }
    return 0;
}

fn luaObjectFoobGet(lua: *Lua) i32 {
    lua.pushInteger(checkObject(lua).foob);
    return 1;
}

fn luaObjectDrinkconWeightDrain(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const amount = intCastOrError(lua, c_int, integer(lua, 2), "amount");
    cdb.obj_drinkcon_weight_drain(obj, amount);
    return 0;
}

fn luaObjectDrinkconNameUpdate(lua: *Lua) i32 {
    cdb.obj_drinkcon_name_update(checkObject(lua));
    return 0;
}

// ---- ObjectScript handle ----

pub fn pushObjectScript(lua: *Lua, obj_id: i64, script_id: []const u8) void {
    const handle = lua.newUserdata(ObjScriptHandle, 0);
    handle.obj_id = obj_id;
    handle.script = std.mem.zeroes([64:0]u8);
    const len = @min(script_id.len, handle.script.len - 1);
    @memcpy(handle.script[0..len], script_id[0..len]);
    _ = lua.getMetatableRegistry(obj_script_metatable);
    lua.setMetatable(-2);
}

fn checkObjScriptHandle(lua: *Lua) *ObjScriptHandle {
    return lua.testUserdata(ObjScriptHandle, 1, obj_script_metatable) catch {
        lua.raiseErrorStr("expected dbat.ObjectScript", .{});
    };
}

fn objScriptObject(lua: *Lua, handle: *ObjScriptHandle) *cdb.obj_data {
    return cdb.obj_by_id(handle.obj_id) orelse lua.raiseErrorStr("stale dbat.ObjectScript object", .{});
}

fn objScriptName(handle: *ObjScriptHandle) [*:0]const u8 {
    return @ptrCast(&handle.script);
}

fn objScriptEventKind(buf: *[192:0]u8, script: []const u8, event: []const u8) ?[:0]u8 {
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

fn luaObjScriptId(lua: *Lua) i32 {
    _ = lua.pushString(std.mem.span(objScriptName(checkObjScriptHandle(lua))));
    return 1;
}

fn luaObjScriptNumberGet(lua: *Lua) i32 {
    const handle = checkObjScriptHandle(lua);
    lua.pushInteger(cdb.obj_script_number_get(objScriptObject(lua, handle), objScriptName(handle), string(lua, 2)));
    return 1;
}

fn luaObjScriptNumberSet(lua: *Lua) i32 {
    const handle = checkObjScriptHandle(lua);
    cdb.obj_script_number_set(objScriptObject(lua, handle), objScriptName(handle), string(lua, 2), intCastOrError(lua, i64, integer(lua, 3), "script number"));
    return 0;
}

fn luaObjScriptNumberMod(lua: *Lua) i32 {
    const handle = checkObjScriptHandle(lua);
    const obj = objScriptObject(lua, handle);
    const name = objScriptName(handle);
    const key = string(lua, 2);
    const delta = intCastOrError(lua, i64, integer(lua, 3), "script number delta");
    const old = cdb.obj_script_number_get(obj, name, key);
    cdb.obj_script_number_set(obj, name, key, old + delta);
    lua.pushInteger(old + delta);
    return 1;
}

fn luaObjScriptTextGet(lua: *Lua) i32 {
    const handle = checkObjScriptHandle(lua);
    pushCString(lua, cdb.obj_script_text_get(objScriptObject(lua, handle), objScriptName(handle), string(lua, 2)));
    return 1;
}

fn luaObjScriptTextSet(lua: *Lua) i32 {
    const handle = checkObjScriptHandle(lua);
    cdb.obj_script_text_set(objScriptObject(lua, handle), objScriptName(handle), string(lua, 2), string(lua, 3));
    return 0;
}

fn luaObjScriptScheduleEvent(lua: *Lua) i32 {
    const handle = checkObjScriptHandle(lua);
    const obj = objScriptObject(lua, handle);
    const event_name = string(lua, 2);
    const delay_ms = integer(lua, 3);
    const interval_ms: i64 = if (lua.isNoneOrNil(4)) 0 else @intCast(integer(lua, 4));
    const script = std.mem.span(objScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = objScriptEventKind(&buf, script, event_name) orelse {
        lua.pushInteger(0);
        return 1;
    };
    _ = eq_cancel_owner(@as(c_int, cdb.EQ_OWNER_OBJ), cdb.obj_id_get(obj), kind.ptr);
    const id = event_schedule_lua_obj_update(event_queue_now_ms() + delay_ms, interval_ms, kind.ptr, cdb.obj_id_get(obj));
    lua.pushInteger(@intCast(id));
    return 1;
}

fn luaObjScriptCancelEvent(lua: *Lua) i32 {
    const handle = checkObjScriptHandle(lua);
    const obj = objScriptObject(lua, handle);
    const event_name = string(lua, 2);
    const script = std.mem.span(objScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = objScriptEventKind(&buf, script, event_name) orelse return 0;
    _ = eq_cancel_owner(@as(c_int, cdb.EQ_OWNER_OBJ), cdb.obj_id_get(obj), kind.ptr);
    return 0;
}

fn luaObjScriptEventPending(lua: *Lua) i32 {
    const handle = checkObjScriptHandle(lua);
    const obj = objScriptObject(lua, handle);
    const event_name = string(lua, 2);
    const script = std.mem.span(objScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = objScriptEventKind(&buf, script, event_name) orelse {
        lua.pushBoolean(false);
        return 1;
    };
    lua.pushBoolean(eq_owner_count(@as(c_int, cdb.EQ_OWNER_OBJ), cdb.obj_id_get(obj), kind.ptr) > 0);
    return 1;
}

fn luaObjScriptEventNextMs(lua: *Lua) i32 {
    const handle = checkObjScriptHandle(lua);
    const obj = objScriptObject(lua, handle);
    const event_name = string(lua, 2);
    const script = std.mem.span(objScriptName(handle));
    var buf: [192:0]u8 = undefined;
    const kind = objScriptEventKind(&buf, script, event_name) orelse {
        lua.pushInteger(-1);
        return 1;
    };
    lua.pushInteger(eq_owner_next_ms(@as(c_int, cdb.EQ_OWNER_OBJ), cdb.obj_id_get(obj), kind.ptr));
    return 1;
}

// ---- Object-level script entity methods ----

fn luaObjectScriptAdd(lua: *Lua) i32 {
    lua.pushBoolean(cdb.obj_script_add(checkObject(lua), string(lua, 2)));
    return 1;
}

fn luaObjectScriptRemove(lua: *Lua) i32 {
    const reason: [*:0]const u8 = if (lua.isNoneOrNil(3)) "removed" else string(lua, 3);
    lua.pushBoolean(cdb.obj_script_remove(checkObject(lua), string(lua, 2), reason));
    return 1;
}

fn luaObjectScriptHas(lua: *Lua) i32 {
    lua.pushBoolean(cdb.obj_script_has(checkObject(lua), string(lua, 2)));
    return 1;
}

fn luaObjectScript(lua: *Lua) i32 {
    const obj = checkObject(lua);
    const script_id = string(lua, 2);
    if (!cdb.obj_script_has(obj, script_id)) {
        lua.pushNil();
        return 1;
    }
    pushObjectScript(lua, cdb.obj_id_get(obj), script_id);
    return 1;
}

fn luaObjectScripts(lua: *Lua) i32 {
    const obj = checkObject(lua);
    lua.newTable();
    var maybe_iter = object_api.objectScriptIterator(obj);
    if (maybe_iter) |*iter| {
        var i: zlua.Integer = 1;
        while (iter.next()) |entry| {
            pushObjectScript(lua, cdb.obj_id_get(obj), entry.name);
            lua.setIndex(-2, i);
            i += 1;
        }
    }
    return valueIterator(lua);
}

fn luaObjectScriptNumberGet(lua: *Lua) i32 {
    lua.pushInteger(cdb.obj_script_number_get(checkObject(lua), string(lua, 2), string(lua, 3)));
    return 1;
}

fn luaObjectScriptNumberSet(lua: *Lua) i32 {
    cdb.obj_script_number_set(checkObject(lua), string(lua, 2), string(lua, 3), intCastOrError(lua, i64, integer(lua, 4), "script number"));
    return 0;
}

fn luaObjectScriptTextGet(lua: *Lua) i32 {
    pushCString(lua, cdb.obj_script_text_get(checkObject(lua), string(lua, 2), string(lua, 3)));
    return 1;
}

fn luaObjectScriptTextSet(lua: *Lua) i32 {
    cdb.obj_script_text_set(checkObject(lua), string(lua, 2), string(lua, 3), string(lua, 4));
    return 0;
}
