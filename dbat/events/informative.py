import uuid
from muplugins.core.events.base import EventBase
from muplugins.core.db.fields import rich_text
from pydantic import Field

class LookLocation(EventBase):
    """
    This is emitted when a character views a Grid location.
    """
    location: tuple[str, uuid.UUID, tuple[int, int, int]] = Field(..., description="A tuple of (location_type, location_id, coordinates)")
    name: rich_text = Field(..., description="The name of the location being looked at")
    description: rich_text = Field(..., description="The description of the location being looked at")
    objects: list[tuple[uuid.UUID, rich_text]] = Field(default_factory=list, description="A list of objects in the location, as tuples of (object_id, object_name)")
    characters: list[tuple[uuid.UUID, rich_text]] = Field(default_factory=list, description="A list of characters in the location, as tuples of (character_id, character_name)")
    exits: list[str] = Field(default_factory=list, description="A list of exits from the location, as strings")

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
        return "look.location"