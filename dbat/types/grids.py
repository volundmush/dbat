import typing
import uuid
from dataclasses import dataclass, field, asdict

from loguru import logger

from dbat.types.characters import Mobile, MobilePrototype
from dbat.types.objects import Object, ObjectPrototype
from .dgscripts import TriggerType, HasDgScripts
from .location import POINT, IsLocation, Location
from .misc import HasFlags, HasExtraDescriptions, ExtraDescription, HasColorName, HasColorDescription
from enum import Enum
from rich.text import Text
import random
import dbat

from .dgscripts import HasDgScripts

POINT = typing.Tuple[int, int, int]  # (x, y, z) coordinates in a grid

class Direction(Enum):
    north = "north"
    east = "east"
    south = "south"
    west = "west"
    up = "up"
    down = "down"
    northwest = "northwest"
    northeast = "northeast"
    southeast = "southeast"
    southwest = "southwest"
    inside = "inside"
    outside = "outside"

    def opposite(self) -> "Direction":
        match self:
            case Direction.north:
                return Direction.south
            case Direction.east:
                return Direction.west
            case Direction.south:
                return Direction.north
            case Direction.west:
                return Direction.east
            case Direction.up:
                return Direction.down
            case Direction.down:
                return Direction.up
            case Direction.northwest:
                return Direction.southeast
            case Direction.northeast:
                return Direction.southwest
            case Direction.southeast:
                return Direction.northwest
            case Direction.southwest:
                return Direction.northeast
            case Direction.inside:
                return Direction.outside
            case Direction.outside:
                return Direction.inside

    def update_coordinates(self, coor: POINT) -> POINT:
        match self:
            case Direction.north:
                return (coor[0], coor[1] + 1, coor[2])
            case Direction.east:
                return (coor[0] + 1, coor[1], coor[2])
            case Direction.south:
                return (coor[0], coor[1] - 1, coor[2])
            case Direction.west:
                return (coor[0] - 1, coor[1], coor[2])
            case Direction.up:
                return (coor[0], coor[1], coor[2] + 1)
            case Direction.down:
                return (coor[0], coor[1], coor[2] - 1)
            case Direction.northeast:
                return (coor[0] + 1, coor[1] + 1, coor[2])
            case Direction.southeast:
                return (coor[0] + 1, coor[1] - 1, coor[2])
            case Direction.southwest:
                return (coor[0] - 1, coor[1] - 1, coor[2])
            case Direction.northwest:
                return (coor[0] - 1, coor[1] + 1, coor[2])
            case _:
                return coor

@dataclass(slots=True)
class ResetCommand:
    command: str = ""
    target: str = ""
    max_world: int = 0
    max_location: int = 0
    chance: int = 100
    ex: int = 0
    key: str = ""
    value: str = ""
    subcommands: list[ResetCommand] = field(default_factory=list)

    @classmethod
    def load(cls, data: dict) -> ResetCommand:
        subs = data.pop("subcommands", list())
        cmd = cls(**data)
        if subs:
            cmd.subcommands = [ResetCommand.load(sub) for sub in subs]
        return cmd


    def execute(self, tile: "Tile", last_spawned=None):
        successful = False
        spawned: Object | Mobile = None

        if self.chance < 100:
            roll = random.randint(1, 100)
            if roll > self.chance:
                return  # Command fails due to chance
        
        loc = tile.as_location()

        def perform_counts(proto):
            if self.max_world > 0 and len(proto.instances) >= self.max_world:
                return False  # Max world limit reached, fail silently
            if self.max_location > 0:
                # Count how many instances of this proto are in the current location
                count = sum(1 for instance in proto.instances if instance.spawn_location == loc)
                if count >= self.max_location:
                    return False  # Max location limit reached, fail silently
            return True

        match self.command:
            case "MOB":
                if not (proto := dbat.MOBILE_PROTOTYPES.get(self.target, None)):
                    logger.warning(f"ResetCommand for {tile}: MOB target {self.target} does not exist.")
                    return  # Invalid prototype, fail silently
                if not perform_counts(proto):
                    return  # Count limits reached, fail silently
                spawned = proto.spawn()
                spawned.add_to_location(loc)
                spawned.trigger_dgscripts(TriggerType.LOAD)
                successful = True
            case "GIVE":
                if not last_spawned:
                    logger.warning(f"ResetCommand for {tile}: GIVE command has no last_spawned entity to give to.")
                    return  # No entity to give to, fail silently
                if not isinstance(last_spawned, Mobile):
                    logger.warning(f"ResetCommand for {tile}: GIVE command's last_spawned entity is not a Mobile.")
                    return  # Last spawned entity is not a mobile, fail silently
                if not (proto := dbat.OBJECT_PROTOTYPES.get(self.target, None)):
                    logger.warning(f"ResetCommand for {tile}: GIVE target {self.target} does not exist.")
                    return  # Invalid prototype, fail silently
                spawned = proto.spawn()
                spawned.add_to_location(loc)
                spawned.trigger_dgscripts(TriggerType.LOAD)
                if last_spawned.can_store(spawned):
                    spawned.remove_from_location()
                    last_spawned.add_to_inventory(spawned)
                successful = True
            case "OBJ":
                if not (proto := dbat.OBJECT_PROTOTYPES.get(self.target, None)):
                    logger.warning(f"ResetCommand for {tile}: OBJ target {self.target} does not exist.")
                    return  # Invalid prototype, fail silently
                if not perform_counts(proto):
                    return  # Count limits reached, fail silently
                spawned = proto.spawn()
                spawned.add_to_location(loc)
                spawned.trigger_dgscripts(TriggerType.LOAD)
                successful = True
            case "EQUIP":
                if not last_spawned:
                    logger.warning(f"ResetCommand for {tile}: EQUIP command has no last_spawned entity to equip.")
                    return  # No entity to equip, fail silently
                if not isinstance(last_spawned, Mobile):
                    logger.warning(f"ResetCommand for {tile}: EQUIP command's last_spawned entity is not a Mobile.")
                    return  # Last spawned entity is not a mobile, fail silently
                if not (proto := dbat.OBJECT_PROTOTYPES.get(self.target, None)):
                    logger.warning(f"ResetCommand for {tile}: EQUIP target {self.target} does not exist.")
                    return  # Invalid prototype, fail silently
                if not perform_counts(proto):
                    return  # Count limits reached, fail silently
                if last_spawned.get_equipment(self.key):
                    logger.warning(f"ResetCommand for {tile}: EQUIP target {self.target} slot {self.key} is already equipped.")
                    return  # Slot already equipped, fail silently
                spawned = proto.spawn()
                spawned.add_to_location(loc)
                spawned.trigger_dgscripts(TriggerType.LOAD)
                spawned.remove_from_location()
                last_spawned.add_equipment(self.key, spawned)
                successful = True
            case "PUT":
                # put object inside last_spawned (container) if possible, otherwise put in location
                if not last_spawned:
                    logger.warning(f"ResetCommand for {tile}: PUT command has no last_spawned entity to put into.")
                    return  # No entity to put into, fail silently
                if not isinstance(last_spawned, Object):
                    logger.warning(f"ResetCommand for {tile}: PUT command's last_spawned entity is not an Object.")
                    return  # Last spawned entity is not an object, fail silently
                if not (proto := dbat.OBJECT_PROTOTYPES.get(self.target, None)):
                    logger.warning(f"ResetCommand for {tile}: PUT target {self.target} does not exist.")
                    return  # Invalid prototype, fail silently
                if not perform_counts(proto):
                    return  # Count limits reached, fail silently
                spawned = proto.spawn()
                spawned.add_to_location(loc)
                spawned.trigger_dgscripts(TriggerType.LOAD)
                if last_spawned.can_store(spawned):
                    spawned.remove_from_location()
                    last_spawned.add_to_inventory(spawned)
                successful = True
            case "REMOVE":
                pass
            case "DOOR":
                pass
            case "TRIGGER":
                # add a DgScript to the last_spawned entity regardless of what it is.
                if not last_spawned:
                    logger.warning(f"ResetCommand for {tile}: TRIGGER command has no last_spawned entity to add trigger to.")
                    return  # No entity to add trigger to, fail silently
                if not (dgscript := dbat.DGSCRIPT_PROTOTYPES.get(self.target, None)):
                    logger.warning(f"ResetCommand for {tile}: TRIGGER target {self.target} does not exist.")
                    return  # Invalid prototype, fail silently
                last_spawned.add_dgscript(dgscript)
                pass
            case "VARIABLE":
                pass
            case _:
                pass

        if successful:
            for subcmd in self.subcommands:
                subcmd.execute(tile, last_spawned=spawned)

class SectorType(Enum):
    inside = "inside"       # Indoors
    city = "city"         # In a city
    field = "field"        # In a field
    forest = "forest"       # In a forest
    hills = "hills"        # In the hills
    mountain = "mountain"     # On a mountain
    water_swim = "water_swim"   # Swimmable water
    water_noswim = "water_noswim" # Water - need a boat
    flying = "flying"       # Wheee!
    underwater = "underwater"   # Underwater
    shop = "shop"        # Shop
    important = "important"   # Important Rooms
    desert = "desert"      # A desert
    space = "space"       # This is a space room
    lava = "lava"         # This room always has lava

class Exit(HasFlags):

    def __init__(self, location: Location):
        HasFlags.__init__(self)
        self.location: Location  = location
        self.keywords: str = ""
        self.description: str = ""
        self.dchide: int = 0
        self.dclock: int = 0
        self.keys: set[str] = set()  # keywords for keys that can unlock this exit

    def __bool__(self):
        return bool(self.location)

    def __repr__(self):
        return f"<Exit to {self.location}>"
    
    def dump(self) -> dict:
        return {
            "location": self.location.dump(),
            "flags": list(self.flags),
            "keywords": self.keywords,
            "description": self.description,
            "dchide": self.dchide,
            "dclock": self.dclock,
            "keys": list(self.keys),
        }
    
    @classmethod
    def load(cls, data: dict) -> "Exit":
        exit = cls(Location.load(data["location"]))
        exit.flags = set(data.get("flags", list()))
        exit.keywords = data.get("keywords", "")
        exit.description = data.get("description", "")
        exit.dchide = data.get("dchide", 0)
        exit.dclock = data.get("dclock", 0)
        exit.keys = set(data.get("keys", list()))
        return exit

class Shape(HasColorName, HasColorDescription):

    def __init__(self, grid: "Grid", point: POINT):
        HasColorName.__init__(self)
        HasColorDescription.__init__(self)
        self.grid: "Grid" = grid
        self.point: POINT = point
        self.type: str = "box"
        self.sector_type: str = ""
        self.tile_display: str = ""
        self.north: int = 0
        self.south: int = 0
        self.east: int = 0
        self.west: int = 0
        self.up: int = 0
        self.down: int = 0
        self.priority: int = 0

    def dump(self) -> dict:
        return {
            "type": self.type,
            "sector_type": self.sector_type,
            "tile_display": self.tile_display,
            "north": self.north,
            "south": self.south,
            "east": self.east,
            "west": self.west,
            "up": self.up,
            "down": self.down,
            "priority": self.priority,
        }
    
    @classmethod
    def load(cls, grid: "Grid", point: POINT, data: dict) -> "Shape":
        shape = cls(grid, point)
        shape.type = data["type"]
        shape.sector_type = data["sector_type"]
        shape.tile_display = data.get("tile_display", "")
        shape.north = data.get("north", 0)
        shape.south = data.get("south", 0)
        shape.east = data.get("east", 0)
        shape.west = data.get("west", 0)
        shape.up = data.get("up", 0)
        shape.down = data.get("down", 0)
        shape.priority = data.get("priority", 0)
        return shape
    
    def save(self):
        self.grid.save()


class Tile(HasColorName, HasColorDescription, HasDgScripts, HasFlags, HasExtraDescriptions):
    slug_type: str = "tile"

    def __init__(self, grid: "Grid", point: POINT):
        HasColorName.__init__(self)
        HasColorDescription.__init__(self)
        HasDgScripts.__init__(self)
        HasFlags.__init__(self)
        HasExtraDescriptions.__init__(self)
        self.grid: "Grid" = grid
        self.point: POINT = point
        self.slug: str = ""
        self.sector_type: str = ""
        self.proto_script: list[str] = list()
        self.ground_effect: int = 0
        self.damage: int = 0
        self.reset_commands: list[ResetCommand] = list()
        self.exits: dict[Direction, Exit] = dict()
        self.tile_display: str = ""
    
    def __repr__(self):
        return f"<Tile:{self.point} in {self.grid}>"

    def as_location(self) -> Location:
        return Location(self.grid.location_type, self.grid.id, self.point)

    def dump(self) -> dict:
        return {
            "slug": self.slug,
            "color_name": self.color_name.markup,
            "color_description": self.color_description.markup,
            "sector_type": self.sector_type,
            "proto_script": self.proto_script,
            "ground_effect": self.ground_effect,
            "damage": self.damage,
            "reset_commands": [asdict(cmd) for cmd in self.reset_commands],
            "exits": {direction.value: exit.dump() for direction, exit in self.exits.items()},
            "tile_display": self.tile_display,
            "extra_descriptions": [{"keywords": ed.keywords, "description": ed.description} for ed in self.extra_descriptions],
            "flags": list(self.flags),
        }
    
    @classmethod
    def load(cls, grid: "Grid", point: POINT, data: dict) -> "Tile":
        tile = cls(grid, point)
        tile.slug = data["slug"]
        tile.color_name = Text.from_markup(data["color_name"])
        tile.color_description = Text.from_markup(data["color_description"])
        tile.sector_type = data["sector_type"]
        tile.proto_script = data.get("proto_script", list())
        tile.ground_effect = data.get("ground_effect", 0)
        tile.damage = data.get("damage", 0)
        tile.reset_commands = [ResetCommand.load(cmd) for cmd in data.get("reset_commands", list())]
        tile.exits = {Direction(direction): Exit.load(exit_data) for direction, exit_data in data.get("exits", dict()).items()}
        tile.tile_display = data.get("tile_display", "")
        tile.extra_descriptions = [ExtraDescription(ed["keywords"], ed["description"]) for ed in data.get("extra_descriptions", list())]
        tile.flags = set(data.get("flags", list()))
        return tile

    def execute_reset_commands(self):
        for cmd in self.reset_commands:
            cmd.execute(self)
    
    def save(self):
        self.grid.save()

class Grid(IsLocation, HasFlags, HasColorName, HasColorDescription):
    """
    This is an Abstract class representing a 3D tilegrid.
    Two implementations: Zones, and Structures.
    Zones are used for the main world map.
    Structures are instances for vehicles, dungeons, etc.
    """

    def __init__(self):
        IsLocation.__init__(self)
        HasFlags.__init__(self)
        HasColorName.__init__(self)
        HasColorDescription.__init__(self)
        self.id: uuid.UUID = uuid.UUID(int=0)
        self.slug: str = ""
        self.shapes: dict[POINT, Shape] = dict()
        self.tiles: dict[POINT, Tile] = dict()
        self.landing_spots: dict[str, Location] = dict()
        self.docking_spots: dict[str, Location] = dict()
        self.default_point: POINT = (0, 0, 0)
        self._shape_index: dict[POINT, list[tuple[Shape, int]]] = {}  # point -> [(shape, priority), ...]
        self._shape_index_dirty: bool = True

    def dump(self) -> dict:
        return {
            "id": str(self.id),
            "color_name": self.color_name.markup,
            "color_description": self.color_description.markup,
            "flags": list(self.flags),
            "shapes": [[list(point), shape.dump()] for point, shape in self.shapes.items()],
            "tiles": [[list(point), tile.dump()] for point, tile in self.tiles.items()],
            "landing_spots": {name: loc.dump() for name, loc in self.landing_spots.items()},
            "docking_spots": {name: loc.dump() for name, loc in self.docking_spots.items()},
            "default_point": list(self.default_point),
            "slug": self.slug,
        }
    
    @classmethod
    def load_grid(cls, grid, data: dict) -> "Grid":
        grid.id = uuid.UUID(data["id"])
        grid.color_name = Text.from_markup(data["color_name"])
        grid.color_description = Text.from_markup(data["color_description"])
        grid.flags = set(data["flags"])
        grid.slug = data.get("slug", "")
        # Load shapes and tiles...
        grid.shapes = {tuple(shape_data[0]): Shape.load(grid, tuple(shape_data[0]), shape_data[1]) for shape_data in data.get("shapes", list())}
        grid.tiles = {tuple(tile_data[0]): Tile.load(grid, tuple(tile_data[0]), tile_data[1]) for tile_data in data.get("tiles", list())}
        grid.landing_spots = {name: Location.load(loc_data) for name, loc_data in data.get("landing_spots", dict()).items()}
        grid.docking_spots = {name: Location.load(loc_data) for name, loc_data in data.get("docking_spots", dict()).items()}
        grid.default_point = tuple(data.get("default_point", (0, 0, 0)))
        return grid

    def reset_grid(self):
        for tile in self.tiles.values():
            tile.execute_reset_commands()

    def valid_location_coordinates(self, point: POINT) -> bool:
        # A location is valid if it has a tile or shape at that point.
        return point in self.tiles or self.query_shape(point) is not None

    def _invalidate_shape_index(self):
        self._shape_index_dirty = True

    def add_shape(self, shape: Shape):
        """Add a shape to the grid and invalidate the spatial index."""
        self.shapes[shape.point] = shape
        self._invalidate_shape_index()

    def remove_shape(self, point: POINT) -> Shape | None:
        """Remove a shape from the grid and invalidate the spatial index."""
        shape = self.shapes.pop(point, None)
        if shape:
            self._invalidate_shape_index()
        return shape

    def _build_shape_index(self):
        """Build spatial index mapping each point to shapes that cover it, sorted by priority (highest first)."""
        self._shape_index.clear()
        
        for shape_point, shape in self.shapes.items():
            # Generate all points covered by this shape based on its type
            covered_points = self._get_shape_points(shape_point, shape)
            
            for point in covered_points:
                if point not in self._shape_index:
                    self._shape_index[point] = []
                self._shape_index[point].append((shape, shape.priority))
        
        # Sort each point's list by priority (highest first)
        for point in self._shape_index:
            self._shape_index[point].sort(key=lambda x: x[1], reverse=True)
        
        self._shape_index_dirty = False

    def _get_shape_points(self, shape_point: POINT, shape: Shape) -> set[POINT]:
        """Generate all points covered by a shape based on its type."""
        points = set()
        
        if shape.type == "round":
            # Round shapes use Manhattan distance (diamond in 2D, diamond prism in 3D)
            # radius in each direction
            radius_x = shape.east + shape.west
            radius_y = shape.north + shape.south
            radius_z = shape.up + shape.down
            
            for x in range(shape_point[0] - radius_x, shape_point[0] + radius_x + 1):
                for y in range(shape_point[1] - radius_y, shape_point[1] + radius_y + 1):
                    for z in range(shape_point[2] - radius_z, shape_point[2] + radius_z + 1):
                        # Manhattan distance from center
                        dist = abs(x - shape_point[0]) + abs(y - shape_point[1]) + abs(z - shape_point[2])
                        max_dist = radius_x + radius_y + radius_z
                        if dist <= max_dist:
                            points.add((x, y, z))
        else:
            # Default to box (rectangular prism)
            min_x = shape_point[0] - shape.west
            max_x = shape_point[0] + shape.east
            min_y = shape_point[1] - shape.south
            max_y = shape_point[1] + shape.north
            min_z = shape_point[2] - shape.down
            max_z = shape_point[2] + shape.up
            
            for x in range(min_x, max_x + 1):
                for y in range(min_y, max_y + 1):
                    for z in range(min_z, max_z + 1):
                        points.add((x, y, z))
        
        return points

    def query_shape(self, point: POINT) -> Shape | None:
        """Query the highest-priority shape covering the given point. Returns None if no shape covers it."""
        if self._shape_index_dirty:
            self._build_shape_index()
        
        if point not in self._shape_index:
            return None
        
        # Return the first (highest priority) shape
        shapes_at_point = self._shape_index[point]
        if shapes_at_point:
            return shapes_at_point[0][0]
        return None

    def query_shape_all(self, point: POINT) -> list[Shape]:
        """Query all shapes covering the given point, sorted by priority (highest first)."""
        if self._shape_index_dirty:
            self._build_shape_index()
        
        if point not in self._shape_index:
            return []
        
        return [s for s, _ in self._shape_index[point]]

    def generate_exits_at(self, point: POINT) -> dict[Direction, Exit]:
        exits = dict()
        for direction in Direction:
            if direction in [Direction.inside, Direction.outside]:
                continue  # inside/outside exits are special and not generated here
            adjacent_point = direction.update_coordinates(point)
            if self.valid_location_coordinates(adjacent_point):
                exits[direction] = Exit(Location(self.location_type, self.id, adjacent_point))
        if point in self.tiles:
            exits.update(self.tiles[point].exits)
        return exits
    
    def get_exit_for_direction(self, point: POINT, direction: Direction) -> Exit | None:
        if point in self.tiles and direction in self.tiles[point].exits:
            return self.tiles[point].exits[direction]
        
        adjacent_point = direction.update_coordinates(point)
        if not self.valid_location_coordinates(adjacent_point):
            return None
        return Exit(Location(self.location_type, self.id, adjacent_point))
    
    def get_display_name(self, point: POINT, viewer: "Character") -> str:
        if point in self.tiles and self.tiles[point].color_name:
            return self.tiles[point].color_name
        shape = self.query_shape(point)
        if shape and shape.color_name:
            return shape.color_name
        return "Unremarkable Ground"
    
    def get_display_description(self, point: POINT, viewer: "Character") -> str:
        if point in self.tiles and self.tiles[point].color_description:
            return self.tiles[point].color_description
        shape = self.query_shape(point)
        if shape and shape.color_description:
            return shape.color_description
        return ""