
class Race:

    @property
    def key(self):
        return self.name.lower()
    
    @property
    def name(self):
        return self.__class__.__name__

    @property
    def abbreviation(self):
        return self.name[:3]

    def __str__(self):
        return self.name
    
    def modifier_name(self) -> str:
        return f"Race: {self.name}"
    
    def is_playable(self) -> bool:
        return True
    
    def apply_stat_modifier(self, character: "Character", stat_mod: "StatModifier"):
        """
        Apply a stat modifier to this character. This is used for things like racial stat bonuses.
        By default, this does nothing, but subclasses can override this to provide more nuanced behavior.
        """
        pass


class Human(Race):
    pass


class Saiyan(Race):
    pass


class Icer(Race):
    pass


class Halfbreed(Race):
    
    @property
    def name(self):
        return "Half-Breed"
    
    @property
    def abbreviation(self):
        return "H-B"


class Namekian(Race):
    pass


class Mutant(Race):
    pass

class Konatsu(Race):
    
    def apply_stat_modifier(self, character: "Character", stat_mod: "StatModifier"):
        match stat_mod.storage_key:
            case "vitals:lifeforce_max":
                stat_mod.additive_multipliers.append((self.modifier_name(), -0.15))

class Kanassan(Race):
    pass


class BioAndroid(Race):

    @property
    def name(self):
        return "Bio-Android"


class Android(Race):
    pass


class Demon(Race):
    
    def apply_stat_modifier(self, character: "Character", stat_mod: "StatModifier"):
        match stat_mod.storage_key:
            case "vitals:lifeforce_max":
                stat_mod.additive_multipliers.append((self.modifier_name(), -0.25))


class Majin(Race):
    pass


class Kai(Race):
    pass


class Tuffle(Race):
    pass


class Hoshijin(Race):
    pass


class Arlian(Race):
    
    def apply_stat_modifier(self, character: "Character", stat_mod: "StatModifier"):
        match stat_mod.storage_key:
            case "vitals:lifeforce_max":
                ki_max = character.get_stat("ki_max")
                stamina_max = character.get_stat("stamina_max")
                molt_level = character.get_stat("molt_level")
                total = (ki_max * 0.01) * (molt_level / 100) + (stamina_max * 0.01) * (molt_level / 100)
                stat_mod.additive_multipliers.append((self.modifier_name(), total))
                

class _Unplayable(Race):
    
    def is_playable(self) -> bool:
        return False

class Animal(_Unplayable):
    pass


class Saiba(_Unplayable):
    pass


class Ogre(_Unplayable):
    pass


class Serpent(_Unplayable):
    pass


class Yardratian(_Unplayable):
    pass


class Dragon(_Unplayable):
    
    @property
    def abbreviation(self):
        return "Drg"


class Mechanical(_Unplayable):
    pass


class Spirit(_Unplayable):
    pass