from kai.portal.portal_session import ParserMixin
from rich.table import Table
from rich.style import Style
from rich.console import Group
from rich.text import Text


class AdminMenu(ParserMixin):
    commands = list()
    
    def name(self):
        return self.__class__.__name__
    
    def path(self):
        return "/".join([x.name() for x in self.session.parser_stack])
    
    def __init__(self, parser, parent=None):
        self.parser = parser
        self.session = parser.session
        self.parent = parent
    
    async def render(self):
        g = Group(*[out for x in (self.render_main, self.render_commands) if (out := await x())])
        await self.session.send_rich(g)
    
    async def render_title(self):
        return Text(self.path(), justify="center", style="bold")
    
    async def render_main(self):
        pass
    
    async def render_commands(self):
        table = await self.session.rich_table("Name", "Syntax", "Description", title="Commands")
        for c in self.commands:
            table.add_row(c.name, "N/A", "N/A")
        return table
    
    async def parse(self, command: str):
        await self.execute_command(command)
    
    async def execute_command(self, text: str):
        if not (text := text.strip()):
            return
        orig_text = text
        if " " in text:
            text, args = text.split(" ", 1)
        else:
            args = ""
        async for cmd in self.available_commands():
            if res := await cmd.match(self.session, text):
                command = cmd(self.session, orig_text, res, args)
                await command.run()

    async def available_commands(self):
        for cmd in await self.sorted_commands():
            if await cmd.available(self.session):
                yield cmd

    async def sorted_commands(self) -> list["Command"]:
        out = await self.all_commands()
        return sorted(out, key=lambda c: c.priority, reverse=True)

    async def get_basic_commands(self) -> list["Command"]:
        return self.commands

    async def all_commands(self) -> list["Command"]:
        out = await self.get_basic_commands()

        return list(set(out))