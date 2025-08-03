import dbat
from ..base import BaseParser
from dbat.portal.commands.base import CMD_MATCH
from httpx import HTTPStatusError
from dbat.utils import partial_match

from dbat.models.game import AccountData, PlayerData


class AdminParser(BaseParser):
    """
    Implements the character selection and user management features.
    """

    async def on_start(self):
        await self.handle_look()
    
    async def handle_look(self):
        user = await self.get_user()
        await self.handle_help("", user)
    
    async def handle_help(self, args: str, user = None):
        if user is None:
            user = await self.get_user()
        help_table = self.make_table("Command", "Description", title="Admin Commands")
        help_table.add_row("help", "Displays this help message.")
        help_table.add_row("scripts", "Enter Script Management.")
        help_table.add_row("exit", "Exits back to the user menu.")
        await self.send_rich(help_table)
    
    async def handle_scripts(self, args: str):
        parser_class = dbat.CLASSES["script_parser"]
        parser = parser_class()
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
            case "scripts":
                await self.handle_scripts(args)
            case "exit":
                await self.handle_exit()
            case "look":
                await self.handle_look()
            case _:
                await self.send_line("Invalid command. Type 'help' for help.")
