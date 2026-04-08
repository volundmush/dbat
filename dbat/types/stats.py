import typing
import dbat
from pydantic import BaseModel, Field, ConfigDict, PrivateAttr
from muplugins.core.db.fields import RichText
from rich.text import Text

class StatModifier:
    def __init__(self, stat_def: StatDef):
        self.stat_def = stat_def

        # Absolute value modifiers. The lowest floor, and lowest ceiling are used to clamp.
        self.pre_floors: list[float] = []
        self.pre_ceilings: list[float] = []

        # Raw value modifiers applied before any multiplication.
        self.pre_modifiers: list[float] = []

        # The additive modifiers will be added to a 1.0. So a modifier of 0.5 would result in a 1.5 multiplier, and a modifier of -0.5 would result in a 0.5 multiplier.
        self.additive_multipliers: list[float] = []

        # After additive, the multiplicative modifiers will be multiplied together.
        # So two modifiers of 0.5 would result in a 0.25 multiplier.
        self.multiplicative_multipliers: list[float] = []

        # Raw value modifiers added after all multiplications.
        self.post_modifiers: list[float] = []

        self.post_floors: list[float] = []
        self.post_ceilings: list[float] = []

    @property
    def key(self) -> str:
        return self.stat_def.key

class StatDef:
    # The unique key for this stat. This is used for targeting with commands and storage in dictionaries.
    key: str = ""

    # Tags used for categorizing and listing/searching.
    tags: set[str] = set()

    # Min and max values. if None, there is no limit.
    min_value: float | None = None
    max_value: float | None = None

    # The default value for if not set.
    default_value: float = 0.0
    description: str = ""

    # a set of other stats which must be calculated first.
    # beware of circular dependencies here.
    depends: set[str] = set()

    # If it's flat, it doesn't bother with the complex custom calcs. the
    # effective is the base.
    flat: bool = False

    # a pure derived stat is never saved to the database.
    pure_derived: bool = False

    def get_base(self, character: "Character") -> float:
        """
        Get the base value of this stat for the given character. This is before applying any modifiers.
        """
        return character.stats.stats.get(self.key, self.default_value)

    def gather_dependencies(self, character: "Character") -> dict[str, float]:
        """
        Gather the dependencies for this stat, in order. This is used for calculating stats that are derived from others.
        """
        out = dict()
        for dep in self.depends:
            out[dep] = character.stats.get_current(dep, character)
        return out

    def set(self, character: "Character", value: float):
        """
        Set the base value of this stat for the given character. This is before applying any modifiers.
        """
        character.stats.stats[self.key] = value
        character.stats.invalidate_cache()
    
    def mod(self, character: "Character", delta: float):
        """
        Modify the base value of this stat for the given character by the given delta. This is before applying any modifiers.
        """
        current = self.get_base(character)
        new_value = current + delta
        self.set(character, new_value)
    
    def gain(self, character: "Character", delta: float):
        """
        Gain this stat by the given delta. This is the preferred way to modify stats, as it can trigger things like level up, etc.
        """
        # By default, gain just mods the stat. But some stats might want to override this to do more complex things.
        self.mod(character, delta)

    def calculate_current(self, character: "Character", cached=True) -> float:
        """
        Calculate the current value of this stat for the given character. This is after applying all modifiers.
        """
        if cached and (found := character.stats._cache.get(self.key, None)) is not None:
            return found

        base = self.get_base(character)

        if self.flat:
            character.stats._cache[self.key] = base
            return base

        stat_mod = StatModifier(self)

        for hediff in character.hediffs.values():
            hediff.apply_stat_modifier(character, stat_mod)
        
        # Apply pre modifiers
        pre = base
        pre += sum(stat_mod.pre_modifiers)
        pre_floor = max(stat_mod.pre_floors) if stat_mod.pre_floors else None
        pre_ceiling = min(stat_mod.pre_ceilings) if stat_mod.pre_ceilings else None
        if pre_floor is not None:
            pre = max(pre, pre_floor)
        if pre_ceiling is not None:
            pre = min(pre, pre_ceiling)

        add_mul = 1.0
        add_mul += sum(stat_mod.additive_modifiers)

        mul_mul = 1.0
        for m in stat_mod.multiplicative_modifiers:
            mul_mul *= m
        
        post = pre * add_mul * mul_mul

        post += sum(stat_mod.post_modifiers)

        post_floor = max(stat_mod.post_floors) if stat_mod.post_floors else None
        post_ceiling = min(stat_mod.post_ceilings) if stat_mod.post_ceilings else None
        if post_floor is not None:
            post = max(post, post_floor)
        if post_ceiling is not None:
            post = min(post, post_ceiling)
        
        post = max(post, self.min_value) if self.min_value is not None else post
        post = min(post, self.max_value) if self.max_value is not None else post

        character.stats._cache[self.key] = post

        return post


# Attributes Section
class AttributeDef(StatDef):
    tags: set[str] = {"attribute"}
    min_value: float = 1.0
    default_value: float = 10.0

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

class HealthMax(VitalDef):
    key = "health_max"

class StaminaMax(VitalDef):
    key = "stamina_max"

class KiMax(VitalDef):
    key = "ki_max"

class LifeForceMax(VitalDef):
    key = "lifeforce_max"
    depends: set[str] = {"health_max", "stamina_max", "ki_max"}

    def get_base(self, character: "Character") -> float:
        deps = self.gather_dependencies(character)
        stamina_max = deps.get("stamina_max", 0.0)
        ki_max = deps.get("ki_max", 0.0)

        return (ki_max * 0.5 + stamina_max * 0.5)

        # TODO: Make these HeDiffs based on Race.
        match character.physiology.race:
            case "android":
                # Androids have no lifeforce!
                return 0.0
            case "demon":
                return (ki_max * 0.5 + stamina_max * 0.5) * 0.75
            case "konatsu":
                return (ki_max * 0.5 + stamina_max * 0.5) * 0.85
            case _:
                return (ki_max * 0.5 + stamina_max * 0.5)

_ALL_VITALS = [HealthMax, StaminaMax, KiMax, LifeForceMax]

# Resources
class ResourceDef(StatDef):
    tags: set[str] = {"resource"}
    min_value: float = 0.0
    max_value: float = 100.0
    default_value: float = 100.0
    flat: bool = True

class Health(ResourceDef):
    key = "health"

    def get_base(self, character: "Character") -> float:
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
    min_value: float = -1000.0
    max_value: float = 1000.0
    default_value: float = 0.0

_ALL_OTHER = [Alignment]

# Advancement
class Experience(StatDef):
    tags: set[str] = {"advancement"}
    key = "experience"
    tags: set[str] = {"experience"}
    min_value: float = 0.0
    default_value: float = 0.0

class PracticeSessions(StatDef):
    key = "practice_sessions"
    tags: set[str] = {"advancement"}
    min_value: float = 0.0
    default_value: float = 0.0

class UpgradePoints(StatDef):
    key = "upgrade_points"
    tags: set[str] = {"advancement"}
    min_value: float = 0.0
    default_value: float = 0.0

_ALL_ADVANCEMENT = [Experience, PracticeSessions, UpgradePoints]

# Gain Section
class GainDef(StatDef):
    tags: set[str] = {"gain"}
    min_value: float = 0.0
    default_value: float = 0.0
    pure_derived: bool = True

class StrengthGain(GainDef):
    key = "strength_gain"


class AgilityGain(GainDef):
    key = "agility_gain"


class IntelligenceGain(GainDef):
    key = "intelligence_gain"


class WisdomGain(GainDef):
    key = "wisdom_gain"


class ConstitutionGain(GainDef):
    key = "constitution_gain"


class SpeedGain(GainDef):
    key = "speed_gain"


class HealthGain(GainDef):
    key = "health_gain"


class StaminaGain(GainDef):
    key = "stamina_gain"


class KiGain(GainDef):
    key = "ki_gain"


class ExperienceGain(GainDef):
    key = "experience_gain"


class SkillGain(GainDef):
    key = "skill_gain"

_ALL_GAINS = [StrengthGain, AgilityGain, IntelligenceGain, WisdomGain, ConstitutionGain, SpeedGain, HealthGain, StaminaGain, KiGain, ExperienceGain, SkillGain]

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
class InventoryWeight(StatDef):
    key = "inventory_weight"
    tags: set[str] = {"weight"}
    min_value: float = 0.0
    default_value: float = 0.0
    flat: bool = True
    pure_derived: bool = True

    def get_base(self, character: "Character") -> float:
        return sum(item.weight for item in character.iter_inventory())

class EquippedWeight(StatDef):
    key = "equipped_weight"
    tags: set[str] = {"weight"}
    min_value: float = 0.0
    default_value: float = 0.0
    flat: bool = True
    pure_derived: bool = True

    def get_base(self, character: "Character") -> float:
        return sum(item.weight for slot, item in character.iter_equipped())

class CarryWeight(StatDef):
    key = "carry_weight"
    tags: set[str] = {"weight"}
    min_value: float = 0.0
    default_value: float = 0.0
    depends: set[str] = {"inventory_weight", "equipped_weight"}
    flat: bool = True
    pure_derived: bool = True


    def get_base(self, character: "Character") -> float:
        deps = self.gather_dependencies(character)
        inventory_weight = deps.get("inventory_weight", 0.0)
        equipped_weight = deps.get("equipped_weight", 0.0)
        return inventory_weight + equipped_weight

class Weight(StatDef):
    """
    The actual character's weight. Just their body. This can be affected by transformations and things
    so it's not flat.
    """
    key = "weight"
    tags: set[str] = {"weight"}
    min_value: float = 0.0
    default_value: float = 0.0


class TotalWeight(StatDef):
    """
    The total weight the character is carrying, including their body and everything they have on them.
    """
    key = "total_weight"
    tags: set[str] = {"weight"}
    min_value: float = 0.0
    default_value: float = 0.0
    depends: set[str] = {"weight", "carry_weight"}
    flat: bool = True
    pure_derived: bool = True

    def get_base(self, character: "Character") -> float:
        deps = self.gather_dependencies(character)
        body_weight = deps.get("weight", 0.0)
        carry_weight = deps.get("carry_weight", 0.0)
        return body_weight + carry_weight

class CarryCapacity(StatDef):
    key = "carry_capacity"
    tags: set[str] = {"weight"}
    min_value: float = 100.0
    default_value: float = 0.0
    depends: set[str] = {"weight", "strength", "health_max"}
    pure_derived: bool = True

    def get_base(self, character: "Character") -> float:
        deps = self.gather_dependencies(character)
        body_weight = deps.get("weight", 0.0)
        strength = deps.get("strength", 0.0)
        health_max = deps.get("health_max", 0.0)
        return body_weight + 100.0 + strength * 50.0 + health_max / 200.0


class CarryAvailable(StatDef):
    key = "carry_available"
    tags: set[str] = {"weight"}
    min_value: float = 0.0
    default_value: float = 0.0
    depends: set[str] = {"carry_capacity", "carry_weight"}
    flat: bool = True
    pure_derived: bool = True

    def get_base(self, character: "Character") -> float:
        deps = self.gather_dependencies(character)
        carry_capacity = deps.get("carry_capacity", 0.0)
        carry_weight = deps.get("carry_weight", 0.0)
        return carry_capacity - carry_weight

_ALL_WEIGHT = [InventoryWeight, EquippedWeight, CarryWeight, Weight, TotalWeight, CarryCapacity, CarryAvailable]

ALL_STATS = _ALL_ATTRIBUTES + _ALL_VITALS + _ALL_RESOURCES + _ALL_OTHER + _ALL_ADVANCEMENT + _ALL_GAINS + _ALL_COMBAT