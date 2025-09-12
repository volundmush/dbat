#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stdexcept>

class CommandData {
    std::shared_ptr<const std::string> _original;

    public:
    CommandData() = default;
    CommandData(std::string_view command_line);

    std::string_view original;
    std::string_view cmd;
    std::vector<std::string_view> switches;
    std::string_view switch_mod;
    std::string_view argument;
    std::vector<std::string_view> arguments;
    std::string_view lsargs;
    std::string_view rsargs;
    std::vector<std::string_view> args, lhslist, rhslist, lhscomm, rhscomm;
    bool equals_present;
    std::unordered_map<std::string, std::string> variables;
};

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
