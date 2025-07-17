import typing
from dbat.models.game import AccountData, HelpData
from fastapi import HTTPException, status
from dbat_ext import get_help as get_help_db

def get_help(name: str, level: int) -> HelpData:
    try:
        return HelpData(**get_help_db(name, level))
    except KeyError:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, detail="Help entry not found."
        )
