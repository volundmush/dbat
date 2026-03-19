import asyncio
import uuid
from muforge.application import Service


class RequestHandler(Service):
    def __init__(self, app, plugin):
        super().__init__(app, plugin)
        self.outstanding_requests: dict[uuid.UUID, asyncio.Future] = {}

    @property
    def core(self):
        return self.app.plugins["core"]

    async def handle_response(self, conn, pid, channel, payload):
        try:
            request_id = uuid.UUID(payload)
        except ValueError:
            return

        row = await conn.fetchrow(
            """
            SELECT response_status, response_data
            FROM dbat.api_requests
            WHERE id = $1
            """,
            request_id,
        )
        if not row:
            return

        await conn.execute(
            """
            DELETE FROM dbat.api_requests
            WHERE id = $1
            """,
            request_id,
        )

        future = self.outstanding_requests.pop(request_id, None)
        if future is not None and not future.done():
            future.set_result((row["response_status"], row["response_data"]))

    async def run(self):
        async with self.core.db.connection() as conn:
            await conn.add_listener("dbat_api_request_responded", self.handle_response)
            # Do nothing in order to keep the listener running.
            while True:
                try:
                    await asyncio.sleep(10)
                except asyncio.CancelledError:
                    break

    async def submit_request(self, user_id: uuid.UUID, request_type: str, data: dict):

        async with self.core.db.connection() as conn:
            row = await conn.fetchrow(
                """
                INSERT INTO dbat.api_requests (user_id, request_type, data)
                VALUES ($1, $2, $3, $4)
                RETURNING id
                """,
                user_id,
                request_type,
                data,
            )
            if not row:
                return
            request_id = row["id"]

        future = asyncio.get_running_loop().create_future()
        self.outstanding_requests[request_id] = future
        return await future
