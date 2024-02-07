from sanic.response import json
from sanic import Sanic
from sanic import Blueprint

from sanic_jwt import Initialize
from sanic_jwt import exceptions, protected, inject_user

from .skills import skills
from .accounts import accounts

v1 = Blueprint.group(skills, accounts, version=1)

