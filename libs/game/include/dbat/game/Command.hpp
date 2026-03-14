#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include "dbat/mud/Command.hpp"

using CommandData = dbat::mud::CommandData;

class CommandError : public std::runtime_error {
public:
    explicit CommandError(const std::string& message) : std::runtime_error(message) {}
};

struct Character;
struct HasDgScripts;

typedef void(*CommandFunc)(Character *ch, char *argument, int cmd, int subcmd, CommandData cdata);

typedef int(*SpecialFunc)(Character *ch, HasDgScripts *me, int cmd, char *argument);

struct SpecialFuncStorage {
    SpecialFunc func;
    char *farg;         /* string argument for special function     */
};

#define ACMD(name) void (name)(Character *ch, char *argument, int cmd, int subcmd, CommandData cdata)
#define DECCMD(name) void (name)(Character *ch, char *argument, int cmd, int subcmd, CommandData cdata = {})
#define SPECIAL(name) int (name)(Character *ch, HasDgScripts *me, int cmd, char *argument)
