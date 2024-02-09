import socketio
import logging
import kai
import asyncio
import jwt
import time

class GameParser:
    
    def __init__(self, session):
        self.session = session
        self.jwt_decoded = jwt.decode(self.session.jwt, options={"verify_signature": False})
        
        self.sio = socketio.AsyncClient()
        
        self.sio.on("*", self.on_event)
        self.sio.on("connect", self.on_connect)
        self.sio.on("disconnect", self.on_disconnect)

        self.task = None
    
    @property
    def http(self):
        return self.session.http
    
    def headers(self):
        headers = {
            "Authorization": f"Bearer {self.session.jwt}",
            "X-Forwarded-For": self.session.capabilities.host_address
        }
        return headers
    
    async def run(self):
        await self.sio.connect(
            kai.SETTINGS.PORTAL_URL_TO_GAME, wait=True, headers=self.headers
        )
        self.task = asyncio.create_task(self.run_game())
        await self.task
        
    async def run_game(self):
        await asyncio.gather(*[self.run_messaging(), self.run_idler()])
    
    async def on_connect(self):
        pass

    async def on_disconnect(self):
        pass

    async def on_event(self, event: str, message):
        if found := kai.PORTAL_EVENTS.get(event, None):
            try:
                await found(self.session, event, message)
            except Exception as e:
                logging.error(
                    f"Got an Exception while processing Event {event} for {self.sio.sid}: {e} (message: {message})"
                )
                logging.exception(e)
        else:
            logging.warning(f"Event {event} not found in {kai.PORTAL_EVENTS}")
    
    async def run_idler(self):
        while True:
            await asyncio.sleep(5.0)
            await self.sio.emit("idle", data=dict())

    async def run_messaging(self):
        while msg := await self.session.outgoing_queue.get():
            event = msg[0]
            data = msg[1]
            await self.sio.emit(event, data=data)
    
    async def run_refresh(self):
        while True:
            if not self.jwt_decoded:
                await asyncio.sleep(10)
                continue
            exp = self.jwt_decoded.get("exp")
            current = time.time()
            remaining = exp - current - 120

            if remaining > 0:
                await asyncio.sleep(remaining)
            await self.refresh()
    
    async def refresh(self):
        headers = self.headers()
        result = await self.http.post("/auth/refresh", headers=headers, json={})
        if result.status == 200:
            jwt = result.json().get("access_token")
            self.session.jwt = jwt
            self.jwt_decoded = jwt.decode(jwt, options={"verify_signature": False})
    
    async def close(self):
        if self.task:
            self.task.cancel()
        if self.sio.connected:
            await self.sio.disconnect()
