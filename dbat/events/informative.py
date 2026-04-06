import uuid
from muplugins.core.events.base import EventBase
from muplugins.core.db.fields import RichText
from pydantic import Field, ConfigDict

from ..types.grids import Exit, Direction
from ..types.location import Location, Point
from ..types.misc import HasColorName, HasColorDescription, HasExtraDescriptions, HasFlags

from .views import InventoryLine, LocationCharacterLine, LocationObjectLine, LocationStructureLine

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

    async def handle_event(self, conn):
        await conn.send_rich("[red]--------------[/red]")
        await conn.send_rich(self.name)
        if self.description:
            await conn.send_rich("[red]--------------[/red]")
            await conn.send_rich(self.description)
        if self.objects:
            await conn.send_rich("[bold]Objects:[/bold]")
            for obj in self.objects:
                await conn.send_rich(f"  {obj[1]}")
        if self.characters:
            await conn.send_rich("[bold]Characters:[/bold]")
            for char in self.characters:
                await conn.send_rich(f"  {char[1]}")
        if self.exits:
            await conn.send_rich("[bold]Exits:[/bold]")
            await conn.send_line("  " + ", ".join(self.exits))
    
    @classmethod
    def event_type(cls) -> str:
        return "location.display"