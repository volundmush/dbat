#pragma once
#include <vector>
#include <string>
#include <expected>
#include "dbat/game/Typedefs.hpp"

struct Character;

struct SqlIngredient {
    int oproto_id;
    int quantity;
    bool consumed;
    bool in_room;
};

struct SqlRecipe {
    std::string id;
    std::string name;
    std::string assembly_type;
    int oproto_id;
    std::vector<SqlIngredient> ingredients;
};

std::expected<SqlRecipe, std::string> sqlFindRecipeByName(const std::string& name);

std::expected<SqlRecipe, std::string> sqlFindRecipeByVnum(vnum v);

std::expected<void, std::string> sqlCheckComponents(const SqlRecipe& recipe, Character* ch, bool extract);

void sqlListAssemblies(Character* ch, int type);