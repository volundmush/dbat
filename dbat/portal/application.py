import asyncio
import aiodns
import sys
import traceback

import dbat
from dbat import Application as _Application
from dbat.utils import callables_from_module
from loguru import logger
from rich.ansi import AnsiDecoder
from rich.text import Text

import dbat_ext


def process_circle_color(text: str, parse: bool = True) -> Text:
    as_ansi = dbat_ext.process_colors(text, parse)
    return Text("\n").join(AnsiDecoder().decode(as_ansi))

async def send_circle(self, msg: str):
    t = process_circle_color(msg)
    await self.send_rich(t)

class Application(_Application):
    name = "portal"

    def __init__(self):
        super().__init__()
        self.game_sessions = dict()
        self.resolver = None

        loop = asyncio.get_event_loop()
        if sys.platform != "win32":
            self.resolver = aiodns.DNSResolver(loop=loop)

    async def setup(self):
        from dbat.portal.base_connection import BaseConnection
        BaseConnection.send_circle = send_circle
        
        await super().setup()

        for k, v in dbat.SETTINGS["PORTAL"]["commands"].items():
            for name, command in callables_from_module(v).items():
                dbat.COMMANDS[command.name] = command
                dbat.COMMANDS_PRIORITY[command.priority].append(command)

    async def handle_new_protocol(self, protocol):
        protocol.core = self
        try:
            self.game_sessions[protocol.session_name] = protocol
            await protocol.run()
        except Exception as err:
            logger.error(traceback.format_exc())
            logger.error(err)
        finally:
            del self.game_sessions[protocol.session_name]
