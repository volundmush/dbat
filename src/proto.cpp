#include "dbat/structs.h"
#include "dbat/genolc.h"

proto_data::~proto_data() {
    if (name) free(name);
    if (room_description) free(room_description);
    if (look_description) free(look_description);
    if (short_description) free(short_description);
    if(ex_description) free_ex_descriptions(ex_description);
}

proto_data& proto_data::operator=(const proto_data& other) {
    if (this == &other) return *this; // self-assignment check

    // basic proto data fields
    vn = other.vn;
    if (name) free(name);
    if (room_description) free(room_description);
    if (look_description) free(look_description);
    if (short_description) free(short_description);
    if (ex_description) free_ex_descriptions(ex_description);

    name = other.name ? strdup(other.name) : nullptr;
    room_description = other.room_description ? strdup(other.room_description) : nullptr;
    look_description = other.look_description ? strdup(other.look_description) : nullptr;
    short_description = other.short_description ? strdup(other.short_description) : nullptr;
    if(other.ex_description) {
        ex_description = nullptr;
        copy_ex_descriptions(&ex_description, other.ex_description);
    } else {
        ex_description = nullptr;
    }
    affect_flags = other.affect_flags;
    stats = other.stats;

    return *this;
}

item_proto_data& item_proto_data::operator=(item_proto_data& other) {
    // basic proto data fields
    proto_data::operator=(other);
    type_flag = other.type_flag;
    affected = other.affected;
    wear_flags = other.wear_flags;
    item_flags = other.item_flags;
    size = other.size;

    return *this;
}

item_proto_data::item_proto_data(obj_data& other) {
    name = strdup(other.getName());
    room_description = strdup(other.getRoomDescription());
    look_description = strdup(other.getLookDescription());
    short_description = strdup(other.getShortDescription());
    if(other.ex_description) {
        copy_ex_descriptions(&ex_description, other.ex_description);
    }
    stats = other.stats;
    affect_flags = other.affect_flags;

    type_flag = other.type_flag;
    affected = other.affected;
    wear_flags = other.wear_flags;
    item_flags = other.item_flags;
    size = other.size;

}

obj_data& obj_data::operator=(item_proto_data& other) {
    // basic proto data fields
    proto = &other;
    affect_flags = other.affect_flags;
    stats = other.stats;

    // item proto data fields
    type_flag = other.type_flag;
    affected = other.affected;
    wear_flags = other.wear_flags;
    item_flags = other.item_flags;
    size = other.size;
    
    return *this;
}

void obj_data::commit_iedit(const item_proto_data &other) {
    if(name) free(name);
    if(room_description) free(room_description);
    if(look_description) free(look_description);
    if(short_description) free(short_description);
    if(ex_description) free_ex_descriptions(ex_description);

    name = strdup(other.name);
    room_description = strdup(other.room_description);
    look_description = strdup(other.look_description);
    short_description = strdup(other.short_description);

    // run de-duplicate logic.
    if(proto) {
        if(name && proto->name && !strcmp(name, proto->name)) {
            free(name);
        }
        if(room_description && proto->room_description && !strcmp(room_description, proto->room_description)) {
            free(room_description);
        }
        if(look_description && proto->look_description && !strcmp(look_description, proto->look_description)) {
            free(look_description);
        }
        if(short_description && proto->short_description && !strcmp(short_description, proto->short_description)) {
            free(short_description);
        }
    }

    if(other.ex_description) {
        copy_ex_descriptions(&ex_description, other.ex_description);
    }

    if(other.ex_description) {
        ex_description = nullptr;
        copy_ex_descriptions(&ex_description, other.ex_description);
    } else {
        ex_description = nullptr;
    }

    stats = other.stats;
    affect_flags = other.affect_flags;

    type_flag = other.type_flag;
    affected = other.affected;
    wear_flags = other.wear_flags;
    item_flags = other.item_flags;
    size = other.size;

    // Set the unique save flag
    item_flags.set(ITEM_UNIQUE_SAVE);
}

char_data& char_data::operator=(npc_proto_data& other) {
    // basic proto data fields
    proto = &other;
    affect_flags = other.affect_flags;
    stats = other.stats;

    // item proto data fields
    race = other.race;
    subrace = other.subrace;
    sensei = other.sensei;
    sex = other.sex;
    mob_specials = other.mob_specials;
    character_flags = other.character_flags;
    mob_flags = other.mob_flags;
    bio_genomes = other.bio_genomes;
    mutations = other.mutations;
    clan = other.clan ? strdup(other.clan) : nullptr;
    crank = other.crank;
    size = other.size;
    
    return *this;
}