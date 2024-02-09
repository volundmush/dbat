import asyncio
from dataclasses import dataclass, field
from collections import defaultdict
from rich.color import ColorType, ColorSystem
from typing import Optional
import logging
import enum

import re
import traceback
from rich.console import Console
from rich.table import Table
from rich.box import ASCII2


import kai


from kai.utils.utils import lazy_property
from kai.utils.optionhandler import OptionHandler


@dataclass
class Capabilities:
    protocol: str = "telnet"
    session_name: str = ""
    encryption: bool = False
    client_name: str = "UNKNOWN"
    client_version: str = "UNKNOWN"
    host_address: str = "UNKNOWN"
    host_port: int = -1
    host_names: list[str] = None
    encoding: str = "ascii"
    color: ColorType = ColorType.DEFAULT
    width: int = 78
    height: int = 24
    mccp2: bool = False
    mccp2_enabled: bool = False
    mccp3: bool = False
    mccp3_enabled: bool = False
    gmcp: bool = False
    msdp: bool = False
    mssp: bool = False
    mslp: bool = False
    mtts: bool = False
    naws: bool = False
    sga: bool = False
    linemode: bool = False
    force_endline: bool = False
    screen_reader: bool = False
    mouse_tracking: bool = False
    vt100: bool = False
    osc_color_palette: bool = False
    proxy: bool = False
    mnes: bool = False



class PortalSession:
    def __init__(self):
        self.capabilities = Capabilities()
        self.task_group = asyncio.TaskGroup()
        self.tasks: dict[str, asyncio.Task] = {}
        self.running = True
        # This contains arbitrary data sent by the server which will be sent on a reconnect.
        self.userdata = None
        self.outgoing_queue = asyncio.Queue()
        self.core = None
        self.linked = False
        self.jwt = None
        self.jwt_claims = dict()
        self.parser = None
        self.parser_queue = asyncio.Queue()
        self.parser = None
        
    @property
    def http(self):
        return self.core.http

    @lazy_property
    def console(self):
        return Console(
            color_system=self.rich_color_system(),
            width=self.capabilities.width,
            file=self,
            record=True,
        )

    def rich_color_system(self):
        match self.capabilities.color:
            case ColorType.STANDARD:
                return "standard"
            case ColorType.EIGHT_BIT:
                return "256"
            case ColorType.TRUECOLOR:
                return "truecolor"
        return None

    def write(self, b: str):
        """
        When self.console.print() is called, it writes output to here.
        Not necessarily useful, but it ensures console print doesn't end up sent out stdout or etc.
        """

    def flush(self):
        """
        Do not remove this method. It's needed to trick Console into treating this object
        as a file.
        """

    def print(self, *args, **kwargs) -> str:
        """
        A thin wrapper around Rich.Console's print. Returns the exported data.
        """
        new_kwargs = {"highlight": False}
        new_kwargs.update(kwargs)
        self.console.print(*args, **new_kwargs)
        return self.console.export_text(clear=True, styles=True)

    @lazy_property
    def options(self):
        return OptionHandler(
            self,
            options_dict=kai.SETTINGS.OPTIONS_ACCOUNT_DEFAULT,
        )

    async def uses_screenreader(self) -> bool:
        return await self.options.get("screenreader")

    async def rich_table(self, *args, **kwargs) -> Table:
        options = self.options
        real_kwargs = {
            "box": ASCII2,
            "border_style": await options.get("border_style"),
            "header_style": await options.get("header_style"),
            "title_style": await options.get("header_style"),
            "expand": True,
        }
        real_kwargs.update(kwargs)
        if await self.uses_screenreader():
            real_kwargs["box"] = None
        return Table(*args, **real_kwargs)

    async def run(self):
        await asyncio.gather(*[self.run_protocol(), self.run_parser()])
    
    async def run_protocol(self):
        pass
    
    async def set_parser(self, parser):
        await self.parser_queue.put(parser)
        if self.parser:
            await self.parser.close()
    
    async def run_parser(self):
        while(msg := await self.parser_queue.get()):
            self.parser = msg
            await self.parser.run()
        
    async def send_text(self, text: str, force_endline=True):
        if not text:
            return
        text = text.replace("\r", "")
        text = text.replace("\n", "\r\n")
        if force_endline and not text.endswith("\r\n"):
            text += "\r\n"
        await self.handle_send_text(text)

    async def handle_send_text(self, text: str):
        pass

    async def send_gmcp(self, command: str, data=None):
        pass

    async def send_mssp(self, data: dict[str, str]):
        pass

    async def change_capabilities(self, changed: dict[str, "Any"]):
        for k, v in changed.items():
            self.capabilities.__dict__[k] = v
