# Lua Combat Migration Tracker

Tracks all combat-relevant attack commands and their migration status from C++ to Lua.

**Key paths:**
- Attack definitions: `lua/characters/attacks/<id>.lua`
- Player commands: `lua/characters/commands/attacks/melee/<id>.lua` or `ki/<id>.lua`
- C++ attack handlers: `src/act.attack.cpp`, `src/act.offensive.cpp`
- Command table: `src/command.cpp`

**Status legend:**
- ✅ **Done** — Lua attack def + command file + C++ stub/cleanup complete
- 🔄 **Partial** — Lua attack def exists but command file missing or C++ not yet stubbed
- ❌ **C++** — Fully in C++, not yet started

---

## Melee — Player

| id | damtype# | C++ fn | Status | Notes |
|----|----------|--------|--------|-------|
| punch | 0 | do_punch | ✅ Done | |
| kick | 1 | do_kick | ✅ Done | |
| elbow | 2 | do_elbow | ✅ Done | |
| knee | 3 | do_knee | ✅ Done | |
| roundhouse | 4 | do_roundhouse | ✅ Done | |
| uppercut | 5 | do_uppercut | ✅ Done | |
| slam | 6 | do_slam | ✅ Done | |
| heeldrop | 8 | do_heeldrop | ✅ Done | |
| bash | 51 | do_bash | ✅ Done | |
| headbutt | 52 | do_headbutt | ✅ Done | |
| tailwhip | 56 | do_tailwhip | ✅ Done | |

---

## Melee — NPC Only

| id | C++ trigger | Status | Notes |
|----|-------------|--------|-------|
| bite | mob AI in fight.cpp | ✅ Done | All non-humanoid mobs |
| ram | do_ram → char_cmd_execute | ✅ Done | Non-humanoid; return; stub |
| strike | do_strike → char_cmd_execute | ✅ Done | Serpent NPCs; return; stub |
| breath | do_breath → char_cmd_execute | ✅ Done | Dragon NPCs; return; stub |

---

## Weapon-Triggered (no player command needed)

| id | Status | Notes |
|----|--------|-------|
| slash | ✅ Done | Triggered by weapon type |
| pierce | ✅ Done | Triggered by weapon type |
| crush | ✅ Done | Triggered by weapon type |
| stab | ✅ Done | Triggered by weapon type |
| shoot | ✅ Done | Triggered by weapon type |
| brawl | ✅ Done | Triggered by weapon type |

---

## Ki / Energy Attacks

| id | damtype# | C++ fn | Status | Notes |
|----|----------|--------|--------|-------|
| kiball | 7 | do_kiball | ✅ Done | |
| kiblast | 9 | do_kiblast | ✅ Done | Android damage bonus + knockout mastery |
| beam | 10 | do_beam | ✅ Done | Knockback mastery (try_move) |
| shogekiha | 10 | do_shogekiha | ✅ Done | Kibito no-cooldown; charge-drain mastery; ki regen |
| tsuihidan | 11 | do_tsuihidan | ✅ Done | Stamina drain mastery |
| renzo | 12 | do_renzo | ✅ Done | Multi-shot count via effective_accuracy; Nail sensei bonus |
| kamehameha | 13 | do_kamehameha | ✅ Done | Ki refund mastery; perf type 3 extra lag |
| masenko | 14 | do_masenko | ✅ Done | Piccolo sensei bonus; stamina drain |
| dodonpa | 15 | do_dodonpa | ✅ Done | Kibito sensei bonus; ki drain |
| galikgun | 16 | do_galikgun | ✅ Done | Ki refund mastery |
| deathbeam | 17 | do_deathbeam | ✅ Done | base_accuracy 1.3; lifeforce drain; perf type 3 lag |
| eraser | 18 | do_eraser | ✅ Done | Eraser Cannon; ki refund mastery |
| twinslash | 19 | do_tslash | ❌ C++ | Deferred to batch 3: needs sword-type-check, limb severance |
| psyblast | 20 | do_psyblast | ✅ Done | Psychic Blast; ki drain + shocked condition |
| honoo | 21 | do_honoo | ✅ Done | burned condition; fire damage modifiers |
| dualbeam | 22 | do_dualbeam | ✅ Done | 3-hit multi with recursion guard |
| rogafufuken | 23 | do_rogafufuken | ✅ Done | stamina cost; parry counter-damage |
| bakuhatsuha | 24 | do_baku | ❌ C++ | |
| kienzan | 25 | do_kienzan | ❌ C++ | |
| tribeam | 26 | do_tribeam | ✅ Done | |
| sbc | 27 | do_sbc | ❌ C++ | Deferred: skill not in spell_parser.cpp; dodge causes room damage |
| finalflash | 28 | do_final | ✅ Done | |
| crusher | 29 | do_crusher | ✅ Done | Crusher Ball; perf2=+5 acc, perf3=cost reduction |
| ddslash | 30 | do_ddslash | ❌ C++ | Darkness Dragon Slash |
| pbarrage | 31 | do_pbarrage | ✅ Done | Psychic Barrage; head hit = 1.5× damage |
| hellflash | 32 | do_hellflash | ✅ Done | perf2=+5 acc, perf3=cost reduction |
| hellspear | 33 | do_hellspear | ❌ C++ | Hell Spear Blast |
| kakusanha | 34 | do_kakusanha | ❌ C++ | |
| scatter | 35 | do_scatter | ✅ Done | Scatter Shot; random +10–20 acc; Piccolo sensei cooldown |
| bigbang | 36 | do_bigbang | ✅ Done | |
| phoenix | 37 | do_pslash | ❌ C++ | Phoenix Slash |
| deathball | 38 | do_deathball | ❌ C++ | |
| spiritball | 39 | do_spiritball | ❌ C++ | |
| genkidama | 40 | do_genki | 🔄 Partial | Attack def exists; ki/ command + C++ stub needed |
| genocide | 41 | do_geno | ❌ C++ | |
| kousengan | 42 | do_kousengan | ❌ C++ | |
| waterspikes | 43 | do_spike | ❌ C++ | |
| spiral | 44/45 | do_spiral | ❌ C++ | Spiral Comet (two variants) |
| starbreaker | 46 | do_breaker | ❌ C++ | |
| waterrazor | 47 | do_razor | ❌ C++ | |
| koteiru | 48 | do_koteiru | ❌ C++ | Koteiru Bakuha |
| hspiral | 49 | do_hspiral | ❌ C++ | Hell Spiral |
| seishou | 50 | do_seishou | ❌ C++ | Seishou Enko |
| starnova | 53 | do_nova | ❌ C++ | |
| lightgrenade | 57 | do_lightgrenade | ❌ C++ | |

---

## Special / Other Combat Commands

| command | C++ fn | Status | Notes |
|---------|--------|--------|-------|
| balefire | do_balefire | ❌ C++ | |
| blessedhammer | do_blessedhammer | ❌ C++ | |
| combine | do_combine | ❌ C++ | |
| energize | do_energize | ❌ C++ | |
| malice | do_malice | ❌ C++ | |
| selfdestruct | do_selfd | ❌ C++ | |
| sunder | do_sunder | ❌ C++ | |
| throw | do_throw | ❌ C++ | |
| zen | do_zen | ❌ C++ | |

---

## Non-Attack Combat Commands (migrate separately, lower priority)

| command | C++ fn | Notes |
|---------|--------|-------|
| attack | do_attack | Basic attack dispatcher |
| powerup | do_powerup | Power-up mechanic |
| rescue | do_rescue | Rescue ally |
| assist | do_assist | Assist ally |
| kill | do_kill | Kill command |
| flee | do_flee | Flee from combat |
| block | do_block | Blocking mechanic |
| grapple | do_grapple | Grapple mechanic |
| spar | do_spar | Spar toggle |
| trip | do_trip | Trip attack |
| zanzoken | do_zanzoken | Afterimage technique |


## Other tasks
- Make NPC AI auto-combat managed by Lua as lua/characters/npc_combat.lua instead of the C++ hardcode; set_fighting should attach the script. alternatively the fighting.lua fighting Condition could have special behavior for NPCs; that might be simpler.
- 