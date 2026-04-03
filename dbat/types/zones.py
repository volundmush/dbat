import dbat
from .grids import Grid
from .location import IsLocation, Location


class Zone(Grid, IsLocation):
    slug_type: str = "zone"

    def __init__(self):
        Grid.__init__(self)
        IsLocation.__init__(self)
        self.deleted = False
        self.parent: "Zone | None" = None
        self.children: set["Zone"] = set()
        self.launch_destination: Location | None = None
    
    def __bool__(self):
        return not self.deleted
    
    def save(self):
        dbat.DIRTY_ZONES.add(self.id)
    
    def dump(self) -> dict:
        out = super().dump()
        if self.launch_destination:
            out["launch_destination"] = self.launch_destination.dump()
        if self.parent:
            out["parent"] = self.parent.id
        return out

    @classmethod
    def load(cls, data: dict) -> "Zone":
        zone = cls()
        Grid.load_grid(zone, data)
        #zone.launch_destination = Location.load(data["launch_destination"]) if data.get("launch_destination")
        return zone