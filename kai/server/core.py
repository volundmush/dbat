import socketio
import os
import asyncio
import queue
import time
import pathlib
from sanic import Sanic, response, exceptions

from sanic_jwt import Initialize
import circlemud
from circlemud import account_manager

import kai
from kai.utils.utils import class_from_module, callables_from_module
webroot_path = pathlib.Path(os.path.abspath(os.path.join(os.path.dirname(__file__), 'webclient'))) / "dist"
from kai.api import v1


class ServerCore:
    def __init__(self, app: Sanic):
        self.app = app
        sio = socketio.AsyncServer(async_mode="sanic", namespaces="*")
        kai.SOCKETIO = sio
        self.sio = sio
        sio.attach(app)
        app.ctx.socketio = sio
        self.settings = app.ctx.settings
        self.auth = Initialize(app, claim_aud=self.settings.SERVER_INTERFACE, authenticate=account_manager.authenticate, retrieve_user=account_manager.retrieve_user)
        self.tasks = list()
        self.connections: dict[str, circlemud.GameSession] = dict()

        self.setup_routes()

        app.register_listener(self.init_game, "before_server_start")
        app.register_listener(self.run_game, "before_server_start")

        sio.on("*", self.message_handler)
        sio.on("connect", self.connect_handler)
        sio.on("disconnect", self.disconnect_handler)

    def setup_routes(self):
        # Static file serving
        #self.app.static('/assets', os.path.join(webroot_path, 'assets'))
        #self.app.route('/')(self.index)
        self.app.blueprint(v1)
    
    async def init_game(self, *args, **kwargs):
        circlemud.initialize()
    
    async def run_game(self, *args, **kwargs):
        self.app.add_task(circlemud.run_game_loop())

    async def index(self, request):
        return await response.file(os.path.join(webroot_path, 'index.html'))

    def get_real_ip(self, environ):
        best_choice = environ.get("REMOTE_ADDR")
        if (forwarded := environ.get("HTT_X_FORWARDED_FOR", None)):
            best_choice = forwarded.split(",")[0]
        return best_choice

    async def get_payload(self, environ):
        if (token := environ.get("HTTP_AUTHORIZATION", None)):
            return None
        if not token.startswith("Bearer "):
            return None
        token = token.split(" ")[1]
        try:
            # Assuming `token` is the JWT extracted from the SocketIO connection
            # Use Sanic-JWT's internal method to validate and decode the token
            payload = await self.auth.authentication._decode(token, verify=True)
            return payload
        except exceptions.SanicJWTException as e:
            # Handle invalid token cases (e.g., expired, invalid signature)
            print("Invalid JWT:", str(e))
            return False

    async def connect_handler(self, sid, environ):
        ip = self.get_real_ip(environ)
        if not (payload := await self.get_payload(environ)):
            print(f"connect_handler: Invalid JWT from {ip}")
            await self.sio.disconnect(sid)
            return
        user_id = payload.get("user_id")
        new_conn = circlemud.GameSession(sid, self.sio, ip, user_id)
        self.connections[sid] = new_conn
        self.app.add_task(new_conn.run(), name=f"Connection {sid}")

    async def disconnect_handler(self, sid):
        if conn := self.connections.pop(sid, None):
            await conn.handle_disconnect()
        else:
            print(f"disconnect_handler: No connection for sid {sid}")

    async def message_handler(self, event, sid, message):
        if conn := self.connections.get(sid, None):
            await conn.handle_event(event, message)
        else:
            print(f"message_handler: No connection for sid {sid}")


def create_application(settings) -> Sanic:
    kai.SETTINGS = settings

    app = Sanic(settings.NAME)

    app.ctx.settings = settings

    core_class = class_from_module(settings.SERVER_CLASSES["core"])
    app.ctx.core = core_class(app)

    return app
