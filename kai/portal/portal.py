#!/usr/bin/env python3
import asyncio
import os
import pickle
import logging
import sys
import traceback
import aiodns
from logging.handlers import TimedRotatingFileHandler

import kai
from kai.utils.utils import import_from_module, callables_from_module


# Install Rich as the traceback handler.
from rich.traceback import install as install_tb

install_tb(show_locals=True)


class Service:
    def __init__(self, core):
        self.core = core

    async def at_pre_start(self):
        pass

    async def start(self):
        pass


class Core:
    app = "portal"

    def __init__(self, settings):
        self.settings = settings
        kai.GAME = self
        self._log_handler = None
        self.services = dict()
        self.copyover_data = None
        self.cold_start = True
        self.ep = None
        self.game_sessions: dict[str, "GameSession"] = dict()

    def copyover(self):
        data_dict = dict()
        for k, v in kai.SERVICES.items():
            v.do_copyover(data_dict)

        for func in kai.HOOKS["copyover"]:
            func(data_dict)

        data_dict["pid"] = os.getpid()

        with open(f"{self.app}.pickle", mode="wb") as f:
            pickle.dump(data_dict, f)
        cmd = os.path.abspath(sys.modules[__name__].__file__)

        os.execlp(sys.executable, sys.executable, cmd)

    def _setup_logging(self):
        from rich.logging import RichHandler

        # aiomisc handles logging but we'll help it along with some better settings.
        log_handler = TimedRotatingFileHandler(
            filename=os.path.join(self.settings.LOG_DIR, f"{self.app}.log"),
            encoding="utf-8",
            utc=True,
            when="midnight",
            interval=1,
            backupCount=14,
        )
        formatter = logging.Formatter(fmt=f"[%(asctime)s] %(message)s", datefmt="%x %X")
        log_handler.setFormatter(formatter)

        rich_handler = RichHandler(
            rich_tracebacks=True,  # Enable rich tracebacks
            show_time=False,  # Time already shown by file logger
            show_path=False,  # Customize as needed
        )

        # Adding handlers to the root logger
        logging.root.addHandler(log_handler)
        logging.root.addHandler(rich_handler)

        logging.root.addHandler(log_handler)
        logging.root.setLevel(logging.INFO)
        self._log_handler = log_handler
        self._rich_handler = rich_handler

    def get_setting(self, name: str, default=None):
        return getattr(self.settings, f"{self.app.upper()}_{name.upper()}", default)

    def _generate_copyover_data(self) -> dict:
        copyover_data = None
        if os.path.exists(f"{self.app}.pickle"):
            with open(f"{self.app}.pickle", mode="rb") as f:
                try:
                    copyover_data = pickle.load(f)
                except Exception as err:
                    os.remove(f"{self.app}.pickle")
                    raise
                os.remove(f"{self.app}.pickle")
                pid = copyover_data.pop("pid", None)
                if pid != os.getpid():
                    raise Exception("Invalid copyover data! Server going down.")
            return copyover_data

    async def _pre_start(self):
        # as some services might depend on others to be in a usable state
        services_priority = sorted(
            self.services.values(),
            key=lambda s: getattr(s, "load_priority", 0),
            reverse=True,
        )

        # The at_pre_start hook is called regardless and is used for initial setup.
        for s in services_priority:
            if (func := getattr(s, "at_pre_start", None)) is not None:
                # Wrap it in a task and await on the task in order to satisfy a few libraries
                # using aiohttp and timeouts.
                task = asyncio.create_task(func())
                await task

        # the cold start is run if there is no copyover data.
        if self.cold_start:
            for s in services_priority:
                if func := getattr(s, "at_cold_start", None):
                    task = asyncio.create_task(func())
                    await task
        else:
            for s in services_priority:
                if func := getattr(s, "at_copyover_start", None):
                    task = asyncio.create_task(func())
                    await task

    async def _post_stop(self):
        pass

    def run(self):
        executor = asyncio
        try:
            import uvloop

            executor = uvloop
        except ImportError:
            pass

        executor.run(self.full_run())

    async def full_run(self):
        await self.setup()
        await self.execute()

    async def setup(self):
        """
        The big kahuna that starts everything off.
        """
        self._setup_logging()

        self.copyover_data = self._generate_copyover_data()

        if not self.copyover_data:
            logging.info(f"Beginning from Cold Start")
        else:
            self.cold_start = False
            logging.info(f"Copyover Data detected.")

        try:
            # Import and initialize classes and services from settings.
            for k, v in self.get_setting("CLASSES", dict()).items():
                kai.CLASSES[k] = import_from_module(v)
        except Exception as e:
            logging.error(f"{e}")
            logging.error(traceback.format_exc())
            return

        try:
            for k, v in self.get_setting("SERVICES", dict()).items():
                service_class = import_from_module(v)
                if check := getattr(service_class, "is_valid", None):
                    if not check(self.settings):
                        logging.error(f"Invalid service: {k} ({v}), excluding.")
                        continue
                service = service_class(self)
                self.services[k] = service
                kai.SERVICES[k] = service
        except Exception as e:
            logging.error(f"{e}")
            logging.error(traceback.format_exc())
            return

        for module in kai.SETTINGS.VALIDATOR_FUNC_MODULES:
            for k, v in callables_from_module(module).items():
                kai.VALIDATORS[k] = v

        for module in kai.SETTINGS.OPTION_CLASS_MODULES:
            for k, v in callables_from_module(module).items():
                kai.OPTION_CLASSES[k] = v

        for module in kai.SETTINGS.PORTAL_EVENT_HANDLER_MODULES:
            for k, v in callables_from_module(module).items():
                kai.PORTAL_EVENTS[k] = v

        await self._pre_start()

    async def execute(self):
        await asyncio.gather(*[service.start() for service in self.services.values()])

        await self._post_stop()

    async def handle_new_protocol(self, protocol):
        protocol.core = self
        try:
            self.game_sessions[protocol.capabilities.session_name] = protocol
            await protocol.run()
        except Exception as err:
            logging.error(traceback.format_exc())
            logging.error(err)
        finally:
            del self.game_sessions[protocol.capabilities.session_name]
