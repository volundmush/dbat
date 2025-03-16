import typing
from dbat.models.game import AccountData
from fastapi import HTTPException, status
from dbat_ext import account_db

async def list_users() -> typing.AsyncGenerator[AccountData, None]:
    # We have no choice but to turn this generator into a full list
    # of models in order to prevent concurrency issues.
    data = list(account_db)
    for user in data:
        yield user
    

def get_user(account_id: int) -> AccountData:
    try:
        return account_db[account_id]
    except KeyError:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="User not found."
        )

def find_user(username: str) -> AccountData:
    provided = username.upper()
    for user in account_db:
        if user.name.upper() == provided:
            return user
    raise HTTPException(
        status_code=status.HTTP_400_BAD_REQUEST, detail="User not found."
    )