import pydantic
from typing import Annotated, Optional

import mudforge
import typing
import dbat_ext

from fastapi import APIRouter, Depends, Body, HTTPException, status, Request
from fastapi.responses import StreamingResponse
from mudforge.rest.utils import streaming_list
from mudforge.models import fields

from mudforge.rest.utils import get_real_ip
from .utils import get_current_user

from dbat.models.game import AccountData, PlayerData, ChargenData
from mudforge.models.characters import CharacterCreate
from dbat.db import characters as characters_db

router = APIRouter()

from itertools import count
conn_id_counter = count(start=1)

@router.get("/", response_model=typing.List[PlayerData])
async def get_characters(user: Annotated[AccountData, Depends(get_current_user)]):
    if not user.admin_level > 0:
        raise HTTPException(
            status_code=403, detail="You do not have permission to view all characters."
        )

    stream = characters_db.list_characters()

    return streaming_list(stream)


@router.get("/{character_id}", response_model=PlayerData)
async def get_character(
    user: Annotated[AccountData, Depends(get_current_user)], character_id: int
):
    character = characters_db.get_character(character_id)
    if character.id not in user.characters and user.admin_level == 0:
        raise HTTPException(status_code=403, detail="Character does not belong to you.")
    return character

@router.get("/name/{character_name}", response_model=PlayerData)
async def get_character(
    user: Annotated[AccountData, Depends(get_current_user)], character_name: str
):
    character = characters_db.find_character(character_name)
    if character.id not in user.characters and user.admin_level == 0:
        raise HTTPException(status_code=403, detail="Character does not belong to you.")
    return character

@router.get("/{character_id}/events")
async def stream_character_events(
    request: Request,
    user: Annotated[AccountData, Depends(get_current_user)], character_id: int
):
    
    ip = get_real_ip(request)

    # this verifies that user can control character.
    if character_id not in user.characters:
        raise HTTPException(
            status_code=403, detail="You do not have permission to use this character."
        )

    async def event_generator(cid):
        try:
            graceful = False
            queue = mudforge.EVENT_HUB.subscribe(character_id)
            # run until we get a None or False or something stupid like that.
            while item := await queue.get():
                yield f"event: {item.__class__.__name__}\ndata: {item.model_dump_json()}\n\n"
            graceful = True
        finally:
            mudforge.EVENT_HUB.unsubscribe(character_id, queue)
            if not graceful:
                dbat_ext.connection_lost(character_id, cid)

    conn_id = next(conn_id_counter)
    # This might raise an HTTPException!
    dbat_ext.create_join_session(user.id, character_id, conn_id, ip)
    
    return StreamingResponse(event_generator(conn_id), media_type="text/event-stream")


class CommandSubmission(pydantic.BaseModel):
    command: str

@router.post("/{character_id}/command")
async def submit_command(
    user: Annotated[AccountData, Depends(get_current_user)],
    character_id: int,
    command: Annotated[CommandSubmission, Body()],
):
    if character_id not in user.characters:
        raise HTTPException(
            status_code=403, detail="You do not have permission to use this character."
        )
    
    if character_id not in mudforge.EVENT_HUB.online():
        raise HTTPException(
            status_code=403, detail="Character is not online."
        )

    dbat_ext.submit_command(character_id, command.command)

    return {"status": "ok"}

@router.post("/", response_model=PlayerData)
async def create_character(
    user: Annotated[AccountData, Depends(get_current_user)],
    char_data: Annotated[ChargenData, Body()],
):
    result = await characters_db.create_character(user, char_data)
    return result
