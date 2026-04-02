#!/usr/bin/env python3
from dataclasses import dataclass, field

import random
from collections import defaultdict, deque
import re
from pathlib import Path
from enum import IntEnum

class ZoneFlag(IntEnum):
    closed = 0
    no_immortal = 1
    quest = 2
    dragon_balls = 3
    planet = 4
    ether_stream = 5
    has_moon = 6

class AdminFlag(IntEnum):
    tell_all = 0       # Can use 'tell all' to broadcast GOD
    see_invisible = 1  # Sees other chars inventory IMM
    see_secret = 2     # Sees secret doors IMM
    know_weather = 3   # Knows details of weather GOD
    full_where = 4     # Full output of 'where' command IMM
    money = 5          # Char has a bottomless wallet GOD
    eat_anything = 6   # Char can eat anything GOD
    no_poison = 7      # Char can't be poisoned IMM
    walk_anywhere = 8  # Char has unrestricted walking IMM
    no_keys = 9        # Char needs no keys for locks GOD
    instant_kill = 10  # "kill" command is instant IMPL
    no_steal = 11      # Char cannot be stolen from IMM
    trans_all = 12     # Can use 'trans all' GRGOD
    switch_mortal = 13 # Can 'switch' to a mortal PC body IMPL
    force_mass = 14    # Can force rooms or all GRGOD
    all_houses = 15    # Can enter any house GRGOD
    no_damage = 16     # Cannot be damaged IMM
    all_shops = 17     # Can use all shops GOD
    cedit = 18          # Can use cedit IMPL

class PrefFlag(IntEnum):
    brief = 0       # Room descs won't normally be shown
    compact = 1     # No extra CRLF pair before prompts
    deaf = 2        # Can't hear shouts
    notell = 3      # Can't receive tells
    disphp = 4      # Display hit points in prompt
    dispmana = 5    # Display mana points in prompt
    dispmove = 6    # Display move points in prompt
    autoexit = 7    # Display exits in a room
    nohassle = 8    # Aggr mobs won't attack
    quest = 9       # On quest
    summonable = 10 # Can be summoned
    norepeat = 11   # No repetition of comm commands
    holylight = 12  # Can see in dark
    color = 13      # Color
    # spare         = 14 # Used to be second color bit
    nowiz = 15     # Can't hear wizline
    log1 = 16      # On-line System Log (low bit)
    log2 = 17      # On-line System Log (high bit)
    noauct = 18    # Can't hear auction channel
    nogoss = 19    # Can't hear gossip channel
    nogratz = 20   # Can't hear grats channel
    roomflags = 21 # Can see room flags (ROOM_x)
    dispauto = 22  # Show prompt HP MP MV when < 30%.
    cls = 23       # Clear screen in OasisOLC
    buildwalk = 24 # Build new rooms when walking
    afk = 25       # Player is AFK
    autoloot = 26  # Loot everything from a corpse
    autogold = 27  # Loot gold from a corpse
    autosplit = 28 # Split gold with group
    full_exit = 29 # Shows full autoexit details
    autosac = 30   # Sacrifice a corpse
    automem = 31   # Memorize spells
    vieworder = 32 # If you want to see the newest first
    # nocompress    = 33 # If you want to force MCCP2 off
    autoassist = 34 # Auto-assist toggle
    dispki = 35     # Display ki points in prompt
    dispexp = 36    # Display exp points in prompt
    disptnl = 37    # Display TNL exp points in prompt
    test = 38       # Sets triggers safety off for imms
    hide = 39       # Hide on who from other mortals
    nmwarn = 40     # No mail warning
    hints = 41      # Receives hints
    fury = 42       # Sees fury meter
    nodec = 43
    noeqsee = 44
    nomusic = 45
    lkeep = 46
    distime = 47 # Part of Prompt Options
    disgold = 48 # Part of Prompt Options
    disprac = 49 # Part of Prompt Options
    noparry = 50
    dishuth = 51 # Part of Prompt Options
    disperc = 52 # Part of Prompt Options
    carve = 53
    arenawatch = 54
    nogive = 55
    instruct = 56
    ghealth = 57
    ihealth = 58
    energize = 59
    form = 60
    tech = 61

class Size(IntEnum):
    undefined = -1
    fine = 0
    diminutive = 1
    tiny = 2
    small = 3
    medium = 4
    large = 5
    huge = 6
    gargantuan = 7
    colossal = 8


class Skill(IntEnum):
    flex = 400
    genius = 401
    solar_flare = 402
    might = 403
    balance = 404
    build = 405
    tough_skin = 406
    concentration = 407
    kaioken = 408
    spot = 409
    first_aid = 410
    disguise = 411
    escape_artist = 412
    appraise = 413
    heal = 414
    forgery = 415
    hide = 416
    bless = 417
    curse = 418
    listen = 419
    eavesdrop = 420
    poison = 421
    cure_poison = 422
    open_lock = 423
    vigor = 424
    regenerate = 425
    keen_sight = 426
    search = 427
    move_silently = 428
    absorb = 429
    sleight_of_hand = 430
    ingest = 431
    repair = 432
    sense = 433
    survival = 434
    yoikominminken = 435
    create = 436
    stone_spit = 437
    potential_release = 438
    telepathy = 439
    renzokou_energy_dan = 440
    masenko = 441
    dodonpa = 442
    barrier = 443
    galik_gun = 444
    throw_object = 445
    dodge = 446
    parry = 447
    block = 448
    punch = 449
    kick = 450
    elbow = 451
    knee = 452
    roundhouse = 453
    uppercut = 454
    slam = 455
    heeldrop = 456
    focus = 457
    ki_ball = 458
    ki_blast = 459
    beam = 460
    tsuihidan = 461
    shogekiha = 462
    zanzoken = 463
    kamehameha = 464
    dagger = 465
    sword = 466
    club = 467
    spear = 468
    gun = 469
    brawl = 470
    instant_transmission = 471
    deathbeam = 472
    eraser = 473
    twin_slash = 474
    psyblast = 475
    honoo = 476
    dualbeam = 477
    rogafufuken = 478
    special_pose = 479
    bakuhatsuha = 480
    kienzan = 481
    tribeam = 482
    special_beam_cannon = 483
    final_flash = 484
    crusher = 485
    darkness_dragon_slash = 486
    psychic_barrage = 487
    hellflash = 488
    hell_spear_blast = 489
    kakusanha = 490
    hasshuken = 491
    scatter = 492
    big_bang = 493
    phoenix_slash = 494
    deathball = 495
    spirit_ball = 496
    genkidama = 497
    genocide = 498
    dualwield = 499
    kuraiiro_seiki = 500
    tailwhip = 501
    kousengan = 502
    taisha_reiki = 503
    paralyze = 505
    infuse = 506
    roll = 507
    trip = 508
    grapple = 509
    water_spike = 510
    self_destruct = 511
    spiral_comet = 512
    star_breaker = 513
    enlighten = 514
    commune = 515
    mimic = 516
    water_razor = 517
    koteiru_bakuha = 518
    dimizu_toride = 519
    hyogaKabe = 520
    wellspring = 521
    aquaBarrier = 522
    warp_pool = 523
    hellSpiral = 524
    nanite_armor = 525
    fireshield = 526
    cooking = 527
    seishou_enko = 528
    silk = 529
    bash = 530
    headbutt = 531
    ensnare = 532
    starnova = 533
    pursuit = 534
    zen_blade_strike = 535
    sundering_force = 536
    wither = 537
    twohand = 538
    fighting_arts = 539
    dark_metamorphosis = 540
    healing_glow = 541
    runic = 542
    extract = 543
    gardening = 544
    energize_throwing = 545
    malice_breaker = 549
    hayasa = 550
    handling = 551
    mystic_music = 552
    light_grenade = 553
    multiform = 554
    spirit_control = 555
    balefire = 556
    blessed_hammer = 557

    divine_halo = 558
    instinctual_combat = 559

    tiger_stance = 560
    eagle_stance = 561
    ox_stance = 562

class AffectFlag(IntEnum):
    # dontuse          = 0   # DON'T USE!
    blind = 1           # (R) Char is blind
    invisible = 2       # Char is invisible
    detect_align = 3    # Char is sensitive to align
    detect_invis = 4    # Char can see invis chars
    detect_magic = 5    # Char is sensitive to magic
    sense_life = 6      # Char can sense hidden life
    waterwalk = 7       # Char can walk on water
    sanctuary = 8       # Char protected by sanct.
    group = 9           # (R) Char is grouped
    curse = 10          # Char is cursed
    infravision = 11    # Char can see in dark
    poison = 12         # (R) Char is poisoned
    weakened_state = 13 # Char protected from evil?
    protect_good = 14   # Char protected from good
    sleep = 15          # (R) Char magically asleep
    no_track = 16       # Char can't be tracked
    undead = 17         # Char is undead
    paralyze = 18       # Char is paralyzed
    sneak = 19          # Char can move quietly
    hide = 20           # Char is hidden
    # unused_20        = 21  # Room for future expansion
    charm = 22           # Char is charmed
    flying = 23          # Char is flying
    water_breathing = 24 # Char can breathe non-O2
    angelic = 25         # Char is an angelic being
    ethereal = 26        # Char is ethereal
    magic_only = 27      # Char only hurt by magic
    next_partial = 28    # Next action cannot be full
    next_no_action = 29  # Next action cannot attack
    stunned = 30         # Char is stunned
    tamed = 31           # Char has been tamed
    creeping_death = 32  # Char is undergoing creeping death
    spirit = 33          # Char has no body
    stoneskin = 34       # Char has temporary DR
    summoned = 35        # Char is summoned (transient)
    celestial = 36       # Char is celestial
    fiendish = 37        # Char is fiendish
    fire_shield = 38     # Char has fire shield
    low_light = 39       # Char has low light eyes
    zanzoken = 40        # Char is ready to zanzoken
    knocked = 41         # Char is knocked OUT
    might = 42           # Strength +3
    flex = 43            # Agility +3
    genius = 44          # Intelligence +3
    bless = 45           # Bless for better regen
    burnt = 46           # Disintegrated corpse
    burned = 47          # Burned by honoo or similar skill
    mbreak = 48          # Can't charge while flagged
    hasshuken = 49       # Does double punch damage
    future = 50          # Future Sight
    paralyzed = 51       # Real Paralyze
    infuse = 52          # Ki-infused attacks
    enlighten = 53       # Enlighten
    frozen = 54          # They got frozededed
    fireshield = 55      # They have a blazing personality
    ensnared = 56        # Silk ensnaring their arms!
    hayasa = 57          # They are speedy!
    pursuit = 58         # Being followed
    wither = 59          # Their body is withered
    hydrozap = 60        # Custom Skill Kanso Suru
    position = 61        # Better combat position
    psychic_shocked = 62 # Psychic Shock
    metamorph = 63       # Metamorphosis
    healglow = 64        # Healing Glow
    earmor = 65          # Ethereal Armor
    echains = 66         # Ethereal Chains
    wunjo_rune = 67      # Wunjo rune
    purisaz_rune = 68    # Purisaz rune
    ashed = 69           # Leaves ash
    puked = 70
    liquefied = 71
    shell = 72
    immunity = 73
    spiritcontrol = 74
    ginyu_pose = 75
    kyodaika = 76
    shadowstitch = 77
    echains_debuff = 78
    starphase = 79
    mbreak_debuff = 80
    limit_breaking = 81

class Direction(IntEnum):
    north = 0
    east = 1
    south = 2
    west = 3
    up = 4
    down = 5
    northwest = 6
    northeast = 7
    southeast = 8
    southwest = 9
    inside = 10
    outside = 11

    def opposite(self) -> "Direction":
        match self:
            case Direction.north:
                return Direction.south
            case Direction.east:
                return Direction.west
            case Direction.south:
                return Direction.north
            case Direction.west:
                return Direction.east
            case Direction.up:
                return Direction.down
            case Direction.down:
                return Direction.up
            case Direction.northwest:
                return Direction.southeast
            case Direction.northeast:
                return Direction.southwest
            case Direction.southeast:
                return Direction.northwest
            case Direction.southwest:
                return Direction.northeast
            case Direction.inside:
                return Direction.outside
            case Direction.outside:
                return Direction.inside

    def update_coordinates(self, coor: Coordinates) -> Coordinates:
        match self:
            case Direction.north:
                return Coordinates(coor.x, coor.y + 1, coor.z)
            case Direction.east:
                return Coordinates(coor.x + 1, coor.y, coor.z)
            case Direction.south:
                return Coordinates(coor.x, coor.y - 1, coor.z)
            case Direction.west:
                return Coordinates(coor.x - 1, coor.y, coor.z)
            case Direction.up:
                return Coordinates(coor.x, coor.y, coor.z + 1)
            case Direction.down:
                return Coordinates(coor.x, coor.y, coor.z - 1)
            case Direction.northeast:
                return Coordinates(coor.x + 1, coor.y + 1, coor.z)
            case Direction.southeast:
                return Coordinates(coor.x + 1, coor.y - 1, coor.z)
            case Direction.southwest:
                return Coordinates(coor.x - 1, coor.y - 1, coor.z)
            case Direction.northwest:
                return Coordinates(coor.x - 1, coor.y + 1, coor.z)
            case _:
                return Coordinates(coor.x, coor.y, coor.z)


class Race(IntEnum):
    spirit = 0
    human = 1
    saiyan = 2
    icer = 3
    konatsu = 4
    namekian = 5
    mutant = 6
    kanassan = 7
    halfbreed = 8
    bio_android = 9
    android = 10
    demon = 11
    majin = 12
    kai = 13
    tuffle = 14
    hoshijin = 15
    animal = 16
    saiba = 17
    serpent = 18
    ogre = 19
    yardratian = 20
    arlian = 21
    dragon = 22
    mechanical = 23

    def has_tail(self) -> bool:
        match self:
            case Race.icer | Race.bio_android | Race.saiyan | Race.halfbreed:
                return True
            case _:
                return False



class Sensei(IntEnum):
    commoner = 0
    roshi = 1
    piccolo = 2
    crane = 3
    nail = 4
    bardock = 5
    ginyu = 6
    frieza = 7
    tapion = 8
    sixteen = 9
    dabura = 10
    kibito = 11
    jinto = 12
    tsuna = 13
    kurzak = 14

class Sex(IntEnum):
    neutral = 0
    male = 1
    female = 2

class WhereFlag(IntEnum):
    planet_earth = 0             # Room is on Earth
    earth_orbit = 1              # Earth Orbit
    planet_vegeta = 2            # Room is on Vegeta
    vegeta_orbit = 3             # Vegeta Orbit
    planet_frigid = 4            # Room is on Frigid
    frigid_orbit = 5             # Frigid Orbit
    planet_konack = 6            # Room is on Konack
    konack_orbit = 7             # Konack Orbit
    planet_namek = 8             # Room is on Namek
    namek_orbit = 9              # Namek Orbit
    planet_aether = 10           # Room is on Aether
    aether_orbit = 11            # Aether Orbit
    planet_yardrat = 12          # This room is on planet Yardrat
    yardrat_orbit = 13           # Yardrat Orbit
    planet_kanassa = 14          # This room is on planet Kanassa
    kanassa_orbit = 15           # Kanassa Orbit
    planet_arlia = 16            # This room is on planet Arlia
    arlia_orbit = 17             # Arlia Orbit
    planet_cerria = 18           # This room is on planet Cerria
    cerria_orbit = 19            # This room is in Cerria's Orbit
    moon_zenith = 20             # (unspecified)
    zenith_orbit = 21            # zenith orbit
    neo_nirvana = 22             # Room is on Neo
    afterlife = 23               # Room is on AL
    afterlife_hell = 24          # Room is HELLLLLLL
    hyperbolic_time_chamber = 25 # Room is extra special training area
    pendulum_past = 26           # Inside the pendulum room
    space = 27                   # Room is on Space
    nebula = 28                  # Nebulae
    asteroid = 29                # Asteroid
    wormhole = 30                # Wormhole
    space_station = 31           # Space Station
    star = 32                    # Is a star


class SectorType(IntEnum):
    inside = 0       # Indoors
    city = 1         # In a city
    field = 2        # In a field
    forest = 3       # In a forest
    hills = 4        # In the hills
    mountain = 5     # On a mountain
    water_swim = 6   # Swimmable water
    water_noswim = 7 # Water - need a boat
    flying = 8       # Wheee!
    underwater = 9   # Underwater
    shop = 10        # Shop
    important = 11   # Important Rooms
    desert = 12      # A desert
    space = 13       # This is a space room
    lava = 14         # This room always has lava

class RoomFlag(IntEnum):
    dark = 0 # Dark
    # death_trap = 1            # Death trap
    no_mobiles = 2              # MOBs not allowed
    indoors = 3                 # Indoors
    peaceful = 4                # Violence not allowed
    soundproof = 5              # Shouts gossip blocked
    no_track = 6                # Track won't go through
    no_instant_transmission = 7 # IT not allowed
    tunnel = 8                  # room for only 1 pers
    private_room = 9            # Can't teleport in
    god_room = 10               # LVL_GOD+ only allowed
    house = 11                  # (R) Room is a house
    # house_crash = 12     # (R) House needs saving
    atrium = 13 # (R) The door to a house
    olc = 14    # (R) Modifyable/!compress
    # bfs_mark = 15        # (R) breath-first srch mrk
    vehicle = 16     # Requires a vehicle to pass
    underground = 17 # Room is below ground
    # timed_deathtrap = 19        # Room has a timed death trap
    punishment_hell = 28 # Room is Punishment Hell
    regen = 29           # Better regen
    clan_bank = 35       # This room is a clan bank
    ship = 36            # This room is a private ship room
    healing_aura = 40    # This room has an aura around it
    bedroom = 57         # +25% regen
    workout = 58         # Workout Room
    garden_1 = 59        # 8 plant garden
    garden_2 = 60        # 20 plant garden
    fertile_1 = 61       # (unspecified)
    fertile_2 = 62       # (unspecified)
    fishing = 63         # (unspecified)
    fishfresh = 64       # (unspecified)
    can_remodel = 65     # (unspecified)
    # save = 67            # room saves contents


class MobFlag(IntEnum):
    special_proc = 0        # Mob has a callable spec-proc
    sentinel = 1            # Mob should not move
    no_scavenger = 2        # Mob won't pick up items from rooms
    aware = 4               # Mob can't be backstabbed
    aggressive = 5          # Mob auto-attacks everybody nearby
    stay_zone = 6           # Mob shouldn't wander out of zone
    wimpy = 7               # Mob flees if severely injured
    aggressive_evil = 8     # Auto-attack any evil PC's
    aggressive_good = 9     # Auto-attack any good PC's
    aggressive_neutral = 10 # Auto-attack any neutral PC's
    memory = 11             # remember attackers if attacked
    helper = 12             # attack PCs fighting other NPCs
    no_charm = 13           # Mob can't be charmed
    no_summon = 14          # Mob can't be summoned
    no_sleep = 15           # Mob can't be slept
    autobalance = 16        # Mob stats autobalance
    no_blind = 17           # Mob can't be blinded
    no_kill = 18            # Mob can't be killed
    not_dead_yet = 19       # (R) Mob being extracted.
    mountable = 20          # Mob is mountable.
    justdesc = 26           # Mob doesn't use auto desc
    husk = 27               # Is an extracted Husk
    dummy = 29              # This mob will not fight back
    no_poison = 32          # No poison
    know_kaioken = 33       # Knows kaioken


class CharacterFlag(IntEnum):
    tail = 1
    cyber_right_arm = 2 # Cybernetic Right Arm
    cyber_left_arm = 3  # Cybernetic Left Arm
    cyber_right_leg = 4 # Cybernetic Right Leg
    cyber_left_leg = 5  # Cybernetic Left Leg

    sparring = 6    # This is mob sparring
    powering_up = 7 # Is powering up


class PlayerFlag(IntEnum):
    player_killer = 0        # Player is a player-killer
    player_thief = 1         # Player is a player-thief
    frozen = 2               # Player is frozen
    writing = 4              # Player writing (board/mail/olc)
    mailing = 5              # Player is writing mail
    site_ok = 7              # Player has been site-cleared
    no_shout = 8             # Player not allowed to shout/goss
    no_title = 9             # Player not allowed to set title
    loadroom = 11            # Player uses nonstandard loadroom
    no_wizlist = 12          # Player shouldn't be on wizlist
    no_delete = 13           # Player shouldn't be deleted
    wiz_invisible_start = 14 # Player should enter game wizinvis
    not_dead_yet = 16        # (R) Player being extracted.
    piloting = 31            # Player is sitting in the pilots chair
    skillp = 32              # Player made a good choice in CC
    charging = 34            # Player is charging
    knocked_out = 45         # Knocked OUT
    immortal = 51            # The player is immortal
    eyes_closed = 52         # The player has their eyes closed
    disguised = 53           # The player is disguised
    bandaged = 54            # The player has been bandaged
    healing_tank = 56        # Is inside a healing tank
    halfbreed_fury = 57      # Is in fury mode
    ginyu_fighting_pose = 58 # Ginyu Pose Effect
    absorbed = 60
    killed_by_player = 62
    two_hand_wielding = 63
    self_destruct_1 = 64
    self_destruct_2 = 65
    spiral = 66
    biography_approved = 67
    repair_learn = 69
    forgetting_skill = 70
    transmission = 71
    fishing = 72
    majin_goop_state = 73
    multi_hit = 74
    aura_light = 75
    room_display = 76
    stolen = 77
    tail_hide = 78     # Hides tail for S & HB
    no_regrow_tail = 79 # Halt Growth for S & HB

class ItemType(IntEnum):
    unknown = 0          # Item type not defined
    light = 1            # Item is a light source
    scroll = 2           # Item is a scroll
    wand = 3             # Item is a wand
    staff = 4            # Item is a staff
    weapon = 5           # Item is a weapon
    fireweapon = 6       # Unimplemented
    campfire = 7         # Burn things for fun!
    treasure = 8         # Item is a treasure not gold
    armor = 9            # Item is armor
    potion = 10          # Item is a potion
    worn = 11            # Unimplemented
    other = 12           # Misc object
    trash = 13           # Trash - shopkeeps won't buy
    trap = 14            # Unimplemented
    container = 15       # Item is a container
    note = 16            # Item is note
    drink_container = 17 # Item is a drink container
    key = 18             # Item is a key
    food = 19            # Item is food
    money = 20           # Item is money (gold)
    pen = 21             # Item is a pen
    boat = 22            # Item is a boat
    fountain = 23        # Item is a fountain
    vehicle = 24         # Item is a vehicle
    hatch = 25           # Item is a vehicle hatch
    window = 26          # Item is a vehicle window
    control = 27         # Item is a vehicle control
    portal = 28          # Item is a portal
    spellbook = 29       # Item is a spellbook
    board = 30           # Item is a message board
    chair = 31           # Is a chair
    bed = 32             # Is a bed
    yum = 33             # This was good food
    plant = 34           # This will grow!
    fishing_pole = 35    # FOR FISHING
    fishing_bait = 36    # DITTO

class ItemFlag(IntEnum):
    glow = 0           # Item is glowing
    hum = 1            # Item is humming
    no_rent = 2        # Item cannot be rented
    no_donate = 3      # Item cannot be donated
    no_invisible = 4   # Item cannot be made invis
    invisible = 5      # Item is invisible
    magic = 6          # Item is magical
    no_drop = 7         # Item is cursed: can't drop
    bless = 8          # Item is blessed
    nosell = 9         # Shopkeepers won't touch it
    two_hands = 10     # Requires two free hands
    unique_save = 11   # unique object save
    broken = 12        # Item is broken
    unbreakable = 13   # Item is unbreakable
    double_weapon = 14 # Double weapon
    card = 15          # Item is a card
    cboard = 16
    forged = 17
    sheath = 18
    buried = 19
    slot_1 = 20
    slot_2 = 21
    token = 22
    slot_one = 23
    slots_filled = 24
    restring = 25
    custom = 26
    protected_item = 27
    norandom = 28
    throwable = 29 # "throw" is not reserved but renamed to avoid confusion
    hot = 30
    purge = 31
    ice = 32
    duplicate = 33
    mature = 34
    card_case = 35
    no_pickup = 36
    no_steal = 37

class WearFlag(IntEnum):
    take = 0       # Item can be taken
    finger = 1     # Can be worn on finger
    neck = 2       # Can be worn around neck
    body = 3       # Can be worn on body
    head = 4       # Can be worn on head
    legs = 5       # Can be worn on legs
    feet = 6       # Can be worn on feet
    hands = 7      # Can be worn on hands
    arms = 8       # Can be worn on arms
    shield = 9     # Can be used as a shield
    about = 10     # Can be worn about body
    waist = 11     # Can be worn around waist
    wrist = 12     # Can be worn on wrist
    wield = 13     # Can be wielded
    hold = 14      # Can be held
    back = 15      # Can be worn as a backpack
    ear = 16       # Can be worn as an earring
    shoulders = 17 # Can be worn as wings (originally ITEM_WEAR_SH)
    eyes = 18       # Can be worn as a mask

class WearSlot(IntEnum):
    Inventory = 0 # not actually equipped but signifies that something can be in an inventory.
    RightFinger = 1
    LeftFinger = 2
    Neck1 = 3
    Neck2 = 4
    Body = 5
    Head = 6
    Legs = 7
    Feet = 8
    Hands = 9
    Arms = 10
    About = 12
    Waist = 13
    RightWrist = 14
    LeftWrist = 15
    Wield1 = 16
    Wield2 = 17
    Backpack = 18
    RightEar = 19
    LeftEar = 20
    Shield = 21
    Eyes = 22

class ShopFlag(IntEnum):
    start_fight = 0
    bank_money = 1
    allow_steal = 2
    no_broken = 3

@dataclass(slots=True, frozen=True)
class Coordinates:
    x: int = 0
    y: int = 0
    z: int = 0

@dataclass(slots=True)
class Location:
    type: str = "room"
    target: int = -1
    coordinates: Coordinates = field(default_factory=Coordinates)

@dataclass(slots=True)
class Exit(Location):
    destination: Location = field(default_factory=Location)
    keywords: str | None = None
    description: str | None = None
    exit_flags: set[str] = field(default_factory=set)
    key: int | None = None
    dclock: int | None = None
    dchide: int | None = None

@dataclass(slots=True)
class ExtraDescription:
    keywords: str = ""
    description: str = ""

@dataclass(slots=True)
class ResetCommand:
    command: str = ""
    depends_last: bool = False
    target: str = ""
    max: int = 0
    max_location: int = 0
    chance: int = 100
    ex: int = 0
    key: str = ""
    value: str = ""

@dataclass(slots=True)
class Room:
    vnum: int = -1
    name: str = ""
    look_description: str = ""
    extra_descriptions: list[ExtraDescription] = field(default_factory=list)
    exits: dict[Direction, Exit] = field(default_factory=dict)
    room_flags: set[RoomFlag] = field(default_factory=set)
    sector_type: SectorType = SectorType.inside
    where_flags: set[WhereFlag] = field(default_factory=set)
    proto_script: list[int] = field(default_factory=list)
    reset_commands: list[ResetCommand] = field(default_factory=list)
    zone: int = -1

@dataclass(slots=True)
class Affected:
    location: int = 0
    modifier: int = 0
    specific: int = 0
    aff_flags: set[AffectFlag] = field(default_factory=set)
    type: int = 0

    def valid(self) -> bool:
        return self.location != 0

@dataclass
class Entity:
    vnum: int = -1
    name: str = ""
    short_description: str = ""
    room_description: str = ""
    look_description: str = ""
    proto_script: list[int] = field(default_factory=list)
    affect_flags: set[AffectFlag] = field(default_factory=set)
    affected: list[Affected] = field(default_factory=list)

@dataclass
class PickyData:
    not_alignment: set[str] = field(default_factory=set)
    only_alignment: set[str] = field(default_factory=set)
    not_sensei: set[Sensei] = field(default_factory=set)
    only_sensei: set[Sensei] = field(default_factory=set)
    not_race: set[Race] = field(default_factory=set)
    only_race: set[Race] = field(default_factory=set)

    def load_picky(self, bitfield: int):
        for i in range(128):
            if bitfield & (1 << i):
                match i:
                    case 0:  # no good
                        self.not_alignment.add("good")
                    case 1:  # no evil
                        self.not_alignment.add("evil")
                    case 2:  # no neutral
                        self.not_alignment.add("neutral")
                    case 3:  # not roshi
                        self.not_sensei.add(Sensei.roshi)
                    case 4:  # not piccolo
                        self.not_sensei.add(Sensei.piccolo)
                    case 5:  # not crane
                        self.not_sensei.add(Sensei.crane)
                    case 6:  # not nail
                        self.not_sensei.add(Sensei.nail)
                    case 7:  # not human
                        self.not_race.add(Race.human)
                    case 8:  # not icer
                        self.not_race.add(Race.icer)
                    case 9:  # not saiyan
                        self.not_race.add(Race.saiyan)
                    case 10:  # not konatsu
                        self.not_race.add(Race.konatsu)
                    case 11:  # not namek
                        self.not_race.add(Race.namekian)
                    case 12:  # not mutant
                        self.not_race.add(Race.mutant)
                    case 13:  # not kanassan
                        self.not_race.add(Race.kanassan)
                    case 14:  # not bio-android
                        self.not_race.add(Race.bio_android)
                    case 15:  # not android
                        self.not_race.add(Race.android)
                    case 16:  # not demon
                        self.not_race.add(Race.demon)
                    case 17:  # not majin
                        self.not_race.add(Race.majin)
                    case 18:  # not kai
                        self.not_race.add(Race.kai)
                    case 19:  # not truffle
                        self.not_race.add(Race.tuffle)
                    case 29:  # not bardock
                        self.not_sensei.add(Sensei.bardock)
                    case 30:  # not ginyu
                        self.not_sensei.add(Sensei.ginyu)
                    case 32:  # only roshi
                        self.only_sensei.add(Sensei.roshi)
                    case 33:  # only piccolo
                        self.only_sensei.add(Sensei.piccolo)
                    case 34:  # only crane
                        self.only_sensei.add(Sensei.crane)
                    case 35:  # only nail
                        self.only_sensei.add(Sensei.nail)
                    case 36:  # only bardock
                        self.only_sensei.add(Sensei.bardock)
                    case 37:  # only ginyu
                        self.only_sensei.add(Sensei.ginyu)
                    case 38:  # not frieza
                        self.not_sensei.add(Sensei.frieza)
                    case 39:  # not tapion
                        self.not_sensei.add(Sensei.tapion)
                    case 40:  # not sixteen
                        self.not_sensei.add(Sensei.sixteen)
                    case 41:  # not dabura
                        self.not_sensei.add(Sensei.dabura)
                    case 42:  # not kibito
                        self.not_sensei.add(Sensei.kibito)
                    case 43:  # only frieza
                        self.only_sensei.add(Sensei.frieza)
                    case 44:  # only tapion
                        self.only_sensei.add(Sensei.tapion)
                    case 45:  # only sixteen
                        self.only_sensei.add(Sensei.sixteen)
                    case 46:  # only dabura
                        self.only_sensei.add(Sensei.dabura)
                    case 47:  # only kibito
                        self.only_sensei.add(Sensei.kibito)
                    case 48:  # only jinto
                        self.only_sensei.add(Sensei.jinto)
                    case 49:  # only tsuna
                        self.only_sensei.add(Sensei.tsuna)
                    case 50:  # only kurzak
                        self.only_sensei.add(Sensei.kurzak)
                    case 63:  # not jinto
                        self.not_sensei.add(Sensei.jinto)
                    case 64:  # not tsuna
                        self.not_sensei.add(Sensei.tsuna)
                    case 65:  # not kurzak
                        self.not_sensei.add(Sensei.kurzak)


@dataclass
class ObjectPrototype(PickyData, Entity):
    item_type: ItemType = ItemType.unknown
    extra_descriptions: list[ExtraDescription] = field(default_factory=list)
    item_flags: set[ItemFlag] = field(default_factory=set)
    wear_flags: set[WearFlag] = field(default_factory=set)
    values: dict[str, int] = field(default_factory=dict)
    weight: int = 0
    cost: int = 0
    cost_per_day: int = 0
    level: int = 0
    size: Size = Size.medium
    

@dataclass(slots=True)
class SkillData:
    level: int = 0
    bonus: int = 0
    perfs: int = 0

@dataclass(slots=True)
class CharacterPrototype(Entity):
    mob_flags: set[MobFlag] = field(default_factory=set)
    character_flags: set[CharacterFlag] = field(default_factory=set)
    player_flags: set[PlayerFlag] = field(default_factory=set)
    stats: dict[str, float] = field(default_factory=dict)
    skills: dict[Skill, SkillData] = field(default_factory=dict)
    race: Race = Race.human
    sensei: Sensei = Sensei.roshi
    position: int = 0
    sex: Sex = Sex.male
    android_model: str | None = None
    id: int | None = None
    

@dataclass(slots=True)
class Character(CharacterPrototype):
    time: dict[str, int] = field(default_factory=dict)
    affected: list[Affected] = field(default_factory=list)


@dataclass(slots=True)
class DgScriptPrototype:
    vnum: int = -1
    name: str = ""
    attach_type: int = 0
    trigger_type: int = 0
    narg: int = 0
    command: str = ""
    body: str = ""

@dataclass(slots=True)
class ShopBuyData:
    item_type: ItemType = ItemType.unknown
    keywords: str = ""

@dataclass(slots=True)
class Shop(PickyData):
    vnum: int = -1
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
    temper: int = 0
    shop_flags: set[ShopFlag] = field(default_factory=set)
    keeper: int = -1
    in_room: list[int] = field(default_factory=list)
    open1: int = 0
    open2: int = 0
    close1: int = 0
    close2: int = 0

@dataclass(slots=True)
class Guild(PickyData):
    vnum: int = -1
    skills: set[Skill] = field(default_factory=set)
    feats: set[str] = field(default_factory=set)
    charge: float = 1.0
    no_such_skill: str = ""
    not_enough_gold: str = ""
    minlvl: int = 0
    keeper: int = -1
    open: int = 0
    close: int = 0

@dataclass(slots=True)
class Component:
    vnum: int = -1
    consumed: bool = True
    in_room: bool = False

@dataclass(slots=True)
class Assembly:
    vnum: int = -1
    assembly_type: str = "build"
    components: list[Component] = field(default_factory=list)

@dataclass(slots=True)
class LegacyReset:
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
    vnum: int = -1
    name: str = ""
    parent: int = -1
    children: set[int] = field(default_factory=set)
    color_name: str = ""
    lifespan: int = 30
    reset_mode: int = 0
    zone_flags: set[ZoneFlag] = field(default_factory=set)
    resets: list[LegacyReset] = field(default_factory=list)
    bottom: int = 0
    top: int = 0
    builders: str = ""
    min_level: int = 0
    max_level: int = 0
    launch_destination: str = ""
    landing_spots: dict[str, str] = field(default_factory=dict)
    dock_spots: dict[str, str] = field(default_factory=dict)


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

@dataclass
class Dimensions:
    north: int = 0
    south: int = 0
    east: int = 0
    west: int = 0
    up: int = 0
    down: int = 0

@dataclass
class Shape:
    type: str = "box"
    dimensions: Dimensions = field(default_factory=Dimensions)
    coordinates: Coordinates = field(default_factory=Coordinates)
    name: str = ""
    look_description: str = ""
    priority: int = 0
    sector_type: SectorType = SectorType.inside
    tile_display: str = ""

@dataclass
class Tile:
    name: str = ""
    look_description: str = ""
    sector_type: SectorType | None = None
    exits: dict[Direction, Exit] = field(default_factory=dict)
    room_flags: set[RoomFlag] = field(default_factory=set)
    reset_commands: list[ResetCommand] = field(default_factory=list)
    proto_script: list[int] = field(default_factory=list)
    extra_descriptions: list[ExtraDescription] = field(default_factory=list)
    tile_display: str = ""

@dataclass
class Area(Entity):
    vnum: int = -1
    name: str = ""
    zone: int = -1
    look_description: str = ""
    shapes: dict[str, Shape] = field(default_factory=dict)
    tiles: dict[Coordinates, Tile] = field(default_factory=dict)

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
                raw_player_flags = flag_conv(value.split())
            case "Aff":
                out.affect_flags = flags_to_new_set(flag_conv(value.split()), AffectFlag)
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

def convert_object(data: ObjectPrototype, item_values: list[int], item_flags: int):
    item_type = data.item_type
    values: dict[str, int] = {
        "health": item_values[4],
        "max_health": item_values[5],
        "material": item_values[7]
    }

    match item_type:
        case ItemType.light:
            values["time"] = item_values[0]
            values["hours"] = item_values[2]
        case ItemType.scroll | ItemType.wand | ItemType.potion:
            values["level"] = item_values[0]
            values["spell1"] = item_values[1]
            values["spell2"] = item_values[2]
            values["spell3"] = item_values[3]
        case ItemType.staff:
            values["level"] = item_values[0]
            values["max_charges"] = item_values[1]
            values["charges"] = item_values[2]
            values["spell"] = item_values[3]
        case ItemType.weapon:
            values["skill"] = item_values[0]
            values["damage_dice"] = item_values[1]
            values["damage_size"] = item_values[2]
            values["damage_type"] = item_values[3]
            values["critical_type"] = item_values[6]
            values["critical_range"] = item_values[8]
            values["level"] = item_values[9]
        case ItemType.armor:
            values["apply_ac"] = item_values[0]
            values["skill"] = item_values[1]
            values["max_dex_mod"] = item_values[2]
            values["check"] = item_values[3]
            values["spell_fail"] = item_values[6]
        case ItemType.worn:
            values["scouter_level"] = item_values[15]
        case ItemType.other:
            values["seraf_ink"] = item_values[6]
            values["soil_quality"] = item_values[8]
        case ItemType.trap:
            values["spell"] = item_values[0]
            values["health"] = item_values[1]
        case ItemType.container:
            values["capacity"] = item_values[0]
            values["flags"] = item_values[1]
            values["key"] = item_values[2]
            values["corpse"] = item_values[3]
            values["owner"] = item_values[8]
        case ItemType.note:
            values["language"] = item_values[0]
        case ItemType.drink_container | ItemType.fountain:
            values["capacity"] = item_values[0]
            values["how_full"] = item_values[1]
            values["liquid"] = item_values[2]
            values["poison"] = item_values[3]
        case ItemType.key:
            values["keycode"] = item_values[2]
        case ItemType.food | ItemType.yum:
            values["foodval"] = item_values[0]
            values["max_foodval"] = item_values[1]
            values["psbonus"] = item_values[2]
            values["poison"] = item_values[3]
            values["expbonus"] = item_values[6]
            values["candy_pl"] = item_values[8]
            values["candy_ki"] = item_values[9]
            values["candy_st"] = item_values[10]
            values["whichattr"] = item_values[11]
            values["attrchance"] = item_values[12]
        case ItemType.money:
            values["size"] = item_values[0]
        case ItemType.vehicle | ItemType.hatch | ItemType.window | ItemType.control:
            pass
        case ItemType.portal:
            values["destination"] = item_values[0]
            values["flags"] = item_values[1]
            values["dc_move"] = item_values[2]
            values["appear"] = item_values[3]
            values["dc_lock"] = item_values[8]
            values["dc_hide"] = item_values[9]
        case ItemType.board:
            pass
        case ItemType.bed:
            values["comfort_level"] = item_values[8]
            values["htank_charge"] = item_values[9]
        case ItemType.plant:
            values["soil_quality"] = item_values[0]
            values["mat_goal"] = item_values[1]
            values["maturity"] = item_values[2]
            values["max_mature"] = item_values[3]
            values["water_level"] = item_values[6]
        case ItemType.fishing_pole:
            values["bait"] = item_values[0]

    data.values.update({k: v for k, v in values.items() if v})

    for aff in data.affected:
        match aff.location:
            case 1 | 2 | 3 | 4 | 5 | 6:
                newloc = 1 << (aff.location - 1)
                aff.location = 1
                aff.specific = newloc
            case 12 | 37:
                aff.location = 5
                aff.specific = 1 << 1
            case 13:
                aff.location = 5
                aff.specific = 1 << 0
            case 14:
                aff.location = 5
                aff.specific = 1 << 2
            case 25:
                aff.location = 6
                aff.specific = 1 << 1
            case 26:
                aff.location = 6
                aff.specific = 1 << 0
            case 27:
                aff.location = 6
                aff.specific = 1 << 2
            case 28:
                aff.location = 6
                aff.specific = 1 << 3
            case 29:
                aff.location = 6
                aff.specific = 0xFF
            case 46:
                aff.location = 5
                aff.specific = 0xFF
            case 16:
                aff.location = 14
                aff.specific = 1 << 0
            case 21:
                aff.location = 11
                aff.specific = 1 << 1
            case 22:
                aff.location = 6
                aff.specific = 1 << 3
                aff.modifier = aff.modifier / 100.0
            case 17:
                aff.location = 18
                aff.specific = 1 << 2
            case 18:
                aff.location = 18
                aff.specific = 1 << 0
            case 32:
                aff.location = 21
                aff.specific = 1 << 0
            case 33:
                aff.location = 21
                aff.specific = 1 << 1
            case 34:
                aff.location = 20
                aff.specific = 1 << 0
            case 35:
                aff.location = 20
                aff.specific = 1 << 1
            case 41:
                aff.location = 25


    for i in range(128):
        if item_flags & (1 << i):
            reset_flag = True

            match i:
                case 9:
                    data.not_alignment.add("good")
                case 10:
                    data.not_alignment.add("evil")
                case 11:
                    data.not_alignment.add("neutral")
                case 12:
                    data.not_sensei.add(Sensei.roshi)
                case 13:
                    data.not_sensei.add(Sensei.piccolo)
                case 14:
                    data.not_sensei.add(Sensei.crane)
                case 15:
                    data.not_sensei.add(Sensei.nail)
                case 17:
                    data.not_sensei.add(Sensei.tapion)
                case 19:
                    data.not_sensei.add(Sensei.sixteen)
                case 20:
                    data.not_sensei.add(Sensei.dabura)
                case 21:
                    data.not_sensei.add(Sensei.ginyu)
                case 22:
                    data.not_race.add(Race.human)
                case 23:
                    data.not_race.add(Race.icer)
                case 24:
                    data.not_race.add(Race.saiyan)
                case 25:
                    data.not_race.add(Race.konatsu)
                case 29:
                    data.not_sensei.add(Sensei.bardock)
                case 30:
                    data.not_sensei.add(Sensei.kibito)
                case 31:
                    data.not_sensei.add(Sensei.frieza)
                case 41:
                    data.only_race.add(Race.human)
                case 42:
                    data.only_race.add(Race.icer)
                case 43:
                    data.only_race.add(Race.saiyan)
                case 44:
                    data.only_race.add(Race.konatsu)
                case 45:
                    data.only_sensei.add(Sensei.bardock)
                case 46:
                    data.only_sensei.add(Sensei.kibito)
                case 47:
                    data.only_sensei.add(Sensei.frieza)
                case 50:
                    data.not_sensei.add(Sensei.kurzak)
                case 51:
                    data.only_sensei.add(Sensei.kurzak)
                case 75:
                    data.only_sensei.add(Sensei.jinto)
                case 33 | 34 | 35 | 36 | 37 | 38 | 39 | 40 | 48 | 49 | 52 | 53 | 54 | 55 | 56 | 58 | 59 | 60 | 61 | 62:
                    pass
                case 63:
                    data.values["scouter_level"] = 500000
                case 64:
                    data.values["scouter_level"] = 10000000
                case 65:
                    data.values["scouter_level"] = 150000000
                case 66:
                    data.values["scouter_level"] = 9223372036854775807
                case 67 | 68 | 69 | 70 | 71:
                    data.values["scroll_level"] = i - 66
                case _:
                    reset_flag = False
            
            if reset_flag:
                item_flags &= ~(1 << i)
    
    for i in range(96):
        if item_flags & (1 << i):
            match i:
                case 16:
                    item_flags |= 1 << 9
                case 18:
                    item_flags |= 1 << 10
                case 26 | 27 | 28:
                    item_flags |= 1 << (i - 15)
                case 32:
                    item_flags |= 1 << 14
                case 57:
                    item_flags |= 1 << 15
                case 72 | 73 | 74:
                    item_flags |= 1 << (i - 56)
                case _:
                    if i >= 76 and i <= 94:
                        item_flags |= 1 << (i - 57)
    
    data.item_flags = flags_to_new_set(item_flags, ItemFlag)

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
        out.long_description = f.readuntil("~").rstrip("~")
        f.readline()
        out.action_description = f.readuntil("~").rstrip("~")
        f.readline()
        
        symbols = f.readline().split()
        out.item_type = ItemType(int(symbols[0]))
        raw_item_flags = flag_conv(symbols[1:5])
        
        out.wear_flags = flags_to_new_set(flag_conv(symbols[5:9]), WearFlag)
        out.affect_flags = flags_to_new_set(flag_conv(symbols[9:13]), AffectFlag)

        item_values = list(map(int, f.readline().split()))
        
        weight, cost, cost_per_day, level = list(map(int, f.readline().split()))
        out.weight = weight
        out.cost = cost
        out.cost_per_day = cost_per_day
        out.level = level

        pos = f.tell()
        
        def handle_z():
            data = f.readline()
            out.size = Size(int(data))

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
            line = f.readline()
            if not line:
                break
            if line.startswith("#"):
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
        
        convert_object(out, item_values, raw_item_flags)
        yield out

CYBER = {
    46: CharacterFlag.cyber_right_arm,
    47: CharacterFlag.cyber_left_arm,
    48: CharacterFlag.cyber_right_leg,
    49: CharacterFlag.cyber_left_leg
}

MOB_MODELS = {
    31: "Absorb",
    32: "Repair"
}

PLAYER_MODELS = {
    41: "Absorb",
    42: "Repair",
    43: "Sense"
}

def convert_characters(data: CharacterPrototype, player_flags: int, mob_flags: int):

    if data.race.has_tail():
        data.character_flags.add(CharacterFlag.tail)

    if data.id is not None:
        # remove 30 from player_flags bitset...
        player_flags &= ~(1 << 30)

        for k, v in CYBER.items():
            if player_flags & (1 << k):
                data.character_flags.add(v)
                player_flags &= ~(1 << k)
        
        for k, v in PLAYER_MODELS.items():
            if player_flags & (1 << k):
                data.android_model = v
                player_flags &= ~(1 << k)

    else:
        for k, v in MOB_MODELS.items():
            if mob_flags & (1 << k):
                data.android_model = v
                mob_flags &= ~(1 << k)

    data.player_flags = flags_to_new_set(player_flags, PlayerFlag)
    data.mob_flags = flags_to_new_set(mob_flags, MobFlag)

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
        out.short_description = f.readuntil("~").rstrip("~")
        f.readline()
        out.room_description = f.readuntil("~").rstrip("~")
        f.readline()
        out.look_description = f.readuntil("~").rstrip("~")
        f.readline()
        
        symbols = f.readline().split()
        raw_mob_flags = flag_conv(symbols[0:4])
        out.affect_flags = flags_to_new_set(flag_conv(symbols[4:8]), AffectFlag)
        out.stats["good_evil"] = int(symbols[8])
        
        letter = symbols[9]

        def parse_simple():
            symbols = f.readline().replace("+", " ").replace("d", " ").split()
            out.stats["level"] = int(symbols[0])
            out.stats["health"] = int(symbols[3])
            out.stats["ki"] = int(symbols[4])
            out.stats["stamina"] = int(symbols[5])
            out.stats["damage_mod"] = int(symbols[8])

            symbols = f.readline().split()
            out.stats["money_carried"] = int(symbols[0])
            race = int(symbols[2]) + 1
            if race > 23:
                race = 0
            out.race = Race(race)
            sensei = int(symbols[3]) + 1
            if sensei > 14:
                sensei = 0
            out.sensei = Sensei(sensei)

            symbols = f.readline().split()
            out.position = int(symbols[1])
            out.sex = Sex(int(symbols[2]))


        def parse_espec(line):
            etype, data = line.lower().split(":", 1)
            etype = etype.strip()
            data = data.strip()

            match etype:
                case "size":
                    out.stats["size"] = int(data)
                case "str":
                    out.stats["strength"] = int(data)
                case "int":
                    out.stats["intelligence"] = int(data)
                case "wis":
                    out.stats["wisdom"] = int(data)
                case "dex":
                    out.stats["agility"] = int(data)
                case "con":
                    out.stats["constitution"] = int(data)
                case "cha":
                    out.stats["speed"] = int(data)

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
        
        convert_characters(out, 0, raw_mob_flags)
        yield out

WHEREMAP: dict[int, str] = {
    20: "planet_earth",
    21: "planet_vegeta",
    22: "planet_frigid",
    23: "planet_konack",
    24: "planet_namek",
    25: "neo_nirvana",
    26: "afterlife",
    27: "space",
    30: "afterlife_hell",
    32: "planet_aether",
    33: "hyperbolic_time_chamber",
    34: "pendulum_past",
    37: "planet_yardrat",
    38: "planet_kanassa",
    39: "planet_arlia",
    41: "earth_orbit",
    42: "frigid_orbit",
    43: "konack_orbit",
    44: "namek_orbit",
    45: "vegeta_orbit",
    46: "aether_orbit",
    47: "yardrat_orbit",
    48: "kanassa_orbit",
    49: "arlia_orbit",
    50: "nebula",
    51: "asteroid",
    52: "wormhole",
    53: "space_station",
    54: "star",
    55: "planet_cerria",
    56: "cerria_orbit",
    66: "moon_zenith",
    68: "zenith_orbit",
}

def convert_room(data: Room, room_flags: int):
    # room_flags will be treated as a bitset.
    for k, v in WHEREMAP.items():
        if room_flags & (1 << k):
            data.where_flags.add(getattr(WhereFlag, v))
        # unset from room_flags...
            room_flags &= ~(1 << k)

    data.room_flags = flags_to_new_set(room_flags, RoomFlag)

EXTRA_TILES = {
    WhereFlag.nebula: "@m&@n",
    WhereFlag.asteroid: "@y:@n",
    WhereFlag.space_station: "@DS@n",
    WhereFlag.star: "@6 @n"
}


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
        raw_room_flags = flag_conv(symbols[0:4])
        convert_room(out, raw_room_flags)
        out.sector_type = SectorType(int(symbols[5]))

        def setup_dir(line):
            ex = Exit()
            direction = Direction(int(line[1:]))
            if keywords := f.readuntil("~").rstrip("~"):
                ex.keywords = keywords
            f.readline()
            if description := f.readuntil("~").rstrip("~"):
                ex.description = description
            f.readline()
            symbols = f.readline().split()

            match int(symbols[0]):
                case 1:
                    ex.exit_flags = set(["door"])
                case 2:
                    ex.exit_flags = set(["door", "pickproof"])
                case 3:
                    ex.exit_flags = set(["door", "secret"])
                case 4:
                    ex.exit_flags = set(["door", "pickproof", "secret"])
            
            if (key := int(symbols[1])) != -1:
                ex.key = key
            ex.destination = Location("room", int(symbols[2]))
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


LAND_EARTH = (
    ("Nexus City", 300),
    ("South Ocean", 800),
    ("Nexus Field", 1150),
    ("Cherry Blossom Mountain", 1180),
    ("Sandy Desert", 1287),
    ("Northern Plains", 1428),
    ("Korin's Tower", 1456),
    ("Kami's Lookout", 1506),
    ("Shadow Forest", 1636),
    ("Decrepit Area", 1710),
    ("West City", 19510),
    ("Hercule Beach", 2141),
    ("Satan City", 13020),
)
LAND_FRIGID = (
    ("Ice Crown City", 4264),
    ("Ice Highway", 4300),
    ("Topica Snowfield", 4351),
    ("Glug's Volcano", 4400),
    ("Platonic Sea", 4600),
    ("Slave City", 4800),
    ("Acturian Woods", 5100),
    ("Desolate Demesne", 5150),
    ("Chateau Ishran", 5165),
    ("Wyrm Spine Mountain", 5200),
    ("Cloud Ruler Temple", 5500),
    ("Koltoan Mine", 4944),
)
LAND_KONACK = (
    ("Tiranoc City", 8006),
    ("Great Oroist Temple", 8300),
    ("Elzthuan Forest", 8400),
    ("Mazori Farm", 8447),
    ("Dres", 8500),
    ("Colvian Farm", 8600),
    ("St Alucia", 8700),
    ("Meridius Memorial", 8800),
    ("Desert of Illusion", 8900),
    ("Plains of Confusion", 8954),
    ("Turlon Fair", 9200),
    ("Wetlands", 9700),
    ("Kerberos", 9855),
    ("Shaeras Mansion", 9864),
    ("Slavinus Ravine", 9900),
    ("Furian Citadel", 9949),
)
LAND_VEGETA = (
    ("Vegetos City", 2226),
    ("Blood Dunes", 2600),
    ("Ancestral Mountains", 2616),
    ("Destopa Swamp", 2709),
    ("Pride Forest", 2800),
    ("Pride Tower", 2899),
    ("Ruby Cave", 2615),
)
LAND_NAMEK = (
    ("Senzu Village", 11600),
    ("Guru's House", 10182),
    ("Crystalline Cave", 10474),
    ("Elder Village", 13300),
    ("Frieza's Ship", 10203),
    ("Kakureta Village", 10922),
)
LAND_AETHER = (
    ("Haven City", 12010),
    ("Serenity Lake", 12103),
    ("Kaiju Forest", 12300),
    ("Ortusian Temple", 12400),
    ("Silent Glade", 12480),
)
LAND_YARDART = (
    ("Yardra City", 14008),
    ("Jade Forest", 14100),
    ("Jade Cliffs", 14200),
    ("Mount Valaria", 14300),
)
LAND_KANASSA = (
    ("Aquis City", 14904),
    ("Yunkai Pirate Base", 15655),
)
LAND_CERRIA = (
    ("Cerria Colony", 17531),
    ("Crystalline Forest", 7950),
    ("Fistarl Volcano", 17420),
)
LAND_ARLIA = (
    ("Janacre", 16009),
    ("Arlian Wasteland", 16544),
    ("Arlia Mine", 16600),
)
LAND_ZENITH = (
    ("Utatlan City", 3412),
    ("Zenith Jungle", 3520),
    ("Ancient Castle", 19600),
)
DOCK_EARTH = (
    ("1", 409),
    ("2", 411),
    ("3", 412),
    ("4", 410),
    ("Nexus City", 300),
    ("South Ocean", 800),
    ("Nexus Field", 1150),
    ("Cherry Blossom Mountain", 1180),
    ("Sandy Desert", 1287),
    ("Northern Plains", 1428),
    ("Korin's Tower", 1456),
    ("Kami's Lookout", 1506),
    ("Shadow Forest", 1600),
    ("Decrepit Area", 1710),
    ("West City", 19510),
    ("Hercule Beach", 2141),
    ("Satan City", 13020),
)
DOCK_FRIGID = (
    ("1", 4264),
    ("2", 4263),
    ("3", 4261),
    ("4", 4262),
    ("Ice Crown City", 4264),
    ("Ice Highway", 4300),
    ("Topica Snowfield", 4351),
    ("Glug's Volcano", 4400),
    ("Platonic Sea", 4600),
    ("Slave City", 4800),
    ("Acturian Woods", 5100),
    ("Desolate Demesne", 5150),
    ("Chateau Ishran", 5165),
    ("Wyrm Spine Mountain", 5200),
    ("Cloud Ruler Temple", 5500),
    ("Koltoan Mine", 4944),
)
DOCK_KONACK = (
    ("1", 8195),
    ("2", 8196),
    ("3", 8197),
    ("4", 8198),
    ("Tiranoc City", 8006),
    ("Great Oroist Temple", 8300),
    ("Elzthuan Forest", 8400),
    ("Mazori Farm", 8447),
    ("Dres", 8500),
    ("Colvian Farm", 8600),
    ("St Alucia", 8700),
    ("Meridius Memorial", 8800),
    ("Desert of Illusion", 8900),
    ("Plains of Confusion", 8954),
    ("Turlon Fair", 9200),
    ("Wetlands", 9700),
    ("Kerberos", 9855),
    ("Shaeras Mansion", 9864),
    ("Slavinus Ravine", 9900),
    ("Furian Citadel", 9949),
)
DOCK_VEGETA = (
    ("1", 2319),
    ("2", 2318),
    ("3", 2320),
    ("4", 2322),
    ("Vegetos City", 2226),
    ("Blood Dunes", 2600),
    ("Ancestral Mountains", 2616),
    ("Destopa Swamp", 2709),
    ("Pride Forest", 2800),
    ("Pride Tower", 2899),
    ("Ruby Cave", 2615),
)
DOCK_NAMEK = (
    ("1", 11628),
    ("2", 11629),
    ("3", 11630),
    ("4", 11627),
    ("Senzu Village", 11600),
    ("Guru's House", 10182),
    ("Crystalline Cave", 10474),
    ("Elder Village", 13300),
    ("Frieza's Ship", 10203),
    ("Kakureta Village", 10922),
)
DOCK_AETHER = (
    ("1", 12003),
    ("2", 12004),
    ("3", 12006),
    ("4", 12005),
    ("Haven City", 12010),
    ("Serenity Lake", 12103),
    ("Kaiju Forest", 12300),
    ("Ortusian Temple", 12400),
    ("Silent Glade", 12480),
)

DOCK_YARDRAT = (
    ("1", 14003),
    ("2", 14004),
    ("3", 14005),
    ("4", 14006),
    ("Yardra City", 14008),
    ("Jade Forest", 14100),
    ("Jade Cliffs", 14200),
    ("Mount Valaria", 14300),
)

DOCK_KANASSA = (
    ("Aquis City", 14904),
    ("Yunkai Pirate Base", 15655),
)

DOCK_CERRIA = (
    ("Cerria Colony", 17531),
    ("Crystalline Forest", 7950),
    ("Fistarl Volcano", 17420),
)

DOCK_ARLIA = (
    ("1", 16065),
    ("2", 16066),
    ("3", 16067),
    ("4", 16068),
    ("Janacre", 16009),
    ("Arlian Wasteland", 16544),
    ("Arlia Mine", 16600),
    ("Kemabra Wastes", 16816),
)

DOCK_ZENITH = (
    ("Utatlan City", 3412),
    ("Zenith Jungle", 3520),
    ("Ancient Castle", 19600),
)



@dataclass
class ZoneHierarchy:
    name: str
    children: set[int] = field(default_factory=set)
    orbit: int | None = None
    flags: set[ZoneFlag] = field(default_factory=set)
    land_spots: list[tuple[str, int]] = field(default_factory=list)
    dock_spots: list[tuple[str, int]] = field(default_factory=list)

ZONES_TO_LINK: tuple[ZoneHierarchy, ...] = (
    ZoneHierarchy(
        name="@GEarth@n",
        children={2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 58, 67, 77, 130, 131, 134, 159, 162, 164, 195, 224},
        orbit=50,
        flags={ZoneFlag.ether_stream, ZoneFlag.has_moon},
        land_spots=LAND_EARTH,
        dock_spots=DOCK_EARTH,
    ),
    ZoneHierarchy(
        name="@YVegeta@n",
        children={22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
        orbit=53,
        flags=set(),
        land_spots=LAND_VEGETA,
        dock_spots=DOCK_VEGETA,
    ),
    ZoneHierarchy(
        name="@BZenith@n",
        children={34, 35, 196, 215},
        orbit=57,
        flags={ZoneFlag.ether_stream},
        land_spots=LAND_ZENITH,
        dock_spots=DOCK_ZENITH,
    ),
    ZoneHierarchy(
        name="Majinton",
        children={36, 37},
        orbit=None,
        flags=set(),
        land_spots=[],
        dock_spots=[],
    ),
    ZoneHierarchy(
        name="@CFrigid@n",
        children={40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 170},
        orbit=51,
        flags={ZoneFlag.has_moon},
        land_spots=LAND_FRIGID,
        dock_spots=DOCK_FRIGID,
    ),
    ZoneHierarchy(
        name="@gNamek@n",
        children={59, 96, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 128, 132, 133, 144, 154, 260},
        orbit=54,
        flags={ZoneFlag.ether_stream},
        land_spots=LAND_NAMEK,
        dock_spots=DOCK_NAMEK,
    ),
    ZoneHierarchy(
        name="Afterlife",
        children={60, 61, 62, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73, 74, 75, 217},
        orbit=None,
        flags=set(),
        land_spots=[],
        dock_spots=[],
    ),
    ZoneHierarchy(
        name="@BKanassa@n",
        children={76, 149, 150, 151, 152, 153, 156},
        orbit=58,
        flags=set(),
        land_spots=LAND_KANASSA,
        dock_spots=DOCK_KANASSA,
    ),
    ZoneHierarchy(
        name="@RCerria@n",
        children={78, 79, 174, 175, 176},
        orbit=198,
        flags=set(),
        land_spots=LAND_CERRIA,
        dock_spots=DOCK_CERRIA,
    ),
    ZoneHierarchy(
        name="@MKonack@n",
        children={80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 92, 93, 97, 98, 99, 127, 192, 193, 194},
        orbit=52,
        flags=set(),
        land_spots=LAND_KONACK,
        dock_spots=DOCK_KONACK,
    ),
    ZoneHierarchy(
        name="@MAether@n",
        children={120, 121, 122, 123, 124, 125, 155},
        orbit=55,
        flags={ZoneFlag.ether_stream, ZoneFlag.has_moon},
        land_spots=LAND_AETHER,
        dock_spots=DOCK_AETHER,
    ),
    ZoneHierarchy(
        name="Neo Nirvana",
        children={135, 136, 137, 138, 139, 145, 146, 147, 148},
        orbit=None,
        flags=set(),
        land_spots=[],
        dock_spots=[],
    ),
    ZoneHierarchy(
        name="@GArlia@n",
        children={160, 161, 165, 166, 167, 168, 169},
        orbit=59,
        flags=set(),
        land_spots=LAND_ARLIA,
        dock_spots=DOCK_ARLIA,
    ),
    ZoneHierarchy(
        name="Space",
        children={200, 201, 202, 203, 204, 206, 207, 208, 205, 163, 171, 172, 173, 178, 212, 256},
        orbit=None,
        flags=set(),
        land_spots=[],
        dock_spots=[],
    ),
    ZoneHierarchy(
        name="@mYardrat@n",
        children={140, 141, 142, 143},
        orbit=56,
        flags=set(),
        land_spots=LAND_YARDART,
        dock_spots=DOCK_YARDRAT,
    ),
)

class LegacyDatabase:
    
    def __init__(self):
        self.zones: dict[int, Zone] = dict()
        self.oproto: dict[int, ObjectPrototype] = dict()
        self.nproto: dict[int, CharacterPrototype] = dict()
        self.dgproto: dict[int, DgScriptPrototype] = dict()
        self.rooms: dict[int, Room] = dict()
        self.shops: dict[int, Shop] = dict()
        self.guilds: dict[int, Guild] = dict()
        self.areas: dict[int, "Area"] = dict()
        self.structures: dict[int, "Structure"] = dict()

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
            for reset in zone.resets:
                r = None

                match reset.command:
                    case "M" | "O" | "T" | "V":
                        r = self.rooms.get(reset.arg3)
                    case "D" | "R":
                        r = self.rooms.get(reset.arg1)
                
                if r is None:
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
