import logging
import kai
import asyncio
import jwt
import time

from kai.portal.portal_session import ParserMixin

from .menus.main import MainMenu

class AdminParser(ParserMixin):
    
    def __init__(self, session, level):
        self.session = session
        self.task = None
        self.parser_stack = list()
        self.parser_stack.append(MainMenu(self))
        self.level = level
    
    async def run(self):
        await self.session.send_text("You have entered admin mode!")
        await self.parser_stack[-1].render()
        self.task = asyncio.create_task(self.run_admin())
        await self.task
        
    async def run_admin(self):
        await asyncio.gather(*[self.run_messaging()])
    
    
    async def run_messaging(self):
        while msg := await self.session.outgoing_queue.get():
            event = msg[0]
            data = msg[1]
            match event:
                case "Command":
                    line = data.get("data", "")
                    await self.handle_command(line)
                case _:
                    pass
    
    async def handle_command(self, line: str):
        if not self.parser_stack:
            return
        await self.parser_stack[-1].parse(line)
    
    async def close(self):
        pass