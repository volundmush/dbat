from sanic.response import json
from sanic import Sanic, response
from sanic import Blueprint

from sanic_jwt import Initialize
from sanic_jwt import exceptions, protected, inject_user
from jsonschema import validate, ValidationError

import time
import kai
from kai import CRYPT_CONTEXT
from kai.utils.utils import get_true_ip

account_data_schema = {
    "type": "object",
    "properties": {
        "vn": {"type": "number"},
        "name": {"type": "string", "minLength": 1},  # Name is required and must be a non-empty string
        "passHash": {"type": "string"},
        "password": {"type": "string"},
        "email": {"type": "string", "format": "email"},  # You can enforce email format
        "created": {"type": "number"},
        "lastLogin": {"type": "number"},
        "lastLogout": {"type": "number"},
        "lastPasswordChanged": {"type": "number"},
        "totalPlayTime": {"type": "number"},
        "disabledReason": {"type": "string"},
        "disabledUntil": {"type": "number"},
        "adminLevel": {"type": "number"},
        "rpp": {"type": "number"},
        "slots": {"type": "number"},
        "characters": {
            "type": "array",
            "items": {"type": "number"}
        }
    },
    "required": ["name"],  # Only name is strictly required
    "additionalProperties": False  # Disallow properties not listed above
}

new_account_data_schema = {
    "type": "object",
    "properties": {
        "name": {"type": "string", "minLength": 1},  # Name is required and must be a non-empty string
        "password": {"type": "string"},
        "email": {"type": "string", "format": "email"},  # You can enforce email format
    },
    "required": ["name", "password"],  # Only name is strictly required
    "additionalProperties": False  # Disallow properties not listed above
}

from circlemud import account_manager, player_manager

accounts = Blueprint("accounts", url_prefix="/accounts")

@accounts.get("/<account_id:int>")
@protected()
@inject_user()
async def get_account(request, user, account_id):
    if user["adminLevel"] < 1:
        return response.json({"error": "Unauthorized"}, status=401)
    if account := account_manager.get(account_id):
        return response.json(account)
    else:
        return response.json({"error": "Account not found"}, status=404)

@accounts.get("/")
@protected()
@inject_user()
async def get_account_range(request, user):
    if user["adminLevel"] < 1:
        return response.json({"error": "Unauthorized"}, status=401)
    start = request.args.get("start", default=0, type=int)
    end = request.args.get("end", default=999999, type=int)
    accounts = account_manager.get_range(start, end)
    return response.json(accounts)

CREATION_TRACKER: dict[str, float] = dict()

@accounts.post("/")
async def create_account(request):
    ip = get_true_ip(request)
    if request.remote_addr in CREATION_TRACKER:
        if time.time() - CREATION_TRACKER[ip] < 300:
            return response.json({"error": "Rate limited"}, status=429)
    try:
        validate(request.json, new_account_data_schema)
    except ValidationError as e:
        return response.json({"error": "Invalid account data", "details": e.message}, status=400)
    
    if account_manager.exists(request.json["name"]):
        return response.json({"error": "Name already in use"}, status=400)

    try:
        request.json["passHash"] = CRYPT_CONTEXT.hash(request.json["password"])
    except (TypeError, KeyError):
        return response.json({"error": "Invalid account data", "details": "Password not hashable"}, status=400)
    account = account_manager.create(request.json)
    CREATION_TRACKER[ip] = time.time()
    return response.json({"success": True}, status=200)

PASSWORD_CHANGE_TRACKER: dict[str, float] = dict()

@accounts.patch("/<account_id:int>")
@protected()
@inject_user()
async def update_account(request, user, account_id):
    # TODO: add name validation...
    ip = get_true_ip(request)
    if user["adminLevel"] < 4:
        return response.json({"error": "Unauthorized"}, status=401)
    if not (account := account_manager.get(account_id)):
        return response.json({"error": "Account not found"}, status=404)
    try:
        validate(request.json, account_data_schema)
    except ValidationError as e:
        return response.json({"error": "Invalid account data", "details": e.message}, status=400)
    
    if "adminLevel" in request.json and request.json["adminLevel"] > user["adminLevel"]:
        return response.json({"error": "Unauthorized"}, status=401)

    # strip out the unchangeable fields.
    for field in ("passHash", "vn"):
        request.json.pop(field, None)
    
    if (name := request.json.get("name", None)):
        if name != account["name"]:
            if account_manager.exists(name, exclude=account_id):
                return response.json({"error": "Name already in use"}, status=400)
    
    if (password := request.json.pop("password", None)) is not None:
        if ip in PASSWORD_CHANGE_TRACKER:
            if time.time() - PASSWORD_CHANGE_TRACKER[ip] < 300:
                return response.json({"error": "Rate limited"}, status=429)
        try:
            request.json["passHash"] = CRYPT_CONTEXT.hash(password)
        except (TypeError, KeyError):
            return response.json({"error": "Invalid account data", "details": "Password not hashable"}, status=400)
    
    if err := account_manager.patch(account_id, request.json):
        return response.json({"error": err}, status=400)
    # return 200.
    if password:
        PASSWORD_CHANGE_TRACKER[ip] = time.time()
    return response.json({"success": True}, status=200)

@accounts.get("/<account_id:int>/characters")
@protected()
@inject_user()
async def get_characters(request, user, account_id):
    if not (account := account_manager.get(account_id)):
        return response.json({"error": "Account not found"}, status=404)
    if ((user["adminLevel"] < 1) and (user["id"] != account_id)):
        return response.json({"error": "Unauthorized"}, status=401)
    if not (characters := account.get("characters", [])):
        return response.json({"error": "No characters found"}, status=404)
    out = [{"id": found['id'], "name": found['name']} for x in characters if (found := player_manager.get(x))]
    return response.json(out)