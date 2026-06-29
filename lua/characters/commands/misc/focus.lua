local dbat = require("dbat")

local Search = dbat.lib.search.new
local text   = dbat.lib.text

local AFF = dbat.consts.aff_flags
local BON = dbat.consts.bonuses
local MOB = dbat.consts.mob_flags
local PLR = dbat.consts.player_flags
local POS = dbat.consts.positions

local PULSE_2SEC = 20
local SMH = dbat.consts.secs_per_mud_hour

local function blocked_by_position(ch)
    local pos = ch:position_get()
    if pos >= POS.STANDING or pos == POS.FIGHTING then return false end

    if pos == POS.DEAD then
        ch:send_line("Lie still; you are DEAD!!! :-(")
    elseif pos == POS.INCAP or pos == POS.MORTALLYW then
        ch:send_line("You are in a pretty bad shape, unable to do anything!")
    elseif pos == POS.STUNNED then
        ch:send_line("All you can do right now is think about the stars!")
    elseif pos == POS.SLEEPING then
        ch:send_line("In your dreams, or what?")
    elseif pos == POS.RESTING then
        ch:send_line("Nah... You feel too relaxed to do that..")
    elseif pos == POS.SITTING then
        ch:send_line("Maybe you should get on your feet first?")
    end

    return true
end

local function room_target(ch, name)
    if name == nil or name == "" then return nil end
    return Search(ch):add_room_people(ch:room_get()):find_one(name)
end

local function drain_reveal(ch, div)
    div = div or 20
    ch:meter_mod_int("ki", -math.floor(ch:meter_max("ki") / div))
    ch:reveal_hiding(0)
end

local function ki_check(ch, div)
    div = div or 20
    return ch:meter_current("ki") >= math.floor(ch:meter_max("ki") / div)
end

local function roll_aff_duration(num, add)
    local start = math.floor(num / 20)
    local finish = math.floor(num / 10)
    if finish < start then finish = start end
    return add + math.random(start, finish)
end

local function add_timed(tgt, cond, cat, src, dur)
    tgt:condition_apply_with_duration(cond, dur * SMH, cat, src)
end

local function skill_success(ch, skill)
    return ch:skill_get(skill) >= dbat.axion_dice(0)
end

local function use_self_message(ch, arg, vict)
    ch:send_line("Use focus %s, not focus %s %s.", arg, arg, vict:name_get())
end

local function same_group_family(ch, vict)
    local cm = ch:following_get()
    local vm = vict:following_get()
    return (vm and vm:is_same(ch))
        or (cm and cm:is_same(vict))
        or (cm and vm and cm:is_same(vm))
end

local function maybe_kai_group_xp(ch, vict)
    if ch:race_get() ~= "kai" then return end
    if not (ch:condition_has("group") and vict:condition_has("group")) then return end
    if not same_group_family(ch, vict) then return end
    local next_exp = ch:level_exp(ch:stat_get("level") + 1)
    if next_exp - ch:stat_get("experience") > 0 and math.random(1, 3) == 3 then
        ch:stat_mod("experience", math.floor(next_exp * 0.05))
    end
end

local function jinto_gain_for(ch, skill, vict)
    if ch:race_get() ~= "jinto" then return end
    local receiver = vict or ch
    local next_exp = receiver:level_exp(receiver:stat_get("level") + 1)
    if next_exp - receiver:stat_get("experience") <= 0 then return end
    if ch:stat_get("practices") < 15 or math.random(1, 4) < 3 then return end

    local pct = 0
    local rank = ch:skill_get(skill)
    if rank >= 100 then
        pct = 0.15
    elseif rank >= 60 then
        pct = 0.10
    elseif rank >= 40 then
        pct = 0.05
    end

    local gain = math.floor(next_exp * pct)
    if gain <= 0 then return end
    ch:stat_mod("practices", -15)
    receiver:stat_mod("experience", gain)

    if receiver:is_same(ch) then
        ch:send_line("@GYou gain @g%s@G experience due to your excellence with this skill.@n", text.add_commas(gain))
    else
        receiver:send_line("@GYou gain @g%s@G experience due to the level of enlightenment you have received!@n", text.add_commas(gain))
    end
end

local function apply_buff(ch, vict, arg, spec)
    local self_cast = vict == nil
    local target = vict or ch

    if not self_cast then
        if not ch:can_kill(target, 2) then return end
        if target:is_same(ch) then use_self_message(ch, arg, target); return end
        if target:is_npc() then ch:send_line("Whatever would you waste your ki on them for?"); return end
    end

    if target:condition_has(spec.cond) then ch:send_line(self_cast and spec.already_self or spec.already_target); return end
    if spec.bonus and target:bonus_flagged(spec.bonus.flag) and target:stat_get(spec.bonus.stat) + spec.bonus.add > 70 then
        ch:send_line(self_cast and spec.bonus.self_msg or spec.bonus.target_msg)
        return
    end
    if spec.bonus2 and target:bonus_flagged(spec.bonus2.flag) and target:stat_get(spec.bonus2.stat) + spec.bonus2.add > 70 then
        ch:send_line(self_cast and spec.bonus2.self_msg or spec.bonus2.target_msg)
        return
    end
    if not ki_check(ch) then ch:send_line(self_cast and spec.ki_self or spec.ki_target); return end

    if not skill_success(ch, spec.skill) then
        drain_reveal(ch)
        if self_cast then
            ch:act(spec.fail_self_char, true, nil, nil, "char")
            ch:act(spec.fail_self_room, true, nil, nil, "room")
        else
            ch:act(spec.fail_target_char, true, nil, target, "char")
            ch:act(spec.fail_target_vict, true, nil, target, "vict")
            ch:act(spec.fail_target_room, true, nil, target, "notvict")
        end
        return
    end

    local dur = self_cast and spec.self_duration(ch) or roll_aff_duration(ch:stat_get("intelligence"), 2)
    add_timed(target, spec.cond, "skill", spec.source, dur)
    drain_reveal(ch)
    if self_cast then
        ch:act(spec.ok_self_char, true, nil, nil, "char")
        ch:act(spec.ok_self_room, true, nil, nil, "room")
    else
        ch:act(spec.ok_target_char, true, nil, target, "char")
        ch:act(spec.ok_target_vict, true, nil, target, "vict")
        ch:act(spec.ok_target_room, true, nil, target, "notvict")
    end
    if spec.after_success then spec.after_success(ch, target, self_cast) end
end

local BUFFS = {
    tough = {
        skill = "tough skin", cond = "stoneskin", source = "tough skin",
        missing = "Focus your ki into who's skin?",
        already_self = "You already have tough skin!", already_target = "They already have tough skin!",
        ki_self = "You do not have enough ki to infuse into your skin.", ki_target = "You do not have enough ki to infuse into their skin.",
        self_duration = function(ch) return math.floor(ch:stat_get("intelligence") / 20) end,
        fail_self_char = "You focus ki into your skin, but fail in making it tough!",
        fail_self_room = "$n focuses ki into $s skin, but fails in making it tough!",
        ok_self_char = "You focus ki into your skin, making it tough!",
        ok_self_room = "$n focuses ki into $s skin, making it tough!",
        fail_target_char = "You focus ki into $N's skin, but fail in making it tough!",
        fail_target_vict = "$n focuses ki into your skin, but fails in making it tough!",
        fail_target_room = "$n focuses ki into $N's skin, but fails in making it tough!",
        ok_target_char = "You focus ki into $N's skin, making it tough!",
        ok_target_vict = "$n focuses ki into your skin, making it tough!",
        ok_target_room = "$n focuses ki into $N's skin, making it tough!",
    },
    might = {
        skill = "might", cond = "might", source = "might",
        missing = "Focus your ki into who's muscles?",
        already_self = "You already have mighty muscles!", already_target = "They already have mighty muscles!",
        ki_self = "You do not have enough ki to infuse into your muscles.", ki_target = "You do not have enough ki to infuse into their muscles.",
        self_duration = function(ch) return roll_aff_duration(ch:stat_get("intelligence"), 2) end,
        bonus = { flag = BON.WIMP, stat = "strength", add = 10, self_msg = "Your body is not able to withstand increasing its strength beyond 70.", target_msg = "Their body is not able to withstand increasing its strength beyond 70." },
        bonus2 = { flag = BON.FRAIL, stat = "constitution", add = 2, self_msg = "Your body is not able to withstand increasing its constitution beyond 70.", target_msg = "Their body is not able to withstand increasing its constitution beyond 70." },
        fail_self_char = "You focus ki into your muscles, but fail in making them mighty!",
        fail_self_room = "$n focuses ki into $s muscles, but fails in making them mighty!",
        ok_self_char = "You focus ki into your muscles, making them mighty!",
        ok_self_room = "$n focuses ki into $s muscles, making them mighty!",
        fail_target_char = "You focus ki into $N's muscles, but fail in making them mighty!",
        fail_target_vict = "$n focuses ki into your muscles, but fails in making them mighty!",
        fail_target_room = "$n focuses ki into $N's muscles, but fails in making them mighty!",
        ok_target_char = "You focus ki into $N's muscles, making them mighty!",
        ok_target_vict = "$n focuses ki into your muscles, making them mighty!",
        ok_target_room = "$n focuses ki into $N's muscles, making them mighty!",
    },
    enlighten = {
        skill = "enlighten", cond = "enlighten", source = "enlighten",
        missing = "Focus your ki into who's mind?",
        already_self = "You already have superior wisdom!", already_target = "They already have superior wisdom!",
        ki_self = "You do not have enough ki to use this skill.", ki_target = "You do not have enough ki to use this skill.",
        self_duration = function(ch) return roll_aff_duration(ch:stat_get("intelligence"), 2) end,
        bonus = { flag = BON.FOOLISH, stat = "wisdom", add = 10, self_msg = "You're not able to withstand increasing your wisdom beyond 70.", target_msg = "They're not able to withstand increasing their wisdom beyond 70." },
        fail_self_char = "You focus ki into your mind, but fail in awakening it to cosmic wisdom!",
        fail_self_room = "$n focuses ki into $s mind, but fails in awakening it to cosmic wisdom!",
        ok_self_char = "You focus ki into your mind, awakening it to cosmic wisdom!",
        ok_self_room = "$n focuses ki into $s mind, awakening it to cosmic wisdom!",
        fail_target_char = "You focus ki into $N's mind, but fail in awakening it to cosmic wisdom!",
        fail_target_vict = "$n focuses ki into your mind, but fails in awakening it to cosmic wisdom!",
        fail_target_room = "$n focuses ki into $N's mind, but fails in awakening it to cosmic wisdom!",
        ok_target_char = "You focus ki into $N's mind, awakening it to cosmic wisdom!",
        ok_target_vict = "$n focuses ki into your mind, awakening it to cosmic wisdom!",
        ok_target_room = "$n focuses ki into $N's mind, awakening it to cosmic wisdom!",
        after_success = function(ch, target, self_cast) jinto_gain_for(ch, "enlighten", self_cast and nil or target) end,
    },
    genius = {
        skill = "genius", cond = "genius", source = "genius",
        missing = "Focus your ki into who's mind?",
        already_self = "You already have superior intelligence!", already_target = "They already have superior intelligence!",
        ki_self = "You do not have enough ki to infuse into your mind.", ki_target = "You do not have enough ki to infuse into their mind.",
        self_duration = function(ch) return roll_aff_duration(ch:stat_get("intelligence"), 2) end,
        bonus = { flag = BON.DULL, stat = "intelligence", add = 10, self_msg = "You're not able to withstand increasing your intelligence beyond 70.", target_msg = "They're not able to withstand increasing their intelligence beyond 70." },
        fail_self_char = "You focus ki into your mind, but fail in making it work faster!",
        fail_self_room = "$n focuses ki into $s muscles, but fails in making it work faster!",
        ok_self_char = "You focus ki into your mind, making it work faster!",
        ok_self_room = "$n focuses ki into $s mind, making it work faster!",
        fail_target_char = "You focus ki into $N's mind, but fail in making it work faster!",
        fail_target_vict = "$n focuses ki into your mind, but fails in making it work faster!",
        fail_target_room = "$n focuses ki into $N's mind, but fails in making it work faster!",
        ok_target_char = "You focus ki into $N's mind, making it work faster!",
        ok_target_vict = "$n focuses ki into your mind, making it work faster!",
        ok_target_room = "$n focuses ki into $N's mind, making it work faster!",
        after_success = function(ch, target, self_cast) if not self_cast then maybe_kai_group_xp(ch, target) end end,
    },
    flex = {
        skill = "flex", cond = "flex", source = "flex",
        missing = "Focus your ki into who's limbs?",
        already_self = "You already have superior agility!", already_target = "They already have superior agility!",
        ki_self = "You do not have enough ki to infuse into your limbs.", ki_target = "You do not have enough ki to infuse into their limbs.",
        self_duration = function(ch) return roll_aff_duration(ch:stat_get("intelligence"), 2) end,
        bonus = { flag = BON.CLUMSY, stat = "dexterity", add = 10, self_msg = "You're not able to withstand increasing your agility beyond 70.", target_msg = "They're not able to withstand increasing their agility beyond 70." },
        fail_self_char = "You focus ki into your limbs, but fail in making them more flexible!",
        fail_self_room = "$n focuses ki into $s muscles, but fails in making them more flexible!",
        ok_self_char = "You focus ki into your limbs, making them more flexible!",
        ok_self_room = "$n focuses ki into $s limbs, making them more flexible!",
        fail_target_char = "You focus ki into $N's limbs, but fail in making them more flexible!",
        fail_target_vict = "$n focuses ki into your limbs, but fails in making them more flexible!",
        fail_target_room = "$n focuses ki into $N's limbs, but fails in making them more flexible!",
        ok_target_char = "You focus ki into $N's limbs, making them more flexible!",
        ok_target_vict = "$n focuses ki into your limbs, making them more flexible!",
        ok_target_room = "$n focuses ki into $N's limbs, making them more flexible!",
        after_success = function(ch, target, self_cast) if not self_cast then maybe_kai_group_xp(ch, target) end end,
    },
}

local function do_wither(ch, name)
    if not ch:know_skill("wither") then return end
    local vict = room_target(ch, name)
    if not vict then ch:send_line("Focus your ki into who's muscles?"); return end
    if vict:is_same(ch) then ch:send_line("You don't want to wither your own body!"); return end
    if not ch:can_kill(vict, 2) then return end
    if vict:condition_has("wither") then ch:send_line("They already have been withered!"); return end
    if not ki_check(ch) then ch:send_line("You do not have enough ki to wither them."); return end
    if not skill_success(ch, "wither") then
        drain_reveal(ch)
        ch:act("You focus ki into $N's body, but fail in withering it!", true, nil, vict, "char")
        ch:act("$n focuses ki into your body, but fails in withering it!", true, nil, vict, "vict")
        ch:act("$n focuses ki into $N's body, but fails in withering it!", true, nil, vict, "notvict")
        return
    end
    drain_reveal(ch)
    vict:condition_add("wither", "skill", "wither")
    ch:act("You focus ki into $N's body, and succeed in withering it!", true, nil, vict, "char")
    ch:act("$n focuses ki into your body, and succeeds in withering it!", true, nil, vict, "vict")
    ch:act("$n focuses ki into $N's body, and succeeds in withering it!", true, nil, vict, "notvict")
end

local function do_bless(ch, arg, name)
    if not ch:know_skill("bless") then return end
    local vict = room_target(ch, name)
    local self_cast = not vict
    local target = vict or ch
    if name ~= "" and not vict then ch:send_line("Bless who?"); return end
    if vict then
        if not ch:can_kill(vict, 2) then return end
        if vict:is_same(ch) then use_self_message(ch, arg, vict); return end
        if vict:condition_has("bless") then ch:send_line("They already have been blessed!"); return end
        if vict:is_npc() then ch:send_line("Whatever would you waste your ki on them for?"); return end
    elseif ch:condition_has("bless") then
        ch:send_line("You already are blessed!")
        return
    end
    if not ki_check(ch) then ch:send_line("You do not have enough ki to bless."); return end
    if not skill_success(ch, "bless") then
        drain_reveal(ch)
        if self_cast then
            ch:act("You focus ki while chanting spiritual words. Your blessing does nothing though, you must have messed up!", true, nil, nil, "char")
            ch:act("$n focuses ki while chanting spiritual words. $n seems disappointed.", true, nil, nil, "room")
        else
            ch:act("You focus ki while chanting spiritual words. Your blessing fails!", true, nil, nil, "char")
            ch:act("$n focuses ki while chanting spiritual words. $n places a hand on your head, but nothing happens!", true, nil, vict, "vict")
            ch:act("$n focuses ki while chanting spiritual words. $n places a hand on $N's head, but nothing happens!", true, nil, vict, "notvict")
        end
        return
    end
    drain_reveal(ch)
    local level = (self_cast and ch:race_get() == "kabito" or (not self_cast and ch:race_get() == "kai")) and ch:skill_get("bless") or 0
    if self_cast then
        target:condition_apply("bless", "affect", "bless")
    else
        add_timed(target, "bless", "affect", "bless", roll_aff_duration(ch:stat_get("intelligence"), 3))
    end
    target:condition_number_set("bless", "level", level)
    if self_cast then
        ch:act("You focus ki while chanting spiritual words. You feel your body recovering at above normal speed!", true, nil, nil, "char")
        ch:act("$n focuses ki while chanting spiritual words. $n smiles after finishing $s chant.", true, nil, nil, "room")
    else
        ch:act("You focus ki while chanting spiritual words. Blessing $N with faster regeneration!", true, nil, target, "char")
        ch:act("$n focuses ki while chanting spiritual words. $n then places a hand on your head, blessing you!", true, nil, target, "vict")
        ch:act("$n focuses ki while chanting spiritual words. $n then places a hand on $N's head, blessing them!", true, nil, target, "notvict")
        maybe_kai_group_xp(ch, target)
    end
    if target:condition_has("curse") then
        target:send_line("Your cursing was nullified!")
        target:condition_remove("curse", "focus")
    end
end

local function do_curse(ch, arg, name)
    if not ch:know_skill("curse") then return end
    local vict = room_target(ch, name)
    local self_cast = name == ""
    local target = self_cast and ch or vict
    if not target then ch:send_line("Curse who?"); return end
    if not self_cast then
        if not ch:can_kill(target, 0) then return end
        if target:is_same(ch) then use_self_message(ch, arg, target); return end
        if target:is_npc() then ch:send_line("Whatever would you waste your ki on them for?"); return end
    end
    if target:condition_has("curse") then ch:send_line(self_cast and "You already are cursed!" or "They already have been cursed!"); return end
    if target:race_get() == "demon" then ch:send_line(self_cast and "You are immune to curses!" or "They are immune to curses!"); return end
    if not ki_check(ch) then ch:send_line("You do not have enough ki to CURSE."); return end
    if not skill_success(ch, "curse") then
        drain_reveal(ch)
        if self_cast then
            ch:act("You focus ki while chanting demonic words. Your cursing does nothing though, you must have messed up!", true, nil, nil, "char")
            ch:act("$n focuses ki while chanting demonic words. $n seems disappointed.", true, nil, nil, "room")
        else
            ch:act("You focus ki while chanting demonic words. Your cursing fails!", true, nil, nil, "char")
            ch:act("$n focuses ki while chanting demonic words. $n places a hand on your head, but nothing happens!", true, nil, target, "vict")
            ch:act("$n focuses ki while chanting demonic words. $n places a hand on $N's head, but nothing happens!", true, nil, target, "notvict")
        end
        return
    end
    add_timed(target, "curse", "affect", "curse", roll_aff_duration(ch:stat_get("intelligence"), 3))
    drain_reveal(ch)
    if self_cast then
        ch:act("You focus ki while chanting demonic words. You feel your body recovering at below normal speed!", true, nil, nil, "char")
        ch:act("$n focuses ki while chanting demonic words. $n grins after finishing $s chant.", true, nil, nil, "room")
    else
        ch:act("You focus ki while chanting demonic words. cursing $N with slower regeneration!", true, nil, target, "char")
        ch:act("$n focuses ki while chanting demonic words. $n then places a hand on your head, cursing you!", true, nil, target, "vict")
        ch:act("$n focuses ki while chanting demonic words. $n then places a hand on $N's head, cursing them!", true, nil, target, "notvict")
    end
    if target:condition_has("bless") then
        target:send_line("Your blessing was nullified!")
        target:condition_remove("bless", "affect_removed")
    end
end

local function do_yoik(ch, name)
    if not ch:know_skill("yoikominminken") then return end
    local vict = room_target(ch, name)
    if not vict then ch:send_line("Use Yoikominminken on who?"); return end
    if not ch:can_kill(vict, 0) then return end
    if vict:aff_flagged(AFF.SLEEP) or vict:condition_has("yoikominminken") then ch:send_line("They already have been put to sleep!"); return end
    if vict:player_flagged(PLR.EYEC) then ch:send_line("Their eyes are closed!"); return end
    if vict:aff_flagged(AFF.BLIND) then ch:send_line("They appear to be blind!"); return end
    if not ki_check(ch) then ch:send_line("You do not have enough ki to use Yoikominminken."); return end
    if vict:bonus_flagged(BON.INSOMNIAC) then
        drain_reveal(ch)
        ch:act("You focus ki while moving your hands in lulling patterns, but $N doesn't look the least bit sleepy!", true, nil, vict, "char")
        ch:act("$n focuses ki while moving $s hands in a lulling pattern, but you just don't feel tired.", true, nil, vict, "vict")
        ch:act("$n focuses ki while moving $s hands in a lulling pattern, but $N doesn't look the least bit sleepy!", true, nil, vict, "notvict")
        return
    end
    if not skill_success(ch, "yoikominminken") or ch:stat_get("intelligence") + math.random(1, 3) < vict:stat_get("intelligence") + math.random(1, 5) then
        drain_reveal(ch)
        ch:act("You focus ki while moving your hands in lulling patterns, but fail to put $N to sleep!", true, nil, vict, "char")
        ch:act("$n focuses ki while moving $s hands in a lulling pattern, but you resist the technique!", true, nil, vict, "vict")
        ch:act("$n focuses ki while moving $s hands in a lulling pattern, but $N resists the technique!", true, nil, vict, "notvict")
        return
    end
    add_timed(vict, "yoikominminken", "skill", "yoikominminken", math.random(1, 2))
    drain_reveal(ch)
    ch:act("You focus ki while moving your hands in lulling patterns, putting $N to sleep!", true, nil, vict, "char")
    ch:act("$n focuses ki while moving $s hands in a lulling pattern, before you realise it you are asleep!", true, nil, vict, "vict")
    ch:act("$n focuses ki while moving $s hands in a lulling pattern, putting $N to sleep!", true, nil, vict, "notvict")
    vict:position_set(POS.SLEEPING)
    if vict:condition_has("flying") then vict:condition_remove("flying", "stop_flying") end
end

local function do_vigor(ch, name)
    if not ch:know_skill("vigor") then return end
    local vict = room_target(ch, name)
    local self_cast = name == ""
    local target = self_cast and ch or vict
    if not target then ch:send_line("VIGOR who?"); return end
    if not self_cast then
        if not ch:can_kill(target, 2) then return end
        if target:is_npc() then ch:send_line("Whatever would you waste your ki on them for?"); return end
    end
    if not ki_check(ch, 10) then ch:send_line("You do not have enough ki to use vigor."); return end
    if not self_cast and target:meter_current("stamina") >= target:meter_max("stamina") then
        ch:send_line(self_cast and "You already have full stamina." or "They already have full stamina.")
        return
    end
    if not skill_success(ch, "vigor") then
        drain_reveal(ch, 10)
        if self_cast then
            ch:act("You focus ki into your very cells, but fail at re-engerizing them!", true, nil, nil, "char")
            ch:act("$n focuses ki and glows green for a moment, $e then frowns.", true, nil, nil, "room")
        else
            ch:act("You focus ki into $N's very cells, and fail at re-energizing them!", true, nil, target, "char")
            ch:act("$n focuses ki into your very cells, but nothing happens!", true, nil, target, "vict")
            ch:act("$n focuses ki and $N glows green for a moment, $N frowns.", true, nil, target, "notvict")
        end
        ch:wait_set(PULSE_2SEC)
        return
    end
    if self_cast and target:meter_current("stamina") >= target:meter_max("stamina") then
        ch:send_line("You already have full stamina.")
        return
    end
    local div = ch:bonus_flagged(BON.HEALER) and 8 or 10
    target:meter_mod_int("stamina", math.floor(target:meter_max("ki") / div))
    drain_reveal(ch, div)
    if self_cast then
        ch:act("You focus ki into your very cells, and manage to re-energize them!", true, nil, nil, "char")
        ch:act("$n focuses ki and glows green for a moment, $e then smiles.", true, nil, nil, "room")
    else
        ch:act("You focus ki into $N's very cells, and manage to re-energize them!", true, nil, target, "char")
        ch:act("$n focuses ki into your very cells, and manages to re-energize them!", true, nil, target, "vict")
        ch:act("$n focuses ki and $N glows green for a moment, $N smiles.", true, nil, target, "notvict")
    end
    ch:wait_set(PULSE_2SEC)
end

local function do_cure(ch, arg, name)
    if not ch:know_skill("cure poison") then return end
    local vict = room_target(ch, name)
    local self_cast = name == ""
    local target = self_cast and ch or vict
    if not target then ch:send_line("cure who?"); return end
    if not self_cast then
        if not ch:can_kill(target, 2) then return end
        if target:is_same(ch) then use_self_message(ch, arg, target); return end
    end
    if not target:condition_has("poison") then ch:send_line(self_cast and "You are not poisoned!" or "They are not poisoned!"); return end
    if not ki_check(ch) then ch:send_line("You do not have enough ki to cure."); return end
    if not skill_success(ch, "cure poison") then
        drain_reveal(ch)
        if self_cast then
            ch:act("You focus ki and aim a pulsing light at your body. Nothing happens!", true, nil, nil, "char")
            ch:act("$n focuses ki and aims a pulsing light at $s body. Nothing seems to happen.", true, nil, nil, "room")
        else
            ch:act("You focus ki and aim a pulsing light at $N's body. Nothing happens.", true, nil, target, "char")
            ch:act("$n focuses ki and aims a pulsing light at your body. You are STILL poisoned!", true, nil, target, "vict")
            ch:act("$n focuses ki and aims a pulsing light at $N's body. $N looks disappointed.", true, nil, target, "notvict")
        end
        return
    end
    drain_reveal(ch)
    if self_cast then
        ch:act("You focus ki and aim a pulsing light at your body. You feel the poison in your blood disappear!", true, nil, nil, "char")
        ch:act("$n focuses ki and aims a pulsing light at $s body. $n smiles.", true, nil, nil, "room")
    else
        ch:act("You focus ki and aim a pulsing light at $N's body. $e is cured.", true, nil, target, "char")
        ch:act("$n focuses ki and aims a pulsing light at your body. You have been cured of your poison!", true, nil, target, "vict")
        ch:act("$n focuses ki and aims a pulsing light at $N's body. $N smiles.", true, nil, target, "notvict")
    end
    target:condition_remove("poison", "skill_cure")
end

local function do_poison(ch, name)
    if not ch:know_skill("poison") then return end
    local vict = room_target(ch, name)
    if not vict then ch:send_line("Poison who?"); return end
    if not ch:can_kill(vict, 0) then return end
    if vict:is_same(ch) then ch:send_line("Why poison yourself?"); return end
    if vict:is_npc() and vict:mob_flagged(MOB.NOPOISON) then ch:send_line("You get the feeling that this being is immune to poison."); return end
    if vict:condition_has("poison") then ch:send_line("They already have been poisoned!"); return end
    if not ki_check(ch) then ch:send_line("You do not have enough ki to poison."); return end
    if not skill_success(ch, "poison") then
        drain_reveal(ch)
        ch:act("You focus ki and fling poison at $N. You missed!", true, nil, vict, "char")
        ch:act("$n focuses ki and flings poison at you, but misses!", true, nil, vict, "vict")
        ch:act("$n focuses ki and flings poison at $N, but misses!", true, nil, vict, "notvict")
        return
    end
    drain_reveal(ch)
    ch:act("You focus ki and fling poison at $N! The poison burns into $s skin!", true, nil, vict, "char")
    ch:act("$n focuses ki and flings poison at you! The poison burns into your skin!", true, nil, vict, "vict")
    ch:act("$n focuses ki and flings poison at $N! The poison burns into $s skin!", true, nil, vict, "notvict")
    if vict:is_npc() then vict:start_fighting(ch) end
    if vict:race_get() == "mutant" and (vict:genome_get(0) == 7 or vict:genome_get(1) == 7) then
        ch:act("However $N seems unaffected by the poison.", true, nil, vict, "char")
        ch:act("Your natural immunity to poison prevents it from affecting you.", true, nil, vict, "vict")
        ch:act("However $N seems unaffected by the poison.", true, nil, vict, "notvict")
        return
    end
    if vict:charge_get() > 0 then
        vict:send_line("You lose your concentration and release your charged ki!")
        vict:release_charge()
    end
    add_timed(vict, "poison", "affect", "poison", math.floor(ch:stat_get("intelligence") / 20))
    vict:condition_number_set("poison", "poison_by", ch:id_get())
end

local function execute(ctx)
    local ch = ctx.ch
    local arg = string.lower(ctx.argparams.tokens[1] or "")
    local name = ctx.argparams.tokens[2] or ""

    if blocked_by_position(ch) then return end
    if arg == "" then ch:send_line("Yes but what do you want to focus?"); return end
    if not ch:is_npc() and ch:player_flagged(PLR.HEALT) then ch:send_line("You are inside a healing tank!"); return end
    if ch:condition_has("curse") then ch:send_line("You are cursed and can't focus!"); return end

    local buff = BUFFS[arg]
    if buff then
        if not ch:know_skill(buff.skill) then return end
        local vict = room_target(ch, name)
        if name ~= "" and not vict then ch:send_line(buff.missing); return end
        apply_buff(ch, vict, arg, buff)
        return
    end
    if arg == "wither" then do_wither(ch, name); return end
    if arg == "bless" then do_bless(ch, arg, name); return end
    if arg == "curse" then do_curse(ch, arg, name); return end
    if arg == "yoikominminken" or arg == "yoik" then do_yoik(ch, name); return end
    if arg == "vigor" then do_vigor(ch, name); return end
    if arg == "cure" then do_cure(ch, arg, name); return end
    if arg == "poison" then do_poison(ch, name); return end

    ch:send_line("What do you want to focus?")
end

return {
    id = "focus",
    aliases = { {"focus", 3} },
    execute = execute,
}
