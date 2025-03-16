import dbat_ext
from rich.ansi import AnsiDecoder
from rich.text import Text
from mudforge.portal.application import Application as _OldApp


def process_circle_color(text: str, parse: bool = True) -> Text:
    as_ansi = dbat_ext.process_colors(text, parse)
    return Text("\n").join(AnsiDecoder().decode(as_ansi))

async def send_circle(self, msg: str):
    t = process_circle_color(msg)
    await self.send_rich(t)

class Application(_OldApp):
    
    async def setup(self):
        from mudforge.portal.base_connection import BaseConnection
        BaseConnection.send_circle = send_circle
        await super().setup()