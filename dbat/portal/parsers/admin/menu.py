import mudforge
from .base import BaseParser
from mudforge.portal.commands.base import CMD_MATCH
from httpx import HTTPStatusError
from mudforge.utils import partial_match

from dbat.models.game import AccountData, PlayerData


class AdminParser(BaseParser):
    """
    Implements the character selection and user management features.
    """