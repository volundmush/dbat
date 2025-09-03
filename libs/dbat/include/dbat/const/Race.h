#pragma once
#include <cstdint>

enum class Race : std::uint8_t
{
    spirit = 0,
    human = 1,
    saiyan = 2,
    icer = 3,
    konatsu = 4,
    namekian = 5,
    mutant = 6,
    kanassan = 7,
    halfbreed = 8,
    bio_android = 9,
    android = 10,
    demon = 11,
    majin = 12,
    kai = 13,
    tuffle = 14,
    hoshijin = 15,
    animal = 16,
    saiba = 17,
    serpent = 18,
    ogre = 19,
    yardratian = 20,
    arlian = 21,
    dragon = 22,
    mechanical = 23,
};

enum class SubRace : std::uint8_t
{
    android_model_absorb = 0,
    android_model_repair = 1,
    android_model_sense = 2
};

/* Races */
constexpr Race RACE_HUMAN = Race::human;
constexpr Race RACE_SAIYAN = Race::saiyan;
constexpr Race RACE_ICER = Race::icer;
constexpr Race RACE_KONATSU = Race::konatsu;
constexpr Race RACE_NAMEK = Race::namekian;
constexpr Race RACE_MUTANT = Race::mutant;
constexpr Race RACE_KANASSAN = Race::kanassan;
constexpr Race RACE_HALFBREED = Race::halfbreed;
constexpr Race RACE_BIO = Race::bio_android;
constexpr Race RACE_ANDROID = Race::android;
constexpr Race RACE_DEMON = Race::demon;
constexpr Race RACE_MAJIN = Race::majin;
constexpr Race RACE_KAI = Race::kai;
constexpr Race RACE_TRUFFLE = Race::tuffle;
constexpr Race RACE_GOBLIN = Race::hoshijin;
constexpr Race RACE_ANIMAL = Race::animal;
constexpr Race RACE_SAIBA = Race::saiba;
constexpr Race RACE_SERPENT = Race::serpent;
constexpr Race RACE_OGRE = Race::ogre;
constexpr Race RACE_YARDRATIAN = Race::yardratian;
constexpr Race RACE_ARLIAN = Race::arlian;
constexpr Race RACE_DRAGON = Race::dragon;
constexpr Race RACE_MECHANICAL = Race::mechanical;
constexpr Race RACE_FAERIE = Race::spirit;


