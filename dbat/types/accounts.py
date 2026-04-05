import uuid
import dbat

class Account:
    def __init__(self):
        self.id: uuid.UUID = uuid.NIL
        self.email: str = ""
        self.username: str = ""
        self.rpp: int = 0
        self.rpp_bank: int = 0
        self.admin_level: int = 0
    
    def save(self):
        dbat.DIRTY_ACCOUNTS.add(self.id)