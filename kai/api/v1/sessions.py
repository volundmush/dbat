from sanic import response
from sanic import Blueprint
from sanic_jwt import protected, inject_user

import kai

sessions = Blueprint("sessions", url_prefix="/sessions")

@sessions.get("/")
@protected()
@inject_user()
async def get_sessions(request, user):
    if user["adminLevel"] < 1:
        return response.json({"error": "Unauthorized"}, status=401)
    
    out = {k: v.serialize() for k, v in kai.GAME.connections.items()}
    return response.json(out, status=200)