const cdb = @import("cdb");
const std = @import("std");

var objs_by_id: std.AutoHashMap(i64, *cdb.obj_data) = undefined;

pub fn init(allocator: std.mem.Allocator) void {
    objs_by_id = std.AutoHashMap(i64, *cdb.obj_data).init(allocator);
}

pub fn deinit() void {
    objs_by_id.deinit();
}

pub export fn obj_by_id(id: i64) ?*cdb.obj_data {
    return objs_by_id.get(id) orelse null;
}

pub export fn obj_register_id(id: i64, obj: ?*cdb.obj_data) c_int {
    const ptr = obj orelse {
        _ = objs_by_id.remove(id);
        return 0;
    };

    objs_by_id.put(id, ptr) catch return -1;
    return 0;
}

pub export fn obj_unregister_id(id: i64) void {
    _ = objs_by_id.remove(id);
}
