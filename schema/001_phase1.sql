-- This is using sqlite3 syntax.

INSERT INTO migrations (name) VALUES ('001_phase1.sql');

CREATE TABLE zones (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL DEFAULT '',
    -- space separated list of builder names, for permissions
    builders TEXT NOT NULL DEFAULT '',
    -- how long in real time minutes between zone resets,
    lifespan INTEGER NOT NULL DEFAULT 30,
    age INTEGER NOT NULL DEFAULT 0,
    -- the vnum of the bottom of the zone, inclusive
    bot INTEGER NOT NULL,
    -- the vnum of the top of the zone, inclusive
    top INTEGER NOT NULL,
    -- enum for how the zone reset should be handled.
    -- 0: don't, 1: only if no players are in zone, 2: always.
    reset_mode INTEGER NOT NULL DEFAULT 2,
    -- minimum level for players to enter this zone
    min_level INTEGER NOT NULL DEFAULT 0,
    -- maximum level for players to enter this zone, 0 disables checks
    max_level INTEGER NOT NULL DEFAULT 0,
    -- 128-bit bitfield for zone flags
    zone_flags BLOB NOT NULL DEFAULT 0
);


CREATE TABLE zone_reset_commands (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    zone_id INTEGER NOT NULL REFERENCES zones(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- this maintains the order to check commands in, per zone.
    command_order INTEGER NOT NULL,
    -- 'M' for mobile, 'O' for object, 'E' for equip, etc, * is disabled/nothing
    command_type TEXT NOT NULL DEFAULT '*',
    -- if this command should be executed only if the previous command succeeded
    if_flag INTEGER NOT NULL DEFAULT 0,
    -- meaning depends on command type
    arg1 INTEGER NOT NULL DEFAULT 0,
    arg2 INTEGER NOT NULL DEFAULT 0,
    arg3 INTEGER NOT NULL DEFAULT 0,
    arg4 INTEGER NOT NULL DEFAULT 0,
    arg5 INTEGER NOT NULL DEFAULT 0,

    -- These don't seem to actually be USED in any existing game data.
    -- But they are in the struct.
    sarg1 TEXT NOT NULL DEFAULT '',
    sarg2 TEXT NOT NULL DEFAULT '',
    UNIQUE(zone_id, command_order)
);

CREATE TABLE dgscript_prototypes(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    zone_id INTEGER NOT NULL REFERENCES zones(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    -- enum for what type of thing this script can be attached to
    -- 0: mob, 1: object 2: room
    attach_type INTEGER NOT NULL DEFAULT 0,
    -- is this even used? it doesn't look like it is. No code use is found.
    data_type INTEGER NOT NULL DEFAULT 0,
    -- name can contain color codes.
    name TEXT NOT NULL DEFAULT '',
    -- uint32_t bitfield for what type of trigger this script is, e.g. command, random, etc,
    -- Specific meaning varies by attach_type.
    -- Full list found in dg_script.h
    trigger_type INTEGER NOT NULL DEFAULT 0,
    -- the actual script code. Code is line-based.
    body TEXT NOT NULL,
    -- number of args this script takes, for command triggers
    -- has other meaning for time scripts or direction checks.
    narg INTEGER NOT NULL DEFAULT 0,
    -- space separated list of arg names, for command triggers
    arglist TEXT NOT NULL DEFAULT ''
);

CREATE TABLE object_prototypes(
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    zone_id INTEGER NOT NULL REFERENCES zones(id) ON DELETE RESTRICT ON UPDATE CASCADE,

    -- space separated keywords for searching
    name TEXT NOT NULL DEFAULT '',
    -- the display name of object, can contain color codes
    short_description TEXT NOT NULL DEFAULT '',
    -- the long description of object for when looked at, can contain color codes
    description TEXT NOT NULL DEFAULT '',
    -- The line shown when this object is in a room. Can contain color codes.
    action_description TEXT NOT NULL DEFAULT '',
    -- bitfield for extra flags like glow, hum, etc
    extra_flags BLOB NOT NULL DEFAULT 0,
    -- bitfield for where this object can be worn,
    wear_flags BLOB NOT NULL DEFAULT 0,
    -- bitfield for object properties like magic, invisible, etc
    bitvector BLOB NOT NULL DEFAULT 0,

    type_flag INTEGER NOT NULL DEFAULT 0,

    weight INTEGER NOT NULL DEFAULT 0,
    cost INTEGER NOT NULL DEFAULT 0,

    -- level needed to use the object.
    level INTEGER NOT NULL DEFAULT 0,
    size INTEGER NOT NULL DEFAULT 0
);

-- item values 0-15 for prototypes.
-- the meaning varies based on item type.
CREATE TABLE object_prototype_item_values (
    object_prototype_id INTEGER NOT NULL REFERENCES object_prototypes(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- from 0 to 15, the index of this value in the item_values sequence
    val_index INTEGER NOT NULL,
    -- the integer value
    val_number INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY(object_prototype_id, val_index)
);

CREATE TABLE object_prototype_extra_descriptions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    object_prototype_id INTEGER NOT NULL REFERENCES object_prototypes(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- keywords should not contain color codes.
    keywords TEXT NOT NULL DEFAULT '',
    -- description can, though.
    description TEXT NOT NULL DEFAULT ''
);

-- In memory this is a list, and the order of script_ids DOES matter.
CREATE TABLE object_prototype_dgscript_prototypes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    object_prototype_id INTEGER NOT NULL REFERENCES object_prototypes(id) ON DELETE CASCADE ON UPDATE CASCADE,
    script_id INTEGER NOT NULL REFERENCES dgscript_prototypes(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    UNIQUE(object_prototype_id, script_id)
);

-- Objects can have up to 6 "affects".
CREATE TABLE object_prototype_affects (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    object_prototype_id INTEGER NOT NULL REFERENCES object_prototypes(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- enum for what this affect modifies, e.g. strength, dexterity, etc
    location INTEGER NOT NULL DEFAULT 0,
    -- how much this affect modifies the location by
    modifier INTEGER NOT NULL DEFAULT 0,
    -- bitfield for affect flags, e.g. bless, curse, etc
    specific INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE mobile_prototypes(
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    zone_id INTEGER NOT NULL REFERENCES zones(id) ON DELETE RESTRICT ON UPDATE CASCADE,

    -- space separated keywords for searching, should not contain color codes.
    name TEXT NOT NULL DEFAULT '',
    -- the display name of the mob, can contain color codes.
    short_descr TEXT NOT NULL DEFAULT '',
    -- the long description of the mob for when seen in rooms.
    long_descr TEXT NOT NULL DEFAULT '',
    -- the description of the mob for when looked at, can contain color codes.
    description TEXT NOT NULL DEFAULT '',
    size INTEGER NOT NULL DEFAULT 0,
    -- enum for sex. db/consts/sex.h
    sex INTEGER NOT NULL DEFAULT 0,
    -- enum for race. db/consts/races.h
    race INTEGER NOT NULL DEFAULT 0,
    -- enum for sensei. db/consts/sensei.h
    chclass INTEGER NOT NULL DEFAULT 0,

    -- bitfield for mob flags like sentinel, aggressive, etc.
    -- db/consts/mobflags.h
    act BLOB NOT NULL DEFAULT 0,
    -- bitfield for affect flags like bless, curse, etc
    -- db/consts/affects.h
    affected_by BLOB NOT NULL DEFAULT 0,

    -- ranges from -1000 to +1000, for evil to good. db/consts/aligns.h
    alignment INTEGER NOT NULL DEFAULT 0,

    str INTEGER NOT NULL DEFAULT 0,
    intel INTEGER NOT NULL DEFAULT 0,
    wis INTEGER NOT NULL DEFAULT 0,
    -- dex is labeled agility in code, derp.
    dex INTEGER NOT NULL DEFAULT 0,
    con INTEGER NOT NULL DEFAULT 0,
    -- cha is actually spd in code. because derp.
    cha INTEGER NOT NULL DEFAULT 0,

    position INTEGER NOT NULL DEFAULT 0,

    level INTEGER NOT NULL DEFAULT 0,
    race_level INTEGER NOT NULL DEFAULT 0,
    level_adj INTEGER NOT NULL DEFAULT 0,
    
    armor INTEGER NOT NULL DEFAULT 0,

    basepl INTEGER NOT NULL DEFAULT 0,
    baseki INTEGER NOT NULL DEFAULT 0,
    basest INTEGER NOT NULL DEFAULT 0,
    
    gold INTEGER NOT NULL DEFAULT 0,

    -- anything below this isn't to be included in the instances...
    default_position INTEGER NOT NULL DEFAULT 0

);

-- In memory this is a list, and the order of script_ids DOES matter.
CREATE TABLE mobile_prototype_dgscript_prototypes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    mobile_prototype_id INTEGER NOT NULL REFERENCES mobile_prototypes(id) ON DELETE CASCADE ON UPDATE CASCADE,
    script_id INTEGER NOT NULL REFERENCES dgscript_prototypes(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    UNIQUE(mobile_prototype_id, script_id)
);

CREATE TABLE rooms (
    -- room IDs are handled in code, no autoincrement right now
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    zone_id INTEGER NOT NULL REFERENCES zones(id) ON DELETE RESTRICT ON UPDATE CASCADE,

    -- An enum.
    sector_type INTEGER NOT NULL DEFAULT 0,
    name TEXT NOT NULL DEFAULT '',
    description TEXT NOT NULL DEFAULT '',
    -- bitfield for room flags like indoors, no-mob, etc
    -- check db/consts/roomflags.h
    room_flags BLOB NOT NULL DEFAULT 0,

    -- Included below in preparation for phase 2.
    light INTEGER NOT NULL DEFAULT 0,
    timed INTEGER NOT NULL DEFAULT -1,
    dmg INTEGER NOT NULL DEFAULT 0,
    gravity INTEGER NOT NULL DEFAULT 0,
    geffect INTEGER NOT NULL DEFAULT 0
);

-- IN memory this is a linked list, but its order doesn't matter.
CREATE TABLE room_extra_descriptions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_id INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE ON UPDATE CASCADE,
    keywords TEXT NOT NULL DEFAULT '',
    description TEXT NOT NULL DEFAULT ''
);

-- In memory this is a list, and the order of script_ids DOES matter.
CREATE TABLE room_dgscript_prototypes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_id INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE ON UPDATE CASCADE,
    script_id INTEGER NOT NULL REFERENCES dgscript_prototypes(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    UNIQUE(room_id, script_id)
);

-- If a room has scripts, normally they are checked in the order of prototypes.
-- But if script is added or removed during runtime (but not to the prototypes),
-- then we need a separate listing.
-- created in preparation for phase 2
CREATE TABLE room_dgscript_order (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_id INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE ON UPDATE CASCADE,
    script_id INTEGER NOT NULL REFERENCES dgscript_prototypes(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    script_order INTEGER NOT NULL,
    UNIQUE(room_id, script_id),
    UNIQUE(room_id, script_order)
);

-- created in preparation for phase 2
CREATE TABLE room_dgscript_variables(
    room_id INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE ON UPDATE CASCADE,
    variable_name TEXT NOT NULL,
    variable_value TEXT NOT NULL,
    PRIMARY KEY(room_id, variable_name)
);

CREATE TABLE exits(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_id INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- enum for direction. Defined in db/consts/directions.h
    direction INTEGER NOT NULL DEFAULT 0,
    to_room INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE ON UPDATE CASCADE,
    keywords TEXT NOT NULL DEFAULT '',
    description TEXT NOT NULL DEFAULT '',
    -- the object ID of a key that unlocks this door. nullable here, -1 if none in code.
    key_id INTEGER NULL REFERENCES object_prototypes(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    -- small bitfield for exit flags, e.g. locked, hidden, etc
    exit_info INTEGER NOT NULL DEFAULT 0,
    -- difficulty for various things. Some aren't used.
    dclock INTEGER NOT NULL DEFAULT 0,
    dchide INTEGER NOT NULL DEFAULT 0,
    dcskill INTEGER NOT NULL DEFAULT 0,
    dcmove INTEGER NOT NULL DEFAULT 0,

    -- failsave stuff isn't actually used in current game.
    -- I think this is for 'if you fail to pick a lock, a trapdoor opens'
    -- and drops you in a dungeon cell kind of logic.
    failsavetype INTEGER NOT NULL DEFAULT 0,
    dcfailsave INTEGER NOT NULL DEFAULT 0,
    failroom INTEGER NULL REFERENCES rooms(id) ON DELETE SET NULL ON UPDATE CASCADE,
    totalfailroom INTEGER NULL REFERENCES rooms(id) ON DELETE SET NULL ON UPDATE CASCADE,

    -- a room can only have one "east" or "north" exit, etc.
    UNIQUE(room_id, direction)
);


CREATE TABLE guilds (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    zone_id INTEGER NOT NULL REFERENCES zones(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    charge FLOAT NOT NULL DEFAULT 1.0,
    -- String messages for interactions.
    no_such_skill TEXT NOT NULL DEFAULT '',
    not_enough_gold TEXT NOT NULL DEFAULT '',
    minlvl INTEGER NOT NULL DEFAULT 0,
    -- Guilds shouldn't ever BE without a gm, but may in existence currently are, so foreign key is flexible.
    gm INTEGER NULL DEFAULT NULL REFERENCES mobile_prototypes(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    -- bitfield for which classes/races can use it.
    with_who BLOB NOT NULL DEFAULT 0,
    -- enum for when the guild is open, e.g. always, night, etc
    open INTEGER NOT NULL DEFAULT 0,
    -- enum for when the guild is closed, e.g. always, night, etc
    close INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE guild_skills (
    guild_id INTEGER NOT NULL REFERENCES guilds(id) ON DELETE CASCADE ON UPDATE CASCADE,
    skill_id INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE guild_feats(
    guild_id INTEGER NOT NULL REFERENCES guilds(id) ON DELETE CASCADE ON UPDATE CASCADE,
    feat_id INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE shops (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    zone_id INTEGER NOT NULL REFERENCES zones(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    profit_buy FLOAT NOT NULL DEFAULT 1.0,
    profit_sell FLOAT NOT NULL DEFAULT 1.0,
    no_such_item1 TEXT NOT NULL DEFAULT '',
    no_such_item2 TEXT NOT NULL DEFAULT '',
    missing_cash1 TEXT NOT NULL DEFAULT '',
    missing_cash2 TEXT NOT NULL DEFAULT '',
    do_not_buy TEXT NOT NULL DEFAULT '',
    message_buy TEXT NOT NULL DEFAULT '',
    message_sell TEXT NOT NULL DEFAULT '',
    -- try as I might I can't find a temper2. why is this temper1?
    temper1 INTEGER NOT NULL DEFAULT 0,
    -- bitfield for shop flags, e.g. keepers don't attack, etc
    bitvector INTEGER NOT NULL DEFAULT 0,
    -- this SHOULDN'T be null, I would rather this be NOT NULL, but some shops don't have keepers right now.
    keeper INTEGER NULL DEFAULT NULL REFERENCES mobile_prototypes(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    -- bitfield for which classes/races can use it.
    with_who BLOB NOT NULL DEFAULT 0,
    -- enum for when the shop opens, e.g. always, night, etc
    open1 INTEGER NOT NULL DEFAULT 0,
    -- enum for when the shop closes
    close1 INTEGER NOT NULL DEFAULT 0,
    -- enum for when the shop opens, e.g. always, night, etc
    open2 INTEGER NOT NULL DEFAULT 0,
    -- enum for when the shop closes, etc
    close2 INTEGER NOT NULL DEFAULT 0,
    -- if a keeper has over this amount of money, they'll 'deposit' (get rid of) the extra. This limits thievery.
    bankAccount INTEGER NULL REFERENCES users(id) ON DELETE SET NULL ON UPDATE CASCADE
);

CREATE TABLE shop_products (
    shop_id INTEGER NOT NULL REFERENCES shops(id) ON DELETE CASCADE ON UPDATE CASCADE,
    object_prototype_id INTEGER NOT NULL REFERENCES object_prototypes(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    PRIMARY KEY(shop_id, object_prototype_id)
);

CREATE TABLE shop_rooms (
    shop_id INTEGER NOT NULL REFERENCES shops(id) ON DELETE CASCADE ON UPDATE CASCADE,
    room_id INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE ON UPDATE CASCADE,
    PRIMARY KEY(shop_id, room_id)
);

CREATE TABLE shop_buys (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    shop_id INTEGER NOT NULL REFERENCES shops(id) ON DELETE CASCADE ON UPDATE CASCADE,
    item_type INTEGER NOT NULL DEFAULT 0,
    -- for shops that buy specific items based on keywords rather than object ID.
    keywords TEXT NOT NULL DEFAULT '',
    UNIQUE(shop_id, item_type, keywords)
);

CREATE TABLE help_entries (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    keywords TEXT NOT NULL DEFAULT '',
    entry TEXT NOT NULL DEFAULT '',
    min_level INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE assemblies (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    -- the mobile prototype that this assembly creates, if any. Not all assemblies create mobs, but most do.
    object_prototype_id INTEGER NOT NULL REFERENCES object_prototypes(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    assembly_type INTEGER NOT NULL
);

CREATE TABLE assembly_ingredients (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    assembly_id INTEGER NOT NULL REFERENCES assemblies(id) ON DELETE CASCADE ON UPDATE CASCADE,
    object_prototype_id INTEGER NOT NULL REFERENCES object_prototypes(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    -- whether this ingredient is consumed in the assembly process or not
    consumed INTEGER NOT NULL DEFAULT 1,
    -- whether this ingredient needs to be in the same room as the assembler or not
    in_room INTEGER NOT NULL DEFAULT 0
);


CREATE TABLE users (
    -- Previously users didn't have IDS.
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE COLLATE NOCASE,
    email TEXT NOT NULL DEFAULT '',
    -- Noting that existing passwords are plain text, god damn it.
    -- TODO: Investigate Zig's crypto options.
    password_hash TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    admin_level INTEGER DEFAULT 0,
    rpp_points INTEGER DEFAULT 0,
    rpp_bank INTEGER DEFAULT 0,
    -- How much RPP this user has earned overall.
    -- It doesn't decrease when they spend.
    rpp_awarded INTEGER DEFAULT 0,
    -- I would like to change this to extra_slots and have a global base default.
    -- But just mirroring what currently exists.
    extra_slots INTEGER DEFAULT 0
);

-- Used for player characters right now, but ready for others....
CREATE TABLE characters (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    -- the existing game has a 'generation' field as a time_t used for dupe checks, but that's not needed
    -- because sqlite already solves that problem...
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    -- whether this character is active in the game or not. 
    -- Inactive characters are not part of the game simulation. 
    -- Example: logged off player characters.
    in_game INTEGER NOT NULL DEFAULT 0,

    -- Not all saved characters HAVE users, only player characters... which are currently the only kind of 'saved character'
    user_id INTEGER NULL REFERENCES users(id) ON DELETE CASCADE ON UPDATE CASCADE,

    -- In no situation should both user_id and mobile_prototype_id be set. Just one of them, or possibly neither.
    mobile_prototype_id INTEGER NULL REFERENCES mobile_prototypes(id) ON DELETE RESTRICT ON UPDATE CASCADE,

    -- ALL MOBILE PROTOTYPE DATA HAS TO BE DUPLICATED HERE...
    -- space separated keywords for searching, should not contain color codes.
    name TEXT NOT NULL DEFAULT '',
    -- the display name of the mob, can contain color codes.
    short_descr TEXT NOT NULL DEFAULT '',
    -- the long description of the mob for when seen in rooms.
    long_descr TEXT NOT NULL DEFAULT '',
    -- the description of the mob for when looked at, can contain color codes.
    description TEXT NOT NULL DEFAULT '',
    size INTEGER NOT NULL DEFAULT 0,
    -- enum for sex. db/consts/sex.h
    sex INTEGER NOT NULL DEFAULT 0,
    -- enum for race. db/consts/races.h
    race INTEGER NOT NULL DEFAULT 0,
    -- enum for sensei. db/consts/sensei.h
    chclass INTEGER NOT NULL DEFAULT 0,

    -- bitfield for mob flags like sentinel, aggressive, etc.
    -- db/consts/mobflags.h
    act BLOB NOT NULL DEFAULT 0,
    -- bitfield for affect flags like bless, curse, etc
    -- db/consts/affects.h
    affected_by BLOB NOT NULL DEFAULT 0,

    -- ranges from -1000 to +1000, for evil to good. db/consts/aligns.h
    alignment INTEGER NOT NULL DEFAULT 0,

    str INTEGER NOT NULL DEFAULT 0,
    intel INTEGER NOT NULL DEFAULT 0,
    wis INTEGER NOT NULL DEFAULT 0,
    -- dex is labeled agility in code, derp.
    dex INTEGER NOT NULL DEFAULT 0,
    con INTEGER NOT NULL DEFAULT 0,
    -- cha is actually spd in code. because derp.
    cha INTEGER NOT NULL DEFAULT 0,

    position INTEGER NOT NULL DEFAULT 0,

    level INTEGER NOT NULL DEFAULT 0,
    race_level INTEGER NOT NULL DEFAULT 0,
    level_adj INTEGER NOT NULL DEFAULT 0,
    
    armor INTEGER NOT NULL DEFAULT 0,
    speedboost INTEGER NOT NULL DEFAULT 0,
    armor_last INTEGER NOT NULL DEFAULT 0,
    accuracy INTEGER NOT NULL DEFAULT 0,

    -- Admin stuff.
    admlevel INTEGER NOT NULL DEFAULT 0,
    admflags BLOB NOT NULL DEFAULT 0,

    -- base stats
    basepl INTEGER NOT NULL DEFAULT 0,
    baseki INTEGER NOT NULL DEFAULT 0,
    basest INTEGER NOT NULL DEFAULT 0,
    
    gold INTEGER NOT NULL DEFAULT 0,

    -- END MOBILE PROTOTYPE DATA SECTION

    -- cosmetic stuff related to character appearance.
    hairl INTEGER NOT NULL DEFAULT 0,
    hairc INTEGER NOT NULL DEFAULT 0,
    hairs INTEGER NOT NULL DEFAULT 0,
    skin INTEGER NOT NULL DEFAULT 0,
    eye INTEGER NOT NULL DEFAULT 0,
    distfea INTEGER NOT NULL DEFAULT 0,
    aura INTEGER NOT NULL DEFAULT 0,
    voice TEXT NOT NULL DEFAULT '',
    -- the special RPP 'feature' addition some players buy.
    feature TEXT NOT NULL DEFAULT '',
    title TEXT NOT NULL DEFAULT '',

    rdisplay TEXT NOT NULL DEFAULT '',

    hometown INTEGER NULL REFERENCES rooms(id) ON DELETE SET NULL ON UPDATE CASCADE,
    -- time/date stuff in time_data... skipping for now...

    height INTEGER NOT NULL DEFAULT 0,
    weight INTEGER NOT NULL DEFAULT 0,

    -- the character that is master to this char, if any
    -- this is for special charm affect, pet ownership, etc.
    master_id INTEGER NULL REFERENCES characters(id) ON DELETE SET NULL ON UPDATE CASCADE,

    -- bitfield for body parts like head, arms, legs, etc
    bodyparts BLOB NOT NULL DEFAULT 0,

    -- the character's money.
    bank_gold INTEGER NOT NULL DEFAULT 0,
    lastint INTEGER NOT NULL DEFAULT 0,

    -- accrued experience towards next level.
    exp INTEGER NOT NULL DEFAULT 0,
    -- the current upgrade points for androids.
    upgrade INTEGER NOT NULL DEFAULT 0,
    practices INTEGER NOT NULL DEFAULT 0,

    -- the character's current vital levels, as meters.
    health FLOAT NOT NULL DEFAULT 1.0,
    energy FLOAT NOT NULL DEFAULT 1.0,
    stamina FLOAT NOT NULL DEFAULT 1.0,
    lifeforce FLOAT NOT NULL DEFAULT 1.0,

    -- ship stuff
    ping INTEGER NOT NULL DEFAULT 0,
    radar1 INTEGER NULL REFERENCES rooms(id) ON DELETE SET NULL ON UPDATE CASCADE,
    radar2 INTEGER NULL REFERENCES rooms(id) ON DELETE SET NULL ON UPDATE CASCADE,
    radar3 INTEGER NULL REFERENCES rooms(id) ON DELETE SET NULL ON UPDATE CASCADE,

    -- the room a character died in, if applicable. Used for resurrections maybe.
    -- -1 in game, nullable here
    droom INTEGER NULL REFERENCES rooms(id) ON DELETE SET NULL ON UPDATE CASCADE,
    -- how many times this character has died, for death mechanics and such.
    dcount INTEGER NOT NULL DEFAULT 0,
    -- enum for how the character died, for death mechanics and such.
    death_type INTEGER NOT NULL DEFAULT 0,
    -- the time of the character's death, for the death mechanic.
    deathtime INTEGER NOT NULL DEFAULT 0,

    -- the current kaioken level of the character, if any. 0 if not kaioken'd up.
    kaioken INTEGER NOT NULL DEFAULT 0,

    -- used for fly higher
    altitude INTEGER NOT NULL DEFAULT 0,

    -- how many absorbs the character has done for majins/bios.
    absorbs INTEGER NOT NULL DEFAULT 0,
    -- how many boosts the character has
    boosts INTEGER NOT NULL DEFAULT 0,
    blesslvl INTEGER NOT NULL DEFAULT 0,
    ingestLearned INTEGER NOT NULL DEFAULT 0,
    asb INTEGER NOT NULL DEFAULT 0,
    regen INTEGER NOT NULL DEFAULT 0,
    
    -- how much base PL was gained by majinize, if the character is majinized.
    majinize INTEGER NOT NULL DEFAULT 0,
    majinizer INTEGER NULL REFERENCES characters(id) ON DELETE SET NULL ON UPDATE CASCADE,

    -- the current suppression level of the character, for the suppression mechanic.
    suppression INTEGER NOT NULL DEFAULT 0,

    -- the ID of the race this character is mimicking, if any.
    -- used only by Hoshijins.
    mimic INTEGER NOT NULL DEFAULT 0,

    -- transformation costs class. is 1, 2, or 3
    transclass INTEGER NOT NULL DEFAULT 2,
    preference INTEGER NOT NULL DEFAULT 0,

    -- arlian molt stuff
    -- the amount of experience this character has put into their mobile level, for the mobile level mechanic.
    moltexp INTEGER NOT NULL DEFAULT 0,
    -- the current mobile level of the character, for the mobile level mechanic.
    moltlevel INTEGER NOT NULL DEFAULT 0,

    -- Saiyan and halfbreed and tail-y things
    tail_growth INTEGER NOT NULL DEFAULT 0,
    rage_meter INTEGER NOT NULL DEFAULT 0,
    -- halfbreed fury meter
    fury INTEGER NOT NULL DEFAULT 0,
    
    -- skill stuff
    -- whether the character is currently forgetting a skill, for the skill forget mechanic.
    forgetting INTEGER NOT NULL DEFAULT 0,
    -- how many skills the character has forgotten, for the skill forget mechanic.
    forgetcount INTEGER NOT NULL DEFAULT 0,
    skill_slots INTEGER NOT NULL DEFAULT 0,

    -- time stuff
    time_birth INTEGER NOT NULL DEFAULT 0,
    time_created INTEGER NOT NULL DEFAULT 0,
    time_logon INTEGER NOT NULL DEFAULT 0,
    time_maxage INTEGER NOT NULL DEFAULT 0,
    time_played INTEGER NOT NULL DEFAULT 0,
    lastpl INTEGER NOT NULL DEFAULT 0,

    -- cooldowns
    con_cooldown INTEGER NOT NULL DEFAULT 0,
    relax_count INTEGER NOT NULL DEFAULT 0,
    cooldown INTEGER NOT NULL DEFAULT 0,
    con_sdcooldown INTEGER NOT NULL DEFAULT 0,
    gooptime INTEGER NOT NULL DEFAULT 0,

    olc_zone INTEGER NULL REFERENCES zones(id) ON DELETE SET NULL ON UPDATE CASCADE,

    -- food, drink sleep
    sleeptime INTEGER NOT NULL DEFAULT 0,
    foodr INTEGER NOT NULL DEFAULT 0,
    overf INTEGER NOT NULL DEFAULT 0,

    -- Preferences
    lifeperf INTEGER NOT NULL DEFAULT 0, -- at what level of health% does lifeforce kick in?

    poofin TEXT NOT NULL DEFAULT '',
    poofout TEXT NOT NULL DEFAULT '',
    -- the character ID of the last person to tell this character something, for the reply command and such.
    last_tell INTEGER NULL REFERENCES characters(id) ON DELETE SET NULL ON UPDATE CASCADE,
    -- the health% at which the character will automatically flee from combat, for the wimp mechanic.
    wimp_level INTEGER NOT NULL DEFAULT 0,
    -- the admin level required to freeze this character, for the freeze command and such.
    freeze_level INTEGER NOT NULL DEFAULT 0,
    -- the admin level required to see this character when they are invisible, for the invis command and such.
    invis_level INTEGER NOT NULL DEFAULT 0,
    -- the room this character will load into when they log in, for the loadroom command and such.
    load_room INTEGER NULL REFERENCES rooms(id) ON DELETE SET NULL ON UPDATE CASCADE,
    -- bitfield for various player preferences, e.g. auto-loot, auto-sac, etc
    pref BLOB NOT NULL DEFAULT 0,
    -- the number of lines per page for this character, for the page command and such.
    page_length INTEGER NOT NULL DEFAULT 0,
    -- how many players has this player killed?
    murder INTEGER NOT NULL DEFAULT 0,
    trainstr INTEGER NOT NULL DEFAULT 0,
    trainint INTEGER NOT NULL DEFAULT 0,
    traincon INTEGER NOT NULL DEFAULT 0,
    trainwis INTEGER NOT NULL DEFAULT 0,
    trainagl INTEGER NOT NULL DEFAULT 0,
    trainspd INTEGER NOT NULL DEFAULT 0,
    racial_pref INTEGER NOT NULL DEFAULT 0
);


CREATE TABLE character_bonuses (
    character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- enum for bonus type
    bonus_type INTEGER NOT NULL,
    -- the value of this bonus for this character
    bonus_value INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY(character_id, bonus_type)
);

CREATE TABLE characters_conditions (
    character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- enum for hunger, thirst, etc
    condition_type INTEGER NOT NULL,
    -- the current value of this condition for this character
    condition_value INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY(character_id, condition_type)
);

CREATE TABLE characters_affects (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- 0 for affects, 1 for affectv... is affectv EVEN USED? doesn't seem to be.
    category_id INTEGER NOT NULL DEFAULT 0,
    duration INTEGER NOT NULL DEFAULT 0,
    modifier INTEGER NOT NULL DEFAULT 0,
    location INTEGER NOT NULL DEFAULT 0,
    -- bitfield for affect flags, e.g. bless, curse, etc
    specific INTEGER NOT NULL DEFAULT 0,
    bitvector INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE characters_limb_health (
    character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- enum for limb ID
    limb_id INTEGER NOT NULL,
    -- current health of this limb, from 0 to 100
    health INTEGER NOT NULL DEFAULT 100,
    PRIMARY KEY(character_id, limb_id)
);


CREATE TABLE characters_dgscript_variables(
    -- key value storage of string to string.
    character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE ON UPDATE CASCADE,
    variable_name TEXT NOT NULL,
    variable_value TEXT NOT NULL,
    PRIMARY KEY(character_id, variable_name)
);

CREATE TABLE characters_skills (
    character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- enum for skill ID
    skill_id INTEGER NOT NULL,
    -- the level of this skill for this character
    skill_level INTEGER NOT NULL DEFAULT 0,
    -- any temporary modifier to the skill level, e.g. from a spell affect
    skill_modifier INTEGER NOT NULL DEFAULT 0,
    -- how many times this skill has been perfected, for skills that can be perfected
    skill_perfections INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY(character_id, skill_id)
);

CREATE TABLE characters_boards (
    character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- enum for board ID
    board_id INTEGER NOT NULL,
    -- the date of the last read post on this board for this character, for tracking which posts are new
    last_read_date INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY(character_id, board_id)
);

CREATE TABLE characters_transcosts (
    character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- enum for transformation class
    transtier INTEGER NOT NULL,
    -- the current transformation cost for this character for this transformation class, for the transformation
    paid INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY(character_id, transtier)
);

-- both player character inventory and House contents are stored in this table. the "persistent" objects.
CREATE TABLE objects (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    -- the existing game has a 'generation' field as a time_t used for dupe checks, but that's not needed
    -- because sqlite already solves that problem...
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    -- Not all objects HAVE prototype, but most do.
    object_prototype_id INTEGER NULL REFERENCES object_prototypes(id) ON DELETE SET NULL ON UPDATE CASCADE,

    -- whether this object is active in the game or not. Inactive objects are not part of the game simulation. Example: items in a logged out player's inventory.
    in_game INTEGER NOT NULL DEFAULT 0,

    -- ALL PROTOTYPE DATA HAS TO BE DUPLICATED BELOW HERE...

    -- space separated keywords for searching
    name TEXT NOT NULL DEFAULT '',
    -- the display name of object, can contain color codes
    short_description TEXT NOT NULL DEFAULT '',
    -- the long description of object for when looked at, can contain color codes
    description TEXT NOT NULL DEFAULT '',
    -- The line shown when this object is in a room. Can contain color codes.
    action_description TEXT NOT NULL DEFAULT '',
    -- bitfield for extra flags like glow, hum, etc
    extra_flags BLOB NOT NULL DEFAULT 0,
    -- bitfield for where this object can be worn,
    wear_flags BLOB NOT NULL DEFAULT 0,
    -- bitfield for object properties like magic, invisible, etc
    bitvector BLOB NOT NULL DEFAULT 0,

    weight INTEGER NOT NULL DEFAULT 0,
    cost INTEGER NOT NULL DEFAULT 0,
    -- Cost per day isn't really used...
    cost_per_day INTEGER NOT NULL DEFAULT 0,
    -- level needed to use the object.
    level INTEGER NOT NULL DEFAULT 0,
    size INTEGER NOT NULL DEFAULT 0,

    -- END PROTOTYPE SHARED SECTION

    -- Instance fields below:
    -- the character that is carrying this item, if any
    carried_by INTEGER NULL REFERENCES characters(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- the character that is wearing this item, if any
    worn_by INTEGER NULL REFERENCES characters(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- in_room is used by the House system.
    -- the room this item is in, if not carried or worn by a character
    in_room INTEGER NULL REFERENCES rooms(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- the object this item is in, if not carried or worn by a character
    in_obj INTEGER NULL REFERENCES objects(id) ON DELETE CASCADE ON UPDATE CASCADE,
    
    -- the character that owns this item, regardless of nesting.
    -- IE: if this is in a container held by character, owner_id is that character.
    owner_id INTEGER NULL REFERENCES characters(id) ON DELETE SET NULL ON UPDATE CASCADE
);

CREATE TABLE objects_item_values (
    object_id INTEGER NOT NULL REFERENCES objects(id) ON DELETE CASCADE ON UPDATE CASCADE,
    -- from 0 to 15, the index of this value in the item_values sequence
    val_index INTEGER NOT NULL,
    -- the integer value
    val_number INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY(object_id, val_index)
);

CREATE TABLE objects_dgscript_orders (
    object_id INTEGER NOT NULL REFERENCES objects(id) ON DELETE CASCADE ON UPDATE CASCADE,
    script_id INTEGER NOT NULL REFERENCES dgscript_prototypes(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    script_order INTEGER NOT NULL,
    PRIMARY KEY(object_id, script_id),
    UNIQUE(object_id, script_order)
);

CREATE TABLE objects_dgscript_variables(
    -- key value storage of string to string.
    object_id INTEGER NOT NULL REFERENCES objects(id) ON DELETE CASCADE ON UPDATE CASCADE,
    variable_name TEXT NOT NULL,
    variable_value TEXT NOT NULL,
    PRIMARY KEY(object_id, variable_name)
);

-- Instances of Dgscripts.
-- Preliminary work for phase 2,
-- Not ready for use.
CREATE TABLE dgscripts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    script_id INTEGER NOT NULL REFERENCES dgscript_prototypes(id) ON DELETE RESTRICT ON UPDATE CASCADE,

    -- whether this script is currently active in the game or not. Inactive scripts are not executed by the game loop. Example: a script attached to a logged out player's character.
    is_active INTEGER NOT NULL DEFAULT 0,

    -- What this thing is attached to.
    -- enum for what type of thing this script is attached to, e.g. mob, obj, room, etc
    -- This should be the same as the prototype, but is included here for indexing with UNIQUE
    attach_type INTEGER NOT NULL DEFAULT 0,
    -- the ID of the specific thing this script is attached to, e.g. the mob ID or room ID,
    attach_id INTEGER NOT NULL DEFAULT 0,
    
    -- Script state
    -- how deep this script is in the call stack, for preventing infinite loops and such
    depth INTEGER NOT NULL DEFAULT 0,
    -- how many times this script has looped, for preventing infinite loops and such
    loops INTEGER NOT NULL DEFAULT 0,
    -- whether this script is marked for purging, for the purge mechanic and such
    purged INTEGER NOT NULL DEFAULT 0,
    -- how much time this script has left to wait before it can be executed again, for the wait mechanic and such
    waiting FLOAT NOT NULL DEFAULT 0.0,
    -- a specific script prototype can only be attached once to a specific thing, but the same script prototype can be attached to multiple things, and a specific thing can have multiple scripts attached to it.
    UNIQUE(attach_type, attach_id, script_id)
);

CREATE TABLE dgscripts_variables(
    -- key value storage of string to string.
    dscript_id INTEGER NOT NULL REFERENCES dgscripts(id) ON DELETE CASCADE ON UPDATE CASCADE,
    variable_name TEXT NOT NULL,
    variable_value TEXT NOT NULL,
    PRIMARY KEY(dscript_id, variable_name)
);
