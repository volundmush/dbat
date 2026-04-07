from .base import CharacterCommand
from ..types.grids import Direction

class Move(CharacterCommand):
    key = "dbat/move"
    help_category = "Movement"
    
    help_name = "go"
    match_defs = {"go": 2, 
                  "move": 2,
                  "north": 1,
                  "south": 1,
                  "east": 1,
                  "west": 1,
                  "northeast": 6,
                  "ne": 2,
                  "northwest": 6,
                  "nw": 2,
                  "southeast": 6,
                  "se": 2,
                  "southwest": 6,
                  "sw": 2,
                  "inside": 2,
                  "outside": 3
                  }
    
    def func(self):

        full_command = self.parsed.get("full_command", "")
        if full_command in ("go", "move"):
            direction_str = self.parsed.get("lsargs", "").lower()
        else:
            match full_command:
                case "ne":
                    direction_str = "northeast"
                case "nw":
                    direction_str = "northwest"
                case "se":
                    direction_str = "southeast"
                case "sw":
                    direction_str = "southwest"
                case _:
                    direction_str = full_command.lower()
        
        try:
            direction = Direction(direction_str)
        except ValueError:
            self.send_line(f"Invalid direction: {direction_str}")
            return {"ok": False, "error": f"Invalid direction: {direction_str}"}

        result = self.character.move_direction(direction)
        if not result:
            err = result.unwrap_err()
            self.send_line(f"You can't go {direction_str} from here: {err}")
            return {"ok": False, "error": f"Cannot move {direction_str}: {err}"}

        self.character.look_at(self.character.location)
        return {"ok": True}