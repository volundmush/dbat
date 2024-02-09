from .base import AdminMenu
from ..command import Command


class WhoCommand(Command):
    keys = {"who": None}
    
    async def func(self):
        result = await self.get("/v1/sessions")
        if not result.status_code == 200:
            await self.send_text(f"An error occured: {result.json()}")
            return
        data = result.json()
        table = await self.session.rich_table("SID", "IP", "Time", "Idle", "User", "Character", title="Connections")
        for k, v in data.items():
            table.add_row(k, v["ip"], str(v["connected_time"]), str(v["idle_time"]), v["username"], v.get("character", ""))
        await self.send_rich(table)

class MainMenu(AdminMenu):
    commands = [WhoCommand]