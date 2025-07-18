import typing
from dbat.models.game import AccountData, PlayerData, ChargenData
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

def find_character(username: str, should_raise=True) -> PlayerData:
    provided = username.upper()
    for player in player_db:
        if player["name"].upper() == provided:
            return PlayerData(**player)
    if should_raise:
        raise HTTPException(
        status_code=status.HTTP_400_BAD_REQUEST, detail="Player Character not found."
    )
    return None

async def for_user(user: AccountData) -> typing.AsyncGenerator[PlayerData, None]:
    for character_id in user.characters:
        try:
            player = get_character(character_id)
            yield player
        except HTTPException:
            pass

async def create_character(user: AccountData, char_data: ChargenData) -> PlayerData:
    try:
        char_data.check()
        if (found := find_character(char_data.name, should_raise=False)) is not None:
            raise ValueError(f"Character with name {char_data.name} already exists.")
        return player_db.create_character(user, char_data)
    except ValueError as e:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST, detail=str(e)
        )
    except Exception as e:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR, detail=f"An error occurred while creating the character: {e}"
        )