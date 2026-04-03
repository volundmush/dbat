import uuid

import dbat
from .grids import Grid, POINT, Tile, Shape, Exit, Direction, ResetCommand
from .location import HasLocation, IsLocation, Location
from .misc import HasColorName, HasColorDescription, HasInteractive, HasFlags
from .inventory import HasInventory
import copy

class ExitPrototype(HasFlags):
    """
    ExitPrototypes are templates for spawning Exits.
    This is useful for things like player-owned structures, dungeon instances, etc.
    """

    def __init__(self):
        # Unlike real exits, prototypes only have coordinates.
        # This is because they can only point at a location within
        # the grid they're defined in.
        HasFlags.__init__(self)
        self.location: POINT = (0,0,0)
        self.keywords: str = ""
        self.description: str = ""
        self.dchide: int = 0
        self.dclock: int = 0
        self.keys: set[str] = set()
    
    def spawn(self, tile: Tile) -> Exit:
        exit = Exit(Location(tile.grid.entity_type, tile.grid.id, self.location))
        exit.flags = self.flags.copy()
        exit.keywords = self.keywords
        exit.description = self.description
        exit.dchide = self.dchide
        exit.dclock = self.dclock
        exit.keys = self.keys.copy()
        return exit

class TilePrototype(HasColorName, HasColorDescription, HasFlags):
    """
    TilePrototypes are templates for spawning Tiles.
    This is useful for things like player-owned structures, dungeon instances, etc.
    """

    def __init__(self):
        HasColorName.__init__(self)
        HasColorDescription.__init__(self)
        HasFlags.__init__(self)
        self.exits: dict[Direction, ExitPrototype] = dict()
        self.sector_type: str = ""
        self.reset_commands: list[ResetCommand] = list()
        self.tile_display: str = ""

    def spawn(self, grid: Grid, point: POINT) -> Tile:
        tile = Tile(grid, point)
        tile.color_name = self.color_name.copy()
        tile.color_description = self.color_description.copy()
        tile.flags = self.flags.copy()
        for direction, exit_proto in self.exits.items():
            tile.exits[direction] = exit_proto.spawn(tile)
        tile.reset_commands = copy.deepcopy(self.reset_commands)
        return tile


class ShapePrototype(HasColorName, HasColorDescription):
    """
    ShapePrototypes are templates for spawning Shapes.
    This is useful for things like player-owned structures, dungeon instances, etc.
    """

    def __init__(self):
        HasColorName.__init__(self)
        HasColorDescription.__init__(self)
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

    def spawn(self, grid: Grid, point: POINT) -> Shape:
        shape = Shape(grid, point)
        shape.id = uuid.uuid4()
        shape.color_name = self.color_name.copy()
        shape.color_description = self.color_description.copy()
        shape.type = self.type
        shape.sector_type = self.sector_type
        shape.tile_display = self.tile_display
        shape.north = self.north
        shape.south = self.south
        shape.east = self.east
        shape.west = self.west
        shape.up = self.up
        shape.down = self.down
        shape.priority = self.priority
        return shape

class StructurePrototype(HasColorName, HasColorDescription, HasFlags):
    """
    StructurePrototypes are templates for spawning Structures.
    This is useful for things like player-owned structures, dungeon instances, etc.
    """

    def __init__(self):
        HasColorName.__init__(self)
        HasColorDescription.__init__(self)
        HasFlags.__init__(self)
        self.shapes: dict[POINT, ShapePrototype] = dict()
        self.tiles: dict[POINT, TilePrototype] = dict()
        self.landing_spots: dict[str, POINT] = dict() # name to coordinates mapping for landing spots. These are used for things like exits, entrances, etc.
        self.docking_spots: dict[str, POINT] = dict() # name to coordinates mapping for docking spots. These are used for things like exits, entrances, etc.
        self.default_point: POINT = (0, 0, 0) # The default point is used for things like spawning characters, objects, etc. It should be a valid point within the structure.
        self.instances: set[Structure] = set()

    def spawn(self) -> Structure:
        structure = Structure()
        structure.proto = self
        structure.color_name = self.color_name.copy()
        structure.color_description = self.color_description.copy()
        structure.flags = self.flags.copy()
        structure.proto = self
        structure.default_point = self.default_point
        for point, shape_proto in self.shapes.items():
            structure.shapes[point] = shape_proto.spawn(structure, point)
        for point, tile_proto in self.tiles.items():
            structure.tiles[point] = tile_proto.spawn(structure, point)
        
        structure.game_activate()
        return structure

class Structure(Grid, HasLocation, IsLocation, HasInteractive, HasInventory):
    """
    Structures are a dynamic, possibly player-owned instance of a space. They are loaded
    after Zones. Unlike Zones, they have a Location.

    """
    slug_type: str = "structure"
    location_type: str = "structure"

    def __init__(self):
        Grid.__init__(self)
        HasLocation.__init__(self)
        IsLocation.__init__(self)
        HasInteractive.__init__(self)
        HasFlags.__init__(self)
        HasInventory.__init__(self)
        self.deleted = False
        self.proto: StructurePrototype | None = None
    
    def __bool__(self):
        return not self.deleted
    
    def __repr__(self):
        return f"<Structure: {self.color_name.plain} ({self.id}){f" {self.slug}" if self.slug else ""}>"
    
    def game_activate(self):
        dbat.STRUCTURES[self.id] = self
        if self.proto:
            self.proto.instances.add(self)

    def game_deactivate(self):
        if self.proto:
            self.proto.instances.discard(self)
        dbat.STRUCTURES.pop(self.id, None)

    def save(self):
        dbat.DIRTY_STRUCTURES.add(self.id)
    
    def dump(self) -> dict:
        out = super().dump()

        return out

    @classmethod
    def load(cls, data: dict) -> "Structure":
        structure = cls()
        Grid.load_grid(structure, data)
        # Load shapes and tiles...
        return structure