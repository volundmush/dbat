#include "dbat/structs.h"
#include "dbat/genolc.h"

ThingPrototype::~ThingPrototype()
{
    if (name)
        free(name);
    if (room_description)
        free(room_description);
    if (look_description)
        free(look_description);
    if (short_description)
        free(short_description);
    if (ex_description)
        free_ex_descriptions(ex_description);
}

ThingPrototype &ThingPrototype::operator=(const ThingPrototype &other)
{
    if (this == &other)
        return *this; // self-assignment check

    // basic proto data fields
    vn = other.vn;
    if (name)
        free(name);
    if (room_description)
        free(room_description);
    if (look_description)
        free(look_description);
    if (short_description)
        free(short_description);
    if (ex_description)
        free_ex_descriptions(ex_description);

    name = other.name ? strdup(other.name) : nullptr;
    room_description = other.room_description ? strdup(other.room_description) : nullptr;
    look_description = other.look_description ? strdup(other.look_description) : nullptr;
    short_description = other.short_description ? strdup(other.short_description) : nullptr;
    if (other.ex_description)
    {
        ex_description = nullptr;
        copy_ex_descriptions(&ex_description, other.ex_description);
    }
    else
    {
        ex_description = nullptr;
    }
    affect_flags = other.affect_flags;
    stats = other.stats;

    return *this;
}

ObjectPrototype &ObjectPrototype::operator=(const ObjectPrototype &other)
{
    // basic proto data fields
    ThingPrototype::operator=(other);
    type_flag = other.type_flag;
    affected = other.affected;
    wear_flags = other.wear_flags;
    item_flags = other.item_flags;
    size = other.size;

    return *this;
}

ObjectPrototype::ObjectPrototype(const Object &other)
{
    name = strdup(other.getName());
    room_description = strdup(other.getRoomDescription());
    look_description = strdup(other.getLookDescription());
    short_description = strdup(other.getShortDescription());
    if (!other.extra_descriptions.empty())
    {
        for (const auto &ex : other.extra_descriptions)
        {
            auto e = new extra_descr_data();
            e->keyword = strdup(ex.keyword.c_str());
            e->description = strdup(ex.description.c_str());
            e->next = ex_description;
            ex_description = e;
        }
    }
    stats = other.stats;
    affect_flags = other.affect_flags;

    type_flag = other.type_flag;
    affected = other.affected;
    wear_flags = other.wear_flags;
    item_flags = other.item_flags;
    size = other.size;
}

Object &Object::operator=(const ObjectPrototype &other)
{
    // basic proto data fields
    vn = other.vn;
    if (other.name)
        strings["name"] = other.name;
    if (other.room_description)
        strings["room_description"] = other.room_description;
    if (other.look_description)
        strings["look_description"] = other.look_description;
    if (other.short_description)
        strings["short_description"] = other.short_description;
    affect_flags = other.affect_flags;
    stats = other.stats;
    extra_descriptions.clear();
    if (other.ex_description)
    {
        for (auto e = other.ex_description; e; e = e->next)
        {
            auto &ex = extra_descriptions.emplace_back();
            ex.keyword = e->keyword;
            ex.description = e->description;
        }
    }

    // item proto data fields
    type_flag = other.type_flag;
    affected = other.affected;
    wear_flags = other.wear_flags;
    item_flags = other.item_flags;
    size = other.size;

    return *this;
}

void Object::commit_iedit(const ObjectPrototype &other)
{
    operator=(other);

    // Set the unique save flag
    item_flags.set(ITEM_UNIQUE_SAVE);
}

Character &Character::operator=(CharacterPrototype &other)
{
    // basic proto data fields
    vn = other.vn;
    if (other.name)
        strings["name"] = other.name;
    if (other.room_description)
        strings["room_description"] = other.room_description;
    if (other.look_description)
        strings["look_description"] = other.look_description;
    if (other.short_description)
        strings["short_description"] = other.short_description;
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

std::string ThingPrototype::scriptString() const
{
    std::vector<std::string> vnums;
    for (auto p : proto_script)
        vnums.emplace_back(std::move(std::to_string(p)));

    return fmt::format("@D[@wT{}@D]@n", fmt::join(vnums, ","));
}