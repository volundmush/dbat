import dbat
POINT = tuple[int, int, int]

class IsLocation:
    location_type: str = "abstract"

    def __init__(self):
        self.__contents: list["HasLocation"] = list()

    def iter_contents(self):
        for c in self.__contents.copy():
            if c:
                yield c
    
    def iter_contents_at(self, point: POINT):
        for c in self.__contents.copy():
            if c and c.location and c.location.point == point:
                yield c

    def valid_location_coordinates(self, point: POINT) -> bool:
        """
        Returns whether the given coordinates are valid for this object.
        For example, a room might only have one valid coordinate (0,0,0), while a zone might have many.
        """
        return False

    def add_content(self, con: "HasLocation", loc: POINT):
        """
        Adds an entity to this location. This should only be called by the HasLocation.add_to_location method.
        """
        self.__contents.append(con)
        self.on_contents_add(con, loc)

    def on_contents_add(self, con: "HasLocation", new_loc: POINT):
        """
        This is called when an entity is added to this location. It can be used to trigger events, etc.
        """

    def relocate_content(self, con: "HasLocation", old_loc: POINT, new_loc: POINT):
        self.on_contents_relocate(con, old_loc, new_loc)

    def on_contents_relocate(self, con: "HasLocation", old_loc: POINT, new_loc: POINT):
        """
        This is called when an entity was already IN this location, but is changing coordinates.
        """

    def remove_content(self, con: "HasLocation", old_loc: POINT):
        self.__contents.remove(con)
        self.on_contents_remove(con, old_loc)

    def on_contents_remove(self, con: "HasLocation", old_loc: POINT):
        """
        This is called when an entity was in this location, and is now removed from it.
        """
    
    def make_location(self, point: POINT) -> Location:
        return Location(self, point)

class Location:
    """
    This represents an ADDRESS to a location. It can be serialized. It can also be used to access
    information about the object it targets.
    """

    def __init__(self, entity_type: str, entity_id: str, point: POINT = None):
        if not point:
            point = (0,0,0)
        
        self.entity_type = entity_type
        self.entity_id = entity_id
        self.point: POINT = point
    
    def dump(self) -> dict:
        if not self:
            return {}
        
        return {
            "entity_type": self.entity_type,
            "entity_id": self.entity_id,
            "point": list(self.point)
        }
    
    @classmethod
    def load(cls, data: dict) -> Location:
        return Location(**data)
    
    def __bool__(self):
        if not self.entity:
            return False
        return self.entity.valid_location_coordinates(self.point)

    def __repr__(self):
        return f"<Location {self.entity} at {self.point}>"

    @property
    def entity(self):
        return self.get_target()

    def __repr__(self):
        if not self:
            return "<Location: Nowhere>"
        return f"<Location: {self.entity} at {self.point}>"

    def get_target(self):
        match self.entity_type:
            case "zone":
                return dbat.ZONES.get(self.entity_id)
            case "structure":
                return dbat.STRUCTURES.get(self.entity_id)
            case "character":
                return dbat.CHARACTERS.get(self.entity_id)
            case "object":
                return dbat.OBJECTS.get(self.entity_id)
            case _:
                return None

    def on_map(self) -> bool:
        """
        Returns whether this location is on the map.
        This would be false if it's inside something's inventory, equipped, etc.
        """
        return self.target_type in ("zone", "zone")


class HasLocation:

    def __init__(self, location: Location = None):
        self.__location: Location | None = location

    @property
    def location(self) -> Location | None:
        if self.__location is not None and not self.__location:
            self.__location = None
            return None
        return self.__location
    
    @location.setter
    def location(self, value: Location | None):
        if value is not None and not isinstance(value, Location):
            raise ValueError("Location must be a Location object or None")
        self.__location = value
    
    def add_to_location(self, location: Location):
        if self.location:
            raise ValueError(f"{self} is already in a location")
        self.location = location
        location.entity.add_content(self, location.point)
    
    def remove_from_location(self):
        if not self.location:
            raise ValueError(f"{self} is not in a location")
        loc = self.location
        self.location = None
        loc.entity.remove_content(self, loc.point)