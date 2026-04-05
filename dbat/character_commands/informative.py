from .base import CharacterCommand
from ..types.grids import Direction

class Look(CharacterCommand):
    key = "dbat/look"
    help_category = "Informative"
    
    help_name = "look"
    match_defs = {"look": 1}
    
    def look_location(self, location):
        if not location:
            self.send_line("You are in an unknown location. This isn't going to work well.")
            return
        self.character.look_at(location)

    def func(self):
        if not (target := self.parsed.get("args", "").strip()):
            self.look_location(self.character.location)
            return
        
        self.send_line("Hold tight on looking at specific targets right now...")