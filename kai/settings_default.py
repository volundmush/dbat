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
PORTAL_CLASSES["login_parser"] = "kai.portal.login.LoginParser"
PORTAL_CLASSES["game_parser"] = "kai.portal.game.GameParser"
PORTAL_CLASSES["admin_parser"] = "kai.portal.admin.AdminParser"

PORTAL_SERVICES = {"telnet": "kai.portal.telnet.TelnetService"}

PORTAL_EVENT_HANDLER_MODULES = ["kai.portal.events"]
SERVER_EVENT_HANDLER_MODULES = ["kai.db.events"]

DEBUG = True

LOG_DIR = "logs"

# Modules holding Option classes, responsible for serializing the option and
# calling validator functions on it. Same-named functions in modules added
# later in this list will override those added earlier.
OPTION_CLASS_MODULES = ["kai.utils.optionclasses"]
# Module holding validator functions. These are used as a resource for
# validating options, but can also be used as input validators in general.
# Same-named functions in modules added later in this list will override those
# added earlier.
VALIDATOR_FUNC_MODULES = ["kai.utils.validatorfuncs"]

# This should absolutely be overridden in secret_settings.py!
JWT_SECRET = "adventkai"