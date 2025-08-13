//
// Created by basti on 10/24/2021.
//
#include <boost/algorithm/string.hpp>

#include "dbat/structs.h"
#include "dbat/races.h"
#include "dbat/send.h"
#include "dbat/spells.h"
#include "dbat/comm.h"
#include "dbat/class.h"
#include "dbat/fight.h"
#include "dbat/act.movement.h"
#include "dbat/act.informative.h"
#include "dbat/config.h"
#include "dbat/mail.h"
#include "dbat/dg_comm.h"
#include "dbat/dg_scripts.h"
#include "dbat/interpreter.h"
#include "dbat/players.h"
#include "dbat/transformation.h"
#include "dbat/weather.h"
#include "dbat/attack.h"
#include "dbat/modifiers.h"

std::string Character::juggleRaceName(bool capitalized)
{

    std::string apparent = fmt::format("{}", magic_enum::enum_name(race));

    switch (race)
    {
    case Race::hoshijin:
        if (mimic)
            apparent = fmt::format("{}", magic_enum::enum_name(*mimic));
        break;
    case Race::halfbreed:
    case Race::android:
    case Race::saiyan:
        apparent = getAppearance(Appearance::seeming);
        break;
    }

    if (capitalized)
    {
        apparent[0] = std::toupper(apparent[0]);
    }
    return apparent;
}

void Character::restore_by(Character *ch)
{
    this->restore(true);

    ::act("You have been fully healed by $N!", false, this, nullptr, ch, TO_CHAR | TO_SLEEP);
}

void Character::restore(bool announce)
{
    restoreVitals(announce);
    restoreLimbs(announce);
    restoreStatus(announce);
    restoreVital(CharVital::ki);
}

void Character::lookAtLocation(const Location &loc)
{
    if (auto r = getRoom())
    {
        look_at_room(r, this, 0);
    }
}

void Character::lookAtLocation()
{
    if (!location)
        return;
    lookAtLocation(location);
}

void Character::resurrect(ResurrectionMode mode)
{
    // First, fully heal the character.
    restore(true);
    for (auto f : {AFF_ETHEREAL, AFF_SPIRIT})
        affect_flags.set(f, false);
    player_flags.set(PLR_PDEATH, false);
    // Send them to their starting room and have them 'look'.
    this->clearLocation();
    auto droom = GET_DROOM(this);
    if (droom != NOWHERE && droom != 0 && droom != 1)
    {
        setLocation(droom);
    }
    else
    {
        setLocation(sensei::getStartRoom(sensei));
    }
    lookAtLocation();

    // If Costless, there's not going to be any penalties.
    int dur = 100;
    switch (mode)
    {
    case Costless:
        return;
    case Basic:
    {
        auto dcount = GET_DCOUNT(this);
        if (dcount >= 8 && dcount < 10)
        {
            dur = 90;
        }
        else if (dcount >= 5 && dcount < 8)
        {
            dur = 75;
        }
        else if (dcount >= 3 && dcount < 5)
        {
            dur = 60;
        }
        else if (dcount >= 1 && dcount < 3)
        {
            dur = 40;
        }
    }
    break;
    case RPP:
        dur = 100;
        break;
    }

    // Also no penalties if the character isn't at least level 10.
    if (GET_LEVEL(this) > 9)
    {
        int losschance = axion_dice(0);
        this->sendText("@RThe the strain of this type of revival has caused you to be in a weakened state for 100 hours (Game time)! Strength,itution, wisdom, intelligence, speed, and agility have been reduced by 8 points for the duration.@n\r\n");
        std::map<std::string, int> statReductions = {
            {"strength", -8},
            {"constitution", -8},
            {"intelligence", -8},
            {"wisdom", -8},
            {"speed", -8},
            {"agility", -8}};

        for (auto &[attr, reduction] : statReductions)
        {
            int baseStat = this->getBaseStat(attr);
            int effectiveReduction = std::max(reduction, 15 - baseStat);
            statReductions[attr] = effectiveReduction;
        }

        assign_affect(this, AFF_WEAKENED_STATE, SKILL_WARP, dur,
                      statReductions["strength"],
                      statReductions["constitution"],
                      statReductions["intelligence"],
                      statReductions["agility"],
                      statReductions["wisdom"],
                      statReductions["speed"]);

        if (losschance >= 100)
        {
            int psloss = rand_number(100, 300);
            modPractices(-psloss);
            this->send_to("@R...and a loss of @r%d@R PS!@n", psloss);
        }
    }
    setBaseStat("death_time", 0);
    ::act("$n's body forms in a pool of @Bblue light@n.", true, this, nullptr, nullptr, TO_ROOM);
}

void Character::ghostify()
{
    restore(true);
    for (auto f : {AFF_SPIRIT, AFF_ETHEREAL, AFF_KNOCKED, AFF_SLEEP, AFF_PARALYZE})
        affect_flags.set(f, false);

    // upon death, ghost-bodies gain new natural limbs... unless they're a
    // cyborg and want to keep their implants.
    if (!PRF_FLAGGED(this, PRF_LKEEP))
    {
        for (auto f : {CharacterFlag::cyber_left_arm, CharacterFlag::cyber_right_arm, CharacterFlag::cyber_left_leg, CharacterFlag::cyber_right_leg})
            character_flags.set(f, false);
    }
}

void Character::teleport_to(room_vnum rnum)
{
    this->clearLocation();
    this->setLocation(rnum);
    lookAtLocation();
    update_pos(this);
}

bool Character::in_room_range(room_vnum low_rnum, room_vnum high_rnum)
{
    return getRoomVnum() >= low_rnum && getRoomVnum() <= high_rnum;
}

bool Character::in_past()
{
    return location.getWhereFlag(WhereFlag::pendulum_past);
}

bool Character::is_newbie()
{
    return GET_MAX_HIT(this) <= 10000;
}

bool Character::in_northran()
{
    return in_room_range(17900, 17999);
}

static std::map<int, uint16_t> grav_threshold = {
    {10, 5000},
    {20, 20000},
    {30, 50000},
    {40, 100000},
    {50, 200000},
    {100, 400000},
    {200, 1000000},
    {300, 5000000},
    {400, 8000000},
    {500, 15000000},
    {1000, 25000000},
    {5000, 100000000},
    {10000, 200000000}};

int64_t Character::calc_soft_cap()
{
    return 750000000;
    // auto level = getBaseStat<int>("Level");
    // if(level >= 100) return 5e9;
    // return race::getSoftCap(race, level);
}

bool Character::is_soft_cap(int64_t type)
{
    return is_soft_cap(type, 1.0);
}

bool Character::is_soft_cap(int64_t type, long double mult)
{
    if (IS_NPC(this))
        return true;

    return false;

    // Level 100 characters are never softcapped.
    if (getBaseStat<int>("level") >= 100)
    {
        return false;
    }
    auto cur_cap = calc_soft_cap() * mult;
    std::string stat;
    switch (type)
    {
    case 0:
        stat = "health";
        break;
    case 1:
        stat = "ki";
        break;
    case 2:
        stat = "stamina";
        break;
    default:
        basic_mud_log("Unknown stat type for soft cap check: %d", type);
        return false;
    }

    int64_t against = getBaseStat(stat);

    return against >= cur_cap;
}

int Character::wearing_android_canister()
{
    if (!IS_ANDROID(this))
        return 0;
    auto obj = GET_EQ(this, WEAR_BACKPACK);
    if (!obj)
        return 0;
    switch (GET_OBJ_VNUM(obj))
    {
    case 1806:
        return 1;
    case 1807:
        return 2;
    default:
        return 0;
    }
}

bool Character::hasGravAcclim(int grav)
{
    // 0 is x2, 1 is x5, 2 is x10, 3 is x50 and 4 is x100, 5 is x1000
    if (gravAcclim[grav] >= 10000)
        return true;
    return false;
}

void Character::raiseGravAcclim()
{
    if (rand_number(1, 140) >= getEffectiveStat("strength"))
    {
        auto gravity = location.getEnvironment(ENV_GRAVITY);

        if (gravity >= 1000 && !hasGravAcclim(5) && hasGravAcclim(4))
            gravAcclim[5] += 1;
        else if (gravity >= 100 && !hasGravAcclim(4) && hasGravAcclim(3))
            gravAcclim[4] += 1;
        else if (gravity >= 50 && !hasGravAcclim(3) && hasGravAcclim(2))
            gravAcclim[3] += 1;
        else if (gravity >= 10 && !hasGravAcclim(2) && hasGravAcclim(1))
            gravAcclim[2] += 1;
        else if (gravity >= 5 && !hasGravAcclim(1) && hasGravAcclim(0))
            gravAcclim[1] += 1;
        else if (gravity >= 2 && !hasGravAcclim(0))
            gravAcclim[0] += 1;
    }
}

int64_t Character::calcGravCost(int64_t num)
{
    double gravity = location.getEnvironment(ENV_GRAVITY);

    if (gravity >= 1000 && hasGravAcclim(5))
        gravity /= 1000;
    else if (gravity >= 100 && hasGravAcclim(4))
        gravity /= 100;
    else if (gravity >= 50 && hasGravAcclim(3))
        gravity /= 50;
    else if (gravity >= 10 && hasGravAcclim(2))
        gravity /= 10;
    else if (gravity >= 5 && hasGravAcclim(1))
        gravity /= 5;
    else if (gravity >= 2 && hasGravAcclim(0))
        gravity /= 2;

    int64_t cost = (gravity * gravity);

    if (!num)
    {
        if (cost)
        {
            this->sendText("You sweat bullets struggling against a mighty burden.\r\n");
        }
        if ((this->getCurVital(CharVital::stamina)) > cost)
        {
            this->modCurVital(CharVital::stamina, -cost);
            return 1;
        }
        else
        {
            this->modCurVital(CharVital::stamina, -cost);
            return 0;
        }
    }
    else
    {
        return (this->getCurVital(CharVital::stamina)) > (cost + num);
    }
}

bool Character::isFullVital(CharVital type)
{
    return getBaseStat(fmt::format("{}_damage", magic_enum::enum_name(type))) <= 0.0;
}

double Character::modCurVitalDam(CharVital type, double dam)
{
    return modBaseStat(fmt::format("{}_damage", magic_enum::enum_name(type)), dam);
}

double Character::setCurVitalDam(CharVital type, double dam)
{
    return setBaseStat(fmt::format("{}_damage", magic_enum::enum_name(type)), dam);
}

double Character::getCurVitalDam(CharVital type)
{
    return getBaseStat(fmt::format("{}_damage", magic_enum::enum_name(type)));
}

double Character::getCurVitalMeterPercent(CharVital type)
{
    auto dmg = getCurVitalDam(type);
    return 1.0 - dmg;
}

int64_t Character::getCurVital(CharVital type)
{
    auto effective_stat = getMaxVital(type);
    auto dmg = getCurVitalMeterPercent(type);
    return static_cast<int64_t>(effective_stat * dmg);
}

int64_t Character::getMaxVital(CharVital type)
{
    auto effective_stat = getEffectiveStat(std::string(magic_enum::enum_name(type)));
    return effective_stat;
}

int64_t Character::setCurVital(CharVital type, int64_t amt)
{
    auto m = getMaxVital(type);
    auto ratio = static_cast<double>(amt) / static_cast<double>(m);
    setBaseStat(fmt::format("{}_damage", magic_enum::enum_name(type)), -ratio);
    return getCurVital(type);
}

int64_t Character::modCurVital(CharVital type, int64_t amt)
{
    auto m = getMaxVital(type);
    auto ratio = static_cast<double>(amt) / static_cast<double>(m);
    modBaseStat(fmt::format("{}_damage", magic_enum::enum_name(type)), -ratio);
    return getCurVital(type);
}

void Character::restoreHealth(bool announce)
{
    setCurVitalDam(CharVital::health, 0.0);
}

int64_t Character::getCurVitalPercent(CharVital type, double amt)
{
    auto cur_vital = getEffectiveStat(std::string(magic_enum::enum_name(type)));
    auto dmg = getCurVitalMeterPercent(type);
    return (cur_vital * dmg) * amt;
}

int64_t Character::getMaxVitalPercent(CharVital type, double amt)
{
    auto max_vital = getEffectiveStat(std::string(magic_enum::enum_name(type)));
    return static_cast<int64_t>(max_vital * amt);
}

void Character::restoreVital(CharVital type)
{
    setCurVitalDam(type, 0.0);
}

bool Character::isFullVitals()
{
    for (const auto &t : {CharVital::health, CharVital::ki, CharVital::stamina})
    {
        if (!isFullVital(t))
            return false;
    }
    return true;
}

void Character::restoreVitals(bool announce)
{
    for (const auto &t : {CharVital::health, CharVital::ki, CharVital::stamina})
    {
        setCurVitalDam(t, 0.0);
    }
}

void Character::restoreStatus(bool announce)
{
    cureStatusKnockedOut(announce);
    cureStatusBurn(announce);
    cureStatusPoison(announce);
}

void Character::setStatusKnockedOut()
{
    affect_flags.set(AFF_KNOCKED, true);
    affect_flags.set(AFF_FLYING, false);
    setBaseStat("altitude", 0);
    this->setBaseStat<int>("position", POS_SLEEPING);
}

void Character::cureStatusKnockedOut(bool announce)
{
    if (AFF_FLAGGED(this, AFF_KNOCKED))
    {
        if (announce)
        {
            ::act("@W$n@W is no longer senseless, and wakes up.@n", false, this, nullptr, nullptr, TO_ROOM);
            this->sendText("You are no longer knocked out, and wake up!@n\r\n");
        }

        if (CARRIED_BY(this))
        {
            if (GET_ALIGNMENT(CARRIED_BY(this)) > 50)
            {
                carry_drop(CARRIED_BY(this), 0);
            }
            else
            {
                carry_drop(CARRIED_BY(this), 1);
            }
        }

        affect_flags.set(AFF_KNOCKED, false);
        this->setBaseStat<int>("position", POS_SITTING);
    }
}

void Character::cureStatusBurn(bool announce)
{
    if (AFF_FLAGGED(this, AFF_BURNED))
    {
        if (announce)
        {
            this->sendText("Your burns are healed now.\r\n");
            ::act("$n@w's burns are now healed.@n", true, this, nullptr, nullptr, TO_ROOM);
        }
        affect_flags.set(AFF_BURNED, false);
    }
}

void Character::cureStatusPoison(bool announce)
{
    ::act("@C$n@W suddenly looks a lot better!@b", false, this, nullptr, nullptr, TO_NOTVICT);
    affect_from_char(this, SPELL_POISON);
}

static std::map<int, std::string> limb_names = {
    {0, "right arm"},
    {1, "left arm"},
    {2, "right leg"},
    {3, "left leg"}};

void Character::restoreLimbs(bool announce)
{
    // restore head...
    GET_LIMBCOND(this, 0) = 100;

    // limbs...
    for (const auto &l : limb_names)
    {
        if (announce)
        {
            if (GET_LIMBCOND(this, l.first) <= 0)
                this->send_to("Your %s grows back!\r\n", l.second.c_str());
            else if (GET_LIMBCOND(this, l.first) < 50)
                this->send_to("Your %s is no longer broken!\r\n", l.second.c_str());
        }
        GET_LIMBCOND(this, l.first) = 100;
    }

    // and lastly, tail.
    this->gainTail(announce);
}

void Character::gainTail(bool announce)
{
    if (!race::hasTail(race))
        return;
    if (character_flags.get(CharacterFlag::tail))
        return;
    character_flags.set(CharacterFlag::tail, true);
    if (announce)
    {
        this->sendText("@wYour tail grows back.@n\r\n");
        act("$n@w's tail grows back.@n", true, this, nullptr, nullptr, TO_ROOM);
    }
}

void Character::loseTail()
{
    if (!character_flags.get(CharacterFlag::tail))
        return;
    character_flags.set(CharacterFlag::tail, false);
    remove_limb(this, 6);
    this->setBaseStat<int>("tail_growth", 0);
    oozaru_revert(this);
}

bool Character::hasTail()
{
    return character_flags.get(CharacterFlag::tail);
}

void Character::addTransform(Form form)
{
    transforms.insert({form, trans_data()});
}

void Character::hideTransform(Form form, bool hide)
{
    auto foundForm = transforms.find(form);
    foundForm->second.visible = !hide;
}

bool Character::removeTransform(Form form)
{
    if (transforms.contains(form))
    {
        transforms.erase(form);
        return true;
    }
    // Return if failure
    return false;
}

void Character::attemptLimitBreak()
{
    if (form == Form::base)
        return;
    if (transforms[form].time_spent_in_form > 50000 && rand_number(0, 1000) == 1000)
    {
        transforms[form].limit_broken = true;
        modCurVitalDam(CharVital::health, -0.35);
        modCurVitalDam(CharVital::ki, -0.35);
        modCurVitalDam(CharVital::stamina, -0.35);
        this->sendText("@mA rush of energy bursts through your system as you defy your limits.@n\r\n");

        affect_flags.set(AFF_LIMIT_BREAKING, true);
    }
}

void Character::removeLimitBreak()
{
    if (AFF_FLAGGED(this, AFF_LIMIT_BREAKING))
    {
        this->affect_flags.set(AFF_LIMIT_BREAKING, false);
        this->sendText("@mYou feel your body finally calm down.@n\r\n");
    }
}

int64_t Character::getPL(bool suppressed)
{
    int64_t vitalCalc = (getEffectiveStat<int64_t>("health") + getEffectiveStat<int64_t>("ki")) / 4;
    int attrCalc = (getEffectiveStat("agility") + getEffectiveStat("constitution") + getEffectiveStat("intelligence") + getEffectiveStat("speed") + getEffectiveStat("strength") + getEffectiveStat("wisdom")) / 50;

    auto sup = suppressed ? getBaseStat<int>("suppression") : 100.0;

    double speed = getEffectiveStat("speednar");

    double pl = (vitalCalc * attrCalc * speed) * (sup / 100.0);

    if (IS_NPC(this))
    {
        if (GET_LEVEL(this) < 10)
            pl *= 2;
        else if (GET_LEVEL(this) < 30)
            pl /= 1;
        else if (GET_LEVEL(this) < 50)
            pl /= 4;
        else if (GET_LEVEL(this) < 70)
            pl /= 6;
        else if (GET_LEVEL(this) < 90)
            pl /= 8;
        else
            pl /= 10;
    }

    return pl;
}

void Character::apply_kaioken(int times, bool announce)
{
    setBaseStat("kaioken", times);
    character_flags.set(CharacterFlag::powering_up, false);

    if (announce)
    {
        this->send_to("@rA dark red aura bursts up around your body as you achieve Kaioken x %d!@n\r\n", times);
        ::act("@rA dark red aura bursts up around @R$n@r as they achieve a level of Kaioken!@n", true, this, nullptr,
              nullptr, TO_ROOM);
    }
}

void Character::remove_kaioken(int8_t announce)
{
    auto kaio = getBaseStat<int>("kaioken");
    if (!kaio)
    {
        return;
    }
    setBaseStat("kaioken", 0);

    switch (announce)
    {
    case 1:
        this->sendText("You drop out of kaioken.\r\n");
        ::act("$n@w drops out of kaioken.@n", true, this, nullptr, nullptr, TO_ROOM);
        break;
    case 2:
        this->sendText("You lose focus and your kaioken disappears.\r\n");
        ::act("$n loses focus and $s kaioken aura disappears.", true, this, nullptr, nullptr, TO_ROOM);
    }
}

int Character::getRPP()
{
    if (IS_NPC(this))
    {
        return 0;
    }

    auto &p = players.at(id);

    return p.account->rpp;
}

void Account::modRPP(int amt)
{
    rpp += amt;
    if (rpp < 0)
    {
        rpp = 0;
    }
}

void Character::modRPP(int amt)
{
    if (IS_NPC(this))
    {
        return;
    }

    auto &p = players.at(id);

    p.account->modRPP(amt);
}

int Character::getPractices()
{
    return getBaseStat<int>("practices");
}

void Character::modPractices(int amt)
{
    modBaseStat("practices", amt);
}

void Character::login()
{
    enter_player_game(desc);
    this->send_to("%s", CONFIG_WELC_MESSG);
    ::act("$n has entered the game.", true, this, nullptr, nullptr, TO_ROOM);
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(this)), true, "%s has entered the game.", GET_NAME(this));
    /*~~~ For PCOUNT and HIGHPCOUNT ~~~*/
    auto count = 0;
    auto oldcount = HIGHPCOUNT;
    struct descriptor_data *k;

    for (k = descriptor_list; k; k = k->next)
    {
        if (!IS_NPC(k->character) && GET_MAX_HIT(k->character) > 5000)
        {
            count += 1;
        }

        if (count > PCOUNT)
        {
            PCOUNT = count;
        }

        if (PCOUNT >= HIGHPCOUNT)
        {
            oldcount = HIGHPCOUNT;
            HIGHPCOUNT = PCOUNT;
            PCOUNTDATE = ::time(nullptr);
        }
    }

    time.logon = ::time(nullptr);
    greet_mtrigger(this, -1);
    greet_memory_mtrigger(this);

    STATE(desc) = CON_PLAYING;
    if (PCOUNT < HIGHPCOUNT && PCOUNT >= HIGHPCOUNT - 4)
    {
        payout(0);
    }
    if (PCOUNT == HIGHPCOUNT)
    {
        payout(1);
    }
    if (PCOUNT > oldcount)
    {
        payout(2);
    }

    /*~~~ End PCOUNT and HIGHPCOUNT ~~~*/
    if (GET_LEVEL(this) == 0)
    {
        this->send_to("%s", CONFIG_START_MESSG);
    }
    if (this->location.getVnum() <= 1 && GET_LOADROOM(this) != NOWHERE)
    {
        this->clearLocation();
        this->setLocation(GET_LOADROOM(this));
    }
    else if (this->location.getVnum() <= 1)
    {
        this->clearLocation();
        this->setLocation(300);
    }
    else
    {
        this->lookAtLocation();
    }
    if (has_mail(GET_IDNUM(this)))
        this->sendText("\r\nYou have mail waiting.\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWIMM > GET_BOARD(this, 1))
        this->sendText("\r\n@GMake sure to check the immortal board, there is a new post there.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWCOD > GET_BOARD(this, 2))
        this->sendText("\r\n@GMake sure to check the request file, it has been updated.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWBUI > GET_BOARD(this, 4))
        this->sendText("\r\n@GMake sure to check the builder board, there is a new post there.@n\r\n");
    if (GET_ADMLEVEL(this) >= 1 && BOARDNEWDUO > GET_BOARD(this, 3))
        this->sendText("\r\n@GMake sure to check punishment board, there is a new post there.@n\r\n");
    if (BOARDNEWMORT > GET_BOARD(this, 0))
        this->sendText("\r\n@GThere is a new bulletin board post.@n\r\n");
    if (NEWSUPDATE > GET_LPLAY(this))
        this->send_to("\r\n@GThe NEWS file has been updated, type 'news %d' to see the latest entry or 'news list' to see available entries.@n\r\n", LASTNEWS);

    if (LASTINTEREST != 0 && LASTINTEREST > GET_LINTEREST(this))
    {
        int diff = (LASTINTEREST - GET_LINTEREST(this));
        int mult = 0;
        while (diff > 0)
        {
            if ((diff - 86400) < 0 && mult == 0)
            {
                mult = 1;
            }
            else if ((diff - 86400) >= 0)
            {
                diff -= 86400;
                mult++;
            }
            else
            {
                diff = 0;
            }
        }
        if (mult > 3)
        {
            mult = 3;
        }
        setBaseStat("last_interest", LASTINTEREST);
        if (GET_BANK_GOLD(this) > 0)
        {
            int inc = ((GET_BANK_GOLD(this) / 100) * 2);
            if (inc >= 7500)
            {
                inc = 7500;
            }
            inc *= mult;
            setBaseStat("money_bank", inc);
            this->send_to("Interest happened while you were away, %d times.\r\n"
                          "@cBank Interest@D: @Y%s@n\r\n",
                          mult, add_commas(inc).c_str());
        }
    }

    if (!IS_ANDROID(this))
    {
        char buf3[MAX_INPUT_LENGTH];
        send_to_sense(0, "You sense someone appear suddenly", this);
        sprintf(buf3,
                "@D[@GBlip@D]@Y %s\r\n@RSomeone has suddenly entered your scouter detection range!@n.",
                add_commas(GET_HIT(this)).c_str());
        send_to_scouter(buf3, this, 0, 0);
    }

    desc->has_prompt = 0;
    /* We've updated to 3.1 - some bits might be set wrongly: */
    pref_flags.set(PRF_BUILDWALK, false);
    if (!GET_EQ(this, WEAR_WIELD1) && PLR_FLAGGED(this, PLR_THANDW))
    {
        player_flags.set(PLR_THANDW, false);
    }
}

double Character::getAffectModifier(uint64_t location, uint64_t specific)
{
    double total = 0;
    // Personal modifiers.
    for (auto a = affected; a; a = a->next)
    {
        if (!a->match(location, specific))
            continue;
        total += a->modifier;
    }

    // Equipment modifiers.
    for (auto i = 0; i < NUM_WEARS; i++)
    {
        if (auto obj = GET_EQ(this, i); obj)
            total += obj->getAffectModifier(location, specific);
    }

    total += race::getModifier(this, location, specific);
    total += trans::getModifier(this, location, specific);

    // Position modifier.
    if (auto find = pos_affects.find(getBaseStat<int>("position")); find != pos_affects.end())
    {
        for (auto &eff : find->second)
        {
            if (!eff.match(location, specific))
                continue;
            total += eff.modifier;
            if (eff.func)
                total += eff.func(this);
        }
    }

    return total;
}

int Character::setSize(int val)
{
    this->size = static_cast<Size>(val);
    return static_cast<int>(this->size);
}

int Character::getSize()
{
    return static_cast<int>(size) != SIZE_UNDEFINED ? static_cast<int>(size) : race::getSize(race);
}

double getDaysPassed()
{
    double ingameDays = era_uptime.day + (era_uptime.month * 30) + (era_uptime.year * 365);
    return ingameDays;
}

double Character::getPotential()
{
    // Gain one potential per RL week, reaches 100 in two years
    double timePotential = 1 + (getDaysPassed() / 7);

    int physiquePotential = 1;
    if (hasGravAcclim(0))
        physiquePotential += 1;
    if (hasGravAcclim(1))
        physiquePotential += 1;
    if (hasGravAcclim(2))
        physiquePotential += 1;
    if (hasGravAcclim(3))
        physiquePotential += 1;
    if (hasGravAcclim(4))
        physiquePotential += 1;
    if (hasGravAcclim(5))
        physiquePotential += 1;
    return timePotential * physiquePotential;
}

void Character::gainGrowth()
{
    double modifier = 1;
    if (location.getWhereFlag(WhereFlag::afterlife_hell) || location.getWhereFlag(WhereFlag::afterlife))
    {
        modifier = 1.5;
    }

    // Set with the idea its triggered once every 5 mins. Calculates to 0.5 per day. (288 '5 mins' in a day. Double to half the gain)
    double gain = modifier / (288.0 * 2);
    gainGrowth(gain);
}

void Character::gainGrowth(double gain)
{
    // You cannot exceed the amount of days the server has been online for
    double days = getDaysPassed();

    // Roughly increase by a multiplier of 1 every 60 days past the first 60
    double timeMod = std::max((days / 60.0), 1.0);

    gain *= timeMod;

    auto ltg = getBaseStat("lifetimeGrowth");
    if (ltg + gain < days)
    {
        // If you have stored growth, we double the amount gained and reduce overgrowth by that amount
        if (getBaseStat("overGrowth") >= gain && ltg + (gain * 2) < days)
        {
            modBaseStat("overGrowth", -gain);
            gain *= 2;
        }
        for (const auto &s : {"internalGrowth", "lifetimeGrowth"})
            modBaseStat(s, gain);
    }
    else
    {
        // Any overflow is stored in overGrowth
        double cangain = days - ltg;
        for (const auto &s : {"internalGrowth", "lifetimeGrowth"})
            modBaseStat(s, cangain);

        cangain = gain - cangain;
        modBaseStat("overGrowth", cangain);
    }
}

bool Character::canCarryWeight(weight_t val)
{
    double gravity = location.getEnvironment(ENV_GRAVITY);

    if (gravity >= 1000 && hasGravAcclim(5))
        gravity /= 1000;
    else if (gravity >= 100 && hasGravAcclim(4))
        gravity /= 100;
    else if (gravity >= 50 && hasGravAcclim(3))
        gravity /= 50;
    else if (gravity >= 10 && hasGravAcclim(2))
        gravity /= 10;
    else if (gravity >= 5 && hasGravAcclim(1))
        gravity /= 5;
    else if (gravity >= 2 && hasGravAcclim(0))
        gravity /= 2;

    return getEffectiveStat("carry_available") >= (val * gravity);
}

bool Character::canCarryWeight(Object *obj)
{
    return canCarryWeight(obj->getEffectiveStat("weight_total"));
}

bool Character::canCarryWeight(Character *obj)
{
    return canCarryWeight(obj->getEffectiveStat("weight_total"));
}

room_vnum Character::normalizeLoadRoom(room_vnum in)
{
    // If they were in the void, then we need to use their last good room.
    room_vnum room = NOWHERE;
    room_vnum lroom = NOWHERE;
    // Handle the void issue...
    if (in == 0 || in == 1)
    {
        room = GET_WAS_IN(this);
    }
    else
    {
        room = in;
    }

    // Personal Pocket Dimensions
    // if (room >= 19800 && room <= 19899) {
    // lroom = room;
    //}
    // those stuck in the pendulum room past get returned to Kami's Lookout.
    if (WHERE_FLAGGED(room, WhereFlag::pendulum_past))
    {
        lroom = 1561;
    }
    // the WMAT arena also is not a good place to log off.
    else if (room >= 2002 && room <= 2011)
    {
        lroom = 1960;
    }
    // The two Minecarts are possible trap zones.
    else if (room == 2069)
    {
        lroom = 2017;
    }
    else if (room == 2070)
    {
        lroom = 2046;
    }
    // The higher plane is a problem...
    else if (room == 6030)
    {
        // Stick them on the side of King Yemma's, in case they're broke/weak.
        lroom = 6029;
    }
    // This is the MUD School. If they're not done then put them
    // back at the start. Otherwise, send them to their Sensei.
    else if (room >= 101 && room <= 139)
    {
        if (GET_LEVEL(this) == 1)
        {
            lroom = 100;
            setExperience(0);
        }
        else
        {
            lroom = sensei::getStartRoom(sensei);
        }
    }
    else
    {
        // looks like room might be okay.
        lroom = room;
    }

    // if lroom is valid, save it... else... emergency fallback to mud school.
    if (auto r = get_room(lroom); r)
        return lroom;
    return CONFIG_MORTAL_START;
}

void Character::onAttack(atk::Attack &outgoing)
{
    if (form != Form::base)
        trans::onAttack(this, outgoing, form);
    if (technique != Form::base)
        trans::onAttack(this, outgoing, technique);
}

void Character::onAttacked(atk::Attack &incoming)
{
    if (form != Form::base)
        trans::onAttacked(this, incoming, form);
    if (technique != Form::base)
        trans::onAttacked(this, incoming, technique);
}

int64_t Character::getExperience()
{
    return getEffectiveStat("experience");
}

int64_t Character::setExperience(int64_t value)
{
    return setBaseStat("experience", value);
}

// This returns the exact amount that was modified by.
int64_t Character::modExperience(int64_t value, bool applyBonuses)
{

    if (value < 0)
    {
        // removing experience. We can do this easily.
        auto cur = getExperience();
        auto new_value = setExperience(cur + value);
        // return the actual amount substracted as a negative.
        return cur - new_value;
    }

    // Adding experience may involve bonuses.
    auto gain = value;
    auto cur = getExperience();

    if (!applyBonuses)
    {
        gain *= (1.0 + getAffectModifier(APPLY_CSTAT_GAIN_MULT, static_cast<int>(CharStat::experience)));

        if (AFF_FLAGGED(this, AFF_WUNJO))
        {
            gain *= 1.15;
        }
        if (PLR_FLAGGED(this, PLR_IMMORTAL))
        {
            gain *= 0.95;
        }

        int64_t diff = gain * 0.15;

        if (gain > 0)
        {

            // TODO: Modify the spar booster with APPLY_EXP_GAIN_MULT 0.25
            if (auto obj = GET_EQ(this, WEAR_SH); obj && obj->getVnum() == 1127)
            {
                int64_t spar = gain;
                gain += gain * 0.25;
                spar = gain - spar;
                this->send_to("@D[@BBooster EXP@W: @G+%s@D]\r\n", add_commas(spar).c_str());
            }

            // Post-100 gains.
            if (GET_LEVEL(this) == 100 && GET_ADMLEVEL(this) < 1)
            {
                if (IS_KANASSAN(this) || IS_DEMON(this))
                {
                    diff = diff * 1.3;
                }
                if (IS_ANDROID(this))
                {
                    diff = diff * 1.2;
                }

                if (rand_number(1, 5) >= 2)
                {
                    if (IS_HUMAN(this))
                    {
                        this->gainBaseStat("health", diff * 0.8);
                    }
                    else
                    {
                        this->gainBaseStat("health", diff);
                    }
                    this->send_to("@D[@G+@Y%s @RPL@D]@n ", add_commas(diff).c_str());
                }
                if (rand_number(1, 5) >= 2)
                {
                    if (IS_HALFBREED(this))
                    {
                        this->gainBaseStat("stamina", diff * 0.85);
                    }
                    else
                    {
                        this->gainBaseStat("stamina", diff);
                    }
                    this->send_to("@D[@G+@Y%s @gSTA@D]@n ", add_commas(diff).c_str());
                }
                if (rand_number(1, 5) >= 2)
                {
                    this->gainBaseStat("ki", diff);
                    this->send_to("@D[@G+@Y%s @CKi@D]@n", add_commas(diff).c_str());
                }
            }
        }
    }

    // Amount gained cannot be negative.
    gain = std::max<int64_t>(gain, 0);

    if (MINDLINK(this) && gain > 0 && LINKER(this) == 0)
    {
        if (GET_INT(this) + 20 < GET_INT(MINDLINK(this)) || GET_INT(this) - 20 > GET_INT(MINDLINK(this)))
        {
            MINDLINK(this)->sendText("The intelligence difference between the two of you is too great to gain from mind read.\r\n");
        }
        else
        {
            act("@GYou've absorbed some new experiences from @W$n@G!@n", false, this, nullptr, MINDLINK(this),
                TO_VICT);
            int64_t read = gain * 0.12;
            gain -= read;
            if (read == 0)
                read = 1;
            MINDLINK(this)->modExperience(read, false);
            act("@RYou sense that @W$N@R has stolen some of your experiences with $S mind!@n", false, this,
                nullptr, MINDLINK(this), TO_CHAR);
        }
    }

    if (GET_LEVEL(this) < 100)
    {
        int64_t tnl = level_exp(this, GET_LEVEL(this) + 1);

        if (cur < tnl && (cur + gain) >= tnl)
        {
            this->sendText("@rYou have earned enough experience to gain a @ylevel@r.@n\r\n");
        }

        int64_t max_over_tnl = tnl * 5;
        if ((cur + gain) >= max_over_tnl)
        {
            gain = max_over_tnl - getExperience();
            this->sendText("@WYou -@RNEED@W- to @ylevel@W. You can't hold any more experience!@n\r\n");
        }
    }

    if (gain)
        setExperience(cur + gain);
    return gain;
}

void Character::gazeAtMoon()
{
    if (OOZARU_RACE(this) && character_flags.get(CharacterFlag::tail))
    {
        if (form == Form::oozaru || form == Form::golden_oozaru)
            return;
        Form toForm = Form::oozaru;
        if (transforms.contains(Form::super_saiyan_1) || transforms.contains(Form::super_saiyan_2) || transforms.contains(Form::super_saiyan_3) || transforms.contains(Form::super_saiyan_4))
            toForm = Form::golden_oozaru;

        form = toForm;
    }
    else if (transforms.contains(Form::lycanthrope))
    {
        if (transforms.contains(Form::alpha_lycanthrope))
        {
            form = Form::alpha_lycanthrope;
        }
        else
        {
            form = Form::lycanthrope;
        }
    }
    trans::handleEchoTransform(this, form);
}

static const std::map<std::string, std::string> _attr_names = {
    {"str", "strength"},
    {"wis", "wisdom"},
    {"con", "constitution"},
    {"cha", "speed"},
    {"spd", "speed"},
    {"dex", "agility"},
    {"agi", "agility"},
    {"int", "intelligence"},

    {"strength", "strength"},
    {"wisdom", "wisdom"},
    {"constitution", "constitution"},
    {"speed", "speed"},
    {"agility", "agility"},
    {"intelligence", "intelligence"}};

static const std::map<std::string, std::string> _money_names = {
    {"bank", "money_bank"},
    {"gold", "money_carried"},
    {"zenni", "money_carried"}};

static const std::map<std::string, int> _cond_names = {
    {"hunger", HUNGER},
    {"thirst", THIRST},
    {"drunk", DRUNK}};

static const std::map<std::string, int> _save_names = {
    {"saving_fortitude", SAVING_FORTITUDE},
    {"saving_reflex", SAVING_REFLEX},
    {"saving_will", SAVING_WILL}};

static const std::map<std::string, int> _pflags = {
    {"is_killer", PLR_KILLER},
    {"is_thief", PLR_THIEF}};

static const std::map<std::string, int> _aflags = {
    {"dead", AFF_SPIRIT},
    {"flying", AFF_FLYING}};

std::optional<std::string> Character::dgCallMember(const std::string &member, const std::string &arg)
{
    std::string lmember = member;
    boost::to_lower(lmember);
    boost::trim(lmember);

    if (auto attr = _attr_names.find(lmember); attr != _attr_names.end())
    {
        if (!arg.empty())
        {
            attribute_t addition = atof(arg.c_str());
            modBaseStat(attr->second, addition);
        }
        return fmt::format("{}", getBaseStat(attr->second));
    }

    if (auto mon = _money_names.find(lmember); mon != _money_names.end())
    {
        if (!arg.empty())
        {
            money_t addition = atoll(arg.c_str());
            modBaseStat(mon->second, addition);
        }
        return fmt::format("{}", (money_t)getBaseStat(mon->second));
    }

    if (auto con = _cond_names.find(lmember); con != _cond_names.end())
    {
        if (!arg.empty())
        {
            int addition = atof(arg.c_str());
            GET_COND(this, con->second) = std::clamp<int>(addition, -1, 24);
        }
        return fmt::format("{}", GET_COND(this, con->second));
    }

    if (auto save = _save_names.find(lmember); save != _save_names.end())
    {
        return fmt::format("{}", 0);
    }

    if (auto pf = _pflags.find(lmember); pf != _pflags.end())
    {
        if (!arg.empty())
        {
            if (!strcasecmp("on", arg.c_str()))
                player_flags.set(pf->second, true);
            else if (!strcasecmp("off", arg.c_str()))
                player_flags.set(pf->second, false);
        }
        return player_flags.get(pf->second) ? "1" : "0";
    }

    if (auto af = _aflags.find(lmember); af != _aflags.end())
    {
        return AFF_FLAGGED(this, af->second) ? "1" : "0";
    }

    return {};
}

void Character::setTask(Task t)
{
    task = t;
    if (task == Task::nothing)
    {
        if (wait_input_queue.empty())
            characterSubscriptions.unsubscribe("commandWaitQueue", this);
    }
    else
    {
        characterSubscriptions.subscribe("commandWaitQueue", this);
    }
}

std::string Character::getAppearance(Appearance type, bool withTransform)
{
    if (withTransform && form != Form::base)
    {
        if (auto override = trans::getAppearance(this, form, type); override)
        {
            return override.value();
        }
    }

    if (auto find = appearances.find(type); find != appearances.end())
    {
        return find->second;
    }
    return race::defaultAppearance(this, type);
}

const char *Character::getAppearanceStr(Appearance type)
{
    static char buf[MAX_STRING_LENGTH];
    snprintf(buf, MAX_STRING_LENGTH, "%s", getAppearance(type).c_str());
    return buf;
}

void Character::sendText(const std::string &txt)
{
    if (!desc)
        return;
    desc->sendText(txt);
}

bool Character::isSparring() const
{

    if (IS_NPC(this))
    {
        auto opponent = fighting;
        if (!opponent)
            return false;
        return IS_NPC(opponent) ? false : opponent->character_flags.get(CharacterFlag::sparring) && spar_friendly(opponent, (Character *)this);
    }

    return character_flags.get(CharacterFlag::sparring);
}

std::string Character::getUID(bool active) const
{
    return fmt::format("#C{}{}", id, active ? "!" : "");
}

void Character::onAddToEquip(const std::shared_ptr<Object> &obj, int slot)
{
    obj->container.reset();
    obj->carrier = shared_from_this();
    obj->worn_on = slot;
}

void Character::onRemoveFromEquip(const std::shared_ptr<Object> &obj, int slot)
{
    // Custom logic for when an object is removed from equipment
    obj->container.reset();
    obj->carrier.reset();
    obj->worn_on = -1;
}

void Character::onAddToInventory(const std::shared_ptr<Object> &obj)
{
    obj->container.reset();
    obj->carrier = shared();
    obj->worn_on = -1;
}

void Character::onRemoveFromInventory(const std::shared_ptr<Object> &obj)
{
    obj->container.reset();
    obj->carrier.reset();
    obj->worn_on = -1;
}

void Character::onAddToLocation(const Location &loc)
{
}

void Character::onRemoveFromLocation(const Location &loc)
{
}

void Character::onLocationChanged(const Location &oldloc, const Location &newloc)
{
}

void Character::addToLocation(const Location &loc)
{
    if (!loc.unit)
        return;
    loc.unit->addToContents(loc.position, shared_from_this());
}

void Character::removeFromLocation()
{
    if (!location.unit)
        return;
    location.unit->removeFromContents(shared_from_this());
}