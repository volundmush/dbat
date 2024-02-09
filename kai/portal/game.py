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
    
    
    
    async def run(self):
        await self.sio.connect(
            kai.SETTINGS.PORTAL_URL_TO_GAME, wait=True, headers=self.session.headers()
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
    
    
    
    async def close(self):
        if self.task:
            self.task.cancel()
        if self.sio.connected:
            await self.sio.disconnect()
