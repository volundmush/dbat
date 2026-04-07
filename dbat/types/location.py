import dbat
import uuid
import typing
import math
from pydantic import BaseModel, ConfigDict, Field, PrivateAttr
from rich.text import Text
from muplugins.core.db.fields import RichText



class Point(BaseModel):
    model_config = ConfigDict(frozen=True)
    x: int = Field(0, description="The X coordinate")
    y: int = Field(0, description="The Y coordinate")
    z: int = Field(0, description="The Z coordinate")

    @classmethod
    def from_str(cls, s: str) -> "Point":
        parts = s.split(" ")
        if len(parts) != 3:
            raise ValueError(f"Invalid point string: {s}")
        # we should now have a sequence of ["x=0", "y=0", "z=0"]
        x, y, z = (int(part.split("=")[1]) for part in parts)
        return cls(x=x, y=y, z=z)

    @classmethod
    def deserialize_dict(cls, d: dict[str, typing.Any]) -> dict[Point, typing.Any]:
        return {Point.from_str(k): v for k, v in d.items()}

class IsLocation(BaseModel):
    """
    A base class for objects that "are map locations" where characters and objects can be.
    Currently this is just Grids (Zones and Structures).
    """
    __contents: list[uuid.UUID] = PrivateAttr(default_factory=list)

    def report_location_type(self) -> str:
        """
        The string type of this location.
        """
        raise NotImplementedError("report_location_type must be implemented by subclasses of IsLocation")

    def iter_contents(self):
        for ent_id in self.__contents.copy():
            if (e := dbat.INDEX.entities.get(ent_id, None)) is not None:
                yield e
    
    def iter_contents_at(self, point: Point):
        for ent_id in self.__contents.copy():
            if (e := dbat.INDEX.entities.get(ent_id, None)) is not None and e.location and e.location.point == point:
                yield e

    def valid_location_coordinates(self, point: Point) -> bool:
        """
        Returns whether the given coordinates are valid for this object.
        For example, a room might only have one valid coordinate (0,0,0), while a zone might have many.
        """
        return False

    def add_content(self, con: "HasLocation", loc: Point, loading: bool = False):
        """
        Adds an entity to this location. This should only be called by the HasLocation.add_to_location method.
        """
        self.__contents.append(con.id)
        self.on_contents_add(con, loc, loading)

    def on_contents_add(self, con: "HasLocation", new_loc: Point, loading: bool = False):
        """
        This is called when an entity is added to this location. It can be used to trigger events, etc.
        """

    def relocate_content(self, con: "HasLocation", old_loc: Point, new_loc: Point):
        self.on_contents_relocate(con, old_loc, new_loc)

    def on_contents_relocate(self, con: "HasLocation", old_loc: Point, new_loc: Point):
        """
        This is called when an entity was already IN this location, but is changing coordinates.
        """

    def remove_content(self, con: "HasLocation", old_loc: Point):
        self.__contents.remove(con.id)
        self.on_contents_remove(con, old_loc)

    def on_contents_remove(self, con: "HasLocation", old_loc: Point):
        """
        This is called when an entity was in this location, and is now removed from it.
        """
    
    def make_location(self, point: Point) -> Location:
        return Location(self.report_location_type(), self.id, point)


class TileDisplay(BaseModel):
    sector_type: str = Field(default="", description="The type of sector this tile is. This determines the base color and description of the tile.")
    tile_override: RichText | None = Field(default=None, description="If set, this tile will display as this instead of its normal appearance.")

    @classmethod
    def create_from_location(cls, location: Location, viewer: "Character") -> TileDisplay:
        target = location.get_target()
        point = location.point

        sector_type = ""
        tile_override = None

        if shape := target.query_shape(point):
            if shape.sector_type:
                sector_type = shape.sector_type
            if shape.tile_display:
                tile_override = shape.tile_display
        if point in target.tiles:
            tile = target.tiles[point]
            if tile.sector_type:
                sector_type = tile.sector_type
            if tile.tile_display:
                tile_override = tile.tile_display
        
        return cls(sector_type=sector_type, tile_override=tile_override)

    def display(self) -> Text:
        return Text("?")

class RenderedMap(BaseModel):
    """
    A section of map rendered for 2D view. It's tiles in rows and columns.
    """
    height: int = Field(..., description="The height of the rendered map, in tiles.")
    width: int = Field(..., description="The width of the rendered map, in tiles.")
    tiles: list[list[TileDisplay | None]] = Field(..., description="The tiles of the rendered map.")

    @classmethod
    def create_from_location(cls, location: Location, viewer: "Character", height: int = 7, width: int = 7) -> RenderedMap:
        # We can assume that height and width will always be odd and at least 3.
        # the given Location is our center point.

        # For this, we will be ignoring exits leading up, down, inward, or outward.
        # We also don't use the actual coordinates of Locations, but instead generate a relative coordinate
        # system based on directional traversals, where our center point is (0,0).

        # loc_map stores the places we've been by relative coordinates.
        # in the case of inconsistent geometry, it's first come first registered.
        loc_map: dict[Point, Location] = dict()

        # we will use breadth-first traversal to generate the map, starting from the center point.
        visited: set[Location] = set()

        queue: list[tuple[Location, Point]] = [(location, Point(x=0, y=0, z=0))]

        from ..types.grids import Direction

        while queue:
            loc, point = queue.pop(0)
            x, y, z = point.x, point.y, point.z
            if loc in visited:
                continue
            visited.add(loc)

            # boundary check
            if abs(x) > (width-1) / 2 or abs(y) > (height-1) / 2:
                continue

            loc_map[point] = loc

            if not (target := loc.get_target()):
                continue
            
            exits = target.generate_exits_at(loc.point)
            for direction, exit in exits.items():
                if direction in (Direction.up, Direction.down, Direction.inside, Direction.outside):
                    continue
                if exit.location not in visited:
                    queue.append((exit.location, direction.update_coordinates(point)))

        tiles = list()

        for y in range(-(height-1)//2, (height-1)//2 + 1):
            row = list()
            for x in range(-(width-1)//2, (width-1)//2 + 1):
                point = Point(x=x, y=y, z=0)
                if point in loc_map:
                    loc = loc_map[point]
                    row.append(TileDisplay.create_from_location(loc, viewer))
                else:
                    row.append(None)
            tiles.append(row)

        return cls(height=height, width=width, tiles=tiles)

class Location(BaseModel):
    """
    This represents an ADDRESS to a location. It can be serialized. It can also be used to access
    information about the object it targets.
    """
    model_config = ConfigDict(validate_assignment=True, frozen=True)
    location_type: str = Field(..., description="The type of location, such as 'zone' or 'structure'")
    location_id: uuid.UUID = Field(..., description="The ID of the location object")
    point: Point = Field(default_factory=Point, description="The coordinates within the location")
    
    def __bool__(self):
        if not (e := self.entity):
            return False
        return e.valid_location_coordinates(self.point)

    @property
    def entity(self):
        return self.get_target()

    def __repr__(self):
        if not (e := self.entity):
            return "<Location: Nowhere>"
        if not e.valid_location_coordinates(self.point):
            return f"<Location: Invalid location on {e}>"
        return f"<Location: {e} at {self.point}>"

    def get_target(self):
        match self.location_type:
            case "zone":
                return dbat.INDEX.get_zone(self.location_id)
            case "structure":
                return dbat.INDEX.get_structure(self.location_id)
            case _:
                return None

    def on_map(self) -> bool:
        """
        Returns whether this location is on the map.
        This would be false if it's inside something's inventory, equipped, etc.
        """
        return self.location_type in ("zone", "structure")

    def render_look(self, viewer: "Character"):
        target = self.get_target()
        name = target.get_display_name(self.point, viewer)
        description = target.get_display_description(self.point, viewer)
        objects = list()
        characters = list()
        structures = list()
        from .characters import Character
        from .objects import Object
        from .structures import Structure
        
        for content in target.iter_contents_at(self.point):
            if not viewer.can_see(content):
                continue
            if content is viewer:
                continue
            match content:
                case Character():
                    characters.append(content)
                case Object():
                    objects.append(content)
                case Structure():
                    structures.append(content)
        
        exits = target.generate_exits_at(self.point)

        from ..events.views import LocationCharacterLine, LocationObjectLine, LocationStructureLine
        from ..events.informative import LocationDisplay

        rendered_map = RenderedMap.create_from_location(self, viewer)

        event = LocationDisplay(
            location=self,
            color_name=name,
            color_description=description,
            objects=[LocationObjectLine.from_object(obj, viewer) for obj in objects],
            characters=[LocationCharacterLine.from_character(char, viewer) for char in characters],
            structures=[LocationStructureLine.from_structure(struct, viewer) for struct in structures],
            exits=exits,
            rendered_map=rendered_map
        )
        viewer.send_event(event)

class HasLocation(BaseModel):
    model_config = ConfigDict(validate_assignment=True)
    location: Location | None = Field(default=None, description="The current location of this entity")
    saved_locations: dict[str, Location] = Field(default_factory=dict, description="A dict of saved locations by name")
    
    def add_to_location(self, location: Location):
        if self.location:
            raise ValueError(f"{self} is already in a location")
        self.location = location
        location.entity.add_content(self, location.point)
        self.on_add_to_location(location)
    
    def on_add_to_location(self, location: Location):
        pass
    
    def remove_from_location(self):
        if not self.location:
            raise ValueError(f"{self} is not in a location")
        loc = self.location
        self.location = None
        loc.entity.remove_content(self, loc.point)
    
    def on_remove_from_location(self, location: Location):
        pass
    
    def register_location(self):
        """
        This should be called after loading an object from disk, to register its location with the location's contents.
        """
        if self.location:
            self.location.entity.add_content(self, self.location.point, loading=True)