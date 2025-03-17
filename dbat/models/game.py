import typing
from pydantic import BaseModel, Field

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
    secondsAged: float = 0.0


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
    zone_flags: typing.Set[int] = Field(default_factory=set)

class AffectData(BaseModel):
    location: int = 0
    modifier: int = 0
    specific: int = 0

class AffectedTypeData(AffectData):
    type: int = 0
    duration: int = 0
    bitvector: int = 0

class AccountData(BaseModel):
    vn: int = -1
    name: str = ""
    passHash: str = ""
    email: str = ""
    created: int = 0
    lastLogin: int = 0
    lastLogout: int = 0
    lastPasswordChanged: int = 0
    totalPlayTime: float = 0.0
    disabledReason: str = ""
    disabledUntil: int = 0
    adminLevel: int = 0
    rpp: int = 0
    slots: int = 3
    customs: typing.List[str] = Field(default_factory=list)
    characters: typing.List[int] = Field(default_factory=list)
    
    @property
    def admin_level(self) -> int:
        return self.adminLevel

    @property
    def id(self):
        return self.vn

class AliasData(BaseModel):
    name: str = ""
    replacement: str = ""
    type: int = 0

class PlayerData(BaseModel):
    id: int
    name: str
    aliases: typing.List[AliasData] = Field(default_factory=list)
    sensePlayer: typing.Set[int] = Field(default_factory=set)
    senseMemory: typing.Set[int] = Field(default_factory=set)
    dubNames: typing.Dict[int, str] = Field(default_factory=dict)
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

class ShopData(BaseModel):
    vnum: int = -1
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
    keeper: int = -1
    bitvector: int = 0
    with_who: typing.Set[int] = Field(default_factory=set)
    in_room: typing.Set[int] = Field(default_factory=set)
    open1: int = 0
    close1: int = 0
    open2: int = 0
    close2: int = 0
    bankAccount: int = 0
    lastsort: int = 0

class GuildData(BaseModel):
    vnum: int = -1
    skills: typing.Set[int] = Field(default_factory=set)
    feats: typing.Set[int] = Field(default_factory=set)
    charge: float = 1.0
    no_such_skill: str = ""
    not_enough_gold: str = ""
    minlvl: int = 0
    gm: int = -1
    with_who: typing.Set[int] = Field(default_factory=set)
    open: int = 0
    close: int = 0

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



class RoomDirectionData(BaseModel):
    keyword: str = ""
    general_description: str = ""
    exit_info: typing.Set[int] = Field(default_factory=set)
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
    sector_type: int = 0
    room_flags: typing.Set[int] = Field(default_factory=set)
    dir_option: typing.List[typing.Tuple[int, RoomDirectionData]] = Field(default_factory=list)
    timed: int = 0
    dmg: int = 0
    geffect: int = 0
    

class ThingData(UnitData):
    in_room: int = -1


class ObjectData(ThingData):
    room_loaded: int = -1
    value: typing.List[typing.Tuple[int, int]] = Field(default_factory=list)
    dvalue: typing.List[typing.Tuple[int, float]] = Field(default_factory=list)
    type_flag: int = 0
    level: int = 0
    onlyAlignLawChaos: typing.Set[int] = Field(default_factory=set)
    onlyAlignGoodEvil: typing.Set[int] = Field(default_factory=set)
    antiAlignLawChaos: typing.Set[int] = Field(default_factory=set)
    antiAlignGoodEvil: typing.Set[int] = Field(default_factory=set)
    onlyClass: typing.Set[int] = Field(default_factory=set)
    antiClass: typing.Set[int] = Field(default_factory=set)
    onlyRace: typing.Set[int] = Field(default_factory=set)
    antiRace: typing.Set[int] = Field(default_factory=set)
    wear_flags: typing.Set[int] = Field(default_factory=set)
    extra_flags: typing.Set[int] = Field(default_factory=set)
    weight: float = 0.0
    bitvector: typing.Set[int] = Field(default_factory=set)
    cost: int = 0
    cost_per_day: int = 0
    timer: int = 0
    size: int = 4
    affected: typing.Set[int] = Field(default_factory=set)


class SkillData(BaseModel):
    level: int = 0
    perfs: int = 0

class TransData(BaseModel):
    description: str = ""
    timeSpentInForm: float = 0.0
    grade: int = 1
    visible: bool = True
    limitBroken: bool = True
    unlocked: bool = False
    vars: typing.List[typing.Tuple[int, float]] = Field(default_factory=list)
    blutz: float = 0.0


class CharData(ThingData):
    title: str = ""
    race: int = 0
    chclass: int = 0
    
    trains: typing.List[typing.Tuple[int, int]] = Field(default_factory=list)
    attributes: typing.List[typing.Tuple[int, int]] = Field(default_factory=list)
    moneys: typing.List[typing.Tuple[int, int]] = Field(default_factory=list)
    aligns: typing.List[typing.Tuple[int, int]] = Field(default_factory=list)
    appearances: typing.List[typing.Tuple[int, int]] = Field(default_factory=list)
    vitals: typing.List[typing.Tuple[int, int]] = Field(default_factory=list)
    nums: typing.List[typing.Tuple[int, int]] = Field(default_factory=list)
    stats: typing.List[typing.Tuple[int, int]] = Field(default_factory=list)
    dims: typing.List[typing.Tuple[int, float]] = Field(default_factory=list)
    mobFlags: typing.Set[int] = Field(default_factory=set)
    playerFlags: typing.Set[int] = Field(default_factory=set)
    pref: typing.Set[int] = Field(default_factory=set)
    bodyparts: typing.Set[int] = Field(default_factory=set)
    affected_by: typing.Set[int] = Field(default_factory=set)
    armor: int = 0
    damage_mod: int = 0
    admFlags: typing.Set[int] = Field(default_factory=set)
    mob_specials: MobSpecialData = Field(default_factory=MobSpecialData)