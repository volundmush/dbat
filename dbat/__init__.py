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

    def character_races(self) -> list:
        out = list()

        from .types.races import ALL_RACES

        out.extend(ALL_RACES)
        return out
    
    async def setup_character_races(self):
        for p in self.app.plugin_load_order:
            if not hasattr(p, "character_races"):
                continue
            for race in p.character_races():
                race_handler = race()
                INDEX.character_races[race_handler.key()] = race_handler

    def character_senseis(self) -> list:
        out = list()

        from .types.senseis import ALL_SENSEIS

        out.extend(ALL_SENSEIS)
        return out
    
    async def setup_character_senseis(self):
        for p in self.app.plugin_load_order:
            if not hasattr(p, "character_senseis"):
                continue
            for sensei in p.character_senseis():
                sensei_handler = sensei()
                INDEX.character_senseis[sensei_handler.key()] = sensei_handler

    def character_sexes(self) -> list:
        out = list()

        from .types.sexes import ALL_SEXES
        out.extend(ALL_SEXES)
        return out
    
    async def setup_character_sexes(self):
        for p in self.app.plugin_load_order:
            if not hasattr(p, "character_sexes"):
                continue
            for sex in p.character_sexes():
                sex_handler = sex()
                INDEX.character_sexes[sex_handler.key()] = sex_handler

    def character_bodyparts(self) -> list[type["BodyPartHandler"]]:
        out = list()

        from .types.bodyplans import ALL_BODY_PARTS
        
        out.extend(ALL_BODY_PARTS)

        return out

    async def setup_character_bodyparts(self):
        for p in self.app.plugin_load_order:
            if not hasattr(p, "character_bodyparts"):
                continue
            for part in p.character_bodyparts():
                handler = part()
                INDEX.body_parts[handler.key()] = handler

    def character_bodyplans(self) -> list[type["BodyPlanHandler"]]:
        out = list()

        from .types.bodyplans import ALL_BODY_PLANS
        
        out.extend(ALL_BODY_PLANS)

        return out

    async def setup_character_bodyplans(self):
        for p in self.app.plugin_load_order:
            if not hasattr(p, "character_bodyplans"):
                continue
            for plan in p.character_bodyplans():
                handler = plan()
                INDEX.body_plans[handler.key()] = handler

    def character_stats(self) -> list[type["StatDef"]]:
        out = list()

        from .types.stats import ALL_STATS
        
        out.extend(ALL_STATS)

        return out

    async def setup_character_stats(self):
        for p in self.app.plugin_load_order:
            if not hasattr(p, "character_stats"):
                continue
            for stat in p.character_stats():
                handler = stat()
                INDEX.character_stats[handler.key] = handler

    def character_hediffs(self) -> list[type["HeDiffHandler"]]:
        out = list()

        from .types.hediffs import ALL_HEDIFFS
        
        out.extend(ALL_HEDIFFS)

        return out

    async def setup_character_hediffs(self):
        for p in self.app.plugin_load_order:
            if not hasattr(p, "character_hediffs"):
                continue
            for hediff in p.character_hediffs():
                handler = hediff()
                INDEX.hediffs[handler.key()] = handler

    async def setup_final(self, app_name: str):
        match app_name:
            case "game":
                dbat = self.app.services["game"]
                self.app.fastapi_instance.state.dbat_game = dbat
                await self.setup_character_races()
                await self.setup_character_sexes()
                await self.setup_character_senseis()
                await self.setup_character_commands()
                await self.setup_character_bodyparts()
                await self.setup_character_bodyplans()
                await self.setup_character_stats()
                await self.setup_character_hediffs()

    def depends(self):
        return [("core", ">=0.0.1")]

    def game_migrations(self):
        return dict()
    
    def game_classes(self):
        # Replaces the Core session.
        from .sessions import Session

        return {"session": Session}