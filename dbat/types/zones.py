import uuid

import dbat
from .grids import Grid
from .location import IsLocation, Location, Point
from pydantic import Field, ConfigDict, model_validator, PrivateAttr

class Zone(Grid, IsLocation):
    parent_id: uuid.UUID | None = Field(default=None, description="The ID of the parent zone, if any.")
    launch_destination: Location | None = Field(default=None, description="The location to launch into when entering this zone.")

    __children: set[uuid.UUID] = PrivateAttr(default_factory=set)
    __deleted: bool = PrivateAttr(default=False)

    @model_validator(mode="before")
    @classmethod
    def convert_dicts(cls, data: dict) -> dict:
        for field in ("shapes", "tiles"):
            data[field] = Point.deserialize_dict(data.get(field, dict()))
        return data

    def add_child(self, child: "Zone"):
        self.__children.add(child.id)
        child.parent_id = self.id

    def remove_child(self, child: "Zone"):
        self.__children.remove(child.id)
        child.parent_id = None

    def report_slug_type(self):
        return "zone"
    
    def report_location_type(self):
        return "zone"
    
    def __bool__(self):
        return not self.__deleted

    def __repr__(self):
        return f"<Zone: {self.color_name.plain} ({self.id}){f" {self.slug}" if self.slug else ""}>"
    
    def save(self):
        dbat.INDEX.dirty_zones.add(self.id)