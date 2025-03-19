import typing
from dbat.models.game import AccountData, PlayerData
from fastapi import HTTPException, status
from dbat_ext import player_db

async def list_characters() -> typing.AsyncGenerator[PlayerData, None]:
    # We have no choice but to turn this generator into a full list
    # of models in order to prevent concurrency issues.
    data = list(player_db)
    for player in data:
        yield PlayerData(**player)

def get_character(character_id: int) -> PlayerData:
    try:
        return PlayerData(**player_db[character_id])
    except KeyError:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail="Player Character not found."
        )

def find_character(username: str) -> PlayerData:
    provided = username.upper()
    for player in player_db:
        if player["name"].upper() == provided:
            return PlayerData(**player)
    raise HTTPException(
        status_code=status.HTTP_400_BAD_REQUEST, detail="Player Character not found."
    )

async def for_user(user: AccountData) -> typing.AsyncGenerator[PlayerData, None]:
    for character_id in user.characters:
        try:
            player = get_character(character_id)
            yield player
        except HTTPException:
            pass