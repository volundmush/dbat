from .grids import Grid
from .location import IsLocation


class Zone(Grid, IsLocation):
    slug_type: str = "zone"

    __slots__ = ("id", "deleted")
    
    def __init__(self):
        self.id: str = ""
        self.deleted = False