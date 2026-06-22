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
| twinslash | 19 | do_tslash | ✅ Done | Sword type check, weapon-level bonus, limb severing, tail cut |
| psyblast | 20 | do_psyblast | ✅ Done | Psychic Blast; ki drain + shocked condition |
| honoo | 21 | do_honoo | ✅ Done | burned condition; fire damage modifiers |
| dualbeam | 22 | do_dualbeam | ✅ Done | 3-hit multi with recursion guard |
| rogafufuken | 23 | do_rogafufuken | ✅ Done | stamina cost; parry counter-damage |
| bakuhatsuha | 24 | do_baku | ✅ Done | AOE, count-scaled dmg, group ally filter, always knockdown |
| kienzan | 25 | do_kienzan | ✅ Done | skill bonus 25/15/5%; instant-kill vs non-majin/bio if dmg > PL/5 |
| tribeam | 26 | do_tribeam | ✅ Done | |
| sbc | 27 | do_sbc | ✅ Done | Dodge causes room damage (+10) |
| finalflash | 28 | do_final | ✅ Done | |
| crusher | 29 | do_crusher | ✅ Done | Crusher Ball; perf2=+5 acc, perf3=cost reduction |
| ddslash | 30 | do_ddslash | ✅ Done | Sword type check, skill bonus, 33% blind via darkness_dragon_slash condition |
| pbarrage | 31 | do_pbarrage | ✅ Done | Psychic Barrage; head hit = 1.5× damage |
| hellflash | 32 | do_hellflash | ✅ Done | perf2=+5 acc, perf3=cost reduction |
| hellspear | 33 | do_hellspear | ✅ Done | AOE, no group filter, 25% knockdown |
| kakusanha | 34 | do_kakusanha | ✅ Done | AOE 5-beam, count-scaled dmg, group ally filter, env effects, room damage |
| scatter | 35 | do_scatter | ✅ Done | Scatter Shot; random +10–20 acc; Piccolo sensei cooldown |
| bigbang | 36 | do_bigbang | ✅ Done | |
| phoenix | 37 | do_pslash | ✅ Done | Sword type check, fire element, burned condition on hit |
| deathball | 38 | do_deathball | ✅ Done | -8 to -10 accuracy penalty |
| spiritball | 39 | do_spiritball | ✅ Done | dodging drains defender stamina (max/200) |
| genkidama | 40 | do_genki | ✅ Done | Spirit Bomb; gathers group ki; manifested projectile |
| genocide | 41 | do_geno | ✅ Done | Manifested projectile (vnum 83, ki_genocide script), kidist countdown, single-target detonation |
| kousengan | 42 | do_kousengan | ✅ Done | blocked by mystic_melody; +15 acc; sanctuary×3 damage |
| waterspikes | 43 | do_spike | ✅ Done | perf mechanics; ki refund; aqua barrier; head knockdown |
| spiral | 44/45 | do_spiral | ✅ Done | Spiral Comet; Condition with scheduled tick (first hit 50%/follow-ups 5% ki cost, 5 hit locations); PLR_SPIRAL refs replaced with char_condition_has |
| starbreaker | 46 | do_breaker | ✅ Done | EXP theft on hit scaled by level diff |
| waterrazor | 47 | do_razor | ✅ Done | no arm; android blocked; on_hit drain ki+stamina; aqua barrier |
| koteiru | 48 | do_koteiru | ✅ Done | aqua barrier; 25% freeze on hit vs non-demon |
| hspiral | 49 | do_hspiral | ✅ Done | |
| seishou | 50 | do_seishou | ✅ Done | no arm; mystic_melody blocked; molt_level ≥150 doubles dmg |
| starnova | 53 | do_nova | ✅ Done | AOE, time-of-day dmg bonus, group ally filter, 3 perf types |
| lightgrenade | 57 | do_lightgrenade | ✅ Done | Targeted with AoE splash: primary=full dmg, bystanders=0.5x; 25% knockdown |

---

## Special / Other Combat Commands

| command | C++ fn | Status | Notes |
|---------|--------|--------|-------|
| balefire | do_balefire | ✅ Done | random +10–20 acc; Piccolo sensei cooldown |
| blessedhammer | do_blessedhammer | ✅ Done | +15 acc; sanctuary×3 damage |
| combine | do_combine | ❌ C++ | |
| energize | do_energize | ✅ Done | Condition toggle; PREFERENCE_THROWING guard; do_throw updated to char_condition_has("energize") |
| malice | do_malice | ✅ Done | 6 random hit locations, time-of-day dmg bonus, dodge→room_dmg +20 |
| selfdestruct | do_selfd | ✅ Done | Condition with scheduled tick (phase 1→2 via skill roll), 3-phase command, grapple/room AOE, majin/bio survival |
| sunder | do_sunder | ✅ Done | |
| throw | do_throw | ✅ Done | Object-throw (weight/STR/CHA formula, multi-throw loop 1–3, weapon-level bonuses, energize, ICE/HOT effects) + person-throw (speed grab check, damage to both thrown and victim); mob AI pick_n_throw + Shadowdancer use char_cmd_execute |
| zen | do_zen | ✅ Done | Sword, mystic_melody block, limb severing 80%+, majin/bio regen, dodge→room_dmg +5 |

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