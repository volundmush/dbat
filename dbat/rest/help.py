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

from dbat.models.game import AccountData, PlayerData, HelpData
from mudforge.models.characters import CharacterCreate
from dbat.db import help as help_db

router = APIRouter()

@router.get("/restricted/{name}", response_model=HelpData)
async def get_characters(name: str, user: Annotated[AccountData, Depends(get_current_user)]):
    help_data = help_db.get_help(name, user.admin_level)
    if not help_data:
        raise HTTPException(status_code=404, detail="Help entry not found.")
    return help_data

@router.get("/public/{name}", response_model=HelpData)
async def get_characters(name: str):
    help_data = help_db.get_help(name, 0)
    if not help_data:
        raise HTTPException(status_code=404, detail="Help entry not found.")
    return help_data