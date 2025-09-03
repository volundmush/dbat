#pragma once
#include <cstdint>

#include <magic_enum/magic_enum.hpp>


using attribute_t = std::uint8_t;
enum class CharAttribute : std::uint8_t
{
    strength = 1 << 0,     // 1
    agility = 1 << 1,      // 2
    intelligence = 1 << 2, // 4
    wisdom = 1 << 3,       // 8
    constitution = 1 << 4, // 16
    speed = 1 << 5         // 32
};

template <>
struct magic_enum::customize::enum_range<CharAttribute>
{
    static constexpr bool is_flags = true;
};

using attribute_train_t = std::uint32_t;
enum class CharTrain : std::uint8_t
{
    strength = 1 << 0,     // 1
    agility = 1 << 1,      // 2
    intelligence = 1 << 2, // 4
    wisdom = 1 << 3,       // 8
    constitution = 1 << 4, // 16
    speed = 1 << 5         // 32
};

using align_t = std::int16_t;
enum class CharAlign : std::uint8_t
{
    good_evil = 1 << 0,
    law_chaos = 1 << 1,
};

enum class MoralAlign : std::uint8_t
{
    evil = 0,
    neutral = 1,
    good = 2
};

using money_t = std::uint64_t;
enum class CharMoney : std::uint8_t
{
    carried = 1 << 0,
    bank = 1 << 1
};

enum class CharVital : std::uint8_t
{
    health = 1 << 0,
    ki = 1 << 1,
    stamina = 1 << 2,
    lifeforce = 1 << 3
};

enum class CharStat : std::uint8_t
{
    experience = 1 << 0,
    skill_train = 1 << 1,
    practices = 1 << 2,
    upgrade_points = 1 << 3
};

using dim_t = double;
enum class CharDim : std::uint8_t
{
    height = 1 << 0,
    weight = 1 << 1,
};

using weight_t = dim_t;
using effect_t = std::uint16_t;

enum class ComStat : std::uint8_t
{
    accuracy = 1 << 0,
    damage = 1 << 1,
    armor = 1 << 2,
    parry = 1 << 3,
    dodge = 1 << 4,
    block = 1 << 5,
    perfect_dodge = 1 << 6,
    defense = 1 << 7,
};

enum class AtkTier : std::uint8_t
{
    one = 1 << 0,
    two = 1 << 1,
    three = 1 << 2,
    four = 1 << 3,
    five = 1 << 4,
    six = 1 << 5
};

enum class DamType : std::uint8_t
{
    physical = 1 << 0,
    ki = 1 << 1,
};
