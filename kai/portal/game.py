import socketio
import logging
import kai
import asyncio

class GameParser:
    
    def __init__(self, session):
        self.session = session
        
        self.sio = socketio.AsyncClient()
        
        self.sio.on("*", self.on_event)
        self.sio.on("connect", self.on_connect)
        self.sio.on("disconnect", self.on_disconnect)
    
    async def run(self):
        headers = {"X-FORWARDED-FOR": self.session.capabilities.host_address}
        await self.sio.connect(
            kai.SETTINGS.PORTAL_URL_TO_GAME, wait=True, headers=headers
        )
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
        while msg := await self.outgoing_queue.get():
            event = msg[0]
            data = msg[1]
            await self.sio.emit(event, data=data)