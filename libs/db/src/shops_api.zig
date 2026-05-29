const cdb = @import("cdb");
const std = @import("std");
const bitflags = @import("flags.zig");

extern fn strdup(s: [*:0]const u8) ?[*:0]u8;

pub export fn shop_id_get(shop: *cdb.shop_data) cdb.shop_vnum {
    return shop.vnum;
}
pub export fn shop_id_set(shop: *cdb.shop_data, id: cdb.shop_vnum) void {
    shop.vnum = id;
}
pub export fn shop_profit_buy_get(shop: *cdb.shop_data) f32 {
    return shop.profit_buy;
}
pub export fn shop_profit_buy_set(shop: *cdb.shop_data, value: f32) void {
    shop.profit_buy = value;
}
pub export fn shop_profit_sell_get(shop: *cdb.shop_data) f32 {
    return shop.profit_sell;
}
pub export fn shop_profit_sell_set(shop: *cdb.shop_data, value: f32) void {
    shop.profit_sell = value;
}

pub export fn shop_no_such_item1_get(shop: *cdb.shop_data) [*c]const u8 {
    return shop.no_such_item1;
}
pub export fn shop_no_such_item1_set(shop: *cdb.shop_data, value: ?[*:0]const u8) void {
    replaceString(&shop.no_such_item1, value);
}
pub export fn shop_no_such_item2_get(shop: *cdb.shop_data) [*c]const u8 {
    return shop.no_such_item2;
}
pub export fn shop_no_such_item2_set(shop: *cdb.shop_data, value: ?[*:0]const u8) void {
    replaceString(&shop.no_such_item2, value);
}
pub export fn shop_missing_cash1_get(shop: *cdb.shop_data) [*c]const u8 {
    return shop.missing_cash1;
}
pub export fn shop_missing_cash1_set(shop: *cdb.shop_data, value: ?[*:0]const u8) void {
    replaceString(&shop.missing_cash1, value);
}
pub export fn shop_missing_cash2_get(shop: *cdb.shop_data) [*c]const u8 {
    return shop.missing_cash2;
}
pub export fn shop_missing_cash2_set(shop: *cdb.shop_data, value: ?[*:0]const u8) void {
    replaceString(&shop.missing_cash2, value);
}
pub export fn shop_do_not_buy_get(shop: *cdb.shop_data) [*c]const u8 {
    return shop.do_not_buy;
}
pub export fn shop_do_not_buy_set(shop: *cdb.shop_data, value: ?[*:0]const u8) void {
    replaceString(&shop.do_not_buy, value);
}
pub export fn shop_message_buy_get(shop: *cdb.shop_data) [*c]const u8 {
    return shop.message_buy;
}
pub export fn shop_message_buy_set(shop: *cdb.shop_data, value: ?[*:0]const u8) void {
    replaceString(&shop.message_buy, value);
}
pub export fn shop_message_sell_get(shop: *cdb.shop_data) [*c]const u8 {
    return shop.message_sell;
}
pub export fn shop_message_sell_set(shop: *cdb.shop_data, value: ?[*:0]const u8) void {
    replaceString(&shop.message_sell, value);
}

pub export fn shop_temper_get(shop: *cdb.shop_data) c_int {
    return shop.temper1;
}
pub export fn shop_temper_set(shop: *cdb.shop_data, temper: c_int) void {
    shop.temper1 = temper;
}
pub export fn shop_flagged(shop: *cdb.shop_data, pos: c_int) bool {
    return (shop.bitvector & bitMask(pos)) != 0;
}
pub export fn shop_flag_toggle(shop: *cdb.shop_data, pos: c_int) bool {
    const mask = bitMask(pos);
    shop.bitvector ^= mask;
    return (shop.bitvector & mask) != 0;
}
pub export fn shop_flag_set(shop: *cdb.shop_data, pos: c_int, value: bool) void {
    setSingle(&shop.bitvector, pos, value);
}
pub export fn shop_keeper_get(shop: *cdb.shop_data) cdb.mob_vnum {
    return shop.keeper;
}
pub export fn shop_keeper_set(shop: *cdb.shop_data, vnum: cdb.mob_vnum) void {
    shop.keeper = vnum;
}
pub export fn shop_trade_flagged(shop: *cdb.shop_data, pos: c_int) bool {
    return bitflags.get(&shop.with_who, pos);
}
pub export fn shop_trade_flag_toggle(shop: *cdb.shop_data, pos: c_int) bool {
    return bitflags.toggle(&shop.with_who, pos);
}
pub export fn shop_trade_flag_set(shop: *cdb.shop_data, pos: c_int, value: bool) void {
    bitflags.set(&shop.with_who, pos, value);
}

pub export fn shop_open1_get(shop: *cdb.shop_data) c_int {
    return shop.open1;
}
pub export fn shop_open1_set(shop: *cdb.shop_data, value: c_int) void {
    shop.open1 = value;
}
pub export fn shop_open2_get(shop: *cdb.shop_data) c_int {
    return shop.open2;
}
pub export fn shop_open2_set(shop: *cdb.shop_data, value: c_int) void {
    shop.open2 = value;
}
pub export fn shop_close1_get(shop: *cdb.shop_data) c_int {
    return shop.close1;
}
pub export fn shop_close1_set(shop: *cdb.shop_data, value: c_int) void {
    shop.close1 = value;
}
pub export fn shop_close2_get(shop: *cdb.shop_data) c_int {
    return shop.close2;
}
pub export fn shop_close2_set(shop: *cdb.shop_data, value: c_int) void {
    shop.close2 = value;
}
pub export fn shop_bank_get(shop: *cdb.shop_data) c_int {
    return shop.bankAccount;
}
pub export fn shop_bank_set(shop: *cdb.shop_data, value: c_int) void {
    shop.bankAccount = value;
}
pub export fn shop_lastsort_get(shop: *cdb.shop_data) c_int {
    return shop.lastsort;
}
pub export fn shop_lastsort_set(shop: *cdb.shop_data, value: c_int) void {
    shop.lastsort = value;
}
pub export fn shop_func_get(shop: *cdb.shop_data) cdb.SpecialFunc {
    return shop.func;
}
pub export fn shop_func_set(shop: *cdb.shop_data, func: cdb.SpecialFunc) void {
    shop.func = func;
}

pub export fn shop_product_get(shop: *cdb.shop_data, index: usize) cdb.obj_vnum {
    if (shop.producing == null) return cdb.NOTHING;
    return shop.producing[index];
}
pub export fn shop_product_set(shop: *cdb.shop_data, index: usize, vnum: cdb.obj_vnum) void {
    if (shop.producing == null) return;
    shop.producing[index] = vnum;
}
pub export fn shop_room_get(shop: *cdb.shop_data, index: usize) cdb.room_vnum {
    if (shop.in_room == null) return cdb.NOWHERE;
    return shop.in_room[index];
}
pub export fn shop_room_set(shop: *cdb.shop_data, index: usize, vnum: cdb.room_vnum) void {
    if (shop.in_room == null) return;
    shop.in_room[index] = vnum;
}
pub export fn shop_buy_type_get(shop: *cdb.shop_data, index: usize) [*c]cdb.shop_buy_data {
    if (shop.type == null) return null;
    return &shop.type[index];
}

pub export fn shop_buy_data_type_get(data: *cdb.shop_buy_data) c_int {
    return data.type;
}
pub export fn shop_buy_data_type_set(data: *cdb.shop_buy_data, buy_type: c_int) void {
    data.type = buy_type;
}
pub export fn shop_buy_data_keywords_get(data: *cdb.shop_buy_data) [*c]const u8 {
    return data.keywords;
}
pub export fn shop_buy_data_keywords_set(data: *cdb.shop_buy_data, keywords: ?[*:0]const u8) void {
    replaceString(&data.keywords, keywords);
}

fn replaceString(field: *[*c]u8, value: ?[*:0]const u8) void {
    const new_value = if (value) |s| strdup(s) orelse return else null;
    if (field.* != null) std.c.free(field.*);
    field.* = new_value;
}
fn bitMask(pos: c_int) cdb.bitvector_t {
    if (pos < 0 or pos >= @bitSizeOf(cdb.bitvector_t)) return 0;
    return @as(cdb.bitvector_t, 1) << @intCast(pos);
}
fn setSingle(value: *cdb.bitvector_t, pos: c_int, enabled: bool) void {
    const mask = bitMask(pos);
    if (enabled) value.* |= mask else value.* &= ~mask;
}
