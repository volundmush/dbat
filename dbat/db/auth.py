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
        password=hashed_password,
        admin_level=adminLevel
    )
    
    # this will have handled validation so now we just need to do something about the SecretStr
    data = user.model_dump(exclude_unset=True)
    # deal with the secretstr.
    data["password"] = hashed_password
    
    new_user = account_db.create(data)
    
    return AccountData(**new_user)


async def authenticate_user(
    username: str, password: str, ip: str, user_agent: str | None
) -> AccountData:
    try:
        retrieved = find_user(username)
    except KeyError:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="User not found."
        )
    plain_text = False
    if not retrieved.password:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="User has no password."
        )
    retrieved_password = retrieved.password.get_secret_value()
    if password == retrieved_password:
        plain_text = True
    if not plain_text and not crypt_context.verify(password, retrieved_password):
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="Invalid credentials."
        )
    if plain_text:
        # hash the password if it was plain text
        hashed = crypt_context.hash(password)
        retrieved.password = hashed
        account_db.update(retrieved.id, {"password": hashed})
    return retrieved

