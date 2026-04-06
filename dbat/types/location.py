import dbat
import uuid
import typing
from pydantic import BaseModel, ConfigDict, Field, PrivateAttr
from rich.text import Text

from ..events.informative import LookLocation

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

class Location(BaseModel):
    """
    This represents an ADDRESS to a location. It can be serialized. It can also be used to access
    information about the object it targets.
    """
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
        from .characters import Character
        from .objects import Object
        
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
        
        exits = list(target.generate_exits_at(self.point).keys())

        event = LookLocation(
            location=(self.location_type, self.location_id, self.point),
            name=name.markup if isinstance(name, Text) else name,
            description=description.markup if isinstance(description, Text) else description,
            objects=[(obj.id, obj.get_display_name(viewer)) for obj in objects],
            characters=[(char.id, char.get_display_name(viewer)) for char in characters],
            exits=exits
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