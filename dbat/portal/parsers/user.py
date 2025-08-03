import dbat
from .base import BaseParser, StringEditorParser
from dbat.portal.commands.base import CMD_MATCH
from httpx import HTTPStatusError
from dbat.utils import partial_match

from dbat.models.game import AccountData, PlayerData


class UserParser(BaseParser):
    """
    Implements the character selection and user management features.
    """

    async def on_start(self):
        await self.handle_look()
    
    async def handle_help(self, args: str):
        user = await self.get_user()
        help_table = self.make_table("Command", "Description", title="User Commands")
        help_table.add_row("help", "Displays this help message.")
        help_table.add_row("create", "Enter character creation.")
        help_table.add_row("play <name>", "Selects a character to play.")
        help_table.add_row("terminate <username>=YES", "Deletes your user account. Beware!")
        help_table.add_row("delete <name>=YES", "Deletes a character.")
        help_table.add_row("logout", "Logs out of the game.")
        help_table.add_row("look", "Lists all characters.")
        if user.admin_level > 0:
            help_table.add_row("admin", "Enter admin menu for user management.")
        await self.send_rich(help_table)

    async def handle_admin(self, args: str):
        user = await self.get_user()
        parser_class = dbat.CLASSES["admin_parser"]
        parser = parser_class()
        await self.connection.push_parser(parser)

    async def handle_create(self, args: str):
        parser_class = dbat.CLASSES["create_parser"]
        parser = parser_class()
        await self.connection.push_parser(parser)

    async def handle_play(self, args: str):
        if not args:
            await self.send_line("You must supply a name for your character.")
            return
        user = await self.get_user()
        character_data = await self.api_call("GET", f"/users/{user.id}/characters")
        characters = [PlayerData(**c) for c in character_data]

        if not (character := partial_match(args, characters, key=lambda c: c.name)):
            await self.send_line("Character not found.")
            return

        parser_class = dbat.CLASSES["character_parser"]

        parser = parser_class(user, character)
        await self.connection.push_parser(parser)

    async def handle_delete(self, args: str):
        pass

    async def handle_logout(self):
        self.connection.jwt = None
        self.connection.payload = None
        self.connection.refresh_token = None
        await self.connection.pop_parser()

    async def handle_terminate(self, lsargs, rsargs):
        pass

    async def handle_look(self):
        user = await self.get_user()
        character_data = await self.api_call("GET", f"/users/{user.id}/characters")

        characters = [PlayerData(**c) for c in character_data]

        character_table = self.make_table("Name", title="Characters")
        for character in characters:
            character_table.add_row(character.name)
        await self.send_rich(character_table)
        await self.handle_help("")

    async def handle_test(self):
        # this will test the string editor parser.
        
        async def on_save(text: str):
            await self.send_line(f"StringEditor saved with text: {text}")
        
        async def on_abort():
            await self.send_line("StringEditor was aborted.")
        
        parser = StringEditorParser("Test String", "This is a test string editor.", on_save, on_abort)
        await self.connection.push_parser(parser)

    async def handle_command(self, event: str):
        matched = CMD_MATCH.match(event)
        if not matched:
            await self.send_line("Invalid command. Type 'help' for help.")
            return
        match_dict = {k: v for k, v in matched.groupdict().items() if v is not None}
        cmd = match_dict.get("cmd", "")
        args = match_dict.get("args", "")
        lsargs = match_dict.get("lsargs", "")
        rsargs = match_dict.get("rsargs", "")
        match cmd.lower():
            case "help":
                await self.handle_help(args)
            case "create":
                await self.handle_create(args)
            case "play":
                await self.handle_play(args)
            case "delete":
                await self.handle_play(args)
            case "terminate":
                await self.handle_terminate(lsargs, rsargs)
            case "logout":
                await self.handle_logout()
            case "look":
                await self.handle_look()
            case "test":
                await self.handle_test()
            case _:
                await self.send_line("Invalid command. Type 'help' for help.")
