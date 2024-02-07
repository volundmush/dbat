from cython.operator cimport dereference as deref, preincrement as inc
from libcpp.memory cimport shared_ptr
from libcpp.string cimport string

cimport comm
cimport net
cimport db
cimport structs
cimport accounts
cimport spells
cimport utils

import os
import asyncio
import time
import logging
import traceback
import pickle
import orjson
import pathlib
from datetime import datetime
from kai import CRYPT_CONTEXT


cdef class GameSession:
    cdef shared_ptr[net.Connection] conn
    cdef object sid
    cdef object sio
    cdef object running

    def __init__(self, sid, sio):
        self.sid = sid
        self.sio = sio
        self.running = True
        self.conn = net.newConnection(sid.encode())

    async def run(self):
        c = self.conn.get()
        while c.running:
            if c.outQueue.empty():
                await asyncio.sleep(0.05)
                continue

            message = c.outQueue.front()
            c.outQueue.pop_front()

            event = message.first.decode("UTF-8", errors='ignore')
            data = message.second.decode("UTF-8", errors='ignore')

            try:
                out_data = orjson.loads(data)
            except orjson.JSONDecodeError:
                continue

            await self.sio.emit(event, to=self.sid, data=out_data)

        if self.running:
            # the disconnect came from C++.
            await self.sio.disconnect(self.sid)

    async def handle_disconnect(self):
        self.running = False
        self.conn.get().state = net.ConnectionState.Dead

    async def handle_event(self, event: str, message):
        self.conn.get().queueMessage(event.encode(), orjson.dumps(message))

    async def start_resume(self):
        pass

    async def start_fresh(self):
        pass

    async def change_capabilities(self, changed: dict[str, "Any"]):
        for k, v in changed.items():
            setattr(self.capabilities, k, v)
            await self.at_capability_change(k, v)

    async def at_capability_change(self, capability: str, value):
        pass


def initialize():
    comm.init_locale()
    db.load_config()
    os.chdir("lib")
    comm.init_database()
    comm.init_zones()

# Keeping this here as an example of how to iterate through stuff.
def _hash_passwords():
    it = accounts.accounts.begin()
    end = accounts.accounts.end()

    while it != end:
        if not deref(it).second.passHash.empty():
            try:
                password = deref(it).second.passHash.decode("UTF-8", errors='ignore')
                hashed = CRYPT_CONTEXT.hash(password)
                deref(it).second.passHash = hashed.encode()
            except (TypeError, ValueError):
                print(f"Failed to hash password for {deref(it).second.name.decode('UTF-8', errors='ignore')}")
        inc(it)

def migrate():
    comm.init_locale()
    db.load_config()
    os.chdir("lib")
    comm.migrate_db()
    _hash_passwords()
    comm.runSave()

def run_loop_once(deltaTime: float):
    comm.run_loop_once(deltaTime)


RUNNING = True

async def run_game_loop():
    deltaTimeInSeconds: float = 0.1
    loop_frequency: float = 0.1
    save_timer: float = 60.0 * 5.0
    last_time = time.perf_counter()

    while RUNNING:
        start = time.perf_counter()
        comm.run_loop_once(deltaTimeInSeconds)
        end = time.perf_counter()

        save_timer -= deltaTimeInSeconds
        if save_timer <= 0.0:
            comm.runSave()
            save_timer = 60.0 * 5.0

        duration = end - start
        wait_time = loop_frequency - duration
        if wait_time < 0:
            wait_time = 0.001

        await asyncio.sleep(wait_time)
        deltaTimeInSeconds = time.perf_counter() - start



cdef accountToJSON(accounts.account_data* account):
    serialized = account.serialize()
    dumped = utils.jdump(serialized)
    return orjson.loads(dumped)

cdef class _AccountManager:

    def get(self, vn: int):
        account = accounts.accounts.find(vn)
        if account == accounts.accounts.end():
            return None
        return accountToJSON(deref(account).second)

    def create(self, data: dict[str, "Any"]):
        vn = accounts.account_data.getNextID()
        data["vn"] = vn
        j = orjson.dumps(data)
        cdef accounts.account_data* acc = &accounts.accounts[vn]
        acc.deserialize(utils.jparse(j))
    
    def patch(self, target: int, data: dict[str, "Any"]) -> typing.Optional[str]:
        account = accounts.accounts.find(target)
        if account == accounts.accounts.end():
            return "Account not found."
        j = orjson.dumps(data)
        deref(account).second.deserialize(utils.jparse(j))

    async def retrieve_user(self, request, payload, *args, **kwargs):
        if payload:
            if not (user_id := payload.get("user_id", None)):
                return None
            found = accounts.accounts.find(user_id)
            if found == accounts.accounts.end():
                return None
            user = deref(found).second

            out = {"user_id": user.vn, "username": user.name.decode("UTF-8", errors='ignore'), "adminLevel": user.adminLevel}
            if not user.email.empty():
                out["email"] = user.email.decode("UTF-8", errors='ignore')

            return out

        return None

    async def authenticate(self, request, *args, **kwargs):
        from sanic_jwt import exceptions
        username = request.json.get("username", None)
        password = request.json.get("password", None)

        if not username or not password:
            raise exceptions.AuthenticationFailed("Missing username or password.")

        user = accounts.findAccount(username.encode())

        if user is NULL:
            raise exceptions.AuthenticationFailed("Incorrect credentials.")

        passhash = user.passHash.decode("UTF-8", errors='ignore')
        if not passhash:
            raise exceptions.AuthenticationFailed("Incorrect credentials.")
        
        if not CRYPT_CONTEXT.verify(password, passhash):
            raise exceptions.AuthenticationFailed("Incorrect credentials.")

        out = {"user_id": user.vn, "name": user.name.decode("UTF-8", errors='ignore'), "adminLevel": user.adminLevel}
        if not user.email.empty():
            out["email"] = user.email.decode("UTF-8", errors='ignore')
        if not user.characters.empty():
            out["characters"] = [x for x in user.characters]

        return out
    
    def exists(self, name: str, exclude: int = None) -> bool:
        found = accounts.findAccount(name.encode())
        if found is NULL:
            return False
        if exclude is not None:
            if found.vn == exclude:
                return False
        return True

account_manager = _AccountManager()




cdef class _SkillManager:
    min_id = 0
    max_id = 999

    def direct_get(self, num: int):
        out = {
            "id": num
        }
        spell: spells.spell_info_type = spells.spell_info[num]
        if spell.name is not NULL:
            out["name"] = spell.name.decode("UTF-8", errors='ignore')
        return out

    def get_range(self, start: int, end: int):
        order = sorted([start, end])
        true_start = max(self.min_id, order[0])
        true_end = min(self.max_id, order[1])

        out = list()

        for i in range(true_start, true_end):
            out.append([i, self.direct_get(i)])

        return out

    def get_many(self, ids: list[int,...]):
        out = list()
        for num in ids:
            if (found := self.get(num)):
                out.append([num, found])
        return out

    def get(self, num: int):
        if num > self.max_id:
            return dict()
        if num < self.min_id:
            return dict()
        return self.direct_get(num)


skill_manager = _SkillManager()

cdef playerToJSON(structs.player_data* player):
    serialized = player.serialize()
    dumped = utils.jdump(serialized)
    return orjson.loads(dumped)

cdef class _PlayerManager:
    def get(self, vn: int):
        player = db.players.find(vn)
        if player == db.players.end():
            return None
        return playerToJSON(deref(player).second)

    def create(self, data: dict[str, "Any"]):
        vn = accounts.account_data.getNextID()
        data["vn"] = vn
        j = orjson.dumps(data)
        cdef accounts.account_data* acc = &accounts.accounts[vn]
        acc.deserialize(utils.jparse(j))
    
    def patch(self, target: int, data: dict[str, "Any"]) -> typing.Optional[str]:
        account = accounts.accounts.find(target)
        if account == accounts.accounts.end():
            return "Account not found."
        j = orjson.dumps(data)
        deref(account).second.deserialize(utils.jparse(j))

player_manager = _PlayerManager()