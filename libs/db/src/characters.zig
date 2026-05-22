const cdb = @import("cdb");
const std = @import("std");

var chars_by_id: std.AutoHashMap(i64, *cdb.char_data) = undefined;

pub fn init(allocator: std.mem.Allocator) void {
    chars_by_id = std.AutoHashMap(i64, *cdb.char_data).init(allocator);
}

pub fn deinit() void {
    chars_by_id.deinit();
}

pub export fn char_by_id(id: i64) ?*cdb.char_data {
    return chars_by_id.get(id) orelse null;
}

pub export fn char_register_id(id: i64, ch: ?*cdb.char_data) c_int {
    const ptr = ch orelse {
        _ = chars_by_id.remove(id);
        return 0;
    };

    chars_by_id.put(id, ptr) catch return -1;
    return 0;
}

pub export fn char_unregister_id(id: i64) void {
    _ = chars_by_id.remove(id);
}
