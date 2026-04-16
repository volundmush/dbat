from .loader import LegacyDatabase, prepare_migration
import sqlite3
from pathlib import Path


def flag_parts(flag_handler) -> tuple[int, int, int, int]:
    values = list(flag_handler.array)
    if len(values) < 4:
        values.extend([0] * (4 - len(values)))
    return tuple(values[:4])


def nullable_fk(value: int | None) -> int | None:
    if value is None or value <= 0:
        return None
    return value


def as_int(value, default: int = 0) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


class Migrator:
    def __init__(self, db: LegacyDatabase):
        self.db = db
        self.sql = sqlite3.connect("data/database.sqlite3")
        self.user_ids: dict[str, int] = {}

    def _migrate_zones(self):
        query = """
        INSERT INTO zones (
            id, name, builders, lifespan, age, bot, top,
            reset_mode, min_level, max_level,
            zone_flags0, zone_flags1, zone_flags2, zone_flags3
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        query2 = """
        INSERT INTO zone_reset_commands (
            zone_id, command_order, command_type, if_flag,
            arg1, arg2, arg3, arg4, arg5, sarg1, sarg2
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """

        for _, v in self.db.zones.items():
            self.sql.execute(
                query,
                (
                    v.id,
                    v.name,
                    v.builders,
                    v.lifespan,
                    v.age,
                    v.bot,
                    v.top,
                    v.reset_mode,
                    v.min_level,
                    v.max_level,
                    *flag_parts(v.zone_flags),
                ),
            )

            for i, cmd in enumerate(v.resets):
                self.sql.execute(
                    query2,
                    (
                        v.id,
                        i,
                        cmd.command,
                        cmd.if_flag,
                        cmd.arg1,
                        cmd.arg2,
                        cmd.arg3,
                        cmd.arg4,
                        cmd.arg5,
                        cmd.sa1,
                        cmd.sa2,
                    ),
                )

    def _migrate_dgscripts(self):
        query = """
        INSERT INTO dgscript_prototypes (
            id, zone_id, attach_type, name,
            trigger_type, body, narg, arglist
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """
        for _, v in self.db.dgproto.items():
            zone_id = self.db.zone_id_for(v.id)
            self.sql.execute(
                query,
                (v.id, zone_id, v.attach_type, v.name, v.trigger_type, v.body, v.narg, v.command),
            )

    def _migrate_rooms(self):
        query = """
        INSERT INTO rooms (
            id, zone_id, sector_type, name, description,
            room_flags0, room_flags1, room_flags2, room_flags3
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        for _, v in self.db.rooms.items():
            zone_id = self.db.zone_id_for(v.id)
            self.sql.execute(
                query,
                (
                    v.id,
                    zone_id,
                    v.sector_type,
                    v.name,
                    v.description,
                    *flag_parts(v.room_flags),
                ),
            )

        query = """
        INSERT INTO room_extra_descriptions (room_id, keywords, description)
        VALUES (?, ?, ?)
        """
        for _, v in self.db.rooms.items():
            for ed in v.extra_descriptions:
                self.sql.execute(query, (v.id, ed.keywords, ed.description))

        query = """
        INSERT INTO room_dgscript_prototypes (room_id, script_id)
        VALUES (?, ?)
        """
        for _, v in self.db.rooms.items():
            already = set()
            for script_id in v.proto_script:
                if script_id not in already:
                    self.sql.execute(query, (v.id, script_id))
                    already.add(script_id)

        query = """
        INSERT INTO exits (
            room_id, direction, to_room, keywords, description,
            key_id, exit_info, dclock, dchide
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        for _, v in self.db.rooms.items():
            for direction, exit in v.exits.items():
                self.sql.execute(
                    query,
                    (
                        v.id,
                        direction,
                        exit.to_room,
                        exit.keywords,
                        exit.description,
                        nullable_fk(exit.key),
                        exit.exit_info,
                        exit.dclock,
                        exit.dchide,
                    ),
                )

    def _migrate_object_prototypes(self):
        query = """
        INSERT INTO object_prototypes (
            id, zone_id, name, short_description, description, action_description,
            extra_flags0, extra_flags1, extra_flags2, extra_flags3,
            wear_flags0, wear_flags1, wear_flags2, wear_flags3,
            bitvector0, bitvector1, bitvector2, bitvector3,
            type_flag, weight, cost, level, size
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        query2 = """
        INSERT INTO object_prototype_item_values (object_prototype_id, val_index, val_number)
        VALUES (?, ?, ?)
        """
        query3 = """
        INSERT INTO object_prototype_extra_descriptions (object_prototype_id, keywords, description)
        VALUES (?, ?, ?)
        """
        query4 = """
        INSERT INTO object_prototype_dgscript_prototypes (object_prototype_id, script_id)
        VALUES (?, ?)
        """
        query5 = """
        INSERT INTO object_prototype_affects (
            object_prototype_id, location, modifier, specific
        ) VALUES (?, ?, ?, ?)
        """

        for _, v in self.db.oproto.items():
            zone_id = self.db.zone_id_for(v.id)
            self.sql.execute(
                query,
                (
                    v.id,
                    zone_id,
                    v.name,
                    v.short_description,
                    v.description,
                    v.action_description,
                    *flag_parts(v.extra_flags),
                    *flag_parts(v.wear_flags),
                    *flag_parts(v.bitvector),
                    v.item_type,
                    v.weight,
                    v.cost,
                    v.level,
                    v.size,
                ),
            )

            for val_index, val_number in v.values.items():
                self.sql.execute(query2, (v.id, val_index, val_number))

            for ed in v.extra_descriptions:
                self.sql.execute(query3, (v.id, ed.keywords, ed.description))

            for script_id in v.proto_script:
                self.sql.execute(query4, (v.id, script_id))

            for aff in v.affected:
                self.sql.execute(
                    query5,
                    (
                        v.id,
                        as_int(getattr(aff, "location", 0)),
                        as_int(getattr(aff, "modifier", 0)),
                        as_int(getattr(aff, "specific", 0)),
                    ),
                )

    def _migrate_mobile_prototypes(self):
        query = """
        INSERT INTO mobile_prototypes (
            id, zone_id, name, short_descr, long_descr, description,
            size, sex, race, chclass,
            act0, act1, act2, act3,
            affected_by0, affected_by1, affected_by2, affected_by3,
            alignment,
            str, intel, wis, dex, con, cha,
            position, level, race_level, level_adj,
            armor, basepl, baseki, basest, gold,
            default_position
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        query2 = """
        INSERT INTO mobile_prototype_dgscript_prototypes (mobile_prototype_id, script_id)
        VALUES (?, ?)
        """

        for _, v in self.db.nproto.items():
            zone_id = self.db.zone_id_for(v.id)
            self.sql.execute(
                query,
                (
                    v.id,
                    zone_id,
                    v.name,
                    v.short_descr,
                    v.long_descr,
                    v.description,
                    v.size,
                    v.sex,
                    v.race,
                    v.chclass,
                    *flag_parts(v.act),
                    *flag_parts(v.affected_by),
                    v.alignment,
                    v.strength,
                    v.intel,
                    v.wis,
                    v.dex,
                    v.con,
                    v.cha,
                    0,
                    v.level,
                    v.race_level,
                    v.level_adj,
                    v.armor,
                    v.basepl,
                    v.baseki,
                    v.basest,
                    v.gold,
                    v.default_position,
                ),
            )

            for script_id in v.proto_script:
                self.sql.execute(query2, (v.id, script_id))

    def _migrate_shops(self):
        query = """
        INSERT INTO shops (
            id, zone_id, profit_buy, profit_sell,
            no_such_item1, no_such_item2,
            missing_cash1, missing_cash2,
            do_not_buy, message_buy, message_sell,
            temper1, bitvector, keeper,
            with_who0, with_who1, with_who2, with_who3,
            open1, close1, open2, close2, bankAccount
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        query2 = """
        INSERT INTO shop_products (shop_id, object_prototype_id)
        VALUES (?, ?)
        """
        query3 = """
        INSERT INTO shop_buys (shop_id, item_type, keywords)
        VALUES (?, ?, ?)
        """
        query4 = """
        INSERT INTO shop_rooms (shop_id, room_id)
        VALUES (?, ?)
        """

        for _, v in self.db.shops.items():
            zone_id = self.db.zone_id_for(v.id)
            self.sql.execute(
                query,
                (
                    v.id,
                    zone_id,
                    v.profit_buy,
                    v.profit_sell,
                    v.no_such_item1,
                    v.no_such_item2,
                    v.missing_cash1,
                    v.missing_cash2,
                    v.do_not_buy,
                    v.message_buy,
                    v.message_sell,
                    v.temper1,
                    v.shop_flags,
                    nullable_fk(v.keeper),
                    *flag_parts(v.with_who),
                    v.open1,
                    v.close1,
                    v.open2,
                    v.close2,
                    nullable_fk(v.bankAccount),
                ),
            )

            products_added = set()
            for object_id in v.products:
                if object_id not in products_added:
                    self.sql.execute(query2, (v.id, object_id))
                    products_added.add(object_id)

            rooms_added = set()
            for room_id in v.in_room:
                if room_id not in rooms_added:
                    self.sql.execute(query4, (v.id, room_id))
                    rooms_added.add(room_id)

            for sbs in v.types_bought:
                self.sql.execute(query3, (v.id, sbs.item_type, sbs.keywords))

    def _migrate_guilds(self):
        query = """
        INSERT INTO guilds (
            id, zone_id, charge, no_such_skill, not_enough_gold,
            minlvl, gm,
            with_who0, with_who1, with_who2, with_who3,
            open, close
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        query2 = """
        INSERT INTO guild_skills (guild_id, skill_id)
        VALUES (?, ?)
        """
        query3 = """
        INSERT INTO guild_feats (guild_id, feat_id)
        VALUES (?, ?)
        """

        for _, v in self.db.guilds.items():
            zone_id = self.db.zone_id_for(v.id)
            self.sql.execute(
                query,
                (
                    v.id,
                    zone_id,
                    v.charge,
                    v.no_such_skill,
                    v.not_enough_gold,
                    v.minlvl,
                    nullable_fk(v.keeper),
                    *flag_parts(v.with_who),
                    v.open,
                    v.close,
                ),
            )

            for teach in v.skills:
                self.sql.execute(query2, (v.id, teach))
            for teach in v.feats:
                self.sql.execute(query3, (v.id, teach))

    def _migrate_help(self):
        query = """
        INSERT INTO help_entries (keywords, entry, min_level)
        VALUES (?, ?, ?)
        """
        for v in self.db.help:
            # We don't want these malformed helpfiles.
            if "@" in v.name:
                continue
            self.sql.execute(query, (v.name, v.entry, v.min_level))

    def _migrate_assemblies(self):
        query = """
        INSERT INTO assemblies (object_prototype_id, assembly_type)
        VALUES (?, ?)
        """
        query2 = """
        INSERT INTO assembly_ingredients (assembly_id, object_prototype_id, consumed, in_room)
        VALUES (?, ?, ?, ?)
        """

        for v in self.db.assemblies:
            assembly_type = 2
            if isinstance(v.assembly_type, int):
                assembly_type = v.assembly_type
            elif str(v.assembly_type).isdigit():
                assembly_type = int(v.assembly_type)

            cur = self.sql.execute(query, (v.object_prototype_id, assembly_type))
            assembly_id = cur.lastrowid

            for ingredient in v.components:
                self.sql.execute(
                    query2,
                    (
                        assembly_id,
                        ingredient.object_prototype_id,
                        int(ingredient.consumed),
                        int(ingredient.in_room),
                    ),
                )

    def _migrate_houses(self):
        pass

    def _migrate_users(self):
        query = """
        INSERT INTO users (
            username, email, password_hash, admin_level,
            rpp_points, rpp_bank, extra_slots, rpp_awarded
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """
        for _, v in self.db.accounts.items():
            cur = self.sql.execute(
                query,
                (
                    v.name,
                    v.email,
                    v.password,
                    v.admin_level,
                    v.rpp,
                    v.rpp_bank,
                    v.slots - 3,
                    v.rpp + v.rpp_bank,
                ),
            )
            self.user_ids[v.name.lower()] = cur.lastrowid

    def _migrate_players(self):
        columns = (
            "id", "user_id", "in_game",
            "name", "short_descr", "long_descr", "description",
            "size", "sex", "race", "chclass",
            "act0", "act1", "act2", "act3",
            "affected_by0", "affected_by1", "affected_by2", "affected_by3",
            "alignment",
            "str", "intel", "wis", "dex", "con", "cha",
            "position", "level", "race_level", "level_adj",
            "armor", "admlevel",
            "admflags0", "admflags1", "admflags2", "admflags3",
            "basepl", "baseki", "basest", "gold",
            "hairl", "hairc", "hairs", "skin", "eye", "distfea", "aura",
            "voice", "feature", "title", "rdisplay",
            "hometown", "height", "weight",
            "bank_gold", "lastint", "exp", "upgrade", "practices",
            "radar1", "radar2", "radar3",
            "dcount", "deathtime",
            "kaioken", "absorbs", "boosts", "blesslvl", "ingestLearned",
            "majinize", "suppression", "mimic",
            "transclass", "preference",
            "moltexp", "moltlevel",
            "tail_growth", "fury",
            "forgetting", "forgetcount", "skill_slots",
            "time_birth", "time_created", "time_logon", "time_maxage", "time_played",
            "con_sdcooldown",
            "olc_zone", "lifeperf",
            "poofin", "poofout",
            "wimp_level", "load_room",
            "pref0", "pref1", "pref2", "pref3",
            "page_length",
            "trainstr", "trainint", "traincon", "trainwis", "trainagl", "trainspd",
            "racial_pref",
        )
        placeholders = ", ".join("?" for _ in columns)
        query = f"INSERT INTO characters ({', '.join(columns)}) VALUES ({placeholders})"
        query_bonuses = """
        INSERT INTO character_bonuses (character_id, bonus_type, bonus_value)
        VALUES (?, ?, ?)
        """
        query_conditions = """
        INSERT INTO characters_conditions (character_id, condition_type, condition_value)
        VALUES (?, ?, ?)
        """
        query_affects = """
        INSERT INTO characters_affects (
            character_id, category_id, duration, modifier, location, specific, bitvector
        ) VALUES (?, ?, ?, ?, ?, ?, ?)
        """
        query_limb = """
        INSERT INTO characters_limb_health (character_id, limb_id, health)
        VALUES (?, ?, ?)
        """
        query_dgvars = """
        INSERT INTO characters_dgscript_variables (character_id, variable_name, variable_value)
        VALUES (?, ?, ?)
        """
        query_skills = """
        INSERT INTO characters_skills (
            character_id, skill_id, skill_level, skill_modifier, skill_perfections
        ) VALUES (?, ?, ?, ?, ?)
        """
        query_boards = """
        INSERT INTO characters_boards (character_id, board_id, last_read_date)
        VALUES (?, ?, ?)
        """
        query_transcosts = """
        INSERT INTO characters_transcosts (character_id, transtier, paid)
        VALUES (?, ?, ?)
        """

        for _, c in self.db.characters.items():
            user_id = self.user_ids.get(c.username.lower())

            self.sql.execute(
                query,
                (
                    c.id,
                    user_id,
                    0,
                    c.name,
                    c.short_descr,
                    c.long_descr,
                    c.description,
                    c.size,
                    c.sex,
                    c.race,
                    c.chclass,
                    *flag_parts(c.act),
                    *flag_parts(c.affected_by),
                    c.alignment,
                    c.strength,
                    c.intel,
                    c.wis,
                    c.dex,
                    c.con,
                    c.cha,
                    c.position,
                    c.level,
                    c.race_level,
                    c.level_adj,
                    c.armor,
                    c.admin_level,
                    *flag_parts(c.admin_flags),
                    c.basepl,
                    c.baseki,
                    c.basest,
                    c.gold,
                    c.hairl,
                    c.hairc,
                    c.hairs,
                    c.skin,
                    c.eye,
                    c.distfea,
                    c.aura,
                    c.voice,
                    c.feature,
                    c.title,
                    c.rdisplay,
                    nullable_fk(c.hometown),
                    c.height,
                    c.weight,
                    c.bank_gold,
                    c.lastint,
                    c.exp,
                    c.upgrade,
                    c.practices,
                    nullable_fk(c.radar1),
                    nullable_fk(c.radar2),
                    nullable_fk(c.radar3),
                    c.dcount,
                    c.deathtime,
                    c.kaioken,
                    c.absorbs,
                    c.boosts,
                    c.blesslvl,
                    c.ingestLearned,
                    c.majinize,
                    c.suppression,
                    c.mimic,
                    c.transclass,
                    c.preference,
                    c.molt_experience,
                    c.molt_level,
                    c.tail_growth,
                    c.fury,
                    c.forgeting,
                    c.forgetcount,
                    c.skill_slots,
                    c.time_birth,
                    c.time_created,
                    c.time_logon,
                    c.time_maxage,
                    c.time_played,
                    c.con_sdcooldown,
                    nullable_fk(c.olc_zone),
                    c.lifeperc,
                    c.poofin,
                    c.poofout,
                    c.wimp_level,
                    nullable_fk(c.load_room),
                    *flag_parts(c.pref_flags),
                    c.page_length,
                    c.trainstr,
                    c.trainint,
                    c.traincon,
                    c.trainwis,
                    c.trainagl,
                    c.trainspd,
                    c.racial_pref,
                ),
            )

            for bonus_type, bonus_value in c.bonuses.items():
                self.sql.execute(query_bonuses, (c.id, bonus_type, bonus_value))

            for condition_type, condition_value in c.conditions.items():
                self.sql.execute(query_conditions, (c.id, condition_type, condition_value))

            for category_id, affects in ((0, c.affected or []), (1, c.affectedv or [])):
                for aff in affects:
                    self.sql.execute(
                        query_affects,
                        (
                            c.id,
                            category_id,
                            as_int(getattr(aff, "duration", 0)),
                            as_int(getattr(aff, "modifier", 0)),
                            as_int(getattr(aff, "location", 0)),
                            as_int(getattr(aff, "specific", 0)),
                            as_int(getattr(aff, "bitvector", getattr(aff, "aff_flags", 0))),
                        ),
                    )

            for limb_id, health in c.limb_condition.items():
                self.sql.execute(query_limb, (c.id, as_int(limb_id), health))

            for variable_name, variable_value in c.dgscript_variables.items():
                self.sql.execute(query_dgvars, (c.id, variable_name, variable_value))

            for skill_id, skill_data in c.skills.items():
                self.sql.execute(
                    query_skills,
                    (
                        c.id,
                        skill_id,
                        skill_data.level,
                        skill_data.bonus,
                        skill_data.perfs,
                    ),
                )

            for board_id, last_read_date in c.lboard.items():
                self.sql.execute(query_boards, (c.id, board_id, last_read_date))

            for transtier, paid in c.transcost.items():
                self.sql.execute(query_transcosts, (c.id, transtier, paid))

    def migrate(self):
        with self.sql:
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
