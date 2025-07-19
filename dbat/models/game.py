import typing
import pydantic
from enum import IntFlag
from pydantic import BaseModel, Field, ConfigDict, field_serializer
from . import names

class MobSpecialData(BaseModel):
    attack_type: int = 0
    damnodice: int = 0
    damsizedice: int = 0
    newitem: bool = False
    default_pos: int = 8

class TimeData(BaseModel):
    birth: int = 0
    created: int = 0
    maxage: int = 0
    logon: int = 0
    played: float = 0.0
    seconds_aged: float = 0.0


class TimeInfoData(BaseModel):
    remainder: float = 0.0
    seconds: int = 0
    minutes: int = 0
    hours: int = 0
    day: int = 0
    month: int = 0
    year: int = 0


class WeatherData(BaseModel):
    pressure: int = 0
    change: int = 0
    sky: int = 0
    sunlight: int = 0

class ResetCommandData(BaseModel):
    command: str = ""
    if_flag: bool = False
    arg1: int = 0
    arg2: int = 0
    arg3: int = 0
    arg4: int = 0
    arg5: int = 0
    sarg1: str = ""
    sarg2: str = ""

class ZoneData(BaseModel):
    name: str = ""
    builders: str = 0
    lifespan: int = 0
    age: float = 0.0
    bot: int = 0
    top: int = 0
    reset_mode: int = 0
    number: int = 0
    cmd: typing.List[ResetCommandData] = Field(default_factory=list)
    min_level: int = 0
    max_level: int = 0
    zone_flags: typing.Set[names.ZoneFlag] = Field(default_factory=set)

    @field_serializer("zone_flags")
    def serialize_zone_flags(self, value):
        return [value.name for value in value] if value else []

class AffectData(BaseModel):
    location: int = 0
    modifier: int = 0
    specific: int = 0

class AffectedTypeData(AffectData):
    type: int = 0
    duration: int = 0
    bitvector: int = 0

class AccountData(BaseModel):
    id: int
    name: str
    password: pydantic.SecretStr = pydantic.SecretStr("")
    email: str = ""
    created: int = 0
    last_login: int = 0
    last_logout: int = 0
    last_change_password: int = 0
    playtime: float = 0.0
    disabled_reason: str = ""
    disabled_until: int = 0
    admin_level: int = 0
    rpp: int = 0
    slots: int = 3
    customs: typing.List[str] = Field(default_factory=list)
    characters: typing.List[int] = Field(default_factory=list)

class AliasData(BaseModel):
    name: str = ""
    replacement: str = ""
    type: int = 0

class PlayerData(BaseModel):
    id: int
    name: str
    aliases: typing.List[AliasData] = Field(default_factory=list)
    sense_player: typing.Set[int] = Field(default_factory=set)
    sense_memory: typing.Set[int] = Field(default_factory=set)
    dub_names: typing.Dict[int, str] = Field(default_factory=dict)
    color_choices: typing.List[str] = Field(default_factory=list)


class TrigVarData(BaseModel):
    name: str = ""
    value: str = ""
    context: int = 0

class TrigData(BaseModel):
    vn: int = -1
    name: str = ""
    attach_type: int = 0
    data_type: int = 0
    trigger_type: int = 0
    depth: int = 0
    loops: int = 0
    waiting: float = 0.0
    curr_state: int = 0
    var_list: typing.List[TrigVarData] = Field(default_factory=list)
    narg: int = 0
    arglist: str = ""
    order: int = 0
    active: bool = False
    id: int = -1
    generation: int = 0

class ShopBuyData(BaseModel):
    type: int
    keywords: str

class _Picky(BaseModel):
    not_alignment: typing.Set[names.Align] = Field(default_factory=set)
    only_alignment: typing.Set[names.Align] = Field(default_factory=set)
    only_sensei: typing.Set[names.Sensei] = Field(default_factory=set)
    not_sensei: typing.Set[names.Sensei] = Field(default_factory=set)
    only_race: typing.Set[names.Race] = Field(default_factory=set)
    not_race: typing.Set[names.Race] = Field(default_factory=set)

    @field_serializer("not_alignment")
    def serialize_not_alignment(self, value):
        return [value.name for value in value] if value else []
    
    @field_serializer("only_alignment")
    def serialize_only_alignment(self, value):
        return [value.name for value in value] if value else []
    
    @field_serializer("only_sensei")
    def serialize_only_sensei(self, value):
        return [value.name for value in value] if value else []
    
    @field_serializer("not_sensei")
    def serialize_not_sensei(self, value):
        return [value.name for value in value] if value else []

    @field_serializer("only_race")
    def serialize_only_race(self, value):
        return [value.name for value in value] if value else []

    @field_serializer("not_race")
    def serialize_not_race(self, value):
        return [value.name for value in value] if value else []

class _Org(_Picky):
    vnum: int = -1
    keeper: int = -1
    

class ShopData(_Org):
    producing: typing.List[int] = Field(default_factory=list)
    profit_buy: float = 1.0
    profit_sell: float = 1.0
    type: typing.List[ShopBuyData] = Field(default_factory=list)
    no_such_item1: str = ""
    no_such_item2: str = ""
    missing_cash1: str = ""
    missing_cash2: str = ""
    do_not_buy: str = ""
    message_buy: str = ""
    message_sell: str = ""
    temper1: int = 0
    shop_flags: typing.Set[names.ShopFlag] = Field(default_factory=set)
    in_room: typing.Set[int] = Field(default_factory=set)
    open1: int = 0
    close1: int = 0
    open2: int = 0
    close2: int = 0
    bankAccount: int = 0
    lastsort: int = 0

    @field_serializer("shop_flags")
    def serialize_shop_flags(self, value):
        return [value.name for value in value] if value else []

class GuildData(_Org):
    skills: typing.Set[names.Skill] = Field(default_factory=set)
    charge: float = 1.0
    no_such_skill: str = ""
    not_enough_gold: str = ""
    minlvl: int = 0
    open: int = 0
    close: int = 0

    @field_serializer("skills")
    def serialize_skills(self, value):
        return [skill.name for skill in value] if value else []

class ExtraDescriptionData(BaseModel):
    keyword: str = ""
    description: str = ""

class UnitData(BaseModel):
    vn: int = -1
    id: int = -1
    generation: int = 0
    zone: int = -1
    name: str = ""
    short_description: str = ""
    look_description: str = ""
    room_description: str = ""
    ex_description: typing.List[ExtraDescriptionData] = Field(default_factory=list)
    proto_script: typing.List[int] = Field(default_factory=list)


class ExitInfo(IntFlag):
    is_door = 1
    closed = 2
    locked = 4
    pickproof = 8
    secret = 16


class RoomDirectionData(BaseModel):
    keyword: str = ""
    general_description: str = ""
    exit_info: ExitInfo = Field(default_factory=lambda: ExitInfo(0))
    key: int = -1
    to_room: int = -1
    dclock: int = 0
    dchide: int = 0
    dcskill: int = 0
    dcmove: int = 0
    failsavetype: int = 0
    dcfailsave: int = 0
    failroom: int = -1
    totalfailroom: int = -1

class RoomData(UnitData):
    sector_type: names.SectorType = names.SectorType.inside
    room_flags: typing.Set[names.RoomFlag] = Field(default_factory=set)
    dir_option: typing.List[typing.Tuple[int, RoomDirectionData]] = Field(default_factory=list)
    timed: int = 0
    dmg: int = 0
    geffect: int = 0

    @field_serializer("sector_type")
    def serialize_sector_type(self, value):
        return value.name if value else None

    @field_serializer("room_flags")
    def serialize_room_flags(self, value):
        return [flag.name for flag in value] if value else []
    
class ThingData(UnitData):
    in_room: int = -1
    affect_flags: typing.Set[names.AffectFlag] = Field(default_factory=set)

    @field_serializer("affect_flags")
    def serialize_affect_flags(self, value):
        return [flag.name for flag in value] if value else []

class ObjectData(ThingData, _Picky):
    room_loaded: int = -1
    value: typing.Dict[str, int] = Field(default_factory=dict)
    dvalue: typing.Dict[int, float] = Field(default_factory=dict)
    type_flag: names.ItemType = names.ItemType.unknown
    level: int = 0
    wear_flags: typing.Set[names.WearFlag] = Field(default_factory=set)
    item_flags: typing.Set[names.ItemFlag] = Field(default_factory=set)
    weight: float = 0.0
    cost: int = 0
    cost_per_day: int = 0
    timer: int = 0
    size: int = 4
    affected: typing.List[AffectedTypeData] = Field(default_factory=set)

    @field_serializer("type_flag")
    def serialize_type_flag(self, value):
        return value.name
    
    @field_serializer("wear_flags")
    def serialize_wear_flags(self, value):
        return [flag.name for flag in value] if value else []
    
    @field_serializer("item_flags")
    def serialize_item_flags(self, value):
        return [flag.name for flag in value] if value else []

class SkillData(BaseModel):
    level: int = 0
    perfs: int = 0

class TransData(BaseModel):
    description: str = ""
    time_spent_in_form: float = 0.0
    grade: int = 1
    visible: bool = True
    limit_broken: bool = True
    unlocked: bool = False
    vars: typing.List[typing.Tuple[int, float]] = Field(default_factory=list)
    blutz: float = 0.0


class CharData(ThingData):
    title: str = ""
    race: names.Race = names.Race.spirit
    sensei: names.Sensei = names.Sensei.commoner
    
    trains: typing.Dict[names.AttributeTrain, int] = Field(default_factory=dict)
    attributes: typing.Dict[names.Attribute, int] = Field(default_factory=dict)
    moneys: typing.Dict[names.Money, int] = Field(default_factory=dict)
    aligns: typing.Dict[names.Align, int] = Field(default_factory=dict)
    appearances: typing.Dict[names.Appearance, int] = Field(default_factory=dict)
    vitals: typing.Dict[names.Vital, int] = Field(default_factory=dict)
    nums: typing.Dict[names.Num, int] = Field(default_factory=dict)
    stats: typing.Dict[names.Stat, int] = Field(default_factory=dict)
    dims: typing.Dict[names.Dim, float] = Field(default_factory=dict)
    character_flags: typing.Set[names.CharacterFlag] = Field(default_factory=set)
    mob_flags: typing.Set[names.MobFlag] = Field(default_factory=set)
    player_flags: typing.Set[names.PlayerFlag] = Field(default_factory=set)
    pref_flags: typing.Set[names.PrefFlag] = Field(default_factory=set)
    bodyparts: typing.Set[int] = Field(default_factory=set)
    armor: int = 0
    damage_mod: int = 0
    admin_flags: typing.Set[names.AdminFlag] = Field(default_factory=set)
    mob_specials: MobSpecialData = Field(default_factory=MobSpecialData)
    transforms: typing.Dict[names.Form, TransData] = Field(default_factory=dict)

    @field_serializer("race")
    def serialize_race(self, value):
        return value.name
    
    @field_serializer("sensei")
    def serialize_sensei(self, value):
        return value.name
    
    @field_serializer("trains")
    def serialize_trains(self, value):
        return {train.name: level for train, level in value.items()} if value else {}
    
    @field_serializer("attributes")
    def serialize_attributes(self, value):
        return {attr.name: level for attr, level in value.items()} if value else {}
    
    @field_serializer("moneys")
    def serialize_moneys(self, value):
        return {money.name: amount for money, amount in value.items()} if value else {}
    
    @field_serializer("aligns")
    def serialize_aligns(self, value):
        return {align.name: amount for align, amount in value.items()} if value else {}
    
    @field_serializer("appearances")
    def serialize_appearances(self, value):
        return {appearance.name: level for appearance, level in value.items()} if value else {}
    
    @field_serializer("vitals")
    def serialize_vitals(self, value):
        return {vital.name: level for vital, level in value.items()} if value else {}
    
    @field_serializer("nums")
    def serialize_nums(self, value):
        return {num.name: level for num, level in value.items()} if value else {}
    
    @field_serializer("stats")
    def serialize_stats(self, value):
        return {stat.name: level for stat, level in value.items()} if value else {}
    
    @field_serializer("dims")
    def serialize_dims(self, value):
        return {dim.name: level for dim, level in value.items()} if value else {}
    
    @field_serializer("character_flags")
    def serialize_character_flags(self, value): 
        return [flag.name for flag in value] if value else []
    
    @field_serializer("mob_flags")
    def serialize_mob_flags(self, value):
        return [flag.name for flag in value] if value else []
    
    @field_serializer("player_flags")
    def serialize_player_flags(self, value):
        return [flag.name for flag in value] if value else []
    
    @field_serializer("pref_flags")
    def serialize_pref_flags(self, value):
        return [flag.name for flag in value] if value else []
    
    @field_serializer("admin_flags")
    def serialize_admin_flags(self, value):
        return [flag.name for flag in value] if value else []
    
    @field_serializer("bodyparts")
    def serialize_bodyparts(self, value):
        return [part for part in value] if value else []
    
    @field_serializer("admin_flags")
    def serialize_admin_flags(self, value):
        return [flag.name for flag in value] if value else []

    @field_serializer("transforms")
    def serialize_transforms(self, value):
        return {form.name: data for form, data in value.items()} if value else {}

class ChargenData(BaseModel):
    name: str | None = None
    race: names.Race | None = None
    subrace: names.SubRace | None = None
    sex: names.Sex | None = None
    sensei: names.Sensei | None = None
    mutations: typing.Set[names.Mutation] = Field(default_factory=set)
    bio_genomes: typing.Set[names.BioGenome] = Field(default_factory=set)
    keep_skills: bool = True
    align: int = 0
    
    @field_serializer("race")
    def serialize_race(self, value):
        return value.name if value else None

    @field_serializer("subrace")
    def serialize_subrace(self, value):
        return value.name if value else None
    
    @field_serializer("sex")
    def serialize_sex(self, value):
        return value.name if value else None
    
    @field_serializer("sensei")
    def serialize_sensei(self, value):
        return value.name if value else None
    
    @field_serializer("mutations")
    def serialize_mutations(self, value):
        return [mutation.name for mutation in value] if value else []
    
    @field_serializer("bio_genomes")
    def serialize_bio_genomes(self, value):
        return [genome.name for genome in value] if value else []

    def available_races(self) -> list[names.Race]:
        return [race for race in names.Race if race.name not in ("spirit", "animal", "saiba", "serpent", "ogre", "yardratian", "dragon", "mechanical")]

    def available_subraces(self) -> list[names.SubRace]:
        if self.race == names.Race.android:
            return [s for s in names.SubRace]
        return list()

    def available_senseis(self) -> list[names.Sensei]:
        senseis = [sen for sen in names.Sensei if sen.name not in ("commoner", "sixteen", "jinto", "tsuna", "kurzak", "dabura")]
        match self.race:
            case names.Race.android:
                senseis = [names.Sensei.sixteen]
            case names.Race.kanassan:
                senseis.append(names.Sensei.tsuna)
            case names.Race.hoshijin:
                senseis.append(names.Sensei.jinto)
            case names.Race.arlian:
                senseis.append(names.Sensei.kurzak)
            case names.Race.demon:
                senseis.append(names.Sensei.dabura)
        return senseis

    def available_sexes(self) -> list[names.Sex]:
        match self.race:
            case names.Race.namekian:
                return [names.Sex.neutral]
            case names.Race.android | names.Race.bio_android:
                return [e for e in names.Sex]
            case _:
                return [names.Sex.male, names.Sex.female]

    def check(self) -> bool:
        if not self.name:
            raise ValueError("Name is required.")
        if len(self.name) < 3 or len(self.name) > 20:
            raise ValueError("Name must be between 3 and 20 characters long.")
        if not self.name.isalnum():
            raise ValueError("Name must be alphanumeric.")
        if " " in self.name:
            raise ValueError("Name cannot contain spaces.")
        if self.race is None:
            raise ValueError("Race is required.")
        if self.race == "android" and self.subrace is None:
            raise ValueError("SubRace is required.")
        if self.sex is None:
            raise ValueError("Sex is required.")
        if self.sensei is None:
            raise ValueError("Sensei is required.")
        match self.race:
            case names.Race.android:
                if self.subrace is None:
                    raise ValueError("SubRace is required for androids.")
            case names.Race.mutant:
                if len(self.mutations) != 2:
                    raise ValueError("2 Mutations are required for mutants.")
            case names.Race.bio_android:
                if len(self.bio_genomes) != 2:
                    raise ValueError("2 Bio Genomes are required for bio androids.")
        if self.align < -1000 or self.align > 1000:
            raise ValueError("Align must be between -1000 and 1000.")
        return True

class HelpData(BaseModel):
    index: str = ""
    keywords: str = ""
    entry: str = ""
    duplicate: int = 0
    min_level: int = 0