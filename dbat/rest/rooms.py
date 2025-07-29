import pydantic
from typing import Annotated, Optional

import dbat
import typing
import dbat_ext

from fastapi import APIRouter, Depends, Body, HTTPException, status, Request
from fastapi.responses import StreamingResponse
from dbat.rest.utils import streaming_list
from dbat.models import fields

from dbat.rest.utils import get_real_ip
from .utils import get_current_user

from dbat.models.game import AccountData, PlayerData, RoomData
from dbat.models.characters import CharacterCreate
from dbat.db import rooms as rooms_db

router = APIRouter()

@router.get("/", response_model=typing.List[RoomData])
async def get_characters(user: Annotated[AccountData, Depends(get_current_user)]):
    if not user.admin_level > 0:
        raise HTTPException(
            status_code=403, detail="You do not have permission to view all rooms."
        )

    stream = rooms_db.list_rooms()

    return streaming_list(stream)


@router.get("/{room_id}", response_model=RoomData)
async def get_character(
    user: Annotated[AccountData, Depends(get_current_user)], room_id: int
):
    if not user.admin_level > 0:
        raise HTTPException(
            status_code=403, detail="You do not have permission to view all rooms."
        )
    room = rooms_db.get_room(room_id)
    return room
