import dbat
import importlib
import orjson
import asyncio
from loguru import logger
from lark import Lark
from pathlib import Path
from fastapi import FastAPI
from hypercorn import Config
from hypercorn.asyncio import serve
from dbat import Application as OldApplication
from dbat.utils import callables_from_module, class_from_module, EventHub
import dbat_ext

def decode_json(data: bytes):
    decoded = orjson.loads(data)
    return decoded

class Application(OldApplication):
    name = "game"

    def __init__(self):
        super().__init__()
        self.fastapi_config = None
        self.fastapi_instance = None

    async def setup_fastapi(self):
        settings = dbat.SETTINGS
        shared = settings["SHARED"]
        tls = settings["TLS"]
        networking = settings["GAME"]["networking"]
        self.fastapi_config = Config()
        self.fastapi_config.title = settings["MSSP"]["NAME"]

        external = shared["external"]
        bind_to = f"{external}:{networking['port']}"
        self.fastapi_config.bind = [bind_to]

        if Path(tls["certificate"]).exists():
            self.fastapi_config.certfile = str(Path(tls["certificate"]).absolute())
        if Path(tls["key"]).exists():
            self.fastapi_config.keyfile = str(Path(tls["key"]).absolute())

        self.fastapi_instance = FastAPI()
        routers = settings["FASTAPI"]["routers"]
        for k, v in routers.items():
            if not v:
                continue
            v = importlib.import_module(v)
            self.fastapi_instance.include_router(v.router, prefix=f"/{k}", tags=[k])

    async def setup_lark(self):
        absolute_phantasm = Path(dbat.__file__).parent
        grammar = absolute_phantasm / "grammar.lark"
        with open(grammar, "r") as f:
            data = f.read()
            parser = Lark(data)
            dbat.LOCKPARSER = parser

    async def setup_dbat(self):
        dbat_ext.load_db()

    async def setup(self):
        await super().setup()
        dbat.EVENT_HUB = EventHub()
        await self.setup_lark()
        await self.setup_fastapi()

        for k, v in dbat.SETTINGS["GAME"].get("lockfuncs", dict()).items():
            lock_funcs = callables_from_module(v)
            for name, func in lock_funcs.items():
                dbat.LOCKFUNCS[name] = func
        
        await self.setup_dbat()

    async def system_pinger(self):
        from dbat.events.system import SystemPing

        try:
            while True:
                await dbat.EVENT_HUB.broadcast(SystemPing())
                await asyncio.sleep(15)
        except asyncio.CancelledError:
            return

    async def start(self):
        self.task_group.create_task(serve(self.fastapi_instance, self.fastapi_config))
        self.task_group.create_task(self.system_pinger())
        self.task_group.create_task(dbat_ext.run_game(0.05, 60.0 * 5.0))
