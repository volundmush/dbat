from fastapi import HTTPException, status

from dbat.models.game import AccountData
from mudforge.utils import crypt_context
from .users import find_user
from dbat_ext import account_db

async def register_user(
    username: str, hashed_password: str
) -> AccountData:
    adminLevel = 0

    # if there are no users, make this user an admin.
    if not len(account_db):
        adminLevel = 10

    user = AccountData(
        name=username,
        passHash=hashed_password,
        adminLevel=adminLevel
    )
    
    # TODO: actually save it.
    
    return user


async def authenticate_user(
    username: str, password: str, ip: str, user_agent: str | None
) -> AccountData:
    try:
        retrieved = find_user(username)
    except KeyError:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="User not found."
        )
    if not crypt_context.verify(password, retrieved.passHash):
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="Invalid credentials."
        )
    return retrieved

