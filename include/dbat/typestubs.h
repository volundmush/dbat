#pragma once
struct GameEntity;
struct zone_data;
struct reset_com;
struct BaseCharacter;
struct Object;
struct Room;
struct Exit;
struct shop_buy_data;
struct command_info;
struct descriptor_data;
struct account_data;
struct player_data;
struct trig_data;
struct trans_data;
struct mob_special_data;
struct Shop;
struct Guild;

struct memory_rec_struct {
    int id;
    struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;

namespace race {
    class Race;

    struct transform_bonus;
}

namespace sensei {
    class Sensei;
}

namespace net {
    class Connection;
}