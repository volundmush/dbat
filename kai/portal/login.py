import socketio
import logging
import kai
import asyncio
from kai.utils.utils import partial_match
import pathlib

GREET = ""

def get_greet():
    global GREET
    if not GREET:
        path = pathlib.Path() / "lib" / "text" / "greetansi"
        GREET = open(path, mode="r").read()
    return GREET

INSTRUCTIONS = """
@rO@b---------------------------------------------------------------------@rO@n
This game has an account system. Characters are created/selected after login.
Note: Usernames are alphanumeric, no spaces allowed.
@Wconnect <username> <password>@n to login
 @Wcreate <username> <password>@n to register.
  @Wadmin <username> <password>@n to engage admin panel.
@rO@b---------------------------------------------------------------------@rO@n
"""

class LoginParser:
    
    def __init__(self, session):
        self.session = session
        self.running = True

    async def handle_create(self, username, password):
        if not username or not password:
            await self.session.send_text("Invalid username or password. Please try again.")
            return
        result = await self.session.http.post("/v1/accounts", json={"name": username, "password": password})
        if result.status_code  != 201:
            await self.session.send_text(f"Account creation failed. Error: {result.json().get('error', 'N/A')}.")
            return
        await self.session.send_text("Account created successfully.")
        await self.session.send_text("Please use the 'connect' command to login.")
        await self.session.send_text("Usage: connect <username> <password>")
        await self.session.send_text(f"Example: connect {username} {password}")
    
    
    async def handle_login(self, username, password):
        if not username or not password:
            await self.session.send_text("Invalid username or password. Please try again.")
            return
        result = await self.session.http.post("/auth", json={"username": username, "password": password})
        if result.status_code  != 200:
            await self.session.send_text(f"Login failed. Error: {result.json().get('reasons', 'N/A')}.")
            return
        if not (token := result.json().get("access_token", None)):
            await self.session.send_text("Login failed. No token received. Contact staff.")
            return
        self.session.jwt = token
        return token
    
    async def handle_connect(self, username, password):
        if not (await self.handle_login(username, password)):
            return
        self.running = False
        await self.session.send_text("Login successful.")
        await self.session.set_parser(kai.CLASSES["game_parser"](self.session))
    
    async def handle_admin(self, username, password):
        if not (await self.handle_login(username, password)):
            return
        headers = {
            "Authorization": f"Bearer {self.session.jwt}"
        }
        result = await self.session.http.get("/v1/admin", headers=headers)
        if result.status_code  != 200:
            await self.session.send_text(f"Admin login failed. Error: {result.json().get('error', 'N/A')}.")
            return
        self.running = False
        await self.session.set_parser(kai.CLASSES["admin_parser"](self.session))
    
    async def run(self):
        circle = kai.PORTAL_EVENTS["CircleText"]
        for text in (get_greet(), INSTRUCTIONS):
            await circle(self.session, "CircleText", {"data": text})
        
        while self.running:
            event, data = await self.session.outgoing_queue.get()
            if event != "Command":
                continue
            cmd = data.get("data", None)
            splitted = cmd.split(" ", 2)
            if len(splitted) != 3:
                await self.session.send_text("Invalid command. Please try again.")
                continue
            
            if not (operation := partial_match(splitted[0], ["create", "connect", "admin"])):
                await self.session.send_text("Invalid command. Please try again.")
                continue

            username, password = splitted[1], splitted[2]
            if not (func := getattr(self, f"handle_{operation}", None)):
                await self.session.send_text("Invalid command. Please try again.")
                continue
            await func(username, password)
    
    async def close(self):
        self.running = False