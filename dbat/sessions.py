import typing
import dbat
from muplugins.core.sessions import Session as _BaseSession

from muplugins.core.events.messages import GMCPEvent, TextEvent,  RichTextEvent
from rich.text import Text

if typing.TYPE_CHECKING:
    from .types.accounts import Account
    from .types.characters import Character, PlayerCharacter

class Session(_BaseSession):

    def __init__(self, core, acting):
        super().__init__(core, acting)
        self.account: Account | None = None
        self.original: PlayerCharacter | None = None
        self.puppet: Character | None = None

    def repl_globals(self, data):
        super().repl_globals(data)
        data["self"] = self.original
        data["original"] = self.original
        data["puppet"] = self.puppet
        data["dbat"] = dbat
        target = self.current_target()
        if target:
            data["here"] = target.location

    def append_text(self, text: str):
        self.send_event_nowait(TextEvent(text=text))

    def append_rich(self, text: str):
        self.send_event_nowait(RichTextEvent(text=text.markup if isinstance(text, Text) else text))

    def append_line(self, text: str):
        if not text.endswith("\n"):
            text += "\n"
        self.append_text(text)

    def append_gmcp(self, package: str, data: dict):
        self.send_event_nowait(GMCPEvent(package, data))
    
    def append_event(self, event):
        self.send_event_nowait(event)

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
            target.enqueue_command(command)
    
    async def on_start(self):
        if not (acc := dbat.ACCOUNTS.get(self.user.id, None)):
            # we have to load the account into the system from postgres.
            from .types.accounts import Account
            acc = Account()
            acc.id = self.user.id
            acc.email = self.user.email
            acc.username = self.user.username
            acc.admin_level = self.user.admin_level
            
            # now we retrieve the 'dbat' component from user_components...
            async with self.core.db.connection() as conn:
                row = await conn.fetchrow(
                    "SELECT data FROM user_components WHERE user_id = $1 AND component_name = $2", acc.id, "dbat"
                )
                if row:
                    data = row["data"]
                    acc.rpp = data.get("rpp", 0)
                    acc.rpp_bank = data.get("rpp_bank", 0)

            dbat.ACCOUNTS[acc.id] = acc
        
        self.account = acc

        if not (char := dbat.PLAYERS.get(self.pc.id, None)):
            # we have to load the character into the system from postgres.
            from .types.characters import PlayerCharacter
            char = PlayerCharacter()
            char.id = self.pc.id
            char.set_color_name(self.pc.name)
            char.account = acc
            self.original = char
            char.session = self
            char.game_activate()
        else:
            char.session = self
        
        
        
        