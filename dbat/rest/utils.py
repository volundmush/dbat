import mudforge
import jwt
import uuid
import pydantic
import orjson
import typing
from datetime import datetime
from dataclasses import dataclass
from typing import Annotated, Optional

from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm
from fastapi import Request, Depends, HTTPException, status
from fastapi.responses import StreamingResponse

from dbat.models.game import AccountData
from dbat.db.users import find_user

from mudforge.rest.utils import oauth2_scheme

async def get_current_user(token: Annotated[str, Depends(oauth2_scheme)]) -> AccountData:
    credentials_exception = HTTPException(
        status_code=status.HTTP_401_UNAUTHORIZED,
        detail="Could not validate credentials",
        headers={"WWW-Authenticate": "Bearer"},
    )
    jwt_settings = mudforge.SETTINGS["JWT"]
    try:
        payload = jwt.decode(
            token, jwt_settings["secret"], algorithms=[jwt_settings["algorithm"]]
        )
        if (user_id := payload.get("sub", None)) is None:
            raise credentials_exception
    except jwt.PyJWTError as e:
        raise credentials_exception

    try:
        user = find_user(user_id)
    except KeyError:
        raise credentials_exception

    return user