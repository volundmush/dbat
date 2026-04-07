class Sex:
    
    def valid_for_race(self, race: str) -> bool:
        return True


class Male(Sex):
    
    def valid_for_race(self, race: str) -> bool:
        return race.lower() != "namekian"


class Female(Sex):
    
    def valid_for_race(self, race: str) -> bool:
        return race.lower() != "namekian"


class Other(Sex):
    pass