#include "dbat/attack.h"
#include "dbat/combat.h"
#include "dbat/utils.h"
#include "dbat/comm.h"
#include "dbat/techniques.h"
#include "dbat/class.h"
#include "dbat/races.h"
#include "dbat/random.h"
#include "dbat/fight.h"

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
        trim(input);
        args = split(input, ' ');
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
                send_to_char(user, "You need %d in %s to use this attack.\r\n", required, spell_info[skillID].name);
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
        if(GET_CHARGE(user) < currentKiCost || GET_CHARGE(user) <= 0) {
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

    DefenseResult Attack::attackOutcome(char_data* user, char_data* victim, int skillID, bool kiAttack) {
        initStats();

        currentHitProbability = roll_accuracy(user, init_skill(user, skillID), kiAttack);
        currentChanceToHit = chance_to_hit(user);

        if(isPhysical() && !usesWeapon()) {
            if (IS_KABITO(user) && !IS_NPC(user)) {
                if (GET_SKILL_BASE(user, SKILL_STYLE) >= 75)
                    currentChanceToHit -= currentChanceToHit * 0.2;
            }
        }

        currentSpeedIndexCheck = handle_speed(user, victim);

        currentHitProbability += currentSpeedIndexCheck;

        handle_defense(victim, &currentParryCheck, &currentBlockCheck, &currentDodgeCheck);

        tech_handle_posmodifier(victim, currentParryCheck, currentBlockCheck, currentDodgeCheck, currentHitProbability);

        if(canZanzoken()) {
            if(isKiAttack() && !isPhysical()) {
                if (!tech_handle_zanzoken(user, victim, getName().c_str())) {
                    dodge_ki(user, victim, getHoming(), getAtkID(), initSkill, SKILL_TSUIHIDAN); /* Effects on the room from dodging a ki attack
                               Num 1: [ 0 for non-homing, 1 for homing ki attacks, 2 for guided ]
                               Num 2: [ Number of attack for damtype ]*/
                    pcost(victim, 0, GET_MAX_HIT(victim) / 200);
                    pcost(user, currentKiCost, 0);
                    return DefenseResult::Missed;
                }
            } else {
                if (!tech_handle_zanzoken(user, victim, getName().c_str())) {
                    COMBO(user) = -1;
                    COMBHITS(user) = 0;
                    currentStaminaCost /= 2;
                    pcost(victim, 0, GET_MAX_HIT(victim) / 200);
                    return DefenseResult::Missed;
                }
            }

        }

        calcDamage = damtype(user, getAtkID(), initSkill, attPerc);
        if(currentHitProbability < currentChanceToHit - 20) {
            // So you just missed...
            return DefenseResult::Missed;
        } else {
            // it was a clean hit! Or should be...

            if(victim->getAffectModifier(APPLY_PERFECT_DODGE) != 0) {
                return DefenseResult::Perfect_Dodged;
            }


            if(victim->getCurST() > 0) {
                return calculateDefense();
            }

            //Victim failed to defend, we have a clean hit!
            return DefenseResult::Failed;
        }
        //Default to fail
        return DefenseResult::Failed;
    }

    DefenseResult Attack::calculateDefense() {
        double dodgeChance = ((double) currentDodgeCheck / 5.0) * ((double) axion_dice(0) / 120.0) * (1.0 + victim->getAffectModifier(APPLY_DODGE_PERC));
        double blockChance = ((double) currentBlockCheck / 3.0) * ((double) axion_dice(0) / 120.0) * (1.0 + victim->getAffectModifier(APPLY_BLOCK_PERC));

        double overcomeDodge = (double) currentChanceToHit * ((double) axion_dice(0) / 120.0);
        double overcomeBlock = (double) currentChanceToHit * ((double) axion_dice(0) / 120.0);
        

        if(canParry() && calculateDeflect()) return DefenseResult::Parried;
        if(canDodge() && dodgeChance > overcomeDodge) return DefenseResult::Dodged;
        if(canBlock() && blockChance > overcomeBlock) return DefenseResult::Blocked;

        return DefenseResult::Failed;
    }

    Result Attack::attackCharacter() {
        if(!can_kill(user, victim, nullptr, canKillType())) return Result::Canceled;
        if (handle_defender(victim, user)) {
            victim = GET_DEFENDER(victim);
            // should I do can_kill again here???
        }

        DefenseResult result = attackOutcome(user, victim, getSkillID(), isKiAttack());
        if(result == DefenseResult::Blocked) return handleBlock();
        if(result == DefenseResult::Parried) return handleParry();
        if(result == DefenseResult::Dodged) return handleDodge();
        if(result == DefenseResult::Perfect_Dodged) return handlePerfectDodge();
        if(result == DefenseResult::Failed) return handleCleanHit();
        if(result == DefenseResult::Missed) return Result::Missed;

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
        double parryChance = ((double) currentParryCheck / 6.0) * ((double) axion_dice(0) / 120.0) * (1.0 + victim->getAffectModifier(APPLY_PARRY_PERC));
        double overcomeParry = (double) currentChanceToHit * ((double) axion_dice(0) / 120.0);

        return ((!IS_NPC(victim) || !MOB_FLAGGED(victim, MOB_DUMMY)) && parryChance > overcomeParry);
    }

    Result Attack::attackObject() {
        if(!can_kill(user, nullptr, obj, canKillType())) return Result::Canceled;
        calcDamage = calculateObjectDamage();
        announceObject();
        return Result::Landed;
    }

    void Attack::attackPreprocess() {

    }

    void Attack::attackPostprocess() {

    }

    void Attack::onLanded() {
        attackPreprocess();
        switch(defenseResult) {
            case DefenseResult::Blocked:
                calcDamage /= 4;
                break;
        }
        if(calcDamage < 1) calcDamage = 1;
        hurt(0, limbhurtChance(), user, victim, obj, calcDamage, isKiAttack());
        if(victim) {
            if(isPhysical()) tech_handle_fireshield(user, victim, getBodyPart().c_str());
            if(canCombo()) handle_multihit(user, victim);
        }
        attackPostprocess();
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

    Result MeleeAttack::handlePerfectDodge() {
        actVictim("@C$n@W moves so slowly that you dodge their attack with ease.@n");
        actUser("@WYou move quickly and yet @C$N@W simply sidesteps you!@n");
        actOthers("@C$n@W moves quickly and yet @c$N@W dodges with ease!@n");

        if(victim->getCurKI() > 0) {
            victim->decCurKI(calcDamage * 1 + victim->getAffectModifier(APPLY_PERFECT_DODGE));
        } else {
            victim->decCurST(2 * calcDamage * 1 + victim->getAffectModifier(APPLY_PERFECT_DODGE));
            actVictim("@WContinuing to dodge without Ki takes a heavy toll.@n");
        }

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

    void MeleeAttack::handleHitspot() {
        switch(hitspot) {
            case 1:
                if (GET_BONUS(user, BONUS_SOFT)) 
                    calcDamage *= calc_critical(user, 2);
                break;
            case 2:
                calcDamage *= calc_critical(user, 0);
                break;
            case 3:
                if (GET_BONUS(user, BONUS_SOFT)) 
                    calcDamage *= calc_critical(user, 2);
                break;
            case 4:
            case 5:
                calcDamage *= calc_critical(user, 1);
                break;
        }
    }

    // RangedAttack
    int RangedAttack::canKillType() {
        return 1;
    }

    void MeleeAttack::announceParry() {
        actUser("@C$N@W parries your " + getName() + " with a punch of $S own!@n");
        actVictim("@WYou parry @C$n's@W " + getName() + " with a punch of your own!@n");
        actOthers("@C$N@W parries @c$n's@W " + getName() + " with a punch of $S own!@n");
    }

    void MeleeAttack::announceBlock() {
        actUser("@C$N@W moves quickly and blocks your " + getName() + "!@n");
        actVictim("@WYou move quickly and block @C$n's@W " + getName() + "!@n");
        actOthers("@C$N@W moves quickly and blocks @c$n's@W " + getName() + "!@n");
    }

    void MeleeAttack::announceDodge() {
        actUser("@C$N@W manages to dodge your " + getName() + "!@n");
        actVictim("@WYou dodge @C$n's@W " + getName() + "!@n");
        actOthers("@C$N@W manages to dodge @c$n's@W " + getName() + "!@n");
    }

    void MeleeAttack::announceMiss() {
        actUser("@WYou can't believe it but your " + getName() + " misses!@n");
        actVictim("@C$n@W throws a " + getName() + " at you but somehow misses!@n");
        actOthers("@c$n@W throws a " + getName() + " at @C$N@W but somehow misses!@n");
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
    void Kick::announceParry() {
        actUser("@C$N@W parries your kick with a kick of $S own!@n");
        actVictim("@WYou parry @C$n's@W kick with a kick of your own!@n");
        actOthers("@C$N@W parries @c$n's@W kick with a kick of $S own!@n");
    }

    void Kick::announceBlock() {
        actUser("@C$N@W moves quickly and blocks your kick!@n");
        actVictim("@WYou move quickly and block @C$n's@W kick!@n");
        actOthers("@C$N@W moves quickly and blocks @c$n's@W kick!@n");
    }

    void Kick::announceDodge() {
        actUser("@C$N@W manages to dodge your kick!@n");
        actVictim("@WYou dodge @C$n's@W kick!@n");
        actOthers("@C$N@W manages to dodge @c$n's@W kick!@n");
    }

    void Kick::announceMiss() {
        actUser("@WYou can't believe it, your kick misses!@n");
        actVictim("@C$n@W throws a kick at you, but thankfully misses!@n");
        actOthers("@c$n@W throws a kick at @C$N@W, but misses!@n");
    }

    void Kick::announceObject() {
        actUser("@WYou kick $p@W as hard as you can!@n");
        actRoom("@C$n@W kicks $p@W extremely hard!@n");
    }

    void Kick::announceHitspot() {
        switch (hitspot) {
            case 1:
                actUser("@WYou slam your foot into @C$N's@W body!@n");
                actVictim("@C$n@W slams $s foot into your body!@n");
                actOthers("@c$n@W slams $s foot into @C$N's@W body!@n");
                break;
            case 2: /* Critical */
                actUser("@WYou slam your foot into @C$N's@W face!@n");
                actVictim("@C$n@W slams $s foot into your face!@n");
                actOthers("@c$n@W slams $s foot into @C$N's@W face!@n");
                break;
            case 3:
                actUser("@WYou land your foot against @C$N's@W gut!@n");
                actVictim("@C$n@W lands $s foot against your gut!@n");
                actOthers("@C$n@W lands $s foot against @C$N's@W gut!@n");
                break;
            case 4: /* Weak */
                actUser("@WYou land your foot against @C$N's@W leg!@n");
                actVictim("@C$n@W lands $s foot against your leg!@n");
                actOthers("@C$n@W lands $s foot against @C$N's@W leg!@n");
                break;
            case 5: /* Weak */
                actUser("@WYou land your foot against @C$N's@W arm!@n");
                actVictim("@C$n@W lands $s foot against your arm!@n");
                actOthers("@C$n@W lands $s foot against @C$N's@W arm!@n");
                break;
        }
    }

    // ELBOW
    void Elbow::announceParry() {
        actUser("@C$N@W parries your elbow with an elbow of $S own!@n");
        actVictim("@WYou parry @C$n's@W elbow with an elbow of your own!@n");
        actOthers("@C$N@W parries @c$n's@W elbow with an elbow of $S own!@n");
    }

    void Elbow::announceBlock() {
        actUser("@C$N@W blocks your elbow strike!@n");
        actVictim("@WYou block @C$n's@W elbow strike!@n");
        actOthers("@C$N@W blocks @c$n's@W elbow strike!@n");
    }

    void Elbow::announceDodge() {
        actUser("@C$N@W dodges your elbow strike!@n");
        actVictim("@WYou dodge @C$n's@W elbow strike!@n");
        actOthers("@C$N@W dodges @c$n's@W elbow strike!@n");
    }

    void Elbow::announceMiss() {
        actUser("@WYou can't believe it, your elbow misses!@n");
        actVictim("@C$n@W throws an elbow at you, but thankfully misses!@n");
        actOthers("@c$n@W throws an elbow at @C$N@W, but misses!@n");
    }

    void Elbow::announceObject() {
        actUser("@WYou elbow $p@W as hard as you can!@n");
        actRoom("@C$n@W elbows $p@W extremely hard!@n");
    }

    void Elbow::announceHitspot() {
        switch (hitspot) {
            case 1:
                actUser("@WYou slam your elbow into @C$N's@W body!@n");
                actVictim("@C$n@W slams $s elbow into your body!@n");
                actOthers("@c$n@W slams $s elbow into @C$N's@W body!@n");
                break;
            case 2: /* Critical */
                actUser("@WYou slam your elbow into @C$N's@W face!@n");
                actVictim("@C$n@W slams $s elbow into your face!@n");
                actOthers("@c$n@W slams $s elbow into @C$N's@W face!@n");
                break;
            case 3:
                actUser("@WYou land your elbow against @C$N's@W gut!@n");
                actVictim("@C$n@W lands $s elbow against your gut!@n");
                actOthers("@C$n@W lands $s elbow against @C$N's@W gut!@n");
                break;
            case 4: /* Weak */
                actUser("@WYou land your elbow against @C$N's@W leg!@n");
                actVictim("@C$n@W lands $s elbow against your leg!@n");
                actOthers("@C$n@W lands $s elbow against @C$N's@W leg!@n");
                break;
            case 5: /* Weak */
                actUser("@WYou land your elbow against @C$N's@W arm!@n");
                actVictim("@C$n@W lands $s elbow against your arm!@n");
                actOthers("@C$n@W lands $s elbow against @C$N's@W arm!@n");
                break;
        }
    }


    // KNEE
    void Knee::announceParry() {
        actUser("@C$N@W parries your knee with a knee of $S own!@n");
        actVictim("@WYou parry @C$n's@W knee with a knee of your own!@n");
        actOthers("@C$N@W parries @c$n's@W knee with a knee of $S own!@n");
    }

    void Knee::announceBlock() {
        actUser("@C$N@W blocks your knee strike!@n");
        actVictim("@WYou block @C$n's@W knee strike!@n");
        actOthers("@C$N@W blocks @c$n's@W knee strike!@n");
    }

    void Knee::announceDodge() {
        actUser("@C$N@W dodges your knee strike!@n");
        actVictim("@WYou dodge @C$n's@W knee strike!@n");
        actOthers("@C$N@W dodges @c$n's@W knee strike!@n");
    }

    void Knee::announceMiss() {
        actUser("@WYou can't believe it, your knee strike misses!@n");
        actVictim("@C$n@W throws a knee strike at you, but thankfully misses!@n");
        actOthers("@c$n@W throws a knee strike at @C$N@W, but misses!@n");
    }

    void Knee::announceObject() {
        actUser("@WYou knee $p@W as hard as you can!@n");
        actRoom("@C$n@W knees $p@W extremely hard!@n");
    }

    void Knee::announceHitspot() {
        switch (hitspot) {
            case 1:
                actUser("@WYou slam your knee into @C$N's@W body!@n");
                actVictim("@C$n@W slams $s knee into your body!@n");
                actOthers("@c$n@W slams $s knee into @C$N's@W body!@n");
                break;
            case 2: /* Critical */
                actUser("@WYou slam your knee into @C$N's@W face!@n");
                actVictim("@C$n@W slams $s knee into your face!@n");
                actOthers("@c$n@W slams $s knee into @C$N's@W face!@n");
                break;
            case 3:
                actUser("@WYou land your knee against @C$N's@W gut!@n");
                actVictim("@C$n@W lands $s knee against your gut!@n");
                actOthers("@C$n@W lands $s knee against @C$N's@W gut!@n");
                break;
            case 4: /* Weak */
                actUser("@WYou land your knee against @C$N's@W leg!@n");
                actVictim("@C$n@W lands $s knee against your leg!@n");
                actOthers("@C$n@W lands $s knee against @C$N's@W leg!@n");
                break;
            case 5: /* Weak */
                actUser("@WYou land your knee against @C$N's@W arm!@n");
                actVictim("@C$n@W lands $s knee against your arm!@n");
                actOthers("@C$n@W lands $s knee against @C$N's@W arm!@n");
                break;
        }
    }

    // ROUNDHOUSE
    std::optional<int> Roundhouse::hasCooldown() {
        if(IS_NAIL(user) && GET_SKILL_BASE(user, SKILL_STYLE) >= 75) return 5;
        return 7;
    }

    void Roundhouse::announceParry() {
        actUser("@C$N@W parries your roundhouse with a roundhouse of $S own!@n");
        actVictim("@WYou parry @C$n's@W roundhouse with a roundhouse of your own!@n");
        actOthers("@C$N@W parries @c$n's@W roundhouse with a roundhouse of $S own!@n");
    }

    void Roundhouse::announceBlock() {
        actUser("@C$N@W moves quickly and blocks your roundhouse!@n");
        actVictim("@WYou move quickly and block @C$n's@W roundhouse!@n");
        actOthers("@C$N@W moves quickly and blocks @c$n's@W roundhouse!@n");
    }

    void Roundhouse::announceDodge() {
        actUser("@C$N@W manages to dodge your roundhouse!@n");
        actVictim("@WYou dodge @C$n's@W roundhouse!@n");
        actOthers("@C$N@W manages to dodge @c$n's@W roundhouse!@n");
    }

    void Roundhouse::announceMiss() {
        actUser("@WYou can't believe it but your roundhouse misses!@n");
        actVictim("@C$n@W throws a roundhouse at you but somehow misses!@n");
        actOthers("@c$n@W throws a roundhouse at @C$N@W but somehow misses!@n");
    }

    void Roundhouse::announceObject() {
        actUser("@WYou roundhouse $p@W as hard as you can!@n");
        actRoom("@C$n@W roundhouses $p@W extremely hard!@n");
    }

    void Roundhouse::handleHitspot() {
        LegAttack::handleHitspot();
        if (hitspot == 2 && !AFF_FLAGGED(victim, AFF_FLYING) && GET_POS(victim) == POS_STANDING && rand_number(1, 8) >= 7) {
            handle_knockdown(victim);
        }
    }

    void Roundhouse::announceHitspot() {
        switch (hitspot) {
            case 1:
                actUser("@WYou spin in mid air and land a kick into @C$N's@W body!@n");
                actVictim("@C$n@W spins in mid air and lands a kick into your body!@n");
                actOthers("@c$n@W spins in mid air and lands a kick into @C$N's@W body!@n");
                break;
            case 2: /* Critical */
                actUser("@WYou spin a fierce roundhouse into @C$N's@W gut!@n");
                actVictim("@C$n@W spins a fierce roundhouse into your gut!@n");
                actOthers("@c$n@W spins a fierce roundhouse into @C$N's@W gut!@n");
                break;
            case 3:
                actUser("@WYou throw a roundhouse at @C$N@W, hitting $M directly in the neck!@n");
                actVictim("@C$n@W throws a roundhouse at you, hitting YOU directly in the neck!@n");
                actOthers("@c$n@W throws a roundhouse at @C$N@W, hitting $M directly in the face!@n");
                break;
            case 4: /* Weak */
                actUser("@WYour poorly aimed roundhouse hits @C$N@W in the arm!@n");
                actVictim("@C$n@W poorly aims a roundhouse and hits you in the arm!@n");
                actOthers("@c$n@W poorly aims a roundhouse and hits @C$N@W in the arm!@n");
                break;
            case 5: /* Weak 2 */
                actUser("@WYou slam a roundhouse into @C$N's@W leg!@n");
                actVictim("@C$n@W slams a roundhouse into your leg!@n");
                actOthers("@c$n@W slams a roundhouse into @C$N's@W leg!@n");
                break;
        }
    }

    int Roundhouse::limbhurtChance() {
        switch(hitspot) {
            case 4:
            case 5:
                return 195;
        }
        return 0;
    }


    // Uppercut
    std::optional<int> Uppercut::hasCooldown() {
        if (IS_FRIEZA(user) && GET_SKILL_BASE(user, SKILL_STYLE) >= 75) 
            handle_cooldown(user, 5);
        else
            handle_cooldown(user, 7);
    }

    void Uppercut::announceObject() {
        actUser("@WYou uppercut $p@W as hard as you can!@n");
        actRoom("@C$n@W uppercuts $p@W extremely hard!@n");
    }

    void Uppercut::announceHitspot() {
        switch (hitspot) {
            case 1:
                actUser("@WYou leap up and launch an uppercut into @C$N's@W body!@n");
                actVictim("@C$n@W leaps up and launches an uppercut into your body!@n");
                actOthers("@c$n@W leaps up and launches an uppercut into @C$N's@W body!@n");
                break;
            case 2: /* Critical */
                actUser("@WYou smash an uppercut into @C$N's@W chin!@n");
                actVictim("@C$n@W smashes an uppercut into your chin!@n");
                actOthers("@c$n@W smashes an uppercut into @C$N's@W chin!@n");
                break;
            case 3:
                actUser("@WYou uppercut @C$N@W, hitting $M directly in chest!@n");
                actVictim("@C$n@W uppercuts you, hitting you directly in the chest!@n");
                actOthers("@c$n@W uppercuts @C$N@W, hitting $M directly in the chest!@n");
                break;
            case 4: /* Weak */
                actUser("@WYour poorly aimed uppercut hits @C$N@W in the arm!@n");
                actVictim("@C$n@W poorly aims an uppercut and hits you in the arm!@n");
                actOthers("@c$n@W poorly aims an uppercut and hits @C$N@W in the arm!@n");
                break;
            case 5: /* Weak 2 */
                actUser("@WYou slam an uppercut into @C$N's@W leg!@n");
                actVictim("@C$n@W slams an uppercut into your leg!@n");
                actOthers("@c$n@W slams an uppercut into @C$N's@W leg!@n");
                break;
        }
    }

    void Uppercut::handleHitspot() {
        switch(hitspot) {
            case 1:
            case 2:
                if (GET_BONUS(user, BONUS_SOFT)) {
                    calcDamage *= calc_critical(user, 2);
                }
                if (!AFF_FLAGGED(victim, AFF_KNOCKED) &&
                        (rand_number(1, 8) >= 7 && (GET_HIT(victim) > GET_HIT(user) / 5) &&
                         !AFF_FLAGGED(victim, AFF_SANCTUARY))) {
                        act("@C$N@W is knocked out!@n", true, user, nullptr, victim, TO_CHAR);
                        act("@WYou are knocked out!@n", true, user, nullptr, victim, TO_VICT);
                        act("@C$N@W is knocked out!@n", true, user, nullptr, victim, TO_NOTVICT);
                        victim->setStatusKnockedOut();
                }
                break;
            case 3:
                calcDamage *= calc_critical(user, 0);
                break;
            case 4:
            case 5:
                calcDamage *= calc_critical(user, 1);
                break;
        }
    }


    //Slam
    std::optional<int> Slam::hasCooldown() {
        if (IS_BARDOCK(user) && GET_SKILL_BASE(user, SKILL_STYLE) >= 75) 
            handle_cooldown(user, 7);
        else
            handle_cooldown(user, 9);
    }

    void Slam::announceObject() {
        actUser("@WYou slam $p@W as hard as you can!@n");
        actRoom("@C$n@W slams $p@W extremely hard!@n");
    }

    void Slam::announceHitspot() {
        switch (hitspot) {
            case 1:
                actUser("@WYou disappear, appearing above @C$N@W and slam a double fisted blow into $M!@n");
                actVictim("@C$n@W disappears, only to appear above you, slamming a double fisted blow into you!@n");
                actOthers("@c$n@W disappears, only to appear above @C$N@W, slamming a double fisted blow into $M!@n");
                break;
            case 2: /* Critical */
                actUser("@WYou disappear, reappearing in front of @C$N@W, you grab $M! Spinning you send $M flying into the ground!@n");
                actVictim("@C$n@W disappears, reappearing in front of you, and $e grabs you! Spinning quickly $e sends you flying into the ground!@n");
                actOthers("@c$n@W disappears, reappearing in front of @C$N@W, and grabs $M! Spinning quickly $e sends $M flying into the ground!@n");

                if (ROOM_DAMAGE(IN_ROOM(victim)) <= 95 && !ROOM_FLAGGED(IN_ROOM(victim), ROOM_SPACE)) {
                        act("@W$N@W slams into the ground forming a large crater with $S body!@n", true, user, nullptr,
                            victim,
                            TO_CHAR);
                        act("@WYou slam into the ground forming a large crater with your body!@n", true, user, nullptr,
                            victim,
                            TO_VICT);
                        act("@W$N@W slams into the ground forming a large crater with $S body!@n", true, user, nullptr,
                            victim,
                            TO_NOTVICT);
                        if (SECT(IN_ROOM(victim)) != SECT_INSIDE && SECT(IN_ROOM(victim)) != SECT_UNDERWATER &&
                            SECT(IN_ROOM(victim)) != SECT_WATER_SWIM && SECT(IN_ROOM(victim)) != SECT_WATER_NOSWIM) {
                            impact_sound(user, "@wA loud roar is heard nearby!@n\r\n");
                            switch (rand_number(1, 8)) {
                                case 1:
                                    act("Debris is thrown into the air and showers down thunderously!", true, user,
                                        nullptr,
                                        victim, TO_CHAR);
                                    act("Debris is thrown into the air and showers down thunderously!", true, user,
                                        nullptr,
                                        victim, TO_ROOM);
                                    break;
                                case 2:
                                    if (rand_number(1, 4) == 4 && ROOM_EFFECT(IN_ROOM(victim)) == 0) {
                                        ROOM_EFFECT(IN_ROOM(victim)) = 1;
                                        act("Lava leaks up through cracks in the crater!", true, user, nullptr, victim,
                                            TO_CHAR);
                                        act("Lava leaks up through cracks in the crater!", true, user, nullptr, victim,
                                            TO_ROOM);
                                    }
                                    break;
                                case 3:
                                    act("A cloud of dust envelopes the entire area!", true, user, nullptr, victim, TO_CHAR);
                                    act("A cloud of dust envelopes the entire area!", true, user, nullptr, victim, TO_ROOM);
                                    break;
                                case 4:
                                    act("The surrounding area roars and shudders from the impact!", true, user, nullptr,
                                        victim,
                                        TO_CHAR);
                                    act("The surrounding area roars and shudders from the impact!", true, user, nullptr,
                                        victim,
                                        TO_ROOM);
                                    break;
                                case 5:
                                    act("The ground shatters apart from the stress of the impact!", true, user, nullptr,
                                        victim,
                                        TO_CHAR);
                                    act("The ground shatters apart from the stress of the impact!", true, user, nullptr,
                                        victim,
                                        TO_ROOM);
                                    break;
                                case 6:
                                    /* One less message */
                                    break;
                                default:
                                    /* we want no message for the default */
                                    break;
                            }
                        }
                        if (SECT(IN_ROOM(victim)) == SECT_UNDERWATER) {
                            switch (rand_number(1, 3)) {
                                case 1:
                                    act("The water churns violently!", true, user, nullptr, victim, TO_CHAR);
                                    act("The water churns violently!", true, user, nullptr, victim, TO_ROOM);
                                    break;
                                case 2:
                                    act("Large bubbles rise from the movement!", true, user, nullptr, victim, TO_CHAR);
                                    act("Large bubbles rise from the movement!", true, user, nullptr, victim, TO_ROOM);
                                    break;
                                case 3:
                                    act("The water collapses in on the hole created!", true, user, nullptr, victim,
                                        TO_CHAR);
                                    act("The water collapses in on the hole create!", true, user, nullptr, victim, TO_ROOM);
                                    break;
                            }
                        }
                        if (SECT(IN_ROOM(victim)) == SECT_WATER_SWIM || SECT(IN_ROOM(victim)) == SECT_WATER_NOSWIM) {
                            switch (rand_number(1, 3)) {
                                case 1:
                                    act("A huge column of water erupts from the impact!", true, user, nullptr, victim,
                                        TO_CHAR);
                                    act("A huge column of water erupts from the impact!", true, user, nullptr, victim,
                                        TO_ROOM);
                                    break;
                                case 2:
                                    act("The impact briefly causes a swirling vortex of water!", true, user, nullptr,
                                        victim,
                                        TO_CHAR);
                                    act("The impact briefly causes a swirling vortex of water!", true, user, nullptr,
                                        victim,
                                        TO_ROOM);
                                    break;
                                case 3:
                                    act("A huge depression forms in the water and erupts into a wave from the impact!",
                                        true, user, nullptr, victim, TO_CHAR);
                                    act("A huge depression forms in the water and erupts into a wave from the impact!",
                                        true, user, nullptr, victim, TO_ROOM);
                                    break;
                            }
                        }
                        if (SECT(IN_ROOM(victim)) == SECT_INSIDE) {
                            impact_sound(user, "@wA loud roar is heard nearby!@n\r\n");
                            switch (rand_number(1, 8)) {
                                case 1:
                                    act("Debris is thrown into the air and showers down thunderously!", true, user,
                                        nullptr,
                                        victim, TO_CHAR);
                                    act("Debris is thrown into the air and showers down thunderously!", true, user,
                                        nullptr,
                                        victim, TO_ROOM);
                                    break;
                                case 2:
                                    act("The structure of the surrounding room cracks and quakes from the impact!",
                                        true, user, nullptr, victim, TO_CHAR);
                                    act("The structure of the surrounding room cracks and quakes from the impact!",
                                        true, user, nullptr, victim, TO_ROOM);
                                    break;
                                case 3:
                                    act("Parts of the ceiling collapse, crushing into the floor!", true, user, nullptr,
                                        victim,
                                        TO_CHAR);
                                    act("Parts of the ceiling collapse, crushing into the floor!", true, user, nullptr,
                                        victim,
                                        TO_ROOM);
                                    break;
                                case 4:
                                    act("The surrounding area roars and shudders from the impact!", true, user, nullptr,
                                        victim,
                                        TO_CHAR);
                                    act("The surrounding area roars and shudders from the impact!", true, user, nullptr,
                                        victim,
                                        TO_ROOM);
                                    break;
                                case 5:
                                    act("The ground shatters apart from the stress of the impact!", true, user, nullptr,
                                        victim,
                                        TO_CHAR);
                                    act("The ground shatters apart from the stress of the impact!", true, user, nullptr,
                                        victim,
                                        TO_ROOM);
                                    break;
                                case 6:
                                    act("The walls of the surrounding room crack in the same instant!", true, user,
                                        nullptr,
                                        victim, TO_CHAR);
                                    act("The walls of the surrounding room crack in the same instant!", true, user,
                                        nullptr,
                                        victim, TO_ROOM);
                                    break;
                                default:
                                    /* we want no message for the default */
                                    break;
                        }
                    }
                }
        
                break;
            case 3:
                actUser("@WYou fly at @C$N@W, slamming both your fists into $S gut as you fly!@n");
                actVictim("@C$n@W flies at you, slamming both $s fists into your gut as $e flies!@n");
                actOthers("@c$n@W flies at @C$N@W, slamming both $s fists into $S gut as $e flies!@n");
                break;
            case 4: /* Weak */
                actUser("@WYou slam both your fists into @C$N@W, hitting $M in the arm!@n");
                actVictim("@C$n@W slams both $s fists into you, hitting you in the arm!@n");
                actOthers("@c$n@W slams both $s fists into @C$N@W, hitting $M in the arm!@n");
                break;
            case 5: /* Weak 2 */
                actUser("@WYou slam both your fists into @C$N's@W leg!@n");
                actVictim("@C$n@W slams both $s fists into your leg!@n");
                actOthers("@c$n@W slams both $s fists into @C$N's@W leg!@n");
                break;
        }
    }

    void Slam::handleHitspot() {
        switch(hitspot) {
            case 1:
                if (GET_BONUS(user, BONUS_SOFT)) 
                    calcDamage *= calc_critical(user, 2);
                break;
            case 2:
                calcDamage *= calc_critical(user, 0);
                if (!AFF_FLAGGED(victim, AFF_KNOCKED) &&
                        (rand_number(1, 8) >= 7 && (GET_HIT(victim) > GET_HIT(user) / 5) &&
                         !AFF_FLAGGED(victim, AFF_SANCTUARY))) {
                    act("@C$N@W is knocked out!@n", true, user, nullptr, victim, TO_CHAR);
                    act("@WYou are knocked out!@n", true, user, nullptr, victim, TO_VICT);
                    act("@C$N@W is knocked out!@n", true, user, nullptr, victim, TO_NOTVICT);
                    victim->setStatusKnockedOut();
                } else if ((GET_POS(victim) == POS_STANDING || GET_POS(victim) == POS_FIGHTING) &&
                               !AFF_FLAGGED(victim, AFF_KNOCKED)) {
                    GET_POS(victim) = POS_SITTING;
                }
                victim->getRoom()->modDamage(5);
                break;
            case 3:
                if (GET_BONUS(user, BONUS_SOFT)) 
                    calcDamage *= calc_critical(user, 2);
                break;
            case 4:
            case 5:
                calcDamage *= calc_critical(user, 1);
                break;
        }
    }


    // Heeldrop
    std::optional<int> Heeldrop::hasCooldown() {
        if(IS_PICCOLO(user) && GET_SKILL_BASE(user, SKILL_STYLE) >= 75) 
            return 5;
        else
            return 7;
    }

    void Heeldrop::announceObject() {
        actUser("@WYou heeldrop $p@W as hard as you can!@n");
        actRoom("@C$n@W heeldrops $p@W extremely hard!@n");
    }

    void Heeldrop::handleHitspot() {
        switch (hitspot) {
            case 1:
                tech_handle_crashdown(user, victim);
                if (GET_BONUS(user, BONUS_SOFT)) 
                    calcDamage *= calc_critical(user, 2);
                break;
            case 2:
                tech_handle_crashdown(user, victim);
                calcDamage *= calc_critical(user, 0);
                break;
            case 3:
                if (GET_BONUS(user, BONUS_SOFT)) 
                    calcDamage *= calc_critical(user, 2);
                break;
            case 4:
            case 5:
                calcDamage *= calc_critical(user, 1);
                break;
        }
        
    }

    void Heeldrop::announceHitspot() {
        switch (hitspot) {
            case 1:
                actUser("@WYou disappear, appearing above @C$N@W you spin and heeldrop $M in the face!@n");
                actVictim("@C$n@W disappears, only to appear above you, spinning quickly and heeldropping you in the face!@n");
                actOthers("@c$n@W disappears, only to appear above @C$N@W, spinning quickly and heeldropping $M in the face!@n");
                break;
            case 2: /* Critical */
                actUser("@WYou disappear, reappearing in front of @C$N@W, you flip upside down and slam your heel into the top of $S head!@n");
                actVictim("@C$n@W disappears, reappearing in front of you, $e flips upside down and slams $s heel into the top of your head!@n");
                actOthers("@c$n@W disappears, reappearing in front of @C$N@W, $e flips upside down and slams $s heel into the top of @C$N@W's head!@n");
                break;
            case 3:
                actUser("@WYou fly at @C$N@W, heeldropping $S gut as you fly!@n");
                actVictim("@C$n@W flies at you, heeldropping your gut as $e flies!@n");
                actOthers("@c$n@W flies at @C$N@W, heeldropping $S gut as $e flies!@n");
                break;
            case 4: /* Weak */
                actUser("@WYou heeldrop @C$N@W, hitting $M in the arm!@n");
                actVictim("@C$n@W heeldrops you, hitting you in the arm!@n");
                actOthers("@c$n@W heeldrops @C$N@W, hitting $M in the arm!@n");
                break;
            case 5: /* Weak 2 */
                actUser("@WYou heeldrop @C$N's@W leg!@n");
                actVictim("@C$n@W heeldrops your leg!@n");
                actOthers("@c$n@W heeldrops @C$N's@W leg!@n");
                break;
        }
    }


    // Headbutt
    std::optional<int> Headbutt::hasCooldown() {
        if(IS_KURZAK(user) && GET_SKILL_BASE(user, SKILL_STYLE) >= 100) 
            return 5;
        else
            return 6;
    }

    void Headbutt::announceObject() {
        actUser("@WYou headbutt $p@W as hard as you can!@n");
        actRoom("@C$n@W headbutt $p@W extremely hard!@n");
    }

    void Headbutt::handleHitspot() {
        if (IS_KURZAK(user)) {
            if (GET_SKILL(user, SKILL_HEADBUTT) >= 60) {
                calcDamage += calcDamage * 0.1;
            } else if (GET_SKILL(user, SKILL_HEADBUTT) >= 40) {
                calcDamage += calcDamage * 0.05;
            }
        }

        int multiplier = 0;
        switch (hitspot) {
            case 1:
                if (GET_BONUS(user, BONUS_SOFT)) 
                    calcDamage *= calc_critical(user, 2);
                break;
            case 2:
                if (!AFF_FLAGGED(victim, AFF_KNOCKED) &&
                        (rand_number(1, 7) >= 4 && (GET_HIT(victim) > GET_HIT(user) / 5) &&
                         !AFF_FLAGGED(victim, AFF_SANCTUARY))) {
                    act("@C$N@W is knocked out!@n", true, user, nullptr, victim, TO_CHAR);
                    act("@WYou are knocked out!@n", true, user, nullptr, victim, TO_VICT);
                    act("@C$N@W is knocked out!@n", true, user, nullptr, victim, TO_NOTVICT);
                    victim->setStatusKnockedOut();
                }
                
                if (IS_KURZAK(user) && !IS_NPC(user)) {
                    if (GET_SKILL_BASE(user, SKILL_STYLE) >= 75)
                        multiplier += 1;
                }
                calcDamage *= (calc_critical(user, 0) + multiplier);
                break;
            case 3:
                if (GET_BONUS(user, BONUS_SOFT)) 
                    calcDamage *= calc_critical(user, 2);
                break;
            case 4:
            case 5:
                calcDamage *= calc_critical(user, 1);
                break;
        }
        
    }

    void Headbutt::announceHitspot() {
        switch (hitspot) {
            case 1:
                actUser("@WYou grab @c$N@W by the shoulders and slam your head into $S chest!@n");
                actVictim("@C$n@W grabs YOU by the shoulders and slams $s head into YOUR chest!@n");
                actOthers("@C$n@W grabs @c$N@W by the shoulders and slams $s head into @c$N's@W chest!@n");
                break;
            case 2: /* Critical */
                actUser("@WYou grab @c$N@W by the shoulders and slam your head into $S face!@n");
                actVictim("@C$n@W grabs YOU by the shoulders and slams $s head into YOUR face!@n");
                actOthers("@C$n@W grabs @c$N@W by the shoulders and slams $s head into @c$N's@W face!@n");
                break;
            case 3:
                actUser("@WYou grab @c$N@W by the shoulders and slam your head into $S chest!@n");
                actVictim("@C$n@W grabs YOU by the shoulders and slams $s head into YOUR chest!@n");
                actOthers("@C$n@W grabs @c$N@W by the shoulders and slams $s head into @c$N's@W chest!@n");
                break;
            case 4: /* Weak */
                actUser("@WYou grab @c$N@W and barely manage to slam your head into $S leg!@n");
                actVictim("@C$n@W grabs YOU and barely manages to slam $s head into YOUR leg!@n");
                actOthers("@C$n@W grabs @c$N@W and barely manages to slam $s head into @c$N's@W leg!@n");
                break;
            case 5: /* Weak 2 */
                actUser("@WYou grab @c$N@W and barely manage to slam your head into $S arm!@n");
                actVictim("@C$n@W grabs YOU and barely manages to slam $s head into YOUR arm!@n");
                actOthers("@C$n@W grabs @c$N@W and barely manages to slam $s head into @c$N's@W arm!@n");
                break;
        }
    }



    // KI

    int64_t RangedKiAttack::calculateKiCost() {
        if (getTier() == 1)
            attPerc = 0.05;
        if (getTier() == 2)
            attPerc = 0.10;
        if (getTier() == 3)
            attPerc = 0.15;
        if (getTier() == 4)
            attPerc = 0.25;
        if (getTier() == 5)
            attPerc = 0.50;

        return attPerc * getKiEfficiency();
    }

    double RangedKiAttack::getKiEfficiency() {
        return 1;
    }

    Result RangedKiAttack::handleParry() {
        announceParry();
        defenseResult = DefenseResult::Parried;
        pcost(victim, 0, GET_MAX_HIT(victim) / 500);
        improve_skill(victim, SKILL_PARRY, 0);
        parry_ki(attPerc, user, victim, (char*)getName().c_str(), 0, 0, getSkillID(), getAtkID());
        user->getRoom()->modDamage(5);
        return Result::Missed;

    }

    Result RangedKiAttack::handleDodge() {
        announceDodge();
        defenseResult = DefenseResult::Dodged;
        improve_skill(victim, SKILL_DODGE, 0);
        pcost(victim, 0, GET_MAX_HIT(victim) / 500);

        dodge_ki(user, victim, getHoming(), getAtkID(), initSkill, getSkillID());
        user->getRoom()->modDamage(5);
        return Result::Missed;

    }

    Result RangedKiAttack::handlePerfectDodge() {
        actVictim("@C$n@W moves so slowly that you dodge their attack with ease.@n");
        actUser("@WYou move quickly and yet @C$N@W simply sidesteps you!@n");
        actOthers("@C$n@W moves quickly and yet @c$N@W dodges with ease!@n");

        if(victim->getCurKI() > 0) {
            victim->decCurKI(calcDamage * 1 + victim->getAffectModifier(APPLY_PERFECT_DODGE));
        } else {
            victim->decCurST(2 * calcDamage * 1 + victim->getAffectModifier(APPLY_PERFECT_DODGE));
            actVictim("@WContinuing to dodge without Ki takes a heavy toll.@n");
        }

        return Result::Missed;
    }

    Result RangedKiAttack::handleBlock() {
        announceBlock();
        defenseResult = DefenseResult::Blocked;
        improve_skill(victim, SKILL_BLOCK, 0);
        pcost(victim, 0, GET_MAX_HIT(victim) / 500);
        return Result::Landed;

    }

    

    void RangedKiAttack::announceObject() {
        actUser("@WYou fire a " + getName() + " at $p@W!@n");
        actRoom("@C$n@W fires a " + getName() + " at $p@W!@n");
    }

    void RangedKiAttack::announceParry() {
        actUser("@C$N@W deflects your " + getName() + ", sending it flying away!@n");
        actVictim("@WYou deflect @C$n's@W " + getName() + " sending it flying away!@n");
        actOthers("@C$N@W deflects @c$n's@W " + getName() + " sending it flying away!@n");
    }

    void RangedKiAttack::announceBlock() {
        actUser("@C$N@W moves quickly and blocks " + getName() + "!@n");
        actVictim("@WYou move quickly and block @C$n's@W " + getName() + "!@n");
        actOthers("@C$N@W moves quickly and blocks @c$n's@W " + getName() + "!@n");
    }

    void RangedKiAttack::announceDodge() {
        actUser("@C$N@W manages to dodge your " + getName() + ", letting it slam into the surroundings!@n");
        actVictim("@WYou dodge @C$n's@W " + getName() + ", letting it slam into the surroundings!@n");
        actOthers("@C$N@W manages to dodge @c$n's@W " + getName() + ", letting it slam into the surroundings!@n");
    }

    void RangedKiAttack::announceMiss() {
        actUser("@WYou can't believe it but your " + getName() + " misses, flying through the air harmlessly!@n");
        actVictim("@C$n@W fires a " + getName() + " at you, but misses!@n");
        actOthers("@c$n@W fires a " + getName() + " at @C$N@W, but somehow misses!@n");
    }

    


    // Kiball
    void KiBall::execute() {

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

        int mult_roll = rand_number(1, 100), mult_count = 1, mult_chance = 0;

        if(args.empty()) {
            victim = FIGHTING(user);
        } else {
            if(!tech_handle_targeting(user, (char*)args[0].c_str(), &victim, &obj)) return;
        }

        if (initSkill >= 100) {
            mult_chance = 30;
        } else if (initSkill >= 75) {
            mult_chance = 15;
        } else if (initSkill >= 50) {
            mult_chance = 10;
        }

        if (mult_roll <= mult_chance)
            mult_count = rand_number(2, 3);

        while (mult_count > 0) {
            mult_count -= 1;
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
    }

    void KiBall::announceHitspot() {
        switch(hitspot) {
            case 1:
                actUser("@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball slams into $M quickly and explodes with roaring light!@n");
                actVictim("@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball slams into you quickly and explodes with roaring light!@n");
                actOthers("@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball slams into $M quickly and explodes with roaring light!@n");
                break;
            case 2:
                actUser("@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball slams into $S face and explodes, shrouding $S head with smoke!@n");
                actVictim("@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball slams into your face and explodes, leaving you choking on smoke!@n");
                actOthers("@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball slams into $S face and explodes, shrouding $S head with smoke!@n");
                break;
            case 3:
                actUser("@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball slams into $S body and explodes with a loud roar!@n");
                actVictim("@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball slams into your body and explodes with a loud roar!@n");
                actOthers("@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball slams into $S body and explodes with a loud roar!@n");
                break;
            case 4:
                actUser("@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball grazes $S arm and explodes shortly after!@n");
                actVictim("@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball grazes your arm and explodes shortly after!@n");
                actOthers("@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball grazes $S arm and explodes shortly after!@n");
                break;
            case 5:
                actUser("@WYou hold out your hand towards @C$N@W, and fire a bright yellow kiball! The kiball grazes $S leg and explodes shortly after!@n");
                actVictim("@c$n@W holds out $s hand towards you, and fires a bright yellow kiball! The kiball grazes your leg and explodes shortly after!@n");
                actOthers("@c$n@W holds out $s hand towards @C$N@W, and fires a bright yellow kiball! The kiball grazes $S leg and explodes shortly after!@n");
                break;
        }
    }

    //KiBlast
    void KiBlast::attackPreprocess() {
        record = GET_HIT(victim);
        if (IS_ANDROID(user)) {
                if (GET_SKILL(user, SKILL_KIBLAST) >= 100) {
                    calcDamage += calcDamage * 0.15;
                } else if (GET_SKILL(user, SKILL_KIBLAST) >= 60) {
                    calcDamage += calcDamage * 0.10;
                } else if (GET_SKILL(user, SKILL_KIBLAST) >= 40) {
                    calcDamage += calcDamage * 0.05;
                }
        }
    }

    void KiBlast::attackPostprocess() {
        int mastery = rand_number(1, 100), master_pass = false, chance = 0;

        if (initSkill >= 100)
            chance = 30;
        else if (initSkill >= 75)
            chance = 20;
        else if (initSkill >= 50)
            chance = 15;

        if (mastery <= chance)
            master_pass = true;

        if (master_pass == true && record > GET_HIT(victim) &&
            (record - GET_HIT(victim) > (victim->getEffMaxPL()) * 0.025)) {
            if (!AFF_FLAGGED(victim, AFF_KNOCKED) && !AFF_FLAGGED(victim, AFF_SANCTUARY)) {
                act("@C$N@W is knocked out!@n", true, user, nullptr, victim, TO_CHAR);
                act("@WYou are knocked out!@n", true, user, nullptr, victim, TO_VICT);
                act("@C$N@W is knocked out!@n", true, user, nullptr, victim, TO_NOTVICT);
                 victim->setStatusKnockedOut();
            }
        }
    }

    void KiBlast::announceHitspot() {
        switch(hitspot) {
            case 1:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S chest!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your chest!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's chest!@n");
                break;
            case 2:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S face!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your face!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's face!@n");
                break;
            case 3:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S gut!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your gut!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's gut!@n");
                break;
            case 4:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S arm!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your arm!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's arm!@n");
                break;
            case 5:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large kiblast that slams into $S leg!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into your leg!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large kiblast that slams into $N@W's leg!@n");
                break;
        }
    }


    // Beam
    void Beam::announceHitspot() {
        switch(hitspot) {
            case 1:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S chest!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your chest!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's chest!@n");
                break;
            case 2:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S face!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your face!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's face!@n");
                break;
            case 3:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S gut!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your gut!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's gut!@n");
                break;
            case 4:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S arm!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your arm!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's arm!@n");
                break;
            case 5:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large beam that slams into $S leg!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into your leg!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large beam that slams into $N@W's leg!@n");
                break;
        }
    }

    void Beam::attackPostprocess() {
        int master_roll = rand_number(1, 100), master_chance = 0, master_pass = false;

        if (initSkill >= 100)
            master_chance = 20;
        else if (initSkill >= 75)
            master_chance = 10;
        else if (initSkill >= 50)
            master_chance = 5;

        if (master_chance >= master_roll)
            master_pass = true;

        if (GET_HIT(victim) > 0 && calcDamage > GET_MAX_HIT(victim) / 4 && master_pass == true) {
            int attempt = rand_number(0, NUM_OF_DIRS);  /* Select a random direction */
            int count = 0;
            while (count < 12) {
                attempt = count;
                if (CAN_GO(victim, attempt)) {
                    count = 12;
                } else {
                    count++;
                }
            }
            if (CAN_GO(victim, attempt)) {
                act("$N@W is pushed away by the blast!@n", true, user, nullptr, victim, TO_CHAR);
                act("@WYou are pushed away by the blast!@n", true, user, nullptr, victim, TO_VICT);
                act("$N@W is pushed away by the blast!@n", true, user, nullptr, victim, TO_NOTVICT);
                do_simple_move(victim, attempt, true);
            } else {
                act("$N@W is pushed away by the blast, but is slammed into an obstruction!@n", true, user, nullptr,
                    victim,
                    TO_CHAR);
                act("@WYou are pushed away by the blast, but are slammed into an obstruction!@n", true, user, nullptr,
                    victim,
                    TO_VICT);
                act("$N@W is pushed away by the blast, but is slammed into an obstruction!@n", true, user, nullptr,
                    victim,
                    TO_NOTVICT);
                calcDamage *= 2;
                hurt(1, 195, user, victim, nullptr, calcDamage, 1);
            }
        }
    }


    // Tsuihidan
    void Tsuihidan::announceHitspot() {
        switch(hitspot) {
            case 1:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $s chest!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your chest!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's chest!@n");
                break;
            case 2:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $S face!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your face!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's face!@n");
                break;
            case 3:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $S gut!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your gut!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's gut!@n");
                break;
            case 4:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $S arm!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your arm!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's arm!@n");
                break;
            case 5:
                actUser("@WYou aim your hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly you unleash a large tsuihidan that slams into $S leg!@n");
                actVictim("@W$n@W aims $s hand at you, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into your leg!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and bright @Yyellow@W energy begins to pool there. Suddenly $e unleashes a large tsuihidan that slams into $N@W's leg!@n");
                break;
        }
    }

    void Tsuihidan::attackPostprocess() {
        int master_roll = rand_number(1, 100), master_chance = 0, master_pass = false;

        if (initSkill >= 100)
            master_chance = 20;
        else if (initSkill >= 75)
            master_chance = 10;
        else if (initSkill >= 50)
            master_chance = 5;

        if (master_chance >= master_roll)
            master_pass = true;

        if (master_pass == true) {
            victim->decCurST(calcDamage);
            act("@CYour tsuihidan hits a vital spot and seems to sap some of @c$N's@C stamina!@n", true, user,
                nullptr, victim, TO_CHAR);
            act("@C$n's@C tsuihidan hits a vital spot and saps some of your stamina!@n", true, user, nullptr, victim,
                TO_VICT);
            act("@C$n's@C tsuihidan hits a vital spot and saps some of @c$N's@C stamina!", true, user, nullptr, victim,
                TO_NOTVICT);
        }
    }

    // Renzo

    DefenseResult Renzo::calculateDefense() {
        count = 100;

        if (IS_NAIL(user)) {
            if (initSkill >= 100) {
                count += 200;
            } else if (initSkill >= 60) {
                count += 100;
            } else if (initSkill >= 40) {
                count += 40;
            }
        }
        if (rand_number(1, 5) >= 5) { /* Random boost or neg for everyone */
            count += rand_number(-15, 25);
        }



        double dodgeChance = ((double) currentDodgeCheck / 5.0) * ((double) axion_dice(0) / 120.0) * (1.0 + victim->getAffectModifier(APPLY_DODGE_PERC));
        double blockChance = ((double) currentBlockCheck / 3.0) * ((double) axion_dice(0) / 120.0) * (1.0 + victim->getAffectModifier(APPLY_BLOCK_PERC));

        double overcomeDodge = (double) currentChanceToHit * ((double) axion_dice(0) / 120.0);
        double overcomeBlock = (double) currentChanceToHit * ((double) axion_dice(0) / 120.0);
        

        if(canParry() && calculateDeflect()) count -= axion_dice(0);
        if(count <= 0) return DefenseResult::Parried;
        if(canDodge() && dodgeChance > overcomeDodge) count -= axion_dice(0);
        if(count <= 0) return DefenseResult::Dodged;
        if(canBlock() && blockChance > overcomeBlock) return DefenseResult::Blocked;

        return DefenseResult::Failed;
    }

    double Renzo::getKiEfficiency() {
        int master_roll = rand_number(1, 100), master_chance = 0, half_chance = 0, master_pass = 0;

        if (initSkill >= 100) {
            master_chance = 10;
            half_chance = 20;
        } else if (initSkill >= 75) {
            master_chance = 5;
            half_chance = 10;
        } else if (initSkill >= 50) {
            master_chance = 5;
            half_chance = 5;
        }

        if (master_chance >= master_roll)
            master_pass = 1;
        else if (half_chance >= master_roll)
            master_pass = 2;

        if (master_pass == 1) {
            send_to_char(user, "@GYour mastery of the technique has made your use of energy more efficient!@n\r\n");
            return 0.25;
        }
        else if (master_pass == 2) {
            send_to_char(user,
                "@GYour mastery of the technique has made your use of energy as efficient as possible!@n\r\n");
            return 0.5;
        }

        return 1.0;
    }

    void Renzo::attackPreprocess() {
        calcDamage = calcDamage * 0.01;
        calcDamage *= count;
        
    }

    void Renzo::handleHitspot() {
        hitspot = 1;
    }

    void Renzo::announceHitspot() {
        if (count >= 100) {
            actUser("@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! All of the shots hit!");
            actVictim("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! All of the shots hit!");
            actOthers("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! All of the shots hit!");
        }
        if (count >= 75 && count < 100) {
            actUser("@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Most of the shots hit, but some of them are avoided by $M!");
            actVictim("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! Most of the shots hit, but some of them you manage to avoid!");
            actOthers("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Most of the shots hit, but some of them are avoided by $M!");
        }
        if (count >= 50 && count < 75) {
            actUser("@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! About half of the shots hit, the rest are avoided by $M!");
            actVictim("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! About half of the shots hit, the rest you manage to avoid!");
            actOthers("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! About half of the shots hit, the rest are avoided by $M!");
        }
        if (count >= 25 && count < 50) {
            actUser("@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Few of the shots hit, the rest are avoided by $M!");
            actVictim("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! Few of the shots hit, the rest you manage to avoid!");
            actOthers("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Few of the shots hit, the rest are avoided by $M!");
        }
        if (count < 25) {
            actUser("@WYou gather your charged energy into your hands as a golden glow appears around each. You slam your hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Very few of the shots hit, the rest are avoided by $M!");
            actVictim("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at you! Very few of the shots hit, the rest you manage to avoid!");
            actOthers("@w$n gathers charged energy into $s hands as a golden glow appears around each. $e slams $s hands forward rapidly firing hundreds of Renzokou Energy Dan shots at $N@W! Very few of the shots hit, the rest are avoided by $M!");
        }
        
    }


    // Shogekiha
    void Shogekiha::attackPostprocess() {
        int master_roll = rand_number(1, 100), master_chance = 0, master_pass = false;

        if (initSkill >= 100)
            master_chance = 20;
        else if (initSkill >= 75)
            master_chance = 10;
        else if (initSkill >= 50)
            master_chance = 5;

        if (master_chance >= master_roll)
            master_pass = true;

        if (master_pass == true) {
            act("@CYour skillful shogekiha dissipated some of @c$N's@C charged ki!@n", true, user, nullptr, victim,
                TO_CHAR);
            act("@C$n@C's skillful shogekiha dissipated some of YOUR charged ki!@n", true, user, nullptr, victim,
                TO_VICT);
            act("@C$n@C's skillful shogekiha dissipated some of @c$N's@C charged ki!", true, user, nullptr, victim,
                TO_NOTVICT);
            GET_CHARGE(victim) -= GET_CHARGE(victim) * 0.25;
        }
    }


    void Shogekiha::announceHitspot() {
        switch(hitspot) {
            case 1:
                actUser("@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S chest!@n");
                actVictim("@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your chest!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's chest!@n");
                break;
            case 2:
                actUser("@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S face!@n");
                actVictim("@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your face!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's face!@n");
                break;
            case 3:
                actUser("@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S gut!@n");
                actVictim("@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your gut!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's gut!@n");
                break;
            case 4:
                actUser("@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S arm!@n");
                actVictim("@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your arm!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's arm!@n");
                break;
            case 5:
                actUser("@WYou aim your hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly you unleash a large shogekiha that slams into $S leg!@n");
                actVictim("@W$n@W aims $s hand at you, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into your leg!@n");
                actOthers("@W$n@W aims $s hand at $N@W, and nearby loose objects begin to be pushed out by an invisible force. Suddenly $e unleashes a large shogekiha that slams into $N@W's leg!@n");
                break;
        }
    }

    Result Shogekiha::attackObject() {
        if(!can_kill(user, nullptr, obj, canKillType())) return Result::Canceled;
        bool extracted = false;
        calcDamage = calculateObjectDamage();

        if (KICHARGE(obj) > 0 && GET_CHARGE(user) > KICHARGE(obj)) {
            KICHARGE(obj) -= GET_CHARGE(user);
            extract_obj(obj);
            extracted = true;
        } else if (KICHARGE(obj) > 0 && GET_CHARGE(user) < KICHARGE(obj)) {
            KICHARGE(obj) -= GET_CHARGE(user);
            GET_CHARGE(user) = 0;
            calcDamage = KICHARGE(obj);
            hurt(0, 0, USER(obj), user, nullptr, calcDamage, 0);
            extract_obj(obj);
            extracted = true;
        }

        announceObject();
        if(extracted) return Result::Canceled;
        return Result::Landed;
    }

    void Shogekiha::announceObject() {
        if (KICHARGE(obj) > 0 && GET_CHARGE(user) > KICHARGE(obj)) {
            actUser("@WYou leap at $p@W with your arms spread out to your sides. As you are about to make contact with $p@W you scream and shatter the attack with your ki!@n");
            actRoom("@C$n@W leaps out at $p@W with $s arms spead out to $s sides. As $e is about to make contact with $p@W $e screams and shatters the attack with $s ki!@n");
        } else if (KICHARGE(obj) > 0 && GET_CHARGE(user) < KICHARGE(obj)) {
            actUser("@WYou leap at $p@W with your arms spread out to your sides. As you are about to make contact with $p@W you scream and weaken the attack with your ki before taking the rest of the attack in the chest!@n");
            actRoom("@C$n@W leaps out at $p@W with $s arms spead out to $s sides. As $e is about to make contact with $p@W $e screams and weakens the attack with $s ki before taking the rest of the attack in the chest!@n");
        } else {
            actUser("@WYou fire a shogekiha at $p@W!@n");
            actRoom("@C$n@W fires a shogekiha at $p@W!@n");
        }
    }

}