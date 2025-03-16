import os
import orjson
from pathlib import Path
from dbat.models import game as game_models
from fastapi import HTTPException, status
import mudforge

from dbat.events.circle import CircleText

from cython.operator cimport dereference as deref, preincrement as inc
from libcpp.memory cimport shared_ptr
from libcpp.string cimport string


cimport structs
cimport db
from structs cimport unit_data, thing_data, room_data, char_data, obj_data, account_data
from saveload cimport jdumps, jloads, jobject, to_json, from_json

def load_db():
    """
    Wraps the C++ boot_db_new() function.
    """
    cur_path = Path().absolute()
    os.chdir("lib")
    db.boot_db_new()
    os.chdir(cur_path)


cdef class RoomDB:
    
    cdef string _dump(self, room_data* r):
        j = jobject()
        to_json(j, deref(r))
        return jdumps(j)

    def __getitem__(self, vn: int) -> RoomData:
        r = db.get_room(vn)
        if not r:
            raise ValueError(f"Room {vn} not found.")
        return game_models.RoomData.model_validate_json(self._dump(r))

    def __iter__(self) -> typing.AsyncGenerator[RoomData, None]:
        for vn in db.world:
            yield game_models.RoomData.model_validate_json(self._dump(vn.second.get()))
    
    def __len__(self) -> int:
        return db.world.size()
    
    def __contains__(self, vn: int) -> bool:
        return db.world.find(vn) != db.world.end()
    
    def keys(self) -> typing.AsyncGenerator[int, None]:
        for vn in db.world:
            yield vn.first


room_db = RoomDB()

cdef class ObjectPrototypeDB:
    
    cdef string _dump(self, obj_data& o):
        j = jobject()
        to_json(j, o)
        return jdumps(j)

    def __getitem__(self, vn: int) -> game_models.ObjectData:
        o = db.obj_proto.find(vn)
        if o == db.obj_proto.end():
            raise ValueError(f"Object {vn} not found.")
        res = deref(o)
        return game_models.ObjectData.model_validate_json(self._dump(res.second))

    def __iter__(self) -> typing.AsyncGenerator[ObjectData, None]:
        for vn in db.obj_proto:
            yield game_models.ObjectData.model_validate_json(self._dump(vn.second))
    
    def __len__(self) -> int:
        return db.obj_proto.size()
    
    def __contains__(self, vn: int) -> bool:
        return db.obj_proto.find(vn) != db.obj_proto.end()
    
    def keys(self) -> typing.AsyncGenerator[int, None]:
        for vn in db.obj_proto:
            yield vn.first

obj_proto_db = ObjectPrototypeDB()

cdef class MobilePrototypeDB:

    cdef string _dump(self, char_data& c):
        j = jobject()
        to_json(j, c)
        return jdumps(j)

    def __getitem__(self, vn: int) -> game_models.CharData:
        c = db.mob_proto.find(vn)
        if c == db.mob_proto.end():
            raise ValueError(f"Mobile {vn} not found.")
        res = deref(c)
        return game_models.CharData.model_validate_json(self._dump(res.second))

    def __iter__(self) -> typing.AsyncGenerator[game_models.CharData, None]:
        for vn in db.mob_proto:
            yield game_models.CharData.model_validate_json(self._dump(vn.second))
    
    def __len__(self) -> int:
        return db.mob_proto.size()
    
    def __contains__(self, vn: int) -> bool:
        return db.mob_proto.find(vn) != db.mob_proto.end()
    
    def keys(self) -> typing.AsyncGenerator[int, None]:
        for vn in db.mob_proto:
            yield vn.first

mob_proto_db = MobilePrototypeDB()

cdef class ShopDB:
    
    cdef string _dump(self, structs.shop_data& s):
        j = jobject()
        to_json(j, s)
        return jdumps(j)

    def __getitem__(self, vn: int) -> game_models.ShopData:
        s = db.shop_index.find(vn)
        if s == db.shop_index.end():
            raise ValueError(f"Shop {vn} not found.")
        res = deref(s)
        return game_models.ShopData.model_validate_json(self._dump(res.second))
    
    def __iter__(self) -> typing.AsyncGenerator[game_models.ShopData, None]:
        for vn in db.shop_index:
            yield game_models.ShopData.model_validate_json(self._dump(vn.second))
    
    def __len__(self) -> int:
        return db.shop_index.size()
    
    def __contains__(self, vn: int) -> bool:
        return db.shop_index.find(vn) != db.shop_index.end()
    
    def keys(self) -> typing.AsyncGenerator[int, None]:
        for vn in db.shop_index:
            yield vn.first

shop_db = ShopDB()

cdef class GuildDB:
    
    cdef string _dump(self, structs.guild_data& g):
        j = jobject()
        to_json(j, g)
        return jdumps(j)

    def __getitem__(self, vn: int) -> game_models.GuildData:
        g = db.guild_index.find(vn)
        if g == db.guild_index.end():
            raise ValueError(f"Guild {vn} not found.")
        res = deref(g)
        return game_models.GuildData.model_validate_json(self._dump(res.second))
    
    def __iter__(self) -> typing.AsyncGenerator[game_models.GuildData, None]:
        for vn in db.guild_index:
            yield game_models.GuildData.model_validate_json(self._dump(vn.second))
    
    def __len__(self) -> int:
        return db.guild_index.size()
    
    def __contains__(self, vn: int) -> bool:
        return db.guild_index.find(vn) != db.guild_index.end()
    
    def keys(self) -> typing.AsyncGenerator[int, None]:
        for vn in db.guild_index:
            yield vn.first

guild_db = GuildDB()

cdef class ScriptDB:
    
    cdef string _dump(self, structs.trig_data* i):
        j = jobject()
        to_json(j, deref(i))
        return jdumps(j)

    def __getitem__(self, vn: int) -> game_models.IndexData:
        i = db.trig_index.find(vn)
        if i == db.trig_index.end():
            raise ValueError(f"Script {vn} not found.")
        res = deref(i)
        return game_models.IndexData.model_validate_json(self._dump(res.second.proto))
    
    def __iter__(self) -> typing.AsyncGenerator[game_models.IndexData, None]:
        for vn in db.trig_index:
            yield game_models.IndexData.model_validate_json(self._dump(vn.second.proto))
    
    def __len__(self) -> int:
        return db.trig_index.size()
    
    def __contains__(self, vn: int) -> bool:
        return db.trig_index.find(vn) != db.trig_index.end()
    
    def keys(self) -> typing.AsyncGenerator[int, None]:
        for vn in db.trig_index:
            yield vn.first

script_db = ScriptDB()

cdef class AccountDB:
    
    cdef string _dump(self, account_data& a):
        j = jobject()
        to_json(j, a)
        return jdumps(j)

    def __getitem__(self, vn: int) -> game_models.AccountData:
        a = db.accounts.find(vn)
        if a == db.accounts.end():
            raise ValueError(f"Account {vn} not found.")
        res = deref(a)
        return game_models.AccountData.model_validate_json(self._dump(res.second))
    
    def __iter__(self) -> typing.AsyncGenerator[game_models.AccountData, None]:
        for vn in db.accounts:
            yield game_models.AccountData.model_validate_json(self._dump(vn.second))
    
    def __len__(self) -> int:
        return db.accounts.size()
    
    def __contains__(self, vn: int) -> bool:
        return db.accounts.find(vn) != db.accounts.end()
    
    def keys(self) -> typing.AsyncGenerator[int, None]:
        for vn in db.accounts:
            yield vn.first

account_db = AccountDB()

cdef class PlayerDB:
    
    cdef string _dump(self, structs.player_data& p):
        j = jobject()
        to_json(j, p)
        return jdumps(j)

    def __getitem__(self, vn: int) -> game_models.PlayerData:
        p = db.players.find(vn)
        if p == db.players.end():
            raise ValueError(f"Player {vn} not found.")
        res = deref(p)
        return game_models.PlayerData.model_validate_json(self._dump(res.second))
    
    def __iter__(self) -> typing.AsyncGenerator[game_models.PlayerData, None]:
        for vn in db.players:
            yield game_models.PlayerData.model_validate_json(self._dump(vn.second))
    
    def __len__(self) -> int:
        return db.players.size()
    
    def __contains__(self, vn: int) -> bool:
        return db.players.find(vn) != db.players.end()
    
    def keys(self) -> typing.AsyncGenerator[int, None]:
        for vn in db.players:
            yield vn.first

player_db = PlayerDB()


def submit_command(character_id: int, command: str):
    """
    Submit a command to the game.
    """
    sess = db.sessions.find(character_id)
    if sess == db.sessions.end():
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Character session not found.")
    sess = deref(sess)
    sess.raw_input_queue.push_back(command)

def connection_lost(character_id: int, connection_id: int):
    """
    Notify the game that a connection has been lost in the FastAPI.
    """
    passsess = db.sessions.find(character_id)
    if sess == db.sessions.end():
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Character session not found.")
    sess = deref(sess)
    sess.onConnectionLost(connection_id)

def create_join_session(account_id: int, character_id: int, connection_id: int)
    """
    Creates or joins a session for a character. This is called by FastAPI
    when it opens an SSE stream. This will simply wrap a C++ function
    that does the heavy lifting to K.I.S.S.
    """
    pass

async def distribute_output():
    """
    Iterate through all connected sessions and shove their output out to the
    EventHub as CircleText events and clear the buffer.
    """
    hub = mudforge.EVENT_HUB
    for sess in db.sessions:
        desc = sess.second
        if desc.output.empty():
            continue
        event = CircleText(text=desc.output)
        await hub.send(sess.first, event)
        desc.output.clear()