#pragma once
#include <memory>
#include <experimental/memory>
#include <functional>
#include <variant>
#include <nlohmann/json_fwd.hpp>
#include <enchantum/fmt_format.hpp>
#include "DgScriptPrototype.hpp"
#include "HasVariables.hpp"
#include "HasMisc.hpp"
#include "SubscriptionManager.hpp"


struct HasDgScripts;

using DepthType = std::tuple<ScriptLineType, int, bool, std::string>;

enum class DgScriptState : std::uint8_t {
    READY = 0,
    RUNNING = 1,
    WAITING = 2,
    PAUSED = 3,
    ERROR = 4,
    DONE = 5
};

class DgScriptError : public std::runtime_error {
public:
    explicit DgScriptError(const std::string& message)
        : std::runtime_error(message) {}
};

/* structure for triggers */
struct DgScript : public HasVariables, public HasSubscriptions, std::enable_shared_from_this<DgScript> {
    DgScript() = default;
    DgScript(const DgScriptPrototype &other);
    DgScriptPrototype* proto{};
    int getVnum() const;
    UnitType getAttachType() const;
    long getTriggerType() const;
    DgScriptState state{DgScriptState::READY}; /* current state of the script */
    std::vector<DepthType> depth_stack{};
    int current_line{};
    double waiting{0.0};    /* event to pause the trigger      */
    std::experimental::observer_ptr<HasDgScripts> owner{};

    bool active{false};
    void activate();
    void deactivate();

    int execute();
    void reset();

    bool isReady() const;
    void setWaiting(double wait, DgScriptState newState = DgScriptState::WAITING);

private:
    void error(const std::string& message);
    int toReturn{1}; // used to return from the script.
    
    void setState(DgScriptState newState);
    
    void processLine(const ScriptLine& line);

    int locateElseIfElseEnd(int startLine) const;
    int locateCaseDefaultDone(int startLine) const;
    int locateDone(ScriptLineType type, int startLine) const;
    int locateEnd(int startLine) const;

    std::string evaluateExpression(const std::string& expr);
    bool evaluateComparison(const std::string& lhs, const std::string& rhs, const std::string& op);
    void processCommand(const std::string& cmd);
    bool truthy(const std::string& value) const;
    std::string substituteVariables(const std::string& cmd);
};

inline std::string format_as(const DgScript& z) {
    return fmt::format("({}) DgScript {} '{}'", enchantum::to_string(z.getAttachType()), z.getVnum(), z.proto->name);
}

inline std::string format_as_diagnostic(const DgScript& z) {
    return fmt::format("{}, state: {}, current_line: {}, waiting: {}, depth_stack size: {}\r\n    variables: [{}]",
                       format_as(z),
                       enchantum::to_string(z.state), z.current_line, z.waiting,
                       z.depth_stack.size(), fmt::join(z.variables, ", "));
}

extern SubscriptionManager<DgScript> triggerSubscriptions;

// the kinds of return values we can have from variable substitutions.
// this is usually a string but might be a reference to a Room, Character, or Object (HasDgScripts)
// a deliberate null value is represented by an empty string.
using DgReturn = std::variant<std::string, HasDgScripts*>;

// used for things like "time" and "random" and "find" for DgScript subsitutions...
// So for instance, %time.hour% or %find.character(John)%
// the string views might be empty if not used.
using DgFunc = std::function<DgReturn(DgScript*, std::string_view field, std::string_view subfield)>;

#define DGFUNC(fname) DgReturn fname(DgScript* trig, std::string_view field, std::string_view subfield)

void to_json(nlohmann::json& j, const DgScript& t);
void from_json(const nlohmann::json& j, DgScript& t);
