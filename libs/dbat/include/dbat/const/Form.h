#pragma once
#include <cstdint>

enum class Form : std::uint16_t
{
    // Universal,
    base = 0,
    custom_1 = 1,
    custom_2 = 2,
    custom_3 = 3,
    custom_4 = 4,
    custom_5 = 5,
    custom_6 = 6,
    custom_7 = 7,
    custom_8 = 8,
    custom_9 = 9,

    // Saiyan'y forms.
    oozaru = 10,
    golden_oozaru = 11,
    super_saiyan_1 = 12,
    super_saiyan_2 = 13,
    super_saiyan_3 = 14,
    super_saiyan_4 = 15,
    super_saiyan_god = 16,
    super_saiyan_blue = 17,

    // LSSJ
    ikari = 20,
    legendary_saiyan = 21,

    // Human'y Forms
    super_human_1 = 30,
    super_human_2 = 31,
    super_human_3 = 32,
    super_human_4 = 33,

    // Icer'y Forms
    icer_1 = 40,
    icer_2 = 41,
    icer_3 = 42,
    icer_4 = 43,
    icer_metal = 44,
    icer_golden = 45,
    icer_black = 46,

    // Konatsu
    shadow_first = 50,
    shadow_second = 51,
    shadow_third = 52,

    // Lycanthrope
    lesser_lycanthrope = 60,
    lycanthrope = 61,
    alpha_lycanthrope = 62,

    // Namekian
    super_namekian_1 = 70,
    super_namekian_2 = 71,
    super_namekian_3 = 72,
    super_namekian_4 = 73,

    // Mutant
    mutate_1 = 80,
    mutate_2 = 81,
    mutate_3 = 82,

    // BioAndroid
    bio_mature = 90,
    bio_semi_perfect = 91,
    bio_perfect = 92,
    bio_super_perfect = 93,

    // Android
    android_1 = 100,
    android_2 = 101,
    android_3 = 102,
    android_4 = 103,
    android_5 = 104,
    android_6 = 105,

    // Majin
    maj_affinity = 110,
    maj_super = 111,
    maj_true = 112,

    // Kai
    mystic_1 = 120,
    mystic_2 = 121,
    mystic_3 = 123,

    // Kai Alt
    divine_halo = 126,

    // Tuffle
    ascend_1 = 130,
    ascend_2 = 131,
    ascend_3 = 132,

    // Demon
    dark_king = 140,

    // Alternate Unbound Forms
    potential_unleashed = 180,
    evil_aura = 181,
    ultra_instinct = 182,

    // Unbound Perm Forms
    potential_unlocked = 210,
    potential_unlocked_max = 211,
    majinized = 212,
    divine_water = 213,

    // Techniques
    kaioken = 300,
    dark_metamorphosis = 301,

    tiger_stance = 302,
    eagle_stance = 303,
    ox_stance = 304,

    spirit_absorption = 305,

    // Hoshijin Shit
    death_phase = 306,
    birth_phase = 307,
    life_phase = 308

};
