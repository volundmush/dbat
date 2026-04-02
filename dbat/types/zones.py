from .grids import Grid
from .location import IsLocation, Location


class Zone(Grid, IsLocation):
    slug_type: str = "zone"

    def __init__(self):
        Grid.__init__(self)
        IsLocation.__init__(self)
        self.id: str = ""
        self.deleted = False
        self.parent: "Zone | None" = None
        self.children: set["Zone"] = set()
        self.launch_destination: Location | None = None
    
    def __bool__(self):
        return not self.deleted