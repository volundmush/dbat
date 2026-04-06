import uuid
from pydantic import Field, ConfigDict, PrivateAttr
import dbat
from .grids import Grid, Point, Tile, Shape, Exit, Direction, ResetCommand, TileBase, ShapeBase, ExitBase
from .location import HasLocation, IsLocation, Location
from .misc import HasColorName, HasColorDescription, HasInteractive, HasFlags
from .inventory import HasInventory
import copy

class ExitPrototype(ExitBase):
    """
    ExitPrototypes are templates for spawning Exits.
    This is useful for things like player-owned structures, dungeon instances, etc.
    # Unlike real exits, prototypes only have coordinates.
    """
    location: Point = Field(default_factory=Point, description="The coordinates within the tile that this exit leads to.")

    def spawn(self, loc_type: str, grid_id: uuid.UUID) -> Exit:
        data = self.model_dump()
        data["location"] = Location(loc_type, grid_id, self.location)
        return Exit(**data)

class TilePrototype(TileBase):
    """
    TilePrototypes are templates for spawning Tiles.
    This is useful for things like player-owned structures, dungeon instances, etc.
    """
    exits: dict[Direction, ExitPrototype] = Field(default_factory=dict, description="The exits from this tile. The keys are the directions of the exits.")

    def spawn(self, loc_type: str, grid_id: uuid.UUID) -> Tile:
        data = self.model_dump(exclude={"exits"})
        data["grid"] = grid_id
        data["exits"] = {k: v.spawn(loc_type, grid_id) for k, v in self.exits.items()}
        return Tile(**data)


class ShapePrototype(ShapeBase):
    """
    ShapePrototypes are templates for spawning Shapes.
    This is useful for things like player-owned structures, dungeon instances, etc.
    """

    def spawn(self, grid_id: uuid.UUID) -> Shape:
        data = self.model_dump()
        data["grid"] = grid_id
        return Shape(**data)

class StructurePrototype(HasColorName, HasColorDescription, HasFlags):
    """
    StructurePrototypes are templates for spawning Structures.
    This is useful for things like player-owned structures, dungeon instances, etc.
    """
    shapes: dict[Point, ShapePrototype] = Field(default_factory=dict, description="The shapes that make up this structure. The keys are the coordinates of the shapes within the structure.")
    tiles: dict[Point, TilePrototype] = Field(default_factory=dict, description="The tiles that make up this structure. The keys are the coordinates of the tiles within the structure.")
    landing_spots: dict[str, Point] = Field(default_factory=dict, description="Named coordinates within the structure that can be used as landing spots for things like exits, entrances, etc.")     
    docking_spots: dict[str, Point] = Field(default_factory=dict, description="Named coordinates within the structure that can be used as docking spots for things like exits, entrances, etc.")
    default_point: Point = Field(default_factory=Point, description="The default point to place things within the structure, such as when spawning characters, objects, etc. It should be a valid point within the structure.")

    __instances: set[uuid.UUID] = PrivateAttr(default_factory=set)

    def spawn(self) -> Structure:
        data = self.model_dump(exclude={"shapes", "tiles", "landing_spots", "docking_spots"})
        id = uuid.uuid4()
        data["id"] = id
        data["shapes"] = {k: v.spawn("structure", id) for k, v in self.shapes.items()}
        data["tiles"] = {k: v.spawn("structure", id) for k, v in self.tiles.items()}
        structure = Structure(**data)
        structure.proto = self.id
        structure.game_activate()
        return structure

class Structure(Grid, HasLocation, IsLocation, HasInteractive, HasInventory):
    """
    Structures are a dynamic, possibly player-owned instance of a space. They are loaded
    after Zones. Unlike Zones, they have a Location.

    """
    __deleted: bool = PrivateAttr(default=False)
    proto: str = Field(default="", description="The prototype this structure was spawned from, if any.")

    def report_location_type(self):
        return "structure"

    def report_slug_type(self):
        return "structure"
    
    def __bool__(self):
        return not self.__deleted
    
    def __repr__(self):
        return f"<Structure: {self.color_name.plain} ({self.id}){f" {self.slug}" if self.slug else ""}>"
    
    def game_activate(self):
        dbat.INDEX.structures[self.id] = self
        if self.proto:
            proto = dbat.INDEX.structure_prototypes.get(self.proto)
            if proto:
                proto.instances.add(self)

    def game_deactivate(self):
        if self.proto:
            proto = dbat.INDEX.structure_prototypes.get(self.proto)
            if proto:
                proto.instances.discard(self)
        dbat.INDEX.structures.pop(self.id, None)

    def save(self):
        dbat.INDEX.dirty_structures.add(self.id)
