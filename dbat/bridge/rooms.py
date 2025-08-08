import typing
from dbat.bridge.models.game import RoomData, RoomDirectionData
from dbat_ext import room_db

def list_rooms() -> typing.Generator[RoomData, None, None]:
    for room in room_db.list_rooms():
        yield RoomData(**room)

def get_room(room_id: int) -> RoomData:
    return RoomData(**room_db.get_room(room_id))