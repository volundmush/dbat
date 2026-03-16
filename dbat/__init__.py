from muforge.plugin import BasePlugin


class DBAT(BasePlugin):
    async def setup_final(self):
        from .requests import RequestHandler

        self.request_handler = RequestHandler(self)
        self.app.fastapi_instance.state.dbat_requests = self.request_handler
        self.app.task_group.create_task(self.request_handler.listen_events())

    def depends(self):
        return [("core", ">=0.0.1")]
