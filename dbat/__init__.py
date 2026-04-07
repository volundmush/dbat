from muforge.plugin import BasePlugin
from collections import defaultdict

from .utils.indexer import Indexer

INDEX = Indexer()

SUBSCRIPTIONS = defaultdict(set)

CHARACTER_COMMANDS: dict[str, type["CharacterCommand"]] = dict()
CHARACTER_COMMANDS_PRIORITY: dict[int, list[type["CharacterCommand"]]] = defaultdict(list)


class DBAT(BasePlugin):

    def __init__(self, app):
        super().__init__(app)
        self.registered_character_commands = dict()
        self.character_commands_priority = defaultdict(list)

    def name(self):
        return "Dragon Ball: Advent Truth"

    def slug(self):
        return "dbat"

    def version(self):
        return "0.0.1"
    
    def game_services(self):
        from .game import GameService

        return {"game": GameService}

    def core_events(self):
        from .events.informative import LocationDisplay

        all_events = [LocationDisplay]

        return {ev.event_type(): ev for ev in all_events}
    
    def character_commands(self) -> list[type["CharacterCommand"]]:
        out = list()
        from .character_commands.informative import Look

        out.append(Look)

        from .character_commands.movement import Move
        out.append(Move)

        return out

    async def setup_character_commands(self):
        for p in self.app.plugin_load_order:
            if not hasattr(p, "character_commands"):
                continue
            for command in p.character_commands():
                self.registered_character_commands[command.key] = command
        
        # sort by priority
        for command in self.registered_character_commands.values():
            self.character_commands_priority[command.priority].append(command)
        for v in self.character_commands_priority.values():
            v.sort(key=lambda c: c.key)
        
        global CHARACTER_COMMANDS, CHARACTER_COMMANDS_PRIORITY
        CHARACTER_COMMANDS = self.registered_character_commands
        CHARACTER_COMMANDS_PRIORITY = self.character_commands_priority

    async def setup_final(self, app_name: str):
        match app_name:
            case "game":
                dbat = self.app.services["game"]
                self.app.fastapi_instance.state.dbat_game = dbat
                await self.setup_character_commands()

    def depends(self):
        return [("core", ">=0.0.1")]

    def game_migrations(self):
        return dict()
    
    def game_classes(self):
        # Replaces the Core session.
        from .sessions import Session

        return {"session": Session}