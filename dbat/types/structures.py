from .grids import Grid
from .location import HasLocation, IsLocation
from .misc import HasFlags, HasInteractive
from .inventory import HasInventory

class Structure(Grid, HasLocation, IsLocation, HasInteractive, HasFlags, HasInventory):
    """
    Structures are a dynamic, possibly player-owned instance of a space. They are loaded
    after Zones. Unlike Zones, they have a Location.

    """
    slug_type: str = "structure"
    location_type: str = "structure"

    def __init__(self):
        Grid.__init__(self)
        HasLocation.__init__(self)
        IsLocation.__init__(self)
        HasInteractive.__init__(self)
        HasFlags.__init__(self)
        HasInventory.__init__(self)
        self.id: str = ""
        self.deleted = False
    
    def __bool__(self):
        return not self.deleted