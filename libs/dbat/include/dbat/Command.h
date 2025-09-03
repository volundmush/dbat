#pragma once
#include <string>
#include <string_view>
#include <span>
#include <unordered_map>
#include <stdexcept>

struct CommandData {
    CommandData() = default;
    CommandData(std::string_view command_line);
    const std::string _original;
    std::string_view cmd;
    std::string_view switch_type;
    std::string_view switch_mod;
    std::string_view argument;
    std::string_view lsargs;
    std::string_view rsargs;
    std::span<std::string_view> args, lhslist, rhslist;
    bool equals_present;
    std::unordered_map<std::string, std::string> variables;
};

class CommandError : public std::runtime_error {
public:
    explicit CommandError(const std::string& message)
        : std::runtime_error(message) {}
};

struct Character;
struct HasDgScripts;
struct script_data;

typedef void(*CommandFunc)(Character *ch, char *argument, int cmd, int subcmd, CommandData cdata);

typedef int(*SpecialFunc)(Character *ch, HasDgScripts *me, int cmd, char *argument);

#define ACMD(name) void (name)(Character *ch, char *argument, int cmd, int subcmd, CommandData cdata)
#define DECCMD(name) void (name)(Character *ch, char *argument, int cmd, int subcmd, CommandData cdata = {})
#define SPECIAL(name) int (name)(Character *ch, HasDgScripts *me, int cmd, char *argument)
