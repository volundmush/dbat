import uuid
from muplugins.core.events.base import EventBase
from muplugins.core.db.fields import RichText
from pydantic import Field, ConfigDict

from ..types.grids import Exit, Direction
from ..types.location import Location, Point, TileDisplay, RenderedMap
from ..types.misc import HasColorName, HasColorDescription, HasExtraDescriptions, HasFlags

from .views import InventoryLine, LocationCharacterLine, LocationObjectLine, LocationStructureLine

from rich.console import Group
from rich.table import Table
from rich.text import Text

class InventoryDisplay(EventBase):
    """
    This is emitted when a character views their inventory.
    """
    lines: list[InventoryLine] = Field(default_factory=list, description="A list of inventory lines to display.")
    alerts: list[RichText] = Field(default_factory=list, description="A list of alert messages to display at the top of the inventory.")
    
    async def handle_event(self, conn):
        if not self.lines:
            await conn.send_line("Your inventory is empty.")
            return
        await conn.send_line("You are carrying:")
        for line in self.lines:
            await conn.send_rich(line.color_name)
        for alert in self.alerts:
            await conn.send_rich(alert)


class LocationDisplay(EventBase, HasColorName, HasColorDescription, HasFlags, HasExtraDescriptions):
    """
    This is emitted when a character views a Grid location.
    """
    location: Location = Field(..., description="The location being viewed.")
    exits: dict[Direction, Exit] = Field(default_factory=list, description="A list of exits from the location, as strings")
    characters: list[LocationCharacterLine] = Field(default_factory=list, description="A list of characters at the location.")
    objects: list[LocationObjectLine] = Field(default_factory=list, description="A list of objects at the location.")
    structures: list[LocationStructureLine] = Field(default_factory=list, description="A list of structures at the location.")
    rendered_map: RenderedMap | None = Field(default=None, description="The rendered map for the location.")

    async def handle_event(self, conn):
        table = conn.make_table(title=self.color_name)

        map_display = Text()
        if self.rendered_map:
            for y in range(self.rendered_map.height):
                line = Text()
                for x in range(self.rendered_map.width): 
                    if tile := self.rendered_map.tiles[y][x]:
                        line.append(tile.display())
                    else:
                        line.append(" ")
                map_display.append(line)
                map_display.append("\r\n")

        if self.color_description:
            table.add_column("Description")
            table.add_column("Map", min_width=10)
            table.add_row(self.color_description, map_display)
        else:
            table.add_column("Map", min_width=10)
            table.add_row(map_display)
        
        elements = [table]
        for obj in self.objects:
            elements.append(obj.color_name)
        for char in self.characters:
            elements.append(char.color_name)
        for struct in self.structures:
            elements.append(struct.color_name)
        if self.exits:
            exits_text = Text("Exits: " + ", ".join([str(x) for x in self.exits.keys()]))
            elements.append(exits_text)

        await conn.send_rich(Group(*elements))
    
    @classmethod
    def event_type(cls) -> str:
        return "location.display"