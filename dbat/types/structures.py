from .grids import Grid
from .location import HasLocation, IsLocation


class Structure(Grid, HasLocation, IsLocation):
    """
    Structures are a dynamic, possibly player-owned instance of a space. They are loaded
    after Zones. Unlike Zones, they have a Location.

    """
    slug_type: str = "structure"
    location_type: str = "structure"

    __slots__ = ("deleted", "id")

    def __init__(self):
        Grid.__init__(self)
        HasLocation.__init__(self)
        self.id: str = ""
        self.deleted = False
    
    def __bool__(self):
        return not self.deleted