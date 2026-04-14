from .loader import LegacyDatabase, prepare_migration
import sqlite3
from pathlib import Path

def int_to_blob(n: int, width: int = 16) -> bytes:
    if n < 0:
        raise ValueError("bitfield must be non-negative")
    return sqlite3.Binary(n.to_bytes(width, byteorder="little", signed=False))

def blob_to_int(b: bytes) -> int:
    return int.from_bytes(b, byteorder="little", signed=False)

class Migrator:
    def __init__(self, db: LegacyDatabase):
        self.db = db
        self.sql = sqlite3.connect("data/database.sqlite3")

    def prepare_schema(self):
        """
        Search in <cwd>/schemas for all .sql files, execute them in lexicographic order, each in their own transaction.
        """
        schema_path = Path.cwd() / "schema"
        sql_files = sorted(schema_path.glob("*.sql"))
        for sql_file in sql_files:
            with open(sql_file, "r") as f:
                sql_script = f.read()
            self.sql.executescript(sql_script)

    def _migrate_zones(self):
        query = "INSERT INTO zones (id, name, builders, lifespan, age, bot, top, reset_mode, min_level, max_level, zone_flags) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        query2 = "INSERT INTO zone_reset_commands (zone_id, command_order, command_type, if_flag, arg1, arg2, arg3, arg4, arg5, sarg1, sarg2) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        
        for k, v in self.db.zones.items():
            self.sql.execute(query, (v.id, v.name, v.builders, v.lifespan, v.age, v.bot, v.top, v.reset_mode, v.min_level, v.max_level, int_to_blob(v.zone_flags)))

            for i, cmd in enumerate(v.resets):
                self.sql.execute(query2, (v.id, i, cmd.command, cmd.if_flag, cmd.arg1, cmd.arg2, cmd.arg3, cmd.arg4, cmd.arg5, cmd.sa1, cmd.sa2))

    def _migrate_dgscripts(self):
        query = "INSERT INTO dgscript_prototypes (id, zone_id, attach_type, name, trigger_type, body, narg, arglist) VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
        for k, v in self.db.dgproto.items():
            zone_id = self.db.zone_id_for(v.id)
            self.sql.execute(query, (v.id, zone_id, v.attach_type, v.name, v.trigger_type, v.body, v.narg, v.command))

    def _migrate_rooms(self):
        query = "INSERT INTO rooms (id, zone_id, sector_type, name, description, room_flags) VALUES (?, ?, ?, ?, ?, ?)"
        for k, v in self.db.rooms.items():
            zone_id = self.db.zone_id_for(v.id)
            self.sql.execute(query, (v.id, zone_id, v.sector_type, v.name, v.description, int_to_blob(v.room_flags)))

        query = "INSERT INTO room_dgscript_prototypes (room_id, script_id) VALUES (?, ?)"
        for k, v in self.db.rooms.items():
            already = set()
            for script_id in v.proto_script:
                if script_id not in already:
                    self.sql.execute(query, (v.id, script_id))
                    already.add(script_id)

        query = "INSERT INTO exits (room_id, direction, to_room, keywords, description, key_id, exit_info, dclock, dchide) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
        for k, v in self.db.rooms.items():
            for direction, exit in v.exits.items():
                self.sql.execute(query, (v.id, direction, exit.to_room, exit.keywords, exit.description, exit.key, exit.exit_info, exit.dclock, exit.dchide))

    def _migrate_object_prototypes(self):
        query = "INSERT INTO object_prototypes (id, zone_id, name, short_description, description, action_description, extra_flags, wear_flags, bitvector, weight, cost, cost_per_day, level, size) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        query2 = "INSERT INTO object_prototype_item_values (object_prototype_id, val_index, val_number) VALUES (?, ?, ?)"
        query3 = "INSERT INTO object_prototype_extra_descriptions (object_prototype_id, keywords, description) VALUES (?, ?, ?)"
        query4 = "INSERT INTO object_prototype_dgscript_prototypes (object_prototype_id, script_id) VALUES (?, ?)"

        for k, v in self.db.oproto.items():
            zone_id = self.db.zone_id_for(v.id)
            self.sql.execute(query, (v.id, zone_id, v.name, v.short_description, v.description, v.action_description, int_to_blob(v.extra_flags), int_to_blob(v.wear_flags), int_to_blob(v.bitvector), v.weight, v.cost, v.cost_per_day, v.level, v.size))

            for i, val in enumerate(v.values):
                self.sql.execute(query2, (v.id, i, val))

            for ed in v.extra_descriptions:
                self.sql.execute(query3, (v.id, ed.keywords, ed.description))

            for script_id in v.proto_script:
                self.sql.execute(query4, (v.id, script_id))

    def _migrate_mobile_prototypes(self):
        query = "INSERT INTO mobile_prototypes (id, zone_id, name, short_descr, long_descr, description, title, sex, race, chclass, act, affected_by,level,alignment,str,intel,wis,dex,con,cha,default_position) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,?,?)"
        query2 = "INSERT INTO mobile_prototype_dgscript_prototypes (mobile_prototype_id, script_id) VALUES (?, ?)"

        for k, v in self.db.nproto.items():
            zone_id = self.db.zone_id_for(v.id)
            self.sql.execute(query, (v.id, zone_id, v.name, v.short_descr, v.long_descr, v.description, v.title, v.sex, v.race, v.chclass, int_to_blob(v.act), int_to_blob(v.affected_by), v.level, v.alignment, v.strength, v.intel, v.wis, v.dex, v.con, v.cha, v.default_position))

            for script_id in v.proto_script:
                self.sql.execute(query2, (v.id, script_id))

    def _migrate_shops(self):
        query = "INSERT INTo shops (id, zone_id, profit_buy, profit_sell, no_such_item1, no_such_item2, missing_cash1, missing_cash2, do_not_buy, message_buy, message_sell, temper1, bitvector, keeper, with_who, open1, close1, open2, close2, bankAccount) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        query2 = "INSERT INTO shop_products (shop_id, object_prototype_id) VALUES (?, ?)"
        query3 = "INSERT INTO shop_buys (shop_id, item_type, keywords) VALUES (?, ?, ?)"

        for k, v in self.db.shops.items():
            zone_id = self.db.zone_id_for(v.id)
            self.sql.execute(query, (v.id, zone_id, v.profit_buy, v.profit_sell, v.no_such_item1, v.no_such_item2, v.missing_cash1, v.missing_cash2, v.do_not_buy, v.message_buy, v.message_sell, v.temper1, v.shop_flags, v.keeper, int_to_blob(v.with_who), v.open1, v.close1, v.open2, v.close2, v.bankAccount))

            for object_id in v.products:
                products_addded = set()
                if object_id not in products_addded:
                    self.sql.execute(query2, (v.id, object_id))
                    products_addded.add(object_id)

            for sbs in v.types_bought:
                self.sql.execute(query3, (v.id, sbs.item_type, sbs.keywords))

    def _migrate_guilds(self):
        query = "INSERT INTO guilds (id, zone_id, charge, no_such_skill, not_enough_gold, minlvl, gm, with_who, open, close) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        query2 = "INSERT INTO guild_skills (guild_id, skill_id) VALUES (?, ?)"
        query3 = "INSERT INTO guild_feats (guild_id, feat_id) VALUES (?, ?)"

        for k, v in self.db.guilds.items():
            zone_id = self.db.zone_id_for(v.id)
            self.sql.execute(query, (v.id, zone_id, v.charge, v.no_such_skill, v.not_enough_gold, v.minlvl, v.keeper, int_to_blob(v.with_who), v.open, v.close))

            for teach in v.skills:
                self.sql.execute(query2, (v.id, teach))
            for teach in v.feats:
                self.sql.execute(query3, (v.id, teach))

    def _migrate_help(self):
        query = "INSERT INTO help_entries (keywords, entry, min_level) VALUES (?, ?, ?)"
        for v in self.db.help:
            self.sql.execute(query, (v.name, v.entry, v.min_level))

    def _migrate_assemblies(self):
        query = "INSERT INTO assemblies (object_prototype_id, assembly_type) VALUES (?, ?)"
        query2 = "INSERT INTO assembly_ingredients (assembly_id, object_prototype_id, consumed, in_room) VALUES (?, ?, ?, ?)"

        # this will generate new IDs for us, so we need to capture those for query
        for v in self.db.assemblies:
            cur = self.sql.execute(query, (v.object_prototype_id, 2))
            assembly_id = cur.lastrowid

            for ingredient in v.components:
                self.sql.execute(query2, (assembly_id, ingredient.object_prototype_id, ingredient.consumed, ingredient.in_room))

    def _migrate_houses(self):
        pass

    def _migrate_users(self):
        query = "INSERT INTO users (username, email, password_hash, rpp_points, rpp_bank, extra_slots, rpp_awarded) VALUES (?, ?, ?, ?, ?, ?, ?)"
        for username, v in self.db.accounts.items():
            self.sql.execute(query, (v.name, v.email, v.password, v.rpp, v.rpp_bank, v.slots - 3, v.rpp + v.rpp_bank))

    def _migrate_players(self):
        pass

    def migrate(self):
        # this is where we will put all the migration code. it will be a series of SQL commands that alter the database schema and update data as needed.
        # we will also keep track of the current schema version in a separate table, so we can run migrations incrementally.
        with self.sql as transaction:
            self.prepare_schema()
            self._migrate_zones()
            self._migrate_dgscripts()
            self._migrate_rooms()
            self._migrate_object_prototypes()
            self._migrate_mobile_prototypes()
            self._migrate_shops()
            self._migrate_guilds()
            self._migrate_help()
            self._migrate_assemblies()
            self._migrate_houses()
            self._migrate_users()
            self._migrate_players()



def main():
    path = Path.cwd() / "data"
    db = prepare_migration(path)
    migrator = Migrator(db)
    migrator.migrate()


if __name__ == "__main__":
    main()