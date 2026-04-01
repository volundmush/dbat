from .location import POINT, IsLocation, Location
from enum import Enum

from .dgscripts import HasDgScripts

POINT = tuple[int, int, int]  # (x, y, z) coordinates in a grid

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

class Destination:

    __slots__ = ("location", "flags", "keywords", "description")

    def __init__(self, location: Location):
        self.location: Location  = Location
        self.flags: set[str] = set()
        self.keywords: str = ""
        self.description: str = ""

class Shape:

    __slots__ = ("grid", "point", "sector_type", "tile_display", "description", "north", "south", "east", "west", "up", "down")

    def __init__(self, grid: "Grid", point: POINT):
        self.grid: "Grid" = grid
        self.point: POINT = point
        self.sector_type: int = 0
        self.tile_display: str = ""
        self.description: str = ""
        self.north: int = 0
        self.south: int = 0
        self.east: int = 0
        self.west: int = 0
        self.up: int = 0
        self.down: int = 0
        self.priority: int = 0


class Tile(HasDgScripts):
    slug_type: str = "tile"

    __slots__ = ("slug", "name", "description", "sector_type", "proto_script", 
                "ground_effect", "damage", "reset_commands", "exits")

    def __init__(self, grid: "Grid", point: POINT):
        super().__init__()
        self.grid: "Grid" = grid
        self.point: POINT = point
        self.slug: str = ""
        self.name: str = ""
        self.description: str = ""
        self.sector_type: int | None = None
        self.proto_script: list[int] = list()
        self.ground_effect: int = 0
        self.damage: int = 0
        self.reset_commands: list[dict] = list()
        self.exits: dict[Direction, Destination] = dict()

class Grid(IsLocation):
    """
    This is an Abstract class representing a 3D tilegrid.
    Two implementations: Zones, and Structures.
    Zones are used for the main world map.
    Structures are instances for vehicles, dungeons, etc.
    """

    def __init__(self):
        self.name: str = ""
        self.description: str = ""
        self.shapes: dict[POINT, Shape] = dict()
        self.tiles: dict[POINT, Tile] = dict()

    def valid_location_coordinates(self, point: POINT) -> bool:
        # TODO: add proper shape handling still...
        return point in self.shapes or point in self.tiles
