const cdb = @import("cdb");
const std = @import("std");
const characters = @import("characters.zig");
const bitflags = @import("flags.zig");
const obj_api = @import("objects_api.zig");

const TransformData = struct {
    // this is a placeholder for now.
};

const DerivedData = struct {
    // this is a placeholder for now.
};

const CharacterData = struct {
    stats: std.StringHashMap(i64),
    deriveds: std.StringHashMap(DerivedData),
    transforms: std.StringHashMap(TransformData),

    pub fn init(alloc: std.mem.Allocator) CharacterData {
        return CharacterData{
            .stats = std.StringHashMap(i64).init(alloc),
            .deriveds = std.StringHashMap(DerivedData).init(alloc),
            .transforms = std.StringHashMap(TransformData).init(alloc),
        };
    }

    pub fn deinit(self: *CharacterData) void {
        var stats = self.stats.keyIterator();
        while (stats.next()) |key| std.heap.page_allocator.free(key.*);
        self.stats.deinit();
        self.deriveds.deinit();
        self.transforms.deinit();
    }
};

extern fn strdup(s: [*:0]const u8) ?[*:0]u8;

pub fn char_ensure_zigdata(ch: *cdb.char_data) ?*CharacterData {
    if (ch.zigdata == null) {
        const data = std.heap.page_allocator.create(CharacterData) catch return null;
        data.* = CharacterData.init(std.heap.page_allocator);
        ch.zigdata = data;
    }
    return @ptrCast(@alignCast(ch.zigdata.?));
}

pub export fn char_id_get(ch: *cdb.char_data) i64 {
    return ch.id;
}

pub export fn char_id_set(ch: *cdb.char_data, id: i64) void {
    ch.id = @intCast(id);
}

pub export fn char_proto_id_get(ch: *cdb.char_data) cdb.mob_vnum {
    return char_vnum_get(ch);
}

pub export fn char_proto_id_set(ch: *cdb.char_data, vnum: cdb.mob_vnum) void {
    char_vnum_set(ch, vnum);
}

pub export fn char_vnum_get(ch: *cdb.char_data) cdb.mob_vnum {
    if (!validMobRnum(ch.nr)) return cdb.NOTHING;
    return cdb.mob_index[@intCast(ch.nr)].vnum;
}

pub export fn char_vnum_set(ch: *cdb.char_data, vnum: cdb.mob_vnum) void {
    ch.nr = cdb.real_mobile(vnum);
}

pub export fn char_room_get(ch: *cdb.char_data) [*c]cdb.room_data {
    if (!validRoomRnum(ch.in_room)) return null;
    return &cdb.world[@intCast(ch.in_room)];
}

pub export fn char_room_vnum_get(ch: *cdb.char_data) cdb.room_vnum {
    const room = char_room_get(ch);
    if (room == null) return cdb.NOWHERE;
    return room.*.number;
}

pub export fn char_zone_get(ch: *cdb.char_data) [*c]cdb.zone_data {
    const room = char_room_get(ch);
    if (room == null) return null;
    return &cdb.zone_table[@intCast(room.*.zone)];
}

pub export fn char_zone_vnum_get(ch: *cdb.char_data) cdb.zone_vnum {
    const zone = char_zone_get(ch);
    if (zone == null) return cdb.NOWHERE;
    return zone.*.number;
}

pub export fn char_room_vnum_set(ch: *cdb.char_data, vnum: cdb.room_vnum) void {
    ch.in_room = cdb.real_room(vnum);
}

pub export fn char_name_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.name;
}

pub export fn char_name_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(ch, &ch.name, protoString(ch, .name), value);
}

pub export fn char_description_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.description;
}

pub export fn char_description_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(ch, &ch.description, protoString(ch, .description), value);
}

pub export fn char_short_description_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.short_descr;
}

pub export fn char_short_description_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(ch, &ch.short_descr, protoString(ch, .short_descr), value);
}

pub export fn char_long_description_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.long_descr;
}

pub export fn char_long_description_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(ch, &ch.long_descr, protoString(ch, .long_descr), value);
}

pub export fn char_title_get(ch: *cdb.char_data) [*c]const u8 {
    return ch.title;
}

pub export fn char_title_set(ch: *cdb.char_data, value: ?[*:0]const u8) void {
    replaceString(ch, &ch.title, protoString(ch, .title), value);
}

pub export fn char_class_get(ch: *cdb.char_data) c_int {
    return ch.chclass;
}

pub export fn char_class_set(ch: *cdb.char_data, chclass: c_int) void {
    ch.chclass = chclass;
}

pub export fn char_race_get(ch: *cdb.char_data) c_int {
    return ch.race;
}

pub export fn char_race_set(ch: *cdb.char_data, race: c_int) void {
    ch.race = race;
}

pub export fn char_size_get(ch: *cdb.char_data) c_int {
    return ch.size;
}

pub export fn char_size_set(ch: *cdb.char_data, size: c_int) void {
    ch.size = size;
}

pub export fn char_sex_get(ch: *cdb.char_data) c_int {
    return ch.sex;
}

pub export fn char_sex_set(ch: *cdb.char_data, sex: c_int) void {
    ch.sex = @intCast(sex);
}

pub export fn char_admlevel_get(ch: *cdb.char_data) c_int {
    return ch.admlevel;
}

pub export fn char_admlevel_set(ch: *cdb.char_data, admlevel: c_int) void {
    ch.admlevel = admlevel;
}

pub export fn char_admflagged(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.get(&ch.admflags, pos);
}

pub export fn char_admflag_toggle(ch: *cdb.char_data, pos: c_int) bool {
    return bitflags.toggle(&ch.admflags, pos);
}

pub export fn char_admflag_set(ch: *cdb.char_data, pos: c_int, value: bool) void {
    bitflags.set(&ch.admflags, pos, value);
}

pub export fn char_inventory_iterate(ch: *cdb.char_data, recursive: bool, func: ?obj_api.ObjIterFn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    _ = obj_api.objContentsListIterate(ch.carrying, recursive, callback, ctx);
}

pub export fn char_equipment_iterate(ch: *cdb.char_data, recursive: bool, func: ?obj_api.ObjIterFn, ctx: ?*anyopaque) void {
    const callback = func orelse return;
    for (ch.equipment) |obj| {
        if (obj == null) continue;
        if (!callback(&obj.*, ctx)) return;
        if (recursive and !obj_api.objContentsListIterate(obj.*.contains, true, callback, ctx)) return;
    }
}

pub export fn char_inventory_count(ch: *cdb.char_data, recursive: bool) usize {
    var count: usize = 0;
    var current = ch.carrying;
    while (current != null) : (current = current.*.next_content) {
        count += 1;
        if (recursive) count += obj_api.obj_inventory_count(&current.*, true);
    }
    return count;
}

pub export fn char_equipment_count(ch: *cdb.char_data, recursive: bool) usize {
    var count: usize = 0;
    for (ch.equipment) |obj| {
        if (obj == null) continue;
        count += 1;
        if (recursive) count += obj_api.obj_inventory_count(&obj.*, true);
    }
    return count;
}

pub export fn char_inventory_get(ch: *cdb.char_data, pos: usize) [*c]cdb.obj_data {
    var index: usize = 0;
    var current = ch.carrying;
    while (current != null) : (current = current.*.next_content) {
        if (index == pos) return current;
        index += 1;
    }
    return null;
}

pub export fn char_equipment_get(ch: *cdb.char_data, pos: usize) [*c]cdb.obj_data {
    if (pos >= ch.equipment.len) return null;
    return ch.equipment[pos];
}

fn validMobRnum(rnum: cdb.mob_rnum) bool {
    return rnum != cdb.NOBODY and rnum >= 0 and rnum <= cdb.top_of_mobt and cdb.mob_index != null;
}

fn validRoomRnum(rnum: cdb.room_rnum) bool {
    return rnum != cdb.NOWHERE and rnum >= 0 and rnum <= cdb.top_of_world and cdb.world != null;
}

fn validProto(ch: *cdb.char_data) bool {
    return validMobRnum(ch.nr) and cdb.mob_proto != null;
}

const StringField = enum { name, description, short_descr, long_descr, title };

fn protoString(ch: *cdb.char_data, field: StringField) [*c]u8 {
    if (!validProto(ch)) return null;
    const proto = &cdb.mob_proto[@intCast(ch.nr)];
    return switch (field) {
        .name => proto.name,
        .description => proto.description,
        .short_descr => proto.short_descr,
        .long_descr => proto.long_descr,
        .title => proto.title,
    };
}

fn replaceString(ch: *cdb.char_data, field: *[*c]u8, proto_value: [*c]u8, value: ?[*:0]const u8) void {
    _ = ch;
    const new_value = if (value) |new_string| strdup(new_string) orelse return else null;
    if (field.* != null and field.* != proto_value) std.c.free(field.*);
    field.* = new_value;
}

pub export fn char_stat_get(ch: *cdb.char_data, stat: ?[*:0]const u8) i64 {
    if (stat == null) return 0;
    const zigdata = char_ensure_zigdata(ch) orelse return 0;
    return zigdata.stats.get(std.mem.span(stat.?)) orelse 0;
}

pub export fn char_stat_set(ch: *cdb.char_data, stat: ?[*:0]const u8, value: i64) i64 {
    const name = if (stat) |ptr| std.mem.span(ptr) else return 0;
    if (name.len == 0) return 0;

    const zigdata = char_ensure_zigdata(ch) orelse return 0;
    if (zigdata.stats.getPtr(name)) |existing| {
        existing.* = value;
        return value;
    }

    const owned_name = std.heap.page_allocator.dupe(u8, name) catch return 0;
    zigdata.stats.put(owned_name, value) catch {
        std.heap.page_allocator.free(owned_name);
        return 0;
    };
    return value;
}

pub export fn char_stat_mod(ch: *cdb.char_data, stat: ?[*:0]const u8, mod: i64) i64 {
    const value = char_stat_get(ch, stat) + mod;
    return char_stat_set(ch, stat, value);
}
