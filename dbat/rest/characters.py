import pydantic
from typing import Annotated, Optional

import mudforge
import typing
import dbat_ext

from fastapi import APIRouter, Depends, Body, HTTPException, status, Request
from fastapi.responses import StreamingResponse
from mudforge.rest.utils import streaming_list
from mudforge.models import fields

from .utils import get_current_user

from dbat.models.game import AccountData, PlayerData
from mudforge.models.characters import CharacterCreate
from dbat.db import characters as characters_db

router = APIRouter()

from itertools import count
conn_id_counter = count(start=1)

@router.get("/", response_model=typing.List[PlayerData])
async def get_characters(user: Annotated[AccountData, Depends(get_current_user)]):
    if not user.adminLevel > 0:
        raise HTTPException(
            status_code=403, detail="You do not have permission to view all characters."
        )

    stream = characters_db.list_characters()

    return streaming_list(stream)


@router.get("/{character_id}", response_model=PlayerData)
async def get_character(
    user: Annotated[AccountData, Depends(get_current_user)], character_id: int
):
    character = await characters_db.get_character(character_id)
    if character.id not in user.characters and user.adminLevel == 0:
        raise HTTPException(status_code=403, detail="Character does not belong to you.")
    return character


@router.get("/{character_id}/events")
async def stream_character_events(
    user: Annotated[AccountData, Depends(get_current_user)], character_id: int
):
    queue = mudforge.EVENT_HUB.subscribe(character_id)

    # We don't use it; but this verifies that user can control character.
    if character_id not in user.characters:
        raise HTTPException(
            status_code=403, detail="You do not have permission to use this character."
        )

    async def event_generator():
        conn_id = next(conn_id_counter)
        try:
            while True:
                item = await queue.get()  # blocks until a new event
                yield f"event: {item.__class__.__name__}\ndata: {item.model_dump_json()}\n\n"
        finally:
            mudforge.EVENT_HUB.unsubscribe(character_id, queue)

    return StreamingResponse(event_generator(), media_type="text/event-stream")


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
    char_data: Annotated[CharacterCreate, Body()],
):
    result = await characters_db.create_character(user, char_data.name)
    return result
