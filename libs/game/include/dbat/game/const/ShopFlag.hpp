#pragma once
#include <cstdint>

enum class ShopFlag : std::uint8_t
{
    start_fight = 0,
    bank_money = 1,
    allow_steal = 2,
    no_broken = 3
};