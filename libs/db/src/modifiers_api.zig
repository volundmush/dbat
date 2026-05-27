const std = @import("std");
const cdb = @import("cdb");

pub const scale: i64 = 10000;

pub const ModifierKind = enum {
    flat,
    percent,
    multiplier,
    override_min,
    override_max,
    set,
};

pub const Modifier = struct {
    source_category: []const u8,
    source_id: []const u8,
    target_category: []const u8,
    target_id: []const u8,
    kind: ModifierKind,
    value: i64,
    priority: i32 = 0,
    label: []const u8,
    owned_strings: bool = false,
};

pub const ModifierCache = struct {
    allocator: std.mem.Allocator,
    by_target: std.StringHashMap(std.array_list.Managed(Modifier)),
    dirty: bool,

    pub fn init(allocator: std.mem.Allocator) ModifierCache {
        return .{
            .allocator = allocator,
            .by_target = std.StringHashMap(std.array_list.Managed(Modifier)).init(allocator),
            .dirty = true,
        };
    }

    pub fn deinit(self: *ModifierCache) void {
        self.clear();
        self.by_target.deinit();
    }

    pub fn invalidate(self: *ModifierCache) void {
        self.dirty = true;
    }

    pub fn rebuild(self: *ModifierCache, ch: *cdb.char_data) void {
        _ = ch;
        self.clear();
        self.dirty = false;
    }

    pub fn modifiersFor(self: *ModifierCache, category: []const u8, id: []const u8) ?[]const Modifier {
        const key = targetKey(self.allocator, category, id) catch return null;
        defer self.allocator.free(key);
        const list = self.by_target.getPtr(key) orelse return null;
        return list.items;
    }

    pub fn add(self: *ModifierCache, modifier: Modifier) !void {
        const key = try targetKey(self.allocator, modifier.target_category, modifier.target_id);
        if (self.by_target.getPtr(key)) |list| {
            self.allocator.free(key);
            try list.append(modifier);
            return;
        }

        var list = std.array_list.Managed(Modifier).init(self.allocator);
        errdefer list.deinit();
        try list.append(modifier);
        try self.by_target.put(key, list);
    }

    fn clear(self: *ModifierCache) void {
        var it = self.by_target.iterator();
        while (it.next()) |entry| {
            self.allocator.free(entry.key_ptr.*);
            for (entry.value_ptr.items) |modifier| {
                if (!modifier.owned_strings) continue;
                self.allocator.free(modifier.source_category);
                self.allocator.free(modifier.source_id);
                self.allocator.free(modifier.target_category);
                self.allocator.free(modifier.target_id);
                self.allocator.free(modifier.label);
            }
            entry.value_ptr.deinit();
        }
        self.by_target.clearRetainingCapacity();
    }
};

fn targetKey(allocator: std.mem.Allocator, category: []const u8, id: []const u8) ![]u8 {
    return std.fmt.allocPrint(allocator, "{s}:{s}", .{ category, id });
}

pub fn addLegacyDerivedFlat(cache: *ModifierCache, ch: *cdb.char_data, target_id: []const u8, location: c_int, specific: c_int) void {
    const value = cdb.char_legacy_modifier(ch, location, specific);
    if (value == 0) return;
    cache.add(.{
        .source_category = "legacy",
        .source_id = "affects",
        .target_category = "derived",
        .target_id = target_id,
        .kind = .flat,
        .value = value,
        .label = "Legacy affects",
    }) catch {};
}
