import asyncio
import asyncpg
import uuid
import re
import shutil
from pathlib import Path
from .loader import LegacyDatabase, ResetCommand as LegacyReset
from rich.text import Text

from pydantic import EmailStr

import dbat
from dbat.types.grids import Grid, Tile, Shape, Exit, Direction, ResetCommand
from dbat.types.structures import Structure
from dbat.types.characters import MobilePrototype, PlayerCharacter
from dbat.types.misc import ExtraDescription
from dbat.types.objects import ObjectPrototype, WearableComponent, WeaponComponent, ConsumableComponent, ToolComponent, BreakableComponent, ContainerComponent, FuelConsumerComponent, FuelProviderComponent, PriceComponent
from dbat.types.location import Location, Point
from dbat.types.shops import Shop
from dbat.types.guilds import Guild
from dbat.types.zones import Zone
from dbat.types.dgscripts import DgScript, TriggerType, parse_script, compile_dgscript, render_code

from dbat.legacy.ansi import convert_color_string

def convert_reset_commands(legacy: list[LegacyReset]) -> list[ResetCommand]:
    """
    The ONLY difference between the legacy style and new is that
    the legacy has a depends_last boolean, to create groupings.

    For ours, we will just group these as subcommands.
    The legacy style is a flat list, and the if_flag was interpreted LITERALLY, which means that
    to map this correctly each subcommands list will have at most one entry and it'll look like a long snake.
    """
    out = list()
    if not legacy:
        return out
    
    current = ResetCommand()
    out.append(current)

    for x in legacy:
        if x.depends_last:
            current.subcommands.append(ResetCommand(
                command=x.command,
                target=x.target,
                max_world=x.max,
                max_location=x.max_location,
                chance=x.chance,
                ex=x.ex,
                key=x.key,
                value=x.value
            ))
        else:
            current = ResetCommand(
                command=x.command,
                target=x.target,
                max_world=x.max,
                max_location=x.max_location,
                chance=x.chance,
                ex=x.ex,
                key=x.key,
                value=x.value
            )
            out.append(current)
    return out



class Migrator:
    
    def __init__(self, legacy_db: LegacyDatabase):
        self.db = legacy_db
        self.zone_map: dict[int, Zone] = dict()
        self.area_map: dict[int, Zone] = dict()
        self.room_map: dict[int, Tile] = dict()
        self.account_map: dict[str, uuid.UUID] = dict()
        self.account_email_map: dict[str, uuid.UUID] = dict()
        self.account_merge_map: dict[str, str] = dict()
    
    def migrate_zones(self):
        for k, v in self.db.zones.items():
            id = uuid.uuid4()
            z = Zone(id=id)
            z.slug = str(k)
            z.color_name = convert_color_string(v.color_name)
            z.flags.update([str(x) for x in v.zone_flags])
            dbat.INDEX.zones[z.id] = z
            self.zone_map[k] = z
        
        # Now that all are created, setup parent-child relationships.
        for k, v in self.db.zones.items():
            z = self.zone_map[k]
            if v.parent != -1:
                p = self.zone_map[v.parent]
                p.add_child(z)
        
        # We'll turn all of the Areas into Zones.
        for k, v in self.db.areas.items():
            id = uuid.uuid4()
            z = Zone(id=id)
            z.slug = str(f"area_{k}")
            z.color_name = convert_color_string(v.name)
            z.color_description = convert_color_string(v.look_description)

            p = self.zone_map.get(v.zone, None)
            if p:
                z.parent = p
                p.children.add(z)

            dbat.INDEX.zones[z.id] = z
            self.area_map[k] = z

            for shape_name, shape in v.shapes.items():
                s = Shape(grid_type="zone", grid_id=z.id, point=Point(x=shape.coordinates.x, y=shape.coordinates.y, z=shape.coordinates.z))
                s.type = shape.type
                s.color_name = convert_color_string(shape.name)
                s.sector_type = shape.sector_type
                s.tile_display = shape.tile_display
                s.color_description = convert_color_string(shape.look_description)
                s.north = shape.dimensions.north
                s.south = shape.dimensions.south
                s.east = shape.dimensions.east
                s.west = shape.dimensions.west
                s.up = shape.dimensions.up
                s.down = shape.dimensions.down
                s.priority = shape.priority
                if s.point in z.shapes:
                    raise ValueError(f"Duplicate shape point {s.point} in zone {z.id}")
                z.shapes[s.point] = s

            # Tiles come after we import rooms.

        for k, v in self.db.rooms.items():
            z = self.zone_map.get(v.zone)
            p = Point(x=k, y=0, z=0)
            t = Tile(grid_type="zone", grid_id=z.id, point=p)
            t.slug = f"{k}"
            self.room_map[k] = t
            
            t.color_name = convert_color_string(v.name)
            t.color_description = convert_color_string(v.look_description)
            t.sector_type = v.sector_type
            t.extra_descriptions = [ExtraDescription(keywords=ed.keywords, description=convert_color_string(ed.description)) for ed in v.extra_descriptions]
            t.proto_script = [str(x) for x in v.proto_script]
            t.reset_commands = convert_reset_commands(v.reset_commands)
            t.flags.update([str(x.name) for x in v.room_flags])
            z.tiles[p] = t
            dbat.INDEX.slugs[t.report_slug_type()][t.slug] = t
        
        def convert_location(legacy_location) -> Location:
            entity_type = None
            entity_id = None
            x = 0
            y = 0
            z = 0
            match legacy_location.type:
                case "room":
                    tile = self.room_map.get(legacy_location.target, None)
                    if not tile:
                        print(f"Invalid location target {legacy_location.target} for type room")
                        return None
                    entity_type = "zone"
                    entity_id = tile.grid().id
                    x = legacy_location.target
                case "area":
                    area = self.area_map.get(legacy_location.target, None)
                    if not area:
                        print(f"Invalid location target {legacy_location.target} for type area")
                        return None
                    entity_type = "zone"
                    entity_id = area.id
                    x = legacy_location.coordinates.x
                    y = legacy_location.coordinates.y
                    z = legacy_location.coordinates.z
            l = Location(location_type=entity_type, location_id=entity_id, point=Point(x=x, y=y, z=z))
            return l

        def convert_exit(to_tile: Tile, legacy_exits: dict):
            for od, oe in legacy_exits.items():
                # convert old direction to new Direction enum.
                d = getattr(Direction, od.name)

                if oe.destination.target == -1:
                    continue
                
                l = convert_location(oe.destination)
                if not l:
                    print(f"Invalid exit destination {oe.destination} for tile {to_tile.slug} in zone {z.id}")
                    continue

                e = Exit(location=l)
                e.flags = oe.exit_flags
                e.keywords = oe.keywords if oe.keywords else ""
                e.description = oe.description if oe.description else ""
                e.dclock = oe.dclock
                e.dchide = oe.dchide
                to_tile.exits[d] = e

        # now that we've got all of the rooms imported and accessible by the slug index, we can handle exits.
        for k, v in self.db.areas.items():
            z = self.area_map.get(k)

            for coor, t in v.tiles.items():
                tile = Tile(grid_type="zone", grid_id=z.id, point=Point(x=coor.x, y=coor.y, z=coor.z))
                tile.color_name = convert_color_string(t.name)
                tile.color_description = convert_color_string(t.look_description)
                tile.sector_type = str(t.sector_type) if t.sector_type else ""
                tile.extra_descriptions = [ExtraDescription(ed.keywords, convert_color_string(ed.description)) for ed in t.extra_descriptions]
                tile.proto_script = [str(x) for x in t.proto_script]
                tile.reset_commands = convert_reset_commands(t.reset_commands)
                tile.flags.update([str(x.name) for x in t.room_flags])
                tile.tile_display = t.tile_display
                z.tiles[tile.point] = tile

            for coor, t in v.tiles.items():
                tile = z.tiles.get(Point(x=coor.x, y=coor.y, z=coor.z))
                if not tile:
                    print(f"Invalid tile slug {k} for zone {z.id}")
                    continue
                convert_exit(tile, t.exits)

        # now we'll handle exits for legacy rooms.
        for k, v in self.db.rooms.items():
            z = self.zone_map.get(v.zone)
            tile = self.room_map.get(k, None)
            if not tile:
                print(f"Invalid tile slug {k} for zone {z.id}")
                continue

            convert_exit(tile, v.exits)

        # TODO: LAST: handle docking/landing/launch points.

        dbat.INDEX.dirty_zones = set(dbat.INDEX.zones.keys())

    def migrate_objects(self):
        for k, v in self.db.oproto.items():
            o = ObjectPrototype(id=str(k))
            o.keywords = {x.lower() for x in v.name.split()}
            for x in ("a", "the", "an"):
                o.keywords.discard(x)
            o.color_name = convert_color_string(v.short_description)
            o.color_description = convert_color_string(v.look_description)
            o.proto_script = [str(x) for x in v.proto_script]
            
            # TODO: proper object conversion.
            
            dbat.INDEX.object_prototypes[o.id] = o
            dbat.INDEX.dirty_object_prototypes.add(o.id)
    
    def migrate_mobiles(self):
        for k, v in self.db.nproto.items():
            m = MobilePrototype(id=str(k))
            m.keywords = {x.lower() for x in v.name.split()}
            for x in ("a", "the", "an"):
                m.keywords.discard(x)
            m.color_name = convert_color_string(v.short_description)
            m.color_description = convert_color_string(v.look_description)
            m.proto_script = [str(x) for x in v.proto_script]

            # TODO: proper mobile conversion.

            dbat.INDEX.mobile_prototypes[m.id] = m
            dbat.INDEX.dirty_mobile_prototypes.add(m.id)
    
    def migrate_dgscripts(self):
        dgpatches_dir = Path() / "dgpatches"
        dgpatches: dict[int, str] = dict()
        if dgpatches_dir.exists():
            for f in dgpatches_dir.glob("*.txt"):
                dgpatches[int(f.stem)] = f.read_text()

        mobtrig_map = {
            0: TriggerType.GLOBAL,
            1: TriggerType.RANDOM,
            2: TriggerType.COMMAND,
            3: TriggerType.SPEECH,
            4: TriggerType.ACT,
            5: TriggerType.DEATH,
            6: TriggerType.GREET,
            7: TriggerType.GREET_ALL,
            8: TriggerType.ENTRY,
            9: TriggerType.RECEIVE,
            10: TriggerType.FIGHT,
            11: TriggerType.HEALTH_PERCENT,
            12: TriggerType.BRIBE,
            13: TriggerType.LOAD,
            14: TriggerType.MEMORY,
            16: TriggerType.LEAVE,
            17: TriggerType.DOOR,

            19: TriggerType.TIME,
            20: TriggerType.HOURLY,
            21: TriggerType.QUARTERLY
        }

        objtrig_map = {
            0: TriggerType.GLOBAL,
            1: TriggerType.RANDOM,
            2: TriggerType.COMMAND,

            5: TriggerType.TIMER,
            6: TriggerType.GET,
            7: TriggerType.DROP,
            8: TriggerType.GIVE,
            9: TriggerType.WEAR,
            11: TriggerType.REMOVE,
            13: TriggerType.LOAD,
            15: TriggerType.CAST,
            16: TriggerType.LEAVE,
            18: TriggerType.CONSUME,
            19: TriggerType.TIME,
            20: TriggerType.HOURLY,
            21: TriggerType.QUARTERLY
        }

        wldtrig_map = {
            0: TriggerType.GLOBAL,
            1: TriggerType.RANDOM,
            2: TriggerType.COMMAND,
            3: TriggerType.SPEECH,
            5: TriggerType.RESET,
            6: TriggerType.ENTRY,
            7: TriggerType.DROP,
            15: TriggerType.CAST,
            16: TriggerType.LEAVE,
            17: TriggerType.DOOR,
            19: TriggerType.TIME,
            20: TriggerType.HOURLY,
            21: TriggerType.QUARTERLY
        }


        for k, v in self.db.dgproto.items():
            data = {
                "id": str(k),
                "name": v.name,
                "attach_type": v.attach_type,
                "narg": v.narg,
                "command": v.command
            }
            trigmap = None
            match v.attach_type:
                case 0:
                    data["attach_type"] = "mobile"
                    trigmap = mobtrig_map
                case 1:
                    data["attach_type"] = "object"
                    trigmap = objtrig_map
                case 2:
                    data["attach_type"] = "tile"
                    trigmap = wldtrig_map
            data["triggers"] = {flag for bit, flag in trigmap.items() if v.trigger_type & (1 << bit)}
            if not data["triggers"]:
                print(f"WARNING: DG script {data['id']} has no trigger type flags set. Original is: {v.trigger_type}")


            # dgpatches allow us to override the script body with a fixed version.
            code_body = dgpatches.get(k, v.body)

            data["code"] = code_body

            try:
                script = DgScript(**data)
            except Exception as e:
                print(f"Error compiling DG script {data['id']}: {e}")
                dgpatches_dir.mkdir(exist_ok=True)
                patch_file = dgpatches_dir / f"{k}.txt"
                with patch_file.open("w") as f:
                    f.write(code_body)
                print(f"Wrote out patch file to {patch_file} for manual fixing.")
                continue

            dbat.INDEX.dgscript_prototypes[script.id] = script
            dbat.INDEX.dirty_dgscript_prototypes.add(script.id)

    def migrate_shops(self):
        pass

    def migrate_guilds(self):
        pass

    def migrate_assets(self):
        self.migrate_zones()
        self.migrate_objects()
        self.migrate_mobiles()
        self.migrate_dgscripts()
        self.migrate_shops()
        self.migrate_guilds()
        p = Path() / "data"
        
        for x in ("assets", "userdata"):
            f = p / x
            if f.exists():
                shutil.rmtree(f)
        
        dbat.INDEX.dump_assets()

def get_db():
    db = LegacyDatabase()
    db.load_from_files(Path("data"))
    return db

def test():
    db = get_db()
    migrator = Migrator(db)
    migrator.migrate_assets()
    return migrator

async def migrate():
    db = get_db()
    migrator = Migrator(db)
    
    from muforge.utils.boot import get_environment
    app = await get_environment("game")

    core = app.plugins["core"]

    migrator.migrate_assets()
    
    return migrator

if __name__ == "__main__":
    asyncio.run(migrate())