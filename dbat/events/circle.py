from dbat.events.base import EventBase

class CircleText(EventBase):
    text: str
    
    async def handle_event(self, conn: "BaseConnection"):
        await conn.send_circle(self.text)
