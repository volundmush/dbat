class Shop:
    
    def dump(self) -> dict:
        return dict()

    @classmethod
    def load(cls, data: dict) -> "Shop":
        return cls()