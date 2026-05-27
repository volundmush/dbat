const std = @import("std");
const cdb = @import("cdb");
const jsonx = @import("flags_json.zig");

pub const JsonValue = jsonx.JsonValue;
pub const DeserializeOptions = struct { c_allocator: std.mem.Allocator = std.heap.c_allocator };

extern fn calloc(nmemb: usize, size: usize) ?*anyopaque;

pub fn serializeShop(allocator: std.mem.Allocator, shop: *cdb.shop_data) !JsonValue {
    var object = jsonx.newObject(allocator);
    try jsonx.putSlice(&object, allocator, "kind", "shop");
    try jsonx.putInt(&object, allocator, "id", cdb.shop_id_get(shop));
    try jsonx.putFloat(&object, allocator, "profit_buy", cdb.shop_profit_buy_get(shop));
    try jsonx.putFloat(&object, allocator, "profit_sell", cdb.shop_profit_sell_get(shop));
    try jsonx.putString(&object, allocator, "no_such_item1", cdb.shop_no_such_item1_get(shop));
    try jsonx.putString(&object, allocator, "no_such_item2", cdb.shop_no_such_item2_get(shop));
    try jsonx.putString(&object, allocator, "missing_cash1", cdb.shop_missing_cash1_get(shop));
    try jsonx.putString(&object, allocator, "missing_cash2", cdb.shop_missing_cash2_get(shop));
    try jsonx.putString(&object, allocator, "do_not_buy", cdb.shop_do_not_buy_get(shop));
    try jsonx.putString(&object, allocator, "message_buy", cdb.shop_message_buy_get(shop));
    try jsonx.putString(&object, allocator, "message_sell", cdb.shop_message_sell_get(shop));
    try jsonx.putInt(&object, allocator, "temper", cdb.shop_temper_get(shop));
    try jsonx.putInt(&object, allocator, "bitvector", shop.bitvector);
    try jsonx.putInt(&object, allocator, "keeper", cdb.shop_keeper_get(shop));
    try jsonx.putInt(&object, allocator, "open1", cdb.shop_open1_get(shop));
    try jsonx.putInt(&object, allocator, "open2", cdb.shop_open2_get(shop));
    try jsonx.putInt(&object, allocator, "close1", cdb.shop_close1_get(shop));
    try jsonx.putInt(&object, allocator, "close2", cdb.shop_close2_get(shop));
    try jsonx.putInt(&object, allocator, "bank", cdb.shop_bank_get(shop));
    try jsonx.putNonEmpty(&object, allocator, "producing", try serializeSentinelArray(allocator, shop.producing, cdb.NOTHING));
    try jsonx.putNonEmpty(&object, allocator, "in_room", try serializeSentinelArray(allocator, shop.in_room, cdb.NOWHERE));
    try jsonx.putNonEmpty(&object, allocator, "with_who", try jsonx.serializeFlags(allocator, shop, 128, shopTradeFlagged));
    return object;
}

pub fn deserializeShop(shop: *cdb.shop_data, options: DeserializeOptions, value: JsonValue) !void {
    if (value != .object) return error.ExpectedObject;
    if (try jsonx.intField(value, "id", cdb.shop_vnum)) |v| cdb.shop_id_set(shop, v);
    if (try jsonx.floatField(value, "profit_buy", f32)) |v| cdb.shop_profit_buy_set(shop, v);
    if (try jsonx.floatField(value, "profit_sell", f32)) |v| cdb.shop_profit_sell_set(shop, v);
    try setStringField(options.c_allocator, shop, value, "no_such_item1", cdb.shop_no_such_item1_set);
    try setStringField(options.c_allocator, shop, value, "no_such_item2", cdb.shop_no_such_item2_set);
    try setStringField(options.c_allocator, shop, value, "missing_cash1", cdb.shop_missing_cash1_set);
    try setStringField(options.c_allocator, shop, value, "missing_cash2", cdb.shop_missing_cash2_set);
    try setStringField(options.c_allocator, shop, value, "do_not_buy", cdb.shop_do_not_buy_set);
    try setStringField(options.c_allocator, shop, value, "message_buy", cdb.shop_message_buy_set);
    try setStringField(options.c_allocator, shop, value, "message_sell", cdb.shop_message_sell_set);
    if (try jsonx.intField(value, "temper", c_int)) |v| cdb.shop_temper_set(shop, v);
    if (try jsonx.intField(value, "bitvector", cdb.bitvector_t)) |v| shop.bitvector = v;
    if (try jsonx.intField(value, "keeper", cdb.mob_vnum)) |v| cdb.shop_keeper_set(shop, v);
    if (try jsonx.intField(value, "open1", c_int)) |v| cdb.shop_open1_set(shop, v);
    if (try jsonx.intField(value, "open2", c_int)) |v| cdb.shop_open2_set(shop, v);
    if (try jsonx.intField(value, "close1", c_int)) |v| cdb.shop_close1_set(shop, v);
    if (try jsonx.intField(value, "close2", c_int)) |v| cdb.shop_close2_set(shop, v);
    if (try jsonx.intField(value, "bank", c_int)) |v| cdb.shop_bank_set(shop, v);
    if (jsonx.field(value, "producing")) |items| shop.producing = try deserializeSentinelArray(cdb.obj_vnum, shop.producing, items, cdb.NOTHING);
    if (jsonx.field(value, "in_room")) |items| shop.in_room = try deserializeSentinelArray(cdb.room_vnum, shop.in_room, items, cdb.NOWHERE);
    if (jsonx.field(value, "with_who")) |flags| try jsonx.deserializeFlags(shop, flags, 128, shopTradeFlagSet);
}

fn serializeSentinelArray(allocator: std.mem.Allocator, values: anytype, sentinel: anytype) !JsonValue {
    var array = jsonx.JsonArray.init(allocator);
    if (values == null) return .{ .array = array };
    var index: usize = 0;
    while (values[index] != sentinel) : (index += 1) {
        try array.append(.{ .integer = @intCast(values[index]) });
    }
    return .{ .array = array };
}

fn deserializeSentinelArray(comptime T: type, old: [*c]T, value: JsonValue, sentinel: T) ![*c]T {
    if (value != .array) return error.ExpectedArray;
    if (old != null) std.c.free(old);
    const result: [*c]T = @ptrCast(@alignCast(calloc(value.array.items.len + 1, @sizeOf(T)) orelse return error.OutOfMemory));
    for (value.array.items, 0..) |item, index| {
        if (item != .integer) return error.ExpectedInteger;
        result[index] = std.math.cast(T, item.integer) orelse return error.IntegerOutOfRange;
    }
    result[value.array.items.len] = sentinel;
    return result;
}

fn setString(allocator: std.mem.Allocator, shop: *cdb.shop_data, value: []const u8, comptime setter: anytype) !void {
    const z = try allocator.dupeZ(u8, value);
    defer allocator.free(z);
    setter(shop, z);
}

fn setStringField(allocator: std.mem.Allocator, shop: *cdb.shop_data, object: JsonValue, key: []const u8, comptime setter: anytype) !void {
    const value = try jsonx.stringFieldAlloc(allocator, object, key) orelse return;
    defer allocator.free(value);
    try setString(allocator, shop, value, setter);
}

fn shopTradeFlagged(shop: *cdb.shop_data, pos: c_int) bool {
    return cdb.shop_trade_flagged(shop, pos);
}

fn shopTradeFlagSet(shop: *cdb.shop_data, pos: c_int, value: bool) void {
    cdb.shop_trade_flag_set(shop, pos, value);
}
