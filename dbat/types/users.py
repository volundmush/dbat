import uuid
import dbat

class User:
    
    def __init__(self):
        self.id: uuid.UUID = uuid.NIL
        self.username: str = ""
        self.characters: set[uuid.UUID] = set()
        self.rpp: int = 0
        self.rpp_bank: int = 0
        self.admin_level: int = 0
    
    def is_admin(self) -> bool:
        return self.admin_level > 0

    def save(self):
        """
        Saves this user to the database. This should be called whenever the user's data is modified.
        """
        dbat.DIRTY_USERS.add(self.id)