#!/usr/bin/env python3
from dataclasses import dataclass, field

import random
from collections import defaultdict, deque
import re
from pathlib import Path
from enum import IntEnum

@dataclass(slots=True)
class Exit:
    to_room: int = -1
    keywords: str = ""
    description: str = ""
    exit_info: int = 0
    key: int | None = None
    dclock: int  = 0
    dchide: int = 0

@dataclass(slots=True)
class ExtraDescription:
    keywords: str = ""
    description: str = ""

@dataclass(slots=True)
class Room:
    id: int = -1
    name: str = ""
    description: str = ""
    extra_descriptions: list[ExtraDescription] = field(default_factory=list)
    exits: dict[int, Exit] = field(default_factory=dict)
    room_flags: int = 0
    sector_type: int = 0
    proto_script: list[int] = field(default_factory=list)
    zone: int = -1

@dataclass(slots=True)
class Affected:
    location: int = 0
    modifier: int = 0
    specific: int = 0
    aff_flags: int = 0
    type: int = 0

    def valid(self) -> bool:
        return self.location != 0


@dataclass(slots=True)
class ObjAffected:
    location: int = 0
    specific: int = 0
    modifier: int = 0

@dataclass(slots=True)
class ObjectBase:
    item_type: int = -1
    name: str = ""
    short_description: str = ""
    description: str = ""
    action_description: str = ""
    extra_flags: int = 0
    wear_flags: int = 0
    bitvector: int = 0
    extra_descriptions: list[ExtraDescription] = field(default_factory=list)
    values: dict[int, int] = field(default_factory=dict)

    weight: int = 0
    cost: int = 0
    cost_per_day: int = 0
    level: int = 0
    size: int = 0

    affected: list[ObjAffected] = field(default_factory=list)


@dataclass(slots=True)
class ObjectPrototype(ObjectBase):
    id: int = -1
    proto_script: list[int] = field(default_factory=list)


@dataclass(slots=True)
class Object(ObjectBase):
    id: int = -1
    object_prototype_id: int = -1

    carried_by: Character | None = None
    in_room: Room | None = None
    worn_by: Character | None = None
    worn_on: int = 0

    in_obj: Object | None = None
    contains: list[Object] = field(default_factory=list)


@dataclass(slots=True)
class CharacterBase:
    name: str = ""
    short_descr: str = ""
    long_descr: str = ""
    description: str = ""
    title: str = ""
    size: int = 0
    race: int = 0
    chclass: int = 0
    sex: int = 0
    act: int = 0
    affected_by: int = 0
    level: int = 0

    alignment: int = 0
    
    strength: int = 0
    intel: int = 0
    wis: int = 0
    dex: int = 0
    con: int = 0
    cha: int = 0

    gold: int = 0

    basepl: int = 0
    basest: int = 0
    baseki: int = 0

    damage_mod: int = 0

@dataclass(slots=True)
class CharacterPrototype(CharacterBase):
    id: int = -1
    default_position: int = 0
    proto_script: list[int] = field(default_factory=list)
    

@dataclass(slots=True)
class SkillData:
    level: int = 0
    bonus: int = 0
    perfs: int = 0


@dataclass(slots=True)
class Character(CharacterBase):
    id: int = -1
    affected: int = 0
    skills: dict[int, SkillData] = field(default_factory=dict)
    android_model: str | None = None

    dgscript_variables: dict[str, str] = field(default_factory=dict)
    
    armor: int = 0
    admin_level: int = 0
    absorbs: int = 0
    bank_gold: int = 0

    aura: int = 0
    eye: int = 0
    hairc: int = 0
    hairl: int = 0
    hairs: int = 0
    distfea: int = 0
    skin: int = 0
    feature: str = ""
    voice: str = ""

    blesslvl: int = 0
    bonuses: dict[int, int] = field(default_factory=dict)
    boosts: int = 0

    time_birth: int = 0
    time_created: int = 0
    time_logon: int = 0
    time_maxage: int = 0
    time_played: int = 0

    deathtime: int = 0
    dcount: int = 0
    exp: int = 0
    conditions: dict[int, int] = field(default_factory=dict)

    forgeting: int = 0
    forgetcount: int = 0

    fury: int = 0

    genome: dict[int, int] = field(default_factory=dict)
    hometown: int = 300

    ingestLearned: int = 0
    kaioken: int = 0

    practices: int = 0
    lifeperc: int = 0
    limb_condition: dict[str, int] = field(default_factory=dict)

    lastint: int = 0

    molt_experience: int = 0
    molt_level: int = 0

    majinize: int = 0
    mimic: int = 0

    olc_zone: int = 0
    page_length: int = 0
    phase: int = 0

    poofin: str = ""
    poofout: str = ""

    position: int = 0
    preference: int = 0

    racial_pref: int = 0

    rdisplay: str = ""

    rewtime: int = 0
    radar1: int = 0
    radar2: int = 0
    radar3: int = 0

    load_room: int = 300

    con_sdcooldown: int = 0
    skill_slots: int = 0
    suppression: int = 0

    tail_growth: int = 0
    transclass: int = 0
    transcost: dict[int, int] = field(default_factory=dict)

    trainagl: int = 0
    traincon: int = 0
    trainint: int = 0
    trainspd: int = 0
    trainstr: int = 0
    trainwis: int = 0

    upgrade: int = 0

    wimp_level: int = 0

    height: int = 0
    weight: int = 0

    pref_flags: int = 0
    admin_flags: int = 0

    affected: list[Affected] = field(default_factory=list)
    affectedv: list[Affected] = field(default_factory=list)

    lboard: dict[int, int] = field(default_factory=dict)

    contents: list[Object] = field(default_factory=list)
    equipment: dict[int, Object] = field(default_factory=dict)

    username: str = ""

@dataclass(slots=True)
class DgScriptPrototype:
    id: int = -1
    name: str = ""
    attach_type: int = 0
    trigger_type: int = 0
    narg: int = 0
    command: str = ""
    body: str = ""

@dataclass(slots=True)
class ShopBuyData:
    item_type: int = -1
    keywords: str = ""

@dataclass(slots=True)
class Shop:
    id: int = -1
    products: list[int] = field(default_factory=list)
    profit_buy: float = 1.0
    profit_sell: float = 1.0
    types_bought: list[ShopBuyData] = field(default_factory=list)
    no_such_item1: str = ""
    no_such_item2: str = ""
    do_not_buy: str = ""
    missing_cash1: str = ""
    missing_cash2: str = ""
    message_buy: str = ""
    message_sell: str = ""
    temper1: int = 0
    shop_flags: int = 0
    keeper: int | None = None
    in_room: list[int] = field(default_factory=list)
    open1: int = 0
    open2: int = 0
    close1: int = 0
    close2: int = 0
    with_who: int = 0
    bankAccount: int = 0

@dataclass(slots=True)
class Guild:
    id: int = -1
    skills: list[int] = field(default_factory=list)
    feats: list[int] = field(default_factory=list)
    charge: float = 1.0
    no_such_skill: str = ""
    not_enough_gold: str = ""
    minlvl: int = 0
    keeper: int | None = None
    open: int = 0
    close: int = 0
    with_who: int = 0

@dataclass(slots=True)
class Component:
    object_prototype_id: int = -1
    consumed: bool = True
    in_room: bool = False

@dataclass(slots=True)
class Assembly:
    object_prototype_id: int = -1
    assembly_type: str = "build"
    components: list[Component] = field(default_factory=list)

@dataclass(slots=True)
class ResetCommand:
    command: str = ""
    if_flag: bool = False
    arg1: int = 0
    arg2: int = 0
    arg3: int = 0
    arg4: int = 0
    arg5: int = 0
    sa1: str = ""
    sa2: str = ""

@dataclass(slots=True)
class Zone:
    id: int = -1
    name: str = ""
    builders: str = ""
    lifespan: int = 30
    age: int = 0
    reset_mode: int = 2
    zone_flags: int = 0
    resets: list[ResetCommand] = field(default_factory=list)
    bot: int = 0
    top: int = 0
    min_level: int = 0
    max_level: int = 0


@dataclass(slots=True)
class Account:
    name: str = ""
    email: str = ""
    password: str = ""
    slots: int = 3
    rpp: int = 0
    rpp_bank: int = 0
    characters: set[str] = field(default_factory=set)
    admin_level: int = 0
    custom_file: bool = False
    customs: list[str] = field(default_factory=list)

@dataclass(slots=True)
class HelpEntry:
    name: str = ""
    entry: str = ""
    min_level: int = 0


def flags_to_new_set(flags: int, flag_enum: IntEnum) -> set:
    result = set()
    for i in range(128):
        if flags & (1 << i):
            try:
                result.add(flag_enum(i))
            except ValueError:
                pass
    return result

def strip_color(s: str) -> str:
    """
    This will iterate through the characters in a string. all color codes are prefixed with @
    while @@ is a literal @. So if we see an @, we check if the next character is also an @.
    """
    out = ""
    i = 0
    while i < len(s):
        if s[i] == "@":
            if i + 1 < len(s) and s[i + 1] == "@":
                out += "@"
                i += 2
            else:
                i += 2
        else:
            out += s[i]
            i += 1
    return out

def asciiflag_conv(flag: str) -> int:
    flags = 0
    is_num = True
    for c in flag:
        if c.islower():
            flags |= 1 << (ord(c) - ord('a'))
        elif c.isupper():
            flags |= 1 << (26 + (ord(c) - ord('A')))
        if not (c.isdigit() or c == '-'):
            is_num = False
    if is_num:
        flags = int(flag)
    return flags

def flag_conv(flags: list[str]) -> int:
    """
    Given a list of 4 flags (each of which is a 32-bit integer), return a 128-bit number.
    """
    if len(flags) != 4:
        raise ValueError("Expected 4 flags")

    result = 0
    for i, flag in enumerate(flags):
        result |= (asciiflag_conv(flag) & 0xFFFFFFFF) << (32 * i)
    return result

class Scanner:

    def __init__(self, data: str):
        self.data = data
        self.pos = 0
    
    def readline(self) -> str:
        """
        Returns the next line with the \r\n stripped.
        Should work even if there is no \r.
        """
        if self.pos >= len(self.data):
            return ""
        next_pos = self.data.find("\n", self.pos)
        if next_pos == -1:
            next_pos = len(self.data)
        line = self.data[self.pos:next_pos]
        if line.endswith("\r"):
            line = line[:-1]
            next_pos += 1
        self.pos = next_pos + 1
        return line

    def readuntil(self, delimiter: str) -> str:
        if self.pos >= len(self.data):
            return ""
        next_pos = self.data.find(delimiter, self.pos)
        if next_pos == -1:
            next_pos = len(self.data)
        result = self.data[self.pos:next_pos]
        self.pos = next_pos + len(delimiter)
        return result

    def tell(self) -> int:
        return self.pos

    def seek(self, pos: int):
        self.pos = pos

def parse_account(f: Scanner) -> Account:
    acc = {
        "name": f.readline(),
        "email": f.readline().replace("<AT>", "@"),
        "password": f.readline(),
        "slots": int(f.readline()),
        "rpp": int(f.readline())
    }

    characters = list()
    while True:
        pos = f.tell()
        line = f.readline()
        if line.isdigit():
            f.seek(pos)
            break
        if line == "Empty":
            continue
        characters.append(line)

    if characters:
        acc["characters"] = characters
    acc["admin_level"] = int(f.readline())
    acc["custom_file"] = int(f.readline())
    acc["rpp_bank"] = int(f.readline())
    return Account(**acc)


def parse_character(f: Scanner) -> Character:
    out = Character()

    def _parse_affects():
        while a := f.readline():
            values = list(map(int, a.split()))
            if values[0] == 0:
                return
            out.affected.append(Affected(**{
                "type": values[0],
                "duration": values[1],
                "modifier": values[2],
                "location": values[3],
                "bitvector": values[4],
                "specific": values[5]
            }))
    
    def _parse_skills():
        while s := f.readline():
            values = list(map(int, s.split()))
            if values[0] == 0:
                return
            try:
                skill = values[0]
            except ValueError:
                continue
            out.skills[skill] = SkillData(
                level=values[1],
                perfs=values[2]
            )

    def _parse_skill_bonus():
        while s := f.readline():
            values = list(map(int, s.split()))
            if values[0] == 0:
                return

    while line := f.readline():
        if ":" in line:
            key, value = line.split(":", 1)
            key = key.strip()
            value = value.strip()
        else:
            key = line.strip()
            value = ""

        match key:
            case "Ac":
                out.armor = int(value)
            case "Act":
                out.act = flag_conv(value.split())
            case "Aff":
                out.affected_by = flag_conv(value.split())
            case "Affs":
                out.affected = _parse_affects()
            case "Affv":
                out.affectedv = _parse_affects()
            case "AdmL":
                out.admin_level = int(value)
            case "Abso":
                out.absorbs = int(value)
            case "AdmF":
                out.admin_flags = flag_conv(value.split())
            case "Alin":
                out.alignment = int(value)
            case "Aura":
                out.aura = int(value)
            case "Bank":
                out.bank_gold = int(value)
            case "Bki":
                out.baseki = int(value)
            case "Blss":
                out.blesslvl = int(value)
            case "Boam":
                out.lboard[0] = int(value)
            case "Boai":
                out.lboard[1] = int(value)
            case "Boac":
                out.lboard[2] = int(value)
            case "Boad":
                out.lboard[3] = int(value)
            case "Boab":
                out.lboard[4] = int(value)
            case "Bonu":
                bonuses = [int(x) for x in value.split()]
                for i, b in enumerate(bonuses):
                    if b != 0:
                        out.bonuses[i] = b
            case "Boos":
                out.boosts = int(value)
            case "Bpl":
                out.basepl = int(value)
            case "Brth":
                out.time_birth = int(value)
            case "Bst":
                out.basest = int(value)
            case "Cha":
                out.cha = int(value)
            case "Clas":
                out.chclass = int(value)
            case "Con":
                out.con = int(value)
            case "Crtd":
                out.time_created = int(value)
            case "Desc":
                out.description = f.readuntil("~").rstrip("~")
                f.readline() # skip tilde line
            case "Deat":
                out.deathtime = int(value)
            case "Deac":
                out.dcount = int(value)
            case "Dex":
                out.dex = int(value)
            case "Drnk":
                out.conditions[0] = int(value)
            case "Exp":
                out.exp = int(value)
            case "Eye":
                out.eye = int(value)
            case "Forc":
                out.forgetcount = int(value)
            case "Forg":
                out.forgeting = int(value)
            case "Fury":
                out.fury = int(value)
            case "Gold":
                out.gold = int(value)
            case "Geno":
                out.genome[0] = int(value)
            case "Gen1":
                out.genome[1] = int(value)
            case "Hite":
                out.height = int(value)
            case "Home":
                out.hometown = int(value)
            case "Hrc":
                out.hairc = int(value)
            case "Hrl": 
                out.hairl = int(value)
            case "Hrs":
                out.hairs = int(value)
            case "Hung":
                out.conditions[1] = int(value)
            case "Id":
                out.id = int(value)
            case "INGl":
                out.ingestLearned = int(value)
            case "Int":
                out.intel = int(value)
            case "Kaio":
                out.kaioken = int(value)
            case "Last":
                out.time_logon = int(value)
            case "Lern":
                out.practices = int(value)
            case "Levl":
                out.level = int(value)
            case "Lila":
                out.limb_condition[1] = int(value)
            case "Lill":
                out.limb_condition[3] = int(value)
            case "Lira":
                out.limb_condition[0] = int(value)
            case "Lirl":
                out.limb_condition[2] = int(value)
            case "Lint":
                out.lastint = int(value)
            case "Mexp":
                out.molt_experience = int(value)
            case "Mlvl":
                out.molt_level = int(value)
            case "Maji":
                out.majinize = int(value)
            case "Mimi":
                out.mimic = int(value)
            case "MxAg":
                out.time_maxage = int(value)
            case "Name":
                out.name = value
            case "Olc":
                out.olc_zone = int(value)
            case "Page":
                out.page_length = int(value)
            case "Phas":
                out.phase = int(value)
            case "Plyd":
                out.time_played = int(value)
            case "PfIn":
                out.poofin = value
            case "PfOt":
                out.poofout = value
            case "Posi":
                out.position = int(value)
            case "Pref":
                out.pref_flags = flag_conv(value.split())
            case "Prff":
                out.preference = int(value)
            case "Race":
                out.race = int(value)
            case "Raci":
                out.racial_pref = int(value)
            case "rDis":
                out.rdisplay = value
            case "Rtim":
                out.rewtime = int(value)
            case "Rad1":
                out.radar1 = int(value)
            case "Rad2":
                out.radar2 = int(value)
            case "Rad3":
                out.radar3 = int(value)
            case "Room":
                out.load_room = int(value)
            case "RPfe":
                out.feature = value
            case "Sex":
                out.sex = int(value)
            case "Skil":
                _parse_skills()
            case "Size":
                out.size = int(value)
            case "SklB":
                _parse_skill_bonus()
            case "SkCl":
                out.practices += int(value.split()[1])
            case "Slot":
                out.skill_slots = int(value)
            case "Str":
                out.strength = int(value)
            case "Supp":
                out.suppression = int(value)
            case "Tgro":
                out.tail_growth = int(value)
            case "Tcla":
                out.transclass = int(value)
            case "Tcos":
                values = list(map(int, value.split()))
                out.transcost[values[0]] = values[1]
            case "Thir":
                out.conditions[2] = int(value)
            case "Trag":
                out.trainagl = int(value)
            case "Trco":
                out.traincon = int(value)
            case "Trin":
                out.trainint = int(value)
            case "Trsp":
                out.trainspd = int(value)
            case "Trst":
                out.trainstr = int(value)
            case "Trwi":
                out.trainwis = int(value)
            case "Voic":
                out.voice = value
            case "Wate":
                out.weight = int(value)
            case "Wimp":
                out.wimp_level = int(value)
            case "Wis":
                out.wis = int(value)

    return out


def parse_objects(f: Scanner):
    while True:
        line = f.readline()
        if not line:
            break
        # Objects begin with a line containing a # followed by the vnum.
        if line.startswith("$~"):
            break
        if not line.startswith("#"):
            break
        out = ObjectPrototype()
        out.id = int(line[1:])
        out.name = f.readuntil("~").rstrip("~")
        f.readline()
        out.short_description = f.readuntil("~").rstrip("~")
        f.readline()
        out.description = f.readuntil("~").rstrip("~")
        f.readline()
        out.action_description = f.readuntil("~").rstrip("~")
        f.readline()
        
        symbols = f.readline().split()
        out.item_type = int(symbols[0])
        out.extra_flags = flag_conv(symbols[1:5])
        
        out.wear_flags = flag_conv(symbols[5:9])
        out.bitvector = flag_conv(symbols[9:13])

        item_values = list(map(int, f.readline().split()))

        for i, v in enumerate(item_values):
            if not v:
                continue
            out.values[i] = v
        
        weight, cost, cost_per_day, level = list(map(int, f.readline().split()))
        out.weight = weight
        out.cost = cost
        out.cost_per_day = cost_per_day
        out.level = level
        
        def handle_z():
            data = f.readline()
            out.size = int(data)

        def handle_t(data):
            """
            Triggers.
            """
            out.proto_script.append(int(data))

        def handle_s():
            """
            Spell books. We currently skip this.
            """
            data = f.readline()

        def handle_e():
            """
            Extra descriptions.
            """
            keyword = f.readuntil("~").rstrip("~")
            f.readline()
            description = f.readuntil("~").rstrip("~")
            f.readline()
            out.extra_descriptions.append(ExtraDescription(keyword, description))

        def handle_a():
            """
            Affects.
            """
            out.affected.append(Affected(*map(int, f.readline().split())))

        while True:
            pos = f.tell()
            line = f.readline()
            if not line:
                break
            if line.startswith("#"):
                yield out
                f.seek(pos)
                break
            if line.startswith("$~"):
                yield out
                return
            if line.startswith("Z"):
                handle_z()
            elif line.startswith("T"):
                _, data = line.split(" ", 1)
                handle_t(data)
            elif line.startswith("S"):
                handle_s()
            elif line.startswith("E"):
                handle_e()
            elif line.startswith("A"):
                handle_a()
        
        yield out


def parse_mobiles(f: Scanner):
    while True:
        line = f.readline()
        if not line:
            break
        # Mobiles begin with a line containing a # followed by the vnum.
        if line.startswith("$~"):
            break
        if not line.startswith("#"):
            break
        out = CharacterPrototype()
        out.id = int(line[1:])
        out.name = f.readuntil("~").rstrip("~")
        f.readline()
        out.short_descr = f.readuntil("~").rstrip("~")
        f.readline()
        out.long_descr = f.readuntil("~").rstrip("~")
        f.readline()
        out.description = f.readuntil("~").rstrip("~")
        f.readline()
        
        symbols = f.readline().split()
        out.act = flag_conv(symbols[0:4])
        out.affected_by = flag_conv(symbols[4:8])
        out.alignment = int(symbols[8])
        
        letter = symbols[9]

        def parse_simple():
            symbols = f.readline().replace("+", " ").replace("d", " ").split()
            out.level = int(symbols[0])
            out.basepl = int(symbols[3])
            out.baseki = int(symbols[4])
            out.basest = int(symbols[5])
            out.damage_mod = int(symbols[8])

            symbols = f.readline().split()
            out.gold = int(symbols[0])
            out.race = int(symbols[2])
            out.chclass = int(symbols[3])
            symbols = f.readline().split()
            out.default_position = int(symbols[1])
            out.sex = int(symbols[2])


        def parse_espec(line):
            etype, data = line.lower().split(":", 1)
            etype = etype.strip()
            data = data.strip()

            match etype:
                case "size":
                    out.size = int(data)
                case "str":
                    out.strength = int(data)
                case "int":
                    out.intel = int(data)
                case "wis":
                    out.wis = int(data)
                case "dex":
                    out.dex = int(data)
                case "con":
                    out.con = int(data)
                case "cha":
                    out.cha = int(data)

        def parse_enhanced():
            parse_simple()
            
            while True:
                pos = f.tell()
                line = f.readline()
                if not line:
                    break
                if line.startswith("#"):
                    f.seek(pos)
                    break
                if line.startswith("E"):
                    return
                parse_espec(line)

        match letter:
            case "S":
                parse_simple()
            case "E":
                parse_enhanced()
        
        # Following the S/E, we MAY have any number of T <number> lines.
        while True:
            pos = f.tell()
            line = f.readline()
            if not line:
                break
            if line.startswith("#"):
                f.seek(pos)
                break
            if line.startswith("$~"):
                yield out
                return
            if line.startswith("T"):
                _, data = line.split(" ", 1)
                out.proto_script.append(int(data))
        
        yield out


def parse_rooms(f: Scanner):
    while True:
        line = f.readline()
        if not line:
            break
        # Rooms begin with a line containing a # followed by the vnum.
        if line.startswith("$~"):
            print("is this happening?!")
            break
        if not line.startswith("#"):
            print("IS THIS HAPPENING?!")
            break
        out = Room()
        out.id = int(line[1:])
        out.name = f.readuntil("~").rstrip("~")
        f.readline()
        out.description = f.readuntil("~").rstrip("~")
        f.readline()

        symbols = f.readline().split()
        out.room_flags = flag_conv(symbols[0:4])
        out.sector_type = int(symbols[5])

        def setup_dir(line):
            ex = Exit()
            direction = int(line[1:])
            if keywords := f.readuntil("~").rstrip("~"):
                ex.keywords = keywords
            f.readline()
            if description := f.readuntil("~").rstrip("~"):
                ex.description = description
            f.readline()
            symbols = f.readline().split()
            
            ex.exit_info = int(symbols[0])
            ex.key = int(symbols[1])
            ex.to_room = int(symbols[2])
            ex.dclock = int(symbols[3])
            ex.dchide = int(symbols[4])
            return direction, ex

        while True:
            pos = f.tell()
            line = f.readline()
            if not line:
                break
            if line.startswith("#"):
                f.seek(pos)
                break
            if line.startswith("$~"):
                yield out
                return
            if line.startswith("S"):
                # Normal room stuff ends on S; however, T sections MAY follow.
                # For our handling, we'll just ignore S and only respond to # or $~ as terminators.
                continue
            if line.startswith("T"):
                # This is a proto_script line, same as with objects.
                _, data = line.split(" ", 1)
                out.proto_script.append(int(data))
            if line.startswith("E"):
                # extra descriptions, use same pattern as objects
                keyword = f.readuntil("~").rstrip("~")
                f.readline()
                description = f.readuntil("~").rstrip("~")
                f.readline()
                out.extra_descriptions.append(ExtraDescription(keywords=keyword, description=description))
            if line.startswith("D"):
                direction, ex = setup_dir(line)
                out.exits[direction] = ex
        
        yield out

def parse_scripts(f: Scanner):
    while True:
        line = f.readline()
        if not line:
            break
        # Scripts begin with a line containing a # followed by the vnum.
        if line.startswith("$~"):
            break
        if not line.startswith("#"):
            break
        out = DgScriptPrototype()
        out.id = int(line[1:])
        out.name = f.readuntil("~").rstrip("~")
        f.readline()

        symbols = f.readline().split()
        out.attach_type = int(symbols[0])
        out.trigger_type = asciiflag_conv(symbols[1])
        out.narg = int(symbols[2])
 
        command = f.readuntil("~").rstrip("~")
        f.readline()
        if command:
            out.command = command

        body = f.readuntil("~").rstrip("~")
        if body:
            out.body = body
        f.readline()

        yield out

RE_BUYTYPE = re.compile(r"^(\d+)(.*)$")

def parse_shops(f: Scanner):
    f.readline()  # ignore first line, it's just a version number

    while True:
        line = f.readline()
        if not line:
            break
        if line.startswith("$~"):
            break
        if not line.startswith("#"):
            break
        out = Shop()
        out.id = int(line[1:].rstrip("~"))

        # gather products list until we see a -1, which is the terminator for the products list.
        while True:
            product = int(f.readline())
            if product == -1:
                break
            if product in out.products:
                continue
            out.products.append(product)

        out.profit_buy = float(f.readline())
        out.profit_sell = float(f.readline())

        # now we gather the type ids of what the shop buys.
        # This is terminated by a line of just -1
        # but valid lines are <int>[<string>] without a space. The string is keywords.
        while True:
            if (data := f.readline()) == "-1":
                break
            match = RE_BUYTYPE.match(data)
            if not match:
                continue
            type_id = int(match.group(1))
            keywords = match.group(2).strip()
            entry = ShopBuyData(item_type=type_id, keywords=keywords)
            out.types_bought.append(entry)


        for msg in ("no_such_item1", "no_such_item2", "do_not_buy", "missing_cash1", "missing_cash2", "message_buy", "message_sell"):
            setattr(out, msg, f.readuntil("~").rstrip("~"))
            f.readline()
        
        out.temper1 = int(f.readline())
        out.shop_flags = int(f.readline())
        out.keeper = int(f.readline())
        if out.keeper == -1:
            out.keeper = None

        out.with_who = flag_conv(f.readline().split())

        while True:
            data = int(f.readline())
            if data == -1:
                break
            out.in_room.append(data)

        for t in ("open1", "close1", "open2", "close2"):
            setattr(out, t, int(f.readline()))
        
        yield out


def parse_guilds(f: Scanner):
    while True:
        line = f.readline()
        if not line:
            break
        if line.startswith("$~"):
            break
        if not line.startswith("#"):
            break
        out = Guild()
        out.id = int(line[1:].rstrip("~"))

        skills = set()
        feats = set()

        # gather up skill/feat lines, which are <id> <type>, wher type 1 is skill and type 2 is feat.
        # list is terminated by a -1 line.
        while True:
            line = f.readline()
            if line == "-1":
                break
            type_id = 1
            if " " in line:
                skill_id, type_id = map(int, line.split())
            else:
                skill_id = int(line)
            field = skills if type_id == 1 else feats
            field.add(skill_id)
        
        for skill_id in out.skills:
            out.skills.add(skill_id)

        out.feats = feats

        out.charge = float(f.readline())
        for x in ("no_such_skill", "not_enough_gold"):
            setattr(out, x, f.readuntil("~").rstrip("~"))
            f.readline()
        
        for x in ("minlvl", "keeper"):
            setattr(out, x, int(f.readline()))
        if out.keeper == -1:
            out.keeper = None
        
        with_who = list()
        with_who.append(f.readline())

        for x in ("open", "close"):
            setattr(out, x, int(f.readline()))
        
        with_who.extend(f.readline().split())
        out.with_who = flag_conv(with_who)

        yield out

def parse_assemblies(f: Scanner):
    num_newlines = 0
    while True:
        line = f.readline()
        if not line:
            num_newlines += 1
            if num_newlines >= 1:
                break
            continue
        if line.startswith("Vnum"):
            num_newlines = 0
            out = Assembly()
            data = line.split()
            out.object_prototype_id = int(data[1][1:])
            out.assembly_type = data[2]

            while True:
                line = f.readline()
                if not line:
                    num_newlines += 1
                    break
                if line.startswith("Component"):
                    component_data = line.split()
                    component = Component(
                        object_prototype_id=int(component_data[1][1:]),
                        consumed=bool(int(component_data[2])),
                        in_room=bool(int(component_data[3]))
                    )
                    out.components.append(component)
            
            yield out


class LegacyDatabase:
    
    def __init__(self):
        self.zones: dict[int, Zone] = dict()
        self.oproto: dict[int, ObjectPrototype] = dict()
        self.nproto: dict[int, CharacterPrototype] = dict()
        self.dgproto: dict[int, DgScriptPrototype] = dict()
        self.rooms: dict[int, Room] = dict()
        self.shops: dict[int, Shop] = dict()
        self.guilds: dict[int, Guild] = dict()

        self.help: list[HelpEntry] = list()
        self.assemblies: list[Assembly] = list()

        self.objects: dict[int, Object] = dict()  # object id -> object data

        self.accounts: dict[str, Account] = dict()  # username -> account data
        self.characters: dict[int, Character] = dict()
        self.characters_to_account: dict[str, str] = dict()  # character name -> account name
    

    def zone_id_for(self, thing_id: int) -> int:
        for zone_id, zone in self.zones.items():
            if zone.bot <= thing_id <= zone.top:
                return zone_id
        return -1

    def check_affects(self):
        for char in self.nproto.values():
            counter = 0
            for affect in char.affected:
                if affect.valid():
                    counter += 1
            if counter:
                print(f"Character {char.name} has {counter} valid affects.")

        for obj in self.nproto.values():
            counter = 0
            for affect in obj.affected:
                if affect.valid():
                    counter += 1
            if counter:
                print(f"Object {obj.name} has {counter} valid affects.")

    def _load_accounts(self, data_dir: Path):
        account_dir = data_dir / "user"

        # recurse through all subdirectories and find all .usr files...
        for usr_file in account_dir.rglob("*.usr"):
            with open(usr_file, "r", encoding="utf-8", errors="ignore") as f2:
                f = Scanner(f2.read())
                acc = {
                    "name": f.readline(),
                    "email": f.readline().replace("<AT>", "@"),
                    "password": f.readline(),
                    "slots": int(f.readline()),
                    "rpp": int(f.readline())
                }

                characters = list()
                while True:
                    pos = f.tell()
                    line = f.readline()
                    if line.isdigit():
                        f.seek(pos)
                        break
                    if line == "Empty":
                        continue
                    characters.append(line)
                    self.characters_to_account[line] = acc["name"]
                if characters:
                    acc["characters"] = characters
                acc["admin_level"] = int(f.readline())
                acc["custom_file"] = int(f.readline())
                acc["rpp_bank"] = int(f.readline())
                self.accounts[acc["name"].lower()] = Account(**acc)
        
        for cus_file in account_dir.rglob("*.cus"):
            file_name = cus_file.stem
            if acc := self.accounts.get(file_name, None):
                with open(cus_file, "r", encoding="utf-8", errors="ignore") as f2:
                    f = Scanner(f2.read())
                    first = f.readline()
                    customs = list()
                    while line := f.readline():
                        customs.append(line)
                    acc.customs = customs

    


    def _load_help(self, data_dir: Path):
        """
        The helpfiles are stored as a flatfile.
        Each entry begins with a line containing the name.
        Then any number of lines, until it reaches a line beginning with a #
        The # is #<min_level> which is usually #0 but can be higher.

        We are going to set self.help to a list of dicts with keys "name", "entry", and "min_level".
        """
        hfile = data_dir / "text" / "help" / "help.hlp"

        with open(hfile, "r") as f:
            current_entry = None
            for line in f:
                line = line.rstrip()
                if line.startswith("#"):
                    if current_entry is not None:
                        self.help.append(HelpEntry(**current_entry))
                    current_entry = {
                        "name": "",
                        "entry": "",
                        "min_level": int(line[1:]) if len(line) > 1 else 0,
                    }
                elif current_entry is not None:
                    if not current_entry["name"]:
                        current_entry["name"] = line
                    else:
                        current_entry["entry"] += line + "\n"
            if current_entry is not None:
                self.help.append(HelpEntry(**current_entry))
    
    def _load_zones(self, data_dir: Path):
        """
        Zones are stored as <number>.zon files in the zone_dir.

        Each zone file is a line-based flatfile, each being one zone.

        The first line is a version number; ignore.
        The second line is #<number>, where <number> is the Zone's ID.
        The next line is a string terminated by ~, which is the "builders" property.
        The next line is a string terminated by ~, which is the "name" property.
        The next line is a sequence of numbers separated by spaces.
        
        After those are a series of lines representing Zone Resets. They end with a line containing just the letter S
        Only the first 7 tokens of each Zone Reset line is relevant.

        """
        zone_dir = data_dir / "world" / "zon"

        # for starters let's list all .zon files
        for zon_file in zone_dir.glob("*.zon"):
            with open(zon_file, "r") as f:
                lines = f.readlines()
                if len(lines) < 5:
                    continue
                # ignore the first line since it's just a version number
                zone_id_line = lines[1].rstrip()
                if not zone_id_line.startswith("#"):
                    continue
                try:
                    zone_id = int(zone_id_line[1:])
                except ValueError:
                    continue
                builders = lines[2].rstrip().rstrip("~")
                name = lines[3].rstrip().rstrip("~")
                stats = lines[4].rstrip().split()
                bottom = int(stats[0])
                top = int(stats[1])
                lifespan = int(stats[2])
                reset_mode = int(stats[3])
                # the next 4 stats are flags in an array...
                flags_array = stats[4:8]
                zone_flags = flag_conv(flags_array)
                min_level = int(stats[8])
                max_level = int(stats[9])

                resets = []
                for line in lines[5:]:
                    line = line.rstrip()
                    if line == "S":
                        break
                    tokens = line.split()
                    if len(tokens) < 7:
                        continue
                    reset_data = {
                        "command": tokens[0],
                        "if_flag": int(tokens[1]),
                        "arg1": int(tokens[2]),
                        "arg2": int(tokens[3]),
                        "arg3": int(tokens[4]),
                        "arg4": int(tokens[5]),
                        "arg5": int(tokens[6]),
                    }
                    resets.append(ResetCommand(**reset_data))
                
                zone_data = {
                    "id": zone_id,
                    "builders": builders,
                    "name": name,
                    "bot": bottom,
                    "top": top,
                    "lifespan": lifespan,
                    "reset_mode": reset_mode,
                    "zone_flags": zone_flags,
                    "min_level": min_level,
                    "max_level": max_level,
                    "resets": resets
                }
                self.zones[zone_id] = Zone(**zone_data)

    def _load_oproto(self, data_dir: Path):

        obj_dir = data_dir / "world" / "obj"

        for obj_file in obj_dir.glob("*.obj"):
            with open(obj_file, "r", encoding="utf-8", errors="ignore") as f:
                scanner = Scanner(f.read())
                for o in parse_objects(scanner):
                    self.oproto[o.id] = o
    
    def _load_nproto(self, data_dir: Path):
        npc_dir = data_dir / "world" / "mob"

        for npc_file in npc_dir.glob("*.mob"):
            with open(npc_file, "r", encoding="utf-8", errors="ignore") as f:
                scanner = Scanner(f.read())
                for n in parse_mobiles(scanner):
                    self.nproto[n.id] = n

    def _load_rooms(self, data_dir: Path):
        room_dir = data_dir / "world" / "wld"

        for room_file in room_dir.glob("*.wld"):
            with open(room_file, "r", encoding="utf-8", errors="ignore") as f:
                # Rooms are stored in a similar format to objects and mobiles, but with different fields.
                # They also have a vnum, and are terminated by a line containing $~.
                
                # extract zone_id from filename; it's the part before .wld
                zone_id = int(room_file.stem)
                scanner = Scanner(f.read())
                for r in parse_rooms(scanner):
                    r.zone = zone_id
                    self.rooms[r.id] = r

    def _load_dgproto(self, data_dir: Path):
        trg_dir = data_dir / "world" / "trg"

        for trg_file in trg_dir.glob("*.trg"):
            with open(trg_file, "r", encoding="utf-8", errors="ignore") as f:
                scanner = Scanner(f.read())
                for t in parse_scripts(scanner):
                    self.dgproto[t.id] = t

    def _load_shops(self, data_dir: Path):
        shp_dir = data_dir / "world" / "shp"

        for shp_file in shp_dir.glob("*.shp"):
            with open(shp_file, "r", encoding="utf-8", errors="ignore") as f:
                scanner = Scanner(f.read())
                for s in parse_shops(scanner):
                    self.shops[s.id] = s

    def _load_guilds(self, data_dir: Path):
        gld_dir = data_dir / "world" / "gld"

        for gld_file in gld_dir.glob("*.gld"):
            with open(gld_file, "r", encoding="utf-8", errors="ignore") as f:
                scanner = Scanner(f.read())
                for s in parse_guilds(scanner):
                    self.guilds[s.id] = s

    def _load_assemblies(self, data_dir: Path):
        ass_file = data_dir / "etc" / "assemblies"

        with open(ass_file, "r", encoding="utf-8", errors="ignore") as f:
            scanner = Scanner(f.read())
            for a in parse_assemblies(scanner):
                self.assemblies.append(a)
    
    def _load_houses(self, data_dir: Path):
        pass

    def _load_characters(self, data_dir: Path):
        player_dir = data_dir / "plrfiles"

        for plr_file in player_dir.rglob("*.plr"):
            if not plr_file.stem.isascii():
                continue
            with open(plr_file, "r", encoding="utf-8", errors="ignore") as f2:
                f = Scanner(f2.read())
                c = parse_character(f)

                c.username = self.characters_to_account[c.name]

                self.characters[c.id] = c

    def load_from_files(self, data_dir: Path):
        self._load_help(data_dir)
        self._load_zones(data_dir)
        self._load_oproto(data_dir)
        self._load_nproto(data_dir)
        self._load_rooms(data_dir)
        self._load_dgproto(data_dir)
        self._load_shops(data_dir)
        self._load_guilds(data_dir)
        self._load_assemblies(data_dir)
        self._load_houses(data_dir)
        self._load_accounts(data_dir)
        self._load_characters(data_dir)


def prepare_migration(path: Path) -> LegacyDatabase:
    db = LegacyDatabase()
    db.load_from_files(path)
    return db

def test() -> LegacyDatabase:
    path = Path("data")
    return prepare_migration(path)