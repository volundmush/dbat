import asyncio
import asyncpg
import uuid
from pathlib import Path
from .loader import LegacyDatabase, ResetCommand as LegacyReset
from rich.text import Text

import dbat
from dbat.types.grids import Grid, Tile, Shape, Exit, POINT, Direction, ResetCommand
from dbat.types.structures import Structure
from dbat.types.characters import MobilePrototype, PlayerCharacter
from dbat.types.misc import ExtraDescription
from dbat.types.objects import ObjectPrototype, WearableComponent, WeaponComponent, ConsumableComponent, ToolComponent, BreakableComponent, ContainerComponent, FuelConsumerComponent, FuelProviderComponent, ValueComponent
from dbat.types.location import Location
from dbat.types.shops import Shop
from dbat.types.guilds import Guild
from dbat.types.zones import Zone

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

def convert_location(legacy_location) -> Location:
    entity = None
    x = 0
    y = 0
    z = 0
    match legacy_location.type:
        case "room":
            entity = dbat.SLUGS["tile"][f"{legacy_location.target}"].grid
            x = legacy_location.target
        case "area":
            entity = dbat.ZONES[f"area_{legacy_location.target}"]
            x = legacy_location.coordinates.x
            y = legacy_location.coordinates.y
            z = legacy_location.coordinates.z
    l = Location(entity, point=(x, y, z))
    return l

class Migrator:
    
    def __init__(self, legacy_db: LegacyDatabase):
        self.db = legacy_db
    
    def migrate_zones(self):
        for k, v in self.db.zones.items():
            z = Zone()
            z.id = str(k)
            z.set_color_name(convert_color_string(v.color_name))
            z.flags.update([str(x) for x in v.zone_flags])
            dbat.ZONES[z.id] = z
        
        # Now that all are created, setup parent-child relationships.
        for k, v in self.db.zones.items():
            z = dbat.ZONES[str(k)]
            if v.parent != -1:
                p = dbat.ZONES[str(v.parent)]
                z.parent = p
                p.children.add(z)
        
        # We'll turn all of the Areas into Zones.
        for k, v in self.db.areas.items():
            area_id = f"area_{k}"
            z = Zone()
            z.id = area_id
            z.set_color_name(convert_color_string(v.name))
            z.set_color_description(convert_color_string(v.look_description))

            p = dbat.ZONES.get(str(v.zone), None)
            if p:
                z.parent = p
                p.children.add(z)

            dbat.ZONES[z.id] = z

            for shape_name, shape in v.shapes.items():
                s = Shape(z, (shape.coordinates.x, shape.coordinates.y, shape.coordinates.z))
                s.type = shape.type
                s.set_color_name(convert_color_string(shape.name))
                s.sector_type = shape.sector_type
                s.tile_display = shape.tile_display
                s.set_color_description(convert_color_string(shape.look_description))
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
            z = dbat.ZONES.get(f"{v.zone}")
            p = (k, 0, 0)
            t = Tile(z, p)
            t.slug = f"{k}"
            t.set_color_name(convert_color_string(v.name))
            t.set_color_description(convert_color_string(v.look_description))
            t.sector_type = v.sector_type
            t.extra_descriptions = [ExtraDescription(ed.keywords, ed.description) for ed in v.extra_descriptions]
            t.proto_script = v.proto_script.copy()
            t.reset_commands = convert_reset_commands(v.reset_commands)
            t.flags.update([str(x.name) for x in v.room_flags])
            z.tiles[p] = t
            dbat.SLUGS[t.slug_type][t.slug] = t

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

                e = Exit(l)
                e.flags = oe.exit_flags
                e.keywords = oe.keywords
                e.description = oe.description
                e.dclock = oe.dclock
                e.dchide = oe.dchide
                to_tile.exits[d] = e

        # now that we've got all of the rooms imported and accessible by the slug index, we can handle exits.
        for k, v in self.db.areas.items():
            z = dbat.ZONES.get(f"area_{k}")

            for coor, t in v.tiles.items():
                tile = Tile(z, (coor.x, coor.y, coor.z))
                tile.set_color_name(convert_color_string(t.name))
                tile.set_color_description(convert_color_string(t.look_description))
                tile.sector_type = t.sector_type
                tile.extra_descriptions = [ExtraDescription(ed.keywords, ed.description) for ed in t.extra_descriptions]
                tile.proto_script = t.proto_script.copy()
                tile.reset_commands = convert_reset_commands(t.reset_commands)
                tile.flags.update([str(x.name) for x in t.room_flags])
                tile.tile_display = t.tile_display
                z.tiles[tile.point] = tile

            for coor, t in v.tiles.items():
                tile = z.tiles.get((coor.x, coor.y, coor.z))
                if not tile:
                    print(f"Invalid tile slug {k} for zone {z.id}")
                    continue
                convert_exit(tile, t.exits)

        # now we'll handle exits for legacy rooms.
        for k, v in self.db.rooms.items():
            z = dbat.ZONES.get(f"{v.zone}")
            tile = dbat.SLUGS["tile"].get(f"{k}")
            if not tile:
                print(f"Invalid tile slug {k} for zone {z.id}")
                continue

            convert_exit(tile, v.exits)

        # TODO: LAST: handle docking/landing/launch points.

        for k, v in dbat.ZONES.items():
            dbat.DIRTY_ZONES.add(k)

    def migrate_objects(self):
        for k, v in self.db.oproto.items():
            o = ObjectPrototype()
            o.id = str(k)
            o.keywords = [x.lower() for x in v.name.split()]
            o.set_color_name(convert_color_string(v.short_description))
            o.set_color_description(convert_color_string(v.look_description))
            o.proto_script = [str(x) for x in v.proto_script]
            
            # TODO: proper object conversion.
            
            dbat.OBJECT_PROTOTYPES[o.id] = o
            dbat.DIRTY_OBJECT_PROTOTYPES.add(o.id)
    
    def migrate_mobiles(self):
        for k, v in self.db.nproto.items():
            m = MobilePrototype()
            m.id = str(k)
            m.keywords = [x.lower() for x in v.name.split()]
            m.set_color_name(convert_color_string(v.short_description))
            m.set_color_description(convert_color_string(v.look_description))
            m.proto_script = [str(x) for x in v.proto_script]

            # TODO: proper mobile conversion.

            dbat.MOBILE_PROTOTYPES[m.id] = m
            dbat.DIRTY_MOBILE_PROTOTYPES.add(m.id)

    def migrate_assets(self):
        self.migrate_zones()
        self.migrate_objects()
        self.migrate_mobiles()

        dbat.dump_assets()

    async def migrate(self, conn: asyncpg.Connection):
        pass
        

def get_db():
    db = LegacyDatabase()
    db.load_from_files(Path("data"))
    return db

async def test():
    db = get_db()
    migrator = Migrator(db)
    await migrator.migrate(None)

async def migrate(db: LegacyDatabase):
    migrator = Migrator(db)

    conn = await asyncpg.connect("postgresql://localhost/dbat")

    async with asyncpg.connect("postgresql://localhost/dbat") as conn:
        async with conn.transaction() as tx:
            await migrator.migrate(tx)

if __name__ == "__main__":
    db = get_db()
    asyncio.run(migrate(db))