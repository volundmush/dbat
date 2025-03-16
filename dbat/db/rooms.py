import typing
from dbat.models.game import RoomData, RoomDirectionData
from dbat_ext import room_db

def list_rooms() -> typing.Generator[RoomData, None]:
    return room_db.list_rooms()

def get_room(room_id: int) -> RoomData:
    return room_db.get_room(room_id)