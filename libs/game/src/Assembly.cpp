#include "dbat/game/Assembly.hpp"

#include "dbat/game/Database.hpp"
#include "dbat/game/Character.hpp"
#include "dbat/game/Object.hpp"
#include "dbat/game/ObjectPrototype.hpp"
#include "dbat/game/constants.hpp"
#include "dbat/game/handler.hpp"

std::expected<SqlRecipe, std::string> sqlFindRecipeByName(const std::string& name) {
    auto result = dbat::db::txn->exec(
        "SELECT r.id, r.name, r.assembly_type, r.oproto_id "
        "FROM dbat.assembly_recipes r "
        "JOIN dbat.oproto_blob o ON o.id = r.oproto_id "
        "WHERE o.data->>'name' ILIKE $1",
        pqxx::params{"%" + name + "%"}
    );
    
    if (result.empty()) {
        return std::unexpected(std::string("Recipe not found: ") + name);
    }
    
    SqlRecipe recipe;
    recipe.id = result[0][0].as<std::string>();
    recipe.name = result[0][1].as<std::string>();
    recipe.assembly_type = result[0][2].as<std::string>();
    recipe.oproto_id = result[0][3].as<int>();
    
    auto ingredients = dbat::db::txn->exec(
        "SELECT oproto_id, quantity, consumed, in_room FROM dbat.assembly_ingredients WHERE recipe_id = $1",
        pqxx::params{recipe.id}
    );
    
    for (const auto& row : ingredients) {
        SqlIngredient ing;
        ing.oproto_id = row[0].as<int>();
        ing.quantity = row[1].as<int>();
        ing.consumed = row[2].as<bool>();
        ing.in_room = row[3].as<bool>();
        recipe.ingredients.push_back(ing);
    }
    
    return recipe;
}

std::expected<SqlRecipe, std::string> sqlFindRecipeByVnum(vnum v) {
    auto result = dbat::db::txn->exec(
        "SELECT id, name, assembly_type, oproto_id FROM dbat.assembly_recipes WHERE oproto_id = $1",
        {static_cast<int>(v)}
    );
    
    if (result.empty()) {
        return std::unexpected(std::string("Recipe not found for vnum: ") + std::to_string(v));
    }
    
    SqlRecipe recipe;
    recipe.id = result[0][0].as<std::string>();
    recipe.name = result[0][1].as<std::string>();
    recipe.assembly_type = result[0][2].as<std::string>();
    recipe.oproto_id = result[0][3].as<int>();
    
    auto ingredients = dbat::db::txn->exec(
        "SELECT oproto_id, quantity, consumed, in_room FROM dbat.assembly_ingredients WHERE recipe_id = $1",
        pqxx::params{recipe.id}
    );
    
    for (const auto& row : ingredients) {
        SqlIngredient ing;
        ing.oproto_id = row[0].as<int>();
        ing.quantity = row[1].as<int>();
        ing.consumed = row[2].as<bool>();
        ing.in_room = row[3].as<bool>();
        recipe.ingredients.push_back(ing);
    }
    
    return recipe;
}

std::expected<void, std::string> sqlCheckComponents(const SqlRecipe& recipe, Character* ch, bool extract) {
    if (recipe.ingredients.empty()) {
        return std::unexpected("Recipe has no ingredients");
    }
    
    std::vector<Object*> componentObjects;
    componentObjects.resize(recipe.ingredients.size(), nullptr);
    
    for (size_t i = 0; i < recipe.ingredients.size(); ++i) {
        const auto& ing = recipe.ingredients[i];
        long lRnum = real_object(ing.oproto_id);
        
        if (lRnum < 0) {
            return std::unexpected(std::string("Invalid component vnum: ") + std::to_string(ing.oproto_id));
        }
        
        Object* obj = nullptr;
        if (ing.in_room) {
            obj = ch->location.searchObjects(lRnum);
        } else {
            obj = ch->searchInventory(lRnum);
        }
        
        if (!obj) {
            return std::unexpected(std::string("Missing component: ") + std::to_string(ing.oproto_id));
        }
        
        componentObjects[i] = obj;
    }
    
    for (size_t i = 0; i < recipe.ingredients.size(); ++i) {
        const auto& ing = recipe.ingredients[i];
        Object* obj = componentObjects[i];
        
        if (!obj) {
            continue;
        }
        
        obj->clearLocation();
        
        if (ing.consumed && extract) {
            extract_obj(obj);
        } else if (ing.in_room) {
            obj->moveToLocation(ch);
        } else {
            ch->addToInventory(obj);
        }
    }
    
    return {};
}

void sqlListAssemblies(Character* ch, int type) {
    std::string query;
    pqxx::result result;
    
    if (type == 0) {
        result = dbat::db::txn->exec(
            "SELECT r.oproto_id, r.assembly_type "
            "FROM dbat.assembly_recipes r "
            "ORDER BY r.oproto_id"
        );
    } else {
        result = dbat::db::txn->exec(
            "SELECT r.oproto_id, r.assembly_type "
            "FROM dbat.assembly_recipes r "
            "WHERE r.assembly_type = $1 "
            "ORDER BY r.oproto_id",
            pqxx::params{item_types[type]}
        );
    }
    
    if (result.empty()) {
        ch->sendText("No assemblies exist.\r\n");
        return;
    }
    
    if (type == 0) {
        ch->sendText("The following assemblies exists:\r\n");
    } else {
        ch->send_to("Only displaying [%s]\r\n", item_types[type]);
    }
    
    char szBuffer[MAX_STRING_LENGTH] = {'\0'};
    long lRnum = 0;
    
    for (const auto& row : result) {
        int oproto_id = row[0].as<int>();
        std::string assembly_type = row[1].as<std::string>();
        
        if ((lRnum = real_object(oproto_id)) < 0) {
            ch->sendText("[-----] ***RESERVED***\r\n");
        } else {
            if (type == 0 || type == static_cast<int>(obj_proto.at(lRnum)->type_flag)) {
                sprintf(szBuffer, "[%5d] %s (%s)\r\n", oproto_id,
                        obj_proto.at(lRnum)->short_description.c_str(), assembly_type.c_str());
                ch->sendText(szBuffer);
            }
        }
    }
}