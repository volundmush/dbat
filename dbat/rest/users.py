from typing import Annotated

import typing
import uuid

from fastapi import APIRouter, Depends, HTTPException, status

from mudforge.rest.utils import (
    streaming_list
)

from dbat.rest.utils import get_current_user
from dbat.models.game import AccountData, PlayerData
from dbat.db import users as users_db, characters as character_db

router = APIRouter()


@router.get("/", response_model=typing.List[AccountData])
async def get_users(user: Annotated[AccountData, Depends(get_current_user)]):
    if user.adminLevel < 1:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN, detail="Insufficient permissions."
        )

    users = users_db.list_users()
    return streaming_list(users)


@router.get("/{user_id}", response_model=AccountData)
async def get_user(
    user_id: int, user: Annotated[AccountData, Depends(get_current_user)]
):
    if user.adminLevel < 1 and user.id != user_id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN, detail="Insufficient permissions."
        )

    found = users_db.get_user(user_id)
    return found

@router.get("/name/{user_name}", response_model=AccountData)
async def get_user(
    user_name: str, user: Annotated[AccountData, Depends(get_current_user)]
):
    if user.adminLevel < 1 and user.name.upper() != user_name.upper():
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN, detail="Insufficient permissions."
        )

    found = users_db.find_user(user_name)
    return found


@router.get("/{user_id}/characters", response_model=typing.List[PlayerData])
async def get_user_characters(
    user_id: int, user: Annotated[AccountData, Depends(get_current_user)]
):
    if user.adminLevel < 1 and user.id != user_id:
        raise HTTPException(
            status_code=status.HTTP_403_FORBIDDEN, detail="Insufficient permissions."
        )
    
    if user_id != user.id:
        user = users_db.get_user(user_id)

    found = character_db.for_user(user)
    return streaming_list(found)