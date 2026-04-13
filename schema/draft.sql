-- This is using sqlite3 syntax.
BEGIN TRANSACTION;

CREATE TABLE zones (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL DEFAULT '',
    builders TEXT NOT NULL DEFAULT '', -- space separated list of builder names, for permissions
    lifespan INTEGER NOT NULL, -- how long in real time minutes between zone resets,
    bot INTEGER NOT NULL, -- the vnum of the bottom of the zone, inclusive
    top INTEGER NOT NULL, -- the vnum of the top of the zone, inclusive
    reset_mode INTEGER NOT NULL, -- enum for how the zone reset should be handled
    min_level INTEGER NOT NULL, -- minimum level for players to enter this zone
    max_level INTEGER NOT NULL, -- maximum level for players to enter this zone
    zone_flags BLOB NOT NULL DEFAULT 0 -- 128-bit bitfield for zone flags, not sure if can store like this...

    -- other fields not being saved:
    -- age in minutes, the countdown to a reset.
);

-- The zone_reset_commands table mirrors the existing schema,
-- but in practice the ORDER of a reset command matters.
-- This table does not give any ability to reorganize them,
-- short of deleting everything for a zone and then re-adding them in the new order,
-- and that's a problem.
CREATE TABLE zone_reset_commands (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    zone_id INTEGER NOT NULL REFERENCES zones(id) ON DELETE CASCADE,
    command_type TEXT NOT NULL, -- 'M' for mobile, 'O' for object, 'R' for room, etc
    if_flag INTEGER NOT NULL, -- if this command should be executed only if the previous command succeeded
    arg1 INTEGER NOT NULL, -- meaning depends on command type
    arg2 INTEGER NOT NULL, -- meaning depends on command type
    arg3 INTEGER NOT NULL, -- meaning depends on command type
    arg4 INTEGER NOT NULL, -- meaning depends on command type
    arg5 INTEGER NOT NULL, -- meaning depends on command type

    -- These don't seem to actually be USED in any existing game data.
    -- But they are in the struct.
    sargs1 TEXT NOT NULL, -- meaning depends on command type
    sargs2 TEXT NOT NULL, -- meaning depends on command type
);

CREATE TABLE dgscript_prototypes(
    id INTEGER PRIMARY KEY,
    attach_type INTEGER NOT NULL, -- enum for what type of thing this script can be attached to, e.g. mob, obj, room, etc
    data_type INTEGER NOT NULL, -- is this even used? it doesn't look like it is. No code use is found.
    name TEXT NOT NULL,
    trigger_type INTEGER NOT NULL, -- enum for what type of trigger this script is, e.g. command, random, etc,
    body TEXT NOT NULL, -- the actual script code.
    narg INTEGER NOT NULL, -- number of args this script takes, for command triggers
    arglist TEXT NOT NULL, -- comma separated list of arg names, for command triggers
);

CREATE TABLE object_prototypes(
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    short_description TEXT NOT NULL,
    description TEXT NOT NULL,
    action_description TEXT NOT NULL,
    extra_flags BLOB NOT NULL DEFAULT 0, -- bitfield for extra flags like glow, hum, etc
    wear_flags BLOB NOT NULL DEFAULT 0, -- bitfield for where this object can be worn,
    bitvector BLOB NOT NULL DEFAULT 0, -- bitfield for object properties like magic, invisible, etc
    -- shitload of object prototype data here.

    -- A sequence of 16 numbers that are used differently depending on the item type.
    -- For god's sakes, we'll just use sscanf in code to parse this.
    item_values TEXT NOT NULL,

    weight INTEGER NOT NULL DEFAULT 0,
    cost INTEGER NOT NULL DEFAULT 0,
    -- Cost per day isn't really used...
    cost_per_day INTEGER NOT NULL DEFAULT 0,
    level INTEGER NOT NULL DEFAULT 0,
    size INTEGER NOT NULL DEFAULT 0,
);

CREATE TABLE object_prototype_extra_descriptions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    object_prototype_id INTEGER NOT NULL REFERENCES object_prototypes(id) ON DELETE CASCADE,
    keywords TEXT NOT NULL,
    description TEXT NOT NULL
);

-- In memory this is a list, and the order of script_ids DOES matter.
CREATE TABLE object_prototype_dgscript_prototypes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    object_prototype_id INTEGER NOT NULL REFERENCES object_prototypes(id) ON DELETE CASCADE,
    script_id INTEGER NOT NULL REFERENCES dgscript_prototypes(id) ON DELETE RESTRICT,
    UNIQUE(object_prototype_id, script_id)
);

-- Objects can have up to 6 "affects".
CREATE TABLE object_prototype_affects (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    object_prototype_id INTEGER NOT NULL REFERENCES object_prototypes(id) ON DELETE CASCADE,
    location INTEGER NOT NULL DEFAULT 0, -- enum for what this affect modifies, e.g. strength, dexterity, etc
    modifier INTEGER NOT NULL DEFAULT 0, -- how much this affect modifies the location by
    specific INTEGER NOT NULL DEFAULT 0 -- bitfield for affect flags, e.g. bless, curse, etc
);

CREATE TABLE mobile_prototypes(
    id INTEGER PRIMARY KEY,
    -- shitload of mobile prototype data here.

    name TEXT NOT NULL DEFAULT '',
    short_descr TEXT NOT NULL DEFAULT '',
    long_descr TEXT NOT NULL DEFAULT '',
    description TEXT NOT NULL DEFAULT '',
    title TEXT NOT NULL,
    size INTEGER NOT NULL DEFAULT 0,
    sex INTEGER NOT NULL DEFAULT 0, -- enum
    race INTEGER NOT NULL DEFAULT 0, -- enum
    chclass INTEGER NOT NULL DEFAULT 0, -- enum

    act BLOB NOT NULL DEFAULT 0, -- bitfield for mob flags like sentinel, aggressive, etc. for PCs, it's player flags.
    affected_by BLOB NOT NULL DEFAULT 0, -- bitfield for affect flags like bless, curse, etc
    level INTEGER NOT NULL DEFAULT 0,

    alignment INTEGER NOT NULL DEFAULT 0,

    str INTEGER NOT NULL DEFAULT 0,
    intel INTEGER NOT NULL DEFAULT 0,
    wis INTEGER NOT NULL DEFAULT 0,
    dex INTEGER NOT NULL DEFAULT 0,
    con INTEGER NOT NULL DEFAULT 0,
    cha INTEGER NOT NULL DEFAULT 0,

    -- mobile stuff that's NOT used by instances below...
    default_position INTEGER NOT NULL DEFAULT 0, -- enum for standing, sitting, etc

);

-- In memory this is a list, and the order of script_ids DOES matter.
CREATE TABLE mobile_prototype_dgscript_prototypes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    mobile_prototype_id INTEGER NOT NULL REFERENCES mobile_prototypes(id) ON DELETE CASCADE,
    script_id INTEGER NOT NULL REFERENCES dgscript_prototypes(id) ON DELETE RESTRICT,
    UNIQUE(mobile_prototype_id, script_id)
);

CREATE TABLE rooms (
    -- room IDs are handled in code, no autoincrement right now
    id INTEGER PRIMARY KEY,

    -- currently the zone ID is managed by the game via zone min/max vnums, but this field
    -- is present in the struct... and I would like to be able to handle this manually.. argh.
    -- zone_id INTEGER NOT NULL REFERENCES zones(id) ON DELETE RESTRICT,

    -- An enum.
    sector_type INTEGER NOT NULL DEFAULT 0,
    name TEXT NOT NULL DEFAULT '',
    description TEXT NOT NULL DEFAULT '',
    room_flags BLOB NOT NULL DEFAULT 0, -- bitfield for room flags like indoors, no-mob, etc

    -- other fields on room_data are runtime only at the moment...
);

-- IN memory this is a linked list, but its order doesn't matter.
CREATE TABLE room_extra_descriptions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_id INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE,
    keywords TEXT NOT NULL,
    description TEXT NOT NULL
);

-- In memory this is a list, and the order of script_ids DOES matter.
CREATE TABLE room_dgscript_prototypes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_id INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE,
    script_id INTEGER NOT NULL REFERENCES dgscript_prototypes(id) ON DELETE RESTRICT,
    UNIQUE(room_id, script_id)
);

CREATE TABLE exits(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_id INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE,
    to_room INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE,
    key_id INTEGER NULL REFERENCES object_prototypes(id) ON DELETE RESTRICT, -- the object ID of a key that unlocks this door. -1 if none.
    exit_info INTEGER NOT NULL DEFAULT 0, --bitfield for exit flags, e.g. locked, hidden, etc
);


CREATE TABLE users (
    -- Previously users didn't have IDS.
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    email TEXT NULL,
    -- Noting that existing passwords are plain text, god damn it.
    password_hash TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    admin_level INTEGER DEFAULT 0,
    rpp_points INTEGER DEFAULT 0,
    rpp_bank INTEGER DEFAULT 0,
    -- I would like to change this to extra_slots and have a global base default.
    -- But just mirroring what currently exists.
    character_slots INTEGER DEFAULT 3
);

-- Used for player characters right now, but ready for others....
CREATE TABLE characters(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    -- the existing game has a 'generation' field as a time_t used for dupe checks, but that's not needed
    -- because sqlite already solves that problem...
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    -- Not all saved characters HAVE users, only player characters... which are currently the only kind of 'saved character'
    user_id INTEGER NULL REFERENCES users(id) ON DELETE CASCADE,

    -- ALL MOBILE PROTOTYPE DATA HAS TO BE DUPLICATED HERE...
    -- But not doing it right now.

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

    hometown INTEGER NULL REFERENCES rooms(id) ON DELETE SET NULL,
    -- time/date stuff in time_data... skipping for now...

    height INTEGER NOT NULL DEFAULT 0,
    weight INTEGER NOT NULL DEFAULT 0,

    position INTEGER NOT NULL DEFAULT 0, -- enum for standing, sitting, etc

    master_id INTEGER NULL REFERENCES characters(id) ON DELETE SET NULL, -- the character that is master to this char, if any

    affected_by BLOB NOT NULL DEFAULT 0, -- bitfield for affect flags like bless, curse, etc
    bodyparts BLOB NOT NULL DEFAULT 0, -- bitfield for body parts like head, arms, legs, etc

    gold INTEGER NOT NULL DEFAULT 0,
    bank_gold INTEGER NOT NULL DEFAULT 0,
    exp INTEGER NOT NULL DEFAULT 0,

    basepl INTEGER NOT NULL DEFAULT 0, -- base player level, used for exp calculations, not necessarily the same as actual level due to affects and such
    baseki INTEGER NOT NULL DEFAULT 0, -- base ki level, used for exp calculations, not necessarily the same as actual level due to affects and such
    basest INTEGER NOT NULL DEFAULT 0, -- base st level, used for exp calculations, not necessarily the same as actual level due to affects and such

    health FLOAT NOT NULL DEFAULT 1.0,
    energy FLOAT NOT NULL DEFAULT 1.0,
    stamina FLOAT NOT NULL DEFAULT 1.0,

    droom INTEGER NULL REFERENCES rooms(id) ON DELETE SET NULL, -- the room a character died in, if applicable. Used for resurrections maybe.
    dcount INTEGER NOT NULL DEFAULT 0, -- how many times this character has died, for death mechanics and such.
    death_type INTEGER NOT NULL DEFAULT 0, -- enum for how the character died, for death mechanics and such.

    kaioken INTEGER NOT NULL DEFAULT 0, -- the current kaioken level of the character, if any. 0 if not kaioken'd up. This is a temp affect, but it's important enough to be queried often that it deserves its own column for easy querying
    absorbs INTEGER NOT NULL DEFAULT 0, -- how many absorbs the character has, for the absorb mechanic. Also a temp affect, but important to query often for things like display and ai
    boosts INTEGER NOT NULL DEFAULT 0, -- how many boosts the character has, for the boost mechanic. Also a temp affect, but important to query often for things like display and ai
    upgrade INTEGER NOT NULL DEFAULT 0, -- the current upgrade level of the character, for the upgrade mechanic. Also a temp affect, but important to query often for things like display and ai
    majinize INTEGER NOT NULL DEFAULT 0, -- the current majinize level of the character, for the majinize mechanic. Also a temp affect, but important to query often for things like display and ai
    fury INTEGER NOT NULL DEFAULT 0, -- the current fury level of the character, for the fury mechanic. Also a temp affect, but important to query often for things like display and ai
    deathtime INTEGER NOT NULL DEFAULT 0, -- the time of the character's death, for the death mechanic. Also a temp affect, but important to query often for things like display and ai
    suppression INTEGER NOT NULL DEFAULT 0, -- the current suppression level of the character, for the suppression mechanic. Also a temp affect, but important to query often for things like display and ai

    mimic INTEGER NOT NULL DEFAULT 0, -- the ID of the race this character is mimicking, if any.

    bonuses INTEGER NOT NULL DEFAULT 0, -- bitfield for various chargen bonuses.

    transclass INTEGER NOT NULL DEFAULT 0, -- transformation costs class.

    -- arlian molt stuff
    moltexp INTEGER NOT NULL DEFAULT 0, -- the amount of experience this character has put into their mobile level, for the mobile level mechanic.
    moltlevel INTEGER NOT NULL DEFAULT 0, -- the current mobile level of the character, for the mobile level mechanic.

    -- Saiyan and halfbreed and tail-y things
    tail_growth INTEGER NOT NULL DEFAULT 0,
    rage_meter INTEGER NOT NULL DEFAULT 0,
    fury INTEGER NOT NULL DEFAULT 0, -- halfbreed fury meter
    

    -- skill stuff
    forgetting INTEGER NOT NULL DEFAULT 0, -- whether the character is currently forgetting a skill, for the skill forget mechanic.
    forgetcount INTEGER NOT NULL DEFAULT 0, -- how many skills the character has forgotten, for the skill forget mechanic.
    skill_slots INTEGER NOT NULL DEFAULT 0,

    -- Preferences
    lifeperf INTEGER NOT NULL DEFAULT 0, -- at what level of health% does lifeforce kick in?
);

-- handles the player_special_data struct..
CREATE TABLE player_special(
    id INTEGER PRIMARY KEY REFERENCES characters(id) ON DELETE CASCADE,
    poofin TEXT NOT NULL DEFAULT '',
    poofout TEXT NOT NULL DEFAULT '',
    last_tell INTEGER NULL REFERENCES characters(id) ON DELETE SET NULL, -- the character ID of the last person to tell this character something, for the reply command and such.
    wimp_level INTEGER NOT NULL DEFAULT 0, -- the health% at which the character will automatically flee from combat, for the wimp mechanic.
    freeze_level INTEGER NOT NULL DEFAULT 0, -- the admin level required to freeze this character, for the freeze command and such.
    invis_level INTEGER NOT NULL DEFAULT 0, -- the admin level required to see this character when they are invisible, for the invis command and such.
    load_room INTEGER NULL REFERENCES rooms(id) ON DELETE SET NULL, -- the room this character will load into when they log in, for the loadroom command and such.
    pref BLOB NOT NULL DEFAULT 0, -- bitfield for various player preferences, e.g. auto-loot, auto-sac, etc
    page_length INTEGER NOT NULL DEFAULT 0, -- the number of lines per page for this character, for the page command and such.
    murder INTEGER NOT NULL DEFAULT 0, -- how many players has this player killed?
    trainstr INTEGER NOT NULL DEFAULT 0,
    trainint INTEGER NOT NULL DEFAULT 0,
    traincon INTEGER NOT NULL DEFAULT 0,
    trainwis INTEGER NOT NULL DEFAULT 0,
    trainagl INTEGER NOT NULL DEFAULT 0,
    trainspd INTEGER NOT NULL DEFAULT 0,
    racial_pref INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE character_conditions (
    character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    condition_type INTEGER NOT NULL, -- enum for hunger, thirst, etc
    condition_value INTEGER NOT NULL DEFAULT 0 -- the current value of this condition for this character
);

CREATE TABLE character_affects (
    character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    category_id INTEGER NOT NULL DEFAULT 0, -- 0 for affects, 1 for affectv
    duration INTEGER NOT NULL DEFAULT 0,
    modifier INTEGER NOT NULL DEFAULT 0,
    location INTEGER NOT NULL DEFAULT 0,
    specific INTEGER NOT NULL DEFAULT 0, -- bitfield for affect flags, e.g. bless, curse, etc
    bitvector INTEGER NOT NULL DEFAULT 0,
);

CREATE TABLE character_limb_health (
    character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    limb_id INTEGER NOT NULL, -- enum for limb ID
    health INTEGER NOT NULL DEFAULT 100 -- current health of this limb, from 0 to 100
);


CREATE TABLE character_dgvariables(
    -- key value storage of string to string.
    character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    variable_name TEXT NOT NULL,
    variable_value TEXT NOT NULL,
    UNIQUE(character_id, variable_name)
);

CREATE TABLE character_skills (
    character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
    skill_id INTEGER NOT NULL, -- enum for skill ID
    skill_level INTEGER NOT NULL DEFAULT 0, -- the level of this skill for this character
    skill_modifier INTEGER NOT NULL DEFAULT 0, -- any temporary modifier to the skill level, e.g. from a spell affect
    skill_perfections INTEGER NOT NULL DEFAULT 0, -- how many times this skill has been perfected, for skills that can be perfected
    UNIQUE(character_id, skill_id)
);

-- both player character inventory and House contents are stored in this table. the "persistent" objects.
CREATE TABLE objects (
    id INTEGER PRIMARY KEY AUTOINCREMENT,

    -- the existing game has a 'generation' field as a time_t used for dupe checks, but that's not needed
    -- because sqlite already solves that problem...
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    -- Not all objects HAVE prototype, but most do.
    object_prototype_id INTEGER NULL REFERENCES object_prototypes(id) ON DELETE SET NULL,

    -- ALL OBJECT PROTOTYPE DATA HAS TO BE DUPLICATED HERE...
    -- But not doing it right now.

    -- Instance fields below:
    carried_by INTEGER NULL REFERENCES saved_character_instances(id) ON DELETE CASCADE, -- the character that is carrying this item, if any
    worn_by INTEGER NULL REFERENCES saved_character_instances(id) ON DELETE CASCADE, -- the character that is wearing this item, if any
    -- in_room is used by the House system.
    in_room INTEGER NULL REFERENCES rooms(id) ON DELETE CASCADE, -- the room this item is in, if not carried or worn by a character
    in_obj INTEGER NULL REFERENCES saved_object_instances(id) ON DELETE CASCADE -- the object this item is in, if not carried or worn by a character

);

CREATE TABLE saved_object_dgvariables(
    -- key value storage of string to string.
    object_id INTEGER NOT NULL REFERENCES saved_object_instances(id) ON DELETE CASCADE,
    variable_name TEXT NOT NULL,
    variable_value TEXT NOT NULL,
    UNIQUE(object_id, variable_name)
);




COMMIT;