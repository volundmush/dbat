from mudforge.portal.parsers.base import BaseParser as _Base

class BaseParser(_Base):
    
    async def send_circle(self, text: str):
        await self.connection.send_circle(text)