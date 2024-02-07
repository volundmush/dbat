from collections import defaultdict

# The game name
NAME = "dbat"

# The interface used by the Server to run its webserver/SocketIO on.
SERVER_INTERFACE = "0.0.0.0"
# The port used by the server for its webserver.
SERVER_PORT = 8000

# The interface used by the portal for its listening ports.
PORTAL_INTERFACE = "0.0.0.0"
# The port that the portal will use for telnet.
PORTAL_TELNET = 1280

PORTAL_URL_TO_GAME = "http://127.0.0.1:8000"

# Classes that the server will use for various things.
SERVER_CLASSES = dict()
SERVER_CLASSES["core"] = "kai.server.core.ServerCore"


PARSERS = {
    "login": "kai.server.login.LoginParser",
    "main_menu": "kai.server.main_menu.MainMenuParser",
}

PORTAL_CLASSES = dict()
PORTAL_CLASSES["core"] = "kai.portal.portal.Core"

PORTAL_SERVICES = {"telnet": "kai.portal.telnet.TelnetService"}

PORTAL_EVENT_HANDLER_MODULES = ["kai.portal.events"]
SERVER_EVENT_HANDLER_MODULES = ["kai.db.events"]

DEBUG = True
