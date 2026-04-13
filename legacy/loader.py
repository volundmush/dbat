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
    keywords: str | None = None
    description: str | None = None
    exit_into: int = 0
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


@dataclass(slots=True)
class ObjectPrototype(ObjectBase):
    vnum: int = -1


@dataclass(slots=True)
class Object(ObjectBase):
    id: int = -1
    object_prototype_id: int = -1


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

@dataclass(slots=True)
class CharacterPrototype(CharacterBase):
    vnum: int = -1
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
    keeper: int = -1
    in_room: list[int] = field(default_factory=list)
    open1: int = 0
    open2: int = 0
    close1: int = 0
    close2: int = 0
    with_who: int = 0

@dataclass(slots=True)
class Guild:
    id: int = -1
    skills: list[int] = field(default_factory=list)
    feats: list[int] = field(default_factory=list)
    charge: float = 1.0
    no_such_skill: str = ""
    not_enough_gold: str = ""
    minlvl: int = 0
    keeper: int = -1
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
                "aff_flags": AffectFlag(values[4]),
                "specific": values[5]
            }))
    
    def _parse_skills():
        while s := f.readline():
            values = list(map(int, s.split()))
            if values[0] == 0:
                return
            try:
                skill = Skill(values[0])
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

    raw_player_flags: int = 0

    while line := f.readline():
        if ":" in line:
            key, value = line.split(":", 1)
            key = key.strip()
            value = value.strip()
        else:
            key = line.strip()
            value = ""

        match key:
            case "Act":
                out.act = flag_conv(value.split())
            case "Aff":
                out.affected_by = flag_conv(value.split())
            case "Affs":
                out.affected = _parse_affects()
            case "Affv":
                out.affectedv = _parse_affects()
            case "AdmL":
                out.stats["admin_level"] = int(value)
            case "Abso":
                out.stats["absorbs"] = int(value)
            case "AdmF":
                out.admin_flags = flags_to_new_set(flag_conv(value.split()), AdminFlag)
            case "Alin":
                out.stats["good_evil"] = int(value)
            case "Bank":
                out.stats["money_bank"] = int(value)
            case "Bki":
                out.stats["ki"] = int(value)
            case "Blss":
                out.stats["bless_level"] = int(value)
            case "Bonu":
                f.readline() # skip bonus line
            case "Boos":
                out.stats["boosts"] = int(value)
            case "Bpl":
                out.stats["pl"] = int(value)
            case "Brth":
                out.time["birth"] = int(value)
            case "Bst":
                out.stats["st"] = int(value)
            case "Cha":
                out.stats["speed"] = int(value)
            case "Clas":
                sen = int(value) + 1
                if sen > 14:
                    sen = 0
                out.sensei = Sensei(sen)
            case "Con":
                out.stats["constitution"] = int(value)
            case "Crtd":
                out.time["created"] = int(value)
            case "Desc":
                out.look_description = f.readuntil("~").rstrip("~")
                f.readline() # skip tilde line
            case "Dex":
                out.stats["agility"] = int(value)
            case "Exp":
                out.stats["experience"] = int(value)
            case "Eali":
                out.stats["law_chaos"] = int(value)
            case "Gold":
                out.stats["money_carried"] = int(value)
            case "Hite":
                out.stats["height"] = int(value)
            case "Id":
                out.id = int(value)
            case "Int":
                out.stats["intelligence"] = int(value)
            case "Last":
                out.time["logon"] = int(value)
            case "Lern":
                out.stats["practices"] = int(value)
            case "Mexp":
                out.stats["molt_experience"] = int(value)
            case "Mlvl":
                out.stats["molt_level"] = int(value)
            case "MxAg":
                out.time["maxage"] = int(value)
            case "Name":
                out.name = value
            case "Plyd":
                out.time["played"] = int(value)
            case "Pref":
                out.pref_flags = flags_to_new_set(flag_conv(value.split()), PrefFlag)
            case "Race":
                race = int(value) + 1
                if race > 23:
                    race = 0
                out.race = Race(race)
            case "Sex":
                out.sex = Sex(int(value))
            case "Skil":
                _parse_skills()
            case "Size":
                out.stats["size"] = Size(int(value))
            case "SklB":
                _parse_skill_bonus()
            case "Slot":
                out.stats["skill_slots"] = int(value)
            case "Str":
                out.stats["strength"] = int(value)
            case "Voic":
                out.voice = value
            case "Wate":
                out.stats["weight"] = int(value)
            case "Wis":
                out.stats["wisdom"] = int(value)

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
        out.vnum = int(line[1:])
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
        out.vnum = int(line[1:])
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
            out.stats["level"] = int(symbols[0])
            out.stats["health"] = int(symbols[3])
            out.stats["ki"] = int(symbols[4])
            out.stats["stamina"] = int(symbols[5])
            out.stats["damage_mod"] = int(symbols[8])

            symbols = f.readline().split()
            out.gold = int(symbols[0])
            out.race = int(symbols[2])
            out.sensei = int(symbols[3])
            symbols = f.readline().split()
            out.position = int(symbols[1])
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
        out.vnum = int(line[1:])
        out.name = f.readuntil("~").rstrip("~")
        f.readline()
        out.look_description = f.readuntil("~").rstrip("~")
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
        out.vnum = int(line[1:])
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
        out.vnum = int(line[1:].rstrip("~"))

        # gather products list until we see a -1, which is the terminator for the products list.
        while True:
            product = int(f.readline())
            if product == -1:
                break
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
        
        out.temper = int(f.readline())
        out.shop_flags = flags_to_new_set(int(f.readline()), ShopFlag)
        out.keeper = int(f.readline())

        with_who = flag_conv(f.readline().split())
        out.load_picky(with_who)

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
        out.vnum = int(line[1:].rstrip("~"))

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
            try:
                sk = Skill(skill_id)
                out.skills.add(sk)
            except ValueError:
                continue

        out.feats = feats

        out.charge = float(f.readline())
        for x in ("no_such_skill", "not_enough_gold"):
            setattr(out, x, f.readuntil("~").rstrip("~"))
            f.readline()
        
        for x in ("minlvl", "keeper"):
            setattr(out, x, int(f.readline()))
        
        with_who = list()
        with_who.append(f.readline())

        for x in ("open", "close"):
            setattr(out, x, int(f.readline()))
        
        with_who.extend(f.readline().split())
        out.load_picky(flag_conv(with_who))

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
            out.vnum = int(data[1][1:])
            out.assembly_type = data[2]

            while True:
                line = f.readline()
                if not line:
                    num_newlines += 1
                    break
                if line.startswith("Component"):
                    component_data = line.split()
                    component = Component(
                        vnum=int(component_data[1][1:]),
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

        self.accounts: dict[str, Account] = dict()  # username -> account data
        self.characters: dict[str, Character] = dict()
        self.characters_to_account: dict[str, str] = dict()  # character name -> account name
    
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

    def _load_characters(self, data_dir: Path):
        player_dir = data_dir / "plrfiles"

        for plr_file in player_dir.rglob("*.plr"):
            if not plr_file.stem.isascii():
                continue
            with open(plr_file, "r", encoding="utf-8", errors="ignore") as f2:
                f = Scanner(f2.read())
                c = parse_character(f)

                # we only are migrating admin characters.
                if al := c.stats.get("admin_level", 0) < 1:
                    continue
                self.characters[c.name] = c


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
                        self.help.append(current_entry)
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
                color_name = lines[3].rstrip().rstrip("~")
                name = strip_color(color_name)
                stats = lines[4].rstrip().split()
                bottom = int(stats[0])
                top = int(stats[1])
                lifespan = int(stats[2])
                reset_mode = int(stats[3])
                # the next 4 stats are flags in an array...
                flags_array = stats[4:8]
                zone_flags = flags_to_new_set(flag_conv(flags_array), ZoneFlag)
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
                    resets.append(LegacyReset(**reset_data))
                
                zone_data = {
                    "vnum": zone_id,
                    "builders": builders,
                    "name": name,
                    "color_name": color_name,
                    "bottom": bottom,
                    "top": top,
                    "lifespan": lifespan,
                    "reset_mode": reset_mode,
                    "zone_flags": zone_flags,
                    "min_level": min_level,
                    "max_level": max_level,
                    "resets": resets,
                    "parent": -1
                }
                self.zones[zone_id] = Zone(**zone_data)

    def _load_oproto(self, data_dir: Path):

        obj_dir = data_dir / "world" / "obj"

        for obj_file in obj_dir.glob("*.obj"):
            with open(obj_file, "r", encoding="utf-8", errors="ignore") as f:
                scanner = Scanner(f.read())
                for o in parse_objects(scanner):
                    self.oproto[o.vnum] = o
    
    def _load_nproto(self, data_dir: Path):
        npc_dir = data_dir / "world" / "mob"

        for npc_file in npc_dir.glob("*.mob"):
            with open(npc_file, "r", encoding="utf-8", errors="ignore") as f:
                scanner = Scanner(f.read())
                for n in parse_mobiles(scanner):
                    self.nproto[n.vnum] = n

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
                    self.rooms[r.vnum] = r

    def _load_dgproto(self, data_dir: Path):
        trg_dir = data_dir / "world" / "trg"

        for trg_file in trg_dir.glob("*.trg"):
            with open(trg_file, "r", encoding="utf-8", errors="ignore") as f:
                scanner = Scanner(f.read())
                for t in parse_scripts(scanner):
                    self.dgproto[t.vnum] = t

    def _load_shops(self, data_dir: Path):
        shp_dir = data_dir / "world" / "shp"

        for shp_file in shp_dir.glob("*.shp"):
            with open(shp_file, "r", encoding="utf-8", errors="ignore") as f:
                scanner = Scanner(f.read())
                for s in parse_shops(scanner):
                    self.shops[s.vnum] = s

    def _load_guilds(self, data_dir: Path):
        gld_dir = data_dir / "world" / "gld"

        for gld_file in gld_dir.glob("*.gld"):
            with open(gld_file, "r", encoding="utf-8", errors="ignore") as f:
                scanner = Scanner(f.read())
                for s in parse_guilds(scanner):
                    self.guilds[s.vnum] = s

    def _load_assemblies(self, data_dir: Path):
        ass_file = data_dir / "etc" / "assemblies"

        with open(ass_file, "r", encoding="utf-8", errors="ignore") as f:
            scanner = Scanner(f.read())
            for a in parse_assemblies(scanner):
                self.assemblies.append(a)


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
        self._load_accounts(data_dir)
        self._load_characters(data_dir)

        # now we begin conversion.
        # starting with distributing the reset commands before we start deleting zones.
        self.process_reset_commands()
        
        # now we are safe to start mucking with the grid en masse.
        self.process_room_zones()

        # and then migrate space!
        self.migrate_space(data_dir)
    
    def _rooms_for(self, zone_id: int) -> list[dict]:
        return [room for room in self.rooms.values() if room.zone == zone_id]

    def iter_zones(self, parent_id: int = -1):
        zones = [zone for zone in self.zones.values() if zone.parent == parent_id]
        zones.sort(key=lambda z: z.vnum)
        for zone in zones:
            yield zone
            yield from self.iter_zones(parent_id=zone.vnum)

    def print_zones(self, parent_id: int = -1, depth: int = 0):
        zones = [zone for zone in self.zones.values() if zone.parent == parent_id]
        zones.sort(key=lambda z: z.vnum)
        for zone in zones:
            rooms = self._rooms_for(zone.vnum)
            check, errors = self.geometry_check_zone(zone.vnum)
            print("  " * depth + f"Zone {zone.vnum}: {zone.name} ({len(rooms)} rooms) ({len(errors)} geometry errors)")
            self.print_zones(parent_id=zone.vnum, depth=depth + 2)

    def print_rooms(self, zone_id: int):
        rooms = self._rooms_for(zone_id)
        rooms.sort(key=lambda r: r.vnum)
        for room in rooms:
            print(f"Room {room.vnum}: {strip_color(room.name)}")

    def process_reset_commands(self):
        for zone in self.zones.values():
            r = None
            for reset in zone.resets:
                
                match reset.command:
                    case "M" | "O" | "T" | "V":
                        if not (r := self.rooms.get(reset.arg3)):
                            continue
                    case "D" | "R":
                        if not (r := self.rooms.get(reset.arg1)):
                            continue


                res = ResetCommand()
                res.depends_last = reset.if_flag
                match reset.command:
                    case "M":
                        res.command = "MOB"
                        res.target = str(reset.arg1)
                        res.max = reset.arg2
                        res.max_location = reset.arg4
                        res.chance = 100 - reset.arg5
                    case "G":
                        res.command = "GIVE"
                        res.target = str(reset.arg1)
                        res.max = reset.arg2
                        res.chance = 100 - reset.arg5
                    case "O":
                        res.command = "OBJ"
                        res.target = str(reset.arg1)
                        res.max = reset.arg2
                        res.max_location = reset.arg4
                        res.chance = 100 - reset.arg5
                    case "E":
                        res.command = "EQUIP"
                        res.target = str(reset.arg1)
                        res.max = reset.arg2
                        res.ex = reset.arg3
                        res.chance = 100 - reset.arg5
                    case "P":
                        res.command = "PUT"
                        res.target = str(reset.arg1)
                        res.max = reset.arg2
                        res.ex = reset.arg3
                        res.chance = 100 - reset.arg5
                    case "R":
                        res.command = "REMOVE"
                        res.target = str(reset.arg1)
                    case "D":
                        res.command = "DOOR"
                        res.target = str(reset.arg2)
                        res.ex = reset.arg3
                    case "T":
                        res.command = "TRIGGER"
                        res.target = str(reset.arg2)
                        res.ex = reset.arg1
                    case "V":
                        res.command = "VARIABLE"
                        res.ex = reset.arg1
                        res.key = reset.sarg1
                        res.value = reset.sarg2
                
                r.reset_commands.append(res)

    def process_room_zones(self):
        """
        This method will be used to reorganize the grid.
        """

        def reparent_zone(zone_id: int, parent_id: int):
            parent = self.zones.get(parent_id)
            child = self.zones.get(zone_id)
            parent.children.add(zone_id)
            child.parent = parent_id
        
        def rezone_room(room_id: int, zone_id: int):
            room = self.rooms.get(room_id)
            zone = self.zones.get(zone_id)
            room.zone = zone_id
        
        def next_zone_id() -> int:
            return max(self.zones.keys(), default=0) + 1
        
        def reassign_rooms_range(old_zone_id: int, new_zone_id: int, bottom: int, top: int):
            for room in self.rooms.values():
                if room.zone == old_zone_id and bottom <= room.vnum <= top:
                    room.zone = new_zone_id
        
        def reassign_rooms_name_startswith(old_zone_id: int, new_zone_id: int, prefix: str):
            for room in self.rooms.values():
                if room.zone == old_zone_id and strip_color(room.name).startswith(prefix):
                    room.zone = new_zone_id

        def merge_zones(from_zone_id: int, to_zone_id: int):
            for room in self.rooms.values():
                if room.zone == from_zone_id:
                    room.zone = to_zone_id
            del self.zones[from_zone_id]
        
        def rename_zone(zone_id: int, new_name: str):
            zone = self.zones.get(zone_id)
            zone.name = strip_color(new_name)
            zone.color_name = new_name

        def create_zone(name: str, parent_id: int = -1) -> tuple[int, dict]:
            new_id = next_zone_id()
            z = Zone()
            z.vnum = new_id
            z.name = strip_color(name)
            z.color_name = name
            self.zones[new_id] = z
            if parent_id != -1:
                reparent_zone(new_id, parent_id)
            return new_id, z
        
        new_roots: dict[str, int] = dict()

        for zh in ZONES_TO_LINK:
            new_zone_id, z = create_zone(zh.name)
            z.zone_flags = zh.flags
            new_roots[z.name.lower()] = new_zone_id
            for child in zh.children:
                reparent_zone(child, new_zone_id)
            if zh.orbit is not None:
                z.launch_destination = f"R:{zh.orbit}"
                rezone_room(zh.orbit, new_zone_id)
            if zh.land_spots:
                z.landing_spots = {k: f"R:{v}" for k, v in zh.land_spots}
            if zh.dock_spots:
                z.dock_spots = {k: f"R:{v}" for k, v in zh.dock_spots}
        
        houses, _ = create_zone("Player Housing")
        for i in (188, 189, 190, 191, 209, 210, 211):
            reparent_zone(i, houses)
        
        clans, _ = create_zone("Clans")
        for i in (180, 181, 182, 183, 184, 91):
            reparent_zone(i, clans)
        
        seasonal, _ = create_zone("Seasonal Zones")
        for i in (185, 186, 179):
            reparent_zone(i, seasonal)
        rename_zone(179, "Northran")
        haunted_house, _ = create_zone("Haunted House", parent_id=seasonal)
        rename_zone(186, "Lister's Restaurant")
        reassign_rooms_range(old_zone_id=186, new_zone_id=haunted_house, bottom=18600, top=18639)

        ships, _ = create_zone("Ships")
        for i in (158, 199, 39):
            reparent_zone(i, ships)
        rezone_room(5824, ships)
        
        to_delete, _ = create_zone("To Delete")

        for i in (18691, 18692, 18639, 21, 22):
            rezone_room(i, to_delete)

        # Working on Earth
        rename_zone(225, "Southern Jingle Wilderness (Unfinished)")
        merge_zones(226, 225)
        reparent_zone(225, new_roots["earth"])

        earth_grassy_plains, _ = create_zone("Grassy Plains", parent_id=new_roots["earth"])
        reassign_rooms_name_startswith(old_zone_id=2, new_zone_id=earth_grassy_plains, prefix="Grassy Plains")
        
        rename_zone(2, "East Highway")

        karl_fishing_pond, _ = create_zone("Karl's Fishing Pond", parent_id=2)
        reassign_rooms_name_startswith(old_zone_id=2, new_zone_id=karl_fishing_pond, prefix="Karl's Fishing")

        green_hills, _ = create_zone("Green Hills", parent_id=2)
        reassign_rooms_range(old_zone_id=2, new_zone_id=green_hills, bottom=254, top=299)
        goku_house, _ = create_zone("Goku's House", parent_id=green_hills)
        rezone_room(281, goku_house)

        rename_zone(3, "Nexus City")
        merge_zones(4, 3)
        merge_zones(5, 3)
        merge_zones(6, 3)
        merge_zones(7, 3)

        for i in (19, 20, 23, 21, 22, 80, 81, 97, 98, 99, 199):
            rezone_room(i, 3)

        nexus_spaceport, _ = create_zone("Spaceport", parent_id=3)
        reassign_rooms_name_startswith(old_zone_id=3, new_zone_id=nexus_spaceport, prefix="Nexus Spaceport")

        rosewater_park, _ = create_zone("Rosewater Park", parent_id=3)
        reassign_rooms_name_startswith(old_zone_id=3, new_zone_id=rosewater_park, prefix="Rosewater Park")

        shell_beach, _ = create_zone("Shell Beach", parent_id=3)
        reassign_rooms_name_startswith(old_zone_id=3, new_zone_id=shell_beach, prefix="Shell Beach")

        nexus_sewer, _ = create_zone("Sewer", parent_id=3)
        reassign_rooms_name_startswith(old_zone_id=3, new_zone_id=nexus_sewer, prefix="Nexus City Sewer")

        heavens_gate_dojo, _ = create_zone("Heaven's Gate Dojo", parent_id=3)
        reassign_rooms_name_startswith(old_zone_id=3, new_zone_id=heavens_gate_dojo, prefix="Heaven's Gate Dojo")

        nexus_theater, _ = create_zone("Nexus City Theater", parent_id=3)
        reassign_rooms_name_startswith(old_zone_id=3, new_zone_id=nexus_theater, prefix="Nexus City Theater")

        valeview_heights, _ = create_zone("Valeview Heights", parent_id=3)
        reassign_rooms_range(old_zone_id=3, new_zone_id=valeview_heights, bottom=418, top=444)

        rename_zone(8, "South Ocean")
        merge_zones(9, 8)
        merge_zones(10, 8)
        merge_zones(11, 8)

        reassign_rooms_name_startswith(old_zone_id=3, new_zone_id=8, prefix="South Ocean")

        reparent_zone(67, 8)
        rename_zone(67, "South Ocean Island (Unfinished)")

        roshi_island, _ = create_zone("Roshi's Island", parent_id=8)
        reassign_rooms_name_startswith(old_zone_id=8, new_zone_id=roshi_island, prefix="Small Island")

        little_island, _ = create_zone("Little Island", parent_id=8)
        reassign_rooms_name_startswith(old_zone_id=8, new_zone_id=little_island, prefix="Little Island")

        kame_house, _ = create_zone("Kame House", parent_id=roshi_island)
        reassign_rooms_name_startswith(old_zone_id=8, new_zone_id=kame_house, prefix="Kame House")

        nexus_field, _ = create_zone("Nexus Field", parent_id=new_roots["earth"])
        reassign_rooms_name_startswith(old_zone_id=8, new_zone_id=nexus_field, prefix="Nexus Field")

        rename_zone(12, "Cherry Blossom Mountain")
        rezone_room(1179, 12)
        reassign_rooms_name_startswith(old_zone_id=8, new_zone_id=12, prefix="Cherry Blossom Mountain")

        rename_zone(13, "Sandy Desert")
        rezone_room(1287, 13)
        reassign_rooms_name_startswith(old_zone_id=12, new_zone_id=13, prefix="Sandy Desert")

        small_cave, _ = create_zone("Small Cave", parent_id=13)
        reassign_rooms_name_startswith(old_zone_id=13, new_zone_id=small_cave, prefix="Small Cave")

        # Zone 14 Actually should be the Sacred Land of Korin by modern canon naming conventions...
        rename_zone(14, "Northern Plains")
        reassign_rooms_name_startswith(old_zone_id=14, new_zone_id=13, prefix="Sandy Desert")
        rezone_room(1578, 14)
        rezone_room(1579, 14)

        korin_tower, _ = create_zone("Korin's Tower", parent_id=14)
        reassign_rooms_name_startswith(old_zone_id=14, new_zone_id=korin_tower, prefix="Korin's Tower")

        # Alternatively could just call it "The Lookout"
        rename_zone(15, "Kami's Lookout")
        # The Hyperbolic Time Chamber is actually a separate dimension, but it's linked to the Lookout.
        reparent_zone(224, 15)
        rename_zone(224, "Hyperbolic Time Chamber")

        pendulum_room, _ = create_zone("Pendulum Room", parent_id=15)
        past_vegeta, _ = create_zone("Past Vegeta", parent_id=pendulum_room)
        reassign_rooms_range(old_zone_id=15, new_zone_id=past_vegeta, bottom=1580, top=1589)

        rename_zone(16, "Shadow Forest")
        rename_zone(17, "Sixteen's Retreat")
        reparent_zone(17, 16)
        reassign_rooms_name_startswith(old_zone_id=15, new_zone_id=16, prefix="Shadow Forest")

        piccolor_clearing, _ = create_zone("Piccolo's Peaceful CLearing", parent_id=16)
        reassign_rooms_range(old_zone_id=16, new_zone_id=piccolor_clearing, bottom=1658, top=1666)

        vaus_farm, _ = create_zone("Vaus Farm", parent_id=16)
        reassign_rooms_range(old_zone_id=16, new_zone_id=vaus_farm, bottom=1670, top=1699)

        # move old Gero's out of here... and old west city...
        reparent_zone(18, to_delete)
        reparent_zone(19, to_delete)

        rename_zone(58, "Three Star Elementary")
        reparent_zone(58, 130)
        merge_zones(131, 130)
        rename_zone(130, "Satan City")

        crane_dojo, _ = create_zone("Crane Dojo", parent_id=130)
        reassign_rooms_name_startswith(old_zone_id=131, new_zone_id=crane_dojo, prefix="Crane Dojo")

        satan_city_zoo, _ = create_zone("Satan City Zoo", parent_id=130)
        reassign_rooms_name_startswith(old_zone_id=131, new_zone_id=satan_city_zoo, prefix="Satan City Zoo")

        hercule_gym, _ = create_zone("Hercule Gym", parent_id=130)
        reassign_rooms_name_startswith(old_zone_id=131, new_zone_id=hercule_gym, prefix="Hercule Gym")

        satan_hospital, _ = create_zone("Satan City Hospital", parent_id=130)
        reassign_rooms_name_startswith(old_zone_id=131, new_zone_id=satan_hospital, prefix="Satan City Hospital")

        whirlpool, _ = create_zone("Whirlpool", parent_id=8)
        reassign_rooms_range(old_zone_id=131, new_zone_id=whirlpool, bottom=13155, top=13172)
        kraken_lair, _ = create_zone("Underwater Cavern", parent_id=whirlpool)
        reassign_rooms_range(old_zone_id=131, new_zone_id=kraken_lair, bottom=13173, top=13199)

        reparent_zone(134, 130)
        rename_zone(134, "Dean's Junkyard")

        scab_club, _ = create_zone("Scab Club", parent_id=130)
        reassign_rooms_name_startswith(old_zone_id=131, new_zone_id=scab_club, prefix="Scab")

        rename_zone(159, "Orange Star Campus")    
        reparent_zone(159, 130)
        rename_zone(164, "Orange Star High School")
        reparent_zone(164, 159)

        time_rift, _ = create_zone("Time Rift", parent_id=159)
        reassign_rooms_range(old_zone_id=159, new_zone_id=time_rift, bottom=15938, top=15999)

        duel_dome, _ = create_zone("Duel Dome", parent_id=130)
        reassign_rooms_name_startswith(old_zone_id=1, new_zone_id=duel_dome, prefix="Duel Dome")

        rename_zone(77, "Dr. Gero's Lab")

        rename_zone(162, "Penguin Village")

        rename_zone(20, "West City")
        king_castle, _ = create_zone("King Fury's Castle", parent_id=20)
        reassign_rooms_name_startswith(old_zone_id=126, new_zone_id=king_castle, prefix="King Castle")

        silver_mines, _ = create_zone("Silver Mines", parent_id=20)
        reassign_rooms_name_startswith(old_zone_id=126, new_zone_id=silver_mines, prefix="Silver Mine")
        for i in (2069, 2070, 19577):
            rezone_room(i, silver_mines)

        wmat_west, _ = create_zone("World Martial Arts Stadium", parent_id=to_delete)
        reassign_rooms_name_startswith(old_zone_id=126, new_zone_id=wmat_west, prefix="World Martial Arts")
        reassign_rooms_name_startswith(old_zone_id=195, new_zone_id=wmat_west, prefix="World Martial Arts")
        
        wmat_ring, _ = create_zone("World Martial Arts Ring", parent_id=wmat_west)
        reassign_rooms_name_startswith(old_zone_id=wmat_west, new_zone_id=wmat_ring, prefix="World Martial Arts Ring")

        reparent_zone(38, parent_id=20)

        game_zone, _ = create_zone("Game Zone", parent_id=20)
        reassign_rooms_range(old_zone_id=20, new_zone_id=game_zone, bottom=2075, top=2099)

        capsule_corp, _ = create_zone("Capsule Corporation HQ", parent_id=20)
        reassign_rooms_name_startswith(old_zone_id=195, new_zone_id=capsule_corp, prefix="Capsule Corp")

        merge_zones(195, 20)
        
        # Working on Vegeta
        merge_zones(95, 94)
        rename_zone(94, "Pride Cave")
        rename_zone(197, "Saiyan Lost World (Unfinished)")
        reparent_zone(94, 197)
        reparent_zone(197, new_roots["vegeta"])

        rename_zone(22, "Vegetos City")
        merge_zones(23, 22)
        merge_zones(24, 22)
        #auction house
        rezone_room(82, 22)
        rezone_room(15700, 22)

        bardock_barracks, _ = create_zone("Bardock's Barracks", parent_id=22)
        for i in (2264, 2265, 2266, 2267, 2268):
            rezone_room(i, bardock_barracks)

        training_grounds, _ = create_zone("Training Grounds", parent_id=22)
        reassign_rooms_name_startswith(old_zone_id=22, new_zone_id=training_grounds, prefix="Training Grounds")

        vegetos_colosseum, _ = create_zone("Vegetos Colosseum", parent_id=22)
        reassign_rooms_name_startswith(old_zone_id=22, new_zone_id=vegetos_colosseum, prefix="Vegetos Col")

        vegetos_spaceport, _ = create_zone("Vegetos Spaceport", parent_id=22)
        reassign_rooms_name_startswith(old_zone_id=22, new_zone_id=vegetos_spaceport, prefix="Vegetos Spaceport")

        rehab_center, _ = create_zone("Saiyan Rehab Center", parent_id=22)
        reassign_rooms_name_startswith(old_zone_id=22, new_zone_id=rehab_center, prefix="Saiyan Rehab Center")

        vegetos_palace, _ = create_zone("Vegetos Palace", parent_id=22)
        reassign_rooms_name_startswith(old_zone_id=22, new_zone_id=vegetos_palace, prefix="Vegetos Palace")

        third_class, _ = create_zone("Third Class Barracks", parent_id=vegetos_palace)
        reassign_rooms_name_startswith(old_zone_id=22, new_zone_id=third_class, prefix="Third Class Barracks")

        second_class, _ = create_zone("Second Class Barracks", parent_id=vegetos_palace)
        reassign_rooms_name_startswith(old_zone_id=22, new_zone_id=second_class, prefix="Second Class Barracks")

        first_class, _ = create_zone("First Class Barracks", parent_id=vegetos_palace)
        reassign_rooms_name_startswith(old_zone_id=22, new_zone_id=first_class, prefix="First Class Barracks")

        palace_prison, _ = create_zone("Prison", parent_id=vegetos_palace)
        for i in (2452, 2453, 2454, 2455, 2456, 2457, 2458, 2459, 2460, 2461, 2462):
            rezone_room(i, palace_prison)
        
        amnu_nation, _ = create_zone("Amnu-Nation Weapons Club", parent_id=new_roots["vegeta"])
        reassign_rooms_name_startswith(old_zone_id=22, new_zone_id=amnu_nation, prefix="Amnu-Nation")

        rename_zone(25, "Blood Dunes")
        rezone_room(2477, 25)
        reassign_rooms_name_startswith(old_zone_id=26, new_zone_id=25, prefix="Blood Dunes")
        reassign_rooms_name_startswith(old_zone_id=27, new_zone_id=25, prefix="Blood Dunes")

        hunting_camp, _ = create_zone("Saiyan Hunting Camp", parent_id=25)
        for i in (155, 156, 15701):
            rezone_room(i, hunting_camp)
        
        rename_zone(26, "Ancestral Mountains")

        destopa_swamp, _ = create_zone("Destopa Swamp", parent_id=new_roots["vegeta"])
        reassign_rooms_name_startswith(old_zone_id=27, new_zone_id=destopa_swamp, prefix="Destopa Swamp")

        rename_zone(27, "Rebellion Headquarters")
        reparent_zone(27, destopa_swamp)
        
        rename_zone(28, "Pride Forest")
        rename_zone(29, "Pride Tower")
        merge_zones(30, 29)
        merge_zones(31, 29)
        rename_zone(32, "Ruby Cave")

        rename_zone(33, "Tuffle Outpost (Unfinished)")

        # Working on Zenith
        rename_zone(34, "Utatlan")
        utatlan = 34
        reassign_rooms_name_startswith(old_zone_id=34, new_zone_id=utatlan, prefix="Utatlan")

        jaguar_dojo, _ = create_zone("Jaguar Dojo", parent_id=utatlan)
        reassign_rooms_name_startswith(old_zone_id=34, new_zone_id=jaguar_dojo, prefix="Jaguar Dojo")

        kukul_library, _ = create_zone("Kukul Library", parent_id=utatlan)
        reassign_rooms_name_startswith(old_zone_id=34, new_zone_id=kukul_library, prefix="Kukul Library")

        temple_eldritch, _ = create_zone("Temple of the Eldritch Star", parent_id=utatlan)
        reassign_rooms_name_startswith(old_zone_id=34, new_zone_id=temple_eldritch, prefix="Temple of the Eldritch Star")

        rename_zone(35, "Zenith Jungle")
        rename_zone(196, "Ancient Castle")
        rename_zone(215, "Underground Cavern")

        # Working on Majinton
        rename_zone(36, "Sihnon")
        rename_zone(37, "Majinton")

        roasted_mallow, _ = create_zone("Roasted Mallow Dojo", parent_id=37)
        reassign_rooms_name_startswith(old_zone_id=37, new_zone_id=roasted_mallow, prefix="Roasted Mallow Dojo")

        kilos_chocolate, _ = create_zone("Kilos Chocolate Factory", parent_id=37)
        reassign_rooms_name_startswith(old_zone_id=37, new_zone_id=kilos_chocolate, prefix="Kilos Chocolate Factory")

        # Working on Frigid
        merge_zones(41, 40)
        merge_zones(42, 40)
        rename_zone(40, "Ice Crown City")
        rezone_room(83, 40)

        top_level, _ = create_zone("Top Level", parent_id=40)
        reassign_rooms_name_startswith(old_zone_id=40, new_zone_id=top_level, prefix="Ice Crown City- Top Level")

        middle_level, _ = create_zone("Middle Level", parent_id=40)
        reassign_rooms_name_startswith(old_zone_id=40, new_zone_id=middle_level, prefix="Ice Crown City- Middle Level")

        lower_level, _ = create_zone("Lower Level", parent_id=40)
        reassign_rooms_name_startswith(old_zone_id=40, new_zone_id=lower_level, prefix="Ice Crown City- Lower Level")

        abandoned_level, _ = create_zone("Abandoned Level", parent_id=40)
        reassign_rooms_name_startswith(old_zone_id=40, new_zone_id=abandoned_level, prefix="Ice Crown City - Abandoned Level")

        circle_hotel, _ = create_zone("Circle Hotel", parent_id=40)
        reassign_rooms_name_startswith(old_zone_id=40, new_zone_id=circle_hotel, prefix="Circle Hotel")

        security_hq, _ = create_zone("Security HQ", parent_id=40)
        reassign_rooms_name_startswith(old_zone_id=40, new_zone_id=security_hq, prefix="Ice Crown City Security")

        rezone_room(4199, to_delete)

        rename_zone(43, "Ice Highway")

        rename_zone(44, "Glug's Volcano")
        rename_zone(45, "Secret Laboratory")
        reparent_zone(45, parent_id=44)
        reassign_rooms_range(old_zone_id=45, new_zone_id=44, bottom=4500, top=4516)
        
        rename_zone(46, "Platonic Sea")
        rename_zone(47, "Ghost Ship Blackrock")
        reparent_zone(47, parent_id=46)

        rename_zone(48, "Slave City")
        
        slave_market, _ = create_zone("Slave Market", parent_id=48)
        reassign_rooms_name_startswith(old_zone_id=48, new_zone_id=slave_market, prefix="Slave Market")

        house_glacier, _ = create_zone("House of Glacier", parent_id=48)
        reassign_rooms_name_startswith(old_zone_id=48, new_zone_id=house_glacier, prefix="House of Glacier")

        rename_zone(49, "Topica Snowfield")
        reassign_rooms_name_startswith(old_zone_id=43, new_zone_id=49, prefix="Topica Snowfield")

        rename_zone(50, "Mirror Shard Maze")

        rename_zone(51, "Acturian Woods")
        
        desolate_demesne, _ = create_zone("Desolate Demesne", parent_id=51)
        reassign_rooms_name_startswith(old_zone_id=51, new_zone_id=desolate_demesne, prefix="Desolate Demesne")

        chateau_ishran, _ = create_zone("Chateau Ishran", parent_id=51)
        reassign_rooms_name_startswith(old_zone_id=51, new_zone_id=chateau_ishran, prefix="Chateau Ishran")

        rename_zone(52, "Wyrm Spine Mountain")

        rename_zone(53, "Aromina Hunting Preserve")
        merge_zones(54, 53)

        rename_zone(55, "Cloud Ruler Temple")

        rename_zone(56, "Koltoan Mine")

        reparent_zone(170, new_roots["space"])

        # Working on Namek
        reparent_zone(59, to_delete)
        
        rename_zone(100, "Namekian Ocean")
        for i in (101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116):
            merge_zones(i, 100)

        wisdom_island, _ = create_zone("Wisdom Island", parent_id=new_roots["namek"])
        for i in (11261, 11262, 11301, 11302, 11341):
            rezone_room(i, wisdom_island)
        
        rename_zone(96, "Tower of Wisdom")
        reparent_zone(96, wisdom_island)

        grassy_island_1, _ = create_zone("Railroad Island", parent_id=new_roots["namek"])
        for i in (11347, 11386, 11387, 11426, 11427, 11467):
            rezone_room(i, grassy_island_1)
        
        oboe_island, _ = create_zone("Oboe's Island", parent_id=new_roots["namek"])
        for i in (10871, 10872, 10873, 10913, 10914, 10915, 10953, 10954, 10955, 10992, 10993):
            rezone_room(i, oboe_island)
        
        fishing_island, _ = create_zone("Fishing Island", parent_id=new_roots["namek"])
        for i in (10646, 10686, 10687, 10725, 10726, 10727, 10728, 10764, 10765, 10766, 10767, 10805):
            rezone_room(i, fishing_island)

        frieza_encampment, _ = create_zone("Frieza Encampment", parent_id=new_roots["namek"])
        reassign_rooms_name_startswith(old_zone_id=100, new_zone_id=frieza_encampment, prefix="Namek: Frieza Encampment")
        for i in (11699, 11233, 11234, 11235):
            rezone_room(i, frieza_encampment)
        
        guru_island, _ = create_zone("Guru's Island", parent_id=new_roots["namek"])
        for i in (10142, 10181, 10182, 10183, 10221, 10222, 10223, 10262):
            rezone_room(i, guru_island)

        guru_plains, _ = create_zone("Guru's Plains", parent_id=guru_island)
        reassign_rooms_range(old_zone_id=128, new_zone_id=guru_plains, bottom=12800, top=12815)
        for i in (12820, 12817):
            rezone_room(i, guru_plains)
        
        guru_cliffs, _ = create_zone("Guru's Cliff", parent_id=guru_plains)
        reassign_rooms_range(old_zone_id=100, new_zone_id=guru_cliffs, bottom=11672, top=11683)

        guru_house, _ = create_zone("Guru's House", parent_id=guru_cliffs)
        reassign_rooms_range(old_zone_id=100, new_zone_id=guru_house, bottom=11684, top=11688)

        invaded_island, _ = create_zone("Invaded Island", parent_id=new_roots["namek"])
        for i in (10082, 10122, 10123, 10124, 10163, 10164, 10165, 10166, 10202, 10203, 10204, 10242, 10243, 10244, 10245, 10283, 10284):
            rezone_room(i, invaded_island)

        rename_zone(132, "Frieza's Ship")
        reparent_zone(132, parent_id=invaded_island)

        leviathan_whirlpool, _ = create_zone("Leviathan's Domain", parent_id=100)
        reassign_rooms_range(old_zone_id=132, new_zone_id=leviathan_whirlpool, bottom=13232, top=13289)

        elder_island, _ = create_zone("Elder Island", parent_id=new_roots["namek"])
        for i in (11046, 11086, 11125, 11126, 11127, 11164, 11165, 11166, 11167, 11168, 11204, 11205, 11206, 11207, 11208, 11209, 11245, 11246, 11247, 11248, 11249, 11250, 11286, 11287, 11288, 11289, 11290, 11291, 11325, 11326, 11327, 11330, 11331, 11332, 11364, 11365, 11366, 11371, 11372, 11373, 11375, 11376, 11377, 11404, 11405, 11412, 11413, 11414, 11415, 11416, 11417, 11418, 11453, 11454, 11455, 11456, 11457, 11495, 11496, 11497, 11536):
            rezone_room(i, elder_island)
        
        rename_zone(133, "Elder Village")
        reparent_zone(133, parent_id=elder_island)

        senzu_island, _ = create_zone("Senzu Island", parent_id=new_roots["namek"])
        for i in (10542, 10543, 10583, 10620, 10621, 10623, 10624, 10659, 10660, 10661, 10663, 10664, 10665, 10700, 10701, 10702, 10703, 10704, 10705, 10737, 10738, 10741, 10742, 10743, 10745, 10777, 10778, 10781, 10782, 10783, 10816, 10817, 10818, 10819, 10820, 10821, 10822, 10855, 10856, 10857, 10858, 10859, 10860, 10861, 10894, 10895, 10896, 10899, 10900, 10901, 10902, 10933, 10934, 10935, 10936, 10937, 10939, 10940, 10941, 10942, 10943, 10973, 10974, 10975, 10976, 10977, 10978, 10979, 10980, 10981, 10981, 10982, 10983, 10984, 11015, 11016, 11017, 11018, 11020, 11021, 11022, 11023, 11024, 11025, 11060, 11061, 11063, 11064, 11101, 11103):
            rezone_room(i, senzu_island)

        senzu_village, _ = create_zone("Senzu Village", parent_id=senzu_island)
        reassign_rooms_name_startswith(old_zone_id=100, new_zone_id=senzu_village, prefix="Senzu Village")
        for i in (11666, 11667, 11669, 84):
            rezone_room(i, senzu_village)

        battle_dome, _ = create_zone("Namekian Battle Dome", parent_id=senzu_village)
        reassign_rooms_name_startswith(old_zone_id=100, new_zone_id=battle_dome, prefix="Namekian Battle Dome")

        namekian_advanced_school, _ = create_zone("Namekian Advanced School", parent_id=senzu_village)
        reassign_rooms_range(old_zone_id=100, new_zone_id=namekian_advanced_school, bottom=11689, top=11698)

        crystal_island, _ = create_zone("Crystal Island", parent_id=new_roots["namek"])
        for i in (10316, 10317, 10356, 10357, 10391, 10392, 10394, 10395, 10396, 10431, 10432, 10433, 10434, 10435, 10436, 10437, 10472, 10473, 10474, 10475, 10476, 10477, 10510, 10511, 10512, 10513, 10514, 10515, 10516, 10517, 10552, 10553, 10554, 10593, 10594, 10595, 10633, 10634, 10673):
            rezone_room(i, crystal_island)

        rename_zone(117, "Crystalline Cave")
        for i in (118, 119):
            merge_zones(i, 117)
        reparent_zone(117, parent_id=crystal_island)

        rename_zone(128, "Nail's House and Training Grounds")
        reparent_zone(128, guru_island)
        tranquil_palm_dojo, _ = create_zone("Tranquil Palm Dojo", parent_id=128)
        reassign_rooms_name_startswith(old_zone_id=128, new_zone_id=tranquil_palm_dojo, prefix="Tranquil Palm Dojo")

        reassign_rooms_name_startswith(old_zone_id=128, new_zone_id=260, prefix="Underground Passage")

        monadnock_island, _ = create_zone("Monadnock Island", parent_id=new_roots["namek"])
        for i in (10841, 10881, 10882, 10883, 10921, 10922, 10923, 10962, 10963, 11003):
            rezone_room(i, monadnock_island)

        rename_zone(144, "Kakureta Village")
        merge_zones(154, 144)
        reparent_zone(144, monadnock_island)

        rename_zone(260, "Underground Railroad")
        
        # Working on Afterlife
        celestial_realm, _ = create_zone("Celestial Realm", parent_id=new_roots["afterlife"])

        reparent_zone(60, parent_id=celestial_realm)
        rename_zone(60, "King Yemma's Check-In Station")

        snake_way, _ = create_zone("Snake Way", parent_id=celestial_realm)
        reassign_rooms_name_startswith(old_zone_id=60, new_zone_id=snake_way, prefix="Snake Way")

        reparent_zone(61, parent_id=celestial_realm)
        rename_zone(61, "North Kai's Planet")

        north_kai_home, _ = create_zone("North Kai's Home", parent_id=61)
        for i in (6136, 6137, 6138):
            rezone_room(i, north_kai_home)

        serpent_castle, _ = create_zone("Serpent Castle", parent_id=snake_way)
        reassign_rooms_name_startswith(old_zone_id=61, new_zone_id=serpent_castle, prefix="Serpent Castle")

        infernal_realm, _ = create_zone("Infernal Realm", parent_id=new_roots["afterlife"])
        reparent_zone(62, parent_id=infernal_realm)
        rename_zone(62, "Hell")

        rename_zone(63, "Sands of Time")
        reparent_zone(63, parent_id=infernal_realm)

        rename_zone(64, "Hellfire City")
        reparent_zone(64, parent_id=infernal_realm)
        merge_zones(65, 64)

        for i in (6600, 6699):
            rezone_room(i, 64)

        torture_rack, _ = create_zone("Torture Rack", parent_id=64)
        reassign_rooms_name_startswith(old_zone_id=64, new_zone_id=torture_rack, prefix="Torture")

        flaming_bag_dojo, _ = create_zone("Flaming Bag Dojo", parent_id=64)
        reassign_rooms_name_startswith(old_zone_id=64, new_zone_id=flaming_bag_dojo, prefix="Flaming Bag")
        
        rename_zone(66, "Entrail Graveyard")
        reparent_zone(66, parent_id=infernal_realm)

        rename_zone(68, "Grand Kai's Planet")
        reparent_zone(68, parent_id=celestial_realm)
        merge_zones(69, 68)
        merge_zones(70, 68)

        grand_kai_palace, _ = create_zone("Grand Kai's Palace", parent_id=68)
        reassign_rooms_name_startswith(old_zone_id=68, new_zone_id=grand_kai_palace, prefix="Grand Palace")

        rename_zone(71, "Maze of Echoes")
        reparent_zone(71, parent_id=celestial_realm)

        rename_zone(72, "Dark Catacombs")
        reparent_zone(72, parent_id=71)

        rename_zone(73, "Twilight Caverns")
        reparent_zone(73, parent_id=celestial_realm)
        merge_zones(74, 73)
        
        # The Dead Zone doesn't really belong here but it's kind of here for the moment...
        rename_zone(75, "Dead Zone")
        reparent_zone(75, parent_id=infernal_realm)

        # This is some weird clan special location that still exists as a cool spot.
        rename_zone(217, "Valhalla")
        reparent_zone(217, parent_id=celestial_realm)

        # Working on Kanassa
        rezone_room(177, 149)
        merge_zones(150, 149)
        rename_zone(149, "Aquis City")

        tsuna_school, _ = create_zone("Tsuna's School of Hydro Mastery", parent_id=149)
        reassign_rooms_name_startswith(old_zone_id=149, new_zone_id=tsuna_school, prefix="Tsuna")

        aquis_temple, _ = create_zone("Aquis Temple", parent_id=149)
        reassign_rooms_name_startswith(old_zone_id=149, new_zone_id=aquis_temple, prefix="Aquis Temple")

        aquis_tower, _ = create_zone("Aquis Tower", parent_id=149)
        reassign_rooms_name_startswith(old_zone_id=126, new_zone_id=aquis_tower, prefix="Aquis")

        dark_depths_bar, _ = create_zone("Dark Depths Bar", parent_id=149)
        reassign_rooms_range(old_zone_id=149, new_zone_id=dark_depths_bar, bottom=15055, top=15098)

        rename_zone(151, "Tambrus Ocean")
        reassign_rooms_range(old_zone_id=152, new_zone_id=151, bottom=15200, top=15204)

        rename_zone(152, "Tambrus Sunken Maze")

        dark_underwater_cave, _ = create_zone("Dark Underwater Cave", parent_id=152)
        reassign_rooms_name_startswith(old_zone_id=153, new_zone_id=dark_underwater_cave, prefix="Dark Underwater Cave")

        coldwater_cave, _ = create_zone("Coldwater Cave", parent_id=152)
        reassign_rooms_name_startswith(old_zone_id=153, new_zone_id=coldwater_cave, prefix="Coldwater Cave")

        rename_zone(153, "Ancient Temple")
        reparent_zone(153, parent_id=152)

        kraken, _ = create_zone("Kraken", parent_id=new_roots["space"])
        reassign_rooms_name_startswith(old_zone_id=156, new_zone_id=kraken, prefix="Kraken")

        rename_zone(156, "Yunkai Pirate Base")

        rename_zone(76, "Lost City (Unfinished?)")

        # Working on Cerria
        rename_zone(78, "Orium Cave")
        rename_zone(79, "Crystalline Forest")
        rename_zone(174, "Fistarl Volcano")

        merge_zones(176, 175)

        cerria_city_hall, _ = create_zone("Cerria City Hall", parent_id=175)
        reassign_rooms_name_startswith(old_zone_id=175, new_zone_id=cerria_city_hall, prefix="Cerria City Hall")
        
        # Working on Konack
        rename_zone(80, "Tiranoc City")
        merge_zones(81, 80)
        merge_zones(82, 80)
        rezone_room(86, 80)
        rezone_room(657, 80)

        for x in ("Sereg-Vanma Manor", "Taesal Manor", "Dres Manor", "Indoril Compound", "Ferios Park", "Konack Starport", "Emerald Dreams Tower"):
            x_zone, _ = create_zone(x, parent_id=80)
            reassign_rooms_name_startswith(old_zone_id=80, new_zone_id=x_zone, prefix=x)

        tapion_music_box, _ = create_zone("Tapion's Music Box", parent_id=80)
        for i in (8230, 8231, 8232, 8233):
            rezone_room(i, tapion_music_box)
        
        tiranoc_sewer, _ = create_zone("Tiranoc Sewers", parent_id=80)
        reassign_rooms_range(old_zone_id=80, new_zone_id=tiranoc_sewer, bottom=8234, top=8282)

        rename_zone(83, "Great Oroist Temple")
        reparent_zone(83, 80)

        mazori_farm, _ = create_zone("Mazori Farm", parent_id=new_roots["konack"])
        reassign_rooms_name_startswith(old_zone_id=84, new_zone_id=mazori_farm, prefix="Mazori")
        rename_zone(84, "Elzthuan Forest")

        dres, _ = create_zone("Dres", parent_id=new_roots["konack"])
        reassign_rooms_name_startswith(old_zone_id=85, new_zone_id=dres, prefix="Dres")
        rename_zone(85, "Dirt Road")

        rename_zone(86, "Colvian Farm")

        rename_zone(87, "Saint Alucia")

        rename_zone(88, "Miridius Memorial")

        rename_zone(89, "Desert of Illusion")

        for x in ("Plains of Confusion", "Desolate Stone Path"):
            x_zone, _ = create_zone(x, parent_id=new_roots["konack"])
            reassign_rooms_name_startswith(old_zone_id=89, new_zone_id=x_zone, prefix=x)

        rename_zone(90, "Shadowlas Temple")
        rename_zone(92, "Turlon Fair")
        rename_zone(93, "Veldryth Mountains")
        
        monastery_of_balance, _ = create_zone("Monastery of Balance", parent_id=93)
        reassign_rooms_name_startswith(old_zone_id=93, new_zone_id=monastery_of_balance, prefix="Monastery of Balance")
        rezone_room(9365, monastery_of_balance)
        
        merge_zones(98, 97)

        rename_zone(97, "Jormun Swamp")
        lake_gander, _ = create_zone("Lake Gander", parent_id=new_roots["konack"])
        reassign_rooms_name_startswith(old_zone_id=97, new_zone_id=lake_gander, prefix="Lake Gander")

        kerberos, _ = create_zone(x, parent_id=new_roots["konack"])
        reassign_rooms_name_startswith(old_zone_id=97, new_zone_id=kerberos, prefix="Kerberos")
        rezone_room(9882, kerberos)
        
        shaeras_mansion, _ = create_zone("Shaeras Mansion", parent_id=kerberos)
        reassign_rooms_range(old_zone_id=97, new_zone_id=shaeras_mansion, bottom=9867, top=9882)

        rename_zone(99, "Old Kerberos")
        furion_citadel, _ = create_zone("Furion Citadel", new_roots["konack"])
        reassign_rooms_name_startswith(old_zone_id=99, new_zone_id=furion_citadel, prefix="Furion")

        rename_zone(127, "Machiavila")
        rename_zone(192, "Laron Forest")

        rename_zone(193, "Nazrin Village")

        chieftains_house, _ = create_zone("Chieftain's House", parent_id=193)
        reassign_rooms_name_startswith(old_zone_id=193, new_zone_id=chieftains_house, prefix="Chieften")

        rename_zone(194, "Maze of Shadows")
        reparent_zone(194, chieftains_house)

        # Working on Aether
        rename_zone(120, "Haven City")
        rezone_room(85, 120)
        
        haven_spaceport, _ = create_zone("Haven Spaceport", parent_id=120)
        reassign_rooms_name_startswith(old_zone_id=120, new_zone_id=haven_spaceport, prefix="Haven Spaceport")

        combat_meadow, _ = create_zone("Combat Meadow", parent_id=120)
        reassign_rooms_range(old_zone_id=120, new_zone_id=combat_meadow, bottom=12034, top=12056)
        reassign_rooms_range(old_zone_id=120, new_zone_id=combat_meadow, bottom=12060, top=12079)

        advanced_kinetic_dojo, _ = create_zone("Advanced Kinetic Dojo", parent_id=120)
        reassign_rooms_range(old_zone_id=177, new_zone_id=advanced_kinetic_dojo, bottom=17743, top=17751)

        harmony_park, _ = create_zone("Harmony Park", parent_id=120)
        for i in (120, 121):
            reassign_rooms_name_startswith(old_zone_id=i, new_zone_id=harmony_park, prefix="Harmony Park")
        rename_zone(121, "Serenity Lake")
        reassign_rooms_name_startswith(old_zone_id=122, new_zone_id=121, prefix="Serenity Lake")

        rename_zone(122, "Yakavita Yacht")
        reparent_zone(122, 121)

        rename_zone(123, "Kaiju Forest")

        rename_zone(124, "Ortusian Temple")
        silent_glade, _ = create_zone("Silent Glade", parent_id=new_roots["aether"])
        reassign_rooms_name_startswith(old_zone_id=124, new_zone_id=silent_glade, prefix="Silent Glade")

        rename_zone(125, "Shallow Cave")
        excavation_site, _ = create_zone("Excavation Site", parent_id=125)
        reassign_rooms_name_startswith(old_zone_id=125, new_zone_id=excavation_site, prefix="Excavation Site")

        rename_zone(155, "Captured Aether City")

        # Working on Neo Nirvana
        for i in (135, 136, 137, 138, 139, 145, 146, 147, 148):
            merge_zones(i, 272)

        combat_training_deck, _ = create_zone("Combat Training Deck", parent_id=272)
        reassign_rooms_range(old_zone_id=272, new_zone_id=combat_training_deck, bottom=13520, top=13529)
        reassign_rooms_name_startswith(old_zone_id=272, new_zone_id=combat_training_deck, prefix="Hologram Combat")

        hologram_combat, _ = create_zone("Hologram Combat", parent_id=combat_training_deck)
        
        for x in ("Nexus Field", "Namek: Grassy Island", "Slave Market", "Kanassa: Blasted Battlefield", "Silent Glade", "Hell - Flat Plains", "Sandy Desert", "Topica Snowfield", "Gero's Lab: Obsolete Labs", "Gero's Lab: Bio Android Labs", "Candy Land", "Ancestral Mountains", "Elzthuan Forest", "Yardra City", "Ancient Coliseum"):
            x_zone, _ = create_zone(x, parent_id=hologram_combat)
            reassign_rooms_name_startswith(old_zone_id=272, new_zone_id=x_zone, prefix=x)

        for x in ("Revolution Park", "Akatsuki Labs"):
            x_zone, _ = create_zone(x, parent_id=272)
            reassign_rooms_name_startswith(old_zone_id=272, new_zone_id=x_zone, prefix=x)

        fortran_complex, _ = create_zone("Fortran Complex", parent_id=272)
        reassign_rooms_range(old_zone_id=272, new_zone_id=fortran_complex, bottom=14743, top=14772)

        commercial_deck, _ = create_zone("Commercial Deck", parent_id=272)
        reassign_rooms_range(old_zone_id=272, new_zone_id=commercial_deck, bottom=13509, top=13518)
        reassign_rooms_range(old_zone_id=272, new_zone_id=commercial_deck, bottom=14782, top=14790)

        command_deck, _ = create_zone("Command Deck", parent_id=272)
        reassign_rooms_range(old_zone_id=272, new_zone_id=command_deck, bottom=13500, top=13506)
        reassign_rooms_range(old_zone_id=272, new_zone_id=command_deck, bottom=13547, top=13551)

        # Working on Arlia
        rename_zone(160, "Janacre City")

        janacre_spaceport, _ = create_zone("Janacre Spaceport", parent_id=160)
        reassign_rooms_range(old_zone_id=160, new_zone_id=janacre_spaceport, bottom=16062, top=16068)
        
        rename_zone(126, "Moai's Palace")
        reparent_zone(126, parent_id=160)

        rename_zone(161, "Under Colosseum")
        reparent_zone(161, parent_id=160)

        hall_of_kurzak, _ = create_zone("Hall of Kurzak", parent_id=161)
        reassign_rooms_range(old_zone_id=161, new_zone_id=hall_of_kurzak, bottom=16100, top=16104)

        rename_zone(165, "Yatamari Wasteland")
        rename_zone(166, "Arlia Mines")
        rename_zone(167, "Kilnak Cavern")
        rename_zone(168, "Kemabra Wastes")

        dayou_mountain, _ = create_zone("Dayou Mountain", parent_id=160)
        reassign_rooms_name_startswith(old_zone_id=160, new_zone_id=dayou_mountain, prefix="Dayou Mountain")

        rename_zone(169, "Gaxzixite Hive")

        # Working on Yardrat
        rezone_room(26, 140)

        yardra_spaceport, _ = create_zone("Yardra Spaceport", parent_id=140)
        reassign_rooms_name_startswith(old_zone_id=26, new_zone_id=yardra_spaceport, prefix="Yardra Spaceport")
        
        minto_warrior_academy, _ = create_zone("Minto Warrior Academy", parent_id=140)
        reassign_rooms_name_startswith(old_zone_id=26, new_zone_id=minto_warrior_academy, prefix="Minto Warrior Academy")

        rename_zone(141, "Jade Forest")
        rename_zone(142, "Jade Cliffs")
        jade_cave, _ = create_zone("Jade Cave", parent_id=142)
        reassign_rooms_name_startswith(old_zone_id=142, new_zone_id=jade_cave, prefix="Jade Cave")

        rename_zone(143, "Mount Valaria")
        temperance_temple, _ = create_zone("Temperance Temple", parent_id=143)
        reassign_rooms_name_startswith(old_zone_id=143, new_zone_id=temperance_temple, prefix="Temperance Temple")

        # Working on Space
        for i in (60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 79, 70):
            rezone_room(i, new_roots["space"])

        for i in (65198, 65199):
            rezone_room(i, 256)
        
        rename_zone(170, "Cooler's Ship")

        celestial_corp, _ = create_zone("Celestial Corp", parent_id=172)
        reassign_rooms_name_startswith(old_zone_id=172, new_zone_id=celestial_corp, prefix="Celestial Corp")

        for i in (205, 57, 157, 187):
            reparent_zone(i, to_delete)

        # Miscellaneous
        for i in (157, 171, 29, 39, 47):
            rezone_room(i, to_delete)
        
        rezone_room(46, 224)

        reparent_zone(218, parent_id=new_roots["space"])
        intrepid_base, _ = create_zone("Intrepid Base", parent_id=new_roots["space"])
        reassign_rooms_name_startswith(old_zone_id=218, new_zone_id=intrepid_base, prefix="Intrepid Base")
        forgotten_temple, _ = create_zone("Forgotten Temple", parent_id=218)
        reassign_rooms_name_startswith(old_zone_id=218, new_zone_id=forgotten_temple, prefix="Forgotten Temple")
    
    def migrate_space(self, data_dir: Path):

        with open(data_dir / "surface.map", mode="r", encoding="utf-8", errors="ignore") as f:
            data = f.read()
        self.mapnums = [list(map(int, line.split())) for line in data.splitlines()]

        self.spacemap: dict[int, Coordinates] = dict()
        
        for i in range(len(self.mapnums)):
            row = self.mapnums[i]
            for j in range(len(row)):
                vn = row[j]
                coor = Coordinates(x=int(j - (200 / 2)), y=int((200 / 2) - i), z=0)
                self.spacemap[vn] = coor
        
        a = Area()
        a.vnum = 1
        a.name = "North Galazy"
        a.zone = 232

        s = Shape()
        s.name = "@WDepths of Space@n"
        s.look_description = " @DThis dark void has very little light.  The light it does have comes from\ndistant astral bodies such as stars or planets.  Occasional clouds of gas,\nasteroids, or comets can be seen.  Other than that it is empty blackness for\nthousands upon thousands of miles in every direction.  @n\n"
        s.coordinates = Coordinates(x=0, y=0, z=0)
        s.dimensions = Dimensions(north=100, south=100, east=100, west=100)
        s.priority = 0
        s.sector_type = "space"

        a.shapes["space"] = s

        tile_overrides: dict[Coordinates, Tile] = defaultdict(Tile)

        center_points = (
            ("earth", 40979, "@GE@n", 2),
            ("vegeta", 32365, "@YV@n", 2),
            ("frigid", 30889, "@CF@n", 2),
            ("namek", 42880, "@gN@n", 2),
            ("konack", 27065, "@mK@n", 2),
            ("aether", 41959, "@BA@n", 2),
            ("yardrat", 34899, "@MY@n", 2),
            ("kanassa", 53859, "@CK@n", 2),
            ("cerria", 59071, "@MC@n", 2),
            ("arlia", 52434, "@mA@n", 2),
            ("zenith", 50772, "@cZ@n", 1)
        )

        for name, vnum, symbol, radius in center_points:
            coor = self.spacemap[vnum]
            shape = Shape()
            shape.type = "round"
            shape.coordinates = coor
            shape.dimensions = Dimensions(north=radius, south=radius, east=radius, west=radius)
            shape.priority = 1
            shape.tile_display = symbol
            shape.sector_type = SectorType.space
            a.shapes[name] = shape
        
        for k, v in self.rooms.items():
            if (in_space := self.spacemap.get(k, None)) is not None:
                # The room is in "old space". Our goal is to scan for exits that lead somewhere NOT in self.spacemap,
                # and if we find one we need to get-or-create a tile_override and create a mirroring exit.
                tile = Tile()
                over = False
                for direction, ex in v.exits.items():
                    if ex.destination.target in self.spacemap:
                        continue
                    # This exit leads somewhere outside of space. We need to create a mirror exit in the spacemap.
                    # since it's a defaultdict, we'll just get a dictionary!
                    tile.exits[direction] = ex
                    over = True
                
                if v.reset_commands:
                    tile.reset_commands = v.reset_commands
                    over = True

                if v.name != "@WDepths of Space@n":
                    tile.name = v.name
                    over = True
                
                if v.look_description != s.look_description:
                    tile.look_description = v.look_description
                    over = True
                
                for wf, disp in EXTRA_TILES.items():
                    if wf in v.where_flags:
                        tile.tile_display = disp
                        over = True
                        break

                if over:
                    tile_overrides[in_space] = tile
            else:
                # This room isn't part of "old space", but it MIGHT have an exit that leads into it.
                for direction, ex in v.exits.items():
                    if ex.destination.target in self.spacemap:
                        # This exit leads into space. We need to alter its destination to the appropriate coordinates.
                        coor = self.spacemap[ex.destination.target]
                        ex.destination.target = 1
                        ex.destination.type = "area"
                        ex.destination.coordinates = coor
            
            a.tiles.update(tile_overrides)
        
        self.areas[1] = a

        for vnum in self.spacemap.keys():
            self.rooms.pop(vnum, None)

    def geometry_check_zone(self, zone_id: int, origin_id: int | None = None) -> tuple[bool, list[str]]:
        """
        Given a Zone ID and an optional room vnum within the zone, determine if the Zone is 
        geometrically consistent according to the following logic:

        - The rooms for the Zone are treated as a graph. The graph must be fully traversable.
        - Exits that lead to other Zones will be ignored.
        - Any exits to "inside" or "outside" must lead outside the Zone.
        - An origin (0,0,0) point will be either provided or selected randomly from the Zone's Rooms. 
          All directional exits will be traversed and plotted onto a coordinate grid

        Things to beware of:
        - Inconsistent geometry: two rooms should not occupy the same coordinates.
        - Inconsistent geometry: after traversing an exit north, the south exit should always lead back to where one just was, same with other reversibles.
        - Isolated rooms: all rooms should be reachable from the origin room. Report any unreachable rooms.
        
        Returns:
        - A boolean indicating whether the Zone is geometrically consistent.
        - A list of strings describing any inconsistencies found.
        
        """
        rooms: list[Room] = self._rooms_for(zone_id)

        if not rooms:
            return [True, []]

        grid: dict[Coordinates, Room] = dict()
        visited: set[int] = set()
        origin: Room = self.rooms[origin_id] if origin_id is not None else random.choice(rooms)
        errors: list[str] = list()

        # first let's make sure that any inside/outside exits always lead to other zones.
        for r in rooms:
            for d, ex in r.exits.items():
                if d not in (Direction.inside, Direction.outside):
                    continue
                if ex.destination.type != "room":
                    continue
                target = self.rooms.get(ex.destination.target, None)
                if target.zone == zone_id:
                    errors.append(f"Room {r.vnum} has an exit {d.name} that leads to room {ex.destination.target} which is in the same zone!")

        queue: deque[tuple[Room, Coordinates]] = deque([(origin, Coordinates(0, 0, 0))])

        while queue:
            room, coor = queue.popleft()
            if room.vnum in visited:
                continue
            visited.add(room.vnum)

            if coor in grid:
                errors.append(f"Rooms {room.vnum} and {grid[coor].vnum} occupy the same coordinates {coor}")
            else:
                grid[coor] = room
            
            for d, ex in room.exits.items():
                if ex.destination.type != "room":
                    continue
                target = self.rooms.get(ex.destination.target, None)
                if not target:
                    continue
                if target.zone != zone_id:
                    continue
                
                new_coor = d.update_coordinates(coor)
                queue.append((target, new_coor))

                # Check for reversibility
                opp = d.opposite()
                if opp not in target.exits:
                    errors.append(f"Exit {d.name} from room {room.vnum} to room {target.vnum} is not reversible (missing {opp.name} exit)")
                else:
                    opp_ex = target.exits[opp]
                    if opp_ex.destination.type != "room" or opp_ex.destination.target != room.vnum:
                        errors.append(f"Exit {d.name} from room {room.vnum} to room {target.vnum} is not reversible (opposite exit does not lead back)")
        
        # check for unreachable rooms
        for r in rooms:
            if r.vnum not in visited:
                errors.append(f"Room {r.vnum} is unreachable from origin room {origin.vnum}")
        
        return len(errors) == 0, errors

    def check_geometry(self):
        all_zones = len(self.zones)
        consistent_zones = 0
        inconsistent: set[int] = set()
        
        for i in self.zones.keys():
            if result := self.geometry_check_zone(i):
                if result[0]:
                    consistent_zones += 1
                else:
                    inconsistent.add(i)

        print(f"Percentage of Consistent Zones: {consistent_zones / all_zones * 100:.2f}%")
        if inconsistent:
            print(f"Inconsistent Zones: {inconsistent}")

def prepare_migration(path: Path) -> LegacyDatabase:
    db = LegacyDatabase()
    db.load_from_files(path)
    return db

def test() -> LegacyDatabase:
    path = Path("data")
    return prepare_migration(path)