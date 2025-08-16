#pragma once
struct Zone;
struct reset_com;
struct Character;
struct Object;
struct Room;
struct AbstractLocation;
struct shop_buy_data;
struct command_info;
struct descriptor_data;
struct Account;
struct PlayerData;
struct ThingPrototype;
struct CharacterPrototype;
struct ObjectPrototype;
struct DgScript;
struct DgScriptPrototype;
struct Destination;
struct HasDgScripts;
struct HasMudStrings;
struct HasExtraDescriptions;
struct HasLocation;
struct HasInventory;
struct HasEquipment;
struct Area;
struct Structure;
struct Location;

typedef HasDgScripts script_data;

namespace atk {
    class Attack;
}