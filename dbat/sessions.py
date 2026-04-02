import typing
import dbat
from muplugins.core.sessions import Session as _BaseSession
from rich.text import Text

from muplugins.core.events.messages import RichTextEvent, GMCPEvent, TextEvent

if typing.TYPE_CHECKING:
    from dbat.types.characters import Character, PlayerCharacter

class Session(_BaseSession):

    def __init__(self, core, acting):
        super().__init__(core, acting)
        self.original: PlayerCharacter | None = None
        self.puppet: Character | None = None

    def repl_globals(self, data):
        super().repl_globals(data)
        data["original"] = self.original
        data["puppet"] = self.puppet
        target = self.current_target()
        if target:
            data["here"] = target.location
    
    def send_rich(self, text: Text):
        self.send_event_nowait(RichTextEvent(text))

    def append_text(self, text: str):
        self.send_event_nowait(TextEvent(text))

    def append_line(self, text: str):
        if not text.endswith("\n"):
            text += "\n"
        self.append_text(text)

    def append_gmcp(self, package: str, data: dict):
        self.send_event_nowait(GMCPEvent(package, data))

    def current_target(self) -> Character | None:
        if self.puppet:
            return self.puppet
        if self.original:
            return self.original
        return None

    async def command_passthrough(self, command: str):
        target = self.current_target()
        if command == "--":
            target.clear_command_queue()
            await self.send
        else:
            target.command_queue.append(command)