#include "dbat/attack.h"
#include "dbat/combat.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/techniques.h"
#include "dbat/class.h"
#include "dbat/races.h"

namespace atk {
    std::map<int, std::vector<std::pair<int, int>>> minimumSkillRequired;

    std::map<int, std::vector<std::pair<int, int>>> trainSkillOnSuccess;

    std::map<int, std::vector<std::pair<int, int>>> trainSkillOnFailure;

    std::vector<int> comboSkills = {
            SKILL_PUNCH, SKILL_KICK, SKILL_ELBOW, SKILL_KNEE, SKILL_ROUNDHOUSE, SKILL_UPPERCUT,
            SKILL_SLAM, SKILL_HEELDROP, SKILL_BASH, SKILL_HEADBUTT, SKILL_TAILWHIP
    };

    Attack::Attack(struct char_data *ch, char *arg) : user(ch) {
        if(arg && strlen(arg)) input = arg;
        args = boost::split(args, input, boost::is_space(), boost::token_compress_on);
    }

    std::optional<int> Attack::limbsToCheck() {
        return std::nullopt;
    }

    std::optional<int> Attack::hurtInjuredLimbs() {
        return std::nullopt;
    }

    bool Attack::checkLimbs() {
        auto toCheck = limbsToCheck();
        if(!toCheck) {
            return true;
        }
        return limb_ok(user, *toCheck);
    }

    bool Attack::checkSkills() {
        auto needed = minimumSkillRequired.find(getSkillID());
        if(needed == minimumSkillRequired.end()) return true;

        for(auto &s : needed->second) {
            auto skillID = s.first;
            auto required = s.second;
            if(GET_SKILL(user, skillID) < required) {
                send_to_char(user, "You need %d in %s to use this attack.\r\n", required, spell_info[skillID]);
                return false;
            }
        }
        return true;
    }

    void Attack::doTrain(const std::vector<std::pair<int, int>> &skills) {
        for(auto &s : skills) {
            improve_skill(user, s.first, s.second);
        }
    }


    bool Attack::checkEmptyHands() {
        if(needEmptyHands()) {
            if (GET_EQ(user, WEAR_WIELD1) && GET_EQ(user, WEAR_WIELD2)) {
                send_to_char(user, "Your hands are full!\r\n");
                return false;
            }
        }
        return true;
    }

    int64_t Attack::calculateHealthCost() {
        return 0;
    }

    int64_t Attack::calculateStaminaCost() {
        return 0;
    }

    int64_t Attack::calculateKiCost() {
        return 0;
    }

    bool Attack::checkCosts() {
        currentHealthCost = calculateHealthCost();
        if(GET_HIT(user) < currentHealthCost) {
            send_to_char(user, "You don't have enough health to do that!\r\n");
            return false;
        }
        currentStaminaCost = calculateStaminaCost();
        if(user->getCurST() < currentStaminaCost) {
            send_to_char(user, "You don't have enough stamina to do that!\r\n");
            return false;
        }
        currentKiCost = calculateKiCost();
        if(user->getCurKI() < currentKiCost) {
            send_to_char(user, "You don't have enough ki to do that!\r\n");
            return false;
        }
        return true;
    }

    std::optional<int> Attack::hasCooldown() {
        return std::nullopt;
    }

    void Attack::execute() {

        if(!can_grav(user)) return;
        if(!checkSkills()) return;
        if(!checkLimbs()) return;
        if(!checkEmptyHands()) return;

        if (args.empty() && !FIGHTING(user)) {
            send_to_char(user, "Direct it at who?\r\n");
            return;
        }

        if(!checkCosts()) return;

        initSkill = init_skill(user, getSkillID());

        if(args.empty()) {
            victim = FIGHTING(user);
        } else {
            if(!tech_handle_targeting(user, (char*)args[0].c_str(), &victim, &obj)) return;
        }

        switch(doAttack()) {
            case Result::Landed:
                onLanded();
                onLandedOrMissed();
                break;
            case Result::Missed:
                onMissed();
                onLandedOrMissed();
                break;
            case Result::Canceled:
                onCanceled();
                break;
        }
    }

    Result Attack::doAttack() {

        if(victim) return attackCharacter();
        else if(obj) return attackObject();
        else {
            send_to_char(user, "You can't find that target.\r\n");
            return Result::Canceled;
        }

    }

    int Attack::canKillType() {
        return 0;
    }

    Result Attack::attackCharacter() {
        if(!can_kill(user, victim, nullptr, canKillType())) return Result::Canceled;
        if (handle_defender(victim, user)) {
            victim = GET_DEFENDER(victim);
            // should I do can_kill again here???
        }

        initStats();

        currentSpeedIndexCheck = check_def(victim);
        currentHitProbability = roll_accuracy(user, getSkillID(), isKiAttack());
        currentChanceToHit = chance_to_hit(user);

        if(isPhysical() && !usesWeapon()) {
            if (IS_KABITO(user) && !IS_NPC(user)) {
                if (GET_SKILL_BASE(user, SKILL_STYLE) >= 75)
                    currentChanceToHit -= currentChanceToHit * 0.2;
            }
        }

        currentSpeedIndexCheck = handle_speed(user, victim);

        auto avo = currentSpeedIndexCheck / 4;

        handle_defense(victim, &currentParryCheck, &currentBlockCheck, &currentDodgeCheck);

        currentHitProbability -= avo;

        tech_handle_posmodifier(victim, currentParryCheck, currentBlockCheck, currentDodgeCheck, currentHitProbability);

        if(canZanzoken()) {
            if (!tech_handle_zanzoken(user, victim, getName().c_str())) {
                COMBO(user) = -1;
                COMBHITS(user) = 0;
                currentStaminaCost /= 2;
                pcost(victim, 0, GET_MAX_HIT(victim) / 200);
                return Result::Missed;
            }
        }

        calcDamage = damtype(user, getAtkID(), initSkill, attPerc);

        if(currentHitProbability < currentChanceToHit - 20) {
            // a counter or miss of some kind...
            return handleOtherHit();
        } else {
            // it was a clean hit!
            defenseResult = DefenseResult::Failed;
            return handleCleanHit();
        }

    }

    Result Attack::handleCleanHit() {
        hitspot = roll_hitloc(user, victim, initSkill);
        announceHitspot();
        handleHitspot();
        /* dam_eq_loc: 1 Arms, 2 legs, 3 head, and 4 body. */
        int damLoc = 0;
        switch(hitspot) {
            case 1:
                damLoc = 4;
                break;
            case 2:
                damLoc = 3;
                break;
            case 3:
                damLoc = 4;
                break;
            case 4:
                damLoc = 1;
                break;
            case 5:
                damLoc = 2;
                break;
        }
        if(damLoc) dam_eq_loc(victim, damLoc);
        return Result::Landed;
    }

    void Attack::handleHitspot() {

    }

    void Attack::announceHitspot() {

    }

    Result Attack::handleOtherHit() {
        // Anything less than a clean hit costs half stamina.
        currentStaminaCost /= 2;

        if(victim->getCurST() > 0) {
            if(canParry() && calculateDeflect()) return handleParry();
            if(canDodge() && currentDodgeCheck > axion_dice(10)) return handleDodge();
            if(canBlock() && currentBlockCheck > axion_dice(10)) return handleBlock();
            return handleMiss();
        } else {
            // victim is unable to respond to our fumbled attack...
            return handleMiss();
        }
    }

    Result Attack::handleMiss() {
        announceMiss();
        return Result::Missed;
    }

    bool Attack::calculateDeflect() {
        return currentParryCheck > rand_number(1, 140) && (!IS_NPC(victim) || !MOB_FLAGGED(victim, MOB_DUMMY));
    }

    Result Attack::attackObject() {
        if(!can_kill(user, nullptr, obj, canKillType())) return Result::Canceled;
        calcDamage = calculateObjectDamage();
        announceObject();
        return Result::Landed;
    }

    void Attack::onLanded() {
        switch(defenseResult) {
            case DefenseResult::Blocked:
                calcDamage /= 4;
                break;
        }
        if(calcDamage < 1) calcDamage = 1;
        hurt(0, 0, user, victim, obj, calcDamage, isKiAttack());
        if(isPhysical()) tech_handle_fireshield(user, victim, getBodyPart().c_str());
        if(canCombo()) handle_multihit(user, victim);
    }

    void Attack::onMissed() {
        calcDamage = 0;
    }

    void Attack::onCanceled() {
        currentStaminaCost = 0;
        currentKiCost = 0;
        calcDamage = 0;
    }

    static std::map<int, std::string> bodyParts = {
            {0, "right arm"},
            {1, "left arm"},
            {2, "right leg"},
            {3, "left leg"}
    };

    void Attack::hurtLimb(int limb) {
        auto name = bodyParts[limb];
        send_to_char(user, fmt::format("Using your broken {} has damaged it more!@n\r\n", name).c_str());
        GET_LIMBCOND(user, limb) -= rand_number(3, 5);
        if (GET_LIMBCOND(user, limb) < 0) {
            act(fmt::format("@RYour {} has fallen apart!@n", name).c_str(), true, user, nullptr, nullptr, TO_CHAR);
            act(fmt::format("@r$n@R's {} has fallen apart!@n", name).c_str(), true, user, nullptr, nullptr, TO_ROOM);
        }
    }

    void Attack::onLandedOrMissed() {
        auto atrain = autoTrainSkillID();
        if(atrain != -1) {
            improve_skill(user, getSkillID(), atrain);
        }
        if(cooldownOverride != -1)  {
            WAIT_STATE(user, cooldownOverride);
        } else if(auto cool = hasCooldown(); cool) {
            handle_cooldown(user, *cool);
        }
        if(currentStaminaCost < 0) currentStaminaCost = 0;
        if(currentKiCost < 0) currentKiCost = 0;
        if(currentStaminaCost > 0 || currentKiCost > 0) {
            pcost(user, currentKiCost, currentStaminaCost);
        }
        
        if(auto hlimb = hurtInjuredLimbs(); hlimb) {

            auto checkLimb = [&](int l) {return GET_LIMBCOND(user, l) > 0 && GET_LIMBCOND(user, l) < 50;};

            auto tohurt = *hlimb;
            // type 0 is arms, type 1 is legs. 2 is both.
            if(tohurt == 0 || tohurt == 2) {
                std::vector<int> limbs;
                for(auto i : {0, 1}) if(checkLimb(i)) limbs.emplace_back(i);
                if(!limbs.empty()) {
                    auto limb = Random::get(limbs);
                    hurtLimb(*limb);
                }
            }
            
            if(tohurt == 1 || tohurt == 2) {
                std::vector<int> limbs;
                for(auto i : {2, 3}) if(checkLimb(i)) limbs.emplace_back(i);
                if(!limbs.empty()) {
                    auto limb = Random::get(limbs);
                    hurtLimb(*limb);
                }
            }
        }
        
    }

    void Attack::actUser(const std::string& msg, bool hideInvisible) {
        act(msg.c_str(), hideInvisible, user, obj, victim, TO_CHAR);
    }

    void Attack::actOthers(const std::string &msg, bool hideInvisible) {
        act(msg.c_str(), hideInvisible, user, obj, victim, TO_NOTVICT);
    }

    void Attack::actRoom(const std::string &msg, bool hideInvisible) {
        act(msg.c_str(), hideInvisible, user, obj, victim, TO_ROOM);
    }

    void Attack::actVictim(const std::string &msg, bool hideInvisible) {
        act(msg.c_str(), hideInvisible, user, obj, victim, TO_VICT);
    }

    bool Attack::canBlock() {
        return false;
    }

    bool Attack::canParry() {
        return false;
    }

    bool Attack::canDodge() {
        return false;
    }

    bool Attack::canZanzoken() {
        return true;
    }

    bool Attack::canCombo() {
        return false;
    }

    bool Attack::isPhysical() {
        return false;
    }

    bool Attack::isKiAttack() {
        return false;
    }

    bool Attack::usesWeapon() {
        return false;
    }

    int64_t Attack::calculateObjectDamage() {
        return damtype(user, getAtkID(), initSkill, attPerc);
    }

    void Attack::initStats() {
        currentParryCheck = 2;
        currentDodgeCheck = 2;
        currentBlockCheck = 2;
    }

    // MeleeAttack
    int64_t MeleeAttack::calculateObjectDamage() {
        return ((GET_HIT(user) / 10000) + (GET_STR(user)));
    }

    Result MeleeAttack::handleParry() {
        announceParry();
        defenseResult = DefenseResult::Parried;
        pcost(victim, 0, GET_MAX_HIT(victim) / 500);
        improve_skill(victim, SKILL_PARRY, 0);
        return Result::Missed;

    }

    Result MeleeAttack::handleDodge() {
        announceDodge();
        defenseResult = DefenseResult::Dodged;
        improve_skill(victim, SKILL_DODGE, 0);
        pcost(victim, 0, GET_MAX_HIT(victim) / 500);
        return Result::Missed;

    }

    Result MeleeAttack::handleBlock() {
        announceBlock();
        defenseResult = DefenseResult::Blocked;
        improve_skill(victim, SKILL_BLOCK, 0);
        pcost(victim, 0, GET_MAX_HIT(victim) / 500);
        return Result::Landed;

    }

    int64_t MeleeAttack::calculateStaminaCost() {
        return physical_cost(user, getSkillID());
    }

    // RangedAttack
    int RangedAttack::canKillType() {
        return 1;
    }


    // PUNCH
    void Punch::announceParry() {
        actUser("@C$N@W parries your punch with a punch of $S own!@n");
        actVictim("@WYou parry @C$n's@W punch with a punch of your own!@n");
        actOthers("@C$N@W parries @c$n's@W punch with a punch of $S own!@n");
    }

    void Punch::announceBlock() {
        actUser("@C$N@W moves quickly and blocks your punch!@n");
        actVictim("@WYou move quickly and block @C$n's@W punch!@n");
        actOthers("@C$N@W moves quickly and blocks @c$n's@W punch!@n");
    }

    void Punch::announceDodge() {
        actUser("@C$N@W manages to dodge your punch!@n");
        actVictim("@WYou dodge @C$n's@W punch!@n");
        actOthers("@C$N@W manages to dodge @c$n's@W punch!@n");
    }

    void Punch::announceMiss() {
        actUser("@WYou can't believe it but your punch misses!@n");
        actVictim("@C$n@W throws a punch at you but somehow misses!@n");
        actOthers("@c$n@W throws a punch at @C$N@W but somehow misses!@n");
    }

    void Punch::handleHitspot() {
        switch(hitspot) {
            case 1:
            case 3:
                if (GET_BONUS(user, BONUS_SOFT)) {
                    calcDamage *= calc_critical(user, 2);
                }
                break;
            case 2:
                calcDamage *= calc_critical(user, 0);
                break;
            case 4:
            case 5:
                calcDamage *= calc_critical(user, 1);
                break;
        }
    }

    void Punch::announceHitspot() {
        switch(hitspot) {
            case 1:
                actUser("@WYou slam your fist into @C$N's@W body!@n");
                actVictim("@C$n@W slams $s fist into your body!@n");
                actOthers("@c$n@W slams $s fist into @C$N's@W body!@n");
                break;
            case 2:
                actUser("@WYou slam your fist into @C$N's@W face!@n");
                actVictim("@C$n@W slams $s fist into your face!@n");
                actOthers("@c$n@W slams $s fist into @C$N's@W face!@n");
                break;
            case 3:
                actUser("@WYou punch @C$N@W directly in the gut!@n");
                actVictim("@C$n@W punches you directly in the gut!@n");
                actOthers("@c$n@W punches @C$N@W directly in the gut!@n");
                break;
            case 4:
                actUser("@WYou punch @C$N@W in the arm!@n");
                actVictim("@C$n@W punches you in the arm!@n");
                actOthers("@c$n@W punches @C$N@W in the arm!@n");
                break;
            case 5:
                actUser("@WYou punch @C$N@W in the leg!@n");
                actVictim("@C$n@W punches you in the leg!@n");
                actOthers("@c$n@W punches @C$N@W in the leg!@n");
                break;
        }
    }

    void Punch::announceObject() {
        actUser("@WYou punch $p@W as hard as you can!@n");
        actRoom("@C$n@W punches $p@W extremely hard!@n");
    }

    // KICK


}