class Sensei:
    
    @property
    def key(self) -> str:
        return self.name.lower()

    @property
    def name(self) -> str:
        return self.__class__.__name__
    
    @property
    def abbreviation(self) -> str:
        return self.name[:3]
    

    def is_playable(self) -> bool:
        return True
    
    def valid_for_race(self, race: str) -> bool:
        return race.lower() != "android"


class Nobody(Sensei):
    """
    This is a placeholder sensei for characters who have none. It should never be assigned.
    """
    
    def is_playable(self) -> bool:
        return False
    
    def valid_for_race(self, race: str) -> bool:
        return True


class Roshi(Sensei):
    pass


class Kibito(Sensei):
    pass


class Nail(Sensei):
    pass


class Bardock(Sensei):
    pass



class Crane(Sensei):
    pass


class Tapion(Sensei):
    pass


class Piccolo(Sensei):
    pass


class Sixteen(Sensei):
    
    def valid_for_race(self, race: str) -> bool:
        return race.lower() == "android"


class Dabura(Sensei):
    
    def valid_for_race(self, race: str) -> bool:
        return race.lower() == "demon"


class Frieza(Sensei):
    pass


class Ginyu(Sensei):
    pass


class Jinto(Sensei):
    
    def valid_for_race(self, race: str) -> bool:
        return race.lower() == "hoshijin"


class Kurzak(Sensei):
    
    def valid_for_race(self, race: str) -> bool:
        return race.lower() == "arlian"


class Tsuna(Sensei):
    
    def valid_for_race(self, race: str) -> bool:
        return race.lower() == "kanassan"
