import os
import orjson
import asyncio
import time
from pathlib import Path
from fastapi import HTTPException, status
import mudforge

from dbat.events.circle import CircleText

from cython.operator cimport dereference as deref, preincrement as inc
from libcpp.memory cimport shared_ptr
from libcpp.string cimport string
from libcpp.vector cimport vector


cimport structs
cimport db
from structs cimport unit_data, thing_data, room_data, char_data, obj_data, account_data, help_index_element
from saveload cimport jdumps, jloads, jobject, to_json, from_json, runSave, create_player_character

def load_db():
    """
    Wraps the C++ boot_db_new() function.
    """
    db.load_config()
    cur_path = Path().absolute()
    os.chdir("lib")
    db.init()
    #os.chdir(cur_path)

def get_help(name, level):
    data = db.get_help(name.encode("ascii", errors="ignore"), level)
    if not data:
        return None
    j = jobject()
    to_json(j, deref(data))
    return orjson.loads(jdumps(j))

cdef class RoomDB:
    
    cdef string _dump(self, room_data* r):
        j = jobject()
        to_json(j, deref(r))
        return jdumps(j)

    def __getitem__(self, vn: int) -> dict:
        r = db.get_room(vn)
        if not r:
            raise ValueError(f"Room {vn} not found.")
        return orjson.loads(self._dump(r))

    def __iter__(self) -> typing.AsyncGenerator[dict, None]:
        for vn in db.world:
            yield orjson.loads(self._dump(vn.second.get()))
    
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

    def __getitem__(self, vn: int) -> dict:
        o = db.obj_proto.find(vn)
        if o == db.obj_proto.end():
            raise ValueError(f"Object {vn} not found.")
        res = deref(o)
        return orjson.loads(self._dump(res.second))

    def __iter__(self) -> typing.AsyncGenerator[dict, None]:
        for vn in db.obj_proto:
            yield orjson.loads(self._dump(vn.second))
    
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

    def __getitem__(self, vn: int) -> dict:
        c = db.mob_proto.find(vn)
        if c == db.mob_proto.end():
            raise ValueError(f"Mobile {vn} not found.")
        res = deref(c)
        return orjson.loads(self._dump(res.second))

    def __iter__(self) -> typing.AsyncGenerator[dict, None]:
        for vn in db.mob_proto:
            yield orjson.loads(self._dump(vn.second))
    
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

    def __getitem__(self, vn: int) -> dict:
        s = db.shop_index.find(vn)
        if s == db.shop_index.end():
            raise ValueError(f"Shop {vn} not found.")
        res = deref(s)
        return orjson.loads(self._dump(res.second))
    
    def __iter__(self) -> typing.AsyncGenerator[dict, None]:
        for vn in db.shop_index:
            yield orjson.loads(self._dump(vn.second))
    
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

    def __getitem__(self, vn: int) -> dict:
        g = db.guild_index.find(vn)
        if g == db.guild_index.end():
            raise ValueError(f"Guild {vn} not found.")
        res = deref(g)
        return orjson.loads(self._dump(res.second))
    
    def __iter__(self) -> typing.AsyncGenerator[dict, None]:
        for vn in db.guild_index:
            yield orjson.loads(self._dump(vn.second))
    
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

    def __getitem__(self, vn: int) -> dict:
        i = db.trig_index.find(vn)
        if i == db.trig_index.end():
            raise ValueError(f"Script {vn} not found.")
        res = deref(i)
        return orjson.loads(self._dump(res.second.proto))
    
    def __iter__(self) -> typing.AsyncGenerator[gdict, None]:
        for vn in db.trig_index:
            yield orjson.loads(self._dump(vn.second.proto))
    
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

    def __getitem__(self, vn: int) -> dict:
        a = db.accounts.find(vn)
        if a == db.accounts.end():
            raise ValueError(f"Account {vn} not found.")
        res = deref(a)
        return orjson.loads(self._dump(res.second))
    
    def __iter__(self) -> typing.AsyncGenerator[dict, None]:
        for vn in db.accounts:
            yield orjson.loads(self._dump(vn.second))
    
    def __len__(self) -> int:
        return db.accounts.size()
    
    def __contains__(self, vn: int) -> bool:
        return db.accounts.find(vn) != db.accounts.end()
    
    def keys(self) -> typing.AsyncGenerator[int, None]:
        for vn in db.accounts:
            yield vn.first
    
    def create(self, data: dict):
        # the dict should only contain name, passHash, and adminLevel.
        # we'll add the ID here.
        id = db.getNextAccountID()
        data["vn"] = id
        serialized = orjson.dumps(data)
        # now we'll turn it into a nlohmann::json...
        j = jloads(serialized)
        # and insert into the accounts map.
        a = db.accounts[id]
        from_json(j, a)
        db.accounts[id] = a
        return orjson.loads(self._dump(a))
    
    def update(self, id: int, data: dict):
        found = db.accounts.find(id)
        if found == db.accounts.end():
            raise ValueError(f"Account {id} not found.")
        a = deref(found).second
        serialized = orjson.dumps(data)
        j = jloads(serialized)
        from_json(j, a)
        return orjson.loads(self._dump(a))


account_db = AccountDB()

cdef class PlayerDB:
    
    cdef string _dump(self, structs.player_data& p):
        j = jobject()
        to_json(j, p)
        return jdumps(j)

    def __getitem__(self, vn: int) -> dict:
        p = db.players.find(vn)
        if p == db.players.end():
            raise ValueError(f"Player {vn} not found.")
        res = deref(p)
        return orjson.loads(self._dump(res.second))
    
    def __iter__(self) -> typing.AsyncGenerator[dict, None]:
        for vn in db.players:
            yield orjson.loads(self._dump(vn.second))
    
    def __len__(self) -> int:
        return db.players.size()
    
    def __contains__(self, vn: int) -> bool:
        return db.players.find(vn) != db.players.end()
    
    def keys(self) -> typing.AsyncGenerator[int, None]:
        for vn in db.players:
            yield vn.first
    
    def create_character(self, user, data):
        result = create_player_character(user.id, jloads(data.model_dump_json().encode("utf-8")))
        return orjson.loads(self._dump(deref(result)))

player_db = PlayerDB()

def process_colors(text: str, parse: bool = True) -> str:
    """
    Wraps the C++ processColors function.
    """
    t = text.encode("utf-8")
    out = db.processColors(t, parse, NULL)
    return out.decode("utf-8")

def submit_command(character_id: int, command: str):
    """
    Submit a command to the game.
    """
    sess_found = db.sessions.find(character_id)
    if sess_found == db.sessions.end():
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Character session not found.")
    sess = deref(sess_found)
    cmd = command.encode("utf-8")
    sess.second.raw_input_queue.push_back(cmd)

def connection_lost(character_id: int, connection_id: int):
    """
    Notify the game that a connection has been lost in the FastAPI.
    """
    sess_found = db.sessions.find(character_id)
    if sess_found == db.sessions.end():
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Character session not found.")
    sess = deref(sess_found)
    sess.second.onConnectionLost(connection_id)

def create_join_session(account_id: int, character_id: int, connection_id: int, ip: str):
    """
    Creates or joins a session for a character. This is called by FastAPI
    when it opens an SSE stream. This will simply wrap a C++ function
    that does the heavy lifting to K.I.S.S.
    """
    ip_bytes = ip.encode("utf-8")
    res = db.create_join_session(account_id, character_id, connection_id, ip_bytes)
    if res < 0:
        if res == -1:
            raise HTTPException(status_code=status.HTTP_404_NOT_FOUND, detail="Character not found.")
        elif res == -2:
            raise HTTPException(status_code=status.HTTP_409_CONFLICT, detail="This account already has another character in play.")

cdef void distribute_output() noexcept:
    """
    Iterate through all connected sessions and shove their output out to the
    EventHub as CircleText events and clear the buffer.
    """
    hub = mudforge.EVENT_HUB
    for sess in db.sessions:
        desc = sess.second
        if desc.processed_output.empty():
            continue
        event = CircleText(text=desc.processed_output.decode("utf-8"))
        hub.send_nowait(sess.first, event)
        desc.processed_output.clear()

cdef void send_close(int character_id) noexcept:
    """
    Send a close event to the EventHub for a character.
    """
    hub = mudforge.EVENT_HUB
    event = CircleText(text="Connection closed.")
    hub.send_nowait(character_id, event)
    hub.send_nowait(character_id, None)

RUNNING = True

async def run_game(heartbeat_interval: float, save_interval: float):
    # hook up the C++ functions to the Python functions
    db.g_distribute_output = distribute_output
    db.g_send_close = send_close

    save_timer = save_interval
    try:
        while RUNNING:
            start = time.perf_counter()
            db.runOneLoop(heartbeat_interval)
            end = time.perf_counter()
            distribute_output()

            save_timer -= heartbeat_interval
            if save_timer <= 0:
                save_timer = save_interval
                # save the game state
                runSave()
                
            elapsed = end - start
            if elapsed < heartbeat_interval:
                await asyncio.sleep(heartbeat_interval - elapsed)
            else:
                # we still await in order to prevent blocking.
                await asyncio.sleep(0)
        # If we reached here, the while loop terminated
        # gracefully and we should do a final save dump.
        runSave()
    except asyncio.CancelledError:
        pass
    except Exception as e:
        print(f"Game loop error: {e}")
        raise e
    finally:
        print("Game loop stopped.")

def get_names(category: str) -> list[str]:
    cdef vector[string] names
    
    out = list()
    if category == "races":
        names = db.getRaceNames()
    elif category == "senseis":
        names = db.getSenseiNames()
    elif category == "forms":
        names = db.getFormNames()
    elif category == "skills":
        names = db.getSkillNames()
    elif category == "room_flags":
        names = db.getRoomFlagNames()
    elif category == "sector_types":
        names = db.getSectorTypeNames()
    elif category == "sizes":
        names = db.getSizeNames()
    elif category == "player_flags":
        names = db.getPlayerFlagNames()
    elif category == "mob_flags":
        names = db.getMobFlagNames()
    elif category == "pref_flags":
        names = db.getPrefFlagNames()
    elif category == "affect_flags":
        names = db.getAffectFlagNames()
    elif category == "item_types":
        names = db.getItemTypeNames()
    elif category == "wear_flags":
        names = db.getWearFlagNames()
    elif category == "item_flags":
        names = db.getItemFlagNames()
    elif category == "admin_flags":
        names = db.getAdminFlagNames()
    elif category == "directions":
        names = db.getDirectionNames()
    elif category == "attributes":
        names = db.getAttributeNames()
    elif category == "attribute_trains":
        names = db.getAttributeTrainNames()
    elif category == "appearances":
        names = db.getAppearanceNames()
    elif category == "aligns":
        names = db.getAlignNames()
    elif category == "moneys":
        names = db.getMoneyNames()
    elif category == "vitals":
        names = db.getVitalNames()
    elif category == "nums":
        names = db.getNumNames()
    elif category == "stats":
        names = db.getStatNames()
    elif category == "dims":
        names = db.getDimNames()
    elif category == "com_stats":
        names = db.getComStatNames()
    elif category == "shop_flags":
        names = db.getShopFlagNames()
    elif category == "character_flags":
        names = db.getCharacterFlagNames()
    elif category == "zone_flags":
        names = db.getZoneFlagNames()
    elif category == "subraces":
        names = db.getSubRaceNames()
    elif category == "sexes":
        names = db.getSexNames()
    elif category == "mutations":
        names = db.getMutationNames()
    elif category == "bio_genomes":
        names = db.getBioGenomeNames()
    else:
        return out
    
    for name in names:
        out.append(name.decode("utf-8"))
    return out