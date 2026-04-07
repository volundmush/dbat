import typing
import uuid
from pydantic import BaseModel, Field, ConfigDict, PrivateAttr, model_validator

from loguru import logger

from dbat.types.characters import Mobile, MobilePrototype
from dbat.types.objects import Object, ObjectPrototype
from .dgscripts import DgReference, TriggerType, HasDgScripts
from .location import Point, IsLocation, Location
from .misc import HasFlags, HasExtraDescriptions, ExtraDescription, HasColorName, HasColorDescription
from enum import Enum
from rich.text import Text
import random
import dbat

from .dgscripts import HasDgScripts


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

    def update_coordinates(self, coor: Point) -> Point:
        match self:
            case Direction.north:
                return Point(x=coor.x, y=coor.y + 1, z=coor.z)
            case Direction.east:
                return Point(x=coor.x + 1, y=coor.y, z=coor.z)
            case Direction.south:
                return Point(x=coor.x, y=coor.y - 1, z=coor.z)
            case Direction.west:
                return Point(x=coor.x - 1, y=coor.y, z=coor.z)
            case Direction.up:
                return Point(x=coor.x, y=coor.y, z=coor.z + 1)
            case Direction.down:
                return Point(x=coor.x, y=coor.y, z=coor.z - 1)
            case Direction.northeast:
                return Point(x=coor.x + 1, y=coor.y + 1, z=coor.z)
            case Direction.southeast:
                return Point(x=coor.x + 1, y=coor.y - 1, z=coor.z)
            case Direction.southwest:
                return Point(x=coor.x - 1, y=coor.y - 1, z=coor.z)
            case Direction.northwest:
                return Point(x=coor.x - 1, y=coor.y + 1, z=coor.z)
            case _:
                return coor.model_copy()


class ResetCommand(BaseModel):
    command: str = ""
    target: str = ""
    max_world: int = 0
    max_location: int = 0
    chance: int = 100
    ex: int = 0
    key: str = ""
    value: str = ""
    subcommands: list[ResetCommand] = Field(default_factory=list)

    def execute(self, tile: "Tile", last_spawned=None):
        successful = False
        spawned: Object | Mobile = None

        if self.chance < 100:
            roll = random.randint(1, 100)
            if roll > self.chance:
                return  # Command fails due to chance
        
        loc = tile.as_location()

        def perform_counts(proto):
            if self.max_world > 0 and len(proto._instances) >= self.max_world:
                return False  # Max world limit reached, fail silently
            if self.max_location > 0:
                instances = list()
                for instance_id in proto._instances:
                    instance = dbat.INDEX.entities.get(instance_id, None)
                    if instance and instance.spawn_location and instance.spawn_location == loc:
                        instances.append(instance)
                # Count how many instances of this proto are in the current location
                count = sum(1 for instance in instances)
                if count >= self.max_location:
                    return False  # Max location limit reached, fail silently
            return True

        match self.command:
            case "MOB":
                if not (proto := dbat.INDEX.mobile_prototypes.get(self.target, None)):
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
                if not (proto := dbat.INDEX.object_prototypes.get(self.target, None)):
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
                if not (proto := dbat.INDEX.object_prototypes.get(self.target, None)):
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
                if not (proto := dbat.INDEX.object_prototypes.get(self.target, None)):
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
                if not (proto := dbat.INDEX.object_prototypes.get(self.target, None)):
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
                if not (dgscript := dbat.INDEX.dgscripts.get(self.target, None)):
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

class ExitBase(HasFlags):
    keywords: str = ""
    description: str = ""
    dchide: int = 0
    dclock: int = 0
    keys: set[str] = Field(default_factory=set, description="A set of key object prototypes that can unlock this exit")


class Exit(ExitBase):
    location: Location = Field(default_factory=Location, description="The location this exit leads to")

    def __bool__(self):
        return bool(self.location)

    def __repr__(self):
        return f"<Exit to {self.location}>"

class ShapeBase(HasColorName, HasColorDescription):
    point: Point = Field(default_factory=Point, description="The coordinates of this shape within the grid")
    type: str = Field(default="box", description="The type of shape, such as 'box' or 'round'")
    sector_type: str = Field(default="", description="The sector type of this shape, such as 'inside', 'city', 'field', etc.")
    tile_display: str = Field(default="", description="The tile display string for this shape, used for map rendering")
    north: int = Field(default=0, description="How far this shape extends to the north.")
    south: int = Field(default=0, description="How far this shape extends to the south.")
    east: int = Field(default=0, description="How far this shape extends to the east.")
    west: int = Field(default=0, description="How far this shape extends to the west.")
    up: int = Field(default=0, description="How far this shape extends upwards.")
    down: int = Field(default=0, description="How far this shape extends downwards.")
    priority: int = Field(default=0, description="The priority of this shape when determining which shape covers a point. Higher priority shapes will take precedence over lower priority shapes.")


class HasGrid(BaseModel):
    grid_type: str = Field(..., description="The type of grid this entity is located in, such as 'zone' or 'structure'")
    grid_id: uuid.UUID = Field(..., description="The ID of the grid this entity is located in")

    def grid(self):
        match self.grid_type:
            case "zone":
                return dbat.INDEX.get_zone(self.grid_id)
            case "structure":
                return dbat.INDEX.get_structure(self.grid_id)
            case _:
                return None


class Shape(ShapeBase, HasGrid):
    pass

class TileBase(HasColorName, HasColorDescription, HasDgScripts, HasFlags, HasExtraDescriptions):
    point: Point = Field(default_factory=Point, description="The coordinates of this tile within the grid")
    sector_type: str = Field(default="", description="The sector type of this tile, such as 'inside', 'city', 'field', etc.")
    reset_commands: list[ResetCommand] = Field(default_factory=list, description="A list of reset commands to execute when this tile resets.")
    tile_display: str = Field(default="", description="The tile display string for this tile, used for map rendering")

class Tile(TileBase, HasGrid):
    slug: str = Field(default="", description="The slug for this tile, used for referencing it in reset commands and such.")
    ground_effect: int = Field(default=0, description="The ground effect for this tile, such as damage or healing.")
    damage: int = Field(default=0, description="The damage this tile does to characters that end their turn on it.")
    exits: dict[Direction, Exit] = Field(default_factory=dict, description="A dictionary mapping directions to exits from this tile.")
    
    def report_slug_type(self):
        return "tile"

    def __str__(self):
        return repr(self)

    def __repr__(self):
        return f"<Tile:{self.point} in {self.grid()}>"

    def as_location(self) -> Location | None:
        if not (g := self.grid()):
            return None
        return Location(location_type=g.report_location_type(), location_id=g.id, point=self.point)

    def execute_reset_commands(self):
        for cmd in self.reset_commands:
            cmd.execute(self)
    
    def as_dg_ref(self):
        return DgReference(entity_type=self.grid_type, entity_id=self.grid_id, point=self.point)

class Grid(IsLocation, HasFlags, HasColorName, HasColorDescription):
    """
    This is an Abstract class representing a 3D tilegrid.
    Two implementations: Zones, and Structures.
    Zones are used for the main world map.
    Structures are instances for vehicles, dungeons, etc.
    """
    id: uuid.UUID = Field(..., description="The unique identifier for this grid")
    slug: str = Field("", description="The slug for this grid, used for referencing it in reset commands and such.")
    shapes: dict[Point, Shape] = Field(default_factory=dict, description="A dictionary mapping coordinates to shapes in this grid")
    tiles: dict[Point, Tile] = Field(default_factory=dict, description="A dictionary mapping coordinates to tiles in this grid")
    landing_spots: dict[str, Location] = Field(default_factory=dict, description="A dictionary mapping landing spot names to locations in this grid. Used for vehicles and such.")
    docking_spots: dict[str, Location] = Field(default_factory=dict, description="A dictionary mapping docking spot names to locations in this grid. Used for vehicles and such.")
    default_point: Point = Field(default_factory=Point, description="The default coordinates for this grid, used for placing entities that don't have a specific location within the grid.")
    
    __shape_index: dict[Point, list[tuple[Shape, int]]] = PrivateAttr(default_factory=dict) 
    __shape_index_dirty: bool = PrivateAttr(default=True)

    def __repr__(self):
        return f"<{self.__class__.__name__}: {self.color_name.plain} ({self.id}){f' {self.slug}' if self.slug else ''}>"

    def __str__(self):
        return repr(self)

    @model_validator(mode="before")
    @classmethod
    def convert_dicts(cls, data: dict) -> dict:
        for field in ("shapes", "tiles"):
            data[field] = Point.deserialize_dict(data.get(field, dict()))
        return data

    def reset_grid(self):
        for tile in self.tiles.values():
            tile.execute_reset_commands()

    def valid_location_coordinates(self, point: Point) -> bool:
        # A location is valid if it has a tile or shape at that point.
        return point in self.tiles or self.query_shape(point) is not None

    def _invalidate_shape_index(self):
        self.__shape_index_dirty = True

    def add_shape(self, shape: Shape):
        """Add a shape to the grid and invalidate the spatial index."""
        self.shapes[shape.point] = shape
        self._invalidate_shape_index()

    def remove_shape(self, point: Point) -> Shape | None:
        """Remove a shape from the grid and invalidate the spatial index."""
        shape = self.shapes.pop(point, None)
        if shape:
            self._invalidate_shape_index()
        return shape

    def _build_shape_index(self):
        """Build spatial index mapping each point to shapes that cover it, sorted by priority (highest first)."""
        self.__shape_index.clear()
        
        for shape_point, shape in self.shapes.items():
            # Generate all points covered by this shape based on its type
            covered_points = self._get_shape_points(shape_point, shape)
            
            for point in covered_points:
                if point not in self.__shape_index:
                    self.__shape_index[point] = []
                self.__shape_index[point].append((shape, shape.priority))
        
        # Sort each point's list by priority (highest first)
        for point in self.__shape_index:
            self.__shape_index[point].sort(key=lambda x: x[1], reverse=True)
        
        self.__shape_index_dirty = False

    def _get_shape_points(self, shape_point: Point, shape: Shape) -> set[Point]:
        """Generate all points covered by a shape based on its type."""
        points = set()
        
        if shape.type == "round":
            # Round shapes use Manhattan distance (diamond in 2D, diamond prism in 3D)
            # radius in each direction
            radius_x = shape.east + shape.west
            radius_y = shape.north + shape.south
            radius_z = shape.up + shape.down
            
            for x in range(shape_point.x - radius_x, shape_point.x + radius_x + 1):
                for y in range(shape_point.y - radius_y, shape_point.y + radius_y + 1):
                    for z in range(shape_point.z - radius_z, shape_point.z + radius_z + 1):
                        # Manhattan distance from center
                        dist = abs(x - shape_point.x) + abs(y - shape_point.y) + abs(z - shape_point.z)
                        max_dist = radius_x + radius_y + radius_z
                        if dist <= max_dist:
                            points.add((x, y, z))
        else:
            # Default to box (rectangular prism)
            min_x = shape_point.x - shape.west
            max_x = shape_point.x + shape.east
            min_y = shape_point.y - shape.south
            max_y = shape_point.y + shape.north
            min_z = shape_point.z - shape.down
            max_z = shape_point.z + shape.up
            
            for x in range(min_x, max_x + 1):
                for y in range(min_y, max_y + 1):
                    for z in range(min_z, max_z + 1):
                        points.add((x, y, z))
        
        return points

    def query_shape(self, point: Point) -> Shape | None:
        """Query the highest-priority shape covering the given point. Returns None if no shape covers it."""
        if self.__shape_index_dirty:
            self._build_shape_index()
        
        if point not in self.__shape_index:
            return None
        
        # Return the first (highest priority) shape
        shapes_at_point = self.__shape_index[point]
        if shapes_at_point:
            return shapes_at_point[0][0]
        return None

    def query_shape_all(self, point: Point) -> list[Shape]:
        """Query all shapes covering the given point, sorted by priority (highest first)."""
        if self.__shape_index_dirty:
            self._build_shape_index()
        
        if point not in self.__shape_index:
            return []
        
        return [s for s, _ in self.__shape_index[point]]

    def generate_exits_at(self, point: Point) -> dict[Direction, Exit]:
        exits = dict()
        for direction in Direction:
            if direction in [Direction.inside, Direction.outside]:
                continue  # inside/outside exits are special and not generated here
            adjacent_point = direction.update_coordinates(point)
            if self.valid_location_coordinates(adjacent_point):
                exits[direction] = Exit(location=Location(location_type=self.report_location_type(), location_id=self.id, point=adjacent_point))
        if point in self.tiles:
            exits.update(self.tiles[point].exits)
        return exits
    
    def get_exit_for_direction(self, point: Point, direction: Direction) -> Exit | None:
        if point in self.tiles and direction in self.tiles[point].exits:
            return self.tiles[point].exits[direction]
        
        adjacent_point = direction.update_coordinates(point)
        if not self.valid_location_coordinates(adjacent_point):
            return None
        return Exit(location=Location(location_type=self.report_location_type(), location_id=self.id, point=adjacent_point))
    
    def get_display_name(self, point: Point, viewer: "Character") -> Text:
        if point in self.tiles and self.tiles[point].color_name:
            return self.tiles[point].color_name
        shape = self.query_shape(point)
        if shape and shape.color_name:
            return shape.color_name
        return Text("Unremarkable Ground")
    
    def get_display_description(self, point: Point, viewer: "Character") -> Text:
        if point in self.tiles and self.tiles[point].color_description:
            return self.tiles[point].color_description
        shape = self.query_shape(point)
        if shape and shape.color_description:
            return shape.color_description
        return Text("")