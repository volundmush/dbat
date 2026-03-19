from muforge.plugin import BasePlugin


class DBAT(BasePlugin):

    def name(self):
        return "Dragon Ball: Advent Truth"

    def slug(self):
        return "dbat"

    def version(self):
        return "0.0.1"
    
    def game_services(self):
        from .requests import RequestHandler

        return {"request_handler": RequestHandler}

    async def setup_final(self):
        request_handler = self.app.services["request_handler"]
        self.app.fastapi_instance.state.dbat_requests = request_handler

    def depends(self):
        return [("core", ">=0.0.1")]

    def game_migrations(self):
        from .migrations import version001

        return [("version001", version001)]