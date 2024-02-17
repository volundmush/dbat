#include "dbat/skill.h"

namespace skill {
    static const std::unordered_map<SkillID, std::string> skillNames = {
            {SkillID::Flex, "Flex"},
            {SkillID::Genius, "Genius"},
            {SkillID::SolarFlare, "Solar Flare"},
            {SkillID::Might, "Might"},
            {SkillID::Balance, "Balance"},
            {SkillID::Build, "Build"},
            {SkillID::ToughSkin, "Tough Skin"},
            {SkillID::Concentration, "Concentration"},
            {SkillID::Kaioken, "Kaioken"},
            {SkillID::Spot, "Spot"},
            {SkillID::FirstAid, "First Aid"},
            {SkillID::Disguise, "Disguise"},
            {SkillID::EscapeArtist, "Escape Artist"},
            {SkillID::Appraise, "Appraise"},
            {SkillID::Heal, "Heal"},
            {SkillID::Forgery, "Forgery"},
            {SkillID::Hide, "Hide"},
            {SkillID::Bless, "Bless"},
            {SkillID::Curse, "Curse"},
            {SkillID::Listen, "Listen"},
            {SkillID::Eavesdrop, "Eavesdrop"},
            {SkillID::Poison, "Poison"},
            {SkillID::CurePoison, "Cure Poison"},
            {SkillID::OpenLock, "Open Lock"},
            {SkillID::Vigor, "Vigor"},
            {SkillID::Regenerate, "Regenerate"},
            {SkillID::KeenSight, "Keen Sight"},
            {SkillID::Search, "Search"},
            {SkillID::MoveSilently, "Move Silently"},
            {SkillID::Absorb, "Absorb"},
            {SkillID::SleightOfHand, "Sleight of Hand"},
            {SkillID::Ingest, "Ingest"},
            {SkillID::Repair, "Repair"},
            {SkillID::Sense, "Sense"},
            {SkillID::Survival, "Survival"},
            {SkillID::Yoikominminken, "Yoikominminken"},
            {SkillID::Create, "Create"},
            {SkillID::StoneSpit, "Stone Spit"},
            {SkillID::PotentialRelease, "Potential Release"},
            {SkillID::Telepathy, "Telepathy"},
            {SkillID::RenzokouEnergyDan, "Renzokou Energy Dan"},
            {SkillID::Masenko, "Masenko"},
            {SkillID::Dodonpa, "Dodonpa"},
            {SkillID::Barrier, "Barrier"},
            {SkillID::GalikGun, "Galik Gun"},
            {SkillID::Throw, "Throw"},
            {SkillID::Dodge, "Dodge"},
            {SkillID::Parry, "Parry"},
            {SkillID::Block, "Block"},
            {SkillID::Punch, "Punch"},
            {SkillID::Kick, "Kick"},
            {SkillID::Elbow, "Elbow"},
            {SkillID::Knee, "Knee"},
            {SkillID::Roundhouse, "Roundhouse"},
            {SkillID::Uppercut, "Uppercut"},
            {SkillID::Slam, "Slam"},
            {SkillID::Heeldrop, "Heeldrop"},
            {SkillID::Focus, "Focus"},
            {SkillID::KiBall, "Ki Ball"},
            {SkillID::KiBlast, "Ki Blast"},
            {SkillID::Beam, "Beam"},
            {SkillID::Tsuihidan, "Tsuihidan"},
            {SkillID::Shogekiha, "Shogekiha"},
            {SkillID::Zanzoken, "Zanzoken"},
            {SkillID::KameHameHa, "KameHameHa"},
            {SkillID::Dagger, "Dagger"},
            {SkillID::Sword, "Sword"},
            {SkillID::Club, "Club"},
            {SkillID::Spear, "Spear"},
            {SkillID::Gun, "Gun"},
            {SkillID::Brawl, "Brawl"},
            {SkillID::InstantTransmission, "Instant Transmission"},
            {SkillID::Deathbeam, "Deathbeam"},
            {SkillID::Eraser, "Eraser"},
            {SkillID::TwinSlash, "Twin Slash"},
            {SkillID::Psyblast, "Psyblast"},
            {SkillID::Honoo, "Honoo"},
            {SkillID::Dualbeam, "Dualbeam"},
            {SkillID::Rogafufuken, "Rogafufuken"},
            {SkillID::SpecialPose, "Special Pose"},
            {SkillID::Bakuhatsuha, "Bakuhatsuha"},
            {SkillID::Kienzan, "Kienzan"},
            {SkillID::Tribeam, "Tribeam"},
            {SkillID::SpecialBeamCannon, "Special Beam Cannon"},
            {SkillID::FinalFlash, "Final Flash"},
            {SkillID::Crusher, "Crusher"},
            {SkillID::DarknessDragonSlash, "Darkness Dragon Slash"},
            {SkillID::PsychicBarrage, "Psychic Barrage"},
            {SkillID::Hellflash, "Hellflash"},
            {SkillID::HellSpearBlast, "Hell Spear Blast"},
            {SkillID::Kakusanha, "Kakusanha"},
            {SkillID::Hasshuken, "Hasshuken"},
            {SkillID::Scatter, "Scatter"},
            {SkillID::BigBang, "Big Bang"},
            {SkillID::PhoenixSlash, "Phoenix Slash"},
            {SkillID::Deathball, "Deathball"},
            {SkillID::SpiritBall, "Spirit Ball"},
            {SkillID::Genkidama, "Genkidama"},
            {SkillID::Genocide, "Genocide"},
            {SkillID::Dualwield, "Dualwield"},
            {SkillID::KuraiiroSeiki, "Kuraiiro Seiki"},
            {SkillID::Tailwhip, "Tailwhip"},
            {SkillID::Kousengan, "Kousengan"},
            {SkillID::TaishaReiki, "Taisha Reiki"},
            {SkillID::Paralyze, "Paralyze"},
            {SkillID::Infuse, "Infuse"},
            {SkillID::Roll, "Roll"},
            {SkillID::Trip, "Trip"},
            {SkillID::Grapple, "Grapple"},
            {SkillID::WaterSpike, "Water Spike"},
            {SkillID::SelfDestruct, "Self Destruct"},
            {SkillID::SpiralComet, "Spiral Comet"},
            {SkillID::StarBreaker, "Star Breaker"},
            {SkillID::Enlighten, "Enlighten"},
            {SkillID::Commune, "Commune"},
            {SkillID::Mimic, "Mimic"},
            {SkillID::WaterRazor, "Water Razor"},
            {SkillID::KoteiruBakuha, "Koteiru Bakuha"},
            {SkillID::DimizuToride, "Dimizu Toride"},
            {SkillID::HyogaKabe, "Hyoga Kabe"},
            {SkillID::Wellspring, "Wellspring"},
            {SkillID::AquaBarrier, "Aqua Barrier"},
            {SkillID::WarpPool, "Warp Pool"},
            {SkillID::HellSpiral, "Hell Spiral"},
            {SkillID::NaniteArmor, "Nanite Armor"},
            {SkillID::Fireshield, "Fireshield"},
            {SkillID::Cooking, "Cooking"},
            {SkillID::SeishouEnko, "Seishou Enko"},
            {SkillID::Silk, "Silk"},
            {SkillID::Bash, "Bash"},
            {SkillID::Headbutt, "Headbutt"},
            {SkillID::Ensnare, "Ensnare"},
            {SkillID::Starnova, "Starnova"},
            {SkillID::Pursuit, "Pursuit"},
            {SkillID::ZenBladeStrike, "Zen Blade Strike"},
            {SkillID::SunderingForce, "Sundering Force"},
            {SkillID::Wither, "Wither"},
            {SkillID::Twohand, "Twohand"},
            {SkillID::FightingArts, "Fighting Arts"},
            {SkillID::Metamorph, "Metamorph"},
            {SkillID::HealingGlow, "Healing Glow"},
            {SkillID::Runic, "Runic"},
            {SkillID::Extract, "Extract"},
            {SkillID::Gardening, "Gardening"},
            {SkillID::EnergizeThrowing, "Energize Throwing"},
            {SkillID::MaliceBreaker, "Malice Breaker"},
            {SkillID::Hayasa, "Hayasa"},
            {SkillID::Handling, "Handling"},
            {SkillID::MysticMusic, "Mystic Music"},
            {SkillID::LightGrenade, "Light Grenade"},
            {SkillID::Multiform, "Multiform"},
            {SkillID::SpiritControl, "Spirit Control"},
            {SkillID::Balefire, "Balefire"},
            {SkillID::BlessedHammer, "Blessed Hammer"}
    };

    std::string getName(SkillID skill) {
        if(auto found = skillNames.find(skill); found != skillNames.end()) {
            return found->second;
        }
        return "NOTFOUND";
    }

    struct skill_affect_type {
        int location{};
        double modifier{0.0};
        int specific{-1};
        std::function<double(BaseCharacter *ch)> func{};
    };

    std::unordered_map<SkillID, std::vector<skill_affect_type>> skillAffects = {};

    double getModifier(BaseCharacter* ch, SkillID skill, int location, int specific) {
        double out = 0.0;
        if (auto found = skillAffects.find(skill); found != skillAffects.end()) {
            for (auto& affect: found->second) {
                if (affect.location == location) {if(specific != -1 && specific != affect.specific) continue;
                    out += affect.modifier;
                    if(affect.func) {
                        out += affect.func(ch);
                    }
                }
            }

        }

        return out;
    }

    double getModifiers(BaseCharacter* ch, int location, int specific) {
        double out = 0.0;
        for(auto &[id, data] : ch->skill) {
            if(data.level > 0)
                out += getModifier(ch, static_cast<SkillID>(id), location, specific);
        }
        return out;
    }

}