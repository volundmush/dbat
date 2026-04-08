import typing
import dbat
from pydantic import BaseModel, Field, ConfigDict, PrivateAttr
from muplugins.core.db.fields import RichText
from rich.text import Text

if typing.TYPE_CHECKING:
    from .characters import Character

Modifier = typing.Tuple[str, float]


class StatModifier:
    def __init__(self, stat_def: StatDef, gain=False):
        self.stat_def = stat_def
        self.gain = gain

        # Absolute value modifiers. The lowest floor, and lowest ceiling are used to clamp.
        self.pre_floors: list[Modifier] = []
        self.pre_ceilings: list[Modifier] = []

        # Raw value modifiers applied before any multiplication.
        self.pre_modifiers: list[Modifier] = []

        # The additive modifiers will be added to a 1.0. So a modifier of 0.5 would result in a 1.5 multiplier, and a modifier of -0.5 would result in a 0.5 multiplier.
        self.additive_multipliers: list[Modifier] = []

        # After additive, the multiplicative modifiers will be multiplied together.
        # So two modifiers of 0.5 would result in a 0.25 multiplier.
        self.multiplicative_multipliers: list[Modifier] = []

        # Raw value modifiers added after all multiplications.
        self.post_modifiers: list[Modifier] = []

        self.post_floors: list[Modifier] = []
        self.post_ceilings: list[Modifier] = []

    @property
    def key(self) -> str:
        return self.stat_def.key
    
    @property
    def category(self) -> str:
        return self.stat_def.category
    
    @property
    def storage_key(self) -> str:
        if self.gain:
            return f"{self.stat_def.storage_key()}:gain"
        return self.stat_def.storage_key()

    def apply(self, number: float) -> float:
        pre = number
        pre += sum(mod[1] for mod in self.pre_modifiers)
        pre_floor = max(mod[1] for mod in self.pre_floors) if self.pre_floors else None
        pre_ceiling = min(mod[1] for mod in self.pre_ceilings) if self.pre_ceilings else None
        if pre_floor is not None:
            pre = max(pre, pre_floor)
        if pre_ceiling is not None:
            pre = min(pre, pre_ceiling)

        add_mul = 1.0
        add_mul += sum(mod[1] for mod in self.additive_multipliers)

        mul_mul = 1.0
        for m in self.multiplicative_multipliers:
            mul_mul *= m
        
        post = pre * add_mul * mul_mul

        post += sum(mod[1] for mod in self.post_modifiers)

        post_floor = max(mod[1] for mod in self.post_floors) if self.post_floors else None
        post_ceiling = min(mod[1] for mod in self.post_ceilings) if self.post_ceilings else None
        if post_floor is not None:
            post = max(post, post_floor)
        if post_ceiling is not None:
            post = min(post, post_ceiling)
        
        return post

class StatDef:
    # The unique key for this stat. This is used for targeting with commands and storage in dictionaries.
    key: str = ""
    category: str = ""
    description: str = ""

    # Tags used for categorizing and listing/searching.
    tags: set[str] = set()

    # Min and max values, after all modifiers are applied. if None, there is no limit.
    min_value: float | None = None
    max_value: float | None = None

    # limits on the base value. set, mod, gain, cannot cause the base to go outside these.
    min_base: float | None = None
    max_base: float | None = None
    default_base: float = 0.0

    # a set of other stats which must be calculated first.
    # beware of circular dependencies here.
    depends: set[str] = set()

    # If it's flat, it doesn't bother with the complex custom calcs. the
    # effective current value == the base.
    flat: bool = False

    # a pure derived stat is never saved to the database.
    pure_derived: bool = False

    def storage_key(self) -> str:
        return f"{self.category}:{self.key}"

    def base(self, character: Character, cached: bool = True) -> float:
        """
        Get the base value of this stat for the given character. This is before applying any modifiers.
        """
        if cached and (found := character.stats._cache.get(self.storage_key(), None)) is not None:
            return found

        base = self.calculate_base(character)
        character.stats._cache[self.storage_key()] = base
        return base

    def set_base(self, character: Character, value: float):
        """
        Set the base value of this stat for the given character. This is before applying any modifiers.
        """
        character.stats.stats[self.storage_key()] = value
        character.stats.invalidate_cache()

    def retrieve_base(self, character: Character) -> float:
        return character.stats.get(self.storage_key(), self.default_base)

    def calculate_base(self, character: Character) -> float:
        """
        Get the base value of this stat for the given character. This is before applying any modifiers.
        """
        base = self.retrieve_base(character)
        base = max(base, self.min_base) if self.min_base is not None else base
        base = min(base, self.max_base) if self.max_base is not None else base
        return base

    def gather_dependencies(self, character: Character) -> dict[str, float]:
        """
        Gather the dependencies for this stat, in order. This is used for calculating stats that are derived from others.
        """
        out = dict()
        for dep in self.depends:
            category, key = dep.split(":")
            out[dep] = character.get_stat(category, key)
        return out

    def set(self, character: Character, value: float):
        """
        Set the base value of this stat for the given character. This is before applying any modifiers.
        """
        self.set_base(character, value)
    
    def mod(self, character: Character, delta: float):
        """
        Modify the base value of this stat for the given character by the given delta. This is before applying any modifiers.
        """
        current = self.get_base(character)
        new_value = current + delta
        self.set(character, new_value)
    
    def gain(self, character: Character, delta: float):
        """
        Gain this stat by the given delta. This is the preferred way to modify stats, 
        as it uses the modifier system.
        """
        modifier = self.get_stat_modifier(character, gain=True)

        applied = modifier.apply(delta)
        
        self.mod(character, applied)

    def current(self, character: Character, cached=True) -> float:
        """
        Get the current value of this stat for the given character. This is after applying all modifiers.
        """
        if cached and (found := character.stats._cache.get(self.storage_key(), None)) is not None:
            return found
        
        calculated = self.calculate_current(character)
        character.stats._cache[self.storage_key()] = calculated
        return calculated

    def get_stat_modifiers(self, character: Character, gain: bool = False, cached: bool = True) -> StatModifier:
        """
        Get the stat modifiers for this stat for the given character. This is used for calculating the current value of the stat.
        """
        stat_mod = StatModifier(self, gain=gain)
        if cached and (found := character.stats._modifiers.get(stat_mod.storage_key(), None)) is not None:
            return found
        character.gather_stat_modifiers(stat_mod)
        character.stats._modifiers[stat_mod.storage_key()] = stat_mod
        return stat_mod

    def calculate_value(self, character: Character) -> float:
        """
        Calculate the current value of this stat for the given character. This is after applying all modifiers.
        """

        base = self.get_base(character)

        if self.flat:
            character.stats._cache[self.storage_key()] = base
            return base

        stat_mod = self.get_stat_modifier(character)

        post = stat_mod.apply(base)
        
        post = max(post, self.min_value) if self.min_value is not None else post
        post = min(post, self.max_value) if self.max_value is not None else post

        return post


# Attributes Section
class AttributeDef(StatDef):
    tags: set[str] = {"attribute"}
    min_value: float = 1.0
    max_value: float = 150.0
    min_base: float = 1.0
    max_base: float = 80.0
    default_value: float = 10.0
    category = "attribute"

class Strength(AttributeDef):
    key = "strength"

class Agility(AttributeDef):
    key = "agility"

class Intelligence(AttributeDef):
    key = "intelligence"

class Wisdom(AttributeDef):
    key = "wisdom"

class Constitution(AttributeDef):
    key = "constitution"

class Speed(AttributeDef):
    key = "speed"

_ALL_ATTRIBUTES = [Strength, Agility, Intelligence, Wisdom, Constitution, Speed]

# Vitals
class VitalDef(StatDef):
    tags: set[str] = {"vital"}
    min_value: float = 1.0
    category = "vitals"

class HealthMax(VitalDef):
    key = "health_max"

class StaminaMax(VitalDef):
    key = "stamina_max"

class KiMax(VitalDef):
    key = "ki_max"

class LifeForceMax(VitalDef):
    key = "lifeforce_max"
    depends: set[str] = {"vitals:health_max", "vitals:stamina_max", "vitals:ki_max"}

    def get_base(self, character: Character) -> float:
        deps = self.gather_dependencies(character)
        stamina_max = deps.get("vitals:stamina_max", 0.0)
        ki_max = deps.get("vitals:ki_max", 0.0)

        return (ki_max * 0.5 + stamina_max * 0.5)

_ALL_VITALS = [HealthMax, StaminaMax, KiMax, LifeForceMax]

# Resources
class ResourceDef(StatDef):
    """
    Resources are meters that can be spent/recovered.
    They go from 0 to 100%.
    """
    tags: set[str] = {"resource"}
    min_value: float = 0.0
    max_value: float = 100.0
    default_value: float = 100.0
    flat: bool = True
    category = "resources"

class Health(ResourceDef):
    """
    Health is a little special.
    It starts at 100% and is decreased by the amount of injuries in HeDiffs.
    """
    key = "health"

    def get_base(self, character: Character) -> float:
        # Health is a special case, because we actually want to calculate the hediff injury total.
        total_severity = 0.0
        for k, v in character.hediffs.items():
            if v.is_injury:
                total_severity += v.severity
        
        return 100.0 - total_severity
    
class Stamina(ResourceDef):
    key = "stamina"

class Ki(ResourceDef):
    key = "ki"

class LifeForce(ResourceDef):
    key = "lifeforce"


_ALL_RESOURCES = [Health, Stamina, Ki, LifeForce]

# Other Stats
class Alignment(StatDef):
    key = "alignment"
    tags: set[str] = {"alignment"}
    min_base: float = -1000.0
    max_base: float = 1000.0
    min_value: float = -1000.0
    max_value: float = 1000.0
    default_value: float = 0.0

_ALL_OTHER = [Alignment]

# Advancement
class AdvancementDef(StatDef):
    tags: set[str] = {"advancement"}
    min_value: float = 0.0
    default_value: float = 0.0
    category = "advancement"
    flat: bool = True

class Experience(AdvancementDef):
    tags: set[str] = {"advancement"}
    key = "experience"
    tags: set[str] = {"experience"}
    min_value: float = 0.0
    default_value: float = 0.0

class PracticeSessions(AdvancementDef):
    key = "practice_sessions"
    tags: set[str] = {"advancement"}
    min_value: float = 0.0
    default_value: float = 0.0

class UpgradePoints(AdvancementDef):
    key = "upgrade_points"
    tags: set[str] = {"advancement"}
    min_value: float = 0.0
    default_value: float = 0.0

_ALL_ADVANCEMENT = [Experience, PracticeSessions, UpgradePoints]

# Combat Stats
class CombatStatDef(StatDef):
    tags: set[str] = {"combat"}
    min_value: float = 0.0
    default_value: float = 0.0
    pure_derived: bool = True

class Accuracy(CombatStatDef):
    key = "accuracy"
    tags: set[str] = {"combat"}

class Damage(CombatStatDef):
    key = "damage"
    tags: set[str] = {"combat"}


class Armor(CombatStatDef):
    key = "armor"
    tags: set[str] = {"combat"}


class Parry(CombatStatDef):
    key = "parry"
    tags: set[str] = {"combat"}


class Dodge(CombatStatDef):
    key = "dodge"
    tags: set[str] = {"combat"}


class Block(CombatStatDef):
    key = "block"
    tags: set[str] = {"combat"}


class PerfectDodge(CombatStatDef):
    key = "perfect_dodge"
    tags: set[str] = {"combat"}


class Defense(CombatStatDef):
    key = "defense"
    tags: set[str] = {"combat"}

_ALL_COMBAT = [Accuracy, Damage, Armor, Parry, Dodge, Block, PerfectDodge, Defense]

# Weigh stuff
class WeightDef(StatDef):
    tags: set[str] = {"weight"}
    min_value: float = 0.0
    default_value: float = 0.0
    category = "weight"

class InventoryWeight(WeightDef):
    key = "inventory"
    flat: bool = True
    pure_derived: bool = True

    def get_base(self, character: Character) -> float:
        return sum(item.weight for item in character.iter_inventory())

class EquippedWeight(WeightDef):
    key = "equipped"
    flat: bool = True
    pure_derived: bool = True

    def get_base(self, character: Character) -> float:
        return sum(item.weight for slot, item in character.iter_equipped())

class CarriedWeight(WeightDef):
    key = "carried"
    depends: set[str] = {"weight:inventory", "weight:equipped"}
    flat: bool = True
    pure_derived: bool = True


    def get_base(self, character: Character) -> float:
        deps = self.gather_dependencies(character)
        inventory_weight = deps.get("weight:inventory", 0.0)
        equipped_weight = deps.get("weight:equipped", 0.0)
        return inventory_weight + equipped_weight

class Weight(WeightDef):
    """
    The actual character's weight. Just their body. This can be affected by transformations and things
    so it's not flat.
    """
    key = "weight"


class TotalWeight(WeightDef):
    """
    The total weight the character is carrying, including their body and everything they have on them.
    """
    key = "total"
    depends: set[str] = {"weight:weight", "weight:carried"}
    flat: bool = True
    pure_derived: bool = True

    def get_base(self, character: Character) -> float:
        deps = self.gather_dependencies(character)
        body_weight = deps.get("weight:weight", 0.0)
        carry_weight = deps.get("weight:carried", 0.0)
        return body_weight + carry_weight

class CarryCapacity(StatDef):
    key = "carry_capacity"
    tags: set[str] = {"weight"}
    min_value: float = 100.0
    default_value: float = 0.0
    depends: set[str] = {"attributes:strength", "vitals:health_max","weight:weight", "weight:carried"}
    pure_derived: bool = True

    def get_base(self, character: Character) -> float:
        deps = self.gather_dependencies(character)
        body_weight = deps.get("weight:weight", 0.0)
        strength = deps.get("attributes:strength", 0.0)
        health_max = deps.get("vitals:health_max", 0.0)
        return body_weight + 100.0 + strength * 50.0 + health_max / 200.0


class CarryAvailable(StatDef):
    key = "carry_available"
    tags: set[str] = {"weight"}
    min_value: float = 0.0
    default_value: float = 0.0
    depends: set[str] = {"weight:carry_capacity", "weight:carried"}
    flat: bool = True
    pure_derived: bool = True

    def get_base(self, character: Character) -> float:
        deps = self.gather_dependencies(character)
        carry_capacity = deps.get("weight:carry_capacity", 0.0)
        carry_weight = deps.get("weight:carried", 0.0)
        return carry_capacity - carry_weight

_ALL_WEIGHT = [InventoryWeight, EquippedWeight, CarriedWeight, Weight, TotalWeight, CarryCapacity, CarryAvailable]


class SkillDef(StatDef):
    tags: set[str] = {"skill"}
    min_value: float = 0.0
    default_value: float = 0.0
    category = "skills"


class Flex(SkillDef):
    key = "flex"


class Genius(SkillDef):
    key = "genius"


ALL_STATS = _ALL_ATTRIBUTES + _ALL_VITALS + _ALL_RESOURCES + _ALL_OTHER + _ALL_ADVANCEMENT + _ALL_COMBAT + _ALL_WEIGHT