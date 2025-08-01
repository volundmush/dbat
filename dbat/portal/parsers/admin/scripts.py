import dbat
from ..base import BaseParser
from dbat.portal.commands.base import CMD_MATCH
from httpx import HTTPStatusError
from dbat.utils import partial_match

from dbat.models.game import AccountData, PlayerData


class ScriptListParser(BaseParser):
    pass


class ScriptEditorParser(BaseParser):
    pass