from sanic.response import json
from sanic import Sanic, response
from sanic import Blueprint
from sanic_jwt import exceptions, protected, inject_user

admin = Blueprint("admin", url_prefix="/admin")

@admin.get("/")
@protected()
@inject_user()
async def get_admin(request, user):
    if user["adminLevel"] > 0:
        return response.json({"error": False}, status=200)
    else:
        return response.json({"error": "Unauthorized."}, status=401)