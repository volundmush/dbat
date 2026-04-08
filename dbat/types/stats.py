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
    min_base: float = 0.0
    max_base: float = 100.0
    min_value: float = 0.0
    max_value: float = 105.0
    default_value: float = 0.0
    category = "skills"
    skill_flags: set[str] = set()


class Flex(SkillDef):
    key = "flex"
    skill_flags = {"cha_mod", "need_train"}


class Genius(SkillDef):
    key = "genius"
    skill_flags = {"int_mod", "need_train"}


class Enlighten(SkillDef):
    key = "enlighten"
    skill_flags = {"int_mod", "need_train"}


class ToughSkin(SkillDef):
    key = "tough_skin"
    skill_flags = {"str_mod", "armor_all"}


class Kaioken(SkillDef):
    key = "kaioken"
    skill_flags = {"int_mod", "need_train"}


class Bless(SkillDef):
    key = "bless"
    skill_flags = {"wis_mod", "need_train"}


class Curse(SkillDef):
    key = "curse"
    skill_flags = {"wis_mod", "need_train"}


class Poison(SkillDef):
    key = "poison"
    skill_flags = {"wis_mod", "need_train"}


class Vigor(SkillDef):
    key = "vigor"
    skill_flags = {"wis_mod", "need_train"}


class Pose(SkillDef):
    key = "special_pose"
    skill_flags = {"int_mod", "need_train"}


class Hasshuken(SkillDef):
    key = "hasshuken"
    skill_flags = {"int_mod", "need_train"}


class Gardening(SkillDef):
    key = "gardening"
    skill_flags = {"int_mod", "need_train"}


class Extract(SkillDef):
    key = "extract"
    skill_flags = {"int_mod", "need_train"}


class Runic(SkillDef):
    key = "runic"
    skill_flags = {"int_mod", "need_train"}


class Commune(SkillDef):
    key = "commune"
    skill_flags = {"int_mod", "need_train"}


class SolarFlare(SkillDef):
    key = "solar_flare"
    skill_flags = {"int_mod", "need_train"}


class Might(SkillDef):
    key = "might"
    skill_flags = {"str_mod", "need_train"}


class Balance(SkillDef):
    key = "balance"
    skill_flags = {"dex_mod", "armor_all"}


class Build(SkillDef):
    key = "build"
    skill_flags = {"int_mod", "need_train"}


class Concentration(SkillDef):
    key = "concentration"
    skill_flags = {"con_mod"}


class Spot(SkillDef):
    key = "spot"
    skill_flags = {"wis_mod"}


class FirstAid(SkillDef):
    key = "first_aid"
    skill_flags = {"wis_mod", "need_train"}


class Disguise(SkillDef):
    key = "disguise"
    skill_flags = {"cha_mod"}


class EscapeArtist(SkillDef):
    key = "escape"
    skill_flags = {"dex_mod", "armor_all"}


class Appraise(SkillDef):
    key = "appraise"
    skill_flags = {"int_mod"}


class Heal(SkillDef):
    key = "heal"
    skill_flags = {"wis_mod", "armor_bad"}


class Forgery(SkillDef):
    key = "forgery"
    skill_flags = {"int_mod"}


class Hide(SkillDef):
    key = "hide"
    skill_flags = {"dex_mod", "armor_all"}


class Listen(SkillDef):
    key = "listen"
    skill_flags = {"wis_mod"}


class Eavesdrop(SkillDef):
    key = "eavesdrop"
    skill_flags = {"int_mod"}


class Cure(SkillDef):
    key = "cure_poison"
    skill_flags = {"wis_mod", "need_train"}


class OpenLock(SkillDef):
    key = "open_lock"
    skill_flags = {"dex_mod", "need_train", "armor_bad"}


class Regenerate(SkillDef):
    key = "regenerate"
    skill_flags = {"con_mod", "need_train"}


class KeenSight(SkillDef):
    key = "keen_sight"
    skill_flags = {"int_mod", "need_train"}


class Search(SkillDef):
    key = "search"
    skill_flags = {"int_mod"}


class MoveSilently(SkillDef):
    key = "move_silently"
    skill_flags = {"dex_mod", "armor_all"}


class Absorb(SkillDef):
    key = "absorb"
    skill_flags = {"int_mod", "need_train"}


class SleightOfHand(SkillDef):
    key = "sleight_of_hand"
    skill_flags = {"dex_mod", "armor_all"}


class Ingest(SkillDef):
    key = "ingest"
    skill_flags = {"str_mod", "need_train"}


class Repair(SkillDef):
    key = "fix"
    skill_flags = {"int_mod", "need_train"}


class Sense(SkillDef):
    key = "sense"
    skill_flags = {"int_mod", "need_train"}


class Survival(SkillDef):
    key = "survival"
    skill_flags = {"wis_mod", "need_train"}


class Yoik(SkillDef):
    key = "yoikominminken"
    skill_flags = {"int_mod", "need_train"}


class Create(SkillDef):
    key = "create"
    skill_flags = {"int_mod", "need_train"}


class Spit(SkillDef):
    key = "stone_spit"
    skill_flags = {"int_mod", "need_train"}


class Potential(SkillDef):
    key = "potential_release"
    skill_flags = {"int_mod", "need_train"}


class Telepathy(SkillDef):
    key = "telepathy"
    skill_flags = {"int_mod", "need_train"}


class Focus(SkillDef):
    key = "focus"
    skill_flags = {"int_mod", "need_train"}


class InstantTransmission(SkillDef):
    key = "instant_transmission"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class Sword(SkillDef):
    key = "sword"
    skill_flags = {"int_mod", "need_train"}


class Dagger(SkillDef):
    key = "dagger"
    skill_flags = {"int_mod", "need_train"}


class Club(SkillDef):
    key = "club"
    skill_flags = {"int_mod", "need_train"}


class Spear(SkillDef):
    key = "spear"
    skill_flags = {"int_mod", "need_train"}


class Gun(SkillDef):
    key = "gun"
    skill_flags = {"int_mod", "need_train"}


class Brawl(SkillDef):
    key = "brawl"
    skill_flags = {"int_mod", "need_train"}


class Dodge(SkillDef):
    key = "dodge"
    skill_flags = {"cha_mod", "need_train"}


class Parry(SkillDef):
    key = "parry"
    skill_flags = {"dex_mod", "need_train"}


class Block(SkillDef):
    key = "block"
    skill_flags = {"dex_mod", "need_train"}


class Zanzoken(SkillDef):
    key = "zanzoken"
    skill_flags = {"int_mod", "need_train"}


class Barrier(SkillDef):
    key = "barrier"
    skill_flags = {"int_mod", "need_train"}


class Throw(SkillDef):
    key = "throw"
    skill_flags = {"dex_mod", "need_train"}


class Punch(SkillDef):
    key = "punch"
    skill_flags = {"str_mod", "need_train"}


class Kick(SkillDef):
    key = "kick"
    skill_flags = {"str_mod", "need_train"}


class Elbow(SkillDef):
    key = "elbow"
    skill_flags = {"str_mod", "need_train"}


class Knee(SkillDef):
    key = "knee"
    skill_flags = {"str_mod", "need_train"}


class Roundhouse(SkillDef):
    key = "roundhouse"
    skill_flags = {"str_mod", "need_train"}


class Uppercut(SkillDef):
    key = "uppercut"
    skill_flags = {"str_mod", "need_train"}


class Slam(SkillDef):
    key = "slam"
    skill_flags = {"str_mod", "need_train", "tier_2"}


class Heeldrop(SkillDef):
    key = "heeldrop"
    skill_flags = {"str_mod", "need_train", "tier_2"}


class KiBall(SkillDef):
    key = "kiball"
    skill_flags = {"int_mod", "need_train"}


class KiBlast(SkillDef):
    key = "kiblast"
    skill_flags = {"int_mod", "need_train"}


class Beam(SkillDef):
    key = "beam"
    skill_flags = {"int_mod", "need_train"}


class Tsuihidan(SkillDef):
    key = "tsuihidan"
    skill_flags = {"int_mod", "need_train"}


class Shogekiha(SkillDef):
    key = "shogekiha"
    skill_flags = {"int_mod", "need_train"}


class Renzo(SkillDef):
    key = "renzokou_energy_dan"
    skill_flags = {"int_mod", "need_train"}


class Masenko(SkillDef):
    key = "masenko"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class Dodonpa(SkillDef):
    key = "dodonpa"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class GalikGun(SkillDef):
    key = "galik_gun"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class Kamehameha(SkillDef):
    key = "kamehameha"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class DeathBeam(SkillDef):
    key = "deathbeam"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class Eraser(SkillDef):
    key = "eraser_cannon"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class TSlash(SkillDef):
    key = "twin_slash"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class PsyBlast(SkillDef):
    key = "psychic_blast"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class Honoo(SkillDef):
    key = "honoo"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class DualBeam(SkillDef):
    key = "dual_beam"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class Rogafufuken(SkillDef):
    key = "rogafufuken"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class Bakuhatsuha(SkillDef):
    key = "bakuhatsuha"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class Kienzan(SkillDef):
    key = "kienzan"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class TriBeam(SkillDef):
    key = "tribeam"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class SBC(SkillDef):
    key = "special_beam_cannon"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class FinalFlash(SkillDef):
    key = "final_flash"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class Crusher(SkillDef):
    key = "crusher_ball"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class DdSlash(SkillDef):
    key = "darkness_dragon_slash"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class PBarrage(SkillDef):
    key = "psychic_barrage"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class HellFlash(SkillDef):
    key = "hell_flash"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class HellSpear(SkillDef):
    key = "hell_spear_blast"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class Kakusanha(SkillDef):
    key = "kakusanha"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class Scatter(SkillDef):
    key = "scatter_shot"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class BigBang(SkillDef):
    key = "big_bang"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class PSlash(SkillDef):
    key = "phoenix_slash"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class DeathBall(SkillDef):
    key = "deathball"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class SpiritBall(SkillDef):
    key = "spirit_ball"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class GenkiDama(SkillDef):
    key = "genki_dama"
    skill_flags = {"int_mod", "need_train", "tier_5"}


class Genocide(SkillDef):
    key = "genocide"
    skill_flags = {"int_mod", "need_train", "tier_5"}


class DualWield(SkillDef):
    key = "dual_wield"
    skill_flags = {"int_mod", "need_train"}


class TwoHand(SkillDef):
    key = "twohand"
    skill_flags = {"int_mod", "need_train"}


class Style(SkillDef):
    key = "fighting_arts"
    skill_flags = {"int_mod"}


class Kura(SkillDef):
    key = "kuraiiro_seiki"
    skill_flags = {"int_mod", "need_train"}


class TailWhip(SkillDef):
    key = "tailwhip"
    skill_flags = {"int_mod", "need_train", "tier_1"}


class Kousengan(SkillDef):
    key = "kousengan"
    skill_flags = {"int_mod", "need_train", "tier_1"}


class Taisha(SkillDef):
    key = "taisha_reiki"
    skill_flags = {"int_mod", "need_train"}


class Paralyze(SkillDef):
    key = "paralyze"
    skill_flags = {"int_mod", "need_train"}


class Infuse(SkillDef):
    key = "infuse"
    skill_flags = {"int_mod", "need_train"}


class Roll(SkillDef):
    key = "roll"
    skill_flags = {"int_mod", "need_train"}


class Trip(SkillDef):
    key = "trip"
    skill_flags = {"int_mod", "need_train"}


class Grapple(SkillDef):
    key = "grapple"
    skill_flags = {"int_mod", "need_train"}


class WSpike(SkillDef):
    key = "water_spikes"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class SelfD(SkillDef):
    key = "self_destruct"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class Spiral(SkillDef):
    key = "spiral_comet"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class Breaker(SkillDef):
    key = "star_breaker"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class Mimic(SkillDef):
    key = "mimic"
    skill_flags = {"int_mod", "need_train"}


class WRazor(SkillDef):
    key = "water_razor"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class Koteiru(SkillDef):
    key = "koteiru_bakuha"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class Dimizu(SkillDef):
    key = "dimizu_toride"
    skill_flags = {"int_mod", "need_train"}


class HyogaKabe(SkillDef):
    key = "hyoga_kabe"
    skill_flags = {"int_mod", "need_train"}


class Wellspring(SkillDef):
    key = "wellspring"
    skill_flags = {"int_mod", "need_train"}


class AquaBarrier(SkillDef):
    key = "aqua_barrier"
    skill_flags = {"int_mod", "need_train"}


class Warp(SkillDef):
    key = "warp_pool"
    skill_flags = {"int_mod", "need_train"}


class HSpiral(SkillDef):
    key = "hell_spiral"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class Armor(SkillDef):
    key = "nanite_armor"
    skill_flags = {"int_mod", "need_train"}


class FireShield(SkillDef):
    key = "fireshield"
    skill_flags = {"int_mod", "need_train"}


class Cooking(SkillDef):
    key = "cooking"
    skill_flags = {"int_mod", "need_train"}


class Seishou(SkillDef):
    key = "seishou_enko"
    skill_flags = {"int_mod", "need_train", "tier_2"}


class Silk(SkillDef):
    key = "silk"
    skill_flags = {"int_mod", "need_train"}


class Bash(SkillDef):
    key = "bash"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class Headbutt(SkillDef):
    key = "headbutt"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class Ensnare(SkillDef):
    key = "ensnare"
    skill_flags = {"int_mod", "need_train"}


class StarNova(SkillDef):
    key = "starnova"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class Malice(SkillDef):
    key = "malice_breaker"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class Zen(SkillDef):
    key = "zen_blade_strike"
    skill_flags = {"int_mod", "need_train", "tier_3"}


class Sunder(SkillDef):
    key = "sundering_force"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class Wither(SkillDef):
    key = "wither"
    skill_flags = {"int_mod", "need_train"}


class Metamorph(SkillDef):
    key = "dark_metamorphosis"
    skill_flags = {"int_mod", "need_train"}


class Hayasa(SkillDef):
    key = "hayasa"
    skill_flags = {"int_mod", "need_train"}


class Energize(SkillDef):
    key = "energize_throwing"
    skill_flags = {"int_mod", "need_train"}


class Pursuit(SkillDef):
    key = "pursuit"
    skill_flags = {"int_mod", "need_train"}


class HealGlow(SkillDef):
    key = "healing_glow"
    skill_flags = {"int_mod", "need_train"}


class Handling(SkillDef):
    key = "handling"
    skill_flags = {"int_mod", "need_train"}


class MysticMusic(SkillDef):
    key = "mystic_music"
    skill_flags = {"int_mod", "need_train"}


class LightGrenade(SkillDef):
    key = "light_grenade"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class Multiform(SkillDef):
    key = "multiform"
    skill_flags = {"int_mod", "need_train", "tier_1"}


class SpiritControl(SkillDef):
    key = "spirit_control"
    skill_flags = {"int_mod", "need_train", "tier_1"}


class Balefire(SkillDef):
    key = "balefire"
    skill_flags = {"int_mod", "need_train", "tier_4"}


class BlessedHammer(SkillDef):
    key = "blessed_hammer"
    skill_flags = {"int_mod", "need_train", "tier_1"}


_ALL_SKILLS = [
    Flex, Genius, Enlighten, ToughSkin, Kaioken, Bless, Curse, Poison, Vigor, Pose, Hasshuken,
    Gardening, Extract, Runic, Commune, SolarFlare, Might, Balance, Build, Concentration, Spot,
    FirstAid, Disguise, EscapeArtist, Appraise, Heal, Forgery, Hide, Listen, Eavesdrop, Cure,
    OpenLock, Regenerate, KeenSight, Search, MoveSilently, Absorb, SleightOfHand, Ingest, Repair,
    Sense, Survival, Yoik, Create, Spit, Potential, Telepathy, Focus, InstantTransmission,
    Sword, Dagger, Club, Spear, Gun, Brawl,
    Dodge, Parry, Block, Zanzoken, Barrier,
    Throw, Punch, Kick, Elbow, Knee, Roundhouse, Uppercut, Slam, Heeldrop, KiBall, KiBlast, Beam,
    Tsuihidan, Shogekiha, Renzo, Masenko, Dodonpa, GalikGun, Kamehameha, DeathBeam, Eraser,
    TSlash, PsyBlast, Honoo, DualBeam, Rogafufuken, Bakuhatsuha, Kienzan, TriBeam, SBC,
    FinalFlash, Crusher, DdSlash, PBarrage, HellFlash, HellSpear, Kakusanha, Scatter, BigBang,
    PSlash, DeathBall, SpiritBall, GenkiDama, Genocide, DualWield, TwoHand, Style, Kura,
    TailWhip, Kousengan, Taisha, Paralyze, Infuse, Roll, Trip, Grapple, WSpike, SelfD, Spiral,
    Breaker, Mimic, WRazor, Koteiru, Dimizu, HyogaKabe, Wellspring, AquaBarrier, Warp, HSpiral,
    Armor, FireShield, Cooking, Seishou, Silk, Bash, Headbutt, Ensnare, StarNova, Malice, Zen,
    Sunder, Wither, Metamorph, Hayasa, Energize, Pursuit, HealGlow, Handling, MysticMusic,
    LightGrenade, Multiform, SpiritControl, Balefire, BlessedHammer,
]

ALL_STATS = _ALL_ATTRIBUTES + _ALL_VITALS + _ALL_RESOURCES + _ALL_OTHER + _ALL_ADVANCEMENT + _ALL_COMBAT + _ALL_WEIGHT + _ALL_SKILLS