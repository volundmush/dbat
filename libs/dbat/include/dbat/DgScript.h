#pragma once
#include <memory>
#include <experimental/memory>
#include <magic_enum/magic_enum_format.hpp>
#include "DgScriptPrototype.h"
#include "HasVariables.h"
#include "HasMisc.h"
#include "SubscriptionManager.h"


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
    return fmt::format("({}) DgScript {} '{}'", magic_enum::enum_name(z.getAttachType()), z.getVnum(), z.proto->name);
}

inline std::string format_as_diagnostic(const DgScript& z) {
    return fmt::format("{}, state: {}, current_line: {}, waiting: {}, depth_stack size: {}\r\n    variables: [{}]",
                       format_as(z),
                       magic_enum::enum_name(z.state), z.current_line, z.waiting,
                       z.depth_stack.size(), fmt::join(z.variables, ", "));
}

extern SubscriptionManager<DgScript> triggerSubscriptions;