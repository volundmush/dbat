import socketio
import logging
import kai
import asyncio


class LoginParser:
    
    def __init__(self, session):
        self.session = session